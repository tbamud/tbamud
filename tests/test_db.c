/* Flag fields and the unknown-flag check in src/db.c.
 *
 * db.c is included rather than linked so its static helpers can be called
 * directly.  It brings its own definitions of the world globals, so this
 * suite does not link test_stubs.c; section GC drops the rest of db.c, which
 * needs the running game. */
#include <stdarg.h>
#include <string.h>

#include "unity.h"
#include "../src/db.c"

#ifdef UNITY_SUPPORT_64
#define TEST_ASSERT_FLAGS(expected, actual) TEST_ASSERT_EQUAL_HEX64((expected), (actual))
#else
#define TEST_ASSERT_FLAGS(expected, actual) TEST_ASSERT_EQUAL_HEX32((expected), (actual))
#endif

#define LOG_LINES 64

static char logged[LOG_LINES][256];
static int log_count;

void basic_mud_log(const char *format, ...)
{
  va_list args;

  if (log_count < LOG_LINES) {
    va_start(args, format);
    vsnprintf(logged[log_count], sizeof(logged[0]), format, args);
    va_end(args);
  }
  log_count++;
}

/* How many logged lines contain text. */
static int logged_with(const char *text)
{
  int i, found = 0;

  for (i = 0; i < log_count && i < LOG_LINES; i++)
    if (strstr(logged[i], text))
      found++;
  return found;
}

void setUp(void)
{
  log_count = 0;
}

void tearDown(void)
{
}

/* sprintascii() in genolc.c writes bit n of a flag field as the nth of these. */
static const char flag_letters[] = "abcdefghijklmnopqrstuvwxyzABCDEF";

void test_asciiflag_conv_reads_each_letter_sprintascii_writes_as_its_bit(void)
{
  char field[2] = "";
  int i;

  for (i = 0; flag_letters[i]; i++) {
    field[0] = flag_letters[i];
    TEST_ASSERT_FLAGS((bitvector_t)1 << i, asciiflag_conv(field));
  }
  TEST_ASSERT_EQUAL_INT(0, log_count);
}

/* 'F' is bit 31.  As `1 << 31` on an int it came back negative, and on a
 * 64-bit bitvector_t that set bits 32-63 as well. */
void test_asciiflag_conv_reads_F_as_bit_31_alone(void)
{
  char all[sizeof(flag_letters)];

  TEST_ASSERT_FLAGS((bitvector_t)1 << 31, asciiflag_conv("F"));

  strcpy(all, flag_letters);
  TEST_ASSERT_FLAGS((bitvector_t)0xFFFFFFFFUL, asciiflag_conv(all));
  TEST_ASSERT_EQUAL_INT(0, log_count);
}

void test_asciiflag_conv_reads_numeric_fields_as_numbers(void)
{
  TEST_ASSERT_FLAGS(0, asciiflag_conv("0"));
  TEST_ASSERT_FLAGS(1024, asciiflag_conv("1024"));
  TEST_ASSERT_EQUAL_INT(0, log_count);
}

/* 'G' would be bit 32, which no flag field holds.  The shift count wrapped
 * on x86, so it used to set bit 0, the 'a' flag. */
void test_asciiflag_conv_reports_and_ignores_letters_past_F(void)
{
  char field[8];

  strcpy(field, "G");
  TEST_ASSERT_FLAGS(0, asciiflag_conv(field));
  TEST_ASSERT_EQUAL_INT(1, log_count);
  TEST_ASSERT_EQUAL_INT(1, logged_with("SYSERR"));
  TEST_ASSERT_EQUAL_INT(1, logged_with("'G'"));

  log_count = 0;
  strcpy(field, "aZc");
  TEST_ASSERT_FLAGS(((bitvector_t)1 << 0) | ((bitvector_t)1 << 2), asciiflag_conv(field));
  TEST_ASSERT_EQUAL_INT(1, log_count);
  TEST_ASSERT_EQUAL_INT(1, logged_with("'Z'"));
}

/* Old affect fields name each bit one lower than tbaMUD does, because
 * AFF_DONTUSE took bit 0. */
void test_asciiflag_conv_aff_reads_each_letter_one_bit_up(void)
{
  char field[2] = "";
  int i;

  for (i = 0; flag_letters[i] != 'F'; i++) {
    field[0] = flag_letters[i];
    TEST_ASSERT_FLAGS((bitvector_t)1 << (i + 1), asciiflag_conv_aff(field));
  }
  TEST_ASSERT_EQUAL_INT(0, log_count);
}

/* 'E' is bit 31 here, and 'F' would be bit 32: that one wrapped to bit 0,
 * AFF_DONTUSE. */
void test_asciiflag_conv_aff_reads_E_as_bit_31_and_ignores_F(void)
{
  char field[8];

  strcpy(field, "E");
  TEST_ASSERT_FLAGS((bitvector_t)1 << 31, asciiflag_conv_aff(field));
  TEST_ASSERT_EQUAL_INT(0, log_count);

  strcpy(field, "aF");
  TEST_ASSERT_FLAGS((bitvector_t)1 << 1, asciiflag_conv_aff(field));
  TEST_ASSERT_EQUAL_INT(1, log_count);
  TEST_ASSERT_EQUAL_INT(1, logged_with("'F'"));
}

void test_check_bitvector_names_passes_known_bits(void)
{
  int flags[RF_ARRAY_MAX] = { 0 };

  flags[0] = asciiflag_conv("abcd");
  TEST_ASSERT_EQUAL_INT(FALSE, check_bitvector_names(flags[0], 4, "room #1", "room"));
  TEST_ASSERT_EQUAL_INT(0, log_count);
}

/* Bits 32 and up of the old loop were `1 << flagnum` wrapping back to
 * bits 0-31, so every set bit was also reported 32 higher. */
void test_check_bitvector_names_reports_only_the_unknown_bit(void)
{
  int flags[RF_ARRAY_MAX] = { 0 };

  flags[0] = asciiflag_conv("af");
  TEST_ASSERT_EQUAL_INT(TRUE, check_bitvector_names(flags[0], 4, "room #1", "room"));
  TEST_ASSERT_EQUAL_INT(1, log_count);
  TEST_ASSERT_EQUAL_INT(1, logged_with("bit 5 ("));
}

/* A flag word holding 'F' is negative as the int the world arrays keep.
 * Widened to a 64-bit bitvector_t it had bits 31-63 set, and the old loop
 * reported bit 63 (and bit 32, for the 'a') along with bit 31. */
void test_check_bitvector_names_reports_bit_31_once(void)
{
  int flags[RF_ARRAY_MAX] = { 0 };

  flags[0] = asciiflag_conv("aF");
  TEST_ASSERT_TRUE(flags[0] < 0);
  TEST_ASSERT_EQUAL_INT(TRUE, check_bitvector_names(flags[0], 4, "room #1", "room"));
  TEST_ASSERT_EQUAL_INT(1, log_count);
  TEST_ASSERT_EQUAL_INT(1, logged_with("bit 31 ("));
  TEST_ASSERT_EQUAL_INT(0, logged_with("bit 32 ("));
  TEST_ASSERT_EQUAL_INT(0, logged_with("bit 63 ("));
}

/* With a name for every bit of the word, nothing in it is unknown. */
void test_check_bitvector_names_passes_a_fully_named_word(void)
{
  int flags[RF_ARRAY_MAX] = { 0 };
  char all[sizeof(flag_letters)];

  strcpy(all, flag_letters);
  flags[0] = asciiflag_conv(all);
  TEST_ASSERT_EQUAL_INT(FALSE, check_bitvector_names(flags[0], 32, "room #1", "room"));
  TEST_ASSERT_EQUAL_INT(FALSE, check_bitvector_names(flags[0], 40, "room #1", "room"));
  TEST_ASSERT_EQUAL_INT(0, log_count);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_asciiflag_conv_reads_each_letter_sprintascii_writes_as_its_bit);
  RUN_TEST(test_asciiflag_conv_reads_F_as_bit_31_alone);
  RUN_TEST(test_asciiflag_conv_reads_numeric_fields_as_numbers);
  RUN_TEST(test_asciiflag_conv_reports_and_ignores_letters_past_F);
  RUN_TEST(test_asciiflag_conv_aff_reads_each_letter_one_bit_up);
  RUN_TEST(test_asciiflag_conv_aff_reads_E_as_bit_31_and_ignores_F);
  RUN_TEST(test_check_bitvector_names_passes_known_bits);
  RUN_TEST(test_check_bitvector_names_reports_only_the_unknown_bit);
  RUN_TEST(test_check_bitvector_names_reports_bit_31_once);
  RUN_TEST(test_check_bitvector_names_passes_a_fully_named_word);
  return UNITY_END();
}
