/**
 * @file test_interpreter.c
 * Unit tests for pure string-handling functions in src/interpreter.c:
 *   is_number, is_abbrev, delete_doubledollar, any_one_arg, one_word
 */

#include "unity.h"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"

extern FILE *logfile;

void setUp(void)    { logfile = stderr; }
void tearDown(void) { logfile = NULL; }

/* =========================================================
 * is_number
 * ========================================================= */

void test_is_number_digits_only(void)
{
    TEST_ASSERT_TRUE(is_number("42"));
}

void test_is_number_zero(void)
{
    TEST_ASSERT_TRUE(is_number("0"));
}

void test_is_number_negative(void)
{
    TEST_ASSERT_TRUE(is_number("-5"));
}

void test_is_number_empty_string(void)
{
    TEST_ASSERT_FALSE(is_number(""));
}

void test_is_number_contains_letter(void)
{
    TEST_ASSERT_FALSE(is_number("12x3"));
}

void test_is_number_minus_only(void)
{
    TEST_ASSERT_FALSE(is_number("-"));
}

void test_is_number_float(void)
{
    TEST_ASSERT_FALSE(is_number("3.14"));
}

/* =========================================================
 * is_abbrev
 * ========================================================= */

void test_is_abbrev_exact_match(void)
{
    TEST_ASSERT_TRUE(is_abbrev("north", "north"));
}

void test_is_abbrev_valid_prefix(void)
{
    TEST_ASSERT_TRUE(is_abbrev("nort", "north"));
    TEST_ASSERT_TRUE(is_abbrev("n", "north"));
}

void test_is_abbrev_non_prefix(void)
{
    TEST_ASSERT_FALSE(is_abbrev("south", "north"));
}

void test_is_abbrev_empty_arg1(void)
{
    TEST_ASSERT_FALSE(is_abbrev("", "north"));
}

void test_is_abbrev_arg1_longer_than_arg2(void)
{
    TEST_ASSERT_FALSE(is_abbrev("northward", "north"));
}

void test_is_abbrev_case_insensitive(void)
{
    TEST_ASSERT_TRUE(is_abbrev("Nor", "north"));
    TEST_ASSERT_TRUE(is_abbrev("NOR", "NORTH"));
}

/* =========================================================
 * delete_doubledollar
 * ========================================================= */

void test_delete_doubledollar_no_dollars(void)
{
    char s[] = "hello";
    delete_doubledollar(s);
    TEST_ASSERT_EQUAL_STRING("hello", s);
}

void test_delete_doubledollar_double_at_start(void)
{
    char s[] = "$$hello";
    delete_doubledollar(s);
    TEST_ASSERT_EQUAL_STRING("$hello", s);
}

void test_delete_doubledollar_double_in_middle(void)
{
    char s[] = "hello$$world";
    delete_doubledollar(s);
    TEST_ASSERT_EQUAL_STRING("hello$world", s);
}

void test_delete_doubledollar_four_dollars(void)
{
    char s[] = "$$$$";
    delete_doubledollar(s);
    TEST_ASSERT_EQUAL_STRING("$$", s);
}

void test_delete_doubledollar_single_dollar_unchanged(void)
{
    char s[] = "hello$world";
    delete_doubledollar(s);
    TEST_ASSERT_EQUAL_STRING("hello$world", s);
}

/* =========================================================
 * any_one_arg
 * ========================================================= */

void test_any_one_arg_basic(void)
{
    char first[64];
    char *rest = any_one_arg("hello world", first);
    TEST_ASSERT_EQUAL_STRING("hello", first);
    TEST_ASSERT_EQUAL_STRING(" world", rest);
}

void test_any_one_arg_leading_spaces(void)
{
    char first[64];
    any_one_arg("  hello world", first);
    TEST_ASSERT_EQUAL_STRING("hello", first);
}

void test_any_one_arg_single_word(void)
{
    char first[64];
    char *rest = any_one_arg("hello", first);
    TEST_ASSERT_EQUAL_STRING("hello", first);
    TEST_ASSERT_EQUAL_STRING("", rest);
}

void test_any_one_arg_empty_string(void)
{
    char first[64];
    any_one_arg("", first);
    TEST_ASSERT_EQUAL_STRING("", first);
}

void test_any_one_arg_lowercases(void)
{
    char first[64];
    any_one_arg("HELLO", first);
    TEST_ASSERT_EQUAL_STRING("hello", first);
}

/* =========================================================
 * one_word
 * ========================================================= */

void test_one_word_unquoted_like_any_one_arg(void)
{
    char first[64];
    char *rest = one_word("hello world", first);
    TEST_ASSERT_EQUAL_STRING("hello", first);
    TEST_ASSERT_EQUAL_STRING(" world", rest);
}

void test_one_word_quoted_string(void)
{
    char first[64];
    char *rest = one_word("\"hello world\" rest", first);
    TEST_ASSERT_EQUAL_STRING("hello world", first);
    /* rest points just past the closing quote */
    TEST_ASSERT_EQUAL_STRING(" rest", rest);
}

void test_one_word_empty_quoted(void)
{
    char first[64];
    one_word("\"\" rest", first);
    TEST_ASSERT_EQUAL_STRING("", first);
}

void test_one_word_leading_spaces_skipped(void)
{
    char first[64];
    one_word("  hello", first);
    TEST_ASSERT_EQUAL_STRING("hello", first);
}

/* =========================================================
 * main
 * ========================================================= */

int main(void)
{
    UNITY_BEGIN();

    /* is_number */
    RUN_TEST(test_is_number_digits_only);
    RUN_TEST(test_is_number_zero);
    RUN_TEST(test_is_number_negative);
    RUN_TEST(test_is_number_empty_string);
    RUN_TEST(test_is_number_contains_letter);
    RUN_TEST(test_is_number_minus_only);
    RUN_TEST(test_is_number_float);

    /* is_abbrev */
    RUN_TEST(test_is_abbrev_exact_match);
    RUN_TEST(test_is_abbrev_valid_prefix);
    RUN_TEST(test_is_abbrev_non_prefix);
    RUN_TEST(test_is_abbrev_empty_arg1);
    RUN_TEST(test_is_abbrev_arg1_longer_than_arg2);
    RUN_TEST(test_is_abbrev_case_insensitive);

    /* delete_doubledollar */
    RUN_TEST(test_delete_doubledollar_no_dollars);
    RUN_TEST(test_delete_doubledollar_double_at_start);
    RUN_TEST(test_delete_doubledollar_double_in_middle);
    RUN_TEST(test_delete_doubledollar_four_dollars);
    RUN_TEST(test_delete_doubledollar_single_dollar_unchanged);

    /* any_one_arg */
    RUN_TEST(test_any_one_arg_basic);
    RUN_TEST(test_any_one_arg_leading_spaces);
    RUN_TEST(test_any_one_arg_single_word);
    RUN_TEST(test_any_one_arg_empty_string);
    RUN_TEST(test_any_one_arg_lowercases);

    /* one_word */
    RUN_TEST(test_one_word_unquoted_like_any_one_arg);
    RUN_TEST(test_one_word_quoted_string);
    RUN_TEST(test_one_word_empty_quoted);
    RUN_TEST(test_one_word_leading_spaces_skipped);

    return UNITY_END();
}
