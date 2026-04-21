/**
 * @file test_random.c
 * Unit tests for src/random.c and the random-number helpers in src/utils.c
 * (rand_number, dice).
 */

#include "unity.h"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"

extern FILE *logfile;

void setUp(void)    { logfile = stderr; }
void tearDown(void) { logfile = NULL; }

/* =========================================================
 * circle_srandom / circle_random — deterministic sequence
 *
 * The Park-Miller generator with seed s produces:
 *   G(s) = (16807 * s) mod 2147483647
 * Precomputed for seed=1:
 *   call 1 → 16807
 *   call 2 → 282475249
 *   call 3 → 1622136673
 * ========================================================= */

void test_circle_random_deterministic_first(void)
{
    circle_srandom(1);
    TEST_ASSERT_EQUAL_UINT32(16807UL, circle_random());
}

void test_circle_random_deterministic_second(void)
{
    circle_srandom(1);
    circle_random();                           /* discard first */
    TEST_ASSERT_EQUAL_UINT32(282475249UL, circle_random());
}

void test_circle_random_deterministic_third(void)
{
    circle_srandom(1);
    circle_random();
    circle_random();
    TEST_ASSERT_EQUAL_UINT32(1622650073UL, circle_random());
}

void test_circle_random_same_seed_same_sequence(void)
{
    circle_srandom(42);
    unsigned long a = circle_random();
    unsigned long b = circle_random();

    circle_srandom(42);
    TEST_ASSERT_EQUAL_UINT32(a, circle_random());
    TEST_ASSERT_EQUAL_UINT32(b, circle_random());
}

/* =========================================================
 * rand_number — result always in [from, to]
 * ========================================================= */

void test_rand_number_in_range(void)
{
    int i;
    circle_srandom(12345);
    for (i = 0; i < 200; i++) {
        int v = rand_number(1, 10);
        TEST_ASSERT_GREATER_OR_EQUAL_INT(1, v);
        TEST_ASSERT_LESS_OR_EQUAL_INT(10, v);
    }
}

void test_rand_number_same_low_high(void)
{
    int i;
    circle_srandom(1);
    for (i = 0; i < 50; i++)
        TEST_ASSERT_EQUAL_INT(7, rand_number(7, 7));
}

void test_rand_number_inverted_args(void)
{
    /* rand_number logs SYSERR and swaps; result must still be in [1,10] */
    int i;
    circle_srandom(1);
    for (i = 0; i < 50; i++) {
        int v = rand_number(10, 1);
        TEST_ASSERT_GREATER_OR_EQUAL_INT(1, v);
        TEST_ASSERT_LESS_OR_EQUAL_INT(10, v);
    }
}

/* =========================================================
 * dice — num dice each of size sides
 * ========================================================= */

void test_dice_zero_dice(void)
{
    circle_srandom(1);
    TEST_ASSERT_EQUAL_INT(0, dice(0, 6));
}

void test_dice_zero_sides(void)
{
    circle_srandom(1);
    TEST_ASSERT_EQUAL_INT(0, dice(3, 0));
}

void test_dice_result_in_range(void)
{
    int i;
    circle_srandom(99);
    for (i = 0; i < 200; i++) {
        int v = dice(2, 6);
        TEST_ASSERT_GREATER_OR_EQUAL_INT(2, v);
        TEST_ASSERT_LESS_OR_EQUAL_INT(12, v);
    }
}

void test_dice_one_die_one_side(void)
{
    circle_srandom(1);
    TEST_ASSERT_EQUAL_INT(1, dice(1, 1));
}

/* =========================================================
 * main
 * ========================================================= */

int main(void)
{
    UNITY_BEGIN();

    /* circle_srandom / circle_random */
    RUN_TEST(test_circle_random_deterministic_first);
    RUN_TEST(test_circle_random_deterministic_second);
    RUN_TEST(test_circle_random_deterministic_third);
    RUN_TEST(test_circle_random_same_seed_same_sequence);

    /* rand_number */
    RUN_TEST(test_rand_number_in_range);
    RUN_TEST(test_rand_number_same_low_high);
    RUN_TEST(test_rand_number_inverted_args);

    /* dice */
    RUN_TEST(test_dice_zero_dice);
    RUN_TEST(test_dice_zero_sides);
    RUN_TEST(test_dice_result_in_range);
    RUN_TEST(test_dice_one_die_one_side);

    return UNITY_END();
}
