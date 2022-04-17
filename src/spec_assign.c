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
  /*assign_kings_castle();*/

  /*ASSIGNMOB(3095, cryogenicist);*/

  /*ASSIGNMOB(120, guild);*/

  /*ASSIGNMOB(3105, mayor);*/

  /*ASSIGNMOB(110, postmaster);*/

  /*ASSIGNMOB(1200, receptionist);*/
}

/* assign special procedures to objects */
void assign_objects(void)
{
  ASSIGNOBJ(99, gen_board); /* immortal board */

  /*ASSIGNOBJ(115, bank);*/
}

/* assign special procedures to rooms */
void assign_rooms(void)
{
  room_rnum i;

  /*ASSIGNROOM(3031, pet_shops);*/

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

