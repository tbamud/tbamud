/* Rescue quests need a living, stable captive and a living player.  A
 * captive left dying when the last guard falls can still be healed or
 * bandaged to finish the rescue. */
#include <limits.h>

#include "unity.h"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "dg_scripts.h"
#include "fight.h"
#include "handler.h"
#include "spells.h"
#include "act.h"
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
struct message_list fight_messages[MAX_MESSAGES];

/* generic_complete_quest() refers to these; the fixture quests reward no
 * gold, experience or object, so none of them is reached. */
obj_rnum real_object(obj_vnum vnum) { (void)vnum; return NOTHING; }
struct obj_data *read_object(obj_vnum nr, int type) { (void)nr; (void)type; return NULL; }
void gain_exp(struct char_data *ch, int gain) { (void)ch; (void)gain; }
int increase_gold(struct char_data *ch, int amt) { (void)ch; return amt; }

/* Combat runs without world scripts, shops, fleeing or mobile memory. */
mob_rnum real_mobile(mob_vnum vnum) { (void)vnum; return NOBODY; }
int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim)
{ (void)ch; (void)victim; return TRUE; }
int damage_mtrigger(struct char_data *ch, struct char_data *victim, int dam, int attacktype)
{ (void)ch; (void)victim; (void)attacktype; return dam; }
int death_mtrigger(struct char_data *ch, struct char_data *killer)
{ (void)ch; (void)killer; return TRUE; }
int remove_otrigger(struct obj_data *obj, struct char_data *ch)
{ (void)obj; (void)ch; return TRUE; }
void remember(struct char_data *ch, struct char_data *victim)
{ (void)ch; (void)victim; }
int do_simple_move(struct char_data *ch, int dir, int following)
{
  (void)ch; (void)dir; (void)following;
  TEST_FAIL_MESSAGE("unexpected movement during combat");
  return FALSE;
}

struct obj_data *create_obj(void)
{
  struct obj_data *obj = calloc(1, sizeof(*obj));

  TEST_ASSERT_NOT_NULL(obj);
  obj->next = object_list;
  object_list = obj;
  return obj;
}

enum { CELL, HALL };                   /* room rnums */
enum { CAPTIVE, GUARD, PET };          /* mob rnums */

#define Q_RESCUE 10   /* save the captive */

static char empty[] = "";
static struct room_data rooms[2];
static struct index_data mobs[3];
static struct aq_data quests[1];
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
  ch->next = character_list;
  character_list = ch;
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
  mob_index[nr].number++;
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
  mobs[PET].vnum = 400;
  mob_index = mobs;
  top_of_mobt = PET;

  memset(quests, 0, sizeof(quests));
  init_quest(&quests[0], Q_RESCUE, AQ_MOB_SAVE, 200);
  aquest_table = quests;
  total_quests = 1;

  num_made = 0;
}

void tearDown(void)
{
  int n;
  struct obj_data *obj;

  extract_pending_chars();
  character_list = NULL;
  combat_list = NULL;
  for (n = 0; n < num_made; n++) {
    struct char_data *ch = made[n];

    if (ch->player_specials != &dummy_mob) {
      free(ch->player_specials->saved.completed_quests);
      free(ch->player_specials);
    }
    free(ch);
  }
  while ((obj = object_list)) {
    object_list = obj->next;
    free(obj->name);
    free(obj->description);
    free(obj->short_description);
    free(obj);
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

static void test_a_player_death_does_not_complete_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *victim = new_player(CELL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  GET_POS(victim) = POS_DEAD;
  SET_BIT_AR(PLR_FLAGS(victim), PLR_NOTDEADYET);
  autoquest_trigger_check(holder, victim, NULL, AQ_MOB_SAVE);
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
}

static void test_an_npc_death_in_another_room_does_not_complete_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, HALL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  holder_sees_death(holder, guard);
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
}

static void test_a_missing_kill_victim_does_not_complete_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  autoquest_trigger_check(holder, NULL, NULL, AQ_MOB_SAVE);
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
}

static void test_a_living_pet_does_not_complete_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_pet(holder, CELL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  autoquest_trigger_check(holder, pet, NULL, AQ_MOB_SAVE);
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
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

static struct char_data *new_combat_pet(struct char_data *owner)
{
  static char name[] = "pet";
  struct char_data *pet = new_mob(PET, IN_ROOM(owner));
  struct affected_type charm;

  pet->player.name = pet->player.short_descr = name;
  GET_HIT(pet) = GET_MAX_HIT(pet) = 10;
  add_follower(pet, owner);
  new_affect(&charm);
  charm.spell = SPELL_CHARM;
  charm.duration = 10;
  SET_BIT_AR(charm.bitvector, AFF_CHARM);
  affect_to_char(pet, &charm);
  TEST_ASSERT_TRUE(AFF_FLAGGED(pet, AFF_CHARM));
  return pet;
}

static void test_killing_another_players_charmed_pet_does_not_complete_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *owner = new_player(CELL);
  struct char_data *pet = new_combat_pet(owner);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  TEST_ASSERT_TRUE(DEAD(pet));
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
}

static void test_killing_a_pet_after_the_owners_first_hit_does_not_complete_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_combat_pet(holder);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  TEST_ASSERT_EQUAL_INT(1, damage(holder, pet, 1, TYPE_HIT));
  TEST_ASSERT_FALSE(DEAD(pet));
  TEST_ASSERT_FALSE(AFF_FLAGGED(pet, AFF_CHARM));
  TEST_ASSERT_NULL(pet->master);
  assert_not_rescued(holder);

  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  TEST_ASSERT_TRUE(DEAD(pet));
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
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

static void test_a_heal_that_leaves_the_captive_dying_finishes_nothing(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *captive = new_dying_captive(CELL);

  GET_HIT(captive) = -10;
  update_pos(captive);
  give_quest(holder, Q_RESCUE);
  circle_srandom(3);  /* The first level-zero Cure Light restores 7 HP. */

  mag_points(0, holder, captive, SPELL_CURE_LIGHT, 0);
  TEST_ASSERT_EQUAL_INT(POS_INCAP, GET_POS(captive));
  assert_not_rescued(holder);
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

/* Treating minor injuries in a clear room leaves the quest open. */
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

/* The holder's own pet, left dying in a cell someone else cleared. */
static struct char_data *new_dying_pet(struct char_data *master, room_rnum room)
{
  static char name[] = "pet";
  struct char_data *pet = new_pet(master, room);

  pet->player.name = name;
  GET_MAX_HIT(pet) = 20;
  GET_HIT(pet) = -4;
  GET_POS(pet) = POS_INCAP;
  return pet;
}

/* A pet may stay with the captive, so healing one changes nothing about the
 * rescue; only the captive's own recovery can finish it. */
static void test_healing_a_dying_pet_finishes_nothing(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_dying_pet(holder, CELL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  mag_points(20, holder, pet, SPELL_CURE_CRITIC, 0);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(pet));
  assert_not_rescued(holder);
}

static void test_bandaging_a_dying_pet_finishes_nothing(void)
{
  char arg[] = "pet";
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_dying_pet(holder, CELL);

  new_mob(CAPTIVE, CELL);
  SET_SKILL(holder, SKILL_BANDAGE, 101);
  give_quest(holder, Q_RESCUE);
  do_bandage(holder, arg, 0, 0);
  TEST_ASSERT_EQUAL_INT(POS_STUNNED, GET_POS(pet));
  assert_not_rescued(holder);
}

static struct char_data *new_pending_corpse(room_rnum room)
{
  static char name[] = "guard";
  struct char_data *corpse = new_mob(GUARD, room);

  corpse->player.name = name;
  GET_MAX_HIT(corpse) = 20;
  GET_HIT(corpse) = -11;
  GET_POS(corpse) = POS_DEAD;
  kill_mob(corpse);
  return corpse;
}

static void test_healing_a_pending_corpse_finishes_nothing(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *corpse = new_pending_corpse(CELL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);

  /* A healing spell can receive an NPC still in the room during extraction. */
  mag_points(20, holder, corpse, SPELL_HEAL, 0);
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
}

static void test_bandaging_a_pending_corpse_finishes_nothing(void)
{
  char arg[] = "guard";
  struct char_data *holder = new_player(CELL);

  new_mob(CAPTIVE, CELL);
  new_pending_corpse(CELL);
  SET_SKILL(holder, SKILL_BANDAGE, 101);
  give_quest(holder, Q_RESCUE);

  do_bandage(holder, arg, 0, 0);
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
}

static void treat_the_same_captive_twice(struct char_data *holder,
                                        struct char_data *captive)
{
  char arg[] = "captive";

  GET_HIT(captive) = -4;
  update_pos(captive);
  mag_points(20, holder, captive, SPELL_CURE_CRITIC, 0);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(captive));

  GET_HIT(captive) = -4;
  update_pos(captive);
  SET_SKILL(holder, SKILL_BANDAGE, 101);
  do_bandage(holder, arg, 0, 0);
  TEST_ASSERT_EQUAL_INT(POS_STUNNED, GET_POS(captive));
}

static void test_repeated_treatment_does_not_spend_multi_target_goals(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *captive = new_dying_captive(CELL);

  QST_QUANTITY(0) = 2;
  give_quest(holder, Q_RESCUE);

  treat_the_same_captive_twice(holder, captive);
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(2, GET_QUEST_COUNTER(holder));
}

static void test_treatment_does_not_finish_a_multi_target_quest_with_one_goal_left(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);
  struct char_data *captive = new_dying_captive(CELL);

  GET_HIT(captive) = 5;
  update_pos(captive);
  QST_QUANTITY(0) = 2;
  give_quest(holder, Q_RESCUE);
  holder_sees_death(holder, guard);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));

  treat_the_same_captive_twice(holder, captive);
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
}

static void test_healing_the_captive_in_an_already_clear_room_completes_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *captive = new_dying_captive(CELL);

  give_quest(holder, Q_RESCUE);
  mag_points(20, holder, captive, SPELL_CURE_CRITIC, 0);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(captive));
  assert_rescued(holder);
}

static void test_healing_completes_a_rescue_with_a_nonpositive_quantity(void)
{
  const int quantities[] = {0, INT_MIN};
  size_t i;

  for (i = 0; i < sizeof(quantities) / sizeof(quantities[0]); i++) {
    struct char_data *holder = new_player(CELL);
    struct char_data *captive = new_dying_captive(CELL);

    QST_QUANTITY(0) = quantities[i];
    give_quest(holder, Q_RESCUE);
    TEST_ASSERT_EQUAL_INT(quantities[i], GET_QUEST_COUNTER(holder));
    mag_points(20, holder, captive, SPELL_CURE_CRITIC, 0);
    assert_rescued(holder);
  }
}

static void test_bandaging_the_captive_in_an_already_clear_room_completes_the_rescue(void)
{
  char arg[] = "captive";
  struct char_data *holder = new_player(CELL);
  struct char_data *captive = new_dying_captive(CELL);

  SET_SKILL(holder, SKILL_BANDAGE, 101);
  give_quest(holder, Q_RESCUE);
  do_bandage(holder, arg, 0, 0);
  TEST_ASSERT_EQUAL_INT(POS_STUNNED, GET_POS(captive));
  assert_rescued(holder);
}

static void test_healing_a_remote_copy_does_not_rescue_the_local_captive(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *remote_captive = new_dying_captive(HALL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);

  mag_points(20, holder, remote_captive, SPELL_CURE_CRITIC, 0);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(remote_captive));
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_rescue_completes_when_the_captive_is_left_alive);
  RUN_TEST(test_rescue_waits_while_a_captor_lives);
  RUN_TEST(test_rescue_needs_the_captive_in_the_room);
  RUN_TEST(test_a_player_death_does_not_complete_the_rescue);
  RUN_TEST(test_an_npc_death_in_another_room_does_not_complete_the_rescue);
  RUN_TEST(test_a_missing_kill_victim_does_not_complete_the_rescue);
  RUN_TEST(test_a_living_pet_does_not_complete_the_rescue);
  RUN_TEST(test_rescue_fails_when_the_captive_died_first);
  RUN_TEST(test_killing_a_copy_of_the_captive_is_no_rescue);
  RUN_TEST(test_a_dying_captive_does_not_count);
  RUN_TEST(test_a_stunned_captive_still_counts);
  RUN_TEST(test_a_players_pet_may_stay_with_the_captive);
  RUN_TEST(test_a_captors_charmed_minion_blocks_the_rescue);
  RUN_TEST(test_killing_another_players_charmed_pet_does_not_complete_the_rescue);
  RUN_TEST(test_killing_a_pet_after_the_owners_first_hit_does_not_complete_the_rescue);
  RUN_TEST(test_a_dead_holder_is_not_rewarded);
  RUN_TEST(test_healing_a_captive_left_dying_finishes_the_rescue);
  RUN_TEST(test_a_heal_that_leaves_the_captive_dying_finishes_nothing);
  RUN_TEST(test_bandaging_a_captive_left_dying_finishes_the_rescue);
  RUN_TEST(test_healing_a_captive_that_was_not_dying_finishes_nothing);
  RUN_TEST(test_healing_a_dying_pet_finishes_nothing);
  RUN_TEST(test_bandaging_a_dying_pet_finishes_nothing);
  RUN_TEST(test_healing_a_pending_corpse_finishes_nothing);
  RUN_TEST(test_bandaging_a_pending_corpse_finishes_nothing);
  RUN_TEST(test_repeated_treatment_does_not_spend_multi_target_goals);
  RUN_TEST(test_treatment_does_not_finish_a_multi_target_quest_with_one_goal_left);
  RUN_TEST(test_healing_the_captive_in_an_already_clear_room_completes_the_rescue);
  RUN_TEST(test_healing_completes_a_rescue_with_a_nonpositive_quantity);
  RUN_TEST(test_bandaging_the_captive_in_an_already_clear_room_completes_the_rescue);
  RUN_TEST(test_healing_a_remote_copy_does_not_rescue_the_local_captive);
  return UNITY_END();
}
