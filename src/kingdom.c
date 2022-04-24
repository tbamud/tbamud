/**************************************************************************
*  File: kingdom.c                                         Part of tbaMUD *
*  Usage: Source file for kingdom specific code.                          *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/* This file attempts to concentrate most of the code which must be changed
 * in order for new kingdoms to be added.  If you're adding a new kingdom, you
 * should go through this entire file from beginning to end and add the
 * appropriate new special cases for your new kingdom. */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "act.h"
#include "class.h"

/* Names first */
const char *kingdom_abbrevs[] = {
  "Re",
  "Bl",
  "\n"
};

const char *pc_kingdom_types[] = {
  "Red",
  "Blue",
  "\n"
};

/* The menu for choosing a kingdom in interpreter.c: */
const char *kingdom_menu =
"\r\n"
"Select a kingdom:\r\n"
"  [\t(R\t)]ed\r\n"
"  [\t(B\t)]lue\r\n";

/* The code to interpret a kingdom letter.  Used in interpreter.c when a new
 * character is selecting a kingdom and by 'set kingdom' in act.wizard.c. */
int parse_kingdom(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
  case 'r': return KINGDOM_RED;
  case 'b': return KINGDOM_BLUE;
  default:  return KINGDOM_UNDEFINED;
  }
}

/* bitvectors (i.e., powers of two) for each kingdom, mainly for use in do_who
 * and do_users.  Add new kingdoms at the end so that all kingdoms use
 * sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5,
 * etc.) up to the limit of your bitvector_t, typically 0-31. */
bitvector_t find_kingdom_bitvector(const char *arg)
{
  size_t rpos, ret = 0;

  for (rpos = 0; rpos < strlen(arg); rpos++)
    ret |= (1 << parse_kingdom(arg[rpos]));

  return (ret);
}

/* The appropriate rooms for each king/king's guard; controls which types
 * of people the various king's guards let through.  i.e., the first line
 * shows that from room 3017, only KINGDOM_RED are allowed to go south.  Don't
 * forget to visit spec_assign.c if you create any new mobiles that should be
 * a king or guard so they can act appropriately.  If you "recycle" the
 * existing mobs that are used in other kingdoms for your new kingdom, then
 * you don't have to change that file, only here.  King's guards are now
 * implemented via triggers.  This code remains as an example. */
struct kingdom_info_type kingdom_info[] = {

  /* */
  {KINGDOM_RED,  3017, SOUTH, {-1, -1}, {0, 0}, {0, 0}},
  {KINGDOM_BLUE, 3004, NORTH, {-1, -1}, {0, 0}, {0, 0}},

/* this must go last -- add new kings above! */
  { -1, NOWHERE, -1}
};
