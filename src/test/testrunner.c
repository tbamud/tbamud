#include "testrunner.h"
#include "test.handler.h"
#include "test.act.item.h"

static void simple_world();
static void add_char();

static char_data* test_char;

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
  logfile = stderr;
  simple_world();
  add_char();
  return munit_suite_main(&test_suite, (void*) "µnit", argc, argv);
}


/*
 * test-fixtures common for many tests
*/

static void simple_world()
{
  CREATE(world, struct room_data, 1);
  top_of_world = 1;
}

char_data *get_test_char() {
  return test_char;
}

static void add_char()
{
    char_data *ch = create_char();
    CREATE(ch->player_specials, struct player_special_data, 1);
    new_mobile_data(ch);
    ch->char_specials.position = POS_STANDING;
    CREATE(ch->desc, struct descriptor_data, 1);

    char_to_room(ch, 0);
    test_char = ch;
}


static char testbuf[MAX_OUTPUT_BUFFER];
static int testbuf_size = 0;

size_t __wrap_send_to_char(struct char_data *ch, const char *messg, ...) 
{
  int size = testbuf_size;
  va_list args;

  va_start(args, messg);
  testbuf_size += vsnprintf(testbuf + size, MAX_OUTPUT_BUFFER - size, messg, args);
  va_end(args);
  
  return testbuf_size;
}

size_t __wrap_vwrite_to_output(struct descriptor_data *t, const char *format, va_list args)
{
  int size = testbuf_size;
  testbuf_size += vsnprintf(testbuf + size, MAX_OUTPUT_BUFFER - size, format, args);
  return testbuf_size;
}

char *get_last_messages()
{
  char *stored_response = strdup(testbuf);
  testbuf_size = 0;
  *testbuf = '\0';
  return stored_response;
}