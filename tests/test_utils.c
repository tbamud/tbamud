/**
 * @file test_utils.c
 * Unit tests for pure / near-pure functions in src/utils.c
 */

#include "unity.h"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"

/* Redirect mud log output so basic_mud_vlog() doesn't print the
 * "SYSERR: Using log() before stream was initialized!" warning. */
extern FILE *logfile;

void setUp(void)    { logfile = stderr; }
void tearDown(void) { logfile = NULL; }

/* =========================================================
 * prune_crlf
 * ========================================================= */

void test_prune_crlf_strips_crlf(void)
{
    char s[] = "hello\r\n";
    prune_crlf(s);
    TEST_ASSERT_EQUAL_STRING("hello", s);
}

void test_prune_crlf_strips_lf_only(void)
{
    char s[] = "hello\n";
    prune_crlf(s);
    TEST_ASSERT_EQUAL_STRING("hello", s);
}

void test_prune_crlf_no_op_on_clean(void)
{
    char s[] = "hello";
    prune_crlf(s);
    TEST_ASSERT_EQUAL_STRING("hello", s);
}

void test_prune_crlf_multiple_trailing(void)
{
    char s[] = "hi\r\n\r\n";
    prune_crlf(s);
    TEST_ASSERT_EQUAL_STRING("hi", s);
}

/* =========================================================
 * str_cmp  (may be an alias for strcasecmp on this platform)
 * ========================================================= */

void test_str_cmp_equal_strings(void)
{
    TEST_ASSERT_EQUAL_INT(0, str_cmp("hello", "hello"));
}

void test_str_cmp_case_insensitive(void)
{
    TEST_ASSERT_EQUAL_INT(0, str_cmp("Hello", "hello"));
    TEST_ASSERT_EQUAL_INT(0, str_cmp("HELLO", "hello"));
}

void test_str_cmp_ordering_less(void)
{
    TEST_ASSERT_LESS_THAN(0, str_cmp("a", "b"));
}

void test_str_cmp_ordering_greater(void)
{
    TEST_ASSERT_GREATER_THAN(0, str_cmp("b", "a"));
}

void test_str_cmp_empty_equal(void)
{
    TEST_ASSERT_EQUAL_INT(0, str_cmp("", ""));
}

/* =========================================================
 * strn_cmp  (may be an alias for strncasecmp on this platform)
 * ========================================================= */

void test_strn_cmp_equal_prefix(void)
{
    TEST_ASSERT_EQUAL_INT(0, strn_cmp("hello", "hello world", 5));
}

void test_strn_cmp_differ_past_n(void)
{
    /* First 5 chars same, so strn_cmp("hello!", "hellox", 5) == 0 */
    TEST_ASSERT_EQUAL_INT(0, strn_cmp("hello!", "hellox", 5));
}

void test_strn_cmp_differ_within_n(void)
{
    TEST_ASSERT_NOT_EQUAL(0, strn_cmp("abc", "xyz", 3));
}

/* =========================================================
 * sprintbit
 * ========================================================= */

void test_sprintbit_no_bits_set(void)
{
    static const char *names[] = { "FLAG_A", "FLAG_B", "\n" };
    char result[256];
    sprintbit(0, names, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING("NOBITS ", result);
}

void test_sprintbit_single_bit(void)
{
    static const char *names[] = { "FLAG_A", "FLAG_B", "\n" };
    char result[256];
    sprintbit(1, names, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING("FLAG_A ", result);
}

void test_sprintbit_multiple_bits(void)
{
    static const char *names[] = { "FLAG_A", "FLAG_B", "\n" };
    char result[256];
    sprintbit(3, names, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING("FLAG_A FLAG_B ", result);
}

void test_sprintbit_undefined_bit(void)
{
    /* Bit 2 is beyond the named array – should produce "UNDEFINED" */
    static const char *names[] = { "FLAG_A", "FLAG_B", "\n" };
    char result[256];
    sprintbit(4, names, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING("UNDEFINED ", result);
}

/* =========================================================
 * sprinttype
 * ========================================================= */

void test_sprinttype_valid_index_zero(void)
{
    static const char *names[] = { "ZERO", "ONE", "\n" };
    char result[64];
    sprinttype(0, names, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING("ZERO", result);
}

void test_sprinttype_valid_index_one(void)
{
    static const char *names[] = { "ZERO", "ONE", "\n" };
    char result[64];
    sprinttype(1, names, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING("ONE", result);
}

void test_sprinttype_out_of_range(void)
{
    static const char *names[] = { "ZERO", "ONE", "\n" };
    char result[64];
    sprinttype(5, names, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING("UNDEFINED", result);
}

/* =========================================================
 * levenshtein_distance
 * ========================================================= */

void test_levenshtein_identical_strings(void)
{
    TEST_ASSERT_EQUAL_INT(0, levenshtein_distance("hello", "hello"));
}

void test_levenshtein_empty_and_nonempty(void)
{
    TEST_ASSERT_EQUAL_INT(5, levenshtein_distance("", "hello"));
    TEST_ASSERT_EQUAL_INT(5, levenshtein_distance("hello", ""));
}

void test_levenshtein_single_insertion(void)
{
    TEST_ASSERT_EQUAL_INT(1, levenshtein_distance("abc", "abcd"));
}

void test_levenshtein_single_deletion(void)
{
    TEST_ASSERT_EQUAL_INT(1, levenshtein_distance("abcd", "abc"));
}

void test_levenshtein_single_substitution(void)
{
    TEST_ASSERT_EQUAL_INT(1, levenshtein_distance("abc", "axc"));
}

void test_levenshtein_both_empty(void)
{
    TEST_ASSERT_EQUAL_INT(0, levenshtein_distance("", ""));
}

/* =========================================================
 * count_color_chars
 * ========================================================= */

void test_count_color_chars_no_codes(void)
{
    TEST_ASSERT_EQUAL_INT(0, count_color_chars("hello"));
}

void test_count_color_chars_empty(void)
{
    TEST_ASSERT_EQUAL_INT(0, count_color_chars(""));
}

void test_count_color_chars_single_color_code(void)
{
    /* "\tR" is a two-char color escape; both are skipped (counted as overhead) */
    TEST_ASSERT_EQUAL_INT(2, count_color_chars("\tR"));
}

void test_count_color_chars_double_tab(void)
{
    /* "\t\t" is an escaped literal tab; costs 1 overhead char */
    TEST_ASSERT_EQUAL_INT(1, count_color_chars("\t\t"));
}

void test_count_color_chars_mixed(void)
{
    /* "\tRhello" → 2 overhead + 0 for "hello" */
    TEST_ASSERT_EQUAL_INT(2, count_color_chars("\tRhello"));
}

/* =========================================================
 * count_non_protocol_chars
 * ========================================================= */

void test_count_non_protocol_chars_plain(void)
{
    TEST_ASSERT_EQUAL_INT(5, count_non_protocol_chars("hello"));
}

void test_count_non_protocol_chars_empty(void)
{
    TEST_ASSERT_EQUAL_INT(0, count_non_protocol_chars(""));
}

void test_count_non_protocol_chars_newlines_skipped(void)
{
    TEST_ASSERT_EQUAL_INT(5, count_non_protocol_chars("\r\nhello"));
}

void test_count_non_protocol_chars_bracket_tag(void)
{
    /* "@[bold]hi" – "@[bold]" is a protocol tag; only "hi" counted */
    TEST_ASSERT_EQUAL_INT(2, count_non_protocol_chars("@[bold]hi"));
}

/* =========================================================
 * atoidx
 * ========================================================= */

void test_atoidx_valid_number(void)
{
    TEST_ASSERT_EQUAL_INT(42, (int)atoidx("42"));
}

void test_atoidx_zero(void)
{
    TEST_ASSERT_EQUAL_INT(0, (int)atoidx("0"));
}

void test_atoidx_negative_returns_nowhere(void)
{
    TEST_ASSERT_EQUAL_INT((int)NOWHERE, (int)atoidx("-1"));
}

void test_atoidx_overflow_returns_nowhere(void)
{
    /* IDXTYPE_MAX is 65535 on this build; a larger value overflows */
    TEST_ASSERT_EQUAL_INT((int)NOWHERE, (int)atoidx("99999999"));
}

/* =========================================================
 * right_trim_whitespace
 * ========================================================= */

void test_right_trim_whitespace_trailing_spaces(void)
{
    char *r = right_trim_whitespace("hello   ");
    TEST_ASSERT_EQUAL_STRING("hello", r);
    free(r);
}

void test_right_trim_whitespace_no_trailing(void)
{
    char *r = right_trim_whitespace("hello");
    TEST_ASSERT_EQUAL_STRING("hello", r);
    free(r);
}

void test_right_trim_whitespace_all_whitespace(void)
{
    char *r = right_trim_whitespace("   ");
    TEST_ASSERT_EQUAL_STRING("", r);
    free(r);
}

void test_right_trim_whitespace_empty(void)
{
    char *r = right_trim_whitespace("");
    TEST_ASSERT_EQUAL_STRING("", r);
    free(r);
}

/* =========================================================
 * remove_from_string
 * ========================================================= */

void test_remove_from_string_word_present(void)
{
    char s[] = "hello world";
    remove_from_string(s, "world");
    /* "world" and the trailing NUL shift left; "hello " remains */
    TEST_ASSERT_EQUAL_STRING("hello ", s);
}

void test_remove_from_string_word_absent(void)
{
    char s[] = "hello world";
    remove_from_string(s, "nope");
    TEST_ASSERT_EQUAL_STRING("hello world", s);
}

void test_remove_from_string_word_at_start(void)
{
    char s[] = "hello world";
    remove_from_string(s, "hello");
    /* "hello" removed; " world" remains */
    TEST_ASSERT_EQUAL_STRING(" world", s);
}

/* =========================================================
 * real_time_passed
 * ========================================================= */

void test_real_time_passed_hours(void)
{
    time_t base = 1000000;
    struct time_info_data *t = real_time_passed(base + 3 * SECS_PER_REAL_HOUR, base);
    TEST_ASSERT_EQUAL_INT(3, t->hours);
    TEST_ASSERT_EQUAL_INT(0, t->day);
}

void test_real_time_passed_days(void)
{
    time_t base = 1000000;
    struct time_info_data *t = real_time_passed(base + 2 * SECS_PER_REAL_DAY + SECS_PER_REAL_HOUR, base);
    TEST_ASSERT_EQUAL_INT(1, t->hours);
    TEST_ASSERT_EQUAL_INT(2, t->day);
}

/* =========================================================
 * mud_time_passed
 * ========================================================= */

void test_mud_time_passed_hours(void)
{
    time_t base = 1000000;
    struct time_info_data *t = mud_time_passed(base + 2 * SECS_PER_MUD_HOUR, base);
    TEST_ASSERT_EQUAL_INT(2, t->hours);
    TEST_ASSERT_EQUAL_INT(0, t->day);
    TEST_ASSERT_EQUAL_INT(0, t->month);
    TEST_ASSERT_EQUAL_INT(0, t->year);
}

void test_mud_time_passed_days(void)
{
    time_t base = 1000000;
    struct time_info_data *t = mud_time_passed(base + SECS_PER_MUD_DAY, base);
    TEST_ASSERT_EQUAL_INT(0, t->hours);
    TEST_ASSERT_EQUAL_INT(1, t->day);
    TEST_ASSERT_EQUAL_INT(0, t->month);
    TEST_ASSERT_EQUAL_INT(0, t->year);
}

void test_mud_time_passed_months(void)
{
    time_t base = 1000000;
    struct time_info_data *t = mud_time_passed(base + SECS_PER_MUD_MONTH, base);
    TEST_ASSERT_EQUAL_INT(0, t->hours);
    TEST_ASSERT_EQUAL_INT(0, t->day);
    TEST_ASSERT_EQUAL_INT(1, t->month);
    TEST_ASSERT_EQUAL_INT(0, t->year);
}

/* =========================================================
 * main
 * ========================================================= */

int main(void)
{
    UNITY_BEGIN();

    /* prune_crlf */
    RUN_TEST(test_prune_crlf_strips_crlf);
    RUN_TEST(test_prune_crlf_strips_lf_only);
    RUN_TEST(test_prune_crlf_no_op_on_clean);
    RUN_TEST(test_prune_crlf_multiple_trailing);

    /* str_cmp */
    RUN_TEST(test_str_cmp_equal_strings);
    RUN_TEST(test_str_cmp_case_insensitive);
    RUN_TEST(test_str_cmp_ordering_less);
    RUN_TEST(test_str_cmp_ordering_greater);
    RUN_TEST(test_str_cmp_empty_equal);

    /* strn_cmp */
    RUN_TEST(test_strn_cmp_equal_prefix);
    RUN_TEST(test_strn_cmp_differ_past_n);
    RUN_TEST(test_strn_cmp_differ_within_n);

    /* sprintbit */
    RUN_TEST(test_sprintbit_no_bits_set);
    RUN_TEST(test_sprintbit_single_bit);
    RUN_TEST(test_sprintbit_multiple_bits);
    RUN_TEST(test_sprintbit_undefined_bit);

    /* sprinttype */
    RUN_TEST(test_sprinttype_valid_index_zero);
    RUN_TEST(test_sprinttype_valid_index_one);
    RUN_TEST(test_sprinttype_out_of_range);

    /* levenshtein_distance */
    RUN_TEST(test_levenshtein_identical_strings);
    RUN_TEST(test_levenshtein_empty_and_nonempty);
    RUN_TEST(test_levenshtein_single_insertion);
    RUN_TEST(test_levenshtein_single_deletion);
    RUN_TEST(test_levenshtein_single_substitution);
    RUN_TEST(test_levenshtein_both_empty);

    /* count_color_chars */
    RUN_TEST(test_count_color_chars_no_codes);
    RUN_TEST(test_count_color_chars_empty);
    RUN_TEST(test_count_color_chars_single_color_code);
    RUN_TEST(test_count_color_chars_double_tab);
    RUN_TEST(test_count_color_chars_mixed);

    /* count_non_protocol_chars */
    RUN_TEST(test_count_non_protocol_chars_plain);
    RUN_TEST(test_count_non_protocol_chars_empty);
    RUN_TEST(test_count_non_protocol_chars_newlines_skipped);
    RUN_TEST(test_count_non_protocol_chars_bracket_tag);

    /* atoidx */
    RUN_TEST(test_atoidx_valid_number);
    RUN_TEST(test_atoidx_zero);
    RUN_TEST(test_atoidx_negative_returns_nowhere);
    RUN_TEST(test_atoidx_overflow_returns_nowhere);

    /* right_trim_whitespace */
    RUN_TEST(test_right_trim_whitespace_trailing_spaces);
    RUN_TEST(test_right_trim_whitespace_no_trailing);
    RUN_TEST(test_right_trim_whitespace_all_whitespace);
    RUN_TEST(test_right_trim_whitespace_empty);

    /* remove_from_string */
    RUN_TEST(test_remove_from_string_word_present);
    RUN_TEST(test_remove_from_string_word_absent);
    RUN_TEST(test_remove_from_string_word_at_start);

    /* real_time_passed */
    RUN_TEST(test_real_time_passed_hours);
    RUN_TEST(test_real_time_passed_days);

    /* mud_time_passed */
    RUN_TEST(test_mud_time_passed_hours);
    RUN_TEST(test_mud_time_passed_days);
    RUN_TEST(test_mud_time_passed_months);

    return UNITY_END();
}
