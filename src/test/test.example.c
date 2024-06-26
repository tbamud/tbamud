#include "test.example.h"

UNIT_TEST(example_test)
{
  return MUNIT_OK;
}


MunitTest test_example_c_tests[] = {
    STD_TEST("/example/example_test", example_test),

    // end of array marker
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
