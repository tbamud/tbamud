/* Rescue quests need a living, stable captive and a living player.  A
 * captive left dying when the last guard falls can still be healed or
 * bandaged to finish the rescue.  Kills, heals and bandages count for the
 * side that made them: a charmed pet's master and the whole group, if they
 * are in the room. */
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
struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];
byte saving_throws(int class_num, int type, int level)
{ (void)class_num; (void)type; (void)level; return 100; }

/* generic_complete_quest() refers to these; the fixture quests reward no
 * gold, experience or object, so none of them is reached. */
obj_rnum real_object(obj_vnum vnum) { (void)vnum; return NOTHING; }
struct obj_data *read_object(obj_vnum nr, int type) { (void)nr; (void)type; return NULL; }
void gain_exp(struct char_data *ch, int gain) { (void)ch; (void)gain; }
int increase_gold(struct char_data *ch, int amt) { (void)ch; return amt; }

/* The fixture controls scripts and movement, and stubs shops and mobile memory. */
mob_rnum real_mobile(mob_vnum vnum) { (void)vnum; return NOBODY; }
int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim)
{ (void)ch; (void)victim; return TRUE; }
int damage_mtrigger(struct char_data *ch, struct char_data *victim, int dam, int attacktype)
{ (void)ch; (void)victim; (void)attacktype; return dam; }
static room_rnum death_script_room;
static room_rnum death_script_killer_room;
static void (*death_script)(struct char_data *victim);
int death_mtrigger(struct char_data *ch, struct char_data *killer)
{
  if (death_script)
    death_script(ch);
  if (death_script_room != NOWHERE) {
    char_from_room(ch);
    char_to_room(ch, death_script_room);
  }
  if (death_script_killer_room != NOWHERE) {
    char_from_room(killer);
    char_to_room(killer, death_script_killer_room);
  }
  return TRUE;
}
int remove_otrigger(struct obj_data *obj, struct char_data *ch)
{ (void)obj; (void)ch; return TRUE; }
void remember(struct char_data *ch, struct char_data *victim)
{ (void)ch; (void)victim; }

static struct char_data *fleeing_mob;
int do_simple_move(struct char_data *ch, int dir, int following)
{
  room_rnum destination;

  (void)following;
  TEST_ASSERT_EQUAL_PTR_MESSAGE(fleeing_mob, ch, "unexpected movement during combat");
  destination = EXIT(ch, dir)->to_room;
  char_from_room(ch);
  char_to_room(ch, destination);
  return TRUE;
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

#define Q_KILL    9   /* kill the guard */
#define Q_RESCUE 10   /* save the captive */
#define Q_CLEAR  11   /* clear the target room */

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
  death_script_room = NOWHERE;
  death_script_killer_room = NOWHERE;
  death_script = NULL;
  fleeing_mob = NULL;

  memset(rooms, 0, sizeof(rooms));
  rooms[CELL].number = 3001;
  rooms[CELL].name = "cell";
  rooms[HALL].number = 3002;
  rooms[HALL].name = "hall";
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

  group_list = create_list();
  num_made = 0;
}

void tearDown(void)
{
  int n;
  struct obj_data *obj;

  for (n = 0; n < num_made; n++)
    leave_group(made[n]);
  free_list(group_list);
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
  if (logfile && logfile != stderr)
    fclose(logfile);
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

static void kill_cell_guard(struct char_data *holder)
{
  struct char_data *guard = new_mob(GUARD, CELL);
  guard->player.name = guard->player.short_descr = "guard";
  GET_HIT(guard) = GET_MAX_HIT(guard) = 10;
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, guard, 30, TYPE_HIT));
  TEST_ASSERT_TRUE(DEAD(guard));
  TEST_ASSERT_EQUAL_INT(death_script_room == NOWHERE ? CELL : death_script_room, IN_ROOM(guard));
}

static void test_moving_the_dying_guard_preserves_a_local_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);

  death_script_room = HALL;
  kill_cell_guard(holder);
  TEST_ASSERT_EQUAL_INT(CELL, IN_ROOM(holder));
  assert_rescued(holder);
}

static void test_death_script_cannot_transfer_a_rescue_to_its_destination(void)
{
  struct char_data *holder = new_player(CELL);
  new_mob(CAPTIVE, HALL);
  give_quest(holder, Q_RESCUE);
  death_script_room = HALL;
  death_script_killer_room = HALL;

  kill_cell_guard(holder);
  TEST_ASSERT_EQUAL_INT(HALL, IN_ROOM(holder));
  assert_not_rescued(holder);
}

static void give_room_clear_quest(struct char_data *holder, room_rnum target)
{
  init_quest(&quests[0], Q_CLEAR, AQ_ROOM_CLEAR, GET_ROOM_VNUM(target));
  give_quest(holder, Q_CLEAR);
}

static void test_local_kill_completes_a_room_clear(void)
{
  struct char_data *holder = new_player(CELL);
  give_room_clear_quest(holder, CELL);

  kill_cell_guard(holder);
  TEST_ASSERT_TRUE(is_complete(holder, Q_CLEAR));
}

static void test_moving_only_the_dying_guard_preserves_a_room_clear(void)
{
  struct char_data *holder = new_player(CELL);
  give_room_clear_quest(holder, CELL);
  death_script_room = HALL;

  kill_cell_guard(holder);
  TEST_ASSERT_EQUAL_INT(CELL, IN_ROOM(holder));
  TEST_ASSERT_TRUE(is_complete(holder, Q_CLEAR));
}

static void test_moving_only_the_killer_to_an_empty_target_room_gives_no_clear(void)
{
  struct char_data *holder = new_player(CELL);
  give_room_clear_quest(holder, HALL);
  death_script_killer_room = HALL;

  kill_cell_guard(holder);
  TEST_ASSERT_EQUAL_INT(HALL, IN_ROOM(holder));
  TEST_ASSERT_EQUAL_INT(Q_CLEAR, GET_QUEST(holder));
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
}

static void test_killer_brought_to_the_death_room_by_a_script_gets_no_clear(void)
{
  struct char_data *holder = new_player(HALL);
  give_room_clear_quest(holder, CELL);
  death_script_killer_room = CELL;

  kill_cell_guard(holder);
  TEST_ASSERT_EQUAL_INT(CELL, IN_ROOM(holder));
  TEST_ASSERT_EQUAL_INT(Q_CLEAR, GET_QUEST(holder));
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

static void test_rescue_kill_check_rejects_a_dead_charmed_pet(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_pet(holder, CELL);
  struct char_data *captive = new_mob(CAPTIVE, CELL);

  GET_HIT(captive) = GET_MAX_HIT(captive) = 10;
  give_quest(holder, Q_RESCUE);
  kill_mob(pet);
  TEST_ASSERT_TRUE(is_player_pet(pet));
  TEST_ASSERT_FALSE(pet->mob_specials.fought_as_player_pet);
  check_rescue_on_kill(holder, pet, CELL);
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
}

static void test_rescue_trigger_rejects_a_dead_former_pet(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *former_pet = new_mob(PET, CELL);
  struct char_data *captive = new_mob(CAPTIVE, CELL);

  GET_HIT(captive) = GET_MAX_HIT(captive) = 10;
  give_quest(holder, Q_RESCUE);
  former_pet->mob_specials.fought_as_player_pet = TRUE;
  kill_mob(former_pet);
  TEST_ASSERT_FALSE(is_player_pet(former_pet));
  autoquest_trigger_check(holder, former_pet, NULL, AQ_MOB_SAVE);
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
  /* Death removed the charm. The public trigger can still see the victim. */
  TEST_ASSERT_FALSE(is_player_pet(pet));
  autoquest_trigger_check(holder, pet, NULL, AQ_MOB_SAVE);
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

static void test_a_former_pet_counts_in_a_later_fight(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *former_pet = new_combat_pet(owner);
  struct char_data *holder = new_player(CELL);

  new_mob(CAPTIVE, CELL);
  TEST_ASSERT_EQUAL_INT(1, damage(owner, former_pet, 1, TYPE_HIT));
  /* A successful flee moves the owner and disengages their opponent. */
  char_from_room(owner);
  char_to_room(owner, HALL);
  stop_fighting(former_pet);

  give_quest(holder, Q_RESCUE);
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, former_pet, 30, TYPE_HIT));
  assert_rescued(holder);
}

static void test_rescuing_a_former_pets_owner_does_not_make_its_death_count(void)
{
  char argument[] = "owner";
  struct char_data *owner = new_player(CELL);
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_combat_pet(owner);
  struct char_data *captive = new_mob(CAPTIVE, CELL);

  owner->player.name = "owner";
  holder->player.name = "ally";
  captive->player.name = captive->player.short_descr = "captive";
  GET_HIT(captive) = GET_MAX_HIT(captive) = 10;
  SET_SKILL(holder, SKILL_RESCUE, 101);
  give_quest(holder, Q_RESCUE);

  TEST_ASSERT_EQUAL_INT(1, damage(owner, pet, 1, TYPE_HIT));
  TEST_ASSERT_NULL(pet->master);
  TEST_ASSERT_EQUAL_PTR(owner, FIGHTING(pet));
  do_rescue(holder, argument, 0, 0);
  TEST_ASSERT_NULL(FIGHTING(owner));
  TEST_ASSERT_EQUAL_PTR(pet, FIGHTING(holder));
  TEST_ASSERT_EQUAL_PTR(holder, FIGHTING(pet));

  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
}

static void test_a_downed_former_pet_still_being_attacked_does_not_count(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_combat_pet(holder);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  TEST_ASSERT_EQUAL_INT(14, damage(holder, pet, 14, TYPE_HIT));
  TEST_ASSERT_NULL(FIGHTING(pet));
  TEST_ASSERT_EQUAL_PTR(pet, FIGHTING(holder));
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  assert_not_rescued(holder);
}

static struct char_data *down_pet_while_fighting_another_mob(struct char_data *owner, int hit)
{
  struct char_data *pet = new_combat_pet(owner);
  /* Another player's charmed pet can remain when the room is clear. */
  struct char_data *opponent = new_pet(new_player(CELL), CELL);
  opponent->player.name = opponent->player.short_descr = "opponent";
  GET_HIT(opponent) = GET_MAX_HIT(opponent) = 10;

  TEST_ASSERT_EQUAL_INT(1, damage(owner, opponent, 1, TYPE_HIT));
  TEST_ASSERT_EQUAL_INT(hit, damage(owner, pet, hit, TYPE_HIT));
  TEST_ASSERT_EQUAL_PTR(opponent, FIGHTING(owner));
  TEST_ASSERT_TRUE(is_player_pet(opponent));
  TEST_ASSERT_NULL(FIGHTING(pet));
  TEST_ASSERT_NULL(pet->master);
  return pet;
}

static void test_finishing_a_stunned_pet_while_fighting_another_mob_does_not_count(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet;

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  pet = down_pet_while_fighting_another_mob(holder, 11);
  TEST_ASSERT_EQUAL_INT(POS_STUNNED, GET_POS(pet));
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
}

static void test_returning_to_finish_an_incapacitated_pet_does_not_count(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_combat_pet(holder);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  TEST_ASSERT_EQUAL_INT(14, damage(holder, pet, 14, TYPE_HIT));
  char_from_room(holder);
  char_to_room(holder, HALL);
  char_from_room(holder);
  char_to_room(holder, CELL);
  TEST_ASSERT_EQUAL_INT(POS_INCAP, GET_POS(pet));
  TEST_ASSERT_NULL(FIGHTING(holder));
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
}

static void test_recovered_idle_former_pet_counts_in_a_new_fight(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *pet = down_pet_while_fighting_another_mob(owner, 14);
  struct char_data *holder = new_player(CELL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  TEST_ASSERT_EQUAL_INT(POS_INCAP, GET_POS(pet));
  mag_points(20, holder, pet, SPELL_HEAL, 0);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(pet));
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  assert_rescued(holder);
}

static void test_healing_a_pet_with_an_active_attacker_preserves_the_fight(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *healer = new_player(CELL);
  struct char_data *pet = new_combat_pet(holder);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  TEST_ASSERT_EQUAL_INT(14, damage(holder, pet, 14, TYPE_HIT));
  mag_points(20, healer, pet, SPELL_HEAL, 0);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(pet));
  TEST_ASSERT_EQUAL_PTR(pet, FIGHTING(holder));
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  assert_not_rescued(holder);
}

/* mtransform loads a fresh prototype before copying the ongoing fight. */
struct char_data *read_mobile(mob_vnum vnum, int type)
{
  struct char_data *mob;

  TEST_ASSERT_EQUAL_INT(VIRTUAL, type);
  TEST_ASSERT_EQUAL_INT(mob_index[GUARD].vnum, vnum);
  mob = new_char();
  SET_BIT_AR(MOB_FLAGS(mob), MOB_ISNPC);
  mob->nr = GUARD;
  mob->player.name = mob->player.short_descr = "guard";
  mob->player_specials = &dummy_mob;
  mob_index[GUARD].number++;
  return mob;
}

void script_vlog(const char *format, va_list args)
{
  (void)format; (void)args;
  TEST_FAIL_MESSAGE("unexpected script error");
}

static void test_a_former_pet_transformed_during_the_fight_does_not_count(void)
{
  char argument[] = "300";
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_combat_pet(holder);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  TEST_ASSERT_EQUAL_INT(1, damage(holder, pet, 1, TYPE_HIT));
  do_mtransform(pet, argument, 0, 0);
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  assert_not_rescued(holder);
  free(pet->player.name);
  free(pet->player.short_descr);
}

static void test_a_bashed_pet_stays_stunned_until_it_recovers(void)
{
  char target[] = "pet";
  struct char_data *owner = new_player(CELL);
  struct char_data *holder = new_player(CELL);
  struct char_data *opponent = new_pet(holder, CELL);
  struct char_data *pet = new_combat_pet(owner);
  struct obj_data *weapon = create_obj();

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  GET_HIT(pet) = 1;
  set_fighting(owner, opponent);
  IN_ROOM(weapon) = NOWHERE;
  GET_OBJ_TYPE(weapon) = ITEM_WEAPON;
  equip_char(owner, weapon, WEAR_WIELD);
  SET_SKILL(owner, SKILL_BASH, 101);
  do_bash(owner, target, 0, 0);
  TEST_ASSERT_EQUAL_PTR(weapon, unequip_char(owner, WEAR_WIELD));

  TEST_ASSERT_EQUAL_PTR(opponent, FIGHTING(owner));
  TEST_ASSERT_NULL(FIGHTING(pet));
  TEST_ASSERT_EQUAL_INT(0, GET_HIT(pet));
  TEST_ASSERT_EQUAL_INT(POS_STUNNED, GET_POS(pet));
  TEST_ASSERT_TRUE(pet->mob_specials.fought_as_player_pet);
  mag_points(20, holder, pet, SPELL_HEAL, 0);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(pet));
  assert_not_rescued(holder);
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  assert_rescued(holder);
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

static struct char_data *earthquake_target(mob_rnum nr, int hit_points)
{
  struct char_data *mob = new_mob(nr, CELL);

  mob->player.name = mob->player.short_descr = nr == CAPTIVE ? "captive" : "guard";
  GET_HIT(mob) = GET_MAX_HIT(mob) = hit_points;
  return mob;
}

static void test_earthquake_kills_guard_before_captive(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *captive = earthquake_target(CAPTIVE, 1);
  struct char_data *guard = earthquake_target(GUARD, 1);

  /* Each new arrival is added at the head of the room list. */
  give_quest(holder, Q_RESCUE);
  mag_areas(30, holder, SPELL_EARTHQUAKE, 1);
  TEST_ASSERT_TRUE(DEAD(guard));
  TEST_ASSERT_TRUE(DEAD(captive));
  assert_not_rescued(holder);
}

static void test_earthquake_kills_captive_before_guard(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = earthquake_target(GUARD, 1);
  struct char_data *captive = earthquake_target(CAPTIVE, 1);

  give_quest(holder, Q_RESCUE);
  mag_areas(30, holder, SPELL_EARTHQUAKE, 1);
  TEST_ASSERT_TRUE(DEAD(captive));
  TEST_ASSERT_TRUE(DEAD(guard));
  assert_not_rescued(holder);
}

static void load_replacement_captive(struct char_data *victim)
{
  if (GET_MOB_RNUM(victim) == CAPTIVE)
    earthquake_target(CAPTIVE, 100);
}

static void test_a_replacement_captive_cannot_survive_for_the_original(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *captive = earthquake_target(CAPTIVE, 1);

  earthquake_target(GUARD, 1);
  give_quest(holder, Q_RESCUE);
  death_script = load_replacement_captive;
  mag_areas(30, holder, SPELL_EARTHQUAKE, 1);
  TEST_ASSERT_TRUE(DEAD(captive));
  TEST_ASSERT_EQUAL_INT(100, GET_HIT(world[CELL].people));
  assert_not_rescued(holder);
}

static void test_a_replacement_loaded_before_the_guard_dies_cannot_count(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = earthquake_target(GUARD, 1);
  struct char_data *captive = earthquake_target(CAPTIVE, 1);

  give_quest(holder, Q_RESCUE);
  death_script = load_replacement_captive;
  mag_areas(30, holder, SPELL_EARTHQUAKE, 1);
  TEST_ASSERT_TRUE(DEAD(guard));
  TEST_ASSERT_TRUE(DEAD(captive));
  TEST_ASSERT_EQUAL_INT(100, GET_HIT(world[CELL].people));
  assert_not_rescued(holder);
}

static void test_one_earthquake_advances_one_goal_when_the_captive_survives(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *first_captive = earthquake_target(CAPTIVE, 100);
  struct char_data *second_captive = earthquake_target(CAPTIVE, 100);
  struct char_data *dead_captive = earthquake_target(CAPTIVE, 1);
  struct char_data *first = earthquake_target(GUARD, 1);
  struct char_data *second = earthquake_target(GUARD, 1);

  QST_QUANTITY(0) = 2;
  give_quest(holder, Q_RESCUE);
  mag_areas(30, holder, SPELL_EARTHQUAKE, 1);
  TEST_ASSERT_TRUE(DEAD(first));
  TEST_ASSERT_TRUE(DEAD(second));
  TEST_ASSERT_TRUE(DEAD(dead_captive));
  TEST_ASSERT_TRUE(GET_HIT(first_captive) > 0);
  TEST_ASSERT_TRUE(GET_HIT(second_captive) > 0);
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
}

static void test_killing_a_guard_completes_a_legacy_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_mob(GUARD, CELL);

  new_mob(CAPTIVE, CELL);
  QST_QUANTITY(0) = INT_MIN;
  give_quest(holder, Q_RESCUE);
  holder_sees_death(holder, guard);
  assert_rescued(holder);
}

static void test_visiting_a_room_completes_a_quest_with_a_nonpositive_count(void)
{
  const int quantities[] = {INT_MIN, -1, 0};
  size_t i;

  init_quest(&quests[0], 11, AQ_ROOM_FIND, rooms[CELL].number);
  QST_POINTS(0) = 7;
  for (i = 0; i < sizeof(quantities) / sizeof(quantities[0]); i++) {
    struct char_data *holder = new_player(HALL);

    QST_QUANTITY(0) = quantities[i];
    give_quest(holder, QST_NUM(0));
    char_from_room(holder);
    char_to_room(holder, CELL);
    TEST_ASSERT_EQUAL_INT(NOTHING, GET_QUEST(holder));
    TEST_ASSERT_EQUAL_INT(0, GET_QUEST_COUNTER(holder));
    TEST_ASSERT_TRUE(is_complete(holder, QST_NUM(0)));
    TEST_ASSERT_EQUAL_INT(7, GET_QUESTPOINTS(holder));
  }
}

static void test_room_quest_rewards_only_the_last_goal(void)
{
  struct char_data *holder = new_player(CELL);

  init_quest(&quests[0], 11, AQ_ROOM_FIND, rooms[CELL].number);
  QST_QUANTITY(0) = 2;
  QST_POINTS(0) = 7;
  give_quest(holder, QST_NUM(0));
  autoquest_trigger_check(holder, NULL, NULL, AQ_ROOM_FIND);
  TEST_ASSERT_EQUAL_INT(QST_NUM(0), GET_QUEST(holder));
  TEST_ASSERT_EQUAL_INT(1, GET_QUEST_COUNTER(holder));
  TEST_ASSERT_EQUAL_INT(0, GET_QUESTPOINTS(holder));
  autoquest_trigger_check(holder, NULL, NULL, AQ_ROOM_FIND);
  TEST_ASSERT_TRUE(is_complete(holder, QST_NUM(0)));
  TEST_ASSERT_EQUAL_INT(7, GET_QUESTPOINTS(holder));
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

static void test_treatment_preserves_multiple_goals_after_the_quest_quantity_is_lowered(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *captive = new_dying_captive(CELL);

  QST_QUANTITY(0) = 2;
  give_quest(holder, Q_RESCUE);
  QST_QUANTITY(0) = 1;

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

/* From here on, kills, heals and bandages count for the side that made
 * them: a charmed pet's master and the whole group, if they are in the
 * room. */

/* mag_groups() can also cast group recall; the tests cast only group heal. */
ASPELL(spell_recall) { (void)level; (void)ch; (void)victim; (void)obj; }

static void group_up(struct char_data *leader, struct char_data *member)
{
  join_group(member, GROUP(leader) ? GROUP(leader) : create_group(leader));
}

static void move_to(struct char_data *ch, room_rnum room)
{
  char_from_room(ch);
  char_to_room(ch, room);
}

static struct char_data *new_guard(room_rnum room)
{
  static char name[] = "guard";
  struct char_data *guard = new_mob(GUARD, room);

  guard->player.name = guard->player.short_descr = name;
  return guard;
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

/* A death script that moves one player. */
static struct char_data *script_moves;
static room_rnum script_moves_to;
static void move_someone(struct char_data *victim)
{
  (void)victim;
  move_to(script_moves, script_moves_to);
}

static void test_a_pets_kill_completes_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_pet(holder, CELL);
  struct char_data *guard = new_guard(CELL);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  raw_kill(guard, pet);
  assert_rescued(holder);
}

static void test_a_groupmates_kill_completes_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *guard = new_guard(CELL);

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
  struct char_data *guard = new_guard(CELL);

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
  struct char_data *guard = new_guard(CELL);

  give_room_clear_quest(holder, CELL);
  raw_kill(guard, pet);
  assert_cleared(holder);  /* the pet is still in the room */
}

static void test_a_groupmates_kill_clears_the_room(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *guard = new_guard(CELL);

  group_up(holder, mate);
  give_room_clear_quest(holder, CELL);
  raw_kill(guard, mate);
  assert_cleared(holder);
}

/* Killing the guard finishes Q_KILL, which unlocks the given stage. */
static void chain_kill_stage(qst_vnum next, int type, int target)
{
  init_quest(&quests[0], Q_KILL, AQ_MOB_KILL, mobs[GUARD].vnum);
  init_quest(&quests[1], next, type, target);
  quests[0].next_quest = next;
  total_quests = 2;
}

/* As on master, the death that finishes a kill stage also counts toward the
 * stage it unlocks.  Otherwise killing the room's last mob would leave a
 * Room clear or rescue that nothing could finish until the zone reset. */
static void test_a_kill_also_clears_the_room_for_the_stage_it_unlocks(void)
{
  struct char_data *holder = new_player(CELL);

  chain_kill_stage(Q_CLEAR, AQ_ROOM_CLEAR, rooms[CELL].number);
  give_quest(holder, Q_KILL);
  kill_cell_guard(holder);
  TEST_ASSERT_TRUE(is_complete(holder, Q_KILL));
  assert_cleared(holder);
}

static void test_a_kill_also_finishes_the_rescue_it_unlocks(void)
{
  struct char_data *holder = new_player(CELL);

  new_mob(CAPTIVE, CELL);
  chain_kill_stage(Q_RESCUE, AQ_MOB_SAVE, mobs[CAPTIVE].vnum);
  give_quest(holder, Q_KILL);
  kill_cell_guard(holder);
  TEST_ASSERT_TRUE(is_complete(holder, Q_KILL));
  assert_rescued(holder);
}

static void test_a_groupmates_kill_also_clears_the_room_for_the_stage_it_unlocks(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);

  chain_kill_stage(Q_CLEAR, AQ_ROOM_CLEAR, rooms[CELL].number);
  group_up(holder, mate);
  give_quest(holder, Q_KILL);
  kill_cell_guard(mate);
  TEST_ASSERT_TRUE(is_complete(holder, Q_KILL));
  assert_cleared(holder);
}

static void test_rescuing_a_charmed_captive_does_not_also_complete_the_next_room_clear(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);

  new_pet(mate, CELL);  /* The captive may also stay for the next room clear. */
  init_quest(&quests[0], Q_RESCUE, AQ_MOB_SAVE, mobs[PET].vnum);
  init_quest(&quests[1], Q_CLEAR, AQ_ROOM_CLEAR, rooms[CELL].number);
  quests[0].next_quest = Q_CLEAR;
  total_quests = 2;
  group_up(holder, mate);
  give_quest(holder, Q_RESCUE);

  kill_cell_guard(mate);
  TEST_ASSERT_TRUE(is_complete(holder, Q_RESCUE));
  assert_not_cleared(holder);
  kill_cell_guard(mate);
  assert_cleared(holder);
}

static void test_a_killer_moved_by_the_death_script_cannot_rescue_for_the_group(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);

  new_mob(CAPTIVE, CELL);
  group_up(holder, mate);
  give_quest(holder, Q_RESCUE);
  death_script_killer_room = HALL;
  kill_cell_guard(mate);
  TEST_ASSERT_EQUAL_INT(HALL, IN_ROOM(mate));
  assert_not_rescued(holder);
}

static void test_a_killing_pet_moved_by_the_death_script_cannot_clear_for_its_master(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_pet(holder, CELL);
  struct char_data *guard = new_guard(CELL);

  give_room_clear_quest(holder, CELL);
  death_script_killer_room = HALL;
  raw_kill(guard, pet);
  TEST_ASSERT_EQUAL_INT(HALL, IN_ROOM(pet));
  assert_not_cleared(holder);
}

static void test_a_kill_in_another_room_does_not_count(void)
{
  struct char_data *holder = new_player(HALL);
  struct char_data *mate = new_player(CELL);
  struct char_data *guard = new_guard(CELL);

  new_mob(CAPTIVE, HALL);  /* alone with the holder, but nobody killed here */
  group_up(holder, mate);
  give_quest(holder, Q_RESCUE);
  raw_kill(guard, mate);
  assert_not_rescued(holder);
}

static void test_a_groupmates_kill_from_another_room_does_not_count(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(HALL);
  struct char_data *guard = new_guard(CELL);

  new_mob(CAPTIVE, CELL);
  group_up(holder, mate);
  give_quest(holder, Q_RESCUE);
  raw_kill(guard, mate);
  assert_not_rescued(holder);
}

static void test_a_cell_someone_else_cleared_stays_open(void)
{
  struct char_data *holder = new_player(HALL);
  struct char_data *stranger = new_player(CELL);
  struct char_data *guard = new_guard(CELL);

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
  struct char_data *guard = new_guard(CELL);

  group_up(holder, mate);
  give_room_clear_quest(holder, CELL);
  SET_BIT_AR(PLR_FLAGS(holder), PLR_NOTDEADYET);
  raw_kill(guard, mate);
  assert_not_cleared(holder);
}

static void kill_someone(struct char_data *victim)
{
  (void)victim;
  SET_BIT_AR(PLR_FLAGS(script_moves), PLR_NOTDEADYET);
}

static void test_a_groupmate_a_death_script_kills_gets_no_credit(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *guard = new_guard(CELL);

  group_up(holder, mate);
  give_room_clear_quest(holder, CELL);
  script_moves = holder;
  death_script = kill_someone;
  raw_kill(guard, mate);
  assert_not_cleared(holder);
}

static int script_position;

static void set_witness_position(struct char_data *victim)
{
  (void)victim;
  GET_POS(script_moves) = script_position;
}

static void test_a_witness_already_dead_before_the_script_gets_no_credit(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);

  group_up(holder, mate);
  give_room_clear_quest(holder, CELL);
  GET_POS(holder) = POS_DEAD;
  script_moves = holder;
  script_position = POS_STANDING;
  death_script = set_witness_position;
  kill_cell_guard(mate);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(holder));
  assert_not_cleared(holder);
}

static void test_a_witness_in_dead_position_after_the_script_gets_no_credit(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);

  group_up(holder, mate);
  give_room_clear_quest(holder, CELL);
  script_moves = holder;
  script_position = POS_DEAD;
  death_script = set_witness_position;
  kill_cell_guard(mate);
  TEST_ASSERT_FALSE(DEAD(holder));  /* extraction has not been scheduled */
  TEST_ASSERT_EQUAL_INT(POS_DEAD, GET_POS(holder));
  assert_not_cleared(holder);
}

static void test_a_groupmate_a_death_script_brings_in_gets_no_credit(void)
{
  struct char_data *holder = new_player(HALL);
  struct char_data *mate = new_player(CELL);
  struct char_data *guard = new_guard(CELL);

  group_up(holder, mate);
  give_room_clear_quest(holder, CELL);
  script_moves = holder;
  script_moves_to = CELL;
  death_script = move_someone;
  raw_kill(guard, mate);
  TEST_ASSERT_EQUAL_INT(CELL, IN_ROOM(holder));
  assert_not_cleared(holder);
}

static void test_a_groupmate_a_death_script_takes_away_gets_no_credit(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *guard = new_guard(CELL);

  group_up(holder, mate);
  give_room_clear_quest(holder, HALL);  /* empty, so any check there passes */
  script_moves = holder;
  script_moves_to = HALL;
  death_script = move_someone;
  raw_kill(guard, mate);
  TEST_ASSERT_EQUAL_INT(HALL, IN_ROOM(holder));
  assert_not_cleared(holder);
}

static void test_killing_a_pet_clears_no_room(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_combat_pet(new_player(CELL));

  give_room_clear_quest(holder, CELL);
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  TEST_ASSERT_TRUE(DEAD(pet));
  assert_not_cleared(holder);
}

static void test_a_groupmate_killing_a_pet_after_charm_expires_clears_no_room(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *pet = new_combat_pet(owner);

  group_up(owner, mate);
  give_room_clear_quest(owner, CELL);
  TEST_ASSERT_EQUAL_INT(1, damage(mate, pet, 1, TYPE_HIT));
  pet->affected->duration = 0;
  affect_update();
  TEST_ASSERT_FALSE(is_player_pet(pet));
  TEST_ASSERT_EQUAL_INT(-1, damage(mate, pet, 30, TYPE_HIT));
  assert_not_cleared(owner);
}

static void test_a_pet_that_started_the_fight_still_counts_as_a_pet_when_it_dies(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *pet = new_combat_pet(owner);
  struct char_data *opponent = new_pet(mate, CELL);

  opponent->player.name = opponent->player.short_descr = "opponent";
  GET_HIT(opponent) = GET_MAX_HIT(opponent) = 10;
  group_up(owner, mate);
  give_room_clear_quest(owner, CELL);
  TEST_ASSERT_EQUAL_INT(1, damage(pet, opponent, 1, TYPE_HIT));
  pet->affected->duration = 0;
  affect_update();
  TEST_ASSERT_FALSE(is_player_pet(pet));
  TEST_ASSERT_EQUAL_INT(-1, damage(mate, pet, 30, TYPE_HIT));
  assert_not_cleared(owner);
}

static void test_a_former_pet_still_in_combat_can_stay_when_the_room_is_cleared(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *pet = new_combat_pet(owner);

  group_up(owner, mate);
  give_room_clear_quest(owner, CELL);
  TEST_ASSERT_EQUAL_INT(1, damage(owner, pet, 1, TYPE_HIT));
  TEST_ASSERT_FALSE(is_player_pet(pet));
  TEST_ASSERT_EQUAL_PTR(owner, FIGHTING(pet));
  kill_cell_guard(mate);
  assert_cleared(owner);
}

static void test_a_groupmate_can_stabilize_the_captive_beside_a_fighting_former_pet(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *healer = new_player(CELL);
  struct char_data *pet = new_combat_pet(owner);
  struct char_data *captive = new_dying_captive(CELL);

  group_up(owner, healer);
  give_quest(owner, Q_RESCUE);
  TEST_ASSERT_EQUAL_INT(1, damage(owner, pet, 1, TYPE_HIT));
  TEST_ASSERT_FALSE(is_player_pet(pet));
  TEST_ASSERT_EQUAL_PTR(owner, FIGHTING(pet));
  mag_points(20, healer, captive, SPELL_HEAL, 0);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(captive));
  assert_rescued(owner);
}

static void test_a_recovered_idle_former_pet_must_be_killed_to_clear_the_room(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *pet = new_combat_pet(owner);

  group_up(owner, mate);
  give_room_clear_quest(owner, CELL);
  TEST_ASSERT_EQUAL_INT(1, damage(owner, pet, 1, TYPE_HIT));
  stop_fighting(owner);
  stop_fighting(pet);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(pet));
  kill_cell_guard(mate);
  assert_not_cleared(owner);
  TEST_ASSERT_EQUAL_INT(-1, damage(mate, pet, 30, TYPE_HIT));
  assert_cleared(owner);
}

static void test_a_players_death_clears_no_room(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *victim = new_player(CELL);
  struct kill_witnesses *witnesses;

  give_room_clear_quest(holder, CELL);
  witnesses = note_kill_witnesses(holder, CELL);
  SET_BIT_AR(PLR_FLAGS(victim), PLR_NOTDEADYET);
  credit_kill_witnesses(witnesses, victim);
  assert_not_cleared(holder);
}

static void test_a_pet_dying_of_its_wounds_credits_nobody(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_combat_pet(holder);

  new_mob(CAPTIVE, CELL);  /* someone else killed the guards */
  give_quest(holder, Q_RESCUE);
  raw_kill(pet, pet);
  assert_not_rescued(holder);
}

/* A mob can be in a player's group without being charmed. */
static void test_a_groupmate_dying_of_its_wounds_credits_nobody(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *hireling = new_guard(CELL);

  new_mob(CAPTIVE, CELL);
  group_up(holder, hireling);
  give_quest(holder, Q_RESCUE);
  raw_kill(hireling, hireling);
  assert_not_rescued(holder);
}

static void test_a_groupmates_earthquake_that_spares_the_captive_completes_the_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *captive = earthquake_target(CAPTIVE, 100);
  struct char_data *guard = earthquake_target(GUARD, 1);

  group_up(holder, mate);
  give_quest(holder, Q_RESCUE);
  mag_areas(30, mate, SPELL_EARTHQUAKE, 1);
  TEST_ASSERT_TRUE(DEAD(guard));
  TEST_ASSERT_TRUE(GET_HIT(captive) > 0);
  assert_rescued(holder);
}

static void test_a_groupmates_earthquake_must_spare_the_captive(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *captive = earthquake_target(CAPTIVE, 1);
  struct char_data *guard = earthquake_target(GUARD, 1);

  /* Each new arrival is added at the head of the room list. */
  group_up(holder, mate);
  give_quest(holder, Q_RESCUE);
  mag_areas(30, mate, SPELL_EARTHQUAKE, 1);
  TEST_ASSERT_TRUE(DEAD(guard));
  TEST_ASSERT_TRUE(DEAD(captive));
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
  holder_sees_death(holder, guard);
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
  holder_sees_death(holder, guard);
  assert_not_rescued(holder);

  do_bandage(medic, arg, 0, 0);
  assert_rescued(holder);
}

static void test_a_groupmates_heal_does_not_reward_a_dead_holder(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *cleric = new_player(CELL);
  struct char_data *captive = new_dying_captive(CELL);

  group_up(holder, cleric);
  give_quest(holder, Q_RESCUE);
  SET_BIT_AR(PLR_FLAGS(holder), PLR_NOTDEADYET);
  mag_points(20, cleric, captive, SPELL_CURE_CRITIC, 0);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(captive));
  assert_not_rescued(holder);
}

static void test_a_heal_from_another_room_credits_nobody(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *cleric = new_player(HALL);
  struct char_data *captive = new_dying_captive(CELL);

  group_up(holder, cleric);
  give_quest(holder, Q_RESCUE);
  mag_points(20, cleric, captive, SPELL_CURE_CRITIC, 0);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(captive));
  assert_not_rescued(holder);
}

static void test_a_healer_in_dead_position_cannot_rescue_for_a_living_groupmate(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *healer = new_player(CELL);
  struct char_data *captive = new_dying_captive(CELL);

  group_up(holder, healer);
  give_quest(holder, Q_RESCUE);
  GET_POS(healer) = POS_DEAD;
  mag_points(20, healer, captive, SPELL_HEAL, 0);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(captive));
  assert_not_rescued(holder);
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
  /* One heal restores 100 + 3d8 HP. */
  TEST_ASSERT_GREATER_OR_EQUAL_INT(-4 + 103, GET_HIT(pet));
  TEST_ASSERT_LESS_OR_EQUAL_INT(-4 + 124, GET_HIT(pet));
}

static void test_a_mob_charmed_during_an_existing_fight_clears_no_room(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *guard = new_guard(CELL);
  struct affected_type charm;

  GET_HIT(guard) = GET_MAX_HIT(guard) = 10;
  group_up(holder, mate);
  give_room_clear_quest(holder, CELL);
  TEST_ASSERT_EQUAL_INT(1, damage(mate, guard, 1, TYPE_HIT));

  /* The guard gains and loses charm between two hits in the same fight. */
  add_follower(guard, holder);
  new_affect(&charm);
  charm.spell = SPELL_CHARM;
  charm.duration = 0;
  SET_BIT_AR(charm.bitvector, AFF_CHARM);
  affect_to_char(guard, &charm);
  TEST_ASSERT_TRUE(is_player_pet(guard));
  affect_update();
  TEST_ASSERT_FALSE(is_player_pet(guard));
  TEST_ASSERT_EQUAL_PTR(mate, FIGHTING(guard));

  TEST_ASSERT_EQUAL_INT(-1, damage(mate, guard, 30, TYPE_HIT));
  assert_not_cleared(holder);
}

static void test_killing_a_former_pets_sole_attacker_still_clears_the_room(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *pet = new_combat_pet(owner);
  struct char_data *guard1 = new_guard(CELL);
  struct char_data *guard2 = new_guard(CELL);

  GET_HIT(guard1) = GET_MAX_HIT(guard1) = 10;
  GET_HIT(guard2) = GET_MAX_HIT(guard2) = 10;
  group_up(owner, mate);
  give_room_clear_quest(owner, CELL);

  /* Pet fights guard1 while guard2 also targets pet, then charm expires. */
  TEST_ASSERT_EQUAL_INT(1, damage(pet, guard1, 1, TYPE_HIT));
  TEST_ASSERT_EQUAL_INT(1, damage(guard2, pet, 1, TYPE_HIT));
  pet->affected->duration = 0;
  affect_update();
  TEST_ASSERT_FALSE(is_player_pet(pet));

  /* Killing guard1 ends the first pulse with only guard2 targeting pet. */
  TEST_ASSERT_EQUAL_INT(-1, damage(mate, guard1, 30, TYPE_HIT));
  extract_pending_chars();
  TEST_ASSERT_NULL(FIGHTING(pet));
  TEST_ASSERT_EQUAL_PTR(pet, FIGHTING(guard2));
  TEST_ASSERT_TRUE(pet->mob_specials.fought_as_player_pet);
  assert_not_cleared(owner);

  /* Killing guard2 must still treat pet as part of this fight, then
   * end-of-pulse extraction clears pet's flag once it is idle. */
  TEST_ASSERT_EQUAL_INT(-1, damage(mate, guard2, 30, TYPE_HIT));
  assert_cleared(owner);
  extract_pending_chars();
  TEST_ASSERT_FALSE(pet->mob_specials.fought_as_player_pet);
  stop_follower(pet);
}

static void test_earthquake_killing_sole_attacker_and_former_pet_gives_no_credit(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *owner = new_player(CELL);
  struct char_data *captive = earthquake_target(CAPTIVE, 100);
  struct char_data *pet = new_combat_pet(owner);
  struct char_data *attacker_pet = new_combat_pet(owner);

  give_quest(holder, Q_RESCUE);
  TEST_ASSERT_EQUAL_INT(1, damage(attacker_pet, pet, 1, TYPE_HIT));
  pet->affected->duration = 0;
  attacker_pet->affected->duration = 0;
  affect_update();
  stop_fighting(pet);
  TEST_ASSERT_NULL(FIGHTING(pet));
  TEST_ASSERT_EQUAL_PTR(pet, FIGHTING(attacker_pet));
  TEST_ASSERT_TRUE(pet->mob_specials.fought_as_player_pet);

  /* attacker_pet is ahead of pet in the room list, so earthquake kills it
   * first; pet must stay marked as a former pet for the rest of the spell. */
  GET_HIT(attacker_pet) = 1;
  GET_HIT(pet) = 1;
  mag_areas(30, holder, SPELL_EARTHQUAKE, 1);
  TEST_ASSERT_TRUE(DEAD(attacker_pet));
  TEST_ASSERT_TRUE(DEAD(pet));
  TEST_ASSERT_TRUE(GET_HIT(captive) > 0);
  assert_not_rescued(holder);
}

static void test_earthquake_sparing_a_former_pet_whose_sole_attacker_dies_completes_rescue(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *captive = earthquake_target(CAPTIVE, 100);
  struct char_data *pet = new_combat_pet(holder);
  struct char_data *guard = earthquake_target(GUARD, 10);

  GET_HIT(pet) = GET_MAX_HIT(pet) = 100;
  group_up(holder, mate);
  give_quest(holder, Q_RESCUE);
  TEST_ASSERT_EQUAL_INT(1, damage(guard, pet, 1, TYPE_HIT));
  pet->affected->duration = 0;
  affect_update();
  stop_fighting(pet);
  TEST_ASSERT_NULL(FIGHTING(pet));
  TEST_ASSERT_EQUAL_PTR(pet, FIGHTING(guard));

  GET_HIT(guard) = 1;
  mag_areas(30, mate, SPELL_EARTHQUAKE, 1);
  TEST_ASSERT_TRUE(DEAD(guard));
  TEST_ASSERT_TRUE(GET_HIT(pet) > 0);
  TEST_ASSERT_TRUE(GET_HIT(captive) > 0);
  assert_rescued(holder);
  extract_pending_chars();
  /* The earthquake also hit pet, so pet is now fighting mate until it disengages. */
  TEST_ASSERT_EQUAL_PTR(mate, FIGHTING(pet));
  TEST_ASSERT_TRUE(pet->mob_specials.fought_as_player_pet);
  stop_fighting(pet);
  if (FIGHTING(mate) == pet)
    stop_fighting(mate);
  TEST_ASSERT_FALSE(pet->mob_specials.fought_as_player_pet);
  stop_follower(pet);
}

static void test_stunning_a_sole_attacker_mid_area_action_keeps_former_pet_marked(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *owner = new_player(CELL);
  struct char_data *pet = new_combat_pet(owner);
  struct char_data *attacker_pet = new_combat_pet(owner);

  new_mob(CAPTIVE, CELL);
  give_quest(holder, Q_RESCUE);
  TEST_ASSERT_EQUAL_INT(1, damage(attacker_pet, pet, 1, TYPE_HIT));
  pet->affected->duration = 0;
  attacker_pet->affected->duration = 0;
  affect_update();
  stop_fighting(pet);
  TEST_ASSERT_NULL(FIGHTING(pet));
  TEST_ASSERT_EQUAL_PTR(pet, FIGHTING(attacker_pet));

  /* Down attacker_pet to POS_INCAP mid-action: pet must stay marked through
   * the rest of the area action, so killing pet next grants no rescue credit. */
  begin_rescue_action();
  TEST_ASSERT_EQUAL_INT(14, damage(holder, attacker_pet, 14, TYPE_HIT));
  TEST_ASSERT_EQUAL_INT(POS_INCAP, GET_POS(attacker_pet));
  TEST_ASSERT_NULL(FIGHTING(attacker_pet));
  TEST_ASSERT_TRUE(pet->mob_specials.fought_as_player_pet);
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  end_rescue_action();
  assert_not_rescued(holder);
  stop_follower(attacker_pet);
}

static void test_a_poisoned_downed_pet_losing_charm_clears_no_room(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_combat_pet(owner);

  give_room_clear_quest(holder, CELL);
  /* Self-damage (like poison ticks in point_update) downs pet without combat. */
  TEST_ASSERT_EQUAL_INT(14, damage(pet, pet, 14, SPELL_POISON));
  TEST_ASSERT_EQUAL_INT(POS_INCAP, GET_POS(pet));
  TEST_ASSERT_NULL(FIGHTING(pet));

  pet->affected->duration = 0;
  affect_update();
  TEST_ASSERT_FALSE(is_player_pet(pet));
  TEST_ASSERT_TRUE(pet->mob_specials.fought_as_player_pet);

  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  assert_not_cleared(holder);
}

static void test_a_downed_pet_whose_owner_leaves_clears_no_room(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_combat_pet(owner);

  give_room_clear_quest(holder, CELL);
  /* Simulate script_damage() or non-combat downing, then owner quitting/dying. */
  GET_HIT(pet) = -4;
  GET_POS(pet) = POS_INCAP;
  stop_follower(pet);
  TEST_ASSERT_FALSE(is_player_pet(pet));
  TEST_ASSERT_TRUE(pet->mob_specials.fought_as_player_pet);

  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  assert_not_cleared(holder);
}

static void test_a_downed_former_pet_counts_after_recovering_outside_combat(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_combat_pet(owner);

  give_room_clear_quest(holder, CELL);
  GET_HIT(pet) = -4;
  GET_POS(pet) = POS_INCAP;
  stop_follower(pet);
  TEST_ASSERT_TRUE(pet->mob_specials.fought_as_player_pet);

  mag_points(20, holder, pet, SPELL_HEAL, 0);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(pet));
  TEST_ASSERT_FALSE(pet->mob_specials.fought_as_player_pet);

  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  assert_cleared(holder);
}

static void test_a_mob_charmed_while_downed_stays_a_pet_when_charm_expires(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *holder = new_player(CELL);
  struct char_data *guard = new_guard(CELL);
  struct affected_type charm;

  give_room_clear_quest(holder, CELL);
  GET_HIT(guard) = -4;
  GET_POS(guard) = POS_INCAP;

  add_follower(guard, owner);
  new_affect(&charm);
  charm.spell = SPELL_CHARM;
  charm.duration = 0;
  SET_BIT_AR(charm.bitvector, AFF_CHARM);
  affect_to_char(guard, &charm);
  TEST_ASSERT_TRUE(is_player_pet(guard));
  TEST_ASSERT_TRUE(guard->mob_specials.fought_as_player_pet);

  affect_update();
  TEST_ASSERT_FALSE(is_player_pet(guard));
  TEST_ASSERT_TRUE(guard->mob_specials.fought_as_player_pet);

  TEST_ASSERT_EQUAL_INT(-1, damage(holder, guard, 30, TYPE_HIT));
  assert_not_cleared(holder);
}

static void test_credit_kill_witnesses_rejects_null_and_living_victims(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *stranger_mob = new_guard(CELL);
  struct char_data *living_guard = new_guard(HALL);
  struct kill_witnesses *witnesses;

  /* An ungrouped, uncharmed mob has no player side to record. */
  TEST_ASSERT_NULL(note_kill_witnesses(stranger_mob, CELL));
  char_from_room(stranger_mob);
  char_to_room(stranger_mob, HALL);

  give_room_clear_quest(holder, CELL);
  witnesses = note_kill_witnesses(holder, CELL);
  TEST_ASSERT_NOT_NULL(witnesses);
  credit_kill_witnesses(witnesses, NULL);
  assert_not_cleared(holder);

  witnesses = note_kill_witnesses(holder, CELL);
  TEST_ASSERT_NOT_NULL(witnesses);
  credit_kill_witnesses(witnesses, living_guard);
  assert_not_cleared(holder);
}

static void test_a_large_group_credits_every_living_quest_holder_in_the_room(void)
{
  struct char_data *members[5];
  struct char_data *guard = new_guard(CELL);
  int n;

  for (n = 0; n < 5; n++) {
    members[n] = new_player(CELL);
    if (n > 0)
      group_up(members[0], members[n]);
  }
  for (n = 0; n < 5; n++)
    give_room_clear_quest(members[n], CELL);

  raw_kill(guard, members[4]);
  for (n = 0; n < 5; n++)
    assert_cleared(members[n]);
}

static void expire_charm_while_fighting(struct char_data *pet, struct char_data *guard)
{
  TEST_ASSERT_EQUAL_INT(1, damage(guard, pet, 1, TYPE_HIT));
  pet->affected->duration = 0;
  affect_update();
  TEST_ASSERT_FALSE(is_player_pet(pet));
  TEST_ASSERT_EQUAL_PTR(pet, FIGHTING(guard));
  TEST_ASSERT_EQUAL_PTR(guard, FIGHTING(pet));
}

static void test_an_earthquake_that_makes_a_guard_flee_cannot_count_the_former_pet_it_kills(void)
{
  static struct room_direction_data exit_to_hall;
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_combat_pet(holder);
  struct char_data *guard = new_guard(CELL);  /* earthquake hits the newest arrival first */
  int dir;

  exit_to_hall.to_room = HALL;
  for (dir = 0; dir < DIR_COUNT; dir++)
    rooms[CELL].dir_option[dir] = &exit_to_hall;
  fleeing_mob = guard;
  GET_HIT(guard) = 50;
  GET_MAX_HIT(guard) = 400;
  give_room_clear_quest(holder, CELL);
  expire_charm_while_fighting(pet, guard);
  SET_BIT_AR(MOB_FLAGS(guard), MOB_WIMPY);
  GET_HIT(pet) = 1;

  mag_areas(30, holder, SPELL_EARTHQUAKE, 1);
  TEST_ASSERT_EQUAL_INT(HALL, IN_ROOM(guard));
  TEST_ASSERT_TRUE(DEAD(pet));
  assert_not_cleared(holder);
}

static void test_an_idle_former_pet_counts_in_a_new_fight_before_the_next_heartbeat(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *stranger = new_player(CELL);
  struct char_data *pet = new_combat_pet(holder);
  struct char_data *guard = new_guard(CELL);

  GET_HIT(guard) = GET_MAX_HIT(guard) = 10;
  give_room_clear_quest(holder, CELL);
  expire_charm_while_fighting(pet, guard);
  stop_fighting(pet);  /* Only guard is targeting pet. */

  TEST_ASSERT_EQUAL_INT(-1, damage(stranger, guard, 30, TYPE_HIT));
  assert_not_cleared(holder);
  TEST_ASSERT_EQUAL_INT(POS_STANDING, GET_POS(pet));
  TEST_ASSERT_FALSE(has_combat_opponent(pet));
  TEST_ASSERT_EQUAL_INT(1, damage(holder, pet, 1, TYPE_HIT));
  extract_pending_chars();
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  assert_cleared(holder);
}

/* Here the former pet is still attacking the guard when the guard dies.  Its
 * only combat link is to a mob awaiting extraction, so the next attack treats
 * it as an ordinary mobile without waiting for the heartbeat. */
static void test_a_former_pet_attacking_a_dead_guard_counts_in_a_new_fight(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *stranger = new_player(CELL);
  struct char_data *pet = new_combat_pet(holder);
  struct char_data *guard = new_guard(CELL);

  GET_HIT(guard) = GET_MAX_HIT(guard) = 10;
  give_room_clear_quest(holder, CELL);
  expire_charm_while_fighting(pet, guard);
  TEST_ASSERT_EQUAL_INT(-1, damage(stranger, guard, 30, TYPE_HIT));
  TEST_ASSERT_EQUAL_PTR(guard, FIGHTING(pet));  /* Unlinked at extraction. */
  TEST_ASSERT_FALSE(pet->mob_specials.fought_as_player_pet);
  assert_not_cleared(holder);
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, pet, 30, TYPE_HIT));
  assert_cleared(holder);
}

static void heal_former_pet(struct char_data *guard)
{
  (void)guard;
  GET_HIT(script_moves)++;
  update_pos(script_moves);
}

static void test_a_death_script_healing_a_former_pet_preserves_the_current_kill_credit(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *pet = new_combat_pet(holder);
  struct char_data *guard = new_guard(CELL);

  GET_HIT(guard) = GET_MAX_HIT(guard) = 10;
  give_room_clear_quest(holder, CELL);
  expire_charm_while_fighting(pet, guard);
  stop_fighting(pet);
  script_moves = pet;
  death_script = heal_former_pet;
  TEST_ASSERT_EQUAL_INT(-1, damage(holder, guard, 30, TYPE_HIT));
  TEST_ASSERT_EQUAL_INT(GET_MAX_HIT(pet), GET_HIT(pet));
  assert_cleared(holder);
  stop_follower(pet);
}

static struct char_data *guard_attacking_holder;

static void groupmate_finishes_guard_during_holders_death(struct char_data *holder)
{
  struct char_data *mate = GROUP_LEADER(GROUP(holder));

  death_script = NULL;  /* The guard has no death script of its own. */
  TEST_ASSERT_EQUAL_INT(-1, damage(mate, guard_attacking_holder, 30, TYPE_HIT));
}

static void test_a_dying_holder_cannot_share_a_guard_kill_triggered_by_their_death(void)
{
  struct char_data *holder = new_player(CELL);
  struct char_data *mate = new_player(CELL);
  struct char_data *guard = new_guard(CELL);

  holder->player.name = "holder";
  mate->player.name = "mate";
  GET_HIT(holder) = GET_MAX_HIT(holder) = 10;
  GET_HIT(guard) = GET_MAX_HIT(guard) = 10;
  group_up(mate, holder);
  give_room_clear_quest(holder, CELL);
  give_room_clear_quest(mate, CELL);
  guard_attacking_holder = guard;
  death_script = groupmate_finishes_guard_during_holders_death;
  /* Redirect PC death mudlog away from stderr so it cannot interleave with
   * Unity's buffered stdout lines under make test. */
  logfile = tmpfile();
  TEST_ASSERT_NOT_NULL(logfile);
  TEST_ASSERT_EQUAL_INT(-1, damage(guard, holder, 30, TYPE_HIT));
  TEST_ASSERT_TRUE(DEAD(guard));
  TEST_ASSERT_TRUE(DEAD(holder));
  assert_cleared(mate);
  assert_not_cleared(holder);
}

/* The captive's death script has a groupmate kill the last guard.  The
 * captive still stands in the room until extraction, but it is dying, so
 * the rescue fails. */
static struct char_data *guard_killing_captive, *mate_avenging_captive;

static void groupmate_finishes_guard_during_captives_death(struct char_data *captive)
{
  (void)captive;
  death_script = NULL;  /* The guard has no death script of its own. */
  TEST_ASSERT_EQUAL_INT(-1, damage(mate_avenging_captive, guard_killing_captive, 30, TYPE_HIT));
}

static void test_a_dying_captive_is_not_rescued_by_a_kill_during_its_death(void)
{
  static char name[] = "captive";
  struct char_data *holder = new_player(CELL);
  struct char_data *captive = new_mob(CAPTIVE, CELL);

  mate_avenging_captive = new_player(CELL);
  guard_killing_captive = new_guard(CELL);
  captive->player.name = captive->player.short_descr = name;
  GET_HIT(captive) = GET_MAX_HIT(captive) = 10;
  GET_HIT(guard_killing_captive) = GET_MAX_HIT(guard_killing_captive) = 10;
  group_up(holder, mate_avenging_captive);
  give_quest(holder, Q_RESCUE);
  death_script = groupmate_finishes_guard_during_captives_death;
  TEST_ASSERT_EQUAL_INT(-1, damage(guard_killing_captive, captive, 30, TYPE_HIT));
  TEST_ASSERT_TRUE(DEAD(guard_killing_captive));
  TEST_ASSERT_TRUE(DEAD(captive));
  assert_not_rescued(holder);
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
  RUN_TEST(test_moving_the_dying_guard_preserves_a_local_rescue);
  RUN_TEST(test_death_script_cannot_transfer_a_rescue_to_its_destination);
  RUN_TEST(test_local_kill_completes_a_room_clear);
  RUN_TEST(test_moving_only_the_dying_guard_preserves_a_room_clear);
  RUN_TEST(test_moving_only_the_killer_to_an_empty_target_room_gives_no_clear);
  RUN_TEST(test_killer_brought_to_the_death_room_by_a_script_gets_no_clear);
  RUN_TEST(test_a_living_pet_does_not_complete_the_rescue);
  RUN_TEST(test_rescue_kill_check_rejects_a_dead_charmed_pet);
  RUN_TEST(test_rescue_trigger_rejects_a_dead_former_pet);
  RUN_TEST(test_rescue_fails_when_the_captive_died_first);
  RUN_TEST(test_killing_a_copy_of_the_captive_is_no_rescue);
  RUN_TEST(test_a_dying_captive_does_not_count);
  RUN_TEST(test_a_stunned_captive_still_counts);
  RUN_TEST(test_a_players_pet_may_stay_with_the_captive);
  RUN_TEST(test_a_captors_charmed_minion_blocks_the_rescue);
  RUN_TEST(test_killing_another_players_charmed_pet_does_not_complete_the_rescue);
  RUN_TEST(test_killing_a_pet_after_the_owners_first_hit_does_not_complete_the_rescue);
  RUN_TEST(test_a_former_pet_counts_in_a_later_fight);
  RUN_TEST(test_rescuing_a_former_pets_owner_does_not_make_its_death_count);
  RUN_TEST(test_a_downed_former_pet_still_being_attacked_does_not_count);
  RUN_TEST(test_finishing_a_stunned_pet_while_fighting_another_mob_does_not_count);
  RUN_TEST(test_returning_to_finish_an_incapacitated_pet_does_not_count);
  RUN_TEST(test_recovered_idle_former_pet_counts_in_a_new_fight);
  RUN_TEST(test_healing_a_pet_with_an_active_attacker_preserves_the_fight);
  RUN_TEST(test_a_former_pet_transformed_during_the_fight_does_not_count);
  RUN_TEST(test_a_bashed_pet_stays_stunned_until_it_recovers);
  RUN_TEST(test_a_dead_holder_is_not_rewarded);
  RUN_TEST(test_earthquake_kills_guard_before_captive);
  RUN_TEST(test_earthquake_kills_captive_before_guard);
  RUN_TEST(test_a_replacement_captive_cannot_survive_for_the_original);
  RUN_TEST(test_a_replacement_loaded_before_the_guard_dies_cannot_count);
  RUN_TEST(test_one_earthquake_advances_one_goal_when_the_captive_survives);
  RUN_TEST(test_killing_a_guard_completes_a_legacy_rescue);
  RUN_TEST(test_visiting_a_room_completes_a_quest_with_a_nonpositive_count);
  RUN_TEST(test_room_quest_rewards_only_the_last_goal);
  RUN_TEST(test_healing_a_captive_left_dying_finishes_the_rescue);
  RUN_TEST(test_a_heal_that_leaves_the_captive_dying_finishes_nothing);
  RUN_TEST(test_bandaging_a_captive_left_dying_finishes_the_rescue);
  RUN_TEST(test_healing_a_captive_that_was_not_dying_finishes_nothing);
  RUN_TEST(test_healing_a_dying_pet_finishes_nothing);
  RUN_TEST(test_bandaging_a_dying_pet_finishes_nothing);
  RUN_TEST(test_healing_a_pending_corpse_finishes_nothing);
  RUN_TEST(test_bandaging_a_pending_corpse_finishes_nothing);
  RUN_TEST(test_repeated_treatment_does_not_spend_multi_target_goals);
  RUN_TEST(test_treatment_preserves_multiple_goals_after_the_quest_quantity_is_lowered);
  RUN_TEST(test_treatment_does_not_finish_a_multi_target_quest_with_one_goal_left);
  RUN_TEST(test_healing_the_captive_in_an_already_clear_room_completes_the_rescue);
  RUN_TEST(test_healing_completes_a_rescue_with_a_nonpositive_quantity);
  RUN_TEST(test_bandaging_the_captive_in_an_already_clear_room_completes_the_rescue);
  RUN_TEST(test_healing_a_remote_copy_does_not_rescue_the_local_captive);
  RUN_TEST(test_a_pets_kill_completes_the_rescue);
  RUN_TEST(test_a_groupmates_kill_completes_the_rescue);
  RUN_TEST(test_a_groupmates_pets_kill_completes_the_rescue);
  RUN_TEST(test_a_pets_kill_clears_the_room);
  RUN_TEST(test_a_groupmates_kill_clears_the_room);
  RUN_TEST(test_a_kill_also_clears_the_room_for_the_stage_it_unlocks);
  RUN_TEST(test_a_kill_also_finishes_the_rescue_it_unlocks);
  RUN_TEST(test_a_groupmates_kill_also_clears_the_room_for_the_stage_it_unlocks);
  RUN_TEST(test_rescuing_a_charmed_captive_does_not_also_complete_the_next_room_clear);
  RUN_TEST(test_a_killer_moved_by_the_death_script_cannot_rescue_for_the_group);
  RUN_TEST(test_a_killing_pet_moved_by_the_death_script_cannot_clear_for_its_master);
  RUN_TEST(test_a_kill_in_another_room_does_not_count);
  RUN_TEST(test_a_groupmates_kill_from_another_room_does_not_count);
  RUN_TEST(test_a_cell_someone_else_cleared_stays_open);
  RUN_TEST(test_a_dead_groupmate_is_not_credited);
  RUN_TEST(test_a_groupmate_a_death_script_kills_gets_no_credit);
  RUN_TEST(test_a_witness_already_dead_before_the_script_gets_no_credit);
  RUN_TEST(test_a_witness_in_dead_position_after_the_script_gets_no_credit);
  RUN_TEST(test_a_groupmate_a_death_script_brings_in_gets_no_credit);
  RUN_TEST(test_a_groupmate_a_death_script_takes_away_gets_no_credit);
  RUN_TEST(test_killing_a_pet_clears_no_room);
  RUN_TEST(test_a_groupmate_killing_a_pet_after_charm_expires_clears_no_room);
  RUN_TEST(test_a_pet_that_started_the_fight_still_counts_as_a_pet_when_it_dies);
  RUN_TEST(test_a_former_pet_still_in_combat_can_stay_when_the_room_is_cleared);
  RUN_TEST(test_a_groupmate_can_stabilize_the_captive_beside_a_fighting_former_pet);
  RUN_TEST(test_a_recovered_idle_former_pet_must_be_killed_to_clear_the_room);
  RUN_TEST(test_a_players_death_clears_no_room);
  RUN_TEST(test_a_pet_dying_of_its_wounds_credits_nobody);
  RUN_TEST(test_a_groupmate_dying_of_its_wounds_credits_nobody);
  RUN_TEST(test_a_groupmates_earthquake_that_spares_the_captive_completes_the_rescue);
  RUN_TEST(test_a_groupmates_earthquake_must_spare_the_captive);
  RUN_TEST(test_a_groupmates_heal_finishes_the_rescue);
  RUN_TEST(test_a_groupmates_bandage_finishes_the_rescue);
  RUN_TEST(test_a_groupmates_heal_does_not_reward_a_dead_holder);
  RUN_TEST(test_a_heal_from_another_room_credits_nobody);
  RUN_TEST(test_a_healer_in_dead_position_cannot_rescue_for_a_living_groupmate);
  RUN_TEST(test_a_group_heal_heals_a_dying_pet_once);
  RUN_TEST(test_a_mob_charmed_during_an_existing_fight_clears_no_room);
  RUN_TEST(test_killing_a_former_pets_sole_attacker_still_clears_the_room);
  RUN_TEST(test_earthquake_killing_sole_attacker_and_former_pet_gives_no_credit);
  RUN_TEST(test_earthquake_sparing_a_former_pet_whose_sole_attacker_dies_completes_rescue);
  RUN_TEST(test_stunning_a_sole_attacker_mid_area_action_keeps_former_pet_marked);
  RUN_TEST(test_a_poisoned_downed_pet_losing_charm_clears_no_room);
  RUN_TEST(test_a_downed_pet_whose_owner_leaves_clears_no_room);
  RUN_TEST(test_a_downed_former_pet_counts_after_recovering_outside_combat);
  RUN_TEST(test_a_mob_charmed_while_downed_stays_a_pet_when_charm_expires);
  RUN_TEST(test_credit_kill_witnesses_rejects_null_and_living_victims);
  RUN_TEST(test_a_large_group_credits_every_living_quest_holder_in_the_room);
  RUN_TEST(test_an_earthquake_that_makes_a_guard_flee_cannot_count_the_former_pet_it_kills);
  RUN_TEST(test_an_idle_former_pet_counts_in_a_new_fight_before_the_next_heartbeat);
  RUN_TEST(test_a_former_pet_attacking_a_dead_guard_counts_in_a_new_fight);
  RUN_TEST(test_a_death_script_healing_a_former_pet_preserves_the_current_kill_credit);
  RUN_TEST(test_a_dying_holder_cannot_share_a_guard_kill_triggered_by_their_death);
  RUN_TEST(test_a_dying_captive_is_not_rescued_by_a_kill_during_its_death);
  return UNITY_END();
}
