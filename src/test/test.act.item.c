#include "test.act.item.h"


UNIT_TEST(test_do_remove_should_give_message_on_removing_of_unknown_item) {
    char_data *ch = get_test_char();

    do_remove(ch, "2.ring", 0, 0);
    munit_assert_string_equal(get_last_messages(), "You don't seem to be using a ring.\r\n");
    
    return MUNIT_OK;
}


UNIT_TEST(test_do_remove_should_remove_second_item_by_number) {
    char_data *ch = get_test_char();

    obj_data *ring1 = create_obj();
    ring1->name = strdup("ring");
    ring1->short_description = strdup("ring1");
    
    obj_data *ring2 = create_obj();
    ring2->name = strdup("ring");
    ring2->short_description = strdup("ring2");

    equip_char(ch, ring1, WEAR_FINGER_R);
    equip_char(ch, ring2, WEAR_FINGER_L);

    do_remove(ch, "2.ring", 0, 0);

    munit_assert_ptr_equal(ch->carrying, ring2);
    munit_assert_ptr_equal(ch->carrying->next, ring1);

    return MUNIT_OK;
}

static void* before_each(const MunitParameter params[], void* user_data) {
    simple_world();
    add_test_char(0);
    return NULL;
}

static void after_each(void* fixture) {
    destroy_db();
}

MunitTest act_item_c_tests[] = {
    EXT_TEST("/do_remove/item_not_found", test_do_remove_should_give_message_on_removing_of_unknown_item, before_each, after_each),
    EXT_TEST("/do_remove/remove_second_item", test_do_remove_should_remove_second_item_by_number, before_each, after_each),
  
    // end of array marker
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

