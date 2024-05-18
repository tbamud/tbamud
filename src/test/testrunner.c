#include "testrunner.h"
#include "test.handler.h"
#include "test.act.item.h"

static MunitSuite suites[] = { 
  { "/handler.c", handler_c_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE }, 
  { "/act.item.c", act_item_c_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE }, 
  { NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE }
}; 

static const MunitSuite test_suite = {
  (char*) "",
  /* The first parameter is the array of test suites. */
  NULL,
  /* The second an array of suites to trigger from this one */
  suites,
  MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
  /* Finally, we'll actually run our test suite!  That second argument
   * is the user_data parameter which will be passed either to the
   * test or (if provided) the fixture setup function. */
  return munit_suite_main(&test_suite, (void*) "µnit", argc, argv);
}
