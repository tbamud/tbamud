/**************************************************************************
*  File: mailedit.c                                        Part of tbaMUD *
*  Usage: Oasis OLC - Mudmail.                                            *
*                                                                         *
* Copyright 1996 Harvey Gilpin. 1997-2001 George Greer. 2010 Stefan Cole. *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "modify.h"
#include "mail.h"
#include "oasis.h"
#include "improved-edit.h"

/* External functions */
void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show);

/* external global variables */
extern const struct mail_group mail_groups[];

/*  utility functions */
ACMD(do_mailedit)
{
  struct descriptor_data *d;

  d = ch->desc;

  /* Give descriptor an OLC structure. */
  if (d->olc) {
    mudlog(BRF, ADMLVL_IMMORT, TRUE,
      "SYSERR: do_oasis_medit: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  OLC_ZNUM(d) = NOWHERE;
  OLC_NUM(d)  = 0;

  mailedit_setup(d);
  mailedit_disp_menu(d);

  /* Display messages to the players in the same room as the
     player and also log it. */
  act("$n starts editing mail.", TRUE, ch, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_MAILING);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
  STATE(d) = CON_MAILEDIT;

  mudlog(CMP, ADMLVL_IMMORT, TRUE,"MAIL: %s starts editing mudmail in room %d",
    GET_NAME(ch), world[IN_ROOM(d->character)].number);
}

/* No 'new' or 'existing' to edit - just set up the mudmail structures */
void mailedit_setup(struct descriptor_data *d)
{
  struct mail_edit_data *mail_info;

  /* Allocate a scratch mudmails structure. */
  CREATE(mail_info, struct mail_edit_data, 1);

  mail_info->mail       = create_mail();
  mail_info->recipients = NULL;

  OLC_MAIL(d) = mail_info;

  /* Has changed flag. (It hasn't so far, we just made it.) */
  OLC_VAL(d) = FALSE;

}

/* clear up memory used by the mudmail structures */
void mailedit_cleanup(struct descriptor_data *d)
{
  struct mail_edit_data *mail_info;
  struct mail_data *single_mail;
  struct obj_data *obj;

  mail_info = OLC_MAIL(d);
  single_mail = mail_info->mail;

  if (single_mail) {
    /* Any leftover attachments should be passed back to the owner */
    while (single_mail->attachment) {
      obj = single_mail->attachment;
      obj_from_mail(obj);
      obj_to_char(obj, d->character);
    }
    /* Any leftover gold is returned too */
    if ((single_mail->coins > 0) && !IS_SET_AR(single_mail->mail_flags, MAIL_COD)) {
      GET_GOLD(d->character) += single_mail->coins;
      single_mail->coins = 0;
	}
    if (single_mail->subject) free(single_mail->subject);
    if (single_mail->body)    free(single_mail->body);
    single_mail = NULL;
    free (mail_info->mail);
  }
  clear_recipients(mail_info);
  mail_info = NULL;
  free(OLC_MAIL(d));
  OLC_MAIL(d) = NULL;
}

/* Menu functions */
/* The 'main menu' - displays inbox, with options */
void mailedit_disp_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d, "tbaMUD Mail Editor\r\n");
  show_inbox_to_char(d->character);
  if (CONFIG_DRAFTS_ALLOWED)
    write_to_output(d, "%s(%sC%s)%s Create         %s(%sV%s)%s View          %s(%sR%s)%s Reply         %s(%sF%s)%s Forward       \r\n"
                       "%s(%sE%s)%s Receive        %s(%sD%s)%s Delete        %s(%sO%s)%s Outbox        %s(%sQ%s)%s Quit          \r\n",
                       cyn, yel, cyn, nrm,
                       cyn, yel, cyn, nrm,
                       cyn, yel, cyn, nrm,
                       cyn, yel, cyn, nrm,
                       cyn, yel, cyn, nrm,
                       cyn, yel, cyn, nrm,
                       cyn, yel, cyn, nrm,
                       cyn, yel, cyn, nrm );
  else
    write_to_output(d, "%s(%sC%s)%s Create         %s(%sV%s)%s View          %s(%sR%s)%s Reply         %s(%sF%s)%s Forward       \r\n"
                       "%s(%sE%s)%s Receive        %s(%sD%s)%s Delete        %s(%sQ%s)%s Quit          \r\n",
                       cyn, yel, cyn, nrm,
                       cyn, yel, cyn, nrm,
                       cyn, yel, cyn, nrm,
                       cyn, yel, cyn, nrm,
                       cyn, yel, cyn, nrm,
                       cyn, yel, cyn, nrm,
                       cyn, yel, cyn, nrm );
  write_to_output(d, "Enter Selection: ");

  OLC_MODE(d) = MAILEDIT_INBOX;
}

void mailedit_disp_outbox(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d, "tbaMUD Mail Editor\r\n");
  show_outbox_to_char(d->character);
  write_to_output(d, "%s(%sE%s)%s Edit      %s(%sV%s)%s View      %s(%sD%s)%s Delete      %s(%sI%s)%s Inbox     %s(%sQ%s)%s Quit\r\n",
                     cyn, yel, cyn, nrm,
                     cyn, yel, cyn, nrm,
                     cyn, yel, cyn, nrm,
                     cyn, yel, cyn, nrm,
                     cyn, yel, cyn, nrm );
  write_to_output(d, "Enter Selection: ");

  OLC_MODE(d) = MAILEDIT_OUTBOX;
}

void mailedit_disp_mail(struct descriptor_data *d)
{
  char att[15], gold[25];
  int gold_amt;

  get_char_colors(d->character);
  clear_screen(d);
  bool can_attach = TRUE;

  if (count_recipients(OLC_MAIL(d)) > 1) can_attach = FALSE;

  if (attachment_count(OLC_MAIL(d)->mail) > 0)
    sprintf(att, "%s%d object%s", can_attach ? "@r" : "@y",  attachment_count(OLC_MAIL(d)->mail), attachment_count(OLC_MAIL(d)->mail) > 1 ? "s" : "");
  else
    sprintf(att, "<None>");

  if (MAILEDIT_GOLD > 0) {
    sprintf(gold, "%s%s coins", can_attach ? "@r" : "@y", add_commas(MAILEDIT_GOLD));
  } else {
    sprintf(gold, "<None>");
  }

  gold_amt = CONFIG_STAMP_COST + (attachment_count(OLC_MAIL(d)->mail) * CONFIG_OBJECT_COST);

  write_to_output(d, "tbaMUD Mail Editor\r\n");
  if (GET_LEVEL(d->character) < CONFIG_FREE_MAIL_LEVEL) {
    write_to_output(d, "Current cost to send this mail: %s%d%s coins\r\n", yel, gold_amt, nrm);
  } else {
    write_to_output(d, "Current cost to send this mail: %sFree!%s\r\n", yel, nrm);
  }
  write_to_output(d, "%sA%s)%s Recipients  : %s%s\r\n"
                     "%sB%s)%s Subject     : %s%s\r\n"
                     "%sC%s)%s Urgency     : %s%s\r\n"
                     "%sD%s)%s Attachments : %s%s\r\n"
                     "%sE%s)%s Gold        : %s%s\r\n"
                     "%sF%s)%s Gold Mode   : %s%s\r\n"
                     "%sG%s)%s Body Text   : \r\n%s%s\r\n"
                     "%sS%s)%s Send Mail\r\n"
                     "%sX%s)%s Clear Mail data\r\n"
                     "%sQ%s)%s Quit without sending\r\n",
                     yel, cyn, nrm, yel, OLC_MAIL(d)->recipients ? recipient_list(OLC_MAIL(d)) : "<None!>",
                     yel, cyn, nrm, yel, MAILEDIT_SUBJECT == NULL ? "- No Subject -" : MAILEDIT_SUBJECT,
                     yel, cyn, nrm, yel, MAILEDIT_URGENT ? "@RUrgent!" : "Normal",
                     yel, cyn, CONFIG_CAN_MAIL_OBJ ? nrm : gry, CONFIG_CAN_MAIL_OBJ ? yel : gry, att,
                     yel, cyn, CONFIG_CAN_MAIL_GOLD ? nrm : gry, CONFIG_CAN_MAIL_GOLD ? yel : gry, gold,
                     yel, cyn, nrm, yel, MAILEDIT_COD ? "Cash-on-Delivery (COD)" : "Send Gold",
                     yel, cyn, nrm, yel, MAILEDIT_BODYTEXT == NULL ? "No Text" : MAILEDIT_BODYTEXT,
                     yel, cyn, nrm,
                     yel, cyn, nrm,
                     yel, cyn, nrm);

  write_to_output(d, "Enter Selection: ");

  OLC_MODE(d) = MAILEDIT_MAILEDIT;
}

void mailedit_disp_recipients(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d, "tbaMUD Mail Recipient Editor\r\n");
  write_to_output(d, "Current Recipient List:\r\n");
  write_to_output(d, "%s%s%s\r\n", yel, OLC_MAIL(d)->recipients ? recipient_list(OLC_MAIL(d)) : "<None!>", nrm);
  write_to_output(d, "%sA%s)%s Add a Recipient\r\n"
                     "%sB%s)%s Delete a Recipient\r\n"
                     "%sC%s)%s Clear all Recipients\r\n"
                     "%sQ%s)%s Quit (back to Editor)\r\n",
                     yel, cyn, nrm,
                     yel, cyn, nrm,
                     yel, cyn, nrm,
                     yel, cyn, nrm);
  write_to_output(d, "Enter Selection: ");

  OLC_MODE(d) = MAILEDIT_RECIP_MENU;
}

void mailedit_disp_attachments(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d, "tbaMUD Mail Attachment Editor\r\n");
  write_to_output(d, "Current Attached Objects:\r\n");
  list_obj_to_char((OLC_MAIL(d)->mail)->attachment, d->character, 1, TRUE);
  write_to_output(d, "\r\n");
  write_to_output(d, "%sA%s)%s Add an Object\r\n"
                     "%sB%s)%s Remove an Object\r\n"
                     "%sC%s)%s Remove all Objects\r\n"
                     "%sQ%s)%s Quit (back to Editor)\r\n",
                     yel, cyn, nrm,
                     yel, cyn, nrm,
                     yel, cyn, nrm,
                     yel, cyn, nrm);
  write_to_output(d, "Enter Selection: ");

  OLC_MODE(d) = MAILEDIT_ATTACH_MENU;
}

void mailedit_parse(struct descriptor_data *d, char *arg)
{
  int i, num;
  long mail_id, diff, gold;
  struct mail_data *ml;
  struct obj_data *obj;
  char *oldtext = NULL;

  switch (OLC_MODE(d)) {
    case MAILEDIT_ASK_DRAFT:
      switch (*arg) {
      case 'y':
      case 'Y':
        /* Save the mail as a draft. */
        save_as_draft(d->character, OLC_MAIL(d)->mail);
        clear_mail_data(OLC_MAIL(d)->mail);
        mailedit_disp_outbox(d);
        return;
      case 'n':
      case 'N':
        clear_mail_data(OLC_MAIL(d)->mail);
        mailedit_disp_menu(d);
        return;
      default:
        write_to_output(d, "Invalid choice!\r\n");
        write_to_output(d, "Do you wish to save this mail as a draft? (Y/N): ");
        return;
      }
      break;

    case MAILEDIT_ASK_QUIT:
      switch (*arg) {
      case 'y':
      case 'Y':
        clear_mail_data(OLC_MAIL(d)->mail);
        mailedit_disp_outbox(d);
        return;
      case 'n':
      case 'N':
        mailedit_disp_mail(d);
        return;
      default:
        write_to_output(d, "Invalid choice!\r\n");
        write_to_output(d, "Do you wish to quit and lose your changes? (Y/N): ");
        return;
      }
      break;

    case MAILEDIT_INBOX:
      switch (*arg) {
      case 'q':
      case 'Q':
        if (count_deleted(d->character) > 0) {

          write_to_output(d, "You have mail marked for deletion.\r\n");
          write_to_output(d, "Do you wish to purge all marked mails now? (Y/N)\r\n");
          OLC_MODE(d) = MAILEDIT_PURGE_N_QUIT;
          REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
          return;
        }
         REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
        cleanup_olc(d, CLEANUP_ALL);
        return;
      case 'c':
      case 'C':
        if ((GET_LEVEL(d->character) < CONFIG_FREE_MAIL_LEVEL) && (GET_GOLD(d->character) < CONFIG_STAMP_COST)) {
          write_to_output(d, "Mails cost %d coins, and you can't afford that!\r\n", CONFIG_STAMP_COST);
          mailedit_disp_menu(d);
          return;
		}
        clear_mail_data(OLC_MAIL(d)->mail);
        mailedit_disp_mail(d);
        break;
      case 'v':
      case 'V':
        write_to_output(d, "Which mail do you wish to view? : ");
        OLC_MODE(d) = MAILEDIT_VIEW;
        break;
      case 'r':
      case 'R':
        if ((GET_LEVEL(d->character) < CONFIG_FREE_MAIL_LEVEL) && (GET_GOLD(d->character) < CONFIG_STAMP_COST)) {
          write_to_output(d, "Mails cost %d coins, and you can't afford that!\r\n", CONFIG_STAMP_COST);
          mailedit_disp_menu(d);
          return;
		}
        write_to_output(d, "Which mail do you wish to reply to? : ");
        OLC_MODE(d) = MAILEDIT_REPLY;
        break;
      case 'f':
      case 'F':
        if ((GET_LEVEL(d->character) < CONFIG_FREE_MAIL_LEVEL) && (GET_GOLD(d->character) < CONFIG_STAMP_COST)) {
          write_to_output(d, "Mails cost %d coins, and you can't afford that!\r\n", CONFIG_STAMP_COST);
          mailedit_disp_menu(d);
          return;
		}
        write_to_output(d, "Which mail do you wish to forward? : ");
        OLC_MODE(d) = MAILEDIT_FORWARD;
        break;
      case 'd':
      case 'D':
        write_to_output(d, "Which mail do you wish to delete? : ");
        OLC_MODE(d) = MAILEDIT_DELETE;
        break;
      case 'e':
      case 'E':
        write_to_output(d, "Which mail do you wish to receive? : ");
        OLC_MODE(d) = MAILEDIT_RECEIVE;
        break;
      case 'o':
      case 'O':
        mailedit_disp_outbox(d);
        break;
      default:
        write_to_output(d, "Invalid Choice!\r\nEnter Selection : ");
        break;
      }
      break;

    case MAILEDIT_OUTBOX:
      switch (*arg) {
      case 'q':
      case 'Q':
        cleanup_olc(d, CLEANUP_ALL);
        return;
      case 'e':
      case 'E':
        if ((GET_LEVEL(d->character) < CONFIG_FREE_MAIL_LEVEL) && (GET_GOLD(d->character) < CONFIG_STAMP_COST)) {
          write_to_output(d, "Mails cost %d coins, and you can't afford that!\r\n", CONFIG_STAMP_COST);
          mailedit_disp_menu(d);
          return;
		}
        write_to_output(d, "Which mail do you wish to edit? : ");
        OLC_MODE(d) = MAILEDIT_OUTEDIT;
        break;
      case 'v':
      case 'V':
        write_to_output(d, "Which mail do you wish to view? : ");
        OLC_MODE(d) = MAILEDIT_OUTVIEW;
        break;
      case 'i':
      case 'I':
        mailedit_disp_menu(d);
        break;
      case 'd':
      case 'D':
        write_to_output(d, "Which mail do you wish to delete? : ");
        OLC_MODE(d) = MAILEDIT_OUTDELETE;
        break;
      default:
        write_to_output(d, "Invalid Choice!\r\nEnter Selection : ");
        break;
      }
      break;

    case MAILEDIT_VIEW:
      num = atoi(arg);
      if (num == 0) {
        mailedit_disp_menu(d);
        return;
      }
      mail_id = get_id_by_inbox_num(d->character, num);
      if (mail_id == NO_MAIL) {
        write_to_output(d, "Invalid mail number.\r\n");
        mailedit_disp_menu(d);
        return;
      }
      mail_view(d->character, mail_id);
      write_to_output(d, "-- Press Enter to Continue! --\r\n");
      OLC_MODE(d) = MAILEDIT_BACK_TO_MENU;
      break;

    case MAILEDIT_REPLY:
      num = atoi(arg);
      if (num == 0) {
        mailedit_disp_menu(d);
        return;
      }
      mail_id = get_id_by_inbox_num(d->character, num);
      if (mail_id == NO_MAIL) {
        write_to_output(d, "Invalid mail number.\r\n");
        mailedit_disp_menu(d);
        return;
      }
      if ((ml = create_mail()) != NULL) {
        load_mail(mail_id, ml);
        copy_mail(OLC_MAIL(d)->mail, ml, MAIL_COPY_REPLY);
        for (i=0; i<ML_ARRAY_MAX; i++)
          ml->mail_flags[i]     = 0;
        (OLC_MAIL(d)->mail)->mail_id   = NO_MAIL;
        (OLC_MAIL(d)->mail)->sender    = GET_ID(d->character);
        add_recipient(OLC_MAIL(d), ml->sender);
        mailedit_disp_mail(d);
      }
      break;

    case MAILEDIT_FORWARD:
      num = atoi(arg);
      if (num == 0) {
        mailedit_disp_menu(d);
        return;
      }
      mail_id = get_id_by_inbox_num(d->character, num);
      if (mail_id == NO_MAIL) {
        write_to_output(d, "Invalid mail number.\r\n");
        mailedit_disp_menu(d);
        return;
      }
      if ((ml = create_mail()) != NULL) {
        load_mail(mail_id, ml);
        copy_mail(OLC_MAIL(d)->mail, ml, MAIL_COPY_FORWARD);
        for (i=0; i<ML_ARRAY_MAX; i++)
          ml->mail_flags[i]     = 0;
        (OLC_MAIL(d)->mail)->mail_id   = NO_MAIL;
        (OLC_MAIL(d)->mail)->sender    = GET_ID(d->character);
        mailedit_disp_mail(d);
      }
      break;

    case MAILEDIT_DELETE:
      num = atoi(arg);
      if (num == 0) {
        mailedit_disp_menu(d);
        return;
      }
      mail_id = get_id_by_inbox_num(d->character, num);
      if (mail_id == NO_MAIL) {
        write_to_output(d, "Invalid mail number.\r\n");
        mailedit_disp_menu(d);
        return;
      }
      mail_delmark(d->character, mail_id);
      mailedit_disp_mail(d);
      break;

    case MAILEDIT_RECEIVE:
      num = atoi(arg);
      if (num == 0) {
        mailedit_disp_menu(d);
        return;
      }
      mail_id = get_id_by_inbox_num(d->character, num);
      if (mail_id == NO_MAIL) {
        write_to_output(d, "Invalid mail number.\r\n");
        mailedit_disp_menu(d);
        return;
      }
      if (mail_receive(d->character, mail_id))
        write_to_output(d, "Mail received into your inventory.\r\n");
      mailedit_disp_menu(d);
      break;

    case MAILEDIT_OUTEDIT:
      num = atoi(arg);
      if (num == 0) {
        mailedit_disp_outbox(d);
        return;
      }
      mail_id = get_id_by_outbox_num(d->character, num);
      if (mail_id == NO_MAIL) {
        write_to_output(d, "Invalid mail number.\r\n");
        mailedit_disp_outbox(d);
        return;
      }
      if ((ml = create_mail()) != NULL) {
        load_mail(mail_id, ml);
        copy_mail(OLC_MAIL(d)->mail, ml, MAIL_COPY_NORMAL);
        (OLC_MAIL(d)->mail)->sender    = GET_ID(d->character);
        mailedit_disp_mail(d);
      }
      break;

    case MAILEDIT_OUTVIEW:
      num = atoi(arg);
      if (num == 0) {
        mailedit_disp_menu(d);
        return;
      }
      mail_id = get_id_by_outbox_num(d->character, num);
      if (mail_id == NO_MAIL) {
        write_to_output(d, "Invalid mail number.\r\n");
        mailedit_disp_outbox(d);
        return;
      }
      mail_view(d->character, mail_id);
      write_to_output(d, "-- Press Enter to Continue! --\r\n");
      OLC_MODE(d) = MAILEDIT_BACK_TO_MENU;
      break;

    case MAILEDIT_OUTDELETE:
      num = atoi(arg);
      if (num == 0) {
        mailedit_disp_menu(d);
        return;
      }
      mail_id = get_id_by_outbox_num(d->character, num);
      if (mail_id == NO_MAIL) {
        write_to_output(d, "Invalid mail number.\r\n");
        mailedit_disp_outbox(d);
        return;
      }
      mail_delmark(d->character, mail_id);
      mailedit_disp_mail(d);
      break;

    case MAILEDIT_BACK_TO_MENU:
      mailedit_disp_menu(d);
      break;

    case MAILEDIT_RECIP_MENU:
      switch (*arg) {
        case 'a':
        case 'A':
          /* Mail with attachments can only have one recipient */
          if (count_recipients(OLC_MAIL(d)) > 0) {
            if (((OLC_MAIL(d)->mail)->attachment) || ((OLC_MAIL(d)->mail)->coins > 0 && !IS_SET_AR((OLC_MAIL(d)->mail)->mail_flags, MAIL_COD))) {
              write_to_output(d, "Sorry, mail with attachments cannot be sent to more than one person!\r\n");
              mailedit_disp_recipients(d);
              return;
            }
          }
          write_to_output(d, "Enter a player name to add: ");
          OLC_MODE(d) = MAILEDIT_ADD_RECIP;
          break;
        case 'b':
        case 'B':
          write_to_output(d, "Enter a player name to remove: ");
          OLC_MODE(d) = MAILEDIT_DEL_RECIP;
          break;
        case 'c':
        case 'C':
          write_to_output(d, "Are you sure you wish to remove ALL recipients? (Y/N): ");
          OLC_MODE(d) = MAILEDIT_CLR_RECIP;
          break;
        case 'q':
        case 'Q':
          mailedit_disp_mail(d);
          break;
      }
      break;

    case MAILEDIT_ADD_RECIP:
      /* check 'special cases' (mail groups) */
      if ((mail_id = get_mail_group_by_name(arg)) == MAIL_TO_NOBODY) {
        if ((mail_id = get_id_by_name(arg)) < 0 || !mail_recip_ok(arg)) {
          write_to_output(d, "No one by that name is registered here!\r\n");
          mailedit_disp_recipients(d);
          return;
        }
      }
      if (mail_id == MAIL_TO_ALL && GET_LEVEL(d->character) < CONFIG_MIN_SEND_TO_ALL) {
        write_to_output(d, "Sorry, you don't have sufficient access to send to 'all'\r\n");
        mailedit_disp_recipients(d);
        return;
      }
      add_recipient(OLC_MAIL(d), mail_id);
      mailedit_disp_recipients(d);
      break;

    case MAILEDIT_DEL_RECIP:
      if ((mail_id = get_id_by_name(arg)) < 0 || !mail_recip_ok(arg)) {
        write_to_output(d, "No one by that name is registered here!\r\n");
        mailedit_disp_recipients(d);
        return;
      }
      remove_recipient(OLC_MAIL(d), mail_id);
      mailedit_disp_recipients(d);
      break;

    case MAILEDIT_CLR_RECIP:
      switch (*arg) {
        case 'y':
        case 'Y':
          clear_recipients(OLC_MAIL(d));
          mailedit_disp_recipients(d);
          break;
        case 'n':
        case 'N':
          mailedit_disp_recipients(d);
          break;
        default : write_to_output(d, "Invalid choice!\r\nAre you sure you wish to remove ALL recipients? (Y/N): ");
                  break;
      }
      break;

    case MAILEDIT_ATTACH_MENU:
      switch (*arg) {
        case 'a':
        case 'A':
          /* Mail with attachments can only have one recipient */
          if (count_recipients(OLC_MAIL(d)) > 1) {
            write_to_output(d, "Sorry, mail with multiple recipients cannot have attachments!\r\n");
            mailedit_disp_attachments(d);
            return;
          }
          if ((d->character)->carrying == NULL) {
            write_to_output(d, "Your inventory seems to be empty - you cannot attach anything.\r\n");
            mailedit_disp_attachments(d);
            return;
          }
          list_attachments_numbered((d->character)->carrying, d->character);
          write_to_output(d, "Enter a object to add: ");
          OLC_MODE(d) = MAILEDIT_ADD_ATTACH;
          break;
        case 'b':
        case 'B':
          if ((OLC_MAIL(d)->mail)->attachment == NULL) {
            write_to_output(d, "There are no attachments - you cannot remove anything.\r\n");
            mailedit_disp_attachments(d);
            return;
          }
          list_attachments_numbered((OLC_MAIL(d)->mail)->attachment, d->character);
          write_to_output(d, "Enter an object to remove: ");
          OLC_MODE(d) = MAILEDIT_DEL_ATTACH;
          break;
        case 'c':
        case 'C':
          write_to_output(d, "Are you sure you wish to remove ALL objects? (Y/N): ");
          OLC_MODE(d) = MAILEDIT_CLR_ATTACH;
          break;
        case 'q':
        case 'Q':
          mailedit_disp_mail(d);
          break;
      }
      break;

    case MAILEDIT_ADD_ATTACH:
      num = atoi(arg);
      if ((obj = get_attachment_numbered(d->character, (d->character)->carrying, num)) == NULL) {
        write_to_output(d, "That object number is not in your inventory\r\n");
        mailedit_disp_attachments(d);
        return;
      }
      obj_from_char(obj);
      obj_to_mail(obj, OLC_MAIL(d)->mail);
      mailedit_disp_attachments(d);
      break;

    case MAILEDIT_DEL_ATTACH:
      num = atoi(arg);
      if ((obj = get_attachment_numbered(d->character, (OLC_MAIL(d)->mail)->attachment, num)) == NULL) {
        write_to_output(d, "That object number is not attached.\r\n");
        mailedit_disp_attachments(d);
        return;
      }
      obj_from_mail(obj);
      obj_to_char(obj, d->character);
      mailedit_disp_attachments(d);
      break;

    case MAILEDIT_CLR_ATTACH:
      switch (*arg) {
        case 'y':
        case 'Y':
          while ((OLC_MAIL(d)->mail)->attachment) {
            obj = (OLC_MAIL(d)->mail)->attachment;
            obj_from_mail(obj);
            obj_to_char(obj, d->character);
          }
          mailedit_disp_attachments(d);
          break;
        case 'n':
        case 'N':
          mailedit_disp_attachments(d);
          break;
        default : write_to_output(d, "Invalid choice!\r\nAre you sure you wish to remove ALL attachments? (Y/N): ");
                  break;
      }
      break;

    case MAILEDIT_GET_GOLD:
      gold = atol(arg);
      diff = (gold - (OLC_MAIL(d)->mail)->coins);
      if (diff > GET_GOLD(d->character)) {
        write_to_output(d, "@RYou don't have that much gold!@n\r\n");
        mailedit_disp_mail(d);
        return;
      }
      GET_GOLD(d->character) -= diff;
      (OLC_MAIL(d)->mail)->coins = gold;
      mailedit_disp_mail(d);
      break;

    case MAILEDIT_GET_SUBJECT:
      if ((OLC_MAIL(d)->mail)->subject)
        free ((OLC_MAIL(d)->mail)->subject);
      (OLC_MAIL(d)->mail)->subject = strdup(arg);
      mailedit_disp_mail(d);
      break;

    case MAILEDIT_MAILEDIT:
      switch (*arg) {
      case 'q':
      case 'Q':
        if (CONFIG_DRAFTS_ALLOWED) {
          write_to_output(d, "Do you wish to save this mail as a draft? (Y/N): ");
          OLC_MODE(d) = MAILEDIT_ASK_DRAFT;
        } else {
          write_to_output(d, "Do you wish to quit and lose your changes? (Y/N): ");
          OLC_MODE(d) = MAILEDIT_ASK_QUIT;
        }
        break;
      case 'a':
      case 'A':
        mailedit_disp_recipients(d);
        break;
      case 'b':
      case 'B':
        write_to_output(d, "Enter a subject for this mail: ");
        OLC_MODE(d) = MAILEDIT_GET_SUBJECT;
        break;
      case 'c':
      case 'C':
        TOGGLE_BIT_AR((OLC_MAIL(d)->mail)->mail_flags, MAIL_URGENT);
        mailedit_disp_mail(d);
        break;
      case 'd':
      case 'D':
        if (CONFIG_CAN_MAIL_OBJ) {
          mailedit_disp_attachments(d);
        } else {
          write_to_output(d, "@RSorry, object attachments are disabled!@n\r\n");
          mailedit_disp_mail(d);
        }
        break;
      case 'e':
      case 'E':
        if (CONFIG_CAN_MAIL_GOLD) {
          write_to_output(d, "Enter a number of gold coins: ");
          OLC_MODE(d) = MAILEDIT_GET_GOLD;
        } else {
          write_to_output(d, "@RSorry, gold attachments are disabled!@n\r\n");
          mailedit_disp_mail(d);
        }
        break;
      case 'f':
      case 'F':
        /* Flicking between COD and 'send money', we need to give/take gold from the player */
        if (IS_SET_AR((OLC_MAIL(d)->mail)->mail_flags, MAIL_COD) && ((OLC_MAIL(d)->mail)->coins > 0)) {
          diff = (OLC_MAIL(d)->mail)->coins;
          if (diff > GET_GOLD(d->character)) {
            write_to_output(d, "@RYou don't have enough gold to do that!@n\r\n");
            mailedit_disp_mail(d);
            return;
          }
          GET_GOLD(d->character) -= diff;
		} else if (!IS_SET_AR((OLC_MAIL(d)->mail)->mail_flags, MAIL_COD) && ((OLC_MAIL(d)->mail)->coins > 0)) {
          diff = (OLC_MAIL(d)->mail)->coins;
          GET_GOLD(d->character) += diff;
		}
        TOGGLE_BIT_AR((OLC_MAIL(d)->mail)->mail_flags, MAIL_COD);
        mailedit_disp_mail(d);
        break;
      case 'g':
      case 'G':
        OLC_MODE(d) = MAILEDIT_GET_BODYTEXT;
        send_editor_help(d);
        write_to_output(d, "Enter the main body text for this mail:\r\n\r\n");
        if ((OLC_MAIL(d)->mail)->body) {
          write_to_output(d, "%s", (OLC_MAIL(d)->mail)->body);
          oldtext = strdup((OLC_MAIL(d)->mail)->body);
        }
        string_write(d, &(OLC_MAIL(d)->mail)->body, MAX_MAIL_SIZE, 0, oldtext);
        break;
      case 's':
      case 'S':
        if (GET_LEVEL(d->character) < CONFIG_FREE_MAIL_LEVEL) {
          gold = CONFIG_STAMP_COST + (attachment_count(OLC_MAIL(d)->mail) * CONFIG_OBJECT_COST);
        } else {
          gold = 0;
        }
        if (gold > 0 && GET_GOLD(d->character) < gold ) {
          write_to_output(d, "This mail costs %ld coins, and you can't afford that!\r\n", gold );
          mailedit_disp_mail(d);
          return;
		}
        (OLC_MAIL(d)->mail)->sender = GET_ID(d->character);
        if (perform_send_edited(d->character, OLC_MAIL(d)) == FALSE) {
          write_to_output(d, "@RERROR: Unable to send Mail: Please tell an Imm!@n\r\n");
          /* Sending failed - return to editor, so they can choose to save as draft */
          mailedit_disp_mail(d);
          return;
        }
        /* Sending was successful - clean up */
		if (gold) GET_GOLD(d->character) -= gold;
        clear_mail_data(OLC_MAIL(d)->mail);
        clear_recipients(OLC_MAIL(d));
        write_to_output(d, "Sending of mail successful!  Press Enter to continue.\r\n");
        OLC_MODE(d) = MAILEDIT_BACK_TO_MENU;
        break;
      case 'x':
      case 'X':
        clear_mail_data(OLC_MAIL(d)->mail);
        mailedit_disp_mail(d);
        break;
      default:
        write_to_output(d, "Invalid Choice!\r\nEnter Selection : ");
        break;
      }
      break;

    case MAILEDIT_PURGE_N_QUIT:
      switch (*arg) {
        case 'y':
        case 'Y':
          num = purge_marked_mail(d->character);
          if (num > 0) {
            write_to_output(d, "%d deleted mail%s ha%s been purged.", num, num == 1 ? "" : "s", num == 1 ? "s" : "ve");
          }
          cleanup_olc(d, CLEANUP_ALL);
          break;
        case 'n':
        case 'N':
          cleanup_olc(d, CLEANUP_ALL);
          break;
        default : write_to_output(d, "Invalid choice!\r\nDo you wish to purge all marked mails now? (Y/N): ");
                  break;
      }
      break;

    default:
      write_to_output(d, "Sorry - There appears to be a problem, returning to inbox.\r\n");
      log("SYSERR: mailedit: Invalid submode (%d)", OLC_MODE(d));
      mailedit_disp_menu(d);
      break;
  }
}

void mailedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {
  case MAILEDIT_GET_BODYTEXT:
    mailedit_disp_mail(d);
    break;
  }
}


