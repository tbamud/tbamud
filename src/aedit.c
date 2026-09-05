/**************************************************************************
*  File: aedit.c                                           Part of tbaMUD *
*  Usage: OLC for MUDs -- this one edits socials.                         *
*  by Michael Scott                                                       *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "oasis.h"
#include "screen.h"
#include "constants.h"
#include "genolc.h"
#include "act.h"


/* local utility functions */
static int aedit_find_command(const char *txt);
static void aedit_disp_menu(struct descriptor_data * d);
static void aedit_save_to_disk(struct descriptor_data *d);
/* used in aedit parse */
static void aedit_setup_new(struct descriptor_data *d);
static void aedit_setup_existing(struct descriptor_data *d, int real_num);
static void aedit_save_internally(struct descriptor_data *d);



/* Utils and exported functions. */
ACMD(do_oasis_aedit)
{
  char arg[MAX_INPUT_LENGTH];
  struct descriptor_data *d;
  int i;

  /* No building as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;
    
  if (CONFIG_NEW_SOCIALS == 0) {
    send_to_char(ch, "Socials cannot be edited at the moment.\r\n");
    return;
  }

  if (!can_edit_zone(ch, AEDIT_PERMISSION)) {
    send_to_char(ch, "You don't have access to editing socials.\r\n");
    return;
  }

  for (d = descriptor_list; d; d = d->next)
    if (STATE(d) == CON_AEDIT) {
      send_to_char(ch, "Sorry, only one can edit socials at a time.\r\n");
      return;
    }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Please specify a social to edit.\r\n");
    return;
  }

  d = ch->desc;

  if (!str_cmp("save", arg)) {
    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "OLC: %s saves socials.", GET_NAME(ch));
    send_to_char(ch, "Writing social file.\r\n");
    aedit_save_to_disk(d);
    send_to_char(ch, "Done.\r\n");
    return;
  }

  /* Give descriptor an OLC structure. */
  if (d->olc) {
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_oasis: Player already had olc structure.");
    free(d->olc);
  }
  CREATE(d->olc, struct oasis_olc_data, 1);

  OLC_NUM(d) = 0;
  OLC_STORAGE(d) = strdup(arg);

  for (OLC_ZNUM(d) = 0; (OLC_ZNUM(d) <= top_of_socialt); OLC_ZNUM(d)++)
    if (is_abbrev(OLC_STORAGE(d), soc_mess_list[OLC_ZNUM(d)].command))
      break;

  if (OLC_ZNUM(d) > top_of_socialt)  {
    if ((i = aedit_find_command(OLC_STORAGE(d))) != -1)  {
      send_to_char(ch, "The '%s' command already exists (%s).\r\n", OLC_STORAGE(d), complete_cmd_info[i].command);
      cleanup_olc(d, CLEANUP_ALL);
      return;
    }
    send_to_char(ch, "Do you wish to add the '%s' action? ", OLC_STORAGE(d));
    OLC_MODE(d) = AEDIT_CONFIRM_ADD;
  } else {
    send_to_char(ch, "Do you wish to edit the '%s' action? ", soc_mess_list[OLC_ZNUM(d)].command);
    OLC_MODE(d) = AEDIT_CONFIRM_EDIT;
  }
  STATE(d) = CON_AEDIT;
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
  mudlog(CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "OLC: %s starts editing actions.", GET_NAME(ch));
}

static void aedit_setup_new(struct descriptor_data *d) {
   CREATE(OLC_ACTION(d), struct social_messg, 1);
   OLC_ACTION(d)->command             = strdup(OLC_STORAGE(d));
   OLC_ACTION(d)->sort_as             = strdup(OLC_STORAGE(d));
   OLC_ACTION(d)->hide                = 0;
   OLC_ACTION(d)->min_victim_position = POS_STANDING;
   OLC_ACTION(d)->min_char_position   = POS_STANDING;
   OLC_ACTION(d)->min_level_char      = 0;
   OLC_ACTION(d)->char_no_arg         = strdup("This action is unfinished.");
   OLC_ACTION(d)->others_no_arg       = strdup("This action is unfinished.");
   OLC_ACTION(d)->char_found          = NULL;
   OLC_ACTION(d)->others_found        = NULL;
   OLC_ACTION(d)->vict_found          = NULL;
   OLC_ACTION(d)->not_found           = NULL;
   OLC_ACTION(d)->char_auto           = NULL;
   OLC_ACTION(d)->others_auto         = NULL;
   OLC_ACTION(d)->char_body_found     = NULL;
   OLC_ACTION(d)->others_body_found   = NULL;
   OLC_ACTION(d)->vict_body_found     = NULL;
   OLC_ACTION(d)->char_obj_found      = NULL;
   OLC_ACTION(d)->others_obj_found    = NULL;
   aedit_disp_menu(d);
   OLC_VAL(d) = 0;
}

static void aedit_setup_existing(struct descriptor_data *d, int real_num) {
   CREATE(OLC_ACTION(d), struct social_messg, 1);
   OLC_ACTION(d)->command             = strdup(soc_mess_list[real_num].command);
   OLC_ACTION(d)->sort_as             = strdup(soc_mess_list[real_num].sort_as);
   OLC_ACTION(d)->hide                = soc_mess_list[real_num].hide;
   OLC_ACTION(d)->min_victim_position = soc_mess_list[real_num].min_victim_position;
   OLC_ACTION(d)->min_char_position   = soc_mess_list[real_num].min_char_position;
   OLC_ACTION(d)->min_level_char      = soc_mess_list[real_num].min_level_char;
   if (soc_mess_list[real_num].char_no_arg)
     OLC_ACTION(d)->char_no_arg       = strdup(soc_mess_list[real_num].char_no_arg);
   if (soc_mess_list[real_num].others_no_arg)
     OLC_ACTION(d)->others_no_arg     = strdup(soc_mess_list[real_num].others_no_arg);
   if (soc_mess_list[real_num].char_found)
     OLC_ACTION(d)->char_found        = strdup(soc_mess_list[real_num].char_found);
   if (soc_mess_list[real_num].others_found)
     OLC_ACTION(d)->others_found      = strdup(soc_mess_list[real_num].others_found);
   if (soc_mess_list[real_num].vict_found)
     OLC_ACTION(d)->vict_found        = strdup(soc_mess_list[real_num].vict_found);
   if (soc_mess_list[real_num].not_found)
     OLC_ACTION(d)->not_found         = strdup(soc_mess_list[real_num].not_found);
   if (soc_mess_list[real_num].char_auto)
     OLC_ACTION(d)->char_auto         = strdup(soc_mess_list[real_num].char_auto);
   if (soc_mess_list[real_num].others_auto)
     OLC_ACTION(d)->others_auto       = strdup(soc_mess_list[real_num].others_auto);
   if (soc_mess_list[real_num].char_body_found)
     OLC_ACTION(d)->char_body_found   = strdup(soc_mess_list[real_num].char_body_found);
   if (soc_mess_list[real_num].others_body_found)
     OLC_ACTION(d)->others_body_found = strdup(soc_mess_list[real_num].others_body_found);
   if (soc_mess_list[real_num].vict_body_found)
     OLC_ACTION(d)->vict_body_found   = strdup(soc_mess_list[real_num].vict_body_found);
   if (soc_mess_list[real_num].char_obj_found)
     OLC_ACTION(d)->char_obj_found    = strdup(soc_mess_list[real_num].char_obj_found);
   if (soc_mess_list[real_num].others_obj_found)
     OLC_ACTION(d)->others_obj_found  = strdup(soc_mess_list[real_num].others_obj_found);
   OLC_VAL(d) = 0;
   aedit_disp_menu(d);
}

static void aedit_save_internally(struct descriptor_data *d) {
   struct social_messg *new_soc_mess_list = NULL;
   int i;

   /* add a new social into the list */
   if (OLC_ZNUM(d) > top_of_socialt)  {
      CREATE(new_soc_mess_list, struct social_messg, top_of_socialt + 2);
      for (i = 0; i <= top_of_socialt; i++)
              new_soc_mess_list[i] = soc_mess_list[i];
      new_soc_mess_list[++top_of_socialt] = *OLC_ACTION(d);
      free(soc_mess_list);
      soc_mess_list = new_soc_mess_list;
   }
   /* pass the editted action back to the list - no need to add */
   else {
      aedit_find_command(OLC_ACTION(d)->command);
      OLC_ACTION(d)->act_nr = soc_mess_list[OLC_ZNUM(d)].act_nr;
      /* why did i do this..? hrm */
      free_action(soc_mess_list + OLC_ZNUM(d));
      soc_mess_list[OLC_ZNUM(d)] = *OLC_ACTION(d);
   }

   create_command_list(); /* aedit patch -- M. Scott */

   add_to_save_list(AEDIT_PERMISSION, SL_ACT);
   aedit_save_to_disk(d); /* autosave by Rumble */
}

/* Remove a social from soc_mess_list.
 *
 * soc_mess_list's own act_nr needs no renumbering: it indexes the MERGED
 * command table, and create_command_list() rebuilds that from scratch and
 * reassigns every one. That is NOT true of the merged table's outside
 * users, and an earlier version of this comment said the array was the only
 * thing to get right. Four statics cache a find_command() result and are
 * never reassigned -- cmd_slap and cmd_puke in shop.c, snarl_cmd in
 * mobact.c, spit_social in spec_procs.c -- and between them they reach
 * do_action() at five call sites, where they then resolve the wrong social
 * until the next reboot. Delete a social that sorts BEFORE `slap` and a
 * caught thief gets whatever now sits at slap's old index. Adding one moves
 * it the other way, so this is upstream's bug and not the delete's, but the
 * delete is a second way in. It is fixed at those four statics rather than
 * here, in #284, which retakes every one of them from create_command_list()
 * -- the same place the table they index is built. That belongs there and
 * not in this editor: it changes what a shopkeeper does to a caught thief,
 * not anything about deleting a social.
 *
 * Note top_of_socialt is a LAST INDEX, not a count -- unlike top_of_helpt
 * next door in hedit -- so the bound is `> top_of_socialt` and the shift
 * stops one short of it. */
static int aedit_delete_social(int rnum)
{
  int i;

  if (rnum < 0 || rnum > top_of_socialt)
    return FALSE;

  log("GenOLC: aedit_delete_social: Deleting social '%s'.",
      soc_mess_list[rnum].command ? soc_mess_list[rnum].command : "unnamed");

  free_action(soc_mess_list + rnum);

  for (i = rnum; i < top_of_socialt; i++)
    soc_mess_list[i] = soc_mess_list[i + 1];

  top_of_socialt--;

  return TRUE;
}

static void aedit_save_to_disk(struct descriptor_data *d) {
   FILE *fp;
   int i;
   char buf[MAX_STRING_LENGTH];
   char tmp_name[READ_SIZE];

   /* Write beside the socials file and rename over it once it is whole.
    * This opened the live file with "w+", which truncates it, and called
    * exit(1) if that failed -- so an immortal saving a social on a full
    * disk or a read-only mount took the whole MUD down with them.  Say so
    * and leave the save on the list instead. */
   if (snprintf(tmp_name, sizeof(tmp_name), "%s.tmp", SOCMESS_FILE_NEW) >= (int)sizeof(tmp_name)) {
     log("SYSERR: Socials file name too long to write beside: %s", SOCMESS_FILE_NEW);
     return;
   }

   if (!(fp = fopen(tmp_name, "w")))  {
     log("SYSERR: Can't open socials file '%s': %s", tmp_name, strerror(errno));
     if (d->character)
       send_to_char(d->character, "Could not write the socials file; the save is still pending.\r\n");
     return;
   }

   for (i = 0; i <= top_of_socialt; i++)  {
      fprintf(fp, "~%s %s %d %d %d %d\n",
              soc_mess_list[i].command,
              soc_mess_list[i].sort_as,
              soc_mess_list[i].hide,
              soc_mess_list[i].min_char_position,
              soc_mess_list[i].min_victim_position,
              soc_mess_list[i].min_level_char);
              
      sprintf(buf, "%s\n%s\n%s\n%s\n",
              ((soc_mess_list[i].char_no_arg)?soc_mess_list[i].char_no_arg:"#"),
              ((soc_mess_list[i].others_no_arg)?soc_mess_list[i].others_no_arg:"#"),
              ((soc_mess_list[i].char_found)?soc_mess_list[i].char_found:"#"),
              ((soc_mess_list[i].others_found)?soc_mess_list[i].others_found:"#"));
      fprintf(fp, "%s", convert_from_tabs(buf));
      
      sprintf(buf, "%s\n%s\n%s\n%s\n",
              ((soc_mess_list[i].vict_found)?soc_mess_list[i].vict_found:"#"),
              ((soc_mess_list[i].not_found)?soc_mess_list[i].not_found:"#"),
              ((soc_mess_list[i].char_auto)?soc_mess_list[i].char_auto:"#"),
              ((soc_mess_list[i].others_auto)?soc_mess_list[i].others_auto:"#"));
      fprintf(fp, "%s", convert_from_tabs(buf));
      
      sprintf(buf, "%s\n%s\n%s\n",
              ((soc_mess_list[i].char_body_found)?soc_mess_list[i].char_body_found:"#"),
              ((soc_mess_list[i].others_body_found)?soc_mess_list[i].others_body_found:"#"),
              ((soc_mess_list[i].vict_body_found)?soc_mess_list[i].vict_body_found:"#"));
      fprintf(fp, "%s", convert_from_tabs(buf));
      
      sprintf(buf, "%s\n%s\n\n",
              ((soc_mess_list[i].char_obj_found)?soc_mess_list[i].char_obj_found:"#"),
              ((soc_mess_list[i].others_obj_found)?soc_mess_list[i].others_obj_found:"#"));
      fprintf(fp, "%s", convert_from_tabs(buf));
   }

   fprintf(fp, "$\n");

   if (fflush(fp) == EOF || ferror(fp) || fclose(fp) == EOF) {
     log("SYSERR: Could not write socials file '%s': %s", tmp_name, strerror(errno));
     if (d->character)
       send_to_char(d->character, "Could not write the socials file; the save is still pending.\r\n");
     remove(tmp_name);
     return;
   }

   /* rename() replaces the destination outright on POSIX; the Windows C
    * runtime refuses a name that already exists, hence the second try.
    * objsave.c:548-556 installs rent files the same way. */
   if (rename(tmp_name, SOCMESS_FILE_NEW)) {
     remove(SOCMESS_FILE_NEW);
     if (rename(tmp_name, SOCMESS_FILE_NEW)) {
       log("SYSERR: Could not put the socials file in place: %s", strerror(errno));
       remove(tmp_name);
       return;
     }
   }

   remove_from_save_list(AEDIT_PERMISSION, SL_ACT);
}

/* The Main Menu. */
static void aedit_disp_menu(struct descriptor_data * d) {
   struct social_messg *action = OLC_ACTION(d);
   struct char_data *ch        = d->character;

   get_char_colors(ch);

   write_to_output(d,
           "%s-- Action editor\r\n"
           "%sn%s) Command         : %s%-15.15s%s %s1%s) Sort as Command  : %s%-15.15s%s\r\n"
           "%s2%s) Min Position[CH]: %s%-8.8s        %s3%s) Min Position [VT]: %s%-8.8s\r\n"
           "%s4%s) Min Level   [CH]: %s%-3d             %s5%s) Show if Invisible: %s%s\r\n"
           "%sa%s) Char    [NO ARG]: %s%s\r\n"
           "%sb%s) Others  [NO ARG]: %s%s\r\n"
           "%sc%s) Char [NOT FOUND]: %s%s\r\n"
           "%sd%s) Char  [ARG SELF]: %s%s\r\n"
           "%se%s) Others[ARG SELF]: %s%s\r\n"
           "%sf%s) Char      [VICT]: %s%s\r\n"
           "%sg%s) Others    [VICT]: %s%s\r\n"
           "%sh%s) Victim    [VICT]: %s%s\r\n"
           "%si%s) Char  [BODY PRT]: %s%s\r\n"
           "%sj%s) Others[BODY PRT]: %s%s\r\n"
           "%sk%s) Victim[BODY PRT]: %s%s\r\n"
           "%sl%s) Char       [OBJ]: %s%s\r\n"
           "%sm%s) Others     [OBJ]: %s%s\r\n"
           "%sX%s) Delete this social\r\n"
           "%sq%s) Quit\r\n"
           "Enter Choice:",
           nrm, grn, nrm,
           yel, action->command, nrm,
           grn, nrm,
           yel, action->sort_as, nrm,
           grn, nrm,
           cyn, position_types[action->min_char_position],
           grn, nrm,
           cyn, position_types[action->min_victim_position],
           grn, nrm,
           cyn, action->min_level_char,
           grn, nrm,
           cyn, (action->hide?"HIDDEN":"NOT HIDDEN"),
           grn, nrm, cyn,
           action->char_no_arg ? action->char_no_arg : "<Null>",
           grn, nrm, cyn,
           action->others_no_arg ? action->others_no_arg : "<Null>",
           grn, nrm, cyn,
           action->not_found ? action->not_found : "<Null>",
           grn, nrm, cyn,
           action->char_auto ? action->char_auto : "<Null>",
           grn, nrm, cyn,
           action->others_auto ? action->others_auto : "<Null>",
           grn, nrm, cyn,
           action->char_found ? action->char_found : "<Null>",
           grn, nrm, cyn,
           action->others_found ? action->others_found : "<Null>",
           grn, nrm, cyn,
           action->vict_found ? action->vict_found : "<Null>",
           grn, nrm, cyn,
           action->char_body_found ? action->char_body_found : "<Null>",
           grn, nrm, cyn,
           action->others_body_found ? action->others_body_found : "<Null>",
           grn, nrm, cyn,
           action->vict_body_found ? action->vict_body_found : "<Null>",
           grn, nrm, cyn,
           action->char_obj_found ? action->char_obj_found : "<Null>",
           grn, nrm, cyn,
           action->others_obj_found ? action->others_obj_found : "<Null>",
           grn, nrm,
           grn, nrm);

   OLC_MODE(d) = AEDIT_MAIN_MENU;
}

/* The main loop. */
void aedit_parse(struct descriptor_data * d, char *arg) {
   int i;

   switch (OLC_MODE(d)) {
    case AEDIT_CONFIRM_SAVESTRING:
      switch (*arg) {
       case 'y': case 'Y':
         aedit_save_internally(d);
         mudlog (CMP, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE, "OLC: %s edits action %s",
                 GET_NAME(d->character), OLC_ACTION(d)->command);

         /* do not free the strings.. just the structure */
         cleanup_olc(d, CLEANUP_STRUCTS);
         write_to_output(d, "Action saved to disk.\r\n");
         break;
       case 'n': case 'N':
         /* free everything up, including strings etc */
         cleanup_olc(d, CLEANUP_ALL);
         break;
       default:
         write_to_output(d, "Invalid choice!\r\n"
                            "Do you wish to save your changes? : ");
         break;
      }
      return; /* end of AEDIT_CONFIRM_SAVESTRING */

    case AEDIT_CONFIRM_EDIT:
      switch (*arg)  {
       case 'y': case 'Y':
         aedit_setup_existing(d, OLC_ZNUM(d));
         break;
       case 'q': case 'Q':
         cleanup_olc(d, CLEANUP_ALL);
         break;
       case 'n': case 'N':
         OLC_ZNUM(d)++;
         for (;(OLC_ZNUM(d) <= top_of_socialt); OLC_ZNUM(d)++)
           if (is_abbrev(OLC_STORAGE(d), soc_mess_list[OLC_ZNUM(d)].command))
             break;

         if (OLC_ZNUM(d) > top_of_socialt) {
            if (aedit_find_command(OLC_STORAGE(d)) != -1)  {
               cleanup_olc(d, CLEANUP_ALL);
               break;
            }
            write_to_output(d, "Do you wish to add the '%s' action? ",
                               OLC_STORAGE(d));
            OLC_MODE(d) = AEDIT_CONFIRM_ADD;
         } else  {
            write_to_output(d, "Do you wish to edit the '%s' action? ",
                            soc_mess_list[OLC_ZNUM(d)].command);
            OLC_MODE(d) = AEDIT_CONFIRM_EDIT;
         }
         break;
       default:
         write_to_output(d, "Invalid choice!\r\n"
                            "Do you wish to edit the '%s' action? ",
                            soc_mess_list[OLC_ZNUM(d)].command);
         break;
      }
      return;

    case AEDIT_CONFIRM_ADD:
      switch (*arg)  {
       case 'y': case 'Y':
         aedit_setup_new(d);
         break;
       case 'n': case 'N': case 'q': case 'Q':
         cleanup_olc(d, CLEANUP_ALL);
         break;
       default:
         write_to_output(d, "Invalid choice!\r\n"
                            "Do you wish to add the '%s' action? ",
                            OLC_STORAGE(d));
         break;
      }
      return;

    case AEDIT_CONFIRM_DELETE:
      switch (*arg) {
       case 'y': case 'Y': {
         /* Captured before the delete frees the row it names. */
         char sname[MAX_INPUT_LENGTH];

         snprintf(sname, sizeof(sname), "%s",
                  (OLC_ZNUM(d) >= 0 && OLC_ZNUM(d) <= top_of_socialt &&
                   soc_mess_list[OLC_ZNUM(d)].command)
                    ? soc_mess_list[OLC_ZNUM(d)].command : "?");

         if (aedit_delete_social(OLC_ZNUM(d))) {
           /* create_command_list rebuilds the merged table and reassigns
            * every act_nr; sort_commands then reorders the lookup. Both are
            * what aedit_save_internally does after any other change. */
           create_command_list();
           sort_commands();
           add_to_save_list(AEDIT_PERMISSION, SL_ACT);
           aedit_save_to_disk(d);
           mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE,
                  "OLC: %s deletes social %s", GET_NAME(d->character), sname);
           write_to_output(d, "Social deleted.\r\n");
           cleanup_olc(d, CLEANUP_ALL);
           return;
         }
         write_to_output(d, "Could not delete that social.\r\n");
         aedit_disp_menu(d);
         return;
       }
       case 'n': case 'N':
         aedit_disp_menu(d);
         return;
       default:
         write_to_output(d, "Invalid choice!\r\n");
         /* The TABLE's name, not the working copy's. Menu key n rewrites
          * the copy and leaves soc_mess_list alone, so a rename followed
          * by a delete offered to remove a social that does not exist
          * while removing the one that does. */
         write_to_output(d, "Delete '%s'? : ",
                         (OLC_ZNUM(d) >= 0 && OLC_ZNUM(d) <= top_of_socialt &&
                          soc_mess_list[OLC_ZNUM(d)].command)
                           ? soc_mess_list[OLC_ZNUM(d)].command : "this social");
         return;
      }

    case AEDIT_MAIN_MENU:
      switch (*arg) {
       case 'x': case 'X':
         if (OLC_ZNUM(d) < 0 || OLC_ZNUM(d) > top_of_socialt) {
           write_to_output(d, "That social has not been saved yet -- quit without saving instead.\r\n");
           aedit_disp_menu(d);
           return;
         }
         /* The TABLE's name, not the working copy's. Menu key n rewrites
          * the copy and leaves soc_mess_list alone, so a rename followed
          * by a delete offered to remove a social that does not exist
          * while removing the one that does. */
         write_to_output(d, "Delete '%s'? : ",
                         (OLC_ZNUM(d) >= 0 && OLC_ZNUM(d) <= top_of_socialt &&
                          soc_mess_list[OLC_ZNUM(d)].command)
                           ? soc_mess_list[OLC_ZNUM(d)].command : "this social");
         OLC_MODE(d) = AEDIT_CONFIRM_DELETE;
         return;
       case 'q': case 'Q':
         if (OLC_VAL(d))  { /* Something was modified */
            write_to_output(d, "Do you wish to save your changes? : ");
            OLC_MODE(d) = AEDIT_CONFIRM_SAVESTRING;
         }
         else cleanup_olc(d, CLEANUP_ALL);
         break;
       case 'n':
         write_to_output(d, "Enter action name: ");
         OLC_MODE(d) = AEDIT_ACTION_NAME;
         return;
       case '1':
         write_to_output(d, "Enter sort info for this action (for the command listing): ");
         OLC_MODE(d) = AEDIT_SORT_AS;
         return;
       case '2':
         write_to_output(d, "Enter the minimum position the Character has to be in to activate social:\r\n");
         for (i=POS_DEAD;i<=POS_STANDING;i++)
           write_to_output(d, "   %d) %s\r\n", i, position_types[i]);

         write_to_output(d, "Enter choice: ");
         OLC_MODE(d) = AEDIT_MIN_CHAR_POS;
         return;
       case '3':
         write_to_output(d, "Enter the minimum position the Victim has to be in to activate social:\r\n");
         for (i=POS_DEAD;i<=POS_STANDING;i++)
           write_to_output(d, "   %d) %s\r\n", i, position_types[i]);

         write_to_output(d, "Enter choice: ");
         OLC_MODE(d) = AEDIT_MIN_VICT_POS;
         return;
       case '4':
         write_to_output(d, "Enter new minimum level for social: ");
         OLC_MODE(d) = AEDIT_MIN_CHAR_LEVEL;
         return;
       case '5':
         OLC_ACTION(d)->hide = !OLC_ACTION(d)->hide;
         aedit_disp_menu(d);
         OLC_VAL(d) = 1;
         break;
       case 'a': case 'A':
         write_to_output(d, "Enter social shown to the Character when there is no argument supplied.\r\n"
                            "[OLD]: %s\r\n"
                            "[NEW]: ",
                 ((OLC_ACTION(d)->char_no_arg)?OLC_ACTION(d)->char_no_arg:"NULL"));
         OLC_MODE(d) = AEDIT_NOVICT_CHAR;
         return;
       case 'b': case 'B':
         write_to_output(d, "Enter social shown to Others when there is no argument supplied.\r\n"
                            "[OLD]: %s\r\n"
                            "[NEW]: ",
                            ((OLC_ACTION(d)->others_no_arg)?OLC_ACTION(d)->others_no_arg:"NULL"));
         OLC_MODE(d) = AEDIT_NOVICT_OTHERS;
         return;
       case 'c': case 'C':
         write_to_output(d, "Enter text shown to the Character when his victim isnt found.\r\n"
                            "[OLD]: %s\r\n"
                            "[NEW]: ",
                            ((OLC_ACTION(d)->not_found)?OLC_ACTION(d)->not_found:"NULL"));

         OLC_MODE(d) = AEDIT_VICT_NOT_FOUND;
         return;
       case 'd': case 'D':
         write_to_output(d, "Enter social shown to the Character when it is its own victim.\r\n"
                            "[OLD]: %s\r\n"
                            "[NEW]: ",
                            ((OLC_ACTION(d)->char_auto)?OLC_ACTION(d)->char_auto:"NULL"));

         OLC_MODE(d) = AEDIT_SELF_CHAR;
         return;
       case 'e': case 'E':
         write_to_output(d, "Enter social shown to Others when the Char is its own victim.\r\n"
                            "[OLD]: %s\r\n"
                            "[NEW]: ",
                            ((OLC_ACTION(d)->others_auto)?OLC_ACTION(d)->others_auto:"NULL"));

         OLC_MODE(d) = AEDIT_SELF_OTHERS;
         return;
       case 'f': case 'F':
         write_to_output(d, "Enter normal social shown to the Character when the victim is found.\r\n"
                            "[OLD]: %s\r\n"
                            "[NEW]: ",
                            ((OLC_ACTION(d)->char_found)?OLC_ACTION(d)->char_found:"NULL"));

         OLC_MODE(d) = AEDIT_VICT_CHAR_FOUND;
         return;
       case 'g': case 'G':
         write_to_output(d, "Enter normal social shown to Others when the victim is found.\r\n"
                            "[OLD]: %s\r\n"
                            "[NEW]: ",
                            ((OLC_ACTION(d)->others_found)?OLC_ACTION(d)->others_found:"NULL"));

         OLC_MODE(d) = AEDIT_VICT_OTHERS_FOUND;
         return;
       case 'h': case 'H':
         write_to_output(d, "Enter normal social shown to the Victim when the victim is found.\r\n"
                            "[OLD]: %s\r\n"
                            "[NEW]: ",
                            ((OLC_ACTION(d)->vict_found)?OLC_ACTION(d)->vict_found:"NULL"));

         OLC_MODE(d) = AEDIT_VICT_VICT_FOUND;
         return;
       case 'i': case 'I':
         write_to_output(d, "Enter 'body part' social shown to the Character when the victim is found.\r\n"
                            "[OLD]: %s\r\n"
                            "[NEW]: ",
                            ((OLC_ACTION(d)->char_body_found)?OLC_ACTION(d)->char_body_found:"NULL"));

         OLC_MODE(d) = AEDIT_VICT_CHAR_BODY_FOUND;
         return;
       case 'j': case 'J':
         write_to_output(d, "Enter 'body part' social shown to Others when the victim is found.\r\n"
                            "[OLD]: %s\r\n"
                            "[NEW]: ",
                            ((OLC_ACTION(d)->others_body_found)?OLC_ACTION(d)->others_body_found:"NULL"));

         OLC_MODE(d) = AEDIT_VICT_OTHERS_BODY_FOUND;
         return;
       case 'k': case 'K':
         write_to_output(d, "Enter 'body part' social shown to the Victim when the victim is found.\r\n"
                            "[OLD]: %s\r\n"
                            "[NEW]: ",
                            ((OLC_ACTION(d)->vict_body_found)?OLC_ACTION(d)->vict_body_found:"NULL"));

         OLC_MODE(d) = AEDIT_VICT_VICT_BODY_FOUND;
         return;
       case 'l': case 'L':
         write_to_output(d, "Enter 'object' social shown to the Character when the object is found.\r\n"
                            "[OLD]: %s\r\n"
                            "[NEW]: ",
                            ((OLC_ACTION(d)->char_obj_found)?OLC_ACTION(d)->char_obj_found:"NULL"));

         OLC_MODE(d) = AEDIT_OBJ_CHAR_FOUND;
         return;
       case 'm': case 'M':
         write_to_output(d, "Enter 'object' social shown to the Room when the object is found.\r\n"
                            "[OLD]: %s\r\n"
                            "[NEW]: ",
                            ((OLC_ACTION(d)->others_obj_found)?OLC_ACTION(d)->others_obj_found:"NULL"));

         OLC_MODE(d) = AEDIT_OBJ_OTHERS_FOUND;
         return;
       default:
         aedit_disp_menu(d);
         break;
      }
      return;

    case AEDIT_ACTION_NAME:
      if (!*arg || strchr(arg,' ')) {
        aedit_disp_menu(d);
        return;
      }
      if (OLC_ACTION(d)->command)
        free(OLC_ACTION(d)->command);
      OLC_ACTION(d)->command = strdup(arg);

      break;

    case AEDIT_SORT_AS:
      if (!*arg || strchr(arg,' ')) {
        aedit_disp_menu(d);
        return;
      }
      if (OLC_ACTION(d)->sort_as) 
        free(OLC_ACTION(d)->sort_as);
      OLC_ACTION(d)->sort_as = strdup(arg);
      
      break;

    case AEDIT_MIN_CHAR_POS:
    case AEDIT_MIN_VICT_POS:
      if (!*arg) {
        aedit_disp_menu(d);
        return;
      }
      /* atoi() answers 0 for a word, and 0 is POS_DEAD -- inside the range
       * tested below, so the range cannot refuse a typo on its own. */
      if (!isdigit(*arg) && (*arg != '-' || !isdigit(arg[1]))) {
        aedit_disp_menu(d);
        return;
      }
      i = atoi(arg);
      /* && can never hold: every integer typed here was accepted, and the
       * menu then rendered position_types[] at it. */
      if ((i < POS_DEAD) || (i > POS_STANDING))  {
        aedit_disp_menu(d);
        return;
      }
      if (OLC_MODE(d) == AEDIT_MIN_CHAR_POS)
        OLC_ACTION(d)->min_char_position = i;
      else
        OLC_ACTION(d)->min_victim_position = i;
      break;

    case AEDIT_MIN_CHAR_LEVEL:
      if (!*arg) {
        aedit_disp_menu(d);
        return;
      }
      /* Same two holes, with level 0 as what atoi() hands back for a word. */
      if (!isdigit(*arg) && (*arg != '-' || !isdigit(arg[1]))) {
        aedit_disp_menu(d);
        return;
      }
      i = atoi(arg);
      /* Same broken guard as the two position prompts above. */
      if ((i < 0) || (i > LVL_IMPL))  {
        aedit_disp_menu(d);
        return;
      }
      OLC_ACTION(d)->min_level_char = i;
      break;

    case AEDIT_NOVICT_CHAR:
      if (OLC_ACTION(d)->char_no_arg)
        free(OLC_ACTION(d)->char_no_arg);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->char_no_arg = strdup(arg);
      } else
        OLC_ACTION(d)->char_no_arg = NULL;
      break;

    case AEDIT_NOVICT_OTHERS:
      if (OLC_ACTION(d)->others_no_arg)
        free(OLC_ACTION(d)->others_no_arg);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->others_no_arg = strdup(arg);
      } else
        OLC_ACTION(d)->others_no_arg = NULL;
      break;

    case AEDIT_VICT_CHAR_FOUND:
      if (OLC_ACTION(d)->char_found)
        free(OLC_ACTION(d)->char_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->char_found = strdup(arg);
      } else
        OLC_ACTION(d)->char_found = NULL;
      break;

    case AEDIT_VICT_OTHERS_FOUND:
      if (OLC_ACTION(d)->others_found)
        free(OLC_ACTION(d)->others_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->others_found = strdup(arg);
      } else
        OLC_ACTION(d)->others_found = NULL;
      break;

    case AEDIT_VICT_VICT_FOUND:
      if (OLC_ACTION(d)->vict_found)
        free(OLC_ACTION(d)->vict_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->vict_found = strdup(arg);
      } else
        OLC_ACTION(d)->vict_found = NULL;
      break;

    case AEDIT_VICT_NOT_FOUND:
      if (OLC_ACTION(d)->not_found)
        free(OLC_ACTION(d)->not_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->not_found = strdup(arg);
      } else
        OLC_ACTION(d)->not_found = NULL;
      break;

    case AEDIT_SELF_CHAR:
      if (OLC_ACTION(d)->char_auto)
        free(OLC_ACTION(d)->char_auto);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->char_auto = strdup(arg);
      } else
        OLC_ACTION(d)->char_auto = NULL;
      break;

    case AEDIT_SELF_OTHERS:
      if (OLC_ACTION(d)->others_auto)
        free(OLC_ACTION(d)->others_auto);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->others_auto = strdup(arg);
      } else
        OLC_ACTION(d)->others_auto = NULL;
      break;

    case AEDIT_VICT_CHAR_BODY_FOUND:
      if (OLC_ACTION(d)->char_body_found)
        free(OLC_ACTION(d)->char_body_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->char_body_found = strdup(arg);
      } else
        OLC_ACTION(d)->char_body_found = NULL;
      break;

    case AEDIT_VICT_OTHERS_BODY_FOUND:
      if (OLC_ACTION(d)->others_body_found)
        free(OLC_ACTION(d)->others_body_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->others_body_found = strdup(arg);
      } else
        OLC_ACTION(d)->others_body_found = NULL;
      break;

    case AEDIT_VICT_VICT_BODY_FOUND:
      if (OLC_ACTION(d)->vict_body_found)
        free(OLC_ACTION(d)->vict_body_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->vict_body_found = strdup(arg);
      } else
        OLC_ACTION(d)->vict_body_found = NULL;
      break;

    case AEDIT_OBJ_CHAR_FOUND:
      if (OLC_ACTION(d)->char_obj_found)
        free(OLC_ACTION(d)->char_obj_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->char_obj_found = strdup(arg);
      } else
        OLC_ACTION(d)->char_obj_found = NULL;
      break;

    case AEDIT_OBJ_OTHERS_FOUND:
      if (OLC_ACTION(d)->others_obj_found)
        free(OLC_ACTION(d)->others_obj_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->others_obj_found = strdup(arg);
      } else
        OLC_ACTION(d)->others_obj_found = NULL;
      break;

    default:
      /* we should never get here */
      break;
   }
   OLC_VAL(d) = 1;
   aedit_disp_menu(d);
}

ACMD(do_astat)
{
  int i, real = FALSE;
  char arg[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if(!*arg) {
    send_to_char(ch, "Astat which social?\r\n");
    return;
  }

  for (i = 0; i <= top_of_socialt; i++) {
    if (is_abbrev(arg, soc_mess_list[i].command)) {
      real = TRUE;
      break;
    }
  }

  if (!real) {
    send_to_char(ch, "No such social.\r\n");
    return;
  }

   get_char_colors(ch);
   send_to_char(ch,
    "n) Command         : %s%-15.15s%s 1) Sort as Command : %s%-15.15s%s\r\n"
    "2) Min Position[CH]: %s%-8.8s%s        3) Min Position[VT]: %s%-8.8s%s\r\n"
    "4) Min Level   [CH]: %s%-3d%s             5) Show if Invis   : %s%s%s\r\n"
    "a) Char    [NO ARG]: %s%s%s\r\n"
    "b) Others  [NO ARG]: %s%s%s\r\n"
    "c) Char [NOT FOUND]: %s%s%s\r\n"
    "d) Char  [ARG SELF]: %s%s%s\r\n"
    "e) Others[ARG SELF]: %s%s%s\r\n"
    "f) Char      [VICT]: %s%s%s\r\n"
    "g) Others    [VICT]: %s%s%s\r\n"
    "h) Victim    [VICT]: %s%s%s\r\n"
    "i) Char  [BODY PRT]: %s%s%s\r\n"
    "j) Others[BODY PRT]: %s%s%s\r\n"
    "k) Victim[BODY PRT]: %s%s%s\r\n"
    "l) Char       [OBJ]: %s%s%s\r\n"
    "m) Others     [OBJ]: %s%s%s\r\n",

    yel, soc_mess_list[i].command, nrm,
    yel, soc_mess_list[i].sort_as, nrm,
    cyn, position_types[soc_mess_list[i].min_char_position], nrm,
    cyn, position_types[soc_mess_list[i].min_victim_position], nrm,
    cyn, soc_mess_list[i].min_level_char, nrm,
    cyn, (soc_mess_list[i].hide ? "HIDDEN" : "NOT HIDDEN"), nrm,
    cyn, soc_mess_list[i].char_no_arg ? soc_mess_list[i].char_no_arg : "", nrm,
    cyn, soc_mess_list[i].others_no_arg ? soc_mess_list[i].others_no_arg : "", nrm,
    cyn, soc_mess_list[i].not_found ? soc_mess_list[i].not_found : "", nrm,
    cyn, soc_mess_list[i].char_auto ? soc_mess_list[i].char_auto : "", nrm,
    cyn, soc_mess_list[i].others_auto ? soc_mess_list[i].others_auto : "", nrm,
    cyn, soc_mess_list[i].char_found ? soc_mess_list[i].char_found : "", nrm,
    cyn, soc_mess_list[i].others_found ? soc_mess_list[i].others_found : "", nrm,
    cyn, soc_mess_list[i].vict_found ? soc_mess_list[i].vict_found : "", nrm,
    cyn, soc_mess_list[i].char_body_found ? soc_mess_list[i].char_body_found : "", nrm,
    cyn, soc_mess_list[i].others_body_found ? soc_mess_list[i].others_body_found : "", nrm,
    cyn, soc_mess_list[i].vict_body_found ? soc_mess_list[i].vict_body_found : "", nrm,
    cyn, soc_mess_list[i].char_obj_found ? soc_mess_list[i].char_obj_found : "", nrm,
    cyn, soc_mess_list[i].others_obj_found ? soc_mess_list[i].others_obj_found : "", nrm);

}

static int aedit_find_command(const char *txt)
{
  int cmd;

  for (cmd = 1; *complete_cmd_info[cmd].command != '\n'; cmd++)
    if (!strncmp(complete_cmd_info[cmd].sort_as, txt, strlen(txt)) ||
        !strcmp(complete_cmd_info[cmd].command, txt))
      return (cmd);
  return (-1);
}

