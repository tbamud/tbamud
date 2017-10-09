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
#include "modify.h"
#include "spells.h"
 
#define MAX_OBJ_LIST 100
 
struct obj_list_item {
  obj_vnum vobj;
  int val;
};
/* local functions */
static void list_triggers(struct char_data *ch, zone_rnum rnum, trig_vnum vmin, trig_vnum vmax);
static void list_rooms(struct char_data *ch  , zone_rnum rnum, room_vnum vmin, room_vnum vmax);
static void list_mobiles(struct char_data *ch, zone_rnum rnum, mob_vnum vmin , mob_vnum vmax );
static void list_objects(struct char_data *ch, zone_rnum rnum, obj_vnum vmin , obj_vnum vmax );
static void list_shops(struct char_data *ch  , zone_rnum rnum, shop_vnum vmin, shop_vnum vmax);
static void list_zones(struct char_data *ch, zone_rnum rnum, zone_vnum vmin, zone_vnum vmax, char *name);

static void perform_mob_flag_list(struct char_data * ch, char *arg)
{
  int num, mob_flag, found = 0;
  size_t len;
  struct char_data *mob;
  char buf[MAX_STRING_LENGTH];

  mob_flag = atoi(arg);

  if (mob_flag < 0 || mob_flag > NUM_MOB_FLAGS) {
    send_to_char(ch, "Invalid flag number!\r\n");
    return;
  }

  len = snprintf(buf, sizeof(buf), "Listing mobiles with %s%s%s flag set.\r\n", QYEL, action_bits[mob_flag], QNRM);

  for (num = 0; num <= top_of_mobt; num++) {
    if (IS_SET_AR((mob_proto[num].char_specials.saved.act), mob_flag)) {

      if ((mob = read_mobile(num, REAL)) != NULL) {
        char_to_room(mob, 0);
        len += snprintf(buf + len, sizeof(buf) - len, "%s%3d. %s[%s%5d%s]%s Level %s%-3d%s %s%s\r\n", CCNRM(ch, C_NRM),++found,
                      CCCYN(ch, C_NRM), CCYEL(ch, C_NRM), GET_MOB_VNUM(mob), CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                      CCYEL(ch, C_NRM), GET_LEVEL(mob), CCNRM(ch, C_NRM), GET_NAME(mob), CCNRM(ch, C_NRM));
        extract_char(mob); /* Finished with the mob - remove it from the MUD */
        if (len > sizeof(buf))
		  break;
      }
    }
  }
  if (!found)
    send_to_char(ch,"None Found!\r\n");
  else
    page_string(ch->desc, buf, TRUE);
  return;
}

static void perform_mob_level_list(struct char_data * ch, char *arg)
{
  int num, mob_level, found = 0;
  size_t len;
  struct char_data *mob;
  char buf[MAX_STRING_LENGTH];

  mob_level = atoi(arg);

  if (mob_level < 0 || mob_level > 99) {
    send_to_char(ch, "Invalid mob level!\r\n");
    return;
  }

  len = snprintf(buf, sizeof(buf), "Listing mobiles of level %s%d%s\r\n", QYEL, mob_level, QNRM);
  for (num = 0; num <= top_of_mobt; num++) {
    if ((mob_proto[num].player.level) == mob_level) {
      if ((mob = read_mobile(num, REAL)) != NULL) {
        char_to_room(mob, 0);
        len += snprintf(buf + len, sizeof(buf) - len, "%s%3d. %s[%s%5d%s]%s %s%s\r\n", CCNRM(ch, C_NRM),++found,
                      CCCYN(ch, C_NRM), CCYEL(ch, C_NRM), GET_MOB_VNUM(mob), CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                      GET_NAME(mob), CCNRM(ch, C_NRM));
        extract_char(mob); /* Finished with the mob - remove it from the MUD */
        if (len > sizeof(buf))
		  break;
      }
    }
  }
  if (!found)
    send_to_char(ch,"None Found!\r\n");
  else
    page_string(ch->desc, buf, TRUE);

  return;
}

static void add_to_obj_list(struct obj_list_item *lst, int num_items, obj_vnum nvo, int nval)
{
  int j, tmp_v;
  obj_vnum tmp_ov;

  for (j = 0; j < num_items; j++) {
    if (nval > lst[j].val) {
      tmp_ov = lst[j].vobj;
      tmp_v  = lst[j].val;

      lst[j].vobj = nvo;
      lst[j].val  = nval;

      nvo  = tmp_ov;
      nval = tmp_v;
    }
  }
}

static void perform_obj_type_list(struct char_data * ch, char *arg)
{
  int num, itemtype, v1, v2, found = 0;
  size_t len = 0, tmp_len = 0;
  obj_vnum ov;
  obj_rnum r_num;
  char buf[MAX_STRING_LENGTH];

  itemtype = atoi(arg);

  len = snprintf(buf, sizeof(buf), "Listing all objects of type %s[%s]%s\r\n",
       QYEL, item_types[itemtype], QNRM);

  for (num = 0; num <= top_of_objt; num++) {
    if (obj_proto[num].obj_flags.type_flag == itemtype) {
      if ((r_num = real_object(obj_index[num].vnum)) != NOTHING) { /* Seems silly? */
        /* Set default vals, which may be changed below */
        ov = obj_index[num].vnum;
        v1 = (obj_proto[num].obj_flags.value[0]);

        switch (itemtype) {
          case ITEM_LIGHT:
            v1 = (obj_proto[num].obj_flags.value[2]);
            if (v1 == -1)
              tmp_len = snprintf(buf+len, sizeof(buf)-len, "%s%3d%s) %s[%s%5d%s]%s INFINITE%s %s%s\r\n",
                   QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QBRED, QCYN, obj_proto[r_num].short_description, QNRM);
            else
              tmp_len = snprintf(buf+len, sizeof(buf)-len,"%s%3d%s) %s[%s%5d%s]%s (%-3dhrs) %s%s%s\r\n",
                   QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, QNRM);
            break;

          case ITEM_SCROLL:
          case ITEM_POTION:
            tmp_len = snprintf(buf+len, sizeof(buf)-len,"%s%3d%s) %s[%s%8d%s] %s%s\r\n",
                 QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, obj_proto[r_num].short_description, QNRM);
            break;

          case ITEM_WAND:
          case ITEM_STAFF:
            v1 = (obj_proto[num].obj_flags.value[1]);
            v2 = (obj_proto[num].obj_flags.value[3]);
            tmp_len = snprintf(buf+len, sizeof(buf)-len,"%s%3d%s) %s[%s%8d%s]%s (%dx%s) %s%s%s\r\n",
                 QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, skill_name(v2), QCYN, obj_proto[r_num].short_description, QNRM);
            break;

          case ITEM_WEAPON:
            v1 = ((obj_proto[num].obj_flags.value[2]+1)*(obj_proto[r_num].obj_flags.value[1])) / 2;
            tmp_len = snprintf(buf+len, sizeof(buf)-len,"%s%3d%s) %s[%s%8d%s]%s (%d Avg Dam) %s%s%s\r\n",
                 QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, QNRM);
            break;

          case ITEM_ARMOR:
            tmp_len = snprintf(buf+len, sizeof(buf)-len,"%s%3d%s) %s[%s%8d%s]%s (%dAC) %s%s%s\r\n",
                 QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, QNRM);
            break;

          case ITEM_CONTAINER:
            tmp_len = snprintf(buf+len, sizeof(buf)-len,"%s%3d%s) %s[%s%8d%s]%s (Max: %d) %s%s%s\r\n",
                 QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, QNRM);
            break;

          case ITEM_DRINKCON:
          case ITEM_FOUNTAIN:
            if (v1 != -1)
              tmp_len = snprintf(buf+len, sizeof(buf)-len,"%s%3d%s) %s[%s%8d%s]%s (Max: %d) %s%s%s\r\n",
                   QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, QNRM);
            else
              tmp_len = snprintf(buf+len, sizeof(buf)-len,"%s%3d%s) %s[%s%8d%s] %sINFINITE%s %s%s\r\n",
                   QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QBRED, QCYN, obj_proto[r_num].short_description, QNRM);
            break;

          case ITEM_FOOD:
            v2 = (obj_proto[num].obj_flags.value[3]);
            if (v2 != 0)
              tmp_len = snprintf(buf+len, sizeof(buf)-len,"%s%3d%s) %s[%s%8d%s]%s (%dhrs) %s%s %sPoisoned!%s\r\n",
                   QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, QBGRN, QNRM);
            else
              tmp_len = snprintf(buf+len, sizeof(buf)-len,"%s%3d%s) %s[%s%8d%s]%s (%dhrs) %s%s%s\r\n",
                   QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, QNRM);
            break;

          case ITEM_MONEY:
            tmp_len = snprintf(buf+len, sizeof(buf)-len,"%s%3d%s) %s[%s%8d%s] %s%s (%s%d coins%s)\r\n",
                 QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, obj_proto[r_num].short_description, QNRM, QYEL, v1, QNRM);
            break;

          /* The 'normal' items - don't provide extra info */
          case ITEM_TREASURE:
          case ITEM_TRASH:
          case ITEM_OTHER:
          case ITEM_WORN:
          case ITEM_NOTE:
          case ITEM_PEN:
          case ITEM_BOAT:
          case ITEM_KEY:
            tmp_len = snprintf(buf+len, sizeof(buf)-len,"%s%3d%s) %s[%s%8d%s] %s%s\r\n",
                 QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, obj_proto[r_num].short_description, QNRM);
            break;

          default:
            send_to_char(ch, "Not a valid item type");
            return;
        }
        if (len + tmp_len < sizeof(buf) - 1)
          len += tmp_len;
        else {
		  buf[sizeof(buf) -1] = '\0';
		  break;	
		}
      }
    }
  }
  page_string(ch->desc, buf, TRUE);
}

static void perform_obj_aff_list(struct char_data * ch, char *arg)
{
  int num, i, apply, v1 = 0, found = 0;
  size_t len = 0, tmp_len = 0;
  struct obj_list_item lst[MAX_OBJ_LIST];
  obj_rnum r_num;
  obj_vnum ov;
  char buf[MAX_STRING_LENGTH];

  for (i = 0; i < MAX_OBJ_LIST; i++) {
    lst[i].vobj = NOTHING;
    lst[i].val  = 0;
  }
  apply = atoi(arg);

  if (apply <= 0 || apply >= NUM_APPLIES) {
     send_to_char(ch, "Not a valid affect");
     return;
  }                                   /* Special cases below */
  else if ((apply == APPLY_CLASS) ||  /* olist affect 7 is Weapon Damage      */
           (apply == APPLY_LEVEL) ) { /* olist affect 8 is AC-Apply for Armor */
    for (num = 0; num <= top_of_objt; num++) {
      if ((apply == APPLY_CLASS && obj_proto[num].obj_flags.type_flag == ITEM_WEAPON) ||
          (apply == APPLY_LEVEL && obj_proto[num].obj_flags.type_flag == ITEM_ARMOR) ) {
        ov = obj_index[num].vnum;
        if (apply == APPLY_CLASS)
          v1 = ((obj_proto[num].obj_flags.value[2]+1)*(obj_proto[num].obj_flags.value[1])/2);
        else
          v1 = (obj_proto[num].obj_flags.value[0]);

        if ((r_num = real_object(ov)) != NOTHING)
          add_to_obj_list(lst, MAX_OBJ_LIST, ov, v1);
      }
    }

    if (apply == APPLY_CLASS)
      len = snprintf(buf, sizeof(buf), "Highest average damage per hit for Weapons\r\n");
    else if (apply == APPLY_LEVEL)
      len = snprintf(buf, sizeof(buf), "Highest AC Apply for Armor\r\n");

    for (i = 0; i < MAX_OBJ_LIST; i++){
      if ((r_num = real_object(lst[i].vobj)) != NOTHING) {
        tmp_len = snprintf(buf+len, sizeof(buf)-len, "%s%3d%s) %s[%s%5d%s] %s%3d %s%-*s %s[%s]%s%s\r\n",
                  QGRN, ++found, QNRM, QCYN, QYEL, lst[i].vobj, QCYN,
                  QYEL, lst[i].val, QCYN, 42+count_color_chars(obj_proto[num].short_description),
                  obj_proto[r_num].short_description,
                  QYEL, item_types[obj_proto[num].obj_flags.type_flag], QNRM,
                  obj_proto[num].proto_script ? " [TRIG]" : "");
        len += tmp_len;
        if (len > sizeof(buf))
          break;
      }
    }
    page_string(ch->desc, buf, TRUE);
    return;  /* End of special-case handling */
  }
  /* Non-special cases, list objects by affect */
  for (num = 0; num <= top_of_objt; num++){
    for (i = 0; i < MAX_OBJ_AFFECT; i++){
      if (obj_proto[num].affected[i].modifier) {
        if (obj_proto[num].affected[i].location == apply){
          ov = obj_index[num].vnum;
          v1 = obj_proto[num].affected[i].modifier;

          if ((r_num = real_object(ov)) != NOTHING)
            add_to_obj_list(lst, MAX_OBJ_LIST, ov, v1);
        }
      }
    }
  }
  len = snprintf(buf, sizeof(buf), "Objects with highest %s affect\r\n", apply_types[(apply)]);
  for (i = 0; i < MAX_OBJ_LIST; i++) {
    if ((r_num = real_object(lst[i].vobj)) != NOTHING) {
      tmp_len = snprintf(buf+len, sizeof(buf)-len, "%s%3d%s) %s[%s%8d%s] %s%3d %s%-*s %s[%s]%s%s\r\n",
                QGRN, ++found, QNRM, QCYN, QYEL, lst[i].vobj, QCYN,
                QYEL, lst[i].val, QCYN, 42+count_color_chars(obj_proto[num].short_description),
                obj_proto[r_num].short_description,
                QYEL, item_types[obj_proto[r_num].obj_flags.type_flag], QNRM,
                obj_proto[r_num].proto_script ? " [TRIG]" : "");
      len += tmp_len;
    }
  }
  page_string(ch->desc, buf, TRUE);
}

static void perform_obj_name_list(struct char_data * ch, char *arg)
{
  int num, found = 0;
  size_t len = 0, tmp_len = 0;
  obj_vnum ov;
  char buf[MAX_STRING_LENGTH];

  len = snprintf(buf, sizeof(buf), "Objects with the name '%s'\r\n"
  "Index VNum    Num   Object Name                                Object Type\r\n"
  "----- ------- ----- ------------------------------------------ ----------------\r\n", arg);
  for (num = 0; num <= top_of_objt; num++) {
    if (is_name(arg, obj_proto[num].name)) {
      ov = obj_index[num].vnum;
      tmp_len = snprintf(buf+len, sizeof(buf)-len, "%s%4d%s) %s[%s%5d%s] %s(%s%3d%s)%s %-*s%s [%s]%s%s\r\n",
                QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM,
                QGRN, obj_index[num].number, QNRM, QCYN, 42+count_color_chars(obj_proto[num].short_description),
                obj_proto[num].short_description, QYEL, item_types[obj_proto[num].obj_flags.type_flag], QNRM,
                obj_proto[num].proto_script ? " [TRIG]" : "");
      len += tmp_len;
      if (len > sizeof(buf))
        break;
    }
  }

  page_string(ch->desc, buf, TRUE);
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
  bool use_name = FALSE;
  int i;

  two_arguments(argument, smin, smax);

  if (!*smin || *smin == '.') {
    rzone = world[IN_ROOM(ch)].zone;
  } else if (!*smax) {
    rzone = real_zone(atoi(smin));

    if ((rzone == NOWHERE || rzone == 0) && subcmd == SCMD_OASIS_ZLIST && !isdigit(*smin)) {
      /* Must be zlist, with builder name as arg */
      use_name = TRUE;
    } else if (rzone == NOWHERE) {
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

      if (is_abbrev(arg, "help")) {
        send_to_char(ch, "Usage: %smlist <zone>%s        - List mobiles in a zone\r\n", QYEL, QNRM);
        send_to_char(ch, "       %smlist <vnum> <vnum>%s - List a range of mobiles by vnum\r\n", QYEL, QNRM);
        send_to_char(ch, "       %smlist level <num>%s   - List all mobiles of a specified level\r\n", QYEL, QNRM);
	    send_to_char(ch, "       %smlist flags <num>%s - List all mobiles with flag set\r\n", QYEL, QNRM);
		send_to_char(ch, "Just type %smlist flags%s to view available options.\r\n", QYEL, QNRM);
        return;
      }
      else if (is_abbrev(arg, "level") || is_abbrev(arg, "flags")) {
        if (!*arg2) {
          send_to_char(ch, "Which mobile flag or level do you want to list?\r\n");
          for (i = 0; i < NUM_MOB_FLAGS; i++) {
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
        list_mobiles(ch, rzone, vmin, vmax);
      break;
    case SCMD_OASIS_OLIST:
      two_arguments(argument, arg, arg2);

      if (is_abbrev(arg, "help")) {
        send_to_char(ch, "Usage: %solist <zone>%s        - List objects in a zone\r\n", QYEL, QNRM);
        send_to_char(ch, "       %solist <vnum> <vnum>%s - List a range of objects by vnum\r\n", QYEL, QNRM);
        send_to_char(ch, "       %solist <name>%s        - List all named objects with count\r\n", QYEL, QNRM);
        send_to_char(ch, "       %solist type <num>%s    - List all objects of a specified type\r\n", QYEL, QNRM);
        send_to_char(ch, "       %solist affect <num>%s  - List top %d objects with affect\r\n", QYEL, QNRM, MAX_OBJ_LIST);
        send_to_char(ch, "Just type %solist affect%s or %solist type%s to view available options\r\n", QYEL, QNRM, QYEL, QNRM);
        return;
      } else if (is_abbrev(arg, "type") || is_abbrev(arg, "affect")) {
        if (is_abbrev(arg, "type")) {
          if (!*arg2) {
            send_to_char(ch, "Which object type do you want to list?\r\n");
            for (i=1; i<NUM_ITEM_TYPES; i++)
            {
              send_to_char(ch, "%s%2d%s-%s%-14s%s", QNRM, i, QNRM, QYEL, item_types[i], QNRM);
              if (!(i%4))  send_to_char(ch, "\r\n");
            }
            send_to_char(ch, "\r\n");
            send_to_char(ch, "Usage: %solist type <num>%s\r\n", QYEL, QNRM);
            send_to_char(ch, "Displays objects of the selected type.\r\n");

            return;
          }
          perform_obj_type_list(ch, arg2);
        } else {  /* Assume arg = affect */
          if (!*arg2) {
            send_to_char(ch, "Which object affect do you want to list?\r\n");
            for (i = 0; i < NUM_APPLIES; i++) {
              if (i == APPLY_CLASS)       /* Special Case 1 - Weapon Dam */
                send_to_char(ch, "%s%2d-%s%-14s%s", QNRM, i, QYEL, "Weapon Dam", QNRM);
              else if (i == APPLY_LEVEL)  /* Special Case 2 - Armor AC Apply */
                send_to_char(ch, "%s%2d-%s%-14s%s", QNRM, i, QYEL, "AC Apply", QNRM);
              else
                send_to_char(ch, "%s%2d-%s%-14s%s", QNRM, i, QYEL, apply_types[i], QNRM);
              if (!((i+1)%4))  send_to_char(ch, "\r\n");
            }
            send_to_char(ch, "\r\n");
            send_to_char(ch, "Usage: %solist affect <num>%s\r\n", QYEL, QNRM);
            send_to_char(ch, "Displays top %d objects, in order, with the selected affect.\r\n", MAX_OBJ_LIST);

            return;
          }
          perform_obj_aff_list(ch, arg2);
        }
      } else if (*arg && !isdigit(*arg)) {
        perform_obj_name_list(ch, arg);
      } else
        list_objects(ch, rzone, vmin, vmax);
      break;
    case SCMD_OASIS_RLIST: list_rooms(ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_TLIST: list_triggers(ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_SLIST: list_shops(ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_QLIST: list_quests(ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_ZLIST:
      if (!*smin)        /* No args - list all zones */
        list_zones(ch, NOWHERE, 0, zone_table[top_of_zone_table].number, NULL);
      else if (use_name) /* Builder name as arg */
        list_zones(ch, NOWHERE, 0, zone_table[top_of_zone_table].number, smin);
      else               /* Numerical args */
        list_zones(ch, rzone, vmin, vmax, NULL);
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
      for (j = 0; j < DIR_COUNT; j++) {
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
  size_t len;
  char buf[MAX_STRING_LENGTH];

  /* Expect a minimum / maximum number if the rnum for the zone is NOWHERE. */
  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  len = strlcpy(buf,
  "Index VNum    Room Name                                    Exits\r\n"
  "----- ------- -------------------------------------------- -----\r\n",
  sizeof(buf));

  if (!top_of_world)
    return;

  for (i = 0; i <= top_of_world; i++) {

    /** Check to see if this room is one of the ones needed to be listed.    **/
    if ((world[i].number >= bottom) && (world[i].number <= top)) {
      counter++;

      len += snprintf(buf + len, sizeof(buf) - len, "%4d) [%s%-5d%s] %s%-*s%s %s",
                          counter, QGRN, world[i].number, QNRM,
                          QCYN, count_color_chars(world[i].name)+44, world[i].name, QNRM,
                          world[i].proto_script ? "[TRIG] " : ""
                          );

      for (j = 0; j < DIR_COUNT; j++) {
        if (W_EXIT(i, j) == NULL)
          continue;
        if (W_EXIT(i, j)->to_room == NOWHERE)
          continue;

        if (world[W_EXIT(i, j)->to_room].zone != world[i].zone)
          len += snprintf(buf + len, sizeof(buf) - len, "(%s%d%s)", QYEL, world[W_EXIT(i, j)->to_room].number, QNRM);

      }

      len += snprintf(buf + len, sizeof(buf) - len, "\r\n");
      
      if (len > sizeof(buf))
		break;
    }
  }

  if (counter == 0)
    send_to_char(ch, "No rooms found for zone/range specified.\r\n");
  else
    page_string(ch->desc, buf, TRUE);
}

/* List all mobiles in a zone. */
static void list_mobiles(struct char_data *ch, zone_rnum rnum, mob_vnum vmin, mob_vnum vmax)
{
  mob_rnum i;
  mob_vnum bottom, top;
  int counter = 0;
  size_t len;
  char buf[MAX_STRING_LENGTH];

  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  len = strlcpy(buf,
  "Index VNum    Mobile Name                                  Level\r\n"
  "----- ------- -------------------------------------------- -----\r\n",
  sizeof(buf));

  if (!top_of_mobt)
    return;

  for (i = 0; i <= top_of_mobt; i++) {
    if (mob_index[i].vnum >= bottom && mob_index[i].vnum <= top) {
      counter++;

      len += snprintf(buf + len, sizeof(buf) - len, "%s%4d%s) [%s%-5d%s] %s%-*s %s[%4d]%s%s\r\n",
                   QGRN, counter, QNRM, QGRN, mob_index[i].vnum, QNRM,
                   QCYN, count_color_chars(mob_proto[i].player.short_descr)+44, mob_proto[i].player.short_descr,
                   QYEL, mob_proto[i].player.level, QNRM,
                   mob_proto[i].proto_script ? " [TRIG]" : ""
              );
      if (len > sizeof(buf))
		break;
    }
  }

  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
  else
    page_string(ch->desc, buf, TRUE);
}

/* List all objects in a zone. */
static void list_objects(struct char_data *ch, zone_rnum rnum, obj_vnum vmin, obj_vnum vmax)
{
  obj_rnum i;
  obj_vnum bottom, top;
  char buf[MAX_STRING_LENGTH];
  int counter = 0;
  size_t len;

  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  len = strlcpy(buf,
	"Index VNum    Object Name                                  Object Type\r\n"
	"----- ------- -------------------------------------------- ----------------\r\n",
	sizeof(buf));

  if (!top_of_objt)
    return;

  for (i = 0; i <= top_of_objt; i++) {
    if (obj_index[i].vnum >= bottom && obj_index[i].vnum <= top) {
      counter++;

      len += snprintf(buf + len, sizeof(buf) - len, "%s%4d%s) [%s%-5d%s] %s%-*s %s[%s]%s%s\r\n",
                   QGRN, counter, QNRM, QGRN, obj_index[i].vnum, QNRM,
                   QCYN, count_color_chars(obj_proto[i].short_description)+44, obj_proto[i].short_description, QYEL,
                   item_types[obj_proto[i].obj_flags.type_flag], QNRM,
                   obj_proto[i].proto_script ? " [TRIG]" : ""
              );

      if (len > sizeof(buf))
		break;
    }
  }

  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
  else
    page_string(ch->desc, buf, TRUE);
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
        send_to_char(ch, " %sNone.%s", QCYN, QNRM);

      send_to_char(ch, "\r\n");
    }
  }

  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
}

/* List all zones in the world (sort of like 'show zones'). */
static void list_zones(struct char_data *ch, zone_rnum rnum, zone_vnum vmin, zone_vnum vmax, char *name)
{
  int counter = 0;
  size_t len = 0, tmp_len = 0;
  zone_rnum i;
  zone_vnum bottom, top;
  char buf[MAX_STRING_LENGTH];
  bool use_name=FALSE;

  bottom = vmin;
  top    = vmax;

  if (rnum != NOWHERE) {
    /* Only one parameter was supplied - just list that zone */
     print_zone(ch, zone_table[rnum].number);
    return;
  } else if (name && *name) {
    use_name = TRUE;
    if (!vmin)
      bottom = zone_table[0].number;                 /* Lowest Zone  */
    if (!vmax)
      top    = zone_table[top_of_zone_table].number; /* Highest Zone */
  }

  len = snprintf(buf, sizeof(buf),
  "VNum  Zone Name                      Builder(s)\r\n"
  "----- ------------------------------ --------------------------------------\r\n");

  if (!top_of_zone_table)
    return;

  for (i = 0; i <= top_of_zone_table; i++) {
    if (zone_table[i].number >= bottom && zone_table[i].number <= top) {
      if ((!use_name) || (is_name(name, zone_table[i].builders))) {
        counter++;

        tmp_len = snprintf(buf+len, sizeof(buf)-len, "[%s%3d%s] %s%-*s %s%-1s%s\r\n",
            QGRN, zone_table[i].number, QNRM, QCYN, count_color_chars(zone_table[i].name)+30, zone_table[i].name,
            QYEL, zone_table[i].builders ? zone_table[i].builders : "None.", QNRM);
        len += tmp_len;
        if (len > sizeof(buf))
          break;
      }
    }
  }

  if (!counter)
    send_to_char(ch, "  None found within those parameters.\r\n");
  else
    page_string(ch->desc, buf, TRUE);
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
