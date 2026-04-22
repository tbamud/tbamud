/**
 * @file test_class.c
 * Unit tests for pure functions in src/class.c:
 *   parse_class, thaco, backstab_mult, level_exp
 */

#include "unity.h"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "class.h"

extern FILE *logfile;

void setUp(void)    { logfile = stderr; }
void tearDown(void) { logfile = NULL; }

/* =========================================================
 * parse_class
 * ========================================================= */

void test_parse_class_magic_user_lowercase(void)
{
    TEST_ASSERT_EQUAL_INT(CLASS_MAGIC_USER, parse_class('m'));
}

void test_parse_class_cleric_lowercase(void)
{
    TEST_ASSERT_EQUAL_INT(CLASS_CLERIC, parse_class('c'));
}

void test_parse_class_warrior_lowercase(void)
{
    TEST_ASSERT_EQUAL_INT(CLASS_WARRIOR, parse_class('w'));
}

void test_parse_class_thief_lowercase(void)
{
    TEST_ASSERT_EQUAL_INT(CLASS_THIEF, parse_class('t'));
}

void test_parse_class_uppercase(void)
{
    TEST_ASSERT_EQUAL_INT(CLASS_MAGIC_USER, parse_class('M'));
    TEST_ASSERT_EQUAL_INT(CLASS_CLERIC,     parse_class('C'));
    TEST_ASSERT_EQUAL_INT(CLASS_WARRIOR,    parse_class('W'));
    TEST_ASSERT_EQUAL_INT(CLASS_THIEF,      parse_class('T'));
}

void test_parse_class_invalid(void)
{
    TEST_ASSERT_EQUAL_INT(CLASS_UNDEFINED, parse_class('x'));
    TEST_ASSERT_EQUAL_INT(CLASS_UNDEFINED, parse_class('?'));
    TEST_ASSERT_EQUAL_INT(CLASS_UNDEFINED, parse_class(' '));
}

/* =========================================================
 * thaco
 * ========================================================= */

void test_thaco_magic_user_level_1(void)
{
    TEST_ASSERT_EQUAL_INT(20, thaco(CLASS_MAGIC_USER, 1));
}

void test_thaco_magic_user_level_10(void)
{
    TEST_ASSERT_EQUAL_INT(17, thaco(CLASS_MAGIC_USER, 10));
}

void test_thaco_cleric_level_1(void)
{
    TEST_ASSERT_EQUAL_INT(20, thaco(CLASS_CLERIC, 1));
}

void test_thaco_warrior_level_1(void)
{
    TEST_ASSERT_EQUAL_INT(20, thaco(CLASS_WARRIOR, 1));
}

void test_thaco_warrior_level_20(void)
{
    TEST_ASSERT_EQUAL_INT(2, thaco(CLASS_WARRIOR, 20));
}

void test_thaco_warrior_high_level_is_one(void)
{
    /* Warriors hit thac0=1 around level 21 and stay there */
    TEST_ASSERT_EQUAL_INT(1, thaco(CLASS_WARRIOR, 21));
    TEST_ASSERT_EQUAL_INT(1, thaco(CLASS_WARRIOR, LVL_IMPL));
}

void test_thaco_thief_level_1(void)
{
    TEST_ASSERT_EQUAL_INT(20, thaco(CLASS_THIEF, 1));
}

/* =========================================================
 * backstab_mult
 * ========================================================= */

void test_backstab_mult_level_1(void)
{
    TEST_ASSERT_EQUAL_INT(2, backstab_mult(1));
}

void test_backstab_mult_level_7(void)
{
    TEST_ASSERT_EQUAL_INT(2, backstab_mult(7));
}

void test_backstab_mult_level_8(void)
{
    TEST_ASSERT_EQUAL_INT(3, backstab_mult(8));
}

void test_backstab_mult_level_13(void)
{
    TEST_ASSERT_EQUAL_INT(3, backstab_mult(13));
}

void test_backstab_mult_level_14(void)
{
    TEST_ASSERT_EQUAL_INT(4, backstab_mult(14));
}

void test_backstab_mult_level_20(void)
{
    TEST_ASSERT_EQUAL_INT(4, backstab_mult(20));
}

void test_backstab_mult_level_21(void)
{
    TEST_ASSERT_EQUAL_INT(5, backstab_mult(21));
}

void test_backstab_mult_immortal(void)
{
    TEST_ASSERT_EQUAL_INT(20, backstab_mult(LVL_IMMORT));
}

/* =========================================================
 * level_exp
 * ========================================================= */

void test_level_exp_magic_user_level_0(void)
{
    TEST_ASSERT_EQUAL_INT(0, level_exp(CLASS_MAGIC_USER, 0));
}

void test_level_exp_magic_user_level_1(void)
{
    TEST_ASSERT_EQUAL_INT(1, level_exp(CLASS_MAGIC_USER, 1));
}

void test_level_exp_magic_user_level_2(void)
{
    TEST_ASSERT_EQUAL_INT(2500, level_exp(CLASS_MAGIC_USER, 2));
}

void test_level_exp_cleric_level_1(void)
{
    TEST_ASSERT_EQUAL_INT(1, level_exp(CLASS_CLERIC, 1));
}

void test_level_exp_thief_level_1(void)
{
    TEST_ASSERT_EQUAL_INT(1, level_exp(CLASS_THIEF, 1));
}

void test_level_exp_warrior_level_1(void)
{
    TEST_ASSERT_EQUAL_INT(1, level_exp(CLASS_WARRIOR, 1));
}

void test_level_exp_invalid_level_returns_zero(void)
{
    /* Level > LVL_IMPL or level < 0 → logs error and returns 0 */
    TEST_ASSERT_EQUAL_INT(0, level_exp(CLASS_MAGIC_USER, -1));
    TEST_ASSERT_EQUAL_INT(0, level_exp(CLASS_MAGIC_USER, LVL_IMPL + 1));
}

void test_level_exp_immortal_level(void)
{
    /* LVL_IMMORT for mage → 8000000 */
    TEST_ASSERT_EQUAL_INT(8000000, level_exp(CLASS_MAGIC_USER, LVL_IMMORT));
}

/* =========================================================
 * main
 * ========================================================= */

int main(void)
{
    UNITY_BEGIN();

    /* parse_class */
    RUN_TEST(test_parse_class_magic_user_lowercase);
    RUN_TEST(test_parse_class_cleric_lowercase);
    RUN_TEST(test_parse_class_warrior_lowercase);
    RUN_TEST(test_parse_class_thief_lowercase);
    RUN_TEST(test_parse_class_uppercase);
    RUN_TEST(test_parse_class_invalid);

    /* thaco */
    RUN_TEST(test_thaco_magic_user_level_1);
    RUN_TEST(test_thaco_magic_user_level_10);
    RUN_TEST(test_thaco_cleric_level_1);
    RUN_TEST(test_thaco_warrior_level_1);
    RUN_TEST(test_thaco_warrior_level_20);
    RUN_TEST(test_thaco_warrior_high_level_is_one);
    RUN_TEST(test_thaco_thief_level_1);

    /* backstab_mult */
    RUN_TEST(test_backstab_mult_level_1);
    RUN_TEST(test_backstab_mult_level_7);
    RUN_TEST(test_backstab_mult_level_8);
    RUN_TEST(test_backstab_mult_level_13);
    RUN_TEST(test_backstab_mult_level_14);
    RUN_TEST(test_backstab_mult_level_20);
    RUN_TEST(test_backstab_mult_level_21);
    RUN_TEST(test_backstab_mult_immortal);

    /* level_exp */
    RUN_TEST(test_level_exp_magic_user_level_0);
    RUN_TEST(test_level_exp_magic_user_level_1);
    RUN_TEST(test_level_exp_magic_user_level_2);
    RUN_TEST(test_level_exp_cleric_level_1);
    RUN_TEST(test_level_exp_thief_level_1);
    RUN_TEST(test_level_exp_warrior_level_1);
    RUN_TEST(test_level_exp_invalid_level_returns_zero);
    RUN_TEST(test_level_exp_immortal_level);

    return UNITY_END();
}
