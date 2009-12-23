/**************************************************************************
*  File: oasis_list.c                                      Part of tbaMUD *
*  Usage: Oasis OLC listings.                                             *
*                                                                         *
* By Levork. Copyright 1996 Harvey Gilpin. 1997-2001 George Greer.        *
* 2002 Kip Potter [Mythran].                                              *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "shop.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "quest.h"

/* local functions */
static void list_triggers(struct char_data *ch, zone_rnum rnum, trig_vnum vmin, trig_vnum vmax);
static void list_rooms(struct char_data *ch  , zone_rnum rnum, room_vnum vmin, room_vnum vmax);
static void list_mobiles(struct char_data *ch, zone_rnum rnum, mob_vnum vmin , mob_vnum vmax );
static void list_objects(struct char_data *ch, zone_rnum rnum, obj_vnum vmin , obj_vnum vmax );
static void list_shops(struct char_data *ch  , zone_rnum rnum, shop_vnum vmin, shop_vnum vmax);
static void list_zones(struct char_data *ch, zone_rnum rnum, zone_vnum vmin, zone_vnum vmax);

void perform_mob_flag_list(struct char_data * ch, char *arg)
{
  int num, mob_flag, found = 0;
  struct char_data *mob;

  mob_flag = atoi(arg);

  if (mob_flag < 0 || mob_flag > NUM_MOB_FLAGS) {
    send_to_char(ch, "Invalid flag number!\r\n");
    return;
  }

  send_to_char(ch, "Listing mobiles with %s%s%s flag set.\r\n", QYEL, action_bits[mob_flag], QNRM);

  for(num=0;num<=top_of_mobt;num++) {
    if(IS_SET_AR((mob_proto[num].char_specials.saved.act), mob_flag)) {

      if ((mob = read_mobile(num, REAL)) != NULL) {
        char_to_room(mob, 0);
        send_to_char(ch,"%s%3d. %s[%s%5d%s]%s Level %s%-3d%s %s%s\r\n", CCNRM(ch, C_NRM),++found,
                      CCCYN(ch, C_NRM), CCYEL(ch, C_NRM), GET_MOB_VNUM(mob), CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                      CCYEL(ch, C_NRM), GET_LEVEL(mob), CCNRM(ch, C_NRM), GET_NAME(mob), CCNRM(ch, C_NRM));
        extract_char(mob); /* Finished with the mob - remove it from the MUD */
      }
    }
  }
  if (!found)
    send_to_char(ch,"None Found!\r\n");
  return;
}

void perform_mob_level_list(struct char_data * ch, char *arg)
{
  int num, mob_level, found = 0;
  struct char_data *mob;

  mob_level = atoi(arg);

  if (mob_level < 0 || mob_level > 99) {
    send_to_char(ch, "Invalid mob level!\r\n");
    return;
  }

  send_to_char(ch, "Listing mobiles of level %s%d%s\r\n", QYEL, mob_level, QNRM);
  for(num=0;num<=top_of_mobt;num++) {
    if((mob_proto[num].player.level) == mob_level) {

      if ((mob = read_mobile(num, REAL)) != NULL) {
        char_to_room(mob, 0);
        send_to_char(ch,"%s%3d. %s[%s%5d%s]%s %s%s\r\n", CCNRM(ch, C_NRM),++found,
                      CCCYN(ch, C_NRM), CCYEL(ch, C_NRM), GET_MOB_VNUM(mob), CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                      GET_NAME(mob), CCNRM(ch, C_NRM));
        extract_char(mob); /* Finished with the mob - remove it from the MUD */
      }
    }
  }
  if (!found)
    send_to_char(ch,"None Found!\r\n");

  return;
}

/* Ingame Commands */
ACMD(do_oasis_list)
{
  zone_rnum rzone = NOWHERE;
  room_rnum vmin = NOWHERE;
  room_rnum vmax = NOWHERE;
  char smin[MAX_INPUT_LENGTH];
  char smax[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, smin, smax);

  if (!*smin || *smin == '.') {
    rzone = world[IN_ROOM(ch)].zone;
  } else if (!*smax) {
    rzone = real_zone(atoi(smin));

    if (rzone == NOWHERE) {
      send_to_char(ch, "Sorry, there's no zone with that number\r\n");
      return;
    }
  } else {
    /* Listing by min vnum / max vnum.  Retrieve the numeric values. */
    vmin = atoi(smin);
    vmax = atoi(smax);

    if (vmin > vmax) {
      send_to_char(ch, "List from %d to %d - Aren't we funny today!\r\n", vmin, vmax);
      return;
    }
  }

  switch (subcmd) {
    case SCMD_OASIS_MLIST:

      two_arguments(argument, arg, arg2);

   if (is_abbrev(arg, "level") || is_abbrev(arg, "flags")) {

    int  i;

    if (!*arg2) {
    send_to_char(ch, "Which mobile flag or level do you want to list?\r\n");
    for (i=0; i<NUM_MOB_FLAGS; i++)
    {
      send_to_char(ch, "%s%2d%s-%s%-14s%s", CCNRM(ch, C_NRM), i, CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), action_bits[i], CCNRM(ch, C_NRM));
      if (!((i+1)%4))  send_to_char(ch, "\r\n");
    }
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Usage: %smlist flags <num>%s\r\n", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
    send_to_char(ch, "       %smlist level <num>%s\r\n", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
    send_to_char(ch, "Displays mobs with the selected flag, or at the selected level\r\n\r\n");

    return;
    }
    if (is_abbrev(arg, "level"))
      perform_mob_level_list(ch, arg2);
    else
      perform_mob_flag_list(ch, arg2);
  } else
    list_mobiles(ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_OLIST: list_objects(ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_RLIST: list_rooms(ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_TLIST: list_triggers(ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_SLIST: list_shops(ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_QLIST: list_quests(ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_ZLIST:
      if (!*smin)
        list_zones(ch, NOWHERE, 0, zone_table[top_of_zone_table].number);
      else
        list_zones(ch, rzone, vmin, vmax);
      break;
    default:
      send_to_char(ch, "You can't list that!\r\n");
      mudlog(BRF, LVL_IMMORT, TRUE,
        "SYSERR: do_oasis_list: Unknown list option: %d", subcmd);
  }
}

ACMD(do_oasis_links)
{
  zone_rnum zrnum;
  zone_vnum zvnum;
  room_rnum nr, to_room;
  room_vnum first, last;
  int j;
  char arg[MAX_INPUT_LENGTH];

  skip_spaces(&argument);
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch,
      "Syntax: links <zone_vnum> ('.' for zone you are standing in)\r\n");
      return;
    }

  if (!strcmp(arg, ".")) {
    zrnum = world[IN_ROOM(ch)].zone;
    zvnum = zone_table[zrnum].number;
  } else {
    zvnum = atoi(arg);
    zrnum = real_zone(zvnum);
  }

  if (zrnum == NOWHERE || zvnum == NOWHERE) {
    send_to_char(ch, "No zone was found with that number.\n\r");
    return;
  }

  last  = zone_table[zrnum].top;
  first = zone_table[zrnum].bot;

  send_to_char(ch, "Zone %d is linked to the following zones:\r\n", zvnum);
  for (nr = 0; nr <= top_of_world && (GET_ROOM_VNUM(nr) <= last); nr++) {
    if (GET_ROOM_VNUM(nr) >= first) {
      for (j = 0; j < NUM_OF_DIRS; j++) {
	if (world[nr].dir_option[j]) {
	  to_room = world[nr].dir_option[j]->to_room;
	  if (to_room != NOWHERE && (zrnum != world[to_room].zone))
	    send_to_char(ch, "%3d %-30s%s at %5d (%-5s) ---> %5d\r\n",
	      zone_table[world[to_room].zone].number,
	      zone_table[world[to_room].zone].name, QNRM,
	      GET_ROOM_VNUM(nr), dirs[j], world[to_room].number);
	}
      }
    }
  }
}

/* Helper Functions */
/* List all rooms in a zone. */
static void list_rooms(struct char_data *ch, zone_rnum rnum, room_vnum vmin, room_vnum vmax)
{
  room_rnum i;
  room_vnum bottom, top;
  int j, counter = 0;

  /* Expect a minimum / maximum number if the rnum for the zone is NOWHERE. */
  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  send_to_char (ch,
  "Index VNum    Room Name                                    Exits\r\n"
  "----- ------- -------------------------------------------- -----\r\n");

  if (!top_of_world)
    return;

  for (i = 0; i <= top_of_world; i++) {

    /** Check to see if this room is one of the ones needed to be listed.    **/
    if ((world[i].number >= bottom) && (world[i].number <= top)) {
      counter++;

        send_to_char(ch, "%4d) [%s%-5d%s] %s%-*s%s %s",
                          counter, QGRN, world[i].number, QNRM,
                          QCYN, count_color_chars(world[i].name)+44, world[i].name, QNRM,
                          world[i].proto_script ? "[TRIG] " : ""
                          );

      for (j = 0; j < NUM_OF_DIRS; j++) {
        if (W_EXIT(i, j) == NULL)
          continue;
        if (W_EXIT(i, j)->to_room == NOWHERE)
          continue;

        if (world[W_EXIT(i, j)->to_room].zone != world[i].zone)
          send_to_char(ch, "(%s%d%s)", QYEL, world[W_EXIT(i, j)->to_room].number, QNRM);

      }

      send_to_char(ch, "\r\n");
    }
  }

  if (counter == 0)
    send_to_char(ch, "No rooms found for zone/range specified.\r\n");
}

/* List all mobiles in a zone. */
static void list_mobiles(struct char_data *ch, zone_rnum rnum, mob_vnum vmin, mob_vnum vmax)
{
  mob_rnum i;
  mob_vnum bottom, top;
  int counter = 0;

  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  send_to_char(ch,
  "Index VNum    Mobile Name                                  Level\r\n"
  "----- ------- -------------------------------------------- -----\r\n");

  if (!top_of_mobt)
    return;

  for (i = 0; i <= top_of_mobt; i++) {
    if (mob_index[i].vnum >= bottom && mob_index[i].vnum <= top) {
      counter++;

      send_to_char(ch, "%s%4d%s) [%s%-5d%s] %s%-*s %s[%4d]%s%s\r\n",
                   QGRN, counter, QNRM, QGRN, mob_index[i].vnum, QNRM,
                   QCYN, count_color_chars(mob_proto[i].player.short_descr)+44, mob_proto[i].player.short_descr,
                   QYEL, mob_proto[i].player.level, QNRM,
                   mob_proto[i].proto_script ? " [TRIG]" : ""
                   );
    }
  }

  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
}

/* List all objects in a zone. */
static void list_objects(struct char_data *ch, zone_rnum rnum, obj_vnum vmin, obj_vnum vmax)
{
  obj_rnum i;
  obj_vnum bottom, top;
  int counter = 0;

  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  send_to_char(ch,
  "Index VNum    Object Name                                  Object Type\r\n"
  "----- ------- -------------------------------------------- ----------------\r\n");

  if (!top_of_objt)
    return;

  for (i = 0; i <= top_of_objt; i++) {
    if (obj_index[i].vnum >= bottom && obj_index[i].vnum <= top) {
      counter++;

      send_to_char(ch, "%s%4d%s) [%s%-5d%s] %s%-*s %s[%s]%s%s\r\n",
                   QGRN, counter, QNRM, QGRN, obj_index[i].vnum, QNRM,
                   QCYN, count_color_chars(obj_proto[i].short_description)+44, obj_proto[i].short_description, QYEL,
                   item_types[obj_proto[i].obj_flags.type_flag], QNRM,
                   obj_proto[i].proto_script ? " [TRIG]" : ""
                   );

    }
  }

  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
}

/* List all shops in a zone. */
static void list_shops(struct char_data *ch, zone_rnum rnum, shop_vnum vmin, shop_vnum vmax)
{
  shop_rnum i;
  shop_vnum bottom, top;
  int j, counter = 0;

  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  send_to_char (ch,
  "Index VNum    RNum    Shop Room(s)\r\n"
  "----- ------- ------- -----------------------------------------\r\n");

  for (i = 0; i <= top_shop; i++) {
    if (SHOP_NUM(i) >= bottom && SHOP_NUM(i) <= top) {
      counter++;

      /* the +1 is strange but fits the rest of the shop code */
      send_to_char(ch, "%s%4d%s) [%s%-5d%s] [%s%-5d%s]",
        QGRN, counter, QNRM, QGRN, SHOP_NUM(i), QNRM, QGRN, i + 1, QNRM);

      /* Thanks to Ken Ray for this display fix. -Welcor */
      for (j = 0; SHOP_ROOM(i, j) != NOWHERE; j++)
        send_to_char(ch, "%s%s[%s%-5d%s]%s",
                      ((j > 0) && (j % 6 == 0)) ? "\r\n                      " : " ",
                      QCYN, QYEL, SHOP_ROOM(i, j), QCYN, QNRM);

      if (j == 0)
        send_to_char(ch, "%sNone.%s", QCYN, QNRM);

      send_to_char(ch, "\r\n");
    }
  }

  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
}

/* List all zones in the world (sort of like 'show zones'). */
static void list_zones(struct char_data *ch, zone_rnum rnum, zone_vnum vmin, zone_vnum vmax)
{
  int counter = 0;
  zone_rnum i;
  zone_vnum bottom, top;

  if (rnum != NOWHERE) {
    /* Only one parameter was supplied - just list that zone */
     print_zone(ch, zone_table[rnum].number);
    return;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  send_to_char(ch,
  "VNum  Zone Name                      Builder(s)\r\n"
  "----- ------------------------------ --------------------------------------\r\n");

  if (!top_of_zone_table)
    return;

  for (i = 0; i <= top_of_zone_table; i++) {
    if (zone_table[i].number >= bottom && zone_table[i].number <= top) {
      send_to_char(ch, "[%s%3d%s] %s%-*s %s%-1s%s\r\n",
        QGRN, zone_table[i].number, QNRM, QCYN, count_color_chars(zone_table[i].name)+30, zone_table[i].name,
        QYEL, zone_table[i].builders ? zone_table[i].builders : "None.", QNRM);
      counter++;
    }
  }

  if (!counter)
    send_to_char(ch, "  None found within those parameters.\r\n");
}

/* Prints all of the zone information for the selected zone. */
void print_zone(struct char_data *ch, zone_vnum vnum)
{
  zone_rnum rnum;
  int size_rooms, size_objects, size_mobiles, size_quests, size_shops, size_trigs, i, largest_table;
  room_vnum top, bottom;
  char buf[MAX_STRING_LENGTH];

  if ((rnum = real_zone(vnum)) == NOWHERE) {
    send_to_char(ch, "Zone #%d does not exist in the database.\r\n", vnum);
    return;
  }

  /* Locate the largest of the three, top_of_world, top_of_mobt, or top_of_objt. */
  if (top_of_world >= top_of_objt && top_of_world >= top_of_mobt)
    largest_table = top_of_world;
  else if (top_of_objt >= top_of_mobt && top_of_objt >= top_of_world)
    largest_table = top_of_objt;
  else
    largest_table = top_of_mobt;

  /* Initialize some of the variables. */
  size_rooms   = 0;
  size_objects = 0;
  size_mobiles = 0;
  size_shops   = 0;
  size_trigs   = 0;
  size_quests  = 0;
  top          = zone_table[rnum].top;
  bottom       = zone_table[rnum].bot;

  for (i = 0; i <= largest_table; i++) {
    if (i <= top_of_world)
      if (world[i].zone == rnum)
        size_rooms++;

    if (i <= top_of_objt)
      if (obj_index[i].vnum >= bottom && obj_index[i].vnum <= top)
        size_objects++;

    if (i <= top_of_mobt)
      if (mob_index[i].vnum >= bottom && mob_index[i].vnum <= top)
        size_mobiles++;
  }
  for (i = 0; i<= top_shop; i++)
    if (SHOP_NUM(i) >= bottom && SHOP_NUM(i) <= top)
      size_shops++;

  for (i = 0; i < top_of_trigt; i++)
    if (trig_index[i]->vnum >= bottom && trig_index[i]->vnum <= top)
      size_trigs++;

  size_quests = count_quests(bottom, top);
  sprintbitarray(zone_table[rnum].zone_flags, zone_bits, ZN_ARRAY_MAX, buf);

  /* Display all of the zone information at once. */
  send_to_char(ch,
    "%sVirtual Number = %s%d\r\n"
    "%sName of zone   = %s%s\r\n"
    "%sBuilders       = %s%s\r\n"
    "%sLifespan       = %s%d\r\n"
    "%sAge            = %s%d\r\n"
    "%sBottom of Zone = %s%d\r\n"
    "%sTop of Zone    = %s%d\r\n"
    "%sReset Mode     = %s%s\r\n"
    "%sZone Flags     = %s%s\r\n"
    "%sMin Level      = %s%d\r\n"
    "%sMax Level      = %s%d\r\n"
    "%sSize\r\n"
    "%s   Rooms       = %s%d\r\n"
    "%s   Objects     = %s%d\r\n"
    "%s   Mobiles     = %s%d\r\n"
    "%s   Shops       = %s%d\r\n"
    "%s   Triggers    = %s%d\r\n"
    "%s   Quests      = %s%d%s\r\n",
    QGRN, QCYN, zone_table[rnum].number,
    QGRN, QCYN, zone_table[rnum].name,
    QGRN, QCYN, zone_table[rnum].builders,
    QGRN, QCYN, zone_table[rnum].lifespan,
    QGRN, QCYN, zone_table[rnum].age,
    QGRN, QCYN, zone_table[rnum].bot,
    QGRN, QCYN, zone_table[rnum].top,
    QGRN, QCYN, zone_table[rnum].reset_mode ? ((zone_table[rnum].reset_mode == 1) ?
    "Reset when no players are in zone." : "Normal reset.") : "Never reset",
    QGRN, QCYN, buf, 
    QGRN, QCYN, zone_table[rnum].min_level, 
    QGRN, QCYN, zone_table[rnum].max_level,
    QGRN,
    QGRN, QCYN, size_rooms,
    QGRN, QCYN, size_objects,
    QGRN, QCYN, size_mobiles,
    QGRN, QCYN, size_shops,
    QGRN, QCYN, size_trigs,
    QGRN, QCYN, size_quests, QNRM);
}

/* List code by Ronald Evers. */
static void list_triggers(struct char_data *ch, zone_rnum rnum, trig_vnum vmin, trig_vnum vmax)
{
  int i, bottom, top, counter = 0;
  char trgtypes[256];

  /* Expect a minimum / maximum number if the rnum for the zone is NOWHERE. */
  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }


  /* Store the header for the room listing. */
  send_to_char (ch,
  "Index VNum    Trigger Name                                  Type\r\n"
  "----- ------- --------------------------------------------- ---------\r\n");

  /* Loop through the world and find each room. */
  for (i = 0; i < top_of_trigt; i++) {
    /** Check to see if this room is one of the ones needed to be listed.    **/
    if ((trig_index[i]->vnum >= bottom) && (trig_index[i]->vnum <= top)) {
      counter++;

      send_to_char(ch, "%4d) [%s%5d%s] %s%-45.45s%s ",
        counter, QGRN, trig_index[i]->vnum, QNRM, QCYN, trig_index[i]->proto->name, QNRM);

      if (trig_index[i]->proto->attach_type == OBJ_TRIGGER) {
        sprintbit(GET_TRIG_TYPE(trig_index[i]->proto), otrig_types, trgtypes, sizeof(trgtypes));
        send_to_char(ch, "obj %s%s%s\r\n", QYEL, trgtypes, QNRM);
      } else if (trig_index[i]->proto->attach_type==WLD_TRIGGER) {
        sprintbit(GET_TRIG_TYPE(trig_index[i]->proto), wtrig_types, trgtypes, sizeof(trgtypes));
        send_to_char(ch, "wld %s%s%s\r\n", QYEL, trgtypes, QNRM);
      } else {
        sprintbit(GET_TRIG_TYPE(trig_index[i]->proto), trig_types, trgtypes, sizeof(trgtypes));
        send_to_char(ch, "mob %s%s%s\r\n", QYEL, trgtypes, QNRM);
      }

    }
  }

 if (counter == 0) {
   if (rnum == NOWHERE)
     send_to_char(ch, "No triggers found from %d to %d\r\n", vmin, vmax);
   else
     send_to_char(ch, "No triggers found for zone #%d\r\n", zone_table[rnum].number);
  }
}
