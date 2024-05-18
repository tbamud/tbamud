#include "testrunner.h"
#include "test.handler.h"
#include "test.act.item.h"

void simple_world();

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
  return munit_suite_main(&test_suite, (void*) "µnit", argc, argv);
}


/*
 * test-fixtures common for many tests
*/

void simple_world()
{
  CREATE(world, struct room_data, 1);
  top_of_world = 1;
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