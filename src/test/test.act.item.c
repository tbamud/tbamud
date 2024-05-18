#include "test.act.item.h"

UNIT_TEST(test_do_remove) {



    return MUNIT_OK;
}

MunitTest act_item_c_tests[] = {
    STD_TEST("/do_remove", test_do_remove),
  
    // end of array marker
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};