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


#define UNIT_TEST(test_name) MunitResult (test_name)(const MunitParameter params[], void* data) 


#endif