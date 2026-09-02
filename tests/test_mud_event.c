/**
 * @file test_mud_event.c
 * Unit tests for char_has_mud_event()'s use of the shared simple_list() cursor.
 */

#include "unity.h"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "dg_event.h"
#include "mud_event.h"

#include <string.h>

extern FILE *logfile;

void setUp(void)
{
    logfile = stderr;
    clear_simple_list();
    global_lists = NULL;
    group_list = NULL;
}

void tearDown(void)
{
    clear_simple_list();
    global_lists = NULL;
    group_list = NULL;
    logfile = NULL;
}

/* Give ch an event list holding n mud events carrying ids[], in order.
 * The events are caller-owned storage; nothing here goes through the event
 * queue, so no dg_event scheduling is involved. */
static struct char_data *make_char_with_events(struct char_data *ch,
                                               struct event *ev,
                                               struct mud_event_data *md,
                                               const event_id *ids, int n)
{
    int i;

    memset(ch, 0, sizeof(*ch));
    ch->events = create_list();

    for (i = 0; i < n; i++) {
        md[i].iId = ids[i];
        md[i].pStruct = ch;
        md[i].sVariables = NULL;
        md[i].pEvent = &ev[i];

        ev[i].isMudEvent = TRUE;
        ev[i].event_obj = &md[i];
        ev[i].q_el = NULL;

        add_to_list(&ev[i], ch->events);
    }

    return ch;
}

/* A match leaves the loop through the break. simple_list() shares one cursor,
 * so the list must not still be holding an iterator once the call returns. */
void test_char_has_mud_event_releases_cursor_on_early_exit(void)
{
    struct char_data ch;
    struct event ev[3];
    struct mud_event_data md[3];
    const event_id ids[3] = { eWHIRLWIND, eSPL_DARKNESS, ePROTOCOLS };

    make_char_with_events(&ch, ev, md, ids, 3);

    TEST_ASSERT_NOT_NULL(char_has_mud_event(&ch, eWHIRLWIND));
    TEST_ASSERT_EQUAL_size_t(0, ch.events->iIterators);

    free_list(ch.events);
}

/* The same has to hold for a match on the final element, where the break and
 * natural exhaustion coincide. */
void test_char_has_mud_event_releases_cursor_on_last_element(void)
{
    struct char_data ch;
    struct event ev[3];
    struct mud_event_data md[3];
    const event_id ids[3] = { eWHIRLWIND, eSPL_DARKNESS, ePROTOCOLS };

    make_char_with_events(&ch, ev, md, ids, 3);

    TEST_ASSERT_NOT_NULL(char_has_mud_event(&ch, ePROTOCOLS));
    TEST_ASSERT_EQUAL_size_t(0, ch.events->iIterators);

    free_list(ch.events);
}

/* No match runs the loop to exhaustion, which releases the cursor on its own.
 * Guards against a fix that only covers the break. */
void test_char_has_mud_event_releases_cursor_when_not_found(void)
{
    struct char_data ch;
    struct event ev[3];
    struct mud_event_data md[3];
    const event_id ids[3] = { eWHIRLWIND, eSPL_DARKNESS, ePROTOCOLS };

    make_char_with_events(&ch, ev, md, ids, 3);

    TEST_ASSERT_NULL(char_has_mud_event(&ch, eNULL));
    TEST_ASSERT_EQUAL_size_t(0, ch.events->iIterators);

    free_list(ch.events);
}

/* A cursor left attached to the character's list makes the next simple_list()
 * walk on an unrelated list take the forced-reset recovery path. The walk
 * still yields every element, so assert on the cursor rather than the log. */
void test_unrelated_traversal_after_early_exit(void)
{
    struct char_data ch;
    struct event ev[3];
    struct mud_event_data md[3];
    const event_id ids[3] = { eWHIRLWIND, eSPL_DARKNESS, ePROTOCOLS };
    struct list_data *other;
    int a = 1, b = 2;

    make_char_with_events(&ch, ev, md, ids, 3);

    other = create_list();
    add_to_list(&a, other);
    add_to_list(&b, other);

    TEST_ASSERT_NOT_NULL(char_has_mud_event(&ch, eWHIRLWIND));
    TEST_ASSERT_EQUAL_size_t(0, ch.events->iIterators);

    TEST_ASSERT_EQUAL_PTR(&a, simple_list(other));
    TEST_ASSERT_EQUAL_PTR(&b, simple_list(other));
    TEST_ASSERT_NULL(simple_list(other));

    free_list(other);
    free_list(ch.events);
}

/* Two calls in a row must each start from the beginning. The cursor used to be
 * reset on entry and is now released on exit instead; a second call that
 * silently resumed mid-list would miss everything before where the first
 * one stopped. */
void test_repeated_calls_do_not_resume_mid_list(void)
{
    struct char_data ch;
    struct event ev[3];
    struct mud_event_data md[3];
    const event_id ids[3] = { eWHIRLWIND, eSPL_DARKNESS, ePROTOCOLS };

    make_char_with_events(&ch, ev, md, ids, 3);

    /* eSPL_DARKNESS is second, so the first call stops there. */
    TEST_ASSERT_NOT_NULL(char_has_mud_event(&ch, eSPL_DARKNESS));
    TEST_ASSERT_NOT_NULL(char_has_mud_event(&ch, eSPL_DARKNESS));

    /* eWHIRLWIND is first, and a resumed cursor would never reach back to it. */
    TEST_ASSERT_NOT_NULL(char_has_mud_event(&ch, eWHIRLWIND));

    free_list(ch.events);
}

/* With nothing to walk the shared cursor is never touched at all, so a
 * traversal already in progress elsewhere stays valid across the call. */
void test_early_return_leaves_an_active_traversal_alone(void)
{
    struct char_data no_list, empty;
    struct list_data *other;
    int a = 1, b = 2, c = 3;

    memset(&no_list, 0, sizeof(no_list));
    no_list.events = NULL;

    memset(&empty, 0, sizeof(empty));
    empty.events = create_list();

    other = create_list();
    add_to_list(&a, other);
    add_to_list(&b, other);
    add_to_list(&c, other);

    TEST_ASSERT_EQUAL_PTR(&a, simple_list(other));

    TEST_ASSERT_NULL(char_has_mud_event(&no_list, eWHIRLWIND));
    TEST_ASSERT_NULL(char_has_mud_event(&empty, eWHIRLWIND));

    TEST_ASSERT_EQUAL_PTR(&b, simple_list(other));
    TEST_ASSERT_EQUAL_PTR(&c, simple_list(other));
    TEST_ASSERT_NULL(simple_list(other));

    free_list(other);
    free_list(empty.events);
}

/* A nested call cannot be made safe -- simple_list() has one cursor, so the
 * outer walk gets reset either way. What it can do is say so. Walking without
 * clearing on entry lets simple_list() report the collision instead of having
 * it quietly wiped beforehand. */
void test_nested_call_reports_the_clobbered_traversal(void)
{
    struct char_data ch;
    struct event ev[3];
    struct mud_event_data md[3];
    const event_id ids[3] = { eWHIRLWIND, eSPL_DARKNESS, ePROTOCOLS };
    struct list_data *other;
    FILE *captured;
    char buf[2048];
    size_t n;
    int a = 1, b = 2;

    make_char_with_events(&ch, ev, md, ids, 3);

    other = create_list();
    add_to_list(&a, other);
    add_to_list(&b, other);

    captured = tmpfile();
    TEST_ASSERT_NOT_NULL(captured);
    logfile = captured;

    TEST_ASSERT_EQUAL_PTR(&a, simple_list(other));
    TEST_ASSERT_NOT_NULL(char_has_mud_event(&ch, eWHIRLWIND));

    rewind(captured);
    n = fread(buf, 1, sizeof(buf) - 1, captured);
    buf[n] = 0;

    logfile = stderr;
    fclose(captured);

    TEST_ASSERT_NOT_NULL(strstr(buf, "forced to reset itself"));

    free_list(other);
    free_list(ch.events);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_char_has_mud_event_releases_cursor_on_early_exit);
    RUN_TEST(test_char_has_mud_event_releases_cursor_on_last_element);
    RUN_TEST(test_char_has_mud_event_releases_cursor_when_not_found);
    RUN_TEST(test_unrelated_traversal_after_early_exit);
    RUN_TEST(test_repeated_calls_do_not_resume_mid_list);
    RUN_TEST(test_early_return_leaves_an_active_traversal_alone);
    RUN_TEST(test_nested_call_reports_the_clobbered_traversal);
    return UNITY_END();
}
