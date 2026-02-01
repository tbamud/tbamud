/**************************************************************************
*  File: oedit.c                                           Part of tbaMUD *
*  Usage: Oasis OLC - Objects.                                            *
*                                                                         *
* By Levork. Copyright 1996 Harvey Gilpin. 1997-2001 George Greer.        *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "spells.h"
#include "db.h"
#include "boards.h"
#include "constants.h"
#include "shop.h"
#include "genolc.h"
#include "genobj.h"
#include "genzon.h"
#include "oasis.h"
#include "improved-edit.h"
#include "py_olc.h"
#include "fight.h"
#include "modify.h"

/* local functions */
static void oedit_setup_new(struct descriptor_data *d);
static void oedit_disp_container_flags_menu(struct descriptor_data *d);
static void oedit_disp_extradesc_menu(struct descriptor_data *d);
static void oedit_disp_prompt_apply_menu(struct descriptor_data *d);
static void oedit_liquid_type(struct descriptor_data *d);
static void oedit_disp_apply_menu(struct descriptor_data *d);
static void oedit_disp_weapon_menu(struct descriptor_data *d);
static void oedit_disp_spells_menu(struct descriptor_data *d);
static void oedit_disp_type_menu(struct descriptor_data *d);
static void oedit_disp_extra_menu(struct descriptor_data *d);
static void oedit_disp_wear_menu(struct descriptor_data *d);
static void oedit_disp_menu(struct descriptor_data *d);
static void oedit_disp_perm_menu(struct descriptor_data *d);
static void oedit_save_to_disk(int zone_num);

/* handy macro */
#define S_PRODUCT(s, i) ((s)->producing[(i)])

static const char * const *get_val_labels(struct obj_data *obj)
{
  return obj_value_labels(GET_OBJ_TYPE(obj));
}

/* Utility and exported functions */
ACMD(do_oasis_oedit)
{
  int number = NOWHERE, save = 0, real_num;
  struct descriptor_data *d;
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  /* No building as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

  /* Parse any arguments. */
  two_arguments(argument, buf1, buf2);

  /* If there aren't any arguments they can't modify anything. */
  if (!*buf1) {
    send_to_char(ch, "Specify an object VNUM to edit.\r\n");
    return;
  } else if (!isdigit(*buf1)) {
    if (str_cmp("save", buf1) != 0) {
      send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }

    save = TRUE;

    if (is_number(buf2))
      number = atoi(buf2);
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
  }

  /* If a numeric argument was given, get it. */
  if (number == NOWHERE)
    number = atoi(buf1);

  if (number < IDXTYPE_MIN || number > IDXTYPE_MAX) {
    send_to_char(ch, "That object VNUM can't exist.\r\n");
    return;
  }

  /* Check that whatever it is isn't already being edited. */
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_OEDIT) {
      if (d->olc && OLC_NUM(d) == number) {
        send_to_char(ch, "That object is currently being edited by %s.\r\n",
          PERS(d->character, ch));
        return;
      }
    }
  }

  /* Point d to the builder's descriptor (for easier typing later). */
  d = ch->desc;

  /* Give the descriptor an OLC structure. */
  if (d->olc) {
    mudlog(BRF, LVL_IMMORT, TRUE,
      "SYSERR: do_oasis: Player already had olc structure.");
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

  /* If we need to save, save the objects. */
  if (save) {
    send_to_char(ch, "Saving all objects in zone %d.\r\n",
      zone_table[OLC_ZNUM(d)].number);
    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE,
      "OLC: %s saves object info for zone %d.", GET_NAME(ch),
      zone_table[OLC_ZNUM(d)].number);

    /* Save the objects in this zone. */
    save_objects(OLC_ZNUM(d));

    /* Free the descriptor's OLC structure. */
    free(d->olc);
    d->olc = NULL;
    return;
  }

  OLC_NUM(d) = number;

  /* If a new object, setup new, otherwise setup the existing object. */
  if ((real_num = real_object(number)) != NOTHING)
    oedit_setup_existing(d, real_num);
  else
    oedit_setup_new(d);

  oedit_disp_menu(d);
  STATE(d) = CON_OEDIT;

  /* Send the OLC message to the players in the same room as the builder. */
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  /* Log the OLC message. */
  mudlog(CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "OLC: %s starts editing zone %d allowed zone %d",
    GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}

static void oedit_setup_new(struct descriptor_data *d)
{
  CREATE(OLC_OBJ(d), struct obj_data, 1);

  clear_object(OLC_OBJ(d));
  OLC_OBJ(d)->name = strdup("unfinished object");
  OLC_OBJ(d)->description = strdup("An unfinished object is lying here.");
  OLC_OBJ(d)->short_description = strdup("an unfinished object");
  SET_BIT_AR(GET_OBJ_WEAR(OLC_OBJ(d)), ITEM_WEAR_TAKE);
  OLC_VAL(d) = 0;
  OLC_DIRTY(d) = 0;
  OLC_VAL_SLOT(d) = -1;
  OLC_ITEM_TYPE(d) = OBJ_TRIGGER;

  SCRIPT(OLC_OBJ(d)) = NULL;
  OLC_OBJ(d)->proto_script = OLC_SCRIPT(d) = NULL;
}

void oedit_setup_existing(struct descriptor_data *d, int real_num)
{
  struct obj_data *obj;

  /* Allocate object in memory. */
  CREATE(obj, struct obj_data, 1);
  copy_object(obj, &obj_proto[real_num]);

  /* Attach new object to player's descriptor. */
  OLC_OBJ(d) = obj;
  OLC_VAL(d) = 0;
  OLC_DIRTY(d) = 0;
  OLC_VAL_SLOT(d) = -1;
  OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
  dg_olc_script_copy(d);
  /* The edited obj must not have a script. It will be assigned to the updated
   * obj later, after editing. */
  SCRIPT(obj) = NULL;
  OLC_OBJ(d)->proto_script = NULL;
}

void oedit_save_internally(struct descriptor_data *d)
{
  int i;
  obj_rnum robj_num;
  struct descriptor_data *dsc;
  struct obj_data *obj;

  i = (real_object(OLC_NUM(d)) == NOTHING);

  if ((robj_num = add_object(OLC_OBJ(d), OLC_NUM(d))) == NOTHING) {
    log("oedit_save_internally: add_object failed.");
    return;
  }

  /* Update triggers and free old proto list  */
  if (obj_proto[robj_num].proto_script &&
      obj_proto[robj_num].proto_script != OLC_SCRIPT(d))
    free_proto_script(&obj_proto[robj_num], OBJ_TRIGGER);
  /* this will handle new instances of the object: */
  obj_proto[robj_num].proto_script = OLC_SCRIPT(d);

  /* this takes care of the objects currently in-game */
  for (obj = object_list; obj; obj = obj->next) {
    if (obj->item_number != robj_num)
      continue;
    /* remove any old scripts */
    if (SCRIPT(obj))
      extract_script(obj, OBJ_TRIGGER);

    free_proto_script(obj, OBJ_TRIGGER);
    copy_proto_script(&obj_proto[robj_num], obj, OBJ_TRIGGER);
    assign_triggers(obj, OBJ_TRIGGER);
  }
  /* end trigger update */

  if (!i)	/* If it's not a new object, don't renumber. */
    return;

  /* Renumber produce in shops being edited. */
  for (dsc = descriptor_list; dsc; dsc = dsc->next)
    if (STATE(dsc) == CON_SEDIT)
      for (i = 0; S_PRODUCT(OLC_SHOP(dsc), i) != NOTHING; i++)
	if (S_PRODUCT(OLC_SHOP(dsc), i) >= robj_num)
	  S_PRODUCT(OLC_SHOP(dsc), i)++;


  /* Update other people in zedit too. From: C.Raehl 4/27/99 */
  for (dsc = descriptor_list; dsc; dsc = dsc->next)
    if (STATE(dsc) == CON_ZEDIT)
      for (i = 0; OLC_ZONE(dsc)->cmd[i].command != 'S'; i++)
        switch (OLC_ZONE(dsc)->cmd[i].command) {
          case 'P':
            OLC_ZONE(dsc)->cmd[i].arg3 += (OLC_ZONE(dsc)->cmd[i].arg3 >= robj_num);
            /* Fall through. */
          case 'E':
          case 'G':
          case 'O':
            OLC_ZONE(dsc)->cmd[i].arg1 += (OLC_ZONE(dsc)->cmd[i].arg1 >= robj_num);
            break;
          case 'R':
            OLC_ZONE(dsc)->cmd[i].arg2 += (OLC_ZONE(dsc)->cmd[i].arg2 >= robj_num);
            break;
          default:
          break;
        }
}

static void oedit_save_to_disk(int zone_num)
{
  save_objects(zone_num);
}

/* Menu functions */
/* For container flags. */
static void oedit_disp_container_flags_menu(struct descriptor_data *d)
{
  int i, columns = 0;

  clear_screen(d);
  write_to_output(d, "-- Container Flags Menu --\r\n");

  for (i = 0; i < NUM_CONTAINER_FLAGS; i++) {
    write_to_output(d, "%2d) %-15s%s", i, container_bits[i],
      (++columns % 3 ? "" : "\r\n"));
  }

  if (columns % 3)
    write_to_output(d, "\r\n");

  write_to_output(d, "Enter container flag number (toggles on/off): ");
}

/* For extra descriptions. */
static void oedit_disp_extradesc_menu(struct descriptor_data *d)
{
  struct extra_descr_data *extra_desc = OLC_DESC(d);

  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d,
	  "Extra desc menu\r\n"
	  "%s1%s) Keywords: %s%s\r\n"
	  "%s2%s) Description:\r\n%s%s\r\n"
	  "%s3%s) Goto next description: %s\r\n"
	  "%s0%s) Quit\r\n"
	  "Enter choice : ",

     	  grn, nrm, yel, (extra_desc->keyword && *extra_desc->keyword) ? extra_desc->keyword : "<NONE>",
	  grn, nrm, yel, (extra_desc->description && *extra_desc->description) ? extra_desc->description : "<NONE>",
	  grn, nrm, !extra_desc->next ? "Not set." : "Set.", grn, nrm);
  OLC_MODE(d) = OEDIT_EXTRADESC_MENU;
}

/* Ask for *which* apply to edit. */
static void oedit_disp_prompt_apply_menu(struct descriptor_data *d)
{
  char apply_buf[MAX_STRING_LENGTH];
  int counter;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < MAX_OBJ_AFFECT; counter++) {
    if (OLC_OBJ(d)->affected[counter].modifier) {
      sprinttype(OLC_OBJ(d)->affected[counter].location, apply_types, apply_buf, sizeof(apply_buf));
      write_to_output(d, " %s%d%s) %+d to %s\r\n", grn, counter + 1, nrm,
	      OLC_OBJ(d)->affected[counter].modifier, apply_buf);
    } else {
      write_to_output(d, " %s%d%s) None.\r\n", grn, counter + 1, nrm);
    }
  }
  write_to_output(d, "\r\nEnter affection to modify (0 to quit) : ");
  OLC_MODE(d) = OEDIT_PROMPT_APPLY;
}

/* Ask for liquid type. */
static void oedit_liquid_type(struct descriptor_data *d)
{
  int i, columns = 0;

  clear_screen(d);
  write_to_output(d, "-- Liquid Types Menu --\r\n");

  for (i = 0; i < NUM_LIQ_TYPES; i++) {
    write_to_output(d, "%2d) %-10s%s", i, drinks[i],
      (++columns % 4 ? "" : "\r\n"));
  }

  if (columns % 4)
    write_to_output(d, "\r\n");

  write_to_output(d, "Enter liquid type number : ");
}

/* The actual apply to set. */
static void oedit_disp_apply_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  column_list(d->character, 0, apply_types, NUM_APPLIES, TRUE);
  write_to_output(d, "\r\nEnter apply type (0 is no apply) : ");
  OLC_MODE(d) = OEDIT_APPLY;
}

/* Weapon type. */
static void oedit_disp_weapon_menu(struct descriptor_data *d)
{
  int i;

  clear_screen(d);
  write_to_output(d, "-- Weapon Types Menu --\r\n");

  for (i = 0; i < NUM_ATTACK_TYPES; i++) {
    write_to_output(d, "%2d) %s\r\n", i, attack_hit_text[i].singular);
  }

  write_to_output(d, "Enter weapon type number : ");
}

/* Spell type. */
static void oedit_disp_spells_menu(struct descriptor_data *d)
{
  int i, columns = 0;

  clear_screen(d);
  write_to_output(d, "-- Spells Menu --\r\n");

  for (i = 0; i < NUM_SPELLS; i++) {
    if (*spell_info[i].name != '!')
      write_to_output(d, "%3d) %-20s%s", i, spell_info[i].name,
        (++columns % 3 ? "" : "\r\n"));
  }

  if (columns % 3)
    write_to_output(d, "\r\n");

  write_to_output(d, "Enter spell number (0 = none): ");
}

/* Display all object values dynamically, with labels for known slots */
/* Display all object values dynamically with labels based on type */
static void oedit_disp_values_menu(struct descriptor_data *d)
{
  int i;
  struct obj_data *obj = OLC_OBJ(d);
  const char * const *labels = get_val_labels(obj);

  write_to_output(d, "\r\n-- Object Values Menu --\r\n");

  for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
    /* Hide current_occupants field for furniture - it's managed by the game engine */
    if (GET_OBJ_TYPE(obj) == ITEM_FURNITURE && i == 1) {
      write_to_output(d, "%2d) %-12s : %d (managed by game engine)\r\n",
          i+1, labels[i], GET_OBJ_VAL(obj, i));
    } else {
      write_to_output(d, "%2d) %-12s : %d\r\n",
          i+1, labels[i], GET_OBJ_VAL(obj, i));
    }
  }

  write_to_output(d, "Q) Quit to main menu\r\nEnter choice : ");

  OLC_MODE(d) = OEDIT_VALUES_MENU;
}

/* Object type. */
static void oedit_disp_type_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_ITEM_TYPES; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
		item_types[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter object type : ");
}

/* Object extra flags. */
static void oedit_disp_extra_menu(struct descriptor_data *d)
{
  char buf[MAX_STRING_LENGTH];
  int i, columns = 0;

  clear_screen(d);
  write_to_output(d, "-- Extra Flags Menu --\r\n");

  for (i = 0; i < NUM_EXTRA_FLAGS; i++) {
    /* Menu is 1-based for builders */
    write_to_output(d, "%2d) %-20s%s", i + 1, extra_bits[i],
                    (++columns % 2 ? "" : "\r\n"));
  }
  if (columns % 2)
    write_to_output(d, "\r\n");

  /* Show current flags nicely */
  sprintbitarray(GET_OBJ_EXTRA(OLC_OBJ(d)), extra_bits, EF_ARRAY_MAX, buf);
  write_to_output(d, "\r\nObject flags: %s\r\n", buf);

  write_to_output(d, "Enter object extra flag (0 to quit) : ");
}

/* Object perm flags. */
static void oedit_disp_perm_menu(struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH];
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 1; counter < NUM_AFF_FLAGS; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm, affected_bits[counter], !(++columns % 2) ? "\r\n" : "");
  }
  sprintbitarray(GET_OBJ_AFFECT(OLC_OBJ(d)), affected_bits, EF_ARRAY_MAX, bits);
  write_to_output(d, "\r\nObject permanent flags: %s%s%s\r\n"
          "Enter object perm flag (0 to quit) : ", cyn, bits, nrm);
}

/* Object wear flags. */
static void oedit_disp_wear_menu(struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH];
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_ITEM_WEARS; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
		wear_bits[counter], !(++columns % 2) ? "\r\n" : "");
  }
  sprintbitarray(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits, TW_ARRAY_MAX, bits);
  write_to_output(d, "\r\nWear flags: %s%s%s\r\n"
	  "Enter wear flag, 0 to quit : ", cyn, bits, nrm);
}

/* Display main menu. */
static void oedit_disp_menu(struct descriptor_data *d)
{
  struct obj_data *obj;
  char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  obj = OLC_OBJ(d);

  get_char_colors(d->character);
  clear_screen(d);

  /* Build buffers for type/flags */
  sprinttype(GET_OBJ_TYPE(obj), item_types, buf1, sizeof(buf1));
  sprintbitarray(GET_OBJ_EXTRA(obj), extra_bits, EF_ARRAY_MAX, buf2);

  /* First part of menu */
  write_to_output(d,
    "-- Item Number : [%s%d%s]\r\n"
    "%s1%s) Keywords : %s%s\r\n"
    "%s2%s) S-Desc   : %s%s\r\n"
    "%s3%s) L-Desc   :-\r\n%s%s\r\n"
    "%s4%s) M-Desc   :-\r\n%s%s"
    "%s5%s) Type        : %s%s\r\n"
    "%s6%s) Extra flags : %s%s\r\n",

    cyn, OLC_NUM(d), nrm,
    grn, nrm, yel, (obj->name && *obj->name) ? obj->name : "undefined",
    grn, nrm, yel, (obj->short_description && *obj->short_description) ? obj->short_description : "undefined",
    grn, nrm, yel, (obj->description && *obj->description) ? obj->description : "undefined",
    grn, nrm, yel, (obj->main_description && *obj->main_description) ? obj->main_description : "Not Set.\r\n",
    grn, nrm, cyn, buf1,
    grn, nrm, cyn, buf2
  );

  /* Wear flags and affects */
  sprintbitarray(GET_OBJ_WEAR(obj), wear_bits, TW_ARRAY_MAX, buf1);
  sprintbitarray(GET_OBJ_AFFECT(obj), affected_bits, AF_ARRAY_MAX, buf2);

  /* Second half of menu */
  write_to_output(d,
    "%s7%s) Wear flags  : %s%s\r\n"
    "%s8%s) Weight      : %s%d\r\n"
    "%s9%s) Cost        : %s%d\r\n"
    "%sA%s) Timer       : %s%d\r\n"
    "%sM%s) Min Level   : %s%d\r\n"
    "%sP%s) Perm Affects: %s%s\r\n"
    "%sS%s) Script      : %s%s\r\n"
    "%sW%s) Copy object\r\n"
    "%sX%s) Delete object\r\n",

    grn, nrm, cyn, buf1,
    grn, nrm, cyn, GET_OBJ_WEIGHT(obj),
    grn, nrm, cyn, GET_OBJ_COST(obj),
    grn, nrm, cyn, GET_OBJ_TIMER(obj),
    grn, nrm, cyn, GET_OBJ_LEVEL(obj),
    grn, nrm, cyn, buf2,
    grn, nrm, cyn, OLC_SCRIPT(d) ? "Set." : "Not Set.",
    grn, nrm,
    grn, nrm
  );

  /* Unified values menu */
  write_to_output(d,
    "%sV%s) Edit object values (%d slots total)\r\n",
    grn, nrm, NUM_OBJ_VAL_POSITIONS);

  /* Quit */
  write_to_output(d,
    "%sQ%s) Quit\r\n"
    "Enter choice : ",
    grn, nrm);

  OLC_MODE(d) = OEDIT_MAIN_MENU;
}

/* main loop (of sorts).. basically interpreter throws all input to here. */
void oedit_parse(struct descriptor_data *d, char *arg)
{
  int number;
  char *oldtext = NULL;

  switch (OLC_MODE(d)) {

  case OEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      oedit_save_internally(d);
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE,
             "OLC: %s edits obj %d", GET_NAME(d->character), OLC_NUM(d));
      if (CONFIG_OLC_SAVE) {
        oedit_save_to_disk(real_zone_by_thing(OLC_NUM(d)));
        write_to_output(d, "Object saved to disk.\r\n");
      } else
        write_to_output(d, "Object saved to memory.\r\n");
      cleanup_olc(d, CLEANUP_ALL);
      return;
    case 'n':
    case 'N':
      OLC_OBJ(d)->proto_script = OLC_SCRIPT(d);
      free_proto_script(OLC_OBJ(d), OBJ_TRIGGER);
      cleanup_olc(d, CLEANUP_ALL);
      return;
    case 'a':
    case 'A':
      oedit_disp_menu(d);
      return;
    default:
      write_to_output(d, "Invalid choice!\r\n");
      write_to_output(d, "Do you wish to save your changes? : \r\n");
      return;
    }

  case OEDIT_MAIN_MENU:
    switch (*arg) {
    case 'q': case 'Q':
      if (OLC_DIRTY(d)) {
        write_to_output(d, "Do you wish to save your changes? : ");
        OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING;
      } else
        cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      write_to_output(d, "Enter keywords : ");
      OLC_MODE(d) = OEDIT_KEYWORD;
      break;
    case '2':
      write_to_output(d, "Enter short desc : ");
      OLC_MODE(d) = OEDIT_SHORTDESC;
      break;
    case '3':
      write_to_output(d, "Enter long desc :-\r\n| ");
      OLC_MODE(d) = OEDIT_LONGDESC;
      break;
    case '4':
      OLC_MODE(d) = OEDIT_MAINDESC;
      send_editor_help(d);
      write_to_output(d, "Enter main description:\r\n\r\n");
      if (OLC_OBJ(d)->main_description) {
        write_to_output(d, "%s", OLC_OBJ(d)->main_description);
        oldtext = strdup(OLC_OBJ(d)->main_description);
      }
      string_write(d, &OLC_OBJ(d)->main_description,
                   MAX_MESSAGE_LENGTH, 0, oldtext);
      OLC_DIRTY(d) = 1;
      break;
    case '5':
      oedit_disp_type_menu(d);
      OLC_MODE(d) = OEDIT_TYPE;
      break;
    case '6':
      oedit_disp_extra_menu(d);
      OLC_MODE(d) = OEDIT_EXTRAS;
      break;
    case '7':
      oedit_disp_wear_menu(d);
      OLC_MODE(d) = OEDIT_WEAR;
      break;
    case '8':
      write_to_output(d, "Enter weight : ");
      OLC_MODE(d) = OEDIT_WEIGHT;
      break;
    case '9':
      write_to_output(d, "Enter cost : ");
      OLC_MODE(d) = OEDIT_COST;
      break;
    case 'a': case 'A':
      write_to_output(d, "Enter timer : ");
      OLC_MODE(d) = OEDIT_TIMER;
      break;
    case 'd': case 'D':
      oedit_disp_prompt_apply_menu(d);
      break;
    case 'e': case 'E':
      if (OLC_OBJ(d)->ex_description == NULL) {
        CREATE(OLC_OBJ(d)->ex_description, struct extra_descr_data, 1);
        OLC_OBJ(d)->ex_description->next = NULL;
      }
      OLC_DESC(d) = OLC_OBJ(d)->ex_description;
      oedit_disp_extradesc_menu(d);
      break;
    case 'm': case 'M':
      write_to_output(d, "Enter new minimum level: ");
      OLC_MODE(d) = OEDIT_LEVEL;
      break;
    case 'p': case 'P':
      oedit_disp_perm_menu(d);
      OLC_MODE(d) = OEDIT_PERM;
      break;
    case 's': case 'S':
      OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
      OLC_MODE(d) = OLC_SCRIPT_EDIT;
      dg_script_menu(d);
      return;
    case 'V': case 'v':
      oedit_disp_values_menu(d);
      return;
    case 'w': case 'W':
      write_to_output(d, "Copy what object (vnum or 0 to cancel): ");
      OLC_MODE(d) = OEDIT_COPY;
      break;
    case 'x': case 'X':
      write_to_output(d, "Are you sure you want to delete this object? ");
      OLC_MODE(d) = OEDIT_DELETE;
      break;
    default:
      oedit_disp_menu(d);
      break;
    }
    return;

  case OLC_SCRIPT_EDIT:
  {
    /* Optional: clean, immediate quit without extra DG reprint */
    if (arg && (arg[0] == 'q' || arg[0] == 'Q') && arg[1] == '\0') {
      OLC_MODE(d) = OEDIT_MAIN_MENU;
      oedit_disp_menu(d);
      return;
    }

    /* Let DG handle input first. It returns non-zero to STAY in editor. */
    if (dg_script_edit_parse(d, arg)) {
      /* Still inside DG’s triggers UI (possibly mid-prompt like “pos, vnum”) */
      return;
    }

    /* Return value 0 means DG editor is finished -> go back to OEDIT menu */
    OLC_MODE(d) = OEDIT_MAIN_MENU;
    oedit_disp_menu(d);
    return;
  }

  case OEDIT_KEYWORD:
    if (!genolc_checkstring(d, arg)) {
      write_to_output(d, "Invalid keywords. Try again: ");
      return; /* stay in OEDIT_KEYWORD waiting for a valid line */
    }
    if (OLC_OBJ(d)->name)
      free(OLC_OBJ(d)->name);
    OLC_OBJ(d)->name = str_udup(arg);
    OLC_DIRTY(d) = 1;
    OLC_MODE(d) = OEDIT_MAIN_MENU;
    oedit_disp_menu(d);
    return;

  case OEDIT_SHORTDESC:
    if (!genolc_checkstring(d, arg)) {
      write_to_output(d, "Invalid short desc. Try again: ");
      return; /* stay in OEDIT_SHORTDESC */
    }
    if (OLC_OBJ(d)->short_description)
      free(OLC_OBJ(d)->short_description);
    OLC_OBJ(d)->short_description = str_udup(arg);
    OLC_DIRTY(d) = 1;
    OLC_MODE(d) = OEDIT_MAIN_MENU;
    oedit_disp_menu(d);
    return;

  case OEDIT_LONGDESC:
    if (!genolc_checkstring(d, arg)) {
      write_to_output(d, "Invalid long desc. Try again: ");
      return; /* stay in OEDIT_LONGDESC */
    }
    if (OLC_OBJ(d)->description)
      free(OLC_OBJ(d)->description);
    OLC_OBJ(d)->description = str_udup(arg);
    OLC_DIRTY(d) = 1;
    OLC_MODE(d) = OEDIT_MAIN_MENU;
    oedit_disp_menu(d);
    return;

  case OEDIT_MAINDESC:
    /* Multi-line editor is correct here, requires '@' to finish */
    send_editor_help(d);
    write_to_output(d, "Enter action description:\r\n\r\n");
    if (OLC_OBJ(d)->main_description) {
      write_to_output(d, "%s", OLC_OBJ(d)->main_description);
      oldtext = strdup(OLC_OBJ(d)->main_description);
    }
    string_write(d, &OLC_OBJ(d)->main_description, MAX_MESSAGE_LENGTH, 0, oldtext);
    OLC_DIRTY(d) = 1;
    return;

  case OEDIT_TYPE:
    number = atoi(arg);
    if (number < 0 || number >= NUM_ITEM_TYPES) {
      write_to_output(d, "Invalid choice, try again : ");
      return;
    }
    GET_OBJ_TYPE(OLC_OBJ(d)) = number;
    /* Reset values when type changes */
    for (int i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
      GET_OBJ_VAL(OLC_OBJ(d), i) = 0;

    OLC_DIRTY(d) = 1;
    OLC_MODE(d) = OEDIT_MAIN_MENU;
    oedit_disp_menu(d);
    return;

  case OEDIT_EXTRAS:
    number = atoi(arg);
    if (number < 0 || number > NUM_EXTRA_FLAGS) {
      oedit_disp_extra_menu(d);
      return;
    } else if (number == 0) {
      /* exit extras submenu */
      OLC_MODE(d) = OEDIT_MAIN_MENU;
      oedit_disp_menu(d);
      return;
    } else {
      /* Toggle: user picks 1..N, bit index is 0..N-1 */
      TOGGLE_BIT_AR(GET_OBJ_EXTRA(OLC_OBJ(d)), (number - 1));
      OLC_DIRTY(d) = 1;
      oedit_disp_extra_menu(d);
      return;
    }

  case OEDIT_WEAR:
    number = atoi(arg);
    if (number < 0 || number > NUM_ITEM_WEARS) {
      write_to_output(d, "That's not a valid choice!\r\n");
      oedit_disp_wear_menu(d);
      return;
    } else if (number == 0) {
      /* exit wear submenu */
      OLC_MODE(d) = OEDIT_MAIN_MENU;
      oedit_disp_menu(d);
      return;
    } else {
      TOGGLE_BIT_AR(GET_OBJ_WEAR(OLC_OBJ(d)), (number - 1));
      OLC_DIRTY(d) = 1;
      oedit_disp_wear_menu(d);
      return;
    }

  case OEDIT_WEIGHT:
    GET_OBJ_WEIGHT(OLC_OBJ(d)) = LIMIT(atoi(arg), 0, MAX_OBJ_WEIGHT);
    OLC_DIRTY(d) = 1;
    OLC_MODE(d) = OEDIT_MAIN_MENU;
    oedit_disp_menu(d);
    return;

  case OEDIT_COST:
    GET_OBJ_COST(OLC_OBJ(d)) = LIMIT(atoi(arg), 0, MAX_OBJ_COST);
    OLC_DIRTY(d) = 1;
    OLC_MODE(d) = OEDIT_MAIN_MENU;
    oedit_disp_menu(d);
    return;

  case OEDIT_TIMER:
    GET_OBJ_TIMER(OLC_OBJ(d)) = LIMIT(atoi(arg), 0, MAX_OBJ_TIMER);
    OLC_DIRTY(d) = 1;
    OLC_MODE(d) = OEDIT_MAIN_MENU;
    oedit_disp_menu(d);
    return;

  case OEDIT_LEVEL:
    GET_OBJ_LEVEL(OLC_OBJ(d)) = LIMIT(atoi(arg), 0, LVL_IMPL);
    OLC_DIRTY(d) = 1;
    OLC_MODE(d) = OEDIT_MAIN_MENU;
    oedit_disp_menu(d);
    return;

  case OEDIT_PERM:
    if ((number = atoi(arg)) == 0) {
      /* exit perm affects submenu */
      OLC_MODE(d) = OEDIT_MAIN_MENU;
      oedit_disp_menu(d);
      return;
    }
    if (number > 0 && number < NUM_AFF_FLAGS) {
      if (number != AFF_CHARM) {
        TOGGLE_BIT_AR(GET_OBJ_AFFECT(OLC_OBJ(d)), number);
      }
      OLC_DIRTY(d) = 1;
    }
    oedit_disp_perm_menu(d);
    return;

  /* === Values menu unified === */
  case OEDIT_VALUES_MENU:
    if (*arg == 'Q' || *arg == 'q') {
      oedit_disp_menu(d);
      return;
    } else {
      int i = atoi(arg) - 1;
      if (i >= 0 && i < NUM_OBJ_VAL_POSITIONS) {
        /* Prevent editing current_occupants for furniture - it's managed by the game engine */
        if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_FURNITURE && i == 1) {
          write_to_output(d, "The current_occupants field is managed by the game engine and cannot be edited manually.\r\n");
          oedit_disp_values_menu(d);
          return;
        }
        
        OLC_VAL_SLOT(d) = i;
        const char * const *labels = get_val_labels(OLC_OBJ(d));

        if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_WEAPON && i == 2) {
          oedit_disp_weapon_menu(d);
          OLC_MODE(d) = OEDIT_VALUE_X;
          return;
        }
        if ((GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_SCROLL ||
             GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_POTION ||
             GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_WAND   ||
             GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_STAFF) &&
             (i == 1 || i == 2 || i == 3)) {
          oedit_disp_spells_menu(d);
          OLC_MODE(d) = OEDIT_VALUE_X;
          return;
        }
        if ((GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_DRINKCON ||
             GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_FOUNTAIN) &&
             i == 2) {
          oedit_liquid_type(d);
          OLC_MODE(d) = OEDIT_VALUE_X;
          return;
        }
        if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_CONTAINER && i == 1) {
          oedit_disp_container_flags_menu(d);
          OLC_MODE(d) = OEDIT_VALUE_X;
          return;
        }

        write_to_output(d, "Enter new integer for %s : ", labels[i]);
        OLC_MODE(d) = OEDIT_VALUE_X;
      } else {
        write_to_output(d, "Invalid choice.\r\n");
        oedit_disp_values_menu(d);
      }
    }
    break;

  case OEDIT_VALUE_X:
  {
    int i = OLC_VAL_SLOT(d);
    int number = atoi(arg);

    /* --- Armor-specific semantics --- */
    if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_ARMOR) {
      if (i == VAL_ARMOR_STEALTH_DISADV /* 3 */) {
        GET_OBJ_VAL(OLC_OBJ(d), i) = (number != 0) ? 1 : 0;
        OLC_DIRTY(d) = 1;
        oedit_disp_values_menu(d);
        return;
      }
      if (i == VAL_ARMOR_STR_REQ /* 5 */) {
        /* 0 disables the requirement; otherwise accept a sane STR range */
        if (number < 0 || number > 25) {
          write_to_output(d, "Enter STR requirement (0 disables, 3..25 typical): ");
          return; /* stay in OEDIT_VALUE_X for a valid number */
        }
        GET_OBJ_VAL(OLC_OBJ(d), i) = number;
        OLC_DIRTY(d) = 1;
        oedit_disp_values_menu(d);
        return;
      }
    }

    /* --- Worn-specific semantics (clothing, rings, etc) --- */
    if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_WORN) {

      /* oval 1: closable (0/1) */
      if (i == WORN_CAN_OPEN_CLOSE) {
        if (number != 0 && number != 1) {
          write_to_output(d, "Enter 0 or 1 to set closable: ");
          return; /* stay in OEDIT_VALUE_X */
        }
        GET_OBJ_VAL(OLC_OBJ(d), i) = number;
        OLC_DIRTY(d) = 1;
        oedit_disp_values_menu(d);
        return;
      }

      /* oval 2: hooded (0/1) */
      if (i == WORN_CAN_HOOD) {
        if (number != 0 && number != 1) {
          write_to_output(d, "Enter 0 or 1 to set hooded: ");
          return; /* stay in OEDIT_VALUE_X */
        }
        GET_OBJ_VAL(OLC_OBJ(d), i) = number;

        /* If it can no longer have a hood, force hood state down. */
        if (number == 0) {
          REMOVE_BIT_AR(GET_OBJ_EXTRA(OLC_OBJ(d)), ITEM_HOOD_UP);
        }

        OLC_DIRTY(d) = 1;
        oedit_disp_values_menu(d);
        return;
      }
    }

    /* --- Existing special cases (weapon/liquid/spells/container) remain here --- */
    if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_WEAPON && i == 2) {
      if (number < 0 || number >= NUM_ATTACK_TYPES) {
        oedit_disp_weapon_menu(d);
        return;
      }
      GET_OBJ_VAL(OLC_OBJ(d), i) = number;
      OLC_DIRTY(d) = 1;
      oedit_disp_values_menu(d);
      return;
    }
    else if ((GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_SCROLL ||
              GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_POTION ||
              GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_WAND   ||
              GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_STAFF) &&
            (i == 1 || i == 2 || i == 3)) {
      if (number < 0 || number >= NUM_SPELLS) {
        oedit_disp_spells_menu(d);
        return;
      }
      GET_OBJ_VAL(OLC_OBJ(d), i) = number;
      OLC_DIRTY(d) = 1;
      oedit_disp_values_menu(d);
      return;
    }
    else if ((GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_DRINKCON ||
              GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_FOUNTAIN) &&
            i == 2) {
      if (number < 0 || number >= NUM_LIQ_TYPES) {
        oedit_liquid_type(d);
        return;
      }
      GET_OBJ_VAL(OLC_OBJ(d), i) = number;
      OLC_DIRTY(d) = 1;
      oedit_disp_values_menu(d);
      return;
    }
    else if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_CONTAINER && i == 1) {
      extern const int NUM_CONTAINER_FLAGS;
      if (number < 0 || number >= NUM_CONTAINER_FLAGS) {
        oedit_disp_container_flags_menu(d);
        return;
      }
      TOGGLE_BIT(GET_OBJ_VAL(OLC_OBJ(d), i), 1 << number);
      OLC_DIRTY(d) = 1;
      oedit_disp_values_menu(d);
      return;
    }

    /* --- Default assignment for other slots/types --- */
    GET_OBJ_VAL(OLC_OBJ(d), i) = number;
    OLC_DIRTY(d) = 1;
    oedit_disp_values_menu(d);
    return;
  }

  /* === Apply editing === */
  case OEDIT_PROMPT_APPLY:
    if ((number = atoi(arg)) == 0) break;
    else if (number < 0 || number > MAX_OBJ_AFFECT) {
      oedit_disp_prompt_apply_menu(d);
      return;
    }
    OLC_VAL(d) = number - 1;
    OLC_MODE(d) = OEDIT_APPLY;
    oedit_disp_apply_menu(d);
    return;

  case OEDIT_APPLY:
    if (((number = atoi(arg)) == 0) || ((number = atoi(arg)) == 1)) {
      OLC_OBJ(d)->affected[OLC_VAL(d)].location = 0;
      OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = 0;
      oedit_disp_prompt_apply_menu(d);
    } else if (number < 0 || number > NUM_APPLIES)
      oedit_disp_apply_menu(d);
    else {
      int counter;
      if (GET_LEVEL(d->character) < LVL_IMPL) {
        for (counter = 0; counter < MAX_OBJ_AFFECT; counter++) {
          if (OLC_OBJ(d)->affected[counter].location == number) {
            write_to_output(d, "Object already has that apply.");
            return;
          }
        }
      }
      OLC_OBJ(d)->affected[OLC_VAL(d)].location = number - 1;
      write_to_output(d, "Modifier : ");
      OLC_DIRTY(d) = 1;
      OLC_MODE(d) = OEDIT_APPLYMOD;
    }
    return;

  case OEDIT_APPLYMOD:
    OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = atoi(arg);
    OLC_DIRTY(d) = 1;
    oedit_disp_prompt_apply_menu(d);
    return;

  /* === Extra descriptions === */
  case OEDIT_EXTRADESC_KEY:
    if (genolc_checkstring(d, arg)) {
      if (OLC_DESC(d)->keyword) free(OLC_DESC(d)->keyword);
      OLC_DESC(d)->keyword = str_udup(arg);
    }
    oedit_disp_extradesc_menu(d);
    return;

  case OEDIT_EXTRADESC_MENU:
    switch ((number = atoi(arg))) {
    case 0:
      if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
        struct extra_descr_data *temp;
        if (OLC_DESC(d)->keyword) free(OLC_DESC(d)->keyword);
        if (OLC_DESC(d)->description) free(OLC_DESC(d)->description);
        REMOVE_FROM_LIST(OLC_DESC(d), OLC_OBJ(d)->ex_description, next);
        free(OLC_DESC(d));
        OLC_DESC(d) = NULL;
        OLC_DIRTY(d) = 1;
      }
      break;
    case 1:
      OLC_MODE(d) = OEDIT_EXTRADESC_KEY;
      write_to_output(d, "Enter keywords, separated by spaces :-\r\n| ");
      return;
    case 2:
      OLC_MODE(d) = OEDIT_EXTRADESC_DESCRIPTION;
      send_editor_help(d);
      write_to_output(d, "Enter the extra description:\r\n\r\n");
      if (OLC_DESC(d)->description) {
        write_to_output(d, "%s", OLC_DESC(d)->description);
        oldtext = strdup(OLC_DESC(d)->description);
      }
      string_write(d, &OLC_DESC(d)->description, MAX_MESSAGE_LENGTH, 0, oldtext);
      OLC_DIRTY(d) = 1;
      return;
    case 3:
      if (OLC_DESC(d)->keyword && OLC_DESC(d)->description) {
        struct extra_descr_data *new_extra;
        if (OLC_DESC(d)->next)
          OLC_DESC(d) = OLC_DESC(d)->next;
        else {
          CREATE(new_extra, struct extra_descr_data, 1);
          OLC_DESC(d)->next = new_extra;
          OLC_DESC(d) = OLC_DESC(d)->next;
        }
      }
    default:
      oedit_disp_extradesc_menu(d);
      return;
    }
    break;

  /* === Copy object === */
  case OEDIT_COPY:
  {
    /* Trim leading spaces if you have a helper; otherwise simple checks below handle it */
    /* skip_spaces(&arg); */

    /* Treat empty input as cancel */
    if (!arg || *arg == '\0') {
      write_to_output(d, "Copy cancelled.\r\n");
      OLC_MODE(d) = OEDIT_MAIN_MENU;
      oedit_disp_menu(d);
      return;
    }

    /* Allow 0 or q/Q to cancel */
    if ((arg[0] == '0' && arg[1] == '\0') ||
        (arg[0] == 'q' && arg[1] == '\0') ||
        (arg[0] == 'Q' && arg[1] == '\0')) {
      write_to_output(d, "Copy cancelled.\r\n");
      OLC_MODE(d) = OEDIT_MAIN_MENU;
      oedit_disp_menu(d);
      return;
    }

    /* Require a number */
    if (!is_number(arg)) {
      write_to_output(d, "Please enter a vnum or 0 to cancel: ");
      return; /* stay in OEDIT_COPY */
    }

    int vnum = atoi(arg);
    if (vnum <= 0) {
      write_to_output(d, "Copy cancelled.\r\n");
      OLC_MODE(d) = OEDIT_MAIN_MENU;
      oedit_disp_menu(d);
      return;
    }

    int rnum = real_object(vnum);
    if (rnum == NOTHING) {
      write_to_output(d, "That object does not exist. Enter vnum or 0 to cancel: ");
      return; /* stay in OEDIT_COPY */
    }

    /* Success: clone into editor */
    oedit_setup_existing(d, rnum);
    OLC_MODE(d) = OEDIT_MAIN_MENU;
    oedit_disp_menu(d);
    return;
  }

  /* === Delete object === */
  case OEDIT_DELETE:
    if (*arg == 'y' || *arg == 'Y') {
      if (delete_object(GET_OBJ_RNUM(OLC_OBJ(d))) != NOTHING)
        write_to_output(d, "Object deleted.\r\n");
      else
        write_to_output(d, "Couldn't delete the object!\r\n");
      cleanup_olc(d, CLEANUP_ALL);
    } else if (*arg == 'n' || *arg == 'N') {
      oedit_disp_menu(d);
      OLC_MODE(d) = OEDIT_MAIN_MENU;
    } else
      write_to_output(d, "Please answer 'Y' or 'N': ");
    return;

  default:
    mudlog(BRF, LVL_BUILDER, TRUE,
           "SYSERR: OLC: Reached default case in oedit_parse()!");
    write_to_output(d, "Oops...\r\n");
    break;
  }

  /* Only redisplay main menu if we are in main menu mode */
  if (OLC_MODE(d) == OEDIT_MAIN_MENU) {
    oedit_disp_menu(d);
  }
}

void oedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {
  case OEDIT_MAINDESC:
    OLC_DIRTY(d) = 1;
    oedit_disp_menu(d);
    break;
  case OEDIT_EXTRADESC_DESCRIPTION:
    OLC_DIRTY(d) = 1;
    oedit_disp_extradesc_menu(d);
    break;
  }
}
