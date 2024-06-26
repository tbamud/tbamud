#ifndef TESTRUNNER_H
#define TESTRUNNER_H

#include "test.fixtures.h"

/**
 * Utility macro for defining tests.
*/
#define UNIT_TEST(test_name) MunitResult (test_name)(const MunitParameter params[], void* data) 

/*
 * A "standard test" needs no setup or teardown and doesn't take any parameters. 
 * This is a utility macro for the test suite listing.
*/
#define STD_TEST(test_name, test_fun) { (char *)(test_name), (test_fun), NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }

/*
 * An "extended test" has setup or teardown but doesn't take any parameters.
 * This is a utility macro for the test suite listing.
*/
#define EXT_TEST(test_name, test_fun, setup_fun, teardown_fun) { (char *)(test_name), (test_fun), (setup_fun), (teardown_fun), MUNIT_TEST_OPTION_NONE, NULL }


/*
 * Returns the latest messages sent through send_to_char() or act()
 */
char *get_last_messages();

#endif