#include "test.handler.h"

static void run_single_get_number_test(const char* input_param, const char *name_result, int number_result);

UNIT_TEST(test_get_number)
{
  run_single_get_number_test("1.feather", "feather", 1);
  run_single_get_number_test("2.feather", "feather", 2);
  run_single_get_number_test("1.feat", "feat", 1);
  run_single_get_number_test("2.feat", "feat", 2);
  run_single_get_number_test("feather", "feather", 1);
  run_single_get_number_test("10.feather", "feather", 10);

  return MUNIT_OK;
}

static void run_single_get_number_test(const char* input_param, const char *name_result, int number_result)
{
  char *to_free;
  char *input = to_free = strdup(input_param);

  int number = get_number(&input);
  munit_assert_int32(number, ==, number_result);
  munit_assert_string_equal(input, name_result);
  
  free(to_free);
}

/* Creating a test suite is pretty simple.  First, you'll need an
 * array of tests: */
MunitTest handler_c_tests[] = {
    STD_TEST("/get_number", test_get_number),
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
