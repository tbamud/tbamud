/**
 * @file test_ibt.c
 * Unit tests for the IBT_DATA structure (src/ibt.h).
 */

#include "unity.h"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "ibt.h"

void setUp(void)    {}
void tearDown(void) {}

void test_ibt_dated_is_time_t(void)
{
  IBT_DATA ibt;
  TEST_ASSERT_EQUAL_size_t(sizeof(time_t), sizeof(ibt.dated));
  ibt.dated = (time_t)-1;
  TEST_ASSERT_TRUE(ibt.dated < 0 || sizeof(time_t) > 0);
  ibt.dated = time(NULL);
  TEST_ASSERT_TRUE(ibt.dated > 0);
  _Static_assert(__builtin_types_compatible_p(__typeof__(ibt.dated), time_t),
                 "IBT_DATA.dated must be time_t");
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_ibt_dated_is_time_t);
  return UNITY_END();
}
