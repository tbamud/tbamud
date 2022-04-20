/**************************************************************************
*  File: race.c                                            Part of tbaMUD *
*  Usage: Source file for race specific code.                             *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/* This file attempts to concentrate most of the code which must be changed
 * in order for new races to be added.  If you're adding a new race, you
 * should go through this entire file from beginning to end and add the
 * appropriate new special cases for your new race. */

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
#include "race.h"

/* Names first */
const char *race_abbrevs[] = {
  "Hu",
  "Dw",
  "El",
  "Gi",
  "\n"
};

const char *pc_race_types[] = {
  "Human",
  "Dwarf",
  "Elf",
  "Giant",
  "\n"
};

/* The menu for choosing a race in interpreter.c: */
const char *race_menu =
"\r\n"
"Select a race:\r\n"
"  [\t(H\t)]uman\r\n"
"  [\t(D\t)]warf\r\n"
"  [\t(E\t)]lf\r\n"
"  [\t(G\t)]iant\r\n";

/* The code to interpret a race letter.  Used in interpreter.c when a new
 * character is selecting a race and by 'set race' in act.wizard.c. */
int parse_race(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
  case 'h': return RACE_HUMAN;
  case 'd': return RACE_DWARF;
  case 'e': return RACE_ELF;
  case 'g': return RACE_GIANT;
  default:  return RACE_UNDEFINED;
  }
}

/* bitvectors (i.e., powers of two) for each race, mainly for use in do_who
 * and do_users.  Add new races at the end so that all races use sequential
 * powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, etc.) up to
 * the limit of your bitvector_t, typically 0-31. */
bitvector_t find_race_bitvector(const char *arg)
{
  size_t rpos, ret = 0;

  for (rpos = 0; rpos < strlen(arg); rpos++)
    ret |= (1 << parse_race(arg[rpos]));

  return (ret);
}
