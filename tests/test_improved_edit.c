/**
 * @file test_improved_edit.c
 * Unit tests for the string editor's formatter and its search-and-replace.
 *
 * Both take the buffer the player is editing and walk it a character at a
 * time, and both used to assume something about where that buffer ends.
 * format_text() assumed every string finishes with a newline; replace_str()
 * assumed it would reach its own free(). The cases below pin the shapes that
 * caught them out, and the ordinary shapes either side, so a change to the
 * walk has to keep answering the same thing.
 */

#include "unity.h"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "improved-edit.h"

#include <string.h>

extern FILE *logfile;

static struct descriptor_data d;

void setUp(void)
{
    logfile = stderr;
    memset(&d, 0, sizeof(d));
    d.max_str = MAX_STRING_LENGTH;
}

void tearDown(void)
{
    logfile = NULL;
}

/* format_text() takes char ** and reallocates through it, so every case needs
 * a heap copy of its own rather than a literal. Sized exactly, with no slack:
 * the editor's own buffers carry two spare bytes from RECREATE(strlen + 3),
 * and a read that runs into those is still a read off the end of the string.
 * Under a sanitizer the tight allocation is what makes it visible. */
static char *heap_copy(const char *text)
{
    size_t n = strlen(text) + 1;
    char *p = malloc(n);

    TEST_ASSERT_NOT_NULL(p);
    memcpy(p, text, n);
    return p;
}

/* =========================================================
 * format_text -- the ordinary shapes
 * ========================================================= */

/* Everything the editor builds itself ends "\r\n", because string_add()
 * appends one to every line. This is the case that always worked, and it has
 * to keep formatting the same way. */
void test_format_wraps_a_buffer_that_ends_in_a_newline(void)
{
    char *buf = heap_copy("one\r\ntwo\r\n");
    char **p = &buf;

    TEST_ASSERT_EQUAL_INT(1, format_text(p, 0, &d, d.max_str, 1, 999999));
    TEST_ASSERT_EQUAL_STRING("One two\r\n", *p);
    free(*p);
}

void test_format_indents_when_asked(void)
{
    char *buf = heap_copy("one\r\ntwo\r\n");
    char **p = &buf;

    TEST_ASSERT_EQUAL_INT(1, format_text(p, FORMAT_INDENT, &d, d.max_str, 1, 999999));
    TEST_ASSERT_EQUAL_STRING("   One two\r\n", *p);
    free(*p);
}

/* A sentence delimiter at the end is what the ".!?" walk exists for. */
void test_format_keeps_a_trailing_sentence_delimiter(void)
{
    char *buf = heap_copy("hello world.\r\n");
    char **p = &buf;

    TEST_ASSERT_EQUAL_INT(1, format_text(p, 0, &d, d.max_str, 1, 999999));
    TEST_ASSERT_EQUAL_STRING("Hello world.\r\n", *p);
    free(*p);
}

/* =========================================================
 * format_text -- strings that do not end in a newline
 *
 * fread_string() answers one of these whenever the ~ sits on the same line as
 * the text, so anything the editor is seeded with from a world file can be
 * this shape.
 * ========================================================= */

/* The ".!?" and "\n\r" tests each asked strchr() whether the current
 * character is a delimiter without first asking whether there is one. strchr()
 * matches the terminator -- strchr(s, 0) answers s's own NUL -- so at the end
 * of a string both said yes and both then stepped past it. */
void test_format_stops_at_the_end_of_a_buffer_without_a_newline(void)
{
    char *buf = heap_copy("one\r\ntwo");
    char **p = &buf;

    TEST_ASSERT_EQUAL_INT(1, format_text(p, 0, &d, d.max_str, 1, 999999));
    TEST_ASSERT_EQUAL_STRING("One two\r\n", *p);
    free(*p);
}

void test_format_stops_at_a_lone_line_without_a_newline(void)
{
    char *buf = heap_copy("hello world.");
    char **p = &buf;

    TEST_ASSERT_EQUAL_INT(1, format_text(p, 0, &d, d.max_str, 1, 999999));
    TEST_ASSERT_EQUAL_STRING("Hello world.\r\n", *p);
    free(*p);
}

/* Skipping to the first line of a range takes one line off the front at a
 * time. strtok() answers the whole of what is left when there is no newline in
 * it, so on the last line of a string that does not end in one the "are there
 * that many lines?" test passed and strstr() then answered NULL -- which was
 * dereferenced. */
void test_format_reports_a_range_past_the_end_without_a_newline(void)
{
    char *buf = heap_copy("one\r\ntwo");
    char **p = &buf;

    TEST_ASSERT_EQUAL_INT(0, format_text(p, 0, &d, d.max_str, 5, 5));
    free(*p);
}

/* The same range on the same text with the newline the editor would have put
 * there. This one always answered 0; it is here so the two cannot drift. */
void test_format_reports_a_range_past_the_end_with_a_newline(void)
{
    char *buf = heap_copy("one\r\ntwo\r\n");
    char **p = &buf;

    TEST_ASSERT_EQUAL_INT(0, format_text(p, 0, &d, d.max_str, 5, 5));
    free(*p);
}

/* A range that does fit still formats from the line asked for. */
void test_format_honours_a_range_that_fits(void)
{
    char *buf = heap_copy("one\r\ntwo\r\nthree\r\n");
    char **p = &buf;

    TEST_ASSERT_EQUAL_INT(1, format_text(p, 0, &d, d.max_str, 2, 2));
    TEST_ASSERT_EQUAL_STRING("one\r\nTwo\r\nthree\r\n", *p);
    free(*p);
}

/* =========================================================
 * replace_str
 * ========================================================= */

/* The working buffer is allocated before the search and handed over after it.
 * When nothing matched, the early return used to sit above the free(), losing
 * max_size bytes -- the whole editor buffer -- on every /r that found nothing.
 * A unit test cannot see the leak; build the suite with -fsanitize=address and
 * this case reports it. What it can pin is that the answer and the string are
 * both left alone. */
void test_replace_str_reports_nothing_when_the_pattern_is_absent(void)
{
    char *buf = heap_copy("the quick brown fox\r\n");
    char **p = &buf;

    TEST_ASSERT_EQUAL_INT(0, replace_str(p, "zzzz", "yyyy", 0, 4096));
    TEST_ASSERT_EQUAL_STRING("the quick brown fox\r\n", *p);
    free(*p);
}

void test_replace_str_replaces_the_first_occurrence(void)
{
    char *buf = heap_copy("fox fox fox");
    char **p = &buf;

    TEST_ASSERT_EQUAL_INT(1, replace_str(p, "fox", "cat", 0, 4096));
    TEST_ASSERT_EQUAL_STRING("cat fox fox", *p);
    free(*p);
}

/* Both cases above match at offset 0, which is the one offset that makes the
 * single-replacement path's length arithmetic come out right by accident:
 * len is the distance to the match, and at offset 0 it is zero however it
 * is computed.  Match in the middle so the prefix copy is exercised. */
void test_replace_str_keeps_the_text_either_side_of_a_match(void)
{
    char *buf = heap_copy("the quick brown fox jumps");
    char **p = &buf;

    TEST_ASSERT_EQUAL_INT(1, replace_str(p, "brown", "red", 0, 4096));
    TEST_ASSERT_EQUAL_STRING("the quick red fox jumps", *p);
    free(*p);
}

void test_replace_str_replaces_every_occurrence_when_asked(void)
{
    char *buf = heap_copy("fox fox fox");
    char **p = &buf;

    TEST_ASSERT_EQUAL_INT(3, replace_str(p, "fox", "cat", 1, 4096));
    TEST_ASSERT_EQUAL_STRING("cat cat cat", *p);
    free(*p);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_format_wraps_a_buffer_that_ends_in_a_newline);
    RUN_TEST(test_format_indents_when_asked);
    RUN_TEST(test_format_keeps_a_trailing_sentence_delimiter);

    RUN_TEST(test_format_stops_at_the_end_of_a_buffer_without_a_newline);
    RUN_TEST(test_format_stops_at_a_lone_line_without_a_newline);
    RUN_TEST(test_format_reports_a_range_past_the_end_without_a_newline);
    RUN_TEST(test_format_reports_a_range_past_the_end_with_a_newline);
    RUN_TEST(test_format_honours_a_range_that_fits);

    RUN_TEST(test_replace_str_reports_nothing_when_the_pattern_is_absent);
    RUN_TEST(test_replace_str_replaces_the_first_occurrence);
    RUN_TEST(test_replace_str_keeps_the_text_either_side_of_a_match);
    RUN_TEST(test_replace_str_replaces_every_occurrence_when_asked);

    return UNITY_END();
}
