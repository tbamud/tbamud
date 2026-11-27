#include "unity.h"

/* Exercise the private flight and scuba checks used by do_simple_move(). */
#include "act.movement.c"

static struct char_data player;
static struct obj_data light;
static struct obj_data gear;

void setUp(void)
{
  memset(&player, 0, sizeof(player));
  memset(&light, 0, sizeof(light));
  GET_LEVEL(&player) = 1;
  GET_OBJ_TYPE(&light) = ITEM_LIGHT;
  SET_BIT_AR(GET_OBJ_AFFECT(&light), AFF_FLYING);
  player.carrying = &light;
  light.carried_by = &player;
}

void tearDown(void) {}

static void test_script_granted_light_without_take_grants_inventory_flight(void)
{
  TEST_ASSERT_TRUE(has_flight(&player));
}

static void test_takeable_light_grants_flight_when_equipped(void)
{
  SET_BIT_AR(GET_OBJ_WEAR(&light), ITEM_WEAR_TAKE);
  TEST_ASSERT_FALSE(has_flight(&player));

  player.carrying = NULL;
  light.carried_by = NULL;
  GET_EQ(&player, WEAR_LIGHT) = &light;
  TEST_ASSERT_TRUE(has_flight(&player));
}

/* Replace the light in the inventory with gear of the given type and affect. */
static void carry_gear(int type, int affect)
{
  memset(&gear, 0, sizeof(gear));
  GET_OBJ_TYPE(&gear) = type;
  SET_BIT_AR(GET_OBJ_AFFECT(&gear), affect);
  player.carrying = &gear;
  gear.carried_by = &player;
}

static void test_wearable_gear_must_be_worn(void)
{
  carry_gear(ITEM_WORN, AFF_FLYING);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_FINGER);
  TEST_ASSERT_FALSE(has_flight(&player));
}

/* perform_wear checks only the slot's own bit, so a ring needs no TAKE. */
static void test_wearable_gear_without_take_must_be_worn(void)
{
  carry_gear(ITEM_WORN, AFF_FLYING);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_FINGER);
  TEST_ASSERT_FALSE(has_flight(&player));
}

static void test_weapon_must_be_wielded(void)
{
  carry_gear(ITEM_WEAPON, AFF_FLYING);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_WIELD);
  TEST_ASSERT_FALSE(has_flight(&player));
}

static void test_holdable_gear_must_be_held(void)
{
  carry_gear(ITEM_OTHER, AFF_FLYING);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_HOLD);
  TEST_ASSERT_FALSE(has_flight(&player));
}

/* do_grab lets any wand be held, even without the HOLD bit. */
static void test_wand_must_be_held(void)
{
  carry_gear(ITEM_WAND, AFF_FLYING);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_TAKE);
  TEST_ASSERT_FALSE(has_flight(&player));
}

/* perform_wear refuses the held slot without TAKE, so it works carried. */
static void test_hold_bit_without_take_grants_inventory_flight(void)
{
  carry_gear(ITEM_OTHER, AFF_FLYING);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_HOLD);
  TEST_ASSERT_TRUE(has_flight(&player));
}

static void test_gear_with_no_slot_grants_inventory_flight(void)
{
  carry_gear(ITEM_OTHER, AFF_FLYING);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_TAKE);
  TEST_ASSERT_TRUE(has_flight(&player));
}

/* has_scuba() shares has_flight()'s rule for carried gear. */
static void test_held_scuba_gear_must_be_held(void)
{
  carry_gear(ITEM_OTHER, AFF_SCUBA);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_HOLD);
  TEST_ASSERT_FALSE(has_scuba(&player));
}

static void test_wielded_scuba_gear_must_be_wielded(void)
{
  carry_gear(ITEM_WEAPON, AFF_SCUBA);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_WIELD);
  TEST_ASSERT_FALSE(has_scuba(&player));
}

static void test_scuba_gear_with_no_slot_grants_inventory_breathing(void)
{
  carry_gear(ITEM_OTHER, AFF_SCUBA);
  SET_BIT_AR(GET_OBJ_WEAR(&gear), ITEM_WEAR_TAKE);
  TEST_ASSERT_TRUE(has_scuba(&player));
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_script_granted_light_without_take_grants_inventory_flight);
  RUN_TEST(test_takeable_light_grants_flight_when_equipped);
  RUN_TEST(test_wearable_gear_must_be_worn);
  RUN_TEST(test_wearable_gear_without_take_must_be_worn);
  RUN_TEST(test_weapon_must_be_wielded);
  RUN_TEST(test_holdable_gear_must_be_held);
  RUN_TEST(test_wand_must_be_held);
  RUN_TEST(test_hold_bit_without_take_grants_inventory_flight);
  RUN_TEST(test_gear_with_no_slot_grants_inventory_flight);
  RUN_TEST(test_held_scuba_gear_must_be_held);
  RUN_TEST(test_wielded_scuba_gear_must_be_wielded);
  RUN_TEST(test_scuba_gear_with_no_slot_grants_inventory_breathing);
  return UNITY_END();
}
