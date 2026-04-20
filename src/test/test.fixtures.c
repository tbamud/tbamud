#include "test.fixtures.h"


/*
 * test-fixtures common for many tests
*/

static char_data* test_char;

void simple_world()
{
    int i;
    CREATE(world, struct room_data, 1);
    top_of_world = 0;
    world[0].func = NULL;
    world[0].contents = NULL;
    world[0].people = NULL;
    world[0].light = 0;
    SCRIPT(&world[0]) = NULL;

    for (i = 0; i < NUM_OF_DIRS; i++)
        world[0].dir_option[i] = NULL;

    world[0].ex_description = NULL;
}


char_data *get_test_char() {
    return test_char;
}

void add_test_char(room_rnum target_room_rnum)
{
    if (top_of_world < 0) {
      fprintf(stderr, "World not created, nowhere to put character in add_test_char");
      exit(-1);
    }
    char_data *ch = create_char();
    CREATE(ch->player_specials, struct player_special_data, 1);
    ch->char_specials.position = POS_STANDING;
    CREATE(ch->desc, struct descriptor_data, 1);

    char_to_room(ch, target_room_rnum);
    test_char = ch;
}

