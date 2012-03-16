/**************************************************************************
*  File: zedit.c                                           Part of tbaMUD *
*  Usage: Oasis OLC - Zones and commands.                                 *
*                                                                         *
* Copyright 1996 Harvey Gilpin. 1997-2001 George Greer.                   *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "constants.h"
#include "genolc.h"
#include "genzon.h"
#include "oasis.h"
#include "dg_scripts.h"

/* Nasty internal macros to clean up the code. */
#define MYCMD		(OLC_ZONE(d)->cmd[subcmd])
#define OLC_CMD(d)	(OLC_ZONE(d)->cmd[OLC_VAL(d)])
#define MAX_DUPLICATES 100

/* local functions */
static int start_change_command(struct descriptor_data *d, int pos);
static void zedit_setup(struct descriptor_data *d, int room_num);
static void zedit_new_zone(struct char_data *ch, zone_vnum vzone_num, room_vnum bottom, room_vnum top);
static void zedit_save_internally(struct descriptor_data *d);
static void zedit_save_to_disk(int zone_num);
static void zedit_disp_menu(struct descriptor_data *d);
static void zedit_disp_comtype(struct descriptor_data *d);
static void zedit_disp_arg1(struct descriptor_data *d);
static void zedit_disp_arg2(struct descriptor_data *d);
static void zedit_disp_arg3(struct descriptor_data *d);

ACMD(do_oasis_zedit)
{
  int number = NOWHERE, save = 0, real_num;
  struct descriptor_data *d;
  char *stop;
  char sbot[MAX_STRING_LENGTH];
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  room_vnum bottom, top;

  /* No building as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

  /* Parse any arguments. */
  stop = one_argument(two_arguments(argument, buf1, buf2), sbot);

  /* If no argument was given, use the zone the builder is standing in. */
  if (!*buf1)
    number = GET_ROOM_VNUM(IN_ROOM(ch));
  else if (!isdigit(*buf1)) {
    if (str_cmp("save", buf1) == 0) {
      save = TRUE;

      if (is_number(buf2))
        number = atoidx(buf2);
      else if (GET_OLC_ZONE(ch) > 0) {
        zone_rnum zlok;

        if ((zlok = real_zone(GET_OLC_ZONE(ch))) == NOWHERE)
          number = NOWHERE;
        else
          number = genolc_zone_bottom(zlok);
      }

      if (number == NOWHERE) {
        send_to_char(ch, "Save which zone?\r\n");
        return;
      }
    } else if (GET_LEVEL(ch) >= LVL_IMPL) {
      if (str_cmp("new", buf1) || !stop || !*stop)
        send_to_char(ch, "Format: zedit new <zone number> <bottom-room> "
           "<upper-room>\r\n");
      else {
        if (atoi(stop) < 0 || atoi(sbot) < 0) {
          send_to_char(ch, "Zones cannot contain negative vnums.\r\n");
          return;
        }
        number = atoidx(buf2);
        if (number < 0)
          number = NOWHERE;
        bottom = atoidx(sbot);
        top = atoidx(stop);

        /* Setup the new zone (displays the menu to the builder). */
        zedit_new_zone(ch, number, bottom, top);
      }

      /* Done now, exit the function. */
      return;

    } else {
      send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
  }

  /* If a numeric argument was given, retrieve it. */
  if (number == NOWHERE)
    number = atoidx(buf1);

  /* Check that nobody is currently editing this zone. */
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_ZEDIT) {
      if (d->olc && OLC_NUM(d) == number) {
        send_to_char(ch, "That zone is currently being edited by %s.\r\n",
          PERS(d->character, ch));
        return;
      }
    }
  }

  /* Store the builder's descriptor in d. */
  d = ch->desc;

  /* Give the builder's descriptor an OLC structure. */
  if (d->olc) {
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_oasis_zedit: Player already "
      "had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  /* Find the zone. */
  OLC_ZNUM(d) = save ? real_zone(number) : real_zone_by_thing(number);
  if (OLC_ZNUM(d) == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");

    /* Free the descriptor's OLC structure. */
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /* Everyone but IMPLs can only edit zones they have been assigned. */
  if (!can_edit_zone(ch, OLC_ZNUM(d))) {
    send_cannot_edit(ch, zone_table[OLC_ZNUM(d)].number);
    /* Free the OLC structure. */
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /* If we need to save, then save the zone. */
  if (save) {
    send_to_char(ch, "Saving all zone information for zone %d.\r\n",
      zone_table[OLC_ZNUM(d)].number);
    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE,
      "OLC: %s saves zone information for zone %d.", GET_NAME(ch),
      zone_table[OLC_ZNUM(d)].number);

    /* Save the zone information to the zone file. */
    save_zone(OLC_ZNUM(d));

    /* Free the descriptor's OLC structure. */
    free(d->olc);
    d->olc = NULL;
    return;
  }

  OLC_NUM(d) = number;

  if ((real_num = real_room(number)) == NOWHERE) {
    write_to_output(d, "That room does not exist.\r\n");

    /* Free the descriptor's OLC structure. */
    free(d->olc);
    d->olc = NULL;
    return;
  }

  zedit_setup(d, real_num);
  STATE(d) = CON_ZEDIT;

  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  mudlog(CMP, LVL_IMMORT, TRUE, "OLC: %s starts editing zone %d allowed zone %d",
    GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}

static void zedit_setup(struct descriptor_data *d, int room_num)
{
  struct zone_data *zone;
  int subcmd = 0, count = 0, cmd_room = NOWHERE, i;

  /* Allocate one scratch zone structure. */
  CREATE(zone, struct zone_data, 1);

  /* Copy all the zone header information over. */
  zone->name = strdup(zone_table[OLC_ZNUM(d)].name);
  if (zone_table[OLC_ZNUM(d)].builders)
    zone->builders = strdup(zone_table[OLC_ZNUM(d)].builders);
  zone->lifespan = zone_table[OLC_ZNUM(d)].lifespan;
  zone->bot = zone_table[OLC_ZNUM(d)].bot;
  zone->top = zone_table[OLC_ZNUM(d)].top;
  zone->reset_mode = zone_table[OLC_ZNUM(d)].reset_mode;
  /* The remaining fields are used as a 'has been modified' flag */
  zone->number = 0;	/* Header information has changed.	*/
  zone->age = 0;	/* The commands have changed.		*/

  for (i=0; i<ZN_ARRAY_MAX; i++)
    zone->zone_flags[(i)] = zone_table[OLC_ZNUM(d)].zone_flags[(i)];

  zone->min_level = zone_table[OLC_ZNUM(d)].min_level;
  zone->max_level = zone_table[OLC_ZNUM(d)].max_level;

  /* Start the reset command list with a terminator. */
  CREATE(zone->cmd, struct reset_com, 1);
  zone->cmd[0].command = 'S';

  /* Add all entries in zone_table that relate to this room. */
  while (ZCMD(OLC_ZNUM(d), subcmd).command != 'S') {
    switch (ZCMD(OLC_ZNUM(d), subcmd).command) {
    case 'M':
    case 'O':
    case 'T':
    case 'V':
      cmd_room = ZCMD(OLC_ZNUM(d), subcmd).arg3;
      break;
    case 'D':
    case 'R':
      cmd_room = ZCMD(OLC_ZNUM(d), subcmd).arg1;
      break;
    default:
      break;
    }
    if (cmd_room == room_num) {
      add_cmd_to_list(&(zone->cmd), &ZCMD(OLC_ZNUM(d), subcmd), count);
      count++;
    }
    subcmd++;
  }

  OLC_ZONE(d) = zone;
  /*
   * Display main menu.
   */
  zedit_disp_menu(d);
}

/* Create a new zone. */
static void zedit_new_zone(struct char_data *ch, zone_vnum vzone_num, room_vnum bottom, room_vnum top)
{
  int result;
  const char *error;
  struct descriptor_data *dsc;

  if ((result = create_new_zone(vzone_num, bottom, top, &error)) == NOWHERE) {
    write_to_output(ch->desc, "%s", error);
    return;
  }

  for (dsc = descriptor_list; dsc; dsc = dsc->next) {
    switch (STATE(dsc)) {
      case CON_REDIT:
        OLC_ROOM(dsc)->zone += (OLC_ZNUM(dsc) >= result);
        /* Fall through. */
      case CON_ZEDIT:
      case CON_MEDIT:
      case CON_SEDIT:
      case CON_OEDIT:
      case CON_TRIGEDIT:
      case CON_QEDIT:
        OLC_ZNUM(dsc) += (OLC_ZNUM(dsc) >= result);
        break;
      default:
        break;
    }
  }

  zedit_save_to_disk(result); /* save to disk .. */

  mudlog(BRF, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "OLC: %s creates new zone #%d", GET_NAME(ch), vzone_num);
  write_to_output(ch->desc, "Zone created successfully.\r\n");
}

/* Save all the information in the player's temporary buffer back into
 * the current zone table. */
static void zedit_save_internally(struct descriptor_data *d)
{
  int	mobloaded = FALSE,
	objloaded = FALSE,
	subcmd, i;
  room_rnum room_num = real_room(OLC_NUM(d));

  if (room_num == NOWHERE) {
    log("SYSERR: zedit_save_internally: OLC_NUM(d) room %d not found.", OLC_NUM(d));
    return;
  }

  remove_room_zone_commands(OLC_ZNUM(d), room_num);

  /* Now add all the entries in the players descriptor list */
  for (subcmd = 0; MYCMD.command != 'S'; subcmd++) {
    /* Since Circle does not keep track of what rooms the 'G', 'E', and
     * 'P' commands are exitted in, but OasisOLC groups zone commands
     * by rooms, this creates interesting problems when builders use these
     * commands without loading a mob or object first.  This fix prevents such
     * commands from being saved and 'wandering' through the zone command
     * list looking for mobs/objects to latch onto.
     * C.Raehl 4/27/99 */
    switch (MYCMD.command) {
      /* Possible fail cases. */
      case 'G':
      case 'E':
        if (mobloaded)
          break;
        write_to_output(d, "Equip/Give command not saved since no mob was loaded first.\r\n");
        continue;
      case 'P':
        if (objloaded)
          break;
        write_to_output(d, "Put command not saved since another object was not loaded first.\r\n");
        continue;
      /* Pass cases. */
      case 'M':
        mobloaded = TRUE;
        break;
      case 'O':
        objloaded = TRUE;
        break;
      default:
        mobloaded = objloaded = FALSE;
        break;
    }
    add_cmd_to_list(&(zone_table[OLC_ZNUM(d)].cmd), &MYCMD, subcmd);
  }

  /* Finally, if zone headers have been changed, copy over */
  if (OLC_ZONE(d)->number) {
    free(zone_table[OLC_ZNUM(d)].name);
    free(zone_table[OLC_ZNUM(d)].builders);

    zone_table[OLC_ZNUM(d)].name = strdup(OLC_ZONE(d)->name);
    zone_table[OLC_ZNUM(d)].builders = strdup(OLC_ZONE(d)->builders);
    zone_table[OLC_ZNUM(d)].bot = OLC_ZONE(d)->bot;
    zone_table[OLC_ZNUM(d)].top = OLC_ZONE(d)->top;
    zone_table[OLC_ZNUM(d)].reset_mode = OLC_ZONE(d)->reset_mode;
    zone_table[OLC_ZNUM(d)].lifespan = OLC_ZONE(d)->lifespan;
    zone_table[OLC_ZNUM(d)].min_level = OLC_ZONE(d)->min_level;
    zone_table[OLC_ZNUM(d)].max_level = OLC_ZONE(d)->max_level;
    for (i=0; i<ZN_ARRAY_MAX; i++)
      zone_table[OLC_ZNUM(d)].zone_flags[(i)] = OLC_ZONE(d)->zone_flags[(i)];
  }
  add_to_save_list(zone_table[OLC_ZNUM(d)].number, SL_ZON);
}

static void zedit_save_to_disk(int zone)
{
  save_zone(zone);
}

/* Error check user input and then setup change */
static int start_change_command(struct descriptor_data *d, int pos)
{
  if (pos < 0 || pos >= count_commands(OLC_ZONE(d)->cmd))
    return 0;

  /* Ok, let's get editing. */
  OLC_VAL(d) = pos;
  return 1;
}

/*------------------------------------------------------------------*/
void zedit_disp_flag_menu(struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH];

  clear_screen(d);
  column_list(d->character, 0, zone_bits, NUM_ZONE_FLAGS, TRUE);

  sprintbitarray(OLC_ZONE(d)->zone_flags, zone_bits, ZN_ARRAY_MAX, bits);
  write_to_output(d, "\r\nZone flags: \tc%s\tn\r\n"
         "Enter Zone flags, 0 to quit : ", bits);
  OLC_MODE(d) = ZEDIT_ZONE_FLAGS;
}

/*------------------------------------------------------------------*/
bool zedit_get_levels(struct descriptor_data *d, char *buf)
{
  /* Create a string for the recommended levels for this zone. */
  if ((OLC_ZONE(d)->min_level == -1) && (OLC_ZONE(d)->max_level == -1)) {
    sprintf(buf, "<Not Set!>");
    return FALSE;
  }

  if (OLC_ZONE(d)->min_level == -1) {
    sprintf(buf, "Up to level %d", OLC_ZONE(d)->max_level);
    return TRUE;
  }

  if (OLC_ZONE(d)->max_level == -1) {
    sprintf(buf, "Above level %d", OLC_ZONE(d)->min_level);
    return TRUE;
  }

  sprintf(buf, "Levels %d to %d", OLC_ZONE(d)->min_level, OLC_ZONE(d)->max_level);
  return TRUE;
}

/*------------------------------------------------------------------*/
/* Menu functions */
/* the main menu */
static void zedit_disp_menu(struct descriptor_data *d)
{
  int subcmd = 0, room, counter = 0;
  char buf1[MAX_STRING_LENGTH], lev_string[50];
  bool levels_set = FALSE;

  get_char_colors(d->character);
  clear_screen(d);
  room = real_room(OLC_NUM(d));

  sprintbitarray(OLC_ZONE(d)->zone_flags, zone_bits, ZN_ARRAY_MAX, buf1);
  levels_set = zedit_get_levels(d, lev_string);

  /* Menu header */
  send_to_char(d->character,
	  "Room number: %s%d%s Room zone: %s%d\r\n"
	  "%s1%s) Builders       : %s%s\r\n"
	  "%sZ%s) Zone name      : %s%s\r\n"
	  "%sL%s) Lifespan       : %s%d minutes\r\n"
	  "%sB%s) Bottom of zone : %s%d\r\n"
	  "%sT%s) Top of zone    : %s%d\r\n"
	  "%sR%s) Reset Mode     : %s%s\r\n"
	  "%sF%s) Zone Flags     : %s%s\r\n"
	  "%sM%s) Level Range    : %s%s%s\r\n"
	  "[Command list]\r\n",

	  cyn, OLC_NUM(d), nrm,
	  cyn, zone_table[OLC_ZNUM(d)].number,
	  grn, nrm, yel, OLC_ZONE(d)->builders ? OLC_ZONE(d)->builders : "None.",
	  grn, nrm, yel, OLC_ZONE(d)->name ? OLC_ZONE(d)->name : "<NONE!>",
	  grn, nrm, yel, OLC_ZONE(d)->lifespan,
	  grn, nrm, yel, OLC_ZONE(d)->bot,
	  grn, nrm, yel, OLC_ZONE(d)->top,
	  grn, nrm, yel,
	  OLC_ZONE(d)->reset_mode ? ((OLC_ZONE(d)->reset_mode == 1) ? "Reset when no players are in zone." : "Normal reset.") : "Never reset",
	  grn, nrm, cyn, buf1,
	  grn, nrm, levels_set ? cyn : yel, lev_string,
	  nrm
	  );

  /* Print the commands for this room into display buffer. */
  while (MYCMD.command != 'S') {
    /* Translate what the command means. */
    write_to_output(d, "%s%d - %s", nrm, counter++, yel);
    switch (MYCMD.command) {
    case 'M':
      write_to_output(d, "%sLoad %s [%s%d%s], Max : %d",
              MYCMD.if_flag ? " then " : "",
              mob_proto[MYCMD.arg1].player.short_descr, cyn,
              mob_index[MYCMD.arg1].vnum, yel, MYCMD.arg2
              );
      break;
    case 'G':
      write_to_output(d, "%sGive it %s [%s%d%s], Max : %d",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      MYCMD.arg2
	      );
      break;
    case 'O':
      write_to_output(d, "%sLoad %s [%s%d%s], Max : %d",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      MYCMD.arg2
	      );
      break;
    case 'E':
      write_to_output(d, "%sEquip with %s [%s%d%s], %s, Max : %d",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      equipment_types[MYCMD.arg3],
	      MYCMD.arg2
	      );
      break;
    case 'P':
      write_to_output(d, "%sPut %s [%s%d%s] in %s [%s%d%s], Max : %d",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      obj_proto[MYCMD.arg3].short_description,
	      cyn, obj_index[MYCMD.arg3].vnum, yel,
	      MYCMD.arg2
	      );
      break;
    case 'R':
      write_to_output(d, "%sRemove %s [%s%d%s] from room.",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg2].short_description,
	      cyn, obj_index[MYCMD.arg2].vnum, yel
	      );
      break;
    case 'D':
      write_to_output(d, "%sSet door %s as %s.",
	      MYCMD.if_flag ? " then " : "",
	      dirs[MYCMD.arg2],
	      MYCMD.arg3 ? ((MYCMD.arg3 == 1) ? "closed" : "locked") : "open"
	      );
      break;
    case 'T':
      write_to_output(d, "%sAttach trigger %s%s%s [%s%d%s] to %s",
        MYCMD.if_flag ? " then " : "",
        cyn, trig_index[MYCMD.arg2]->proto->name, yel,
        cyn, trig_index[MYCMD.arg2]->vnum, yel,
        ((MYCMD.arg1 == MOB_TRIGGER) ? "mobile" :
          ((MYCMD.arg1 == OBJ_TRIGGER) ? "object" :
            ((MYCMD.arg1 == WLD_TRIGGER)? "room" : "????"))));
      break;
    case 'V':
      write_to_output(d, "%sAssign global %s:%d to %s = %s",
        MYCMD.if_flag ? " then " : "",
        MYCMD.sarg1, MYCMD.arg2,
        ((MYCMD.arg1 == MOB_TRIGGER) ? "mobile" :
          ((MYCMD.arg1 == OBJ_TRIGGER) ? "object" :
            ((MYCMD.arg1 == WLD_TRIGGER)? "room" : "????"))),
        MYCMD.sarg2);
      break;
    default:
      write_to_output(d, "<Unknown Command>");
      break;
    }
    write_to_output(d, "\r\n");
    subcmd++;
  }
  /* Finish off menu */
   write_to_output(d,
	  "%s%d - <END OF LIST>\r\n"
	  "%sN%s) Insert new command.\r\n"
	  "%sE%s) Edit a command.\r\n"
	  "%sD%s) Delete a command.\r\n"
	  "%sQ%s) Quit\r\nEnter your choice : ",
	  nrm, counter, grn, nrm, grn, nrm, grn, nrm, grn, nrm
	  );

  OLC_MODE(d) = ZEDIT_MAIN_MENU;
}

/* Print the command type menu and setup response catch. */
static void zedit_disp_comtype(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d,
	"\r\n"
	"%sM%s) Load Mobile to room             %sO%s) Load Object to room\r\n"
	"%sE%s) Equip mobile with object        %sG%s) Give an object to a mobile\r\n"
	"%sP%s) Put object in another object    %sD%s) Open/Close/Lock a Door\r\n"
	"%sR%s) Remove an object from the room\r\n"
        "%sT%s) Assign a trigger                %sV%s) Set a global variable\r\n"
	"\r\n"
	"What sort of command will this be? : ",
	grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm,
	grn, nrm, grn, nrm, grn, nrm, grn, nrm
	);
  OLC_MODE(d) = ZEDIT_COMMAND_TYPE;
}

/* Print the appropriate message for the command type for arg1 and set
   up the input catch clause */
static void zedit_disp_arg1(struct descriptor_data *d)
{
  write_to_output(d, "\r\n");

  switch (OLC_CMD(d).command) {
  case 'M':
    write_to_output(d, "Input mob's vnum : ");
    OLC_MODE(d) = ZEDIT_ARG1;
    break;
  case 'O':
  case 'E':
  case 'P':
  case 'G':
    write_to_output(d, "Input object vnum : ");
    OLC_MODE(d) = ZEDIT_ARG1;
    break;
  case 'D':
  case 'R':
    /* Arg1 for these is the room number, skip to arg2 */
    OLC_CMD(d).arg1 = real_room(OLC_NUM(d));
    zedit_disp_arg2(d);
    break;
  case 'T':
  case 'V':
    write_to_output(d, "Input trigger type (0:mob, 1:obj, 2:room) :");
    OLC_MODE(d) = ZEDIT_ARG1;
    break;
  default:
    /* We should never get here. */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: zedit_disp_arg1(): Help!");
    write_to_output(d, "Oops...\r\n");
    return;
  }
}

/* Print the appropriate message for the command type for arg2 and set
   up the input catch clause. */
static void zedit_disp_arg2(struct descriptor_data *d)
{
  int i;

  write_to_output(d, "\r\n");

  switch (OLC_CMD(d).command) {
  case 'M':
  case 'O':
  case 'E':
  case 'P':
  case 'G':
    write_to_output(d, "Input the maximum number that can exist on the mud : ");
    break;
  case 'D':
    for (i = 0; *dirs[i] != '\n'; i++) {
      write_to_output(d, "%d) Exit %s.\r\n", i, dirs[i]);
    }
    write_to_output(d, "Enter exit number for door : ");
    break;
  case 'R':
    write_to_output(d, "Input object's vnum : ");
    break;
  case 'T':
    write_to_output(d, "Enter the trigger VNum : ");
    break;
  case 'V':
    write_to_output(d, "Global's context (0 for none) : ");
    break;
  default:
    /* We should never get here, but just in case.  */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: zedit_disp_arg2(): Help!");
    write_to_output(d, "Oops...\r\n");
    return;
  }
  OLC_MODE(d) = ZEDIT_ARG2;
}

/* Print the appropriate message for the command type for arg3 and set
   up the input catch clause. */
static void zedit_disp_arg3(struct descriptor_data *d)
{

  write_to_output(d, "\r\n");

  switch (OLC_CMD(d).command) {
  case 'E':
    column_list(d->character, 0, equipment_types, NUM_WEARS, TRUE);
    write_to_output(d, "Location to equip : ");
    break;
  case 'P':
    write_to_output(d, "Virtual number of the container : ");
    break;
  case 'D':
    write_to_output(d, 	"0)  Door open\r\n"
		"1)  Door closed\r\n"
		"2)  Door locked\r\n"
		"Enter state of the door : ");
    break;
  case 'V':
  case 'T':
  case 'M':
  case 'O':
  case 'R':
  case 'G':
  default:
    /* We should never get here, just in case. */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: zedit_disp_arg3(): Help!");
    write_to_output(d, "Oops...\r\n");
    return;
  }
  OLC_MODE(d) = ZEDIT_ARG3;
}

/*-------------------------------------------------------------------*/
/*
 * Print the recommended levels menu and setup response catch.
 */
void zedit_disp_levels(struct descriptor_data *d)
{
  char lev_string[50];
  bool levels_set = FALSE;

  levels_set = zedit_get_levels(d, lev_string);

  clear_screen(d);
  write_to_output(d,
	"\r\n"
	"\ty1\tn) Set minimum level recommendation\r\n"
	"\ty2\tn) Set maximum level recommendation\r\n"
	"\ty3\tn) Clear level recommendations\r\n\r\n"
	"\ty0\tn) Quit to main menu\r\n"
        "\tgCurrent Setting: %s%s\r\n"
	"\r\n"
	"Enter choice (0 to quit): ", levels_set ? "\tc" : "\ty", lev_string
	);
  OLC_MODE(d) = ZEDIT_LEVELS;
}

/* The event handler */
void zedit_parse(struct descriptor_data *d, char *arg)
{
  int pos, number;

  switch (OLC_MODE(d)) {
  case ZEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      /* Save the zone in memory, hiding invisible people. */
      zedit_save_internally(d);
      if (CONFIG_OLC_SAVE) {
        write_to_output(d, "Saving zone info to disk.\r\n");
        zedit_save_to_disk(OLC_ZNUM(d));
      } else
        write_to_output(d, "Saving zone info in memory.\r\n");

      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE, "OLC: %s edits zone info for room %d.", GET_NAME(d->character), OLC_NUM(d));
      /* FALL THROUGH */
    case 'n':
    case 'N':
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      write_to_output(d, "Invalid choice!\r\n");
      write_to_output(d, "Do you wish to save your changes? : ");
      break;
    }
    break;
   /* End of ZEDIT_CONFIRM_SAVESTRING */

  case ZEDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_ZONE(d)->age || OLC_ZONE(d)->number) {
	write_to_output(d, "Do you wish to save your changes? : ");
	OLC_MODE(d) = ZEDIT_CONFIRM_SAVESTRING;
      } else {
	write_to_output(d, "No changes made.\r\n");
	cleanup_olc(d, CLEANUP_ALL);
      }
      break;
    case 'n':
    case 'N':
      /* New entry. */
      if (OLC_ZONE(d)->cmd[0].command == 'S') {
        /* first command */
        if (new_command(OLC_ZONE(d), 0) && start_change_command(d, 0)) {
	  zedit_disp_comtype(d);
	  OLC_ZONE(d)->age = 1;
          break;
	}
      }
      write_to_output(d, "What number in the list should the new command be? : ");
      OLC_MODE(d) = ZEDIT_NEW_ENTRY;
      break;
    case 'e':
    case 'E':
      /* Change an entry. */
      write_to_output(d, "Which command do you wish to change? : ");
      OLC_MODE(d) = ZEDIT_CHANGE_ENTRY;
      break;
    case 'd':
    case 'D':
      /* Delete an entry. */
      write_to_output(d, "Which command do you wish to delete? : ");
      OLC_MODE(d) = ZEDIT_DELETE_ENTRY;
      break;
    case 'z':
    case 'Z':
      /* Edit zone name. */
      write_to_output(d, "Enter new zone name : ");
      OLC_MODE(d) = ZEDIT_ZONE_NAME;
      break;
    case '1':
      /* Edit zone builders. */
      write_to_output(d, "Enter new builders list : ");
      OLC_MODE(d) = ZEDIT_ZONE_BUILDERS;
      break;
    case 'b':
    case 'B':
      /* Edit bottom of zone. */
      if (GET_LEVEL(d->character) < LVL_IMPL)
	zedit_disp_menu(d);
      else {
	write_to_output(d, "Enter new bottom of zone : ");
	OLC_MODE(d) = ZEDIT_ZONE_BOT;
      }
      break;
    case 't':
    case 'T':
      /* Edit top of zone. */
      if (GET_LEVEL(d->character) < LVL_IMPL)
	zedit_disp_menu(d);
      else {
	write_to_output(d, "Enter new top of zone : ");
	OLC_MODE(d) = ZEDIT_ZONE_TOP;
      }
      break;
    case 'l':
    case 'L':
      /* Edit zone lifespan. */
      write_to_output(d, "Enter new zone lifespan : ");
      OLC_MODE(d) = ZEDIT_ZONE_LIFE;
      break;
    case 'r':
    case 'R':
      /* Edit zone reset mode. */
      write_to_output(d, "\r\n"
		"0) Never reset\r\n"
		"1) Reset only when no players in zone\r\n"
		"2) Normal reset\r\n"
		"Enter new zone reset type : ");
      OLC_MODE(d) = ZEDIT_ZONE_RESET;
      break;
    case 'f':
    case 'F':
      zedit_disp_flag_menu(d);
      break;
    case 'm':
    case 'M':
      /*** Edit zone level restrictions (sub-menu) ***/
      zedit_disp_levels(d);
      break;
    default:
      zedit_disp_menu(d);
      break;
    }
    break;
    /* End of ZEDIT_MAIN_MENU */

/*-------------------------------------------------------------------*/
  case ZEDIT_LEVELS:
    switch (*arg) {
    case '1': write_to_output(d, "Enter the min level for this zone (0-%d, -1 = none): ", (LVL_IMMORT-1));
              OLC_MODE(d) = ZEDIT_LEV_MIN;
              break;

    case '2': write_to_output(d, "Enter the max level for this zone (0-%d, -1 = none): ", (LVL_IMMORT-1));
              OLC_MODE(d) = ZEDIT_LEV_MAX;
              break;

    case '3': OLC_ZONE(d)->min_level = -1;
              OLC_ZONE(d)->max_level = -1;
              OLC_ZONE(d)->number = 1;
              zedit_disp_menu(d);
              break;

    case '0':
      zedit_disp_menu(d);
      break;

    default: write_to_output(d, "Invalid choice!\r\n");
             break;
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_LEV_MIN:
    pos = atoi(arg);
    OLC_ZONE(d)->min_level = MIN(MAX(pos,-1), 100);
    OLC_ZONE(d)->number = 1;
    zedit_disp_levels(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_LEV_MAX:
    pos = atoi(arg);
    OLC_ZONE(d)->max_level = MIN(MAX(pos,-1), 100);
    OLC_ZONE(d)->number = 1;
    zedit_disp_levels(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_NEW_ENTRY:
    /* Get the line number and insert the new line. */
    pos = atoi(arg);
    if (isdigit(*arg) && new_command(OLC_ZONE(d), pos)) {
      if (start_change_command(d, pos)) {
	zedit_disp_comtype(d);
	OLC_ZONE(d)->age = 1;
      }
    } else
      zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_DELETE_ENTRY:
    /* Get the line number and delete the line. */
    pos = atoi(arg);
    if (isdigit(*arg)) {
      delete_zone_command(OLC_ZONE(d), pos);
      OLC_ZONE(d)->age = 1;
    }
    zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_CHANGE_ENTRY:
    /* Parse the input for which line to edit, and goto next quiz. Abort edit,
       and return to main menu idea from Mark Garringer zizazat@hotmail.com */
    if (toupper(*arg) == 'A') {
      if (OLC_CMD(d).command == 'N') {
        OLC_CMD(d).command = '*';
      }
      zedit_disp_menu(d);
      break;
    }

    pos = atoi(arg);
    if (isdigit(*arg) && start_change_command(d, pos)) {
      zedit_disp_comtype(d);
      OLC_ZONE(d)->age = 1;
    } else
      zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_COMMAND_TYPE:
    /* Parse the input for which type of command this is, and goto next quiz. */
    OLC_CMD(d).command = toupper(*arg);
    if (!OLC_CMD(d).command || (strchr("MOPEDGRTV", OLC_CMD(d).command) == NULL)) {
      write_to_output(d, "Invalid choice, try again : ");
    } else {
      if (OLC_VAL(d)) {	/* If there was a previous command. */
        if (OLC_CMD(d).command == 'T' || OLC_CMD(d).command == 'V') {
          OLC_CMD(d).if_flag = 1;
          zedit_disp_arg1(d);
        } else {
	  write_to_output(d, "Is this command dependent on the success of the previous one? (y/n)\r\n");
	  OLC_MODE(d) = ZEDIT_IF_FLAG;
        }
      } else {	/* 'if-flag' not appropriate. */
	OLC_CMD(d).if_flag = 0;
	zedit_disp_arg1(d);
      }
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_IF_FLAG:
    /* Parse the input for the if flag, and goto next quiz. */
    switch (*arg) {
    case 'y':
    case 'Y':
      OLC_CMD(d).if_flag = 1;
      break;
    case 'n':
    case 'N':
      OLC_CMD(d).if_flag = 0;
      break;
    default:
      write_to_output(d, "Try again : ");
      return;
    }
    zedit_disp_arg1(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ARG1:
    /* Parse the input for arg1, and goto next quiz. */
    if (!isdigit(*arg)) {
      write_to_output(d, "Must be a numeric value, try again : ");
      return;
    }
    switch (OLC_CMD(d).command) {
    case 'M':
      if ((pos = real_mobile(atoi(arg))) != NOBODY) {
	OLC_CMD(d).arg1 = pos;
	zedit_disp_arg2(d);
      } else
	write_to_output(d, "That mobile does not exist, try again : ");
      break;
    case 'O':
    case 'P':
    case 'E':
    case 'G':
      if ((pos = real_object(atoi(arg))) != NOTHING) {
	OLC_CMD(d).arg1 = pos;
	zedit_disp_arg2(d);
      } else
	write_to_output(d, "That object does not exist, try again : ");
      break;
    case 'T':
    case 'V':
      if (atoi(arg)<MOB_TRIGGER || atoi(arg)>WLD_TRIGGER)
        write_to_output(d, "Invalid input.");
      else {
       OLC_CMD(d).arg1 = atoi(arg);
        zedit_disp_arg2(d);
      }
      break;
    case 'D':
    case 'R':
    default:
      /* We should never get here. */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: zedit_parse(): case ARG1: Ack!");
      write_to_output(d, "Oops...\r\n");
      break;
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ARG2:
    /* Parse the input for arg2, and goto next quiz. */
    if (!isdigit(*arg)) {
      write_to_output(d, "Must be a numeric value, try again : ");
      return;
    }
    switch (OLC_CMD(d).command) {
    case 'M':
    case 'O':
      OLC_CMD(d).arg2 = MIN(MAX_DUPLICATES, atoi(arg));
      OLC_CMD(d).arg3 = real_room(OLC_NUM(d));
      zedit_disp_menu(d);
      break;
    case 'G':
      OLC_CMD(d).arg2 = MIN(MAX_DUPLICATES, atoi(arg));
      zedit_disp_menu(d);
      break;
    case 'P':
    case 'E':
      OLC_CMD(d).arg2 = MIN(MAX_DUPLICATES, atoi(arg));
      zedit_disp_arg3(d);
      break;
    case 'V':
      OLC_CMD(d).arg2 = atoi(arg); /* context */
      OLC_CMD(d).arg3 = real_room(OLC_NUM(d));
      write_to_output(d, "Enter the global name : ");
      OLC_MODE(d) = ZEDIT_SARG1;
      break;
    case 'T':
      if (real_trigger(atoi(arg)) != NOTHING) {
        OLC_CMD(d).arg2 = real_trigger(atoi(arg)); /* trigger */
        OLC_CMD(d).arg3 = real_room(OLC_NUM(d));
        zedit_disp_menu(d);
      } else
        write_to_output(d, "That trigger does not exist, try again : ");
      break;
    case 'D':
      pos = atoi(arg);
      /* Count directions. */
      if (pos < 0 || pos > DIR_COUNT)
	write_to_output(d, "Try again : ");
      else {
	OLC_CMD(d).arg2 = pos;
	zedit_disp_arg3(d);
      }
      break;
    case 'R':
      if ((pos = real_object(atoi(arg))) != NOTHING) {
	OLC_CMD(d).arg2 = pos;
	zedit_disp_menu(d);
      } else
	write_to_output(d, "That object does not exist, try again : ");
      break;
    default:
      /* We should never get here, but just in case. */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: zedit_parse(): case ARG2: Ack!");
      write_to_output(d, "Oops...\r\n");
      break;
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ARG3:
    /* Parse the input for arg3, and go back to main menu. */
    if (!isdigit(*arg)) {
      write_to_output(d, "Must be a numeric value, try again : ");
      return;
    }
    switch (OLC_CMD(d).command) {
    case 'E':
      pos = atoi(arg) - 1;
      /* Count number of wear positions. */
      if (pos < 0 || pos >= NUM_WEARS)
	write_to_output(d, "Try again : ");
      else {
	OLC_CMD(d).arg3 = pos;
	zedit_disp_menu(d);
      }
      break;
    case 'P':
      if ((pos = real_object(atoi(arg))) != NOTHING) {
	OLC_CMD(d).arg3 = pos;
	zedit_disp_menu(d);
      } else
	write_to_output(d, "That object does not exist, try again : ");
      break;
    case 'D':
      pos = atoi(arg);
      if (pos < 0 || pos > 2)
	write_to_output(d, "Try again : ");
      else {
	OLC_CMD(d).arg3 = pos;
	zedit_disp_menu(d);
      }
      break;
    case 'M':
    case 'O':
    case 'G':
    case 'R':
    case 'T':
    case 'V':
    default:
      /* We should never get here, but just in case. */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: zedit_parse(): case ARG3: Ack!");
      write_to_output(d, "Oops...\r\n");
      break;
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_SARG1:
    if (strlen(arg)) {
      OLC_CMD(d).sarg1 = strdup(arg);
      OLC_MODE(d) = ZEDIT_SARG2;
      write_to_output(d, "Enter the global value : ");
    } else
      write_to_output(d, "Must have some name to assign : ");
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_SARG2:
    if (strlen(arg)) {
      OLC_CMD(d).sarg2 = strdup(arg);
      zedit_disp_menu(d);
    } else
      write_to_output(d, "Must have some value to set it to :");
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_NAME:
    /* Add new name and return to main menu. */
    if (genolc_checkstring(d, arg)) {
      if (OLC_ZONE(d)->name)
        free(OLC_ZONE(d)->name);
      else
        log("SYSERR: OLC: ZEDIT_ZONE_NAME: no name to free!");
      OLC_ZONE(d)->name = strdup(arg);
      OLC_ZONE(d)->number = 1;
    }
    zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_BUILDERS:
    /* Add new builders list and return to main menu. */
    if (genolc_checkstring(d, arg)) {
      if (OLC_ZONE(d)->builders)
        free(OLC_ZONE(d)->builders);
      else
        log("SYSERR: OLC: ZEDIT_ZONE_BUILDERS: no builders list to free!");
      OLC_ZONE(d)->builders = strdup(arg);
      OLC_ZONE(d)->number = 1;
    }
    zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_RESET:
    /* Parse and add new reset_mode and return to main menu. */
    pos = atoi(arg);
    if (!isdigit(*arg) || pos < 0 || pos > 2)
      write_to_output(d, "Try again (0-2) : ");
    else {
      OLC_ZONE(d)->reset_mode = pos;
      OLC_ZONE(d)->number = 1;
      zedit_disp_menu(d);
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_LIFE:
    /* Parse and add new lifespan and return to main menu. */
    pos = atoi(arg);
    if (!isdigit(*arg) || pos < 0 || pos > 240)
      write_to_output(d, "Try again (0-240) : ");
    else {
      OLC_ZONE(d)->lifespan = pos;
      OLC_ZONE(d)->number = 1;
      zedit_disp_menu(d);
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_FLAGS:
    number = atoi(arg);
    if (number < 0 || number > NUM_ZONE_FLAGS) {
      write_to_output(d, "That is not a valid choice!\r\n");
      zedit_disp_flag_menu(d);
    } else if (number == 0) {
      zedit_disp_menu(d);
      break;
      }
    else {
      /*
       * Toggle the bit.
       */
      TOGGLE_BIT_AR(OLC_ZONE(d)->zone_flags, number - 1);
      OLC_ZONE(d)->number = 1;
      zedit_disp_flag_menu(d);
    }
    return;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_BOT:
    /* Parse and add new bottom room in zone and return to main menu. */
    if (OLC_ZNUM(d) == 0)
      OLC_ZONE(d)->bot = LIMIT(atoi(arg), 0, OLC_ZONE(d)->top);
    else
      OLC_ZONE(d)->bot = LIMIT(atoi(arg), zone_table[OLC_ZNUM(d) - 1].top + 1, OLC_ZONE(d)->top);
    OLC_ZONE(d)->number = 1;
    zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_TOP:
    /* Parse and add new top room in zone and return to main menu. */
    if (OLC_ZNUM(d) == top_of_zone_table)
      OLC_ZONE(d)->top = LIMIT(atoi(arg), genolc_zonep_bottom(OLC_ZONE(d)), 32000);
    else
      OLC_ZONE(d)->top = LIMIT(atoi(arg), genolc_zonep_bottom(OLC_ZONE(d)), genolc_zone_bottom(OLC_ZNUM(d) + 1) - 1);
    OLC_ZONE(d)->number = 1;
    zedit_disp_menu(d);
    break;

  default:
    /* We should never get here, but just in case... */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: zedit_parse(): Reached default case!");
    write_to_output(d, "Oops...\r\n");
    break;
  }
}
