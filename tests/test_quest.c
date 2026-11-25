/* Rescue ("Save mob") quests complete only while the mob to be rescued is
 * alive in the holder's room with nothing but players' pets beside it, and a
 * Find-mob step completes only itself, never the next step of its chain.
 *
 * The checks are driven the way the game drives them: raw_kill() flags the
 * mob that died MOB_NOTDEADYET and passes it as vict, and char_to_room()
 * runs the rescue check when the holder walks in. */
#include "unity.h"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "quest.h"

/* Globals defined strongly in test_stubs.c */
extern FILE *logfile;
extern struct room_data *world;
extern room_rnum top_of_world;
extern struct index_data *mob_index;
extern mob_rnum top_of_mobt;

/* db.c is not linked, so the quest table lives here. */
struct aq_data *aquest_table = NULL;
qst_rnum total_quests = 0;

/* generic_complete_quest() refers to these; the fixture quests reward no
 * gold, experience or object, so none of them is reached. */
obj_rnum real_object(obj_vnum vnum) { (void)vnum; return NOTHING; }
struct obj_data *read_object(obj_vnum nr, int type) { (void)nr; (void)type; return NULL; }
void gain_exp(struct char_data *ch, int gain) { (void)ch; (void)gain; }
int increase_gold(struct char_data *ch, int amt) { (void)ch; return amt; }

enum { CELL, HALL };                   /* room rnums */
enum { CAPTIVE, GUARD, BANDIT, PET };  /* mob rnums  */

#define Q_RESCUE 10   /* save the captive */
#define Q_FIND   20   /* find a bandit, then Q_RESCUE */

static char empty[] = "";
static struct room_data rooms[2];
static struct index_data mobs[4];
static struct aq_data quests[2];
static struct char_data *made[16];
static int num_made;

static void init_quest(struct aq_data *q, qst_vnum vnum, int type, int target)
{
  q->vnum = vnum;
  q->name = q->desc = q->info = q->done = q->quit = empty;
  q->type = type;
  q->qm = NOBODY;
  q->target = target;
  q->prereq = NOTHING;
  q->value[4] = -1;  /* no time limit */
  q->value[6] = 1;   /* quantity */
  q->obj_reward = NOTHING;
  q->prev_quest = NOTHING;
  q->next_quest = NOTHING;
}

static struct char_data *new_char(void)
{
  struct char_data *ch = calloc(1, sizeof(*ch));

  TEST_ASSERT_NOT_NULL(ch);
  TEST_ASSERT_TRUE(num_made < (int)(sizeof(made) / sizeof(made[0])));
  made[num_made++] = ch;
  IN_ROOM(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  return ch;
}

/* A player with no quest yet, so placing them completes nothing. */
static struct char_data *new_player(room_rnum room)
{
  struct char_data *ch = new_char();

  ch->player_specials = calloc(1, sizeof(*ch->player_specials));
  TEST_ASSERT_NOT_NULL(ch->player_specials);
  clear_quest(ch);
  char_to_room(ch, room);
  return ch;
}

static struct char_data *new_mob(mob_rnum nr, room_rnum room)
{
  struct char_data *mob = new_char();

  SET_BIT_AR(MOB_FLAGS(mob), MOB_ISNPC);
  mob->nr = nr;
  mob->player_specials = &dummy_mob;
  char_to_room(mob, room);
  return mob;
}

static struct char_data *new_pet(struct char_data *master, room_rnum room)
{
  struct char_data *pet = new_mob(PET, room);

  SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
  pet->master = master;
  return pet;
}

static void give_quest(struct char_data *ch, qst_vnum vnum)
{
  set_quest(ch, real_quest(vnum));
}

/* What extract_char() does to a mob before raw_kill() runs the checks. */
static void kill_mob(struct char_data *mob)
{
  SET_BIT_AR(MOB_FLAGS(mob), MOB_NOTDEADYET);
}

/* The rescue check raw_kill() runs for the holder once mob has died. */
static void holder_sees_death(struct char_data *holder, struct char_data *mob)
{
  kill_mob(mob);
  autoquest_trigger_check(holder, mob, NULL, AQ_MOB_SAVE);
}

static void move_to(struct char_data *ch, room_rnum room)
{
  char_from_room(ch);
  char_to_room(ch, room);
}

static void assert_rescued(struct char_data *holder)
{
  TEST_ASSERT_EQUAL_INT(NOTHING, GET_QUEST(holder));
  TEST_ASSERT_TRUE(is_complete(holder, Q_RESCUE));
}

static void assert_not_rescued(struct char_data *holder)
{
  TEST_ASSERT_EQUAL_INT(Q_RESCUE, GET_QUEST(holder));
  TEST_ASSERT_FALSE(is_complete(holder, Q_RESCUE));
}

void setUp(void)
{
  logfile = stderr;

  memset(rooms, 0, sizeof(rooms));
  rooms[CELL].number = 3001;
  rooms[HALL].number = 3002;
  world = rooms;
  top_of_world = HALL;

  memset(mobs, 0, sizeof(mobs));
  mobs[CAPTIVE].vnum = 200;
  mobs[GUARD].vnum = 300;
  mobs[BANDIT].vnum = 100;
  mobs[PET].vnum = 400;
  mob_index = mobs;
  top_of_mobt = PET;

  memset(quests, 0, sizeof(quests));
  init_quest(&quests[0], Q_RESCUE, AQ_MOB_SAVE, 200);
  init_quest(&quests[1], Q_FIND, AQ_MOB_FIND, 100);
  quests[1].next_quest = Q_RESCUE;
  aquest_table = quests;
  total_quests = 2;

  num_made = 0;
}

void tearDown(void)
{
  int n;

  for (n = 0; n < num_made; n++) {
    struct char_data *ch = made[n];

    if (ch->player_specials != &dummy_mob) {
      free(ch->player_specials->saved.completed_quests);
      free(ch->player_specials);
    }
    free(ch);
  }
  num_made = 0;
  world = NULL;
  top_of_world = 0;
  mob_index = NULL;
  top_of_mobt = 0;
  aquest_table = NULL;
  total_quests = 0;
  logfile = NULL;
}

static void test_rescue_completes_when_the_captive_is_left_alive(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  holder_sees_death(holder, guard);
  assert_rescued(holder);
}

static void test_rescue_waits_while_a_captor_lives(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  new_mob(GUARD, CELL);
  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  holder_sees_death(holder, guard);
  assert_not_rescued(holder);
}

static void test_rescue_needs_the_captive_in_the_room(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  give_quest(holder, Q_RESCUE);
  holder_sees_death(holder, guard);
  assert_not_rescued(holder);
}

static void test_rescue_fails_when_the_captive_died_first(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  kill_mob(new_mob(CAPTIVE, CELL));
  give_quest(holder, Q_RESCUE);
  holder_sees_death(holder, guard);
  assert_not_rescued(holder);
}

static void test_killing_a_copy_of_the_captive_is_no_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *captive = new_mob(CAPTIVE, CELL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  holder_sees_death(holder, captive);
  assert_not_rescued(holder);
}

static void test_a_dying_captive_does_not_count(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  GET_POS(new_mob(CAPTIVE, CELL)) = POS_MORTALLYW;
  give_quest(holder, Q_RESCUE);
  holder_sees_death(holder, guard);
  assert_not_rescued(holder);
}

static void test_a_stunned_captive_still_counts(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  GET_POS(new_mob(CAPTIVE, CELL)) = POS_STUNNED;
  give_quest(holder, Q_RESCUE);
  holder_sees_death(holder, guard);
  assert_rescued(holder);
}

static void test_a_players_pet_may_stay_with_the_captive(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  new_mob(CAPTIVE, CELL);
  new_pet(holder, CELL);
  give_quest(holder, Q_RESCUE);
  holder_sees_death(holder, guard);
  assert_rescued(holder);
}

static void test_a_captors_charmed_minion_blocks_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  new_mob(CAPTIVE, CELL);
  new_pet(guard, CELL);
  give_quest(holder, Q_RESCUE);
  holder_sees_death(holder, guard);
  assert_not_rescued(holder);
}

static void test_a_dead_holder_is_not_rewarded(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  SET_BIT_AR(PLR_FLAGS(holder), PLR_NOTDEADYET);
  holder_sees_death(holder, guard);
  assert_not_rescued(holder);
}

static void test_walking_into_a_cleared_cell_completes_the_rescue(void)
{
  struct char_data *holder = new_player(HALL);
  struct char_data *guard = new_mob(GUARD, CELL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);

  move_to(holder, CELL);       /* the guard is still there */
  assert_not_rescued(holder);

  move_to(holder, HALL);
  char_from_room(guard);       /* it fled, and died somewhere else */
  move_to(holder, CELL);
  assert_rescued(holder);
}

static void test_a_find_step_does_not_complete_the_next_step(void)
{
  struct char_data *holder = new_player(CELL);

  new_mob(BANDIT, CELL);
  new_mob(BANDIT, CELL);
  give_quest(holder, Q_FIND);
  autoquest_trigger_check(holder, NULL, NULL, AQ_MOB_FIND);

  TEST_ASSERT_TRUE(is_complete(holder, Q_FIND));
  assert_not_rescued(holder);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_rescue_completes_when_the_captive_is_left_alive);
  RUN_TEST(test_rescue_waits_while_a_captor_lives);
  RUN_TEST(test_rescue_needs_the_captive_in_the_room);
  RUN_TEST(test_rescue_fails_when_the_captive_died_first);
  RUN_TEST(test_killing_a_copy_of_the_captive_is_no_rescue);
  RUN_TEST(test_a_dying_captive_does_not_count);
  RUN_TEST(test_a_stunned_captive_still_counts);
  RUN_TEST(test_a_players_pet_may_stay_with_the_captive);
  RUN_TEST(test_a_captors_charmed_minion_blocks_the_rescue);
  RUN_TEST(test_a_dead_holder_is_not_rewarded);
  RUN_TEST(test_walking_into_a_cleared_cell_completes_the_rescue);
  RUN_TEST(test_a_find_step_does_not_complete_the_next_step);
  return UNITY_END();
}
