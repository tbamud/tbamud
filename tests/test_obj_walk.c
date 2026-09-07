/* Object-list iteration must survive extraction of the current object,
 * its successors, and recursively extracted container contents. */
#include "unity.h"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "handler.h"

extern struct obj_data *object_list;
extern FILE *logfile;

/* Unlike the shared no-op stub, really release objects so sanitizers can
 * catch a walk that continues through an extracted object. */
void free_obj(struct obj_data *obj)
{
    free(obj);
}

void setUp(void)
{
    logfile = stderr;
    object_list = NULL;
    protect_obj_walk(NULL);
}

void tearDown(void)
{
    protect_obj_walk(NULL);
    while (object_list)
        extract_obj(object_list);
    logfile = NULL;
}

static struct obj_data *new_object(void)
{
    struct obj_data *obj = calloc(1, sizeof(*obj));

    TEST_ASSERT_NOT_NULL(obj);
    IN_ROOM(obj) = NOWHERE;
    GET_OBJ_RNUM(obj) = NOTHING;
    obj->worn_on = NOWHERE;
    obj->next = object_list;
    object_list = obj;
    return obj;
}

static void test_extract_current_during_walk(void)
{
    struct obj_data *obj, *next = NULL;
    struct obj_data **saved;
    int visited = 0;

    new_object();
    new_object();
    new_object();
    saved = protect_obj_walk(&next);
    for (obj = object_list; obj; obj = next) {
        next = obj->next;
        ++visited;
        extract_obj(obj);
    }
    protect_obj_walk(saved);
    TEST_ASSERT_EQUAL_INT(3, visited);
    TEST_ASSERT_NULL(object_list);
}

static void test_extract_successive_pending_objects(void)
{
    struct obj_data *last = new_object();
    struct obj_data *second = new_object();
    struct obj_data *first = new_object();
    struct obj_data *current = new_object();
    struct obj_data *next = first;
    struct obj_data **saved = protect_obj_walk(&next);

    extract_obj(first);
    TEST_ASSERT_EQUAL_PTR(second, next);
    extract_obj(second);
    TEST_ASSERT_EQUAL_PTR(last, next);
    extract_obj(last);
    TEST_ASSERT_NULL(next);
    TEST_ASSERT_NULL(current->next);
    protect_obj_walk(saved);
}

static void test_recursive_extraction_advances_past_container(void)
{
    struct obj_data *last = new_object();
    struct obj_data *container = new_object();
    struct obj_data *child = new_object();
    struct obj_data *current = new_object();
    struct obj_data *next = child;
    struct obj_data **saved;

    /* List order is current, child, container, last.  Extracting the
     * container first advances next to the container, then past it. */
    obj_to_obj(child, container);
    saved = protect_obj_walk(&next);
    extract_obj(container);
    TEST_ASSERT_EQUAL_PTR(last, next);
    TEST_ASSERT_EQUAL_PTR(last, current->next);
    protect_obj_walk(saved);
}

static void test_restored_registration_receives_extractions(void)
{
    struct obj_data *last = new_object();
    struct obj_data *first = new_object();
    struct obj_data *next = first, *temporary = NULL;
    struct obj_data **saved = protect_obj_walk(&next);
    struct obj_data **outer = protect_obj_walk(&temporary);

    /* No extraction occurs during the temporary registration: nested
     * walks with concurrent extraction are outside this API's contract. */
    TEST_ASSERT_EQUAL_PTR(&next, outer);
    protect_obj_walk(outer);
    extract_obj(first);
    TEST_ASSERT_EQUAL_PTR(last, next);
    protect_obj_walk(saved);
    TEST_ASSERT_NULL(protect_obj_walk(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_extract_current_during_walk);
    RUN_TEST(test_extract_successive_pending_objects);
    RUN_TEST(test_recursive_extraction_advances_past_container);
    RUN_TEST(test_restored_registration_receives_extractions);
    return UNITY_END();
}
