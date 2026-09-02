/**
 * @file test_handler.c
 * Unit tests for innate-affect handling in src/handler.c.
 *
 * A mob's own AFF flags are a source of those flags, like a spell or a worn
 * item, and unlike those two they do not go away while the mob lives.  The
 * bookkeeping in affect_modify_ar() has to leave them alone when some other
 * source is withdrawn, without ever handing back a flag the game took off on
 * purpose.  These tests pin both halves of that.
 */

#include "unity.h"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "handler.h"
#include "spells.h"

#include <string.h>

extern FILE *logfile;

static struct char_data ch;

void setUp(void)
{
    logfile = stderr;

    memset(&ch, 0, sizeof(ch));
    IN_ROOM(&ch) = NOWHERE;
    GET_POS(&ch) = POS_STANDING;
    GET_MOB_RNUM(&ch) = NOBODY;
}

void tearDown(void)
{
    while (ch.affected)
        affect_remove(&ch, ch.affected);

    logfile = NULL;
}

/* Make ch a mob carrying `flag` both in its live flags and in the set it was
 * created with -- i.e. a mob whose mob file sets that flag. */
static void make_innate_mob(int flag)
{
    SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
    SET_BIT_AR(AFF_FLAGS(&ch), flag);
    SET_BIT_AR(GET_INNATE_AFF(&ch), flag);
}

/* Land a spell on ch whose bitvector carries `flag`, then let it expire. */
static void apply_and_expire_spell(int flag)
{
    struct affected_type af;

    new_affect(&af);
    af.spell = SPELL_SANCTUARY;
    af.duration = 4;
    SET_BIT_AR(af.bitvector, flag);

    affect_to_char(&ch, &af);
    TEST_ASSERT_TRUE(AFF_FLAGGED(&ch, flag));

    affect_from_char(&ch, SPELL_SANCTUARY);
}

/* An expiring spell must not take an innate flag with it.  This is the
 * reported bug: sanctuary a mob that already has sanctuary from its mob file,
 * wait for the spell to fade, and the mob used to be left without it. */
static void test_innate_flag_survives_expiring_spell(void)
{
    make_innate_mob(AFF_SANCTUARY);
    apply_and_expire_spell(AFF_SANCTUARY);

    TEST_ASSERT_TRUE(AFF_FLAGGED(&ch, AFF_SANCTUARY));
}

/* Same shape, via equipment rather than a spell. */
static void test_innate_flag_survives_removed_equipment(void)
{
    struct obj_data obj;
    int j;

    make_innate_mob(AFF_SANCTUARY);

    memset(&obj, 0, sizeof(obj));
    IN_ROOM(&obj) = NOWHERE;
    obj.worn_on = NOWHERE;
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
        obj.affected[j].location = APPLY_NONE;
    SET_BIT_AR(GET_OBJ_AFFECT(&obj), AFF_SANCTUARY);

    /* equip_char() bails out quietly on a badly built object, which would
     * leave the rest of this test asserting nothing. */
    equip_char(&ch, &obj, WEAR_BODY);
    TEST_ASSERT_EQUAL_PTR(&obj, GET_EQ(&ch, WEAR_BODY));
    TEST_ASSERT_TRUE(AFF_FLAGGED(&ch, AFF_SANCTUARY));

    TEST_ASSERT_EQUAL_PTR(&obj, unequip_char(&ch, WEAR_BODY));

    TEST_ASSERT_TRUE(AFF_FLAGGED(&ch, AFF_SANCTUARY));
}

/* A flag the mob was not created with is ordinary bookkeeping and still goes
 * away when its source does. */
static void test_non_innate_flag_is_removed(void)
{
    SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);

    apply_and_expire_spell(AFF_SANCTUARY);

    TEST_ASSERT_FALSE(AFF_FLAGGED(&ch, AFF_SANCTUARY));
}

/* The regression the snapshot exists to prevent.  %transform% leaves a mob
 * wearing another prototype's flags while GET_MOB_RNUM() still names the one
 * it was loaded from; reading innateness back out of the prototype would call
 * a flag innate that this mob never had, and an expiring spell would then
 * leave it set for good.  The set the mob was created with is what counts. */
static void test_flag_absent_from_creation_set_is_removed(void)
{
    SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
    GET_MOB_RNUM(&ch) = 0;              /* a real rnum, wrong prototype */

    apply_and_expire_spell(AFF_SANCTUARY);

    TEST_ASSERT_FALSE(AFF_FLAGGED(&ch, AFF_SANCTUARY));
}

/* AFF_INVISIBLE and AFF_HIDE are the exception: the game reveals a mob by
 * taking them off, so a revealed mob has to stay revealed even though its mob
 * file sets the bit. */
static void test_innate_invisibility_is_still_removed(void)
{
    make_innate_mob(AFF_INVISIBLE);
    apply_and_expire_spell(AFF_INVISIBLE);

    TEST_ASSERT_FALSE(AFF_FLAGGED(&ch, AFF_INVISIBLE));
}

static void test_innate_hide_is_still_removed(void)
{
    make_innate_mob(AFF_HIDE);
    apply_and_expire_spell(AFF_HIDE);

    TEST_ASSERT_FALSE(AFF_FLAGGED(&ch, AFF_HIDE));
}

/* Players have no creation set to be innate from. */
static void test_player_flags_are_never_innate(void)
{
    SET_BIT_AR(AFF_FLAGS(&ch), AFF_SANCTUARY);
    SET_BIT_AR(GET_INNATE_AFF(&ch), AFF_SANCTUARY);   /* must be ignored */

    apply_and_expire_spell(AFF_SANCTUARY);

    TEST_ASSERT_FALSE(AFF_FLAGGED(&ch, AFF_SANCTUARY));
}

/* Declining to drop a flag must not disturb the rest of the affect, and an
 * unrelated flag riding the same affect still goes away. */
static void test_other_flags_on_the_same_affect_still_drop(void)
{
    struct affected_type af;

    make_innate_mob(AFF_SANCTUARY);

    new_affect(&af);
    af.spell = SPELL_SANCTUARY;
    af.duration = 4;
    SET_BIT_AR(af.bitvector, AFF_SANCTUARY);
    SET_BIT_AR(af.bitvector, AFF_DETECT_INVIS);

    affect_to_char(&ch, &af);
    TEST_ASSERT_TRUE(AFF_FLAGGED(&ch, AFF_DETECT_INVIS));

    affect_from_char(&ch, SPELL_SANCTUARY);

    TEST_ASSERT_TRUE(AFF_FLAGGED(&ch, AFF_SANCTUARY));
    TEST_ASSERT_FALSE(AFF_FLAGGED(&ch, AFF_DETECT_INVIS));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_innate_flag_survives_expiring_spell);
    RUN_TEST(test_innate_flag_survives_removed_equipment);
    RUN_TEST(test_non_innate_flag_is_removed);
    RUN_TEST(test_flag_absent_from_creation_set_is_removed);
    RUN_TEST(test_innate_invisibility_is_still_removed);
    RUN_TEST(test_innate_hide_is_still_removed);
    RUN_TEST(test_player_flags_are_never_innate);
    RUN_TEST(test_other_flags_on_the_same_affect_still_drop);

    return UNITY_END();
}
