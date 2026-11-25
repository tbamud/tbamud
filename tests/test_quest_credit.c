/* Exercise the real death path and its quest dispatch, with small rooms
 * that stay in memory until the pending extractions are processed. */
#include "unity.h"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "dg_scripts.h"
#include "fight.h"
#include "handler.h"
#include "quest.h"

enum { CELL, HALL };
enum { CAPTIVE, GUARD, PET };
enum { RESCUE = 10, CLEAR_CELL, CLEAR_HALL, KILL_THEN_RESCUE };

struct aq_data *aquest_table;
qst_rnum total_quests;

static struct room_data rooms[2];
static struct index_data mobs[3];
static struct aq_data quests[4];
static struct char_data characters[10];
static struct player_special_data specials[10];
static size_t character_count;
static struct char_data *move_on_death;
static char empty[] = "";
static char name[] = "test character";

/* Provide the external boundaries of raw_kill() and quest completion. */
obj_rnum real_object(obj_vnum vnum) { (void)vnum; return NOTHING; }
struct obj_data *read_object(obj_vnum nr, int type) { (void)nr; (void)type; return NULL; }
void gain_exp(struct char_data *ch, int gain) { (void)ch; (void)gain; }
int increase_gold(struct char_data *ch, int amt) { (void)ch; return amt; }
int remove_otrigger(struct obj_data *obj, struct char_data *ch)
{ (void)obj; (void)ch; return 1; }

int death_mtrigger(struct char_data *ch, struct char_data *killer)
{
  (void)ch;
  (void)killer;
  if (move_on_death) {
    char_from_room(move_on_death);
    char_to_room(move_on_death, HALL);
  }
  return 1;
}

struct obj_data *create_obj(void)
{
  struct obj_data *obj = calloc(1, sizeof(*obj));

  TEST_ASSERT_NOT_NULL(obj);
  obj->next = object_list;
  object_list = obj;
  return obj;
}

static void init_quest(struct aq_data *quest, qst_vnum vnum, int type, int target)
{
  quest->vnum = vnum;
  quest->name = quest->desc = quest->info = quest->done = quest->quit = empty;
  quest->type = type;
  quest->target = target;
  quest->value[4] = -1; /* no time limit */
  quest->value[6] = 1;  /* quantity */
  quest->obj_reward = NOTHING;
  quest->next_quest = NOTHING;
}

static struct char_data *new_character(room_rnum room, mob_rnum mob)
{
  struct char_data *ch;

  TEST_ASSERT_TRUE(character_count < sizeof(characters) / sizeof(characters[0]));
  ch = &characters[character_count];
  IN_ROOM(ch) = NOWHERE;
  GET_HIT(ch) = 10;
  GET_POS(ch) = POS_STANDING;
  GET_PFILEPOS(ch) = -1;
  ch->player.name = ch->player.short_descr = name;
  if (mob == NOBODY) {
    ch->player_specials = &specials[character_count];
    clear_quest(ch);
  } else {
    SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
    ch->nr = mob;
    ch->player_specials = &dummy_mob;
    ++mob_index[mob].number;
  }
  character_count++;
  ch->next = character_list;
  character_list = ch;
  char_to_room(ch, room);
  return ch;
}

static struct char_data *new_player(room_rnum room)
{
  return new_character(room, NOBODY);
}

static void give_quest(struct char_data *ch, qst_vnum vnum)
{
  set_quest(ch, real_quest(vnum));
}

static void kill_character(struct char_data *victim, struct char_data *killer)
{
  GET_HIT(victim) = -11;
  GET_POS(victim) = POS_DEAD;
  raw_kill(victim, killer);
  TEST_ASSERT_TRUE(DEAD(victim));
}

static void assert_active(struct char_data *ch, qst_vnum vnum)
{
  TEST_ASSERT_EQUAL_INT(vnum, GET_QUEST(ch));
  TEST_ASSERT_FALSE(is_complete(ch, vnum));
}

static void assert_completed(struct char_data *ch, qst_vnum vnum)
{
  TEST_ASSERT_EQUAL_INT(NOTHING, GET_QUEST(ch));
  TEST_ASSERT_TRUE(is_complete(ch, vnum));
}

void setUp(void)
{
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
  init_quest(&quests[0], RESCUE, AQ_MOB_SAVE, mobs[CAPTIVE].vnum);
  init_quest(&quests[1], CLEAR_CELL, AQ_ROOM_CLEAR, rooms[CELL].number);
  init_quest(&quests[2], CLEAR_HALL, AQ_ROOM_CLEAR, rooms[HALL].number);
  init_quest(&quests[3], KILL_THEN_RESCUE, AQ_MOB_KILL, mobs[GUARD].vnum);
  quests[3].next_quest = RESCUE;
  aquest_table = quests;
  total_quests = sizeof(quests) / sizeof(quests[0]);

  memset(characters, 0, sizeof(characters));
  memset(specials, 0, sizeof(specials));
  character_count = 0;
  character_list = NULL;
  group_list = create_list();
  move_on_death = NULL;
}

void tearDown(void)
{
  size_t i;
  struct obj_data *obj;

  extract_pending_chars();
  for (i = 0; i < character_count; i++) {
    struct char_data *ch = &characters[i];

    if (ch->master || ch->followers)
      die_follower(ch);
    if (GROUP(ch))
      leave_group(ch);
    free(specials[i].saved.completed_quests);
  }
  free_list(group_list);
  character_list = NULL;

  while ((obj = object_list)) {
    object_list = obj->next;
    free(obj->name);
    free(obj->description);
    free(obj->short_description);
    free(obj);
  }
}

static void test_rescue_credits_nearby_party_only(void)
{
  struct char_data *killer = new_player(CELL);
  struct group_data *group = create_group(killer);
  struct char_data *nearby = new_player(CELL);
  struct char_data *away = new_player(HALL);
  struct char_data *bystander = new_player(CELL);
  struct char_data *guard = new_character(CELL, GUARD);

  new_character(CELL, CAPTIVE);
  new_character(HALL, CAPTIVE);
  join_group(nearby, group);
  join_group(away, group);
  give_quest(nearby, RESCUE);
  give_quest(away, RESCUE);
  give_quest(bystander, RESCUE);

  kill_character(guard, killer);

  assert_completed(nearby, RESCUE);
  assert_active(away, RESCUE);
  assert_active(bystander, RESCUE);
}

static void test_room_clear_credits_only_living_party_in_the_room(void)
{
  struct char_data *killer = new_player(CELL);
  struct group_data *group = create_group(killer);
  struct char_data *nearby = new_player(CELL);
  struct char_data *away = new_player(HALL);
  struct char_data *fallen = new_player(CELL);

  join_group(nearby, group);
  join_group(away, group);
  join_group(fallen, group);
  give_quest(nearby, CLEAR_CELL);
  give_quest(away, CLEAR_HALL);
  give_quest(fallen, CLEAR_CELL);
  extract_char(fallen);

  kill_character(new_character(CELL, GUARD), killer);

  assert_completed(nearby, CLEAR_CELL);
  assert_active(away, CLEAR_HALL);
  assert_active(fallen, CLEAR_CELL);
}

static void test_pet_kill_credits_its_owner_and_party_for_rescue(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *member = new_player(CELL);
  struct char_data *pet = new_character(CELL, PET);

  join_group(member, create_group(owner));
  give_quest(owner, RESCUE);
  give_quest(member, RESCUE);
  new_character(CELL, CAPTIVE);
  add_follower(pet, owner);
  SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);

  kill_character(new_character(CELL, GUARD), pet);

  assert_completed(owner, RESCUE);
  assert_completed(member, RESCUE);
}

static void test_living_pet_prevents_a_room_clear(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *pet = new_character(CELL, PET);

  give_quest(owner, CLEAR_CELL);
  add_follower(pet, owner);
  SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);

  kill_character(new_character(CELL, GUARD), pet);
  assert_active(owner, CLEAR_CELL);
}

static void test_grouped_npc_kill_can_credit_its_party(void)
{
  struct char_data *member = new_player(CELL);
  struct char_data *killer = new_character(CELL, PET);
  struct char_data *guard = new_character(CELL, GUARD);

  join_group(member, create_group(killer));
  give_quest(member, CLEAR_CELL);
  /* The NPC killer still occupies the room, so it blocks a clear itself.
   * Have the guard's death trigger move the killer out. */
  move_on_death = killer;
  kill_character(guard, killer);

  assert_completed(member, CLEAR_CELL);
}

static void test_player_death_does_not_credit_the_killers_party(void)
{
  struct char_data *killer = new_player(CELL);
  struct char_data *member = new_player(CELL);

  join_group(member, create_group(killer));
  give_quest(member, RESCUE);
  new_character(CELL, CAPTIVE);

  kill_character(new_player(CELL), killer);
  assert_active(member, RESCUE);
}

static void test_self_inflicted_npc_death_does_not_credit_its_owner(void)
{
  struct char_data *owner = new_player(CELL);
  struct char_data *pet = new_character(CELL, PET);

  give_quest(owner, RESCUE);
  new_character(CELL, CAPTIVE);
  add_follower(pet, owner);
  SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);

  kill_character(pet, pet);
  assert_active(owner, RESCUE);
}

static void test_npc_self_death_preserves_existing_kill_quest_credit(void)
{
  struct char_data *guard = new_character(CELL, GUARD);
  struct char_data *member = new_player(CELL);

  join_group(member, create_group(guard));
  give_quest(member, KILL_THEN_RESCUE);
  new_character(CELL, CAPTIVE);

  kill_character(guard, guard);

  TEST_ASSERT_TRUE(is_complete(member, KILL_THEN_RESCUE));
  assert_active(member, RESCUE);
}

static void test_environmental_death_does_not_credit_bystanders(void)
{
  struct char_data *bystander = new_player(CELL);

  give_quest(bystander, CLEAR_CELL);
  kill_character(new_character(CELL, GUARD), NULL);
  assert_active(bystander, CLEAR_CELL);
}

static void test_death_trigger_moving_the_victim_does_not_move_credit(void)
{
  struct char_data *killer = new_player(CELL);
  struct group_data *group = create_group(killer);
  struct char_data *nearby = new_player(CELL);
  struct char_data *away = new_player(HALL);
  struct char_data *guard = new_character(CELL, GUARD);

  join_group(nearby, group);
  join_group(away, group);
  new_character(CELL, CAPTIVE);
  new_character(HALL, CAPTIVE);
  give_quest(nearby, RESCUE);
  give_quest(away, RESCUE);
  move_on_death = guard;

  kill_character(guard, killer);

  TEST_ASSERT_EQUAL_INT(HALL, IN_ROOM(guard));
  assert_completed(nearby, RESCUE);
  assert_active(away, RESCUE);
}

static void test_kill_step_does_not_also_complete_its_rescue_follow_up(void)
{
  struct char_data *killer = new_player(CELL);
  struct group_data *group = create_group(killer);
  struct char_data *nearby = new_player(CELL);
  struct char_data *same_zone = new_player(HALL);

  join_group(nearby, group);
  join_group(same_zone, group);
  new_character(CELL, CAPTIVE);
  give_quest(nearby, KILL_THEN_RESCUE);
  give_quest(same_zone, KILL_THEN_RESCUE);

  kill_character(new_character(CELL, GUARD), killer);

  TEST_ASSERT_TRUE(is_complete(nearby, KILL_THEN_RESCUE));
  TEST_ASSERT_TRUE(is_complete(same_zone, KILL_THEN_RESCUE));
  assert_active(nearby, RESCUE);
  assert_active(same_zone, RESCUE);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_rescue_credits_nearby_party_only);
  RUN_TEST(test_room_clear_credits_only_living_party_in_the_room);
  RUN_TEST(test_pet_kill_credits_its_owner_and_party_for_rescue);
  RUN_TEST(test_living_pet_prevents_a_room_clear);
  RUN_TEST(test_grouped_npc_kill_can_credit_its_party);
  RUN_TEST(test_player_death_does_not_credit_the_killers_party);
  RUN_TEST(test_self_inflicted_npc_death_does_not_credit_its_owner);
  RUN_TEST(test_npc_self_death_preserves_existing_kill_quest_credit);
  RUN_TEST(test_environmental_death_does_not_credit_bystanders);
  RUN_TEST(test_death_trigger_moving_the_victim_does_not_move_credit);
  RUN_TEST(test_kill_step_does_not_also_complete_its_rescue_follow_up);
  return UNITY_END();
}
