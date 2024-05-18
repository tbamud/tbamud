#include "test.act.item.h"

UNIT_TEST(test_do_remove) {

    
    char_data *ch = create_char();
    CREATE(ch->player_specials, struct player_special_data, 1);
    new_mobile_data(ch);
    ch->char_specials.position = POS_STANDING;
    CREATE(ch->desc, struct descriptor_data, 1);

    char_to_room(ch, 0);

    do_remove(ch, "2.ring", 0, 0);
    munit_assert_string_equal(get_last_messages(), "You don't seem to be using a ring.\r\n");

    obj_data *ring1 = create_obj();
    ring1->name = "ring";
    ring1->short_description = "ring1";
    
    obj_data *ring2 = create_obj();
    ring2->name = "ring";
    ring2->short_description = "ring2";

    equip_char(ch, ring1, WEAR_FINGER_R);
    equip_char(ch, ring2, WEAR_FINGER_L);

    do_remove(ch, "2.ring", 0, 0);
    munit_assert_ptr_equal(ch->carrying, ring2);

    return MUNIT_OK;
}

MunitTest act_item_c_tests[] = {
    STD_TEST("/do_remove", test_do_remove),
  
    // end of array marker
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

