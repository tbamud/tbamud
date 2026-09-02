/**
 * @file test_handler_prefix.c
 * Unit tests for the keyword-prefix helpers in src/handler.c:
 * get_number() and skip_number().
 */

#include "unity.h"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "handler.h"

extern FILE *logfile;

void setUp(void)    { logfile = stderr; }
void tearDown(void) { logfile = NULL; }

/* get_number() rewrites its argument in place, so every case needs a
 * writable buffer, not a literal. */
static int num_of(char *buf)
{
  char *p = buf;
  return get_number(&p);
}

void test_bare_keyword_is_one(void)
{
  char s[MAX_INPUT_LENGTH] = "sword";
  TEST_ASSERT_EQUAL_INT(1, num_of(s));
  TEST_ASSERT_EQUAL_STRING("sword", s);
}

void test_numeric_prefix_is_stripped(void)
{
  char s[MAX_INPUT_LENGTH] = "2.sword";
  TEST_ASSERT_EQUAL_INT(2, num_of(s));
  TEST_ASSERT_EQUAL_STRING("sword", s);
}

void test_zero_prefix_selects_player_form(void)
{
  char s[MAX_INPUT_LENGTH] = "0.rumble";
  TEST_ASSERT_EQUAL_INT(0, num_of(s));
  TEST_ASSERT_EQUAL_STRING("rumble", s);
}

void test_non_numeric_prefix_is_rejected(void)
{
  char s[MAX_INPUT_LENGTH] = "abc.sword";
  TEST_ASSERT_EQUAL_INT(0, num_of(s));
}

void test_last_prefix_returns_find_index_last(void)
{
  char s[MAX_INPUT_LENGTH] = "last.corpse";
  TEST_ASSERT_EQUAL_INT(FIND_INDEX_LAST, num_of(s));
  TEST_ASSERT_EQUAL_STRING("corpse", s);
}

void test_last_prefix_is_case_insensitive(void)
{
  char s[MAX_INPUT_LENGTH] = "LAST.corpse";
  TEST_ASSERT_EQUAL_INT(FIND_INDEX_LAST, num_of(s));
  char t[MAX_INPUT_LENGTH] = "Last.corpse";
  TEST_ASSERT_EQUAL_INT(FIND_INDEX_LAST, num_of(t));
}

/* FIND_INDEX_LAST must never be mistaken for a real count: the searches
 * treat 0 as "not a number" and count down from anything positive. */
void test_find_index_last_cannot_collide_with_a_count(void)
{
  TEST_ASSERT_TRUE(FIND_INDEX_LAST < 0);
  char s[MAX_INPUT_LENGTH] = "1.sword";
  TEST_ASSERT_NOT_EQUAL_INT(FIND_INDEX_LAST, num_of(s));
}

/* "last." with nothing after it leaves an empty keyword; isname() rejects
 * it, so the searches find nothing rather than matching everything. */
void test_last_with_empty_keyword_leaves_empty_name(void)
{
  char s[MAX_INPUT_LENGTH] = "last.";
  TEST_ASSERT_EQUAL_INT(FIND_INDEX_LAST, num_of(s));
  TEST_ASSERT_EQUAL_STRING("", s);
  TEST_ASSERT_FALSE(isname(s, "corpse sword"));
}

/* =========================================================
 * skip_number  -- the keyword half, for error messages
 * ========================================================= */

void test_skip_number_leaves_a_bare_keyword(void)
{
    TEST_ASSERT_EQUAL_STRING("corpse", skip_number("corpse"));
}

void test_skip_number_strips_last_and_digits(void)
{
    TEST_ASSERT_EQUAL_STRING("corpse", skip_number("last.corpse"));
    TEST_ASSERT_EQUAL_STRING("corpse", skip_number("LAST.corpse"));
    TEST_ASSERT_EQUAL_STRING("sword",  skip_number("2.sword"));
    TEST_ASSERT_EQUAL_STRING("rumble", skip_number("0.rumble"));
}

/* A prefix get_number() would not have consumed is the players own
 * typo; echoing it as the bare keyword would hide what they typed. */
void test_skip_number_keeps_a_prefix_get_number_rejects(void)
{
    TEST_ASSERT_EQUAL_STRING("abc.sword",  skip_number("abc.sword"));
    TEST_ASSERT_EQUAL_STRING("las.sword",  skip_number("las.sword"));
    TEST_ASSERT_EQUAL_STRING("lastt.sword", skip_number("lastt.sword"));
    TEST_ASSERT_EQUAL_STRING(".sword",     skip_number(".sword"));
}

/* Nothing after the dot leaves no keyword worth showing. */
void test_skip_number_keeps_a_prefix_with_no_keyword(void)
{
    TEST_ASSERT_EQUAL_STRING("last.", skip_number("last."));
    TEST_ASSERT_EQUAL_STRING("2.",    skip_number("2."));
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_bare_keyword_is_one);
  RUN_TEST(test_numeric_prefix_is_stripped);
  RUN_TEST(test_zero_prefix_selects_player_form);
  RUN_TEST(test_non_numeric_prefix_is_rejected);
  RUN_TEST(test_last_prefix_returns_find_index_last);
  RUN_TEST(test_last_prefix_is_case_insensitive);
  RUN_TEST(test_find_index_last_cannot_collide_with_a_count);
  RUN_TEST(test_last_with_empty_keyword_leaves_empty_name);
  RUN_TEST(test_skip_number_leaves_a_bare_keyword);
  RUN_TEST(test_skip_number_strips_last_and_digits);
  RUN_TEST(test_skip_number_keeps_a_prefix_get_number_rejects);
  RUN_TEST(test_skip_number_keeps_a_prefix_with_no_keyword);
  return UNITY_END();
}
