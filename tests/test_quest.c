/* Rescue quests need a living, stable captive and a living player.  A
 * captive left dying when the last guard falls can still be healed or
 * bandaged to finish the rescue.  Kills, heals and bandages count for the
 * side that made them: a charmed pet's master and the whole group, if they
 * are in the room. */
#include "unity.h"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "act.h"
#include "fight.h"
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

/* make_corpse() in raw_kill() fills in the object it gets here. */
static struct obj_data *corpses[8];
static int num_corpses;

struct obj_data *create_obj(void)
{
  struct obj_data *obj = calloc(1, sizeof(*obj));

  TEST_ASSERT_NOT_NULL(obj);
  TEST_ASSERT_TRUE(num_corpses < (int)(sizeof(corpses) / sizeof(corpses[0])));
  corpses[num_corpses++] = obj;
  return obj;
}

/* damage() in fight.c, which only a failed bandage calls, reaches these.  No
 * test makes a bandage fail, and real_mobile() is the first of them damage()
 * calls, so it fails the test if one ever does. */
struct message_list fight_messages[MAX_MESSAGES];
mob_rnum real_mobile(mob_vnum vnum)
{
  (void)vnum;
  TEST_FAIL_MESSAGE("damage() reached");
  return NOBODY;
}
int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim)
{ (void)ch; (void)victim; return TRUE; }
int damage_mtrigger(struct char_data *ch, struct char_data *victim, int dam, int attacktype)
{ (void)ch; (void)victim; (void)attacktype; return dam; }
void remember(struct char_data *ch, struct char_data *victim) { (void)ch; (void)victim; }
int do_simple_move(struct char_data *ch, int dir, int following)
{ (void)ch; (void)dir; (void)following; return 0; }

/* mag_groups() can also cast group armor and group recall, which reach these;
 * the tests cast only group heal. */
byte saving_throws(int class_num, int type, int level)
{ (void)class_num; (void)type; (void)level; return 0; }
ASPELL(spell_recall) { (void)level; (void)ch; (void)victim; (void)obj; }

enum { CELL, HALL };                   /* room rnums */
enum { CAPTIVE, GUARD, PET };          /* mob rnums */

#define Q_RESCUE 10   /* save the captive */
#define Q_CLEAR  30   /* clear the cell */

static char empty[] = "";
static struct room_data rooms[2];
static struct index_data mobs[3];
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
  mob->player.short_descr = empty;  /* make_corpse() names the corpse */
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

static void group_up(struct char_data *leader, struct char_data *member)
{
  join_group(member, GROUP(leader) ? GROUP(leader) : create_group(leader));
}

static void move_to(struct char_data *ch, room_rnum room)
{
  char_from_room(ch);
  char_to_room(ch, room);
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

static void assert_cleared(struct char_data *holder)
{
  TEST_ASSERT_EQUAL_INT(NOTHING, GET_QUEST(holder));
  TEST_ASSERT_TRUE(is_complete(holder, Q_CLEAR));
}

static void assert_not_cleared(struct char_data *holder)
{
  TEST_ASSERT_EQUAL_INT(Q_CLEAR, GET_QUEST(holder));
  TEST_ASSERT_FALSE(is_complete(holder, Q_CLEAR));
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
  mobs[PET].vnum = 400;
  mob_index = mobs;
  top_of_mobt = PET;

  memset(quests, 0, sizeof(quests));
  init_quest(&quests[0], Q_RESCUE, AQ_MOB_SAVE, 200);
  init_quest(&quests[1], Q_CLEAR, AQ_ROOM_CLEAR, 3001);
  aquest_table = quests;
  total_quests = 2;

  group_list = create_list();
  num_made = 0;
  num_corpses = 0;
}

void tearDown(void)
{
  int n;

  for (n = 0; n < num_made; n++)
    leave_group(made[n]);
  free_list(group_list);

  for (n = 0; n < num_corpses; n++) {
    free(corpses[n]->name);
    free(corpses[n]->description);
    free(corpses[n]->short_description);
    free(corpses[n]);
  }
  num_corpses = 0;

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

  new_mob(CAPTIVE, HALL);
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

/* A captive left incapacitated, as by an earthquake that also killed the
 * last guard. */
static struct char_data *new_dying_captive(room_rnum room)
{
  static char name[] = "captive";
  struct char_data *captive = new_mob(CAPTIVE, room);

  captive->player.name = name;
  GET_MAX_HIT(captive) = 20;
  GET_HIT(captive) = -4;
  GET_POS(captive) = POS_INCAP;
  return captive;
}

static void test_healing_a_captive_left_dying_finishes_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);
  struct char_data *captive = new_dying_captive(CELL);

  give_quest(holder, Q_RESCUE);
  holder_sees_death(holder, guard);
  assert_not_rescued(holder);

  /* cure critic heals at least 11, so the captive ends up standing. */
  mag_points(20, holder, captive, SPELL_CURE_CRITIC, 0);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(captive));
  assert_rescued(holder);
}

static void test_bandaging_a_captive_left_dying_finishes_the_rescue(void)
{
  char arg[] = "captive";
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);
  struct char_data *captive = new_dying_captive(CELL);

  /* The roll is rand_number(1, 101), so 101 never fails. */
  SET_SKILL(holder, SKILL_BANDAGE, 101);
  give_quest(holder, Q_RESCUE);
  holder_sees_death(holder, guard);
  assert_not_rescued(holder);

  do_bandage(holder, arg, 0, 0);
  TEST_ASSERT_EQUAL_INT(POS_STUNNED, GET_POS(captive));
  assert_rescued(holder);
}

/* Healing only finishes a rescue the captive's dying held up; healing a
 * captive in a cell someone else cleared does not. */
static void test_healing_a_captive_that_was_not_dying_finishes_nothing(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *captive = new_mob(CAPTIVE, CELL);

  GET_MAX_HIT(captive) = 20;
  GET_HIT(captive) = 5;
  give_quest(holder, Q_RESCUE);
  mag_points(20, holder, captive, SPELL_CURE_CRITIC, 0);
  assert_not_rescued(holder);
}

/* From here on the checks run where the game runs them: raw_kill(),
 * mag_points() and do_bandage(). */

static void test_a_pets_kill_completes_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_pet(holder, CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  raw_kill(guard, pet);
  assert_rescued(holder);
}

static void test_a_groupmates_kill_completes_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  new_mob(CAPTIVE, CELL);
  group_up(holder, mate);
  give_quest(holder, Q_RESCUE);
  raw_kill(guard, mate);
  assert_rescued(holder);
}

static void test_a_groupmates_pets_kill_completes_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(HALL);  /* sent the pet in alone */
  struct char_data *pet = new_pet(mate, CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  new_mob(CAPTIVE, CELL);
  group_up(holder, mate);
  give_quest(holder, Q_RESCUE);
  raw_kill(guard, pet);
  assert_rescued(holder);
}

static void test_a_pets_kill_clears_the_room(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_pet(holder, CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  give_quest(holder, Q_CLEAR);
  raw_kill(guard, pet);
  assert_cleared(holder);
}

static void test_a_groupmates_kill_clears_the_room(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  group_up(holder, mate);
  give_quest(holder, Q_CLEAR);
  raw_kill(guard, mate);
  assert_cleared(holder);
}

static void test_a_kill_in_another_room_does_not_count(void)
{
  struct char_data *holder = new_player(HALL);
  struct char_data *mate = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  new_mob(CAPTIVE, HALL);  /* alone with the holder, but nobody killed here */
  group_up(holder, mate);
  give_quest(holder, Q_RESCUE);
  raw_kill(guard, mate);
  assert_not_rescued(holder);
}

static void test_a_cell_someone_else_cleared_stays_open(void)
{
  struct char_data *holder = new_player(HALL);
  struct char_data *stranger = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  raw_kill(guard, stranger);
  assert_not_rescued(holder);

  move_to(holder, CELL);
  assert_not_rescued(holder);
}

static void test_a_dead_groupmate_is_not_credited(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  group_up(holder, mate);
  give_quest(holder, Q_CLEAR);
  SET_BIT_AR(PLR_FLAGS(holder), PLR_NOTDEADYET);
  raw_kill(guard, mate);
  assert_not_cleared(holder);
}

static void test_a_pet_dying_of_its_wounds_credits_nobody(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_pet(holder, CELL);

  new_mob(CAPTIVE, CELL);  /* someone else killed the guards */
  give_quest(holder, Q_RESCUE);
  raw_kill(pet, pet);
  assert_not_rescued(holder);
}

static void test_a_groupmates_heal_finishes_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *cleric = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);
  struct char_data *captive = new_dying_captive(CELL);

  group_up(holder, cleric);
  give_quest(holder, Q_RESCUE);
  raw_kill(guard, holder);
  assert_not_rescued(holder);

  mag_points(20, cleric, captive, SPELL_CURE_CRITIC, 0);
  assert_rescued(holder);
}

static void test_a_groupmates_bandage_finishes_the_rescue(void)
{
  char arg[] = "captive";
  struct char_data *holder = new_player(CELL);
  struct char_data *medic = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  new_dying_captive(CELL);
  SET_SKILL(medic, SKILL_BANDAGE, 101);
  group_up(holder, medic);
  give_quest(holder, Q_RESCUE);
  raw_kill(guard, holder);
  assert_not_rescued(holder);

  do_bandage(medic, arg, 0, 0);
  assert_rescued(holder);
}

/* Group heal walks the group with simple_list() and heals each member in
 * turn.  The rescue check that healing a dying member runs walks the same
 * group, and must leave group heal's place in it alone. */
static void test_a_group_heal_heals_a_dying_pet_once(void)
{
  struct char_data *cleric = new_player(CELL);
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_pet(holder, CELL);

  GET_MAX_HIT(cleric) = GET_HIT(cleric) = 100;
  GET_MAX_HIT(holder) = GET_HIT(holder) = 100;
  GET_MAX_HIT(pet) = 1000;
  GET_HIT(pet) = -4;
  GET_POS(pet) = POS_INCAP;
  group_up(cleric, holder);
  group_up(cleric, pet);

  mag_groups(30, cleric, SPELL_GROUP_HEAL, 0);
  TEST_ASSERT_TRUE(GET_HIT(pet) <= -4 + 124);  /* one heal is 100 + 3d8 */
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
  RUN_TEST(test_healing_a_captive_left_dying_finishes_the_rescue);
  RUN_TEST(test_bandaging_a_captive_left_dying_finishes_the_rescue);
  RUN_TEST(test_healing_a_captive_that_was_not_dying_finishes_nothing);
  RUN_TEST(test_a_pets_kill_completes_the_rescue);
  RUN_TEST(test_a_groupmates_kill_completes_the_rescue);
  RUN_TEST(test_a_groupmates_pets_kill_completes_the_rescue);
  RUN_TEST(test_a_pets_kill_clears_the_room);
  RUN_TEST(test_a_groupmates_kill_clears_the_room);
  RUN_TEST(test_a_kill_in_another_room_does_not_count);
  RUN_TEST(test_a_cell_someone_else_cleared_stays_open);
  RUN_TEST(test_a_dead_groupmate_is_not_credited);
  RUN_TEST(test_a_pet_dying_of_its_wounds_credits_nobody);
  RUN_TEST(test_a_groupmates_heal_finishes_the_rescue);
  RUN_TEST(test_a_groupmates_bandage_finishes_the_rescue);
  RUN_TEST(test_a_group_heal_heals_a_dying_pet_once);
  return UNITY_END();
}
