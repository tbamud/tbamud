/**************************************************************************
*  File: spec_assign.c                                     Part of tbaMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"
#include "spec_procs.h"
#include "ban.h" /* for SPECIAL(gen_board) */
#include "boards.h"
#include "mail.h"

SPECIAL(questmaster); 
SPECIAL(shop_keeper);

/* local (file scope only) functions */
static void ASSIGNROOM(room_vnum room, SPECIAL(fname));
static void ASSIGNMOB(mob_vnum mob, SPECIAL(fname));
static void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname));

/* functions to perform assignments */
static void ASSIGNMOB(mob_vnum mob, SPECIAL(fname))
{
  mob_rnum rnum;

  if ((rnum = real_mobile(mob)) != NOBODY)
    mob_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}

static void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname))
{
  obj_rnum rnum;

  if ((rnum = real_object(obj)) != NOTHING)
    obj_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
}

static void ASSIGNROOM(room_vnum room, SPECIAL(fname))
{
  room_rnum rnum;

  if ((rnum = real_room(room)) != NOWHERE)
    world[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant room #%d", room);
}

/* Assignments */
/* assign special procedures to mobiles. Guildguards, snake, thief, magic user,
 * puff, fido, janitor, and cityguards are now implemented via triggers. */
void assign_mobiles(void)
{
  assign_kings_castle();

  ASSIGNMOB(3095, cryogenicist);

  ASSIGNMOB(120, guild);
  ASSIGNMOB(121, guild);
  ASSIGNMOB(122, guild);
  ASSIGNMOB(123, guild);
  ASSIGNMOB(2556, guild);
  ASSIGNMOB(2559, guild);
  ASSIGNMOB(2562, guild);
  ASSIGNMOB(2564, guild);
  ASSIGNMOB(2800, guild);
  ASSIGNMOB(3020, guild);
  ASSIGNMOB(3021, guild);
  ASSIGNMOB(3022, guild);
  ASSIGNMOB(3023, guild);
  ASSIGNMOB(5400, guild);
  ASSIGNMOB(5401, guild);
  ASSIGNMOB(5402, guild);
  ASSIGNMOB(5403, guild);
  ASSIGNMOB(11518, guild);
  ASSIGNMOB(25720, guild);
  ASSIGNMOB(25721, guild);
  ASSIGNMOB(25722, guild);
  ASSIGNMOB(25723, guild);
  ASSIGNMOB(25726, guild);
  ASSIGNMOB(25732, guild);
  ASSIGNMOB(27572, guild);
  ASSIGNMOB(27573, guild);
  ASSIGNMOB(27574, guild);
  ASSIGNMOB(27575, guild);
  ASSIGNMOB(27721, guild);
  ASSIGNMOB(29204, guild);
  ASSIGNMOB(29227, guild);
  ASSIGNMOB(31601, guild);
  ASSIGNMOB(31603, guild);
  ASSIGNMOB(31605, guild);
  ASSIGNMOB(31607, guild);
  ASSIGNMOB(31609, guild);
  ASSIGNMOB(31611, guild);
  ASSIGNMOB(31639, guild);
  ASSIGNMOB(31641, guild);

  ASSIGNMOB(3105, mayor);

  ASSIGNMOB(110, postmaster);
  ASSIGNMOB(1201, postmaster);
  ASSIGNMOB(3010, postmaster);
  ASSIGNMOB(10412, postmaster);
  ASSIGNMOB(10719, postmaster);
  ASSIGNMOB(25710, postmaster);
  ASSIGNMOB(27164, postmaster);
  ASSIGNMOB(30128, postmaster);
  ASSIGNMOB(31510, postmaster);

  ASSIGNMOB(1200, receptionist);
  ASSIGNMOB(3005, receptionist);
  ASSIGNMOB(5404, receptionist);
  ASSIGNMOB(27713, receptionist);
  ASSIGNMOB(27730, receptionist);
}

/* assign special procedures to objects */
void assign_objects(void)
{
  ASSIGNOBJ(1226, gen_board);   /* builder's board */
  ASSIGNOBJ(1227, gen_board);   /* staff board */
  ASSIGNOBJ(1228, gen_board);   /* advertising board */
  ASSIGNOBJ(3096, gen_board);	/* social board */
  ASSIGNOBJ(3097, gen_board);	/* freeze board */
  ASSIGNOBJ(3098, gen_board);	/* immortal board */
  ASSIGNOBJ(3099, gen_board);	/* mortal board */

  ASSIGNOBJ(115, bank);
  ASSIGNOBJ(334, bank);	        /* atm */
  ASSIGNOBJ(336, bank);	        /* cashcard */
  ASSIGNOBJ(3034, bank);        /* atm */
  ASSIGNOBJ(3036, bank);        /* cashcard */
  ASSIGNOBJ(3907, bank);
  ASSIGNOBJ(10640, bank);
  ASSIGNOBJ(10751, bank);
  ASSIGNOBJ(25758, bank);
}

/* assign special procedures to rooms */
void assign_rooms(void)
{
  room_rnum i;

  ASSIGNROOM(3031, pet_shops);
  ASSIGNROOM(10738, pet_shops);
  ASSIGNROOM(23281, pet_shops);
  ASSIGNROOM(25722, pet_shops);
  ASSIGNROOM(27155, pet_shops);
  ASSIGNROOM(27616, pet_shops);
  ASSIGNROOM(31523, pet_shops);

  if (CONFIG_DTS_ARE_DUMPS)
    for (i = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH))
	world[i].func = dump;
}

struct spec_func_data { 
   char *name; 
   SPECIAL(*func); 
}; 

static struct spec_func_data spec_func_list[] = { 
  {"Mayor",          mayor }, 
  {"Snake",          snake }, 
  {"Thief",          thief }, 
  {"Magic User",     magic_user }, 
  {"Puff",           puff }, 
  {"Fido",           fido }, 
  {"Janitor",        janitor }, 
  {"Cityguard",      cityguard }, 
  {"Postmaster",     postmaster }, 
  {"Receptionist",   receptionist }, 
  {"Cryogenicist",   cryogenicist}, 
  {"Bulletin Board", gen_board }, 
  {"Bank",           bank }, 
  {"Pet Shop",       pet_shops }, 
  {"Dump",           dump }, 
  {"Guildmaster",    guild }, 
  {"Guild Guard",    guild_guard }, 
  {"Questmaster",    questmaster }, 
  {"Shopkeeper",     shop_keeper }, 
  {"\n", NULL} 
}; 

const char *get_spec_func_name(SPECIAL(*func)) 
{ 
  int i; 
  for (i=0; *(spec_func_list[i].name) != '\n'; i++) { 
    if (func == spec_func_list[i].func) return (spec_func_list[i].name); 
  } 
  return NULL; 
} 

