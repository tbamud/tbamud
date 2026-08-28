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

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_char_has_mud_event_releases_cursor_on_early_exit);
    RUN_TEST(test_char_has_mud_event_releases_cursor_on_last_element);
    RUN_TEST(test_char_has_mud_event_releases_cursor_when_not_found);
    RUN_TEST(test_unrelated_traversal_after_early_exit);
    return UNITY_END();
}
