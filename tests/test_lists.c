/**
 * @file test_lists.c
 * Unit tests for the generic list container and iterator lifetime rules.
 */

#include "unity.h"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"

#include <limits.h>
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

static void assert_list_integrity(struct list_data *list)
{
    struct item_data *item;
    struct item_data *previous;
    size_t count;

    TEST_ASSERT_NOT_NULL(list);
    TEST_ASSERT_FALSE(list->pendingFree);

    previous = NULL;
    count = 0;
    for (item = list->pFirstItem; item != NULL; item = item->pNextItem) {
        TEST_ASSERT_EQUAL_PTR(previous, item->pPrevItem);
        TEST_ASSERT_FALSE(item->isRemoved);
        TEST_ASSERT_NOT_NULL(item->pContent);
        previous = item;
        count++;
    }

    TEST_ASSERT_EQUAL_PTR(previous, list->pLastItem);
    TEST_ASSERT_EQUAL_size_t(count, list->iSize);
    if (count == 0) {
        TEST_ASSERT_NULL(list->pFirstItem);
        TEST_ASSERT_NULL(list->pLastItem);
    } else {
        TEST_ASSERT_NULL(list->pFirstItem->pPrevItem);
        TEST_ASSERT_NULL(list->pLastItem->pNextItem);
    }
}

void test_append_remove_and_duplicate_pointer_invariants(void)
{
    struct list_data *list;
    int first;
    int second;

    first = 1;
    second = 2;
    list = create_list();

    add_to_list(&first, list);
    add_to_list(&second, list);
    add_to_list(&first, list);
    assert_list_integrity(list);
    TEST_ASSERT_EQUAL_size_t(3, list->iSize);
    TEST_ASSERT_EQUAL_PTR(&first, list->pFirstItem->pContent);
    TEST_ASSERT_EQUAL_PTR(&first, list->pLastItem->pContent);

    remove_from_list(&first, list);
    assert_list_integrity(list);
    TEST_ASSERT_EQUAL_size_t(2, list->iSize);
    TEST_ASSERT_EQUAL_PTR(&second, list->pFirstItem->pContent);
    TEST_ASSERT_EQUAL_PTR(&first, list->pLastItem->pContent);

    remove_from_list(&first, list);
    remove_from_list(&second, list);
    assert_list_integrity(list);
    free_list(list);
}

void test_simple_iteration_survives_lookup_and_current_removal(void)
{
    struct list_data *list;
    int first;
    int second;
    int third;

    first = 1;
    second = 2;
    third = 3;
    list = create_list();
    add_to_list(&first, list);
    add_to_list(&second, list);
    add_to_list(&third, list);

    TEST_ASSERT_EQUAL_PTR(&first, simple_list(list));
    TEST_ASSERT_NOT_NULL(find_in_list(&second, list));
    TEST_ASSERT_EQUAL_PTR(&second, simple_list(list));

    remove_from_list(&second, list);
    TEST_ASSERT_EQUAL_PTR(&third, simple_list(list));
    TEST_ASSERT_NULL(simple_list(list));
    TEST_ASSERT_EQUAL_size_t(0, list->iIterators);
    assert_list_integrity(list);
    free_list(list);
}

void test_multiple_iterators_survive_current_and_future_removal(void)
{
    struct list_data *list;
    struct iterator_data first_iterator;
    struct iterator_data second_iterator;
    int first;
    int second;
    int third;

    memset(&first_iterator, 0, sizeof(first_iterator));
    memset(&second_iterator, 0, sizeof(second_iterator));
    first = 1;
    second = 2;
    third = 3;
    list = create_list();
    add_to_list(&first, list);
    add_to_list(&second, list);
    add_to_list(&third, list);

    TEST_ASSERT_EQUAL_PTR(&first, merge_iterator(&first_iterator, list));
    TEST_ASSERT_EQUAL_PTR(&first, merge_iterator(&second_iterator, list));
    TEST_ASSERT_EQUAL_size_t(2, list->iIterators);

    remove_from_list(&first, list);
    TEST_ASSERT_EQUAL_size_t(2, list->iSize);
    TEST_ASSERT_NOT_NULL(list->pRemovedItems);
    TEST_ASSERT_EQUAL_PTR(&second, next_in_list(&first_iterator));
    TEST_ASSERT_EQUAL_PTR(&second, next_in_list(&second_iterator));

    remove_from_list(&third, list);
    TEST_ASSERT_NULL(next_in_list(&first_iterator));
    TEST_ASSERT_EQUAL_size_t(1, list->iIterators);
    TEST_ASSERT_NULL(next_in_list(&second_iterator));
    TEST_ASSERT_EQUAL_size_t(0, list->iIterators);
    TEST_ASSERT_NULL(list->pRemovedItems);
    assert_list_integrity(list);

    remove_from_list(&second, list);
    free_list(list);
}

void test_free_waits_for_active_iterators(void)
{
    struct list_data *list;
    struct iterator_data iterator;
    int first;
    int second;

    memset(&iterator, 0, sizeof(iterator));
    first = 1;
    second = 2;
    list = create_list();
    add_to_list(&first, list);
    add_to_list(&second, list);

    TEST_ASSERT_EQUAL_PTR(&first, merge_iterator(&iterator, list));
    free_list(list);
    TEST_ASSERT_TRUE(list->pendingFree);
    TEST_ASSERT_EQUAL_size_t(1, list->iIterators);

    TEST_ASSERT_NULL(next_in_list(&iterator));
    TEST_ASSERT_NULL(iterator.pList);
    TEST_ASSERT_NULL(iterator.pItem);
}

void test_freeing_unrelated_list_preserves_simple_iteration(void)
{
    struct list_data *iterated;
    struct list_data *unrelated;
    int first;
    int second;
    int other;

    first = 1;
    second = 2;
    other = 3;
    iterated = create_list();
    unrelated = create_list();
    add_to_list(&first, iterated);
    add_to_list(&second, iterated);
    add_to_list(&other, unrelated);

    TEST_ASSERT_EQUAL_PTR(&first, simple_list(iterated));
    TEST_ASSERT_EQUAL_size_t(1, iterated->iIterators);
    free_list(unrelated);
    TEST_ASSERT_EQUAL_size_t(1, iterated->iIterators);
    TEST_ASSERT_EQUAL_PTR(&second, simple_list(iterated));
    TEST_ASSERT_NULL(simple_list(iterated));
    TEST_ASSERT_EQUAL_size_t(0, iterated->iIterators);

    free_list(iterated);
}

void test_null_content_is_rejected(void)
{
    struct list_data *list;

    list = create_list();
    add_to_list(NULL, list);
    TEST_ASSERT_EQUAL_size_t(0, list->iSize);
    TEST_ASSERT_NULL(list->pFirstItem);
    free_list(list);
}

void test_randomization_preserves_members_and_consumes_empty_input(void)
{
    struct list_data *registry;
    struct list_data *list;
    struct list_data *randomized;
    int first;
    int second;
    int third;

    registry = create_list();
    global_lists = registry;
    first = 1;
    second = 2;
    third = 3;
    list = create_list();
    add_to_list(&first, list);
    add_to_list(&second, list);
    add_to_list(&third, list);

    randomized = randomize_list(list);
    assert_list_integrity(randomized);
    TEST_ASSERT_EQUAL_size_t(3, randomized->iSize);
    TEST_ASSERT_NOT_NULL(find_in_list(&first, randomized));
    TEST_ASSERT_NOT_NULL(find_in_list(&second, randomized));
    TEST_ASSERT_NOT_NULL(find_in_list(&third, randomized));
    free_list(randomized);

    list = create_list();
    TEST_ASSERT_EQUAL_size_t(1, registry->iSize);
    randomized = randomize_list(list);
    TEST_ASSERT_NULL(randomized);
    TEST_ASSERT_EQUAL_size_t(0, registry->iSize);
    free_list(registry);
}

void test_registry_can_be_destroyed_and_reinitialized(void)
{
    struct list_data *registry;
    struct list_data *first;
    struct list_data *second;

    registry = create_list();
    global_lists = registry;
    first = create_list();
    second = create_list();
    TEST_ASSERT_EQUAL_size_t(2, registry->iSize);

    free_list(first);
    TEST_ASSERT_EQUAL_size_t(1, registry->iSize);
    free_list(registry);
    TEST_ASSERT_NULL(global_lists);
    free_list(second);

    registry = create_list();
    global_lists = registry;
    first = create_list();
    TEST_ASSERT_EQUAL_size_t(1, registry->iSize);
    free_list(first);
    free_list(registry);
}

void test_size_exceeds_legacy_unsigned_short_limit(void)
{
    struct list_data *list;
    size_t count;
    int content;

    content = 1;
    list = create_list();
    for (count = 0; count <= USHRT_MAX; count++)
        add_to_list(&content, list);

    TEST_ASSERT_EQUAL_size_t((size_t)USHRT_MAX + 1, list->iSize);
    assert_list_integrity(list);
    free_list(list);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_append_remove_and_duplicate_pointer_invariants);
    RUN_TEST(test_simple_iteration_survives_lookup_and_current_removal);
    RUN_TEST(test_multiple_iterators_survive_current_and_future_removal);
    RUN_TEST(test_free_waits_for_active_iterators);
    RUN_TEST(test_freeing_unrelated_list_preserves_simple_iteration);
    RUN_TEST(test_null_content_is_rejected);
    RUN_TEST(test_randomization_preserves_members_and_consumes_empty_input);
    RUN_TEST(test_registry_can_be_destroyed_and_reinitialized);
    RUN_TEST(test_size_exceeds_legacy_unsigned_short_limit);

    return UNITY_END();
}
