#include "unity.h"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "quest.h"
#include "dg_scripts.h"

enum { HALL, CELL };
enum { CAPTIVE, GUARD };
enum { RESCUE_QUEST = 10, RESCUE_POINTS = 5 };

static struct room_data rooms[2];
static struct room_direction_data to_cell, to_hall;
static struct zone_data zone;
static struct index_data mobs[] = {{.vnum = 200}, {.vnum = 300}};
static struct aq_data quest;
static struct player_special_data player_specials;
static struct char_data player, captive, guard;
static bool veto_entry, veto_greet, load_guard, move_player, kill_player;
static int saves;

struct zone_data *zone_table = &zone;
struct aq_data *aquest_table = &quest;
qst_rnum total_quests = 1;
extern FILE *logfile;

int leave_mtrigger(struct char_data *ch, int dir) { return TRUE; }
int leave_wtrigger(struct room_data *room, struct char_data *ch, int dir) { return TRUE; }
int leave_otrigger(struct room_data *room, struct char_data *ch, int dir) { return TRUE; }
int entry_mtrigger(struct char_data *ch) { return TRUE; }
void entry_memory_mtrigger(struct char_data *ch) {}

int enter_wtrigger(struct room_data *room, struct char_data *ch, int dir)
{
  return !veto_entry;
}

int greet_mtrigger(struct char_data *ch, int dir)
{
  return !veto_greet;
}

void greet_memory_mtrigger(struct char_data *ch)
{
  if (load_guard)
    char_to_room(&guard, CELL);
  if (move_player) {
    char_from_room(ch);
    char_to_room(ch, HALL);
  }
  if (kill_player)
    SET_BIT_AR(PLR_FLAGS(ch), PLR_NOTDEADYET);
}

int special(struct char_data *ch, int cmd, char *argument) { return FALSE; }
int House_can_enter(struct char_data *ch, room_vnum house) { return TRUE; }
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg) { return -1; }
void death_cry(struct char_data *ch) {}

/* The fixture grants quest points and counts save requests. */
obj_rnum real_object(obj_vnum vnum) { return NOTHING; }
struct obj_data *read_object(obj_vnum vnum, int type) { return NULL; }
int increase_gold(struct char_data *ch, int amount) { return amount; }
void gain_exp(struct char_data *ch, int amount) {}

void save_char(struct char_data *ch)
{
  saves++;
}

static void begin_rescue(int quantity)
{
  QST_QUANTITY(0) = quantity;
  set_quest(&player, 0);
}

static void assert_pending(int quantity)
{
  TEST_ASSERT_EQUAL_INT(RESCUE_QUEST, GET_QUEST(&player));
  TEST_ASSERT_EQUAL_INT(quantity, GET_QUEST_COUNTER(&player));
  TEST_ASSERT_FALSE(is_complete(&player, RESCUE_QUEST));
  TEST_ASSERT_EQUAL_INT(0, GET_QUESTPOINTS(&player));
  TEST_ASSERT_EQUAL_INT(0, saves);
}

void setUp(void)
{
  logfile = stderr;
  memset(rooms, 0, sizeof(rooms));
  memset(&player_specials, 0, sizeof(player_specials));
  to_cell = (struct room_direction_data){.to_room = CELL};
  to_hall = (struct room_direction_data){.to_room = HALL};
  rooms[HALL].number = 3001;
  rooms[HALL].name = "Hall";
  rooms[HALL].dir_option[NORTH] = &to_cell;
  rooms[CELL].number = 3002;
  rooms[CELL].name = "Cell";
  rooms[CELL].dir_option[SOUTH] = &to_hall;
  zone.min_level = -1;
  world = rooms;
  top_of_world = CELL;
  mob_index = mobs;
  top_of_mobt = GUARD;
  quest = (struct aq_data){
    .vnum = RESCUE_QUEST, .type = AQ_MOB_SAVE, .target = mobs[CAPTIVE].vnum,
    .value = {[0] = RESCUE_POINTS, [4] = -1, [6] = 1}, .done = "Rescued!",
    .obj_reward = NOTHING, .next_quest = NOTHING,
  };
  player = (struct char_data){.in_room = NOWHERE, .player_specials = &player_specials};
  captive = (struct char_data){.in_room = NOWHERE, .nr = CAPTIVE};
  guard = (struct char_data){.in_room = NOWHERE, .nr = GUARD};
  player.player.name = "Rescuer";
  GET_LEVEL(&player) = 1;
  GET_MOVE(&player) = 100;
  GET_POS(&player) = GET_POS(&captive) = GET_POS(&guard) = POS_STANDING;
  SET_BIT_AR(MOB_FLAGS(&captive), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&guard), MOB_ISNPC);
  clear_quest(&player);
  char_to_room(&player, HALL);
  char_to_room(&captive, CELL);
  veto_entry = veto_greet = load_guard = move_player = kill_player = FALSE;
  saves = 0;
}

void tearDown(void)
{
  free(player_specials.saved.completed_quests);
  logfile = NULL;
}

static void test_room_placement_keeps_the_rescue_pending(void)
{
  begin_rescue(1);
  char_from_room(&player);
  char_to_room(&player, CELL);
  assert_pending(1);
}

static void test_entry_veto_keeps_the_rescue_pending(void)
{
  begin_rescue(1);
  veto_entry = TRUE;
  TEST_ASSERT_FALSE(do_simple_move(&player, NORTH, FALSE));
  TEST_ASSERT_EQUAL_INT(HALL, IN_ROOM(&player));
  assert_pending(1);
}

static void test_greet_veto_keeps_the_rescue_pending(void)
{
  begin_rescue(1);
  veto_greet = TRUE;
  TEST_ASSERT_FALSE(do_simple_move(&player, NORTH, FALSE));
  TEST_ASSERT_EQUAL_INT(HALL, IN_ROOM(&player));
  assert_pending(1);
}

static void test_guard_added_by_a_greet_trigger_blocks_the_rescue(void)
{
  begin_rescue(1);
  load_guard = TRUE;
  TEST_ASSERT_TRUE(do_simple_move(&player, NORTH, FALSE));
  TEST_ASSERT_EQUAL_INT(CELL, IN_ROOM(&guard));
  assert_pending(1);
}

static void test_relocated_player_keeps_the_rescue_pending(void)
{
  begin_rescue(1);
  move_player = TRUE;
  TEST_ASSERT_TRUE(do_simple_move(&player, NORTH, FALSE));
  TEST_ASSERT_EQUAL_INT(HALL, IN_ROOM(&player));
  assert_pending(1);
}

static void test_player_marked_for_extraction_keeps_the_rescue_pending(void)
{
  begin_rescue(1);
  kill_player = TRUE;
  TEST_ASSERT_TRUE(do_simple_move(&player, NORTH, FALSE));
  TEST_ASSERT_TRUE(DEAD(&player));
  assert_pending(1);
}

static void test_death_trap_keeps_the_rescue_pending(void)
{
  begin_rescue(1);
  SET_BIT_AR(rooms[CELL].room_flags, ROOM_DEATH);
  TEST_ASSERT_FALSE(do_simple_move(&player, NORTH, FALSE));
  TEST_ASSERT_TRUE(DEAD(&player));
  assert_pending(1);
}

static void test_successful_arrival_completes_the_rescue_once(void)
{
  begin_rescue(1);
  TEST_ASSERT_TRUE(do_simple_move(&player, NORTH, FALSE));
  TEST_ASSERT_EQUAL_INT(CELL, IN_ROOM(&player));
  TEST_ASSERT_EQUAL_INT(NOTHING, GET_QUEST(&player));
  TEST_ASSERT_TRUE(is_complete(&player, RESCUE_QUEST));
  TEST_ASSERT_EQUAL_INT(1, GET_NUM_QUESTS(&player));
  TEST_ASSERT_EQUAL_INT(RESCUE_POINTS, GET_QUESTPOINTS(&player));
  TEST_ASSERT_EQUAL_INT(1, saves);

  TEST_ASSERT_TRUE(do_simple_move(&player, SOUTH, FALSE));
  TEST_ASSERT_TRUE(do_simple_move(&player, NORTH, FALSE));
  TEST_ASSERT_EQUAL_INT(1, GET_NUM_QUESTS(&player));
  TEST_ASSERT_EQUAL_INT(RESCUE_POINTS, GET_QUESTPOINTS(&player));
  TEST_ASSERT_EQUAL_INT(1, saves);
}

static void test_repeat_visits_do_not_spend_multi_target_goals(void)
{
  begin_rescue(2);
  TEST_ASSERT_TRUE(do_simple_move(&player, NORTH, FALSE));
  TEST_ASSERT_TRUE(do_simple_move(&player, SOUTH, FALSE));
  TEST_ASSERT_TRUE(do_simple_move(&player, NORTH, FALSE));
  assert_pending(2);

  /* An earlier kill can leave one goal remaining on a multi-target quest. */
  GET_QUEST_COUNTER(&player) = 1;
  TEST_ASSERT_TRUE(do_simple_move(&player, SOUTH, FALSE));
  TEST_ASSERT_TRUE(do_simple_move(&player, NORTH, FALSE));
  assert_pending(1);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_room_placement_keeps_the_rescue_pending);
  RUN_TEST(test_entry_veto_keeps_the_rescue_pending);
  RUN_TEST(test_greet_veto_keeps_the_rescue_pending);
  RUN_TEST(test_guard_added_by_a_greet_trigger_blocks_the_rescue);
  RUN_TEST(test_relocated_player_keeps_the_rescue_pending);
  RUN_TEST(test_player_marked_for_extraction_keeps_the_rescue_pending);
  RUN_TEST(test_death_trap_keeps_the_rescue_pending);
  RUN_TEST(test_successful_arrival_completes_the_rescue_once);
  RUN_TEST(test_repeat_visits_do_not_spend_multi_target_goals);
  return UNITY_END();
}
