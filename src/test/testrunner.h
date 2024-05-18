#ifndef TESTRUNNER_H
#define TESTRUNNER_H

#include "../conf.h"
#include "../sysdep.h"
#include "../structs.h"
#include "../utils.h"
#include "../comm.h"
#include "../db.h"
#include "../handler.h"
#include "../screen.h"
#include "../interpreter.h"
#include "../spells.h"
#include "../dg_scripts.h"
#include "../act.h"
#include "../class.h"
#include "../fight.h"
#include "../quest.h"
#include "../mud_event.h"
#include "../munit/munit.h"

/**
 * Utility macro for defining tests.
*/
#define UNIT_TEST(test_name) MunitResult (test_name)(const MunitParameter params[], void* data) 

/*
 * A "standard test" needs no setup or teardown and doesn't take any parameters. 
 * This is a utility macro for the test suite listing.
*/
#define STD_TEST(test_name, test_fun) { (char *)(test_name), (test_fun), NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }

#endif