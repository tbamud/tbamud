/* The shop "list" and "identify" commands.  A search lists every kind of item
 * it matches, including the last one in the shopkeeper's inventory.  Quest
 * items are priced at their cost in quest points, the amount "buy" charges;
 * everything else is priced in gold with the shop's markup. */
#include <stdarg.h>

#include "unity.h"

/* Reach the private shopping_list() and shopping_identify(). */
#include "shop.c"

/* db.c is not linked. */
struct shop_data *shop_index = NULL;
int top_shop = -1;
struct time_info_data time_info;

/* spell_parser.c is not linked; identify names the spell in a staff. */
const char *skill_name(int num) { (void)num; return "!UNUSED!"; }

/* What the buyer is shown, from both send_to_char() and the pager. */
static char shown[MAX_STRING_LENGTH * 2];

size_t send_to_char(struct char_data *ch, const char *messg, ...)
{
  size_t len = strlen(shown);
  va_list args;

  (void)ch;
  va_start(args, messg);
  vsnprintf(shown + len, sizeof(shown) - len, messg, args);
  va_end(args);
  return strlen(shown) - len;
}

void page_string(struct descriptor_data *d, char *str, int keep_internal)
{
  size_t len = strlen(shown);

  (void)d;
  (void)keep_internal;
  snprintf(shown + len, sizeof(shown) - len, "%s", str);
}

#define MAX_STOCK 1000

static struct room_data shop_room;
static struct shop_data shop;
static obj_vnum no_products[] = { NOTHING };
static room_vnum no_rooms[] = { NOWHERE };
static char no_such_item[] = "%s Sorry, I haven't got exactly that item.";
static char buyer_name[] = "Buyer";
static struct char_data keeper, buyer;
static struct player_special_data buyer_specials;
static struct obj_data stock[MAX_STOCK];
static int num_stock;

void setUp(void)
{
  logfile = stderr;
  memset(shown, 0, sizeof(shown));

  /* One lit room. */
  memset(&shop_room, 0, sizeof(shop_room));
  shop_room.sector_type = SECT_INSIDE;
  world = &shop_room;
  top_of_world = 0;

  /* Open all day, with a 50% markup on gold prices, and no products, so
   * every item shows how many are in stock. */
  memset(&shop, 0, sizeof(shop));
  shop.producing = no_products;
  shop.in_room = no_rooms;
  shop.profit_buy = 1.5;
  shop.profit_sell = 0.5;
  shop.no_such_item1 = no_such_item;
  shop.close1 = 28;
  shop_index = &shop;
  top_shop = 0;
  time_info.hours = 12;

  /* Equal charisma, so the markup is the only change to a gold price. */
  memset(&keeper, 0, sizeof(keeper));
  SET_BIT_AR(MOB_FLAGS(&keeper), MOB_ISNPC);
  keeper.player_specials = &dummy_mob;
  keeper.aff_abils.cha = 11;
  IN_ROOM(&keeper) = 0;

  memset(&buyer, 0, sizeof(buyer));
  memset(&buyer_specials, 0, sizeof(buyer_specials));
  buyer.player_specials = &buyer_specials;
  buyer.player.name = buyer_name;
  buyer.aff_abils.cha = 11;
  GET_LEVEL(&buyer) = 1;
  IN_ROOM(&buyer) = 0;

  memset(stock, 0, sizeof(stock));
  num_stock = 0;
}

void tearDown(void) {}

/* Put an item at the end of the shopkeeper's inventory.  Items with the same
 * rnum and cost are the same kind of item and are listed together. */
static struct obj_data *stock_item(obj_rnum rnum, const char *keywords,
                                   const char *short_desc, int cost, bool quest)
{
  struct obj_data *obj;

  TEST_ASSERT_TRUE_MESSAGE(num_stock < MAX_STOCK, "MAX_STOCK is too small");
  obj = &stock[num_stock];
  obj->item_number = rnum;
  obj->name = (char *)keywords;
  obj->short_description = (char *)short_desc;
  GET_OBJ_TYPE(obj) = ITEM_OTHER;
  GET_OBJ_COST(obj) = cost;
  if (quest)
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_QUEST);
  obj->carried_by = &keeper;
  if (num_stock > 0)
    stock[num_stock - 1].next_content = obj;
  else
    keeper.carrying = obj;
  num_stock++;
  return obj;
}

static void list(const char *search)
{
  char arg[MAX_INPUT_LENGTH];

  strlcpy(arg, search, sizeof(arg));
  shopping_list(arg, &buyer, &keeper, 0);
}

static void identify(const char *search)
{
  char arg[MAX_INPUT_LENGTH];

  strlcpy(arg, search, sizeof(arg));
  shopping_identify(arg, &buyer, &keeper, 0);
}

/* The list line for item number n, without its line ending, or NULL if the
 * list did not show item n. */
static const char *listed(int n)
{
  static char line[256];
  char prefix[16];
  const char *start, *end;

  snprintf(prefix, sizeof(prefix), "\n %2d) ", n);
  if (!(start = strstr(shown, prefix)))
    return NULL;
  start++;
  if (!(end = strstr(start, "\r\n")))
    end = start + strlen(start);
  snprintf(line, sizeof(line), "%.*s", (int)(end - start), start);
  return line;
}

static void assert_listed(int n, const char *item, const char *price)
{
  const char *line = listed(n);
  char msg[128];
  size_t len, plen = strlen(price);

  snprintf(msg, sizeof(msg), "item %d (%s) was not listed", n, item);
  TEST_ASSERT_NOT_NULL_MESSAGE(line, msg);
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(line, item), line);
  len = strlen(line);
  TEST_ASSERT_TRUE_MESSAGE(len >= plen && !strcmp(line + len - plen, price), line);
}

/* How many of item n the list says are in stock. */
static int listed_count(int n)
{
  const char *line = listed(n);
  int number, count;

  TEST_ASSERT_NOT_NULL(line);
  TEST_ASSERT_EQUAL_INT(2, sscanf(line, " %d) %d", &number, &count));
  return count;
}

static void test_a_search_matching_only_the_last_item_lists_it(void)
{
  stock_item(0, "sword long", "a long sword", 100, FALSE);
  stock_item(1, "shield round", "a round shield", 60, FALSE);
  stock_item(2, "staff gnarled", "a gnarled staff", 700, FALSE);

  list("staff");

  TEST_ASSERT_NULL_MESSAGE(strstr(shown, "none of those"), shown);
  TEST_ASSERT_NULL(listed(1));
  TEST_ASSERT_NULL(listed(2));
  assert_listed(3, "A gnarled staff", " 1050");
}

static void test_a_search_lists_each_match_at_its_place_in_the_list(void)
{
  stock_item(0, "sword long", "a long sword", 100, FALSE);
  stock_item(1, "staff gnarled", "a gnarled staff", 700, FALSE);
  stock_item(2, "wand gray", "a gray wand", 300, FALSE);
  stock_item(3, "staff oak", "an oak staff", 200, FALSE);

  list("staff");

  TEST_ASSERT_NULL(listed(1));
  assert_listed(2, "A gnarled staff", " 1050");
  TEST_ASSERT_NULL(listed(3));
  assert_listed(4, "An oak staff", " 300");
}

static void test_a_search_with_no_match_says_none_are_for_sale(void)
{
  stock_item(0, "sword long", "a long sword", 100, FALSE);
  stock_item(1, "shield round", "a round shield", 60, FALSE);

  list("staff");

  TEST_ASSERT_EQUAL_STRING("Presently, none of those are for sale.\r\n", shown);
}

static void test_an_empty_shop_has_nothing_for_sale(void)
{
  list("");

  TEST_ASSERT_EQUAL_STRING("Currently, there is nothing for sale.\r\n", shown);
}

static void test_items_without_a_price_are_left_off_the_end_of_the_list(void)
{
  stock_item(0, "sword long", "a long sword", 100, FALSE);
  stock_item(1, "rock", "a rock", 0, FALSE);

  list("");

  assert_listed(1, "A long sword", " 150");
  TEST_ASSERT_NULL(listed(2));
}

static void test_identical_items_at_the_end_of_the_list_are_listed_once(void)
{
  stock_item(0, "sword long", "a long sword", 100, FALSE);
  stock_item(1, "potion red", "a red potion", 50, FALSE);
  stock_item(1, "potion red", "a red potion", 50, FALSE);
  stock_item(1, "potion red", "a red potion", 50, FALSE);

  list("");

  TEST_ASSERT_EQUAL_INT(1, listed_count(1));
  TEST_ASSERT_EQUAL_INT(3, listed_count(2));
  assert_listed(2, "A red potion", " 75");
  TEST_ASSERT_NULL(listed(3));
}

static void test_gold_items_are_listed_with_the_shops_markup(void)
{
  stock_item(0, "sword long", "a long sword", 100, FALSE);

  list("");

  assert_listed(1, "A long sword", " 150");
  TEST_ASSERT_NULL(strstr(shown, "quest points"));
}

static void test_quest_items_are_listed_at_their_quest_point_cost(void)
{
  stock_item(0, "staff gnarled", "a gnarled staff", 700, TRUE);
  stock_item(1, "sword long", "a long sword", 100, FALSE);

  list("");

  assert_listed(1, "A gnarled staff", " 700 qp");
  assert_listed(2, "A long sword", " 150");
  TEST_ASSERT_NOT_NULL(strstr(shown, "Items flagged \"qp\" require quest points to purchase."));
}

static void test_a_quest_item_at_the_end_of_the_list_gets_the_quest_point_note(void)
{
  stock_item(0, "sword long", "a long sword", 100, FALSE);
  stock_item(1, "staff gnarled", "a gnarled staff", 700, TRUE);

  list("staff");

  assert_listed(2, "A gnarled staff", " 700 qp");
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(shown, "Items flagged \"qp\" require quest points to purchase."), shown);
}

static void test_a_search_excluding_quest_items_omits_the_quest_point_note(void)
{
  stock_item(0, "staff gnarled", "a gnarled staff", 700, TRUE);
  stock_item(1, "sword long", "a long sword", 100, FALSE);

  list("sword");

  TEST_ASSERT_NULL(listed(1));
  assert_listed(2, "A long sword", " 150");
  TEST_ASSERT_NULL_MESSAGE(strstr(shown, "quest points"), shown);
}

static void test_charisma_does_not_change_a_quest_items_price(void)
{
  struct char_data *ch = &buyer;   /* for QYEL and QNRM */
  const char *sword_line;
  char expected[64];

  shop.profit_buy = 1.0;
  buyer.aff_abils.cha = 18;
  stock_item(0, "staff gnarled", "a gnarled staff", 700, TRUE);
  stock_item(1, "sword long", "a long sword", 700, FALSE);

  list("");

  assert_listed(1, "A gnarled staff", " 700 qp");
  sword_line = listed(2);
  TEST_ASSERT_NOT_NULL(sword_line);
  TEST_ASSERT_NULL_MESSAGE(strstr(sword_line, " 700"), sword_line);

  memset(shown, 0, sizeof(shown));
  snprintf(expected, sizeof(expected), "Cost to Buy: %s700%s qp\r\n", QYEL, QNRM);

  identify("staff");

  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(shown, expected), shown);
}

static void test_a_list_longer_than_the_page_stops_when_the_page_is_full(void)
{
  int i;

  for (i = 0; i < MAX_STOCK; i++)
    stock_item(i, "scroll", "a scroll", 10 + i, FALSE);

  list("scroll");

  TEST_ASSERT_EQUAL_size_t(MAX_STRING_LENGTH - 1, strlen(shown));
  assert_listed(1, "A scroll", " 15");
  TEST_ASSERT_NULL(strstr(shown, "none of those"));
}

static void test_identify_prices_a_quest_item_in_quest_points(void)
{
  struct char_data *ch = &buyer;   /* for QYEL and QNRM */
  char expected[64];

  stock_item(0, "staff gnarled", "a gnarled staff", 700, TRUE);
  snprintf(expected, sizeof(expected), "Cost to Buy: %s700%s qp\r\n", QYEL, QNRM);

  identify("staff");

  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(shown, expected), shown);
}

static void test_identify_prices_a_gold_item_with_the_shops_markup(void)
{
  struct char_data *ch = &buyer;   /* for QYEL and QNRM */
  char expected[64];

  stock_item(0, "sword long", "a long sword", 100, FALSE);
  snprintf(expected, sizeof(expected), "Cost to Buy: %s150%s\r\n", QYEL, QNRM);

  identify("sword");

  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(shown, expected), shown);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_a_search_matching_only_the_last_item_lists_it);
  RUN_TEST(test_a_search_lists_each_match_at_its_place_in_the_list);
  RUN_TEST(test_a_search_with_no_match_says_none_are_for_sale);
  RUN_TEST(test_an_empty_shop_has_nothing_for_sale);
  RUN_TEST(test_items_without_a_price_are_left_off_the_end_of_the_list);
  RUN_TEST(test_identical_items_at_the_end_of_the_list_are_listed_once);
  RUN_TEST(test_gold_items_are_listed_with_the_shops_markup);
  RUN_TEST(test_quest_items_are_listed_at_their_quest_point_cost);
  RUN_TEST(test_a_quest_item_at_the_end_of_the_list_gets_the_quest_point_note);
  RUN_TEST(test_a_search_excluding_quest_items_omits_the_quest_point_note);
  RUN_TEST(test_charisma_does_not_change_a_quest_items_price);
  RUN_TEST(test_a_list_longer_than_the_page_stops_when_the_page_is_full);
  RUN_TEST(test_identify_prices_a_quest_item_in_quest_points);
  RUN_TEST(test_identify_prices_a_gold_item_with_the_shops_markup);
  return UNITY_END();
}
