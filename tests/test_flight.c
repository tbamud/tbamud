#include "unity.h"

/* Exercise the same private flight check used by do_simple_move(). */
#include "act.movement.c"

static struct char_data player;
static struct obj_data light;

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

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_script_granted_light_without_take_grants_inventory_flight);
  RUN_TEST(test_takeable_light_grants_flight_when_equipped);
  return UNITY_END();
}
