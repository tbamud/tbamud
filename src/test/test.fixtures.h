#ifndef TEST_FIXTURES_H
#define TEST_FIXTURES_H

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
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

/*
 * test fixtures
 */
char_data *get_test_char();

void simple_world();
void add_test_char(room_rnum target_room);

#endif //TEST_FIXTURES_H
