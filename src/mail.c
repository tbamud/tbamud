/**************************************************************************
*  File: mail.c                                            Part of tbaMUD *
*  Usage: Internal funcs and player spec-procs of mudmail system.         *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*  By Jeremy Elson. Rewritten by Welcor and Jamdog                        *
**************************************************************************/

#include <sys/stat.h>
#include <dirent.h>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "modify.h"
#include "act.h"
#include "genolc.h"
#include "dg_scripts.h"
#include "mail.h"
#include "screen.h"

/* Local Global Variables */
struct mail_index *mail_table = NULL;  /* mail_table is index - mails are loaded as necessary */
int top_of_mail_table=-1;
int top_mail_idnum=-1;

/* Also need an adjustment to perform_send_to_groups below */
const struct mail_group mail_groups[NUM_MAIL_GROUPS+1] = {
    {"immortals", MAIL_TO_IMMS},
    {"all",       MAIL_TO_ALL},
    {"\n",        MAIL_TO_NOBODY}};

/* build_mail_index: loads the mudmail index */
bool build_mail_index(void)
{
  int rec_count = 0, i, count=0;
  FILE *ml_index;
  char index_name[40], line[256], bits[64];
  char subj[80], *strptr;

  sprintf(index_name, "%s%s", LIB_MLFILES, INDEX_FILE);
  if (!(ml_index = fopen(index_name, "r"))) {
    top_of_mail_table = -1;
    log("No mail index file!  Creating new one!");
    return (save_mail_index());
  }

  while (get_line(ml_index, line))
    if (*line != '~' && *line != '$' && *line != '#')  /* Index file can end in ~ or $ */
      rec_count++;
  rewind(ml_index);

  if (rec_count == 0) {
    mail_table = NULL;
    top_of_mail_table = -1;
    return TRUE;
  }

  CREATE(mail_table, struct mail_index, rec_count);
  for (i = 0; i < rec_count; i++) {
    get_line(ml_index, line);
    while (*line && *line == '#') get_line(ml_index, line);  /* Skip comments */
    if ((sscanf(line, "%ld %s %ld %ld %ld %s", &mail_table[i].mail_id, bits, &mail_table[i].sender,
                                               &mail_table[i].recipient, &mail_table[i].sent_time, subj)) != 6) {

    }
    if (!strcmp(subj, "(null)")) {
      mail_table[i].subject = NULL;
    } else {
      count = 0;
	  /* sscanf only grabs the first word of the subject, get it all! */
	  for (strptr=line; strptr && *strptr && count<5; strptr++)
	    if (*strptr == ' ') count++;
	  if (strptr && *strptr) {
        mail_table[i].subject = strdup(strptr);
      } else {
        mail_table[i].subject = NULL;
      }
	}
    mail_table[i].flags = asciiflag_conv(bits);
    top_mail_idnum = MAX(top_mail_idnum, mail_table[i].mail_id);
  }

  fclose(ml_index);
  top_of_mail_table = i - 1;
  return TRUE;
}


/* Find an usused unique mail ID */
long new_mail_id(void)
{
  int i,j;

  /* Have we allocated all mail numbers yet? */
  if (top_mail_idnum < MAX_MAIL_ID) {
	top_mail_idnum++;
	return (top_mail_idnum);
  }

  /* Oops, we reached the max!
     check through all the other numbers to find one that has been deleted */
  for (i=0; i<MAX_MAIL_ID; i++) {
    /* loop through mail index table until id is found */
    for (j=0; j<=top_of_mail_table && mail_table[j].mail_id != i; j++) {
      /* We got to the end without finding it - it doesn't exist */
      if (j == (top_of_mail_table - 1)) return i;
    }
  }
  /* wasn't found - return -1 */
  return NO_MAIL;
}

void free_mail_index(void)
{
  int tm;

  if (!mail_table)
    return;

  for (tm = 0; tm <= top_of_mail_table; tm++)
    if (mail_table[tm].subject)
      free(mail_table[tm].subject);

  free(mail_table);
  mail_table = NULL;
  top_of_mail_table = -1;
}


void extract_mail(struct mail_data *mail)
{
  struct obj_data *obj, *next_obj;

  if (mail->subject)
    free(mail->subject);

  if (mail->body)
    free(mail->body);

  /* Extract all attached objects */
  if (mail->attachment) {
    obj = mail->attachment;
    next_obj = obj->next_content;
    while (obj) {
      extract_obj(obj);
      obj = next_obj;
    }
  }

  free(mail);
}

int find_mail_by_id(long mail_id)
{
  int i;

  for (i=0; i<=top_of_mail_table; i++)
    if (mail_table[i].mail_id == mail_id)
      return i;

  /* Not found */
  return NO_MAIL;
}

void copy_mail_index_entry(struct mail_index *to, struct mail_index *from)
{
//  to->mail_id   = from->mail_id;
  to->recipient = from->recipient;
  to->sender    = from->sender;
  to->sent_time = from->sent_time;
  to->flags     = from->flags;
  if (from->subject)
    to->subject   = strdup(from->subject);
  else
    to->subject   = NULL;
}

void copy_mail(struct mail_data *to, struct mail_data *from, int copy_mode)
{
  int i;
  char new_subject[MAX_STRING_LENGTH];

//  to->mail_id   = from->mail_id;
  to->recipient = from->recipient;
  to->sender    = from->sender;
  to->sent_time = from->sent_time;

  switch (copy_mode) {
    case MAIL_COPY_NORMAL  : if (from->subject)
                               sprintf(new_subject, "%s", from->subject);
                             else
                               *new_subject='\0';
                             break;

    case MAIL_COPY_FORWARD : if (from->subject)
                               sprintf(new_subject, "FW: %s", from->subject);
                             else
                               sprintf(new_subject, "FW: - No Subject -");
                             break;

    case MAIL_COPY_REPLY   : if (from->subject)
                               sprintf(new_subject, "RE: %s", from->subject);
                             else
                               sprintf(new_subject, "RE: - No Subject -");
                             break;

    default:                 log("Invalid copy mode passed to mail_copy.");
                             *new_subject = '\0';
                             break;
  }

  for (i=0; i<ML_ARRAY_MAX; i++)
    to->mail_flags[i]     = from->mail_flags[i];

  if (*new_subject)
    to->subject   = strdup(new_subject);
  else
    to->subject   = NULL;

  if (from->body)
    to->body   = strdup(from->body);
  else
    to->body   = NULL;

  /* Attachments cannot be copied */
  to->attachment = NULL;
  to->coins      = 0;
}

int create_mail_index_entry(struct mail_data *mail)
{
  int i, pos;

  /* Is this mail already listed? */
  if (mail->mail_id != NO_MAIL && find_mail_by_id(mail->mail_id) != NO_MAIL) {
    log("SYSERR: create_mail_index_entry called for mail that already has an index entry (id=%ld)", mail->mail_id);
    return NO_MAIL;
  }

  if (top_of_mail_table < 0) {	/* no table */
    pos = top_of_mail_table = 0;
    CREATE(mail_table, struct mail_index, 1);
  } else {	/* add mail */
    i = ++top_of_mail_table + 1;

    RECREATE(mail_table, struct mail_index, i);
    pos = top_of_mail_table;
  }

  /* Allocate a new ID number */
  if ((mail_table[pos].mail_id = mail->mail_id = new_mail_id()) == NO_MAIL) {
    log("SYSERR: Unable to allocate a unique mail ID for new mudmails");
    return NO_MAIL;
  }

  /* copy subject and other vars */
  if (mail->subject && *(mail->subject)) {
    mail_table[pos].subject = strdup(mail->subject);
  } else {
    mail_table[pos].subject = NULL;
  }
  mail_table[pos].recipient = mail->recipient;
  mail_table[pos].sender    = mail->sender;
  mail_table[pos].sent_time = mail->sent_time;

  /* clear the bitflag in case we have garbage data */
  mail_table[pos].flags = 0;

  /* set urgent or deleted flag if required, and flag if object or gold attached */
  if (IS_SET_AR(mail->mail_flags, MAIL_URGENT))  SET_BIT(mail_table[pos].flags, MINDEX_URGENT);
  if (IS_SET_AR(mail->mail_flags, MAIL_DELETED)) SET_BIT(mail_table[pos].flags, MINDEX_DELETED);
  if (IS_SET_AR(mail->mail_flags, MAIL_COD))     SET_BIT(mail_table[pos].flags, MINDEX_IS_COD);
  if (mail->attachment != NULL) SET_BIT(mail_table[pos].flags, MINDEX_HAS_OBJ);
  if (mail->coins > 0)          SET_BIT(mail_table[pos].flags, MINDEX_HAS_GOLD);

  return (pos);
}

bool delete_mail_index_entry(int mail_id)
{
  int i, found=0;
  struct mail_index *temp_mail;
  bool ret;

  if (top_of_mail_table == -1 || mail_table == NULL) {	/* no table */
    /* There is no table - this function shouldn't even have been called */
    log("SYSERR: Attempt to delete mail id %d, but no mail index exists", mail_id);
    return FALSE;
  } else {	/* create a new smaller table */
    CREATE(temp_mail, struct mail_index, top_of_mail_table);
  }

  /* Copy all index entries to new table */
  for (i=0; i<=top_of_mail_table; i++) {
    /* Is this the one to be removed? */
    if (mail_table[i].mail_id == mail_id) {
      /* Do not copy - just mark as 'found' */
      found++;
    } else {
      /* Not the one to be removed - copy over the data */
      copy_mail_index_entry(&(temp_mail[i-found]), &(mail_table[i]));
    }
  }
  /* Freeing the index loses other vars, so back them up now */
  i = top_of_mail_table - 1;
  found = top_mail_idnum;

  /* All done - free up the old index, and then set the new index as the current one */
  free_mail_index();
  mail_table = temp_mail;

  /* Restore vars */
  top_of_mail_table = i;
  top_mail_idnum = found;

  /* And save the new index */
  ret = save_mail_index();

  return ret;
}

/* This function necessary to save a seperate ASCII mail index */
bool save_mail_index(void)
{
  int i;
  char index_name[50], bits[64];
  FILE *index_file;

  sprintf(index_name, "%s%s", LIB_MLFILES, INDEX_FILE);
  if (!(index_file = fopen(index_name, "w"))) {
    log("SYSERR: Could not write mudmail index file (%s)", index_name);
    return FALSE;
  }

  fprintf(index_file, "# tbaMUD mail index\n");
  fprintf(index_file, "# Format: <ID> <flags> <sender ID/vnum> <recipient ID> <time> <subject>\n");
  fprintf(index_file, "# For 'No Subject', use (null)\n");
  for (i = 0; i <= top_of_mail_table; i++) {
    if (mail_table[i].subject && *mail_table[i].subject) {
      sprintascii(bits, mail_table[i].flags);
      fprintf(index_file, "%ld %s %ld %ld %ld %s\n", mail_table[i].mail_id, *bits ? bits : "0",
      mail_table[i].sender, mail_table[i].recipient,
      mail_table[i].sent_time, mail_table[i].subject);
    } else {
      sprintascii(bits, mail_table[i].flags);
      fprintf(index_file, "%ld %s %ld %ld %ld (null)\n", mail_table[i].mail_id, *bits ? bits : "0",
      mail_table[i].sender, mail_table[i].recipient,
      mail_table[i].sent_time);
    }
  }
  fprintf(index_file, "~\n");

  fclose(index_file);

  return TRUE;
}

int handle_mail_obj(struct obj_data *temp, struct mail_data *ml)
{
  if (!temp || !ml)  /* this should never happen, but.... */
    return FALSE;

  obj_to_mail(temp, ml);

  return TRUE;
}

bool check_mail_dir(long mail_id)
{
#ifndef CIRCLE_UNIX
#define stat _stat
#endif
  struct stat buffer;
  char dirname[PATH_MAX];

  sprintf(dirname, "%s%ld", LIB_MLFILES, (mail_id - (mail_id % 100))/100);  // 1234-> plrmail/12

  if (stat(dirname, &buffer) == 0)
  {
#ifdef CIRCLE_UNIX
    if (S_ISDIR(buffer.st_mode))
#else
    if (buffer.st_mode & _S_IFDIR)
#endif
      return TRUE;   // exists, and is a dir
    else
      return FALSE;  // exists, but not a dir!
  }

  if (errno == ENOENT)
  {
#ifdef CIRCLE_UNIX
    if (mkdir(dirname, S_IRWXU) == 0)
#else
    if (_mkdir(dirname) == 0)
#endif
      return TRUE;

    perror("Unable to create new directory");
    return FALSE;
  }
  perror("Other problem with finding dir");
  return FALSE;
}
#ifndef CIRCLE_UNIX
#undefine stat
#endif

bool save_as_draft(struct char_data *ch, struct mail_data *ml)
{
  bool ret;

  /* Set the draft flag, and other essential data */
  SET_BIT_AR(ml->mail_flags, MAIL_DRAFT);
  ml->sender = GET_ID(ch);

  ret = save_mail(ml);

  return ret;
}

bool save_mail(struct mail_data *ml)
{
  FILE *fl;
  char fname[80], buf[MAX_STRING_LENGTH], bits[127], bits2[127], bits3[127], bits4[127];
  long mail_id;
  struct obj_data *obj, *next_obj;
  bool index_change = FALSE;

  /* Does this mail require a new ID, or is it re-saving an old one? */
  if (ml->mail_id == NO_MAIL) {
    /* Create an index entry and unique ID for this new mail */
    if (create_mail_index_entry(ml) == NO_MAIL) {
      log("SYSERR: Unable to save new mail in mail file: aborting!");
      return FALSE;
    }
    index_change = TRUE;
  }
  mail_id = ml->mail_id;

  if (!check_mail_dir(mail_id)) {
    log("SYSERR: Unable to save mail (%ld) due to folder error: aborting!", mail_id);
    /* Remove the index entry on failure */
    if (index_change) delete_mail_index_entry(mail_id);
    return FALSE;
  }
  /* Construct the filename */
  snprintf(fname, sizeof(fname), "%s%ld%s%ld.%s", LIB_MLFILES, (mail_id - (mail_id % 100))/100, SLASH, mail_id, SUF_MAIL);

  if (!(fl = fopen(fname, "w"))) {
    mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't open mail file %s for write", fname);
    /* Remove the index entry on failure */
    if (index_change) delete_mail_index_entry(mail_id);
    return FALSE;
  }
  /* Write tha actual data */
  fprintf(fl, "MlID: %ld\n", ml->mail_id);
  fprintf(fl, "Send: %ld\n", ml->sender);
  fprintf(fl, "Reci: %ld\n", ml->recipient);
  fprintf(fl, "Sent: %ld\n", ml->sent_time);
  if (ml->subject && *ml->subject) {
    strcpy(buf, ml->subject);
    strip_cr(buf);
    fprintf(fl, "Subj: %s\n", buf);
  }
  if (ml->body && *ml->body) {
    strcpy(buf, ml->body);
    fprintf(fl, "Body:\n%s~\n", buf);
  }
  fprintf(fl, "Gold: %d\n", (ml->coins > 0) ? ml->coins : 0);

  sprintascii(bits,  ml->mail_flags[0]);
  sprintascii(bits2, ml->mail_flags[1]);
  sprintascii(bits3, ml->mail_flags[2]);
  sprintascii(bits4, ml->mail_flags[3]);
  fprintf(fl, "Flag: %s %s %s %s\n", bits, bits2, bits3, bits4);

  /* Save all attached objects */
  if (ml->attachment) {
    obj = ml->attachment;
    next_obj = obj->next_content;
    fprintf(fl, "Objs:\n");
    while (obj) {
      if (!objsave_save_obj_record(obj, fl, 0)) {
        log("SYSERR: Unable to save attachments for mail %ld", mail_id);
      }
      /* No need to extract_obj(obj); - the save function should have already done it */
      obj = next_obj;
    }
  }
  /* EOF */
  fprintf(fl, "$~\n");

  fclose(fl);

  if (index_change) save_mail_index();

  return TRUE;
}

bool load_mail(long mail_id, struct mail_data *ml)
{
  FILE *fl;
  char fname[80], buf[MAX_STRING_LENGTH], line[MAX_INPUT_LENGTH + 1], tag[6];
  char f1[128], f2[128], f3[128], f4[128];
  obj_save_data *loaded, *current;
  int num_objs=0;

  /* Create an index entry and unique ID for this new mail */
  if (find_mail_by_id(mail_id) == NO_MAIL) {
    log("SYSERR: Unable to load mail from file: invalid ID (%ld)", mail_id);
    return FALSE;
  }

  if (!check_mail_dir(mail_id)) {
    log("SYSERR: Unable to load mail (%ld) due to folder error: aborting!", mail_id);
    return FALSE;
  }
  /* Construct the filename */
  snprintf(fname, sizeof(fname), "%s%ld%s%ld.%s", LIB_MLFILES, (mail_id - (mail_id % 100))/100, SLASH, mail_id, SUF_MAIL);

  if (!(fl = fopen(fname, "r"))) {
    mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't open mail file %s for reading.", fname);
    return FALSE;
  }

  while (get_line(fl, line)) {
    tag_argument(line, tag);
    switch (*tag) {
      case 'A':
              if (!strcmp(tag, "Atta"))	{
                loaded = objsave_parse_objects(fl);
                for (current = loaded; current != NULL; current=current->next)
                  num_objs += handle_mail_obj(current->obj, ml);
			  }
         break;
      case 'B':
              if (!strcmp(tag, "Body"))	ml->body = fread_string(fl, buf);
         break;
      case 'F':
              if (!strcmp(tag, "Flag")) {
                if (sscanf(line, "%s %s %s %s", f1, f2, f3, f4) == 4) {
                  ml->mail_flags[0] = asciiflag_conv(f1);
                  ml->mail_flags[1] = asciiflag_conv(f2);
                  ml->mail_flags[2] = asciiflag_conv(f3);
                  ml->mail_flags[3] = asciiflag_conv(f4);
                } else
                  log("SYSERR: Invalid mail flags in file %s", fname);
                }

         break;
      case 'G':
              if (!strcmp(tag, "Gold"))	ml->coins = atoi(line);
         break;
      case 'M':
              if (!strcmp(tag, "MlID"))	ml->mail_id = atol(line);
         break;
      case 'R':
              if (!strcmp(tag, "Reci"))	ml->recipient = atol(line);
         break;
      case 'S':
              if (!strcmp(tag, "Send"))	ml->sender    = atol(line);
         else if (!strcmp(tag, "Sent"))	ml->sent_time = atol(line);
         else if (!strcmp(tag, "Subj"))	{
           if (!*line || !strcmp(line, "(null)"))
             ml->subject = NULL;
           else
             ml->subject = strdup(line);
         }
         break;
      default:
        sprintf(buf, "SYSERR: Unknown tag %s in mail file %s", tag, fname);
    }
  }
  fclose(fl);
  return TRUE;
}

/* Draft timeout should be called hourly from 'heartbeat' */
void draft_timeout(void)
{
  int i, hours;

  if (CONFIG_DRAFT_TIMEOUT == 0) /* No Timeout! */
    return;

  /* Go through all mails, checking the date */
  for (i = 0; i <= top_of_mail_table; i++) {
    if (IS_MAIL_DRAFT(i)) {
      hours = (time(0) - mail_table[i].sent_time) / 3600;  /* How many hours ago */
      /* Has this mail timed out? */
      if (hours > CONFIG_DRAFT_TIMEOUT * 24) {
        delete_mail_index_entry(mail_table[i].mail_id);
      }
    }
  }
}

bool mail_recip_ok(const char *name)
{
  int player_i;
  bool ret = FALSE;

  if ((player_i = get_ptable_by_name(name)) >= 0) {
    if (!IS_SET(player_table[player_i].flags, PINDEX_DELETED))
      ret = TRUE;
  }
  return ret;
}

struct char_data *find_mailman(struct char_data *ch)
{
  struct char_data *mob;
  for (mob=world[IN_ROOM(ch)].people; mob; mob = mob->next_in_room)
    if (IS_NPC(mob) && MOB_FLAGGED(mob, MOB_MAILMAN))
      return mob;

  return NULL;
}

struct char_data *find_immortal_mailman(void)
{
  int r_room;
  struct char_data *mob;

  if ((r_room = real_room(MAIL_IMMORTAL_ROOM)) == NOWHERE) return NULL;

  for (mob=world[r_room].people; mob; mob = mob->next_in_room)
    if (IS_NPC(mob) && MOB_FLAGGED(mob, MOB_MAILMAN))
      return mob;

  return NULL;
}

int show_inbox_to_char(struct char_data *ch)
{
  int i, j, count=0, unread=0;
  char subj[45], d_t[22], sender[MAX_NAME_LENGTH+1], buf[MAX_STRING_LENGTH], line_color[3];

  for (i = 0; i <= top_of_mail_table; i++) {
    if (mail_table[i].recipient == GET_ID(ch) && !IS_MAIL_DRAFT(i)) {
      /* Show header if this is the first item */
      if (!count) {
        send_to_char(ch, "%stbaMUD Mail System                                                        %sInbox%s\r\n", QBYEL, QBWHT, QNRM);
        send_to_char(ch, "%sFlags Num From               Date/Time              Subject%s\r\n", QYEL, QNRM);
      }

      /* Set up the 'subject' string */
      if (mail_table[i].subject && *(mail_table[i].subject)) {
        /* Copy the subject string */
        for (j=0; j<41 && mail_table[i].subject[j]; j++)
          subj[j] = mail_table[i].subject[j];
        subj[j] = '\0';
        /* If we didn't reach the end of the 'subject', add "..." */
        if (j==41) {
          for (j=0; j<3; j++) subj[j+41] = '.';
          subj[j+41] = '\0';
        }
      } else {
        sprintf(subj, "- No Subject -");
      }

      /* Set up the date/time string */
      strlcpy(buf, (char *) asctime(localtime(&(mail_table[i].sent_time))), sizeof(buf));
      snprintf(d_t, 11, "%s", buf);       /* Start of 'date' - Fri Feb 10 */
      snprintf(d_t+10, 6, " %s", buf+20); /* Add the year    - 2008       */
      snprintf(d_t+15, 7, " %s", buf+11); /* Add the 'time'  - 17:56      */

      /* Set up the sender string */
      if (get_sender_name_by_id(mail_table[i].mail_id) != NULL) {
        sprintf(sender, "%s", get_sender_name_by_id(mail_table[i].mail_id));
      } else {
        sprintf(sender, "Unknown!");
      }

      /* Truncate sender name if over 18 chars */
      if (strlen(sender) > 18)
      {
        strcpy(sender+15, "...");
      }

      /* Set up the 'line color' */
      if (IS_MAIL_READ(i)) {
             if (IS_MAIL_DELETED(i)) sprintf(line_color, "@D");
        else if (IS_MAIL_URGENT(i))  sprintf(line_color, "@r");
        else                         sprintf(line_color, "@g");
      } else {
        unread++;
             if (IS_MAIL_DELETED(i)) sprintf(line_color, "@D");
        else if (IS_MAIL_URGENT(i))  sprintf(line_color, "@R");
        else                         sprintf(line_color, "@G");
      }

      /* Show this line */
      send_to_char(ch, "%s%s%s%s%s %s%-3d %s%-18s%s %-22s %-44s%s\r\n",
                                 IS_MAIL_READ(i) ? " " : "@G*",
                                 IS_MAIL_URGENT(i) ? "@R!" : " ",
                                 MAIL_HAS_GOLD(i) ? (IS_MAIL_COD(i) ? "@Y-" : "@Y+") : " ",
                                 MAIL_HAS_OBJECT(i) ? "@WO" : " ",
                                 IS_MAIL_DELETED(i) ? "@DX" : " ",
                                 line_color, ++count,
                                 IS_SET(mail_table[i].flags, MINDEX_FROM_MOB) ? "@y" : "", sender, line_color,
                                 d_t, subj, QNRM);
    }
  }
  if (count) {
    send_to_char(ch, "%s%d%s mudmails in your inbox\r\n", QYEL, count, QNRM);
    if (unread) send_to_char(ch, "%s%d%s unread mudmails\r\n", QYEL, unread, QNRM);
  }
  else
    send_to_char(ch, "Your inbox is currently empty!\r\n");
  return(count);
}

int mail_list_to_admin(struct char_data *ch)
{
  int i, j, count=0, unread=0;
  char subj[45], d_t[22], sender[MAX_NAME_LENGTH+1], recip[MAX_NAME_LENGTH+1], buf[MAX_STRING_LENGTH], line_color[3];

  for (i = 0; i <= top_of_mail_table; i++) {
    if (!IS_MAIL_DRAFT(i)) {
      /* Show header if this is the first item */
      if (!count) {
        send_to_char(ch, "%stbaMUD Mail System                                                        %sInbox%s\r\n", QBYEL, QBWHT, QNRM);
        send_to_char(ch, "%sFlags Num From               To                 Date/Time              Subject%s\r\n", QYEL, QNRM);
      }

      /* Set up the 'subject' string */
      if (mail_table[i].subject && *(mail_table[i].subject)) {
        /* Copy the subject string */
        for (j=0; j<41 && mail_table[i].subject[j]; j++)
          subj[j] = mail_table[i].subject[j];
        subj[j] = '\0';
        /* If we didn't reach the end of the 'subject', add "..." */
        if (j==41) {
          for (j=0; j<3; j++) subj[j+41] = '.';
          subj[j+41] = '\0';
        }
      } else {
        sprintf(subj, "- No Subject -");
      }

      /* Set up the date/time string */
      strlcpy(buf, (char *) asctime(localtime(&(mail_table[i].sent_time))), sizeof(buf));
      snprintf(d_t, 11, "%s", buf);       /* Start of 'date' - Fri Feb 10 */
      snprintf(d_t+10, 6, " %s", buf+20); /* Add the year    - 2008       */
      snprintf(d_t+15, 7, " %s", buf+11); /* Add the 'time'  - 17:56      */

      /* Set up the sender string */
      sprintf(sender, "%s", get_sender_name_by_id(mail_table[i].mail_id));

      /* Truncate sender name if over 18 chars */
      if (strlen(sender) > 18)
      {
        strcpy(sender+15, "...");
      }

      /* Set up the recipient string */
      sprintf(recip, "%s", get_recipient_name_by_id(mail_table[i].mail_id));

      /* Truncate sender name if over 18 chars */
      if (strlen(recip) > 18)
      {
        strcpy(recip+15, "...");
      }
      /* Set up the 'line color' */
      if (IS_MAIL_READ(i)) {
             if (IS_MAIL_DELETED(i)) sprintf(line_color, "@D");
        else if (IS_MAIL_URGENT(i))  sprintf(line_color, "@r");
        else                         sprintf(line_color, "@g");
      } else {
        unread++;
             if (IS_MAIL_DELETED(i)) sprintf(line_color, "@D");
        else if (IS_MAIL_URGENT(i))  sprintf(line_color, "@R");
        else                         sprintf(line_color, "@G");
      }
      send_to_char(ch, "%s%s%s%s%s %s%-3d %s%-18s%s %s%18s%s %-22s %-44s%s\r\n",
                                 IS_MAIL_READ(i) ? " " : "@G*",
                                 IS_MAIL_URGENT(i) ? "@R!" : " ",
                                 MAIL_HAS_GOLD(i) ? (IS_MAIL_COD(i) ? "@Y-" : "@Y+") : " ",
                                 MAIL_HAS_OBJECT(i) ? "@WO" : " ",
                                 IS_MAIL_DELETED(i) ? "@DX" : " ",
                                 line_color, ++count,
                                 IS_SET(mail_table[i].flags, MINDEX_FROM_MOB) ? "@y" : "", sender, line_color,
                                 ((!*recip) || (!strcmp(recip,"(null)"))) ? "@D" : "", recip, line_color,
                                 d_t, subj, QNRM);
    }
  }
  if (count) {
    send_to_char(ch, "There are %s%d%s mudmails in the system\r\n", QYEL, count, QNRM);
    if (unread) send_to_char(ch, "%s%d%s are unread mudmails\r\n", QYEL, unread, QNRM);
  }
  else
    send_to_char(ch, "The mail system is currently empty!\r\n");
  return(count);
}

int show_outbox_to_char(struct char_data *ch)
{
  int i, j, count=0;
  char subj[45], d_t[22], recipient[MAX_NAME_LENGTH+1], buf[MAX_STRING_LENGTH], line_color[3];

  for (i = 0; i <= top_of_mail_table; i++) {
    if (mail_table[i].sender == GET_ID(ch) && IS_MAIL_DRAFT(i)) {
      /* Show header if this is the first item */
      if (!count) {
        send_to_char(ch, "%stbaMUD Mail System                                                       %sOutbox%s\r\n", QBYEL, QBWHT, QNRM);
        send_to_char(ch, "%sFlags Num To                 Date/Time              Subject%s\r\n", QYEL, QNRM);
      }

      /* Set up the 'subject' string */
      if (mail_table[i].subject && *(mail_table[i].subject)) {
        /* Copy the subject string */
        for (j=0; j<41 && mail_table[i].subject[j]; j++)
          subj[j] = mail_table[i].subject[j];
        subj[j] = '\0';
        /* If we didn't reach the end of the 'subject', add "..." */
        if (j==41) {
          for (j=0; j<3; j++) subj[j+41] = '.';
          subj[j+41] = '\0';
        }
      } else {
        sprintf(subj, "- No Subject -");
      }

      /* Set up the date/time string */
      strlcpy(buf, (char *) asctime(localtime(&(mail_table[i].sent_time))), sizeof(buf));
      snprintf(d_t, 11, "%s", buf);       /* Start of 'date' - Fri Feb 10 */
      snprintf(d_t+10, 6, " %s", buf+20); /* Add the year    - 2008       */
      snprintf(d_t+15, 7, " %s", buf+11); /* Add the 'time'  - 17:56      */

      /* Set up the sender string */
      if (mail_table[i].recipient > 0 && mail_table[i].recipient != NOBODY)
        sprintf(recipient, "%s", get_name_by_id(mail_table[i].recipient));
      else
        sprintf(recipient, "<Nobody>");

      /* Truncate sender name if over 18 chars */
      if (strlen(recipient) > 18)
      {
        strcpy(recipient+15, "...");
      }

      /* Set up the 'line color' */
             if (IS_MAIL_DELETED(i)) sprintf(line_color, "@D");
        else if (IS_MAIL_URGENT(i))  sprintf(line_color, "@r");
        else                         sprintf(line_color, "@g");

      /* Show this line */
      send_to_char(ch, "%s%s%s%s%s %s%-3d %s%-18s%s %-22s %-44s%s\r\n",
                                 IS_MAIL_READ(i) ? " " : "@G*",
                                 IS_MAIL_URGENT(i) ? "@R!" : " ",
                                 MAIL_HAS_GOLD(i) ? (IS_MAIL_COD(i) ? "@Y-" : "@Y+") : " ",
                                 MAIL_HAS_OBJECT(i) ? "@WO" : " ",
                                 IS_MAIL_DELETED(i) ? "@DX" : " ",
                                 line_color, ++count,
                                 IS_SET(mail_table[i].flags, MINDEX_FROM_MOB) ? "@y" : "", recipient, line_color,
                                 d_t, subj, QNRM);
    }
  }
  if (count) {
    send_to_char(ch, "%s%d%s unsent mudmails in your outbox\r\n", QYEL, count, QNRM);
  }
  else
    send_to_char(ch, "Your outbox is currently empty!\r\n");
  return(count);
}

void give_mail_attachments(struct char_data *ch, struct mail_data *ml)
{
  struct obj_data *obj;

  /* First, do the object attachments */
  while (ml->attachment) {
    obj = ml->attachment;
    obj_from_mail(obj);
    obj_to_char(obj, ch);
  }

  /* Then the gold */
  if (ml->coins)
    GET_GOLD(ch) += ml->coins;
}

int count_recipients(struct mail_edit_data *ml_list)
{
  int count = 0;
  struct recipient_list *this;

  this = ml_list->recipients;
  while (this) {
    count++;
    this = this->next;
  }
  return count;
}

void clear_mail_data(struct mail_data *ml)
{
  int i=0;

  ml->mail_id    = NO_MAIL;
  ml->sender     = NOBODY;
  ml->recipient  = NOBODY;
  ml->subject    = NULL;
  ml->body       = NULL;
  ml->attachment = NULL;
  ml->sent_time  = time(0);
  ml->coins      = 0;

  for (i=0; i<ML_ARRAY_MAX; i++)
    (ml->mail_flags[(i)]) = 0;
}

int purge_marked_mail(struct char_data *ch)
{
  bool found;
  int i, count = 0;

  do
  {
    found = FALSE;
    for (i = 0; i <= top_of_mail_table && !found; i++) {
      if (mail_table[i].recipient == GET_ID(ch) && (IS_MAIL_DELETED(i))) {
        /* Remove the index entry - the mail file is never removed, but may get overwritten eventually */
        delete_mail_index_entry(mail_table[i].mail_id);
        /* Set the flag, to cause the search to restart, as the list has changed */
        found = TRUE;
        count++;
      }
    }
  } while (found == TRUE);

  return count;
}

struct mail_data *create_mail(void)
{
  struct mail_data *this_mail;

  CREATE(this_mail, struct mail_data, 1);

  if (!this_mail) return NULL;

  clear_mail_data(this_mail);

  return this_mail;
}

bool add_recipient(struct mail_edit_data *ml, long player_id)
{
  struct recipient_list *this;

  CREATE(this, struct recipient_list, 1);

  if (!this) return FALSE;

  this->recipient = player_id;
  this->next      = ml->recipients;
  ml->recipients  = this;

  return TRUE;
}

bool remove_recipient(struct mail_edit_data *ml, long player_id)
{
  struct recipient_list *this, *del;

  this = ml->recipients;

  if (!this) return FALSE;

  if (this->recipient == player_id) {
    ml->recipients = this->next;
    this->next = NULL;
    free(this);
    return TRUE;
  } else {
    while (this && this->next) {
      if ((this->next)->recipient == player_id) {
        del = this->next;
        this->next = del->next;
        del->next = NULL;
        free(del);
        return TRUE;
      }
	}
  }
  return FALSE;
}

void clear_recipients(struct mail_edit_data *ml)
{
  struct recipient_list *del;

  while (ml->recipients) {
    del = ml->recipients;
    ml->recipients = (ml->recipients)->next;
    del->next = NULL;
    free(del);
  }
}

char *recipient_list(struct mail_edit_data *ml)
{
  struct recipient_list *this;
  static char name_list[MAX_STRING_LENGTH];
  char this_name[MAX_NAME_LENGTH+1];
  int grp;

  if (!(ml->recipients)) return NULL;

  *name_list = '\0';
  this = ml->recipients;
  while (this) {
    if (this->recipient < MAIL_TO_NOBODY) {
      for (grp=0; mail_groups[grp].recipient != MAIL_TO_NOBODY; grp++) {
        if (mail_groups[grp].recipient == this->recipient) {
          sprintf(this_name, "%s", mail_groups[grp].name);
          sprintf(name_list, "%s @Y%s@y", name_list, CAP(this_name));
		}
      }
    } else {
      /* Set up the sender string */
      if (this->recipient > 0 && this->recipient != NOBODY)
        sprintf(this_name, "%s", get_name_by_id(this->recipient));
      else
        sprintf(this_name, "<Nobody>");

      sprintf(name_list, "%s %s", name_list, CAP(this_name));
    }
    this = this->next;
  }
  return name_list;
}

long get_mail_group_by_name(char *name)
{
  int grp;

  for (grp=0; mail_groups[grp].recipient != MAIL_TO_NOBODY; grp++)
    if (!strcmp(mail_groups[grp].name, name))
      return (mail_groups[grp].recipient);

   return MAIL_TO_NOBODY;
}

void list_attachments_numbered(struct obj_data *list, struct char_data *ch)
{
  struct obj_data *i, *j;
  bool found;
  int num, line=0;

  found = FALSE;

  for (i = list; i; i = i->next_content) {
    num = 0;
    for (j = list; j != i; j = j->next_content)
      if (!strcmp(j->short_description, i->short_description) && (!strcmp(j->name, i->name)))
      break;
    if (j != i)
      continue;
    for (j = i; j; j = j->next_content)
      if (!strcmp(j->short_description, i->short_description) && (!strcmp(j->name, i->name)))
        num++;
    if (CAN_SEE_OBJ(ch, i) && (*i->description != '.' || (IS_NPC(ch) || PRF_FLAGGED(ch, PRF_HOLYLIGHT)))) {
      if (num != 1)
        send_to_char(ch, "%d) (%2d) ", ++line, num);
      show_obj_to_char(i, ch, 1);  /* 1 is SHOW_OBJ_SHORT, defined in act.informative.c */
      send_to_char(ch, "%s", CCNRM(ch, C_NRM));
      found = TRUE;
    }
  }
  if (!found)
    send_to_char(ch, "  Nothing.\r\n");
}

struct obj_data *get_attachment_numbered(struct char_data *ch, struct obj_data *list, int find_num)
{
  struct obj_data *i, *j;
  int num, line=0;

  for (i = list; i; i = i->next_content) {
    num = 0;
    for (j = list; j != i; j = j->next_content)
      if (!strcmp(j->short_description, i->short_description) && (!strcmp(j->name, i->name)))
      break;
    if (j != i)
      continue;
    for (j = i; j; j = j->next_content)
      if (!strcmp(j->short_description, i->short_description) && (!strcmp(j->name, i->name)))
        num++;
    if (CAN_SEE_OBJ(ch, i) && (*i->description != '.' || (IS_NPC(ch) || PRF_FLAGGED(ch, PRF_HOLYLIGHT)))) {
      if (++line == find_num)
       return(i);
    }
  }
  return NULL;
}

bool mail_from_player(long to, long from, char *message_pointer)
{
  struct mail_data *this_mail;

  if ((this_mail = create_mail()) == NULL) {
    log("SYSERR: Unable to create new mail structure in mail_from_player");
    return FALSE;
  }

  this_mail->recipient = to;
  this_mail->sender = from;
  this_mail->body = message_pointer;

  if (create_mail_index_entry(this_mail) != -1) {
    if (!save_mail(this_mail)) {
      log("SYSERR: Unable to save new mail from %s to %s", get_name_by_id(from), get_name_by_id(to));
      this_mail->body = NULL;
      extract_mail(this_mail); /* don't free the body */
      return FALSE;
    }
  } else
    log("SYSERR: Unable to create new mail from %s to %s", get_name_by_id(from), get_name_by_id(to));

  this_mail->body = NULL;
  extract_mail(this_mail); /* don't free the body */
  return TRUE;
}

bool mail_from_mobile(struct char_data *mob)
{
  struct mail_edit_data *this_mail;
  struct mail_data *ml;

  if (!IS_NPC(mob)) {
    log("SYSERR: PC calling mail_from_mobile");
    return FALSE;
  }
  this_mail = MOB_MAIL(mob);

  if (this_mail->recipients == NULL) {
    log("SYSERR: mail_from_mobile called with no recipients (%s)", GET_NAME(mob));
    return FALSE;
  }
  if ((this_mail->mail)->body == NULL) {
    log("SYSERR: mail_from_mobile called with blank message (%s)", GET_NAME(mob));
    return FALSE;
  }

  ml = this_mail->mail;

  ml->mail_id = NO_MAIL;
  ml->sender  = GET_MOB_VNUM(mob);
  SET_BIT_AR(ml->mail_flags, MAIL_FROM_MOB);

  if (perform_mob_send_edited(mob) == FALSE) {
    log("SYSERR: Unable to create new mail from %s (mob)", GET_NAME(mob));
    return FALSE;
  }
  return TRUE;
}

void make_cod_payment_mail(struct mail_data *orig)
{
  struct mail_data *this_mail;
  char buf[MAX_STRING_LENGTH];

  /* Mobs can't get mails, so don't send a reply */
  if (IS_SET_AR(orig->mail_flags, MAIL_FROM_MOB)) return;

  /* If no coins requested, then don't make a COD mail */
  if (!IS_SET_AR(orig->mail_flags, MAIL_COD) || orig->coins <= 0) return;


  this_mail = create_mail();
  if (this_mail) {
    sprintf(buf, "Automatically generated COD payment mail from @c%s@n.\r\n"
                 "Attached are @y%d@n coins.\r\n"
                 "'receive' this mail to get it's attached gold.\r\n", get_name_by_id(orig->recipient), orig->coins);
    this_mail->recipient = orig->sender;
    this_mail->sender = orig->recipient;
    this_mail->subject = strdup("- COD Payment -");
    this_mail->body = strdup(buf);
    this_mail->coins = orig->coins;

    if (create_mail_index_entry(this_mail) != -1)
      save_mail(this_mail);
    else {
      log("SYSERR: Unable to create COD reply (%d coins) from %s to %s", this_mail->coins,
                                                                         get_name_by_id(this_mail->sender),
                                                                         get_name_by_id(this_mail->recipient));
    }
    extract_mail(this_mail); /* Free the body and subject*/
  }
}
bool perform_send_to_groups(struct char_data *ch, struct mail_edit_data *ml_list)
{
  int i, j;
  struct mail_data *ml;
  struct recipient_list *this;
  bool do_this, ret = FALSE, to_who[NUM_MAIL_GROUPS+1];

  if (!ml_list || (ml = ml_list->mail) == NULL) {
    log("SYSERR: perform_send_to_groups (mail.c): mail data is empty. Aborting!");
    return FALSE;
  }

  /* Set flags for groups */
  for (i=0; i<=NUM_MAIL_GROUPS; i++) to_who[i] = FALSE;
  for (this=ml_list->recipients; this; this = this->next) {
    for (i=0; i<=NUM_MAIL_GROUPS; i++) {
      if (this->recipient == mail_groups[i].recipient) {
        to_who[i] = TRUE;
      }
    }
  }

  /* Now remove the groups from the recipient list */
  for (i=0; i<=NUM_MAIL_GROUPS; i++) {
    if (to_who[i] == TRUE)
      remove_recipient(ml_list, mail_groups[i].recipient);
  }

  for (i=0; i<=top_of_p_table; i++) {
	do_this = FALSE;
    for (j=0; j<=NUM_MAIL_GROUPS; j++) {
      if (to_who[j] == TRUE) {
        if (mail_groups[j].recipient == MAIL_TO_ALL)  do_this = TRUE;
        if (mail_groups[j].recipient == MAIL_TO_IMMS && player_table[i].level >= ADMLVL_IMMORT) do_this = TRUE;
	  }
    }
    /* Do we need to send to this player? */
    if (do_this) {
      ml->recipient = player_table[i].id;   /* Set the recipient for this mail */
      save_mail(ml);                        /* 'send' the mail by saving it    */
      notify_if_playing(ch, ml->recipient); /* Let the recipient know          */
      ml->mail_id = NO_MAIL;                /* remove the unique ID created when saving */
      ret = TRUE;
    }
  }

  return ret;
}

bool perform_send_edited(struct char_data *ch, struct mail_edit_data *ml_list)
{
  struct mail_data *ml;
  struct recipient_list *this;
  long old_id = NO_MAIL;

  if (!ch || IS_NPC(ch)) {
    log("SYSERR: perform_send_edited (mail.c): Invalid character");
    send_to_char(ch, "Your mail could not be sent!\r\n");
    return FALSE;
  }

  if (!ml_list || ((this = ml_list->recipients) == NULL)) {
    log("SYSERR: perform_send_edited (mail.c): recipient list is empty. Aborting!");
    send_to_char(ch, "Your mail could not be sent!\r\n");
    return FALSE;
  }

  if ((ml = ml_list->mail) == NULL) {
    log("SYSERR: perform_send_edited (mail.c): mail data is empty. Aborting!");
    send_to_char(ch, "Your mail could not be sent!\r\n");
    return FALSE;
  }

  if ((count_recipients(ml_list) > 1) && (ml->coins || ml->attachment)) {
    log("SYSERR: perform_send_edited (mail.c): mail to multiple recipients cannot have attachments or gold.");
    give_mail_attachments(ch, ml);
    send_to_char(ch, "Your mail could not be sent!\r\n");
    return FALSE;
  }

  if (ml->mail_id != NO_MAIL && IS_SET_AR(ml->mail_flags, MAIL_DRAFT))
    old_id = ml->mail_id;

  /* First, check for 'mail groups' and send to them */
  perform_send_to_groups(ch, ml_list);

  /* Cycle through remaining recipients sending to each one */
  while(this) {
    ml->recipient = this->recipient;      /* Set the recipient for this mail */
    save_mail(ml);                        /* 'send' the mail by saving it    */
    notify_if_playing(ch, ml->recipient); /* Let the recipient know          */
    ml->mail_id = NO_MAIL;                /* remove the unique ID created when saving */
    this = this->next;                    /* move to the next recipient      */
  }

  /* Was this a draft copy that has been sent, if so, the old draft can be removed */
  if (old_id != NO_MAIL)
    delete_mail_index_entry(old_id);

  send_to_char(ch, "Mail sent!\r\n");
  return TRUE;
}

bool perform_mob_send_edited(struct char_data *mob)
{
  struct mail_data *ml;
  struct mail_edit_data *ml_list;
  struct recipient_list *this;

  if (!mob || !IS_NPC(mob)) {
    log("SYSERR: perform_mob_send_edited (mail.c): Invalid character");
    mob_log(mob, "Invalid mobile for sending mail from");
    send_to_char(mob, "Mob mail could not be sent!  See syslog!\r\n");
    return FALSE;
  }

  ml_list = MOB_MAIL(mob);

  if (!ml_list || ((this = ml_list->recipients) == NULL)) {
    log("SYSERR: perform_mob_send_edited (mail.c): recipient list is empty. Aborting!");
    mob_log(mob, "mail recipient list is empty. Aborting!");
    send_to_char(mob, "Mob mail could not be sent!  See syslog!\r\n");
    return FALSE;
  }

  if ((ml = ml_list->mail) == NULL) {
    log("SYSERR: perform_mob_send_edited (mail.c): mail data is empty. Aborting!");
    mob_log(mob, "internal mail structure problem");
    send_to_char(mob, "Mob mail could not be sent!  See syslog!\r\n");
    return FALSE;
  }

  if ((count_recipients(ml_list) > 1) && (ml->coins || ml->attachment)) {
    log("SYSERR: perform_mob_send_edited (mail.c): mail to multiple recipients cannot have attachments or gold.");
    mob_log(mob, "multiple recipients in mail with attachments");
    give_mail_attachments(mob, ml);
    send_to_char(mob, "Mob mail could not be sent!  See syslog!\r\n");
    return FALSE;
  }

  /* First, check for 'mail groups' and send to them */
  perform_send_to_groups(mob, ml_list);

  /* Cycle through remaining recipients sending to each one */
  while(this) {
    ml->recipient = this->recipient;      /* Set the recipient for this mail */
    save_mail(ml);                        /* 'send' the mail by saving it    */
    notify_if_playing(mob, ml->recipient); /* Let the recipient know          */
    ml->mail_id = NO_MAIL;                /* remove the unique ID created when saving */
    this = this->next;                    /* move to the next recipient      */
  }

  send_to_char(mob, "Mail sent!\r\n");

  /* Now erase the  mail info */
  clear_recipients(MOB_MAIL(mob));
  extract_mail(MOB_MAIL(mob)->mail);
  MOB_MAIL(mob)->mail = NULL;

  return TRUE;
}

void create_mob_mail(struct char_data *mob)
{
  struct mail_data *ml;
  struct mail_edit_data *ml_list;

  if (!MOB_MAIL(mob)) {
    CREATE(ml_list, struct mail_edit_data, 1);
    MOB_MAIL(mob) = ml_list;
    MOB_MAIL(mob)->mail       = NULL;
    MOB_MAIL(mob)->recipients = NULL;
  }

  if (!(MOB_MAIL(mob)->mail)) {
    ml = create_mail();
    MOB_MAIL(mob)->mail = ml;
  }

  (MOB_MAIL(mob)->mail)->sender = GET_MOB_VNUM(mob);
  SET_BIT_AR((MOB_MAIL(mob)->mail)->mail_flags, MAIL_FROM_MOB);
}

bool has_mail(struct char_data *ch)
{
  int i;

  for (i = 0; i <= top_of_mail_table; i++)
    if (mail_table[i].recipient == GET_ID(ch) && !(IS_MAIL_DELETED(i)) &&
                                                 !(IS_MAIL_DRAFT(i)) &&
                                                 !(IS_MAIL_READ(i)) )
      return TRUE;

  return FALSE;
}

int count_deleted(struct char_data *ch)
{
  int i, count=0;

  for (i = 0; i <= top_of_mail_table; i++) {
    if (mail_table[i].recipient == GET_ID(ch) && (IS_MAIL_DELETED(i)) &&
                                                 !(IS_MAIL_DRAFT(i)) )
      count++;
    else if (mail_table[i].sender == GET_ID(ch) && (IS_MAIL_DELETED(i)) &&
                                              (IS_MAIL_DRAFT(i)) )
      count++;
  }
  return count;
}

void notify_if_playing(struct char_data *from, int recipient_id)
{
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next)
    if ((IS_PLAYING(d)) && (GET_IDNUM(d->character) == recipient_id) && (has_mail(d->character)))
      send_to_char(d->character, "You have new mudmail from %s.\r\n", GET_NAME(from));
}

void notify_on_login(struct char_data *ch)
{
  int count=0, urgent=0, i;

  for (i = 0; i <= top_of_mail_table; i++) {
    if (mail_table[i].recipient == GET_ID(ch) && !(IS_MAIL_DELETED(i)) &&
                                                 !(IS_MAIL_DRAFT(i)) &&
                                                 !(IS_MAIL_READ(i)) ) {
      count++;
      if (IS_MAIL_URGENT(i)) urgent++;
    }
  }
  if (count)
    send_to_char(ch, "You have %d new mudmail%s in your inbox.\r\n", count, count > 1 ? "s" : "");
  if (urgent)
    send_to_char(ch, "%d of these %s urgent.\r\n", urgent, urgent > 1 ? "are" : "is");
}

char *get_sender_name(struct mail_data *ml)
{
  static char name[MAX_STRING_LENGTH];
  struct char_data *mob;

  if (!ml) return NULL;

  if (IS_SET_AR(ml->mail_flags, MAIL_FROM_MOB)) {
    mob = read_mobile(ml->sender, VIRTUAL);
    if (!mob) {
      sprintf(name, "Unknown!");
    } else {
	  char_to_room(mob, 0);
	  sprintf(name, "%s", GET_NAME(mob));
	  extract_char(mob);
    }
  } else {
    if (ml->sender > 0 && ml->sender != NOBODY) {
      if (get_name_by_id(ml->sender) != NULL) {
        sprintf(name, "%s", get_name_by_id(ml->sender));
      } else {
        sprintf(name, "Unknown!");
      }
    } else {
      /* Must be a special case ID */
      switch (ml->sender) {
        case MAIL_FROM_MAIL: sprintf(name, "tbaMUD Mail System");
                             break;

        default:             sprintf(name, "Invalid Sender!");
                             break;
      }
    }
  }
  return (CAP(name));
}

char *get_recipient_name(struct mail_data *ml)
{
  static char name[MAX_STRING_LENGTH];

  if (!ml) return NULL;

  if (ml->recipient > 0) {
    sprintf(name, "%s", get_name_by_id(ml->recipient));
  } else {
    sprintf(name, "Invalid Recipient!");
  }
  return (CAP(name));
}

char *get_sender_name_by_id(long mail_id)
{
  struct mail_data *ml;
  static char *name;

  ml = create_mail();
  if (!load_mail(mail_id, ml)) return NULL;

  name = get_sender_name(ml);

  extract_mail(ml);

  return name;
}

char *get_recipient_name_by_id(long mail_id)
{
  struct mail_data *ml;
  static char *name;

  ml = create_mail();
  if (!load_mail(mail_id, ml)) return NULL;

  name = get_recipient_name(ml);

  extract_mail(ml);

  return name;
}

int attachment_count(struct mail_data *ml)
{
  int count=0;
  struct obj_data *obj, *next_obj;

  /* set up the attachments string */
  if (ml->attachment) {
    obj = ml->attachment;
    next_obj = obj->next_content;
    while (obj) {
      obj = next_obj;
      count++;
    }
  } else {
    return 0;
  }
  return count;
}

char *get_mail_text(struct mail_data *ml)
{
  static char mail_text[MAX_STRING_LENGTH];
  char d_t[MAX_STRING_LENGTH], buf[MAX_INPUT_LENGTH], att[66];
  struct char_data *mob = NULL;
  struct obj_data *obj, *next_obj;
  int len=0, this_len=0, count=0, hours, mins;

  if (!ml) return NULL;

  if (IS_SET_AR(ml->mail_flags, MAIL_FROM_MOB)) {
    mob = read_mobile(ml->sender, VIRTUAL);
  }

  /* Set up the date/time string */
  strlcpy(buf, (char *) asctime(localtime(&ml->sent_time)), sizeof(buf));
  snprintf(d_t, 11, "%s", buf);       /* Start of 'date' - Fri Feb 10 */
  snprintf(d_t+10, 6, " %s", buf+20); /* Add the year    - 2008       */
  snprintf(d_t+15, 7, " %s", buf+11); /* Add the 'time'  - 17:56      */

  hours = (time(0) - ml->sent_time) / 3600;  /* How many hours ago */
  mins  = ((time(0) - ml->sent_time) % 3600) / 60;    /* How many mins ago */

  if (hours >= 24)
    sprintf(d_t+21, "  (%d day%s %d hours ago)", hours/24, hours>=48 ? "s" : "", hours % 24);
  else if (hours >= 1)
    sprintf(d_t+21, "  (%d hour%s %d minutes ago)", hours, hours == 1 ? "" : "s", mins);
  else
    sprintf(d_t+21, "  (%d minutes ago)", mins);

  /* set up the attachments string */
  if (ml->attachment) {
    obj = ml->attachment;
    next_obj = obj->next_content;
    while (obj) {
      /* No need to extract_obj(obj); - the save function should have already done it */
      this_len = snprintf(att, sizeof(att)-len-1, "%s", obj->short_description);
      len += this_len;
      obj = next_obj;
      count++;
    }
  } else {
    sprintf(att, "<None!>");
  }
  if (len >= 64) strcpy(att+61, "...");

  len = 0;

  len += snprintf(mail_text, sizeof(mail_text)-len, "@YtbaMUD Mail System                                                    @WView Mail@n\r\n");
  len += snprintf(mail_text+len, sizeof(mail_text)-len, "-------------------------------------------------------------------------------\r\n");
  len += snprintf(mail_text+len, sizeof(mail_text)-len, "From        : %s\r\n", get_sender_name(ml) );
  len += snprintf(mail_text+len, sizeof(mail_text)-len, "Sent        : %s\r\n", d_t);
  len += snprintf(mail_text+len, sizeof(mail_text)-len, "Subject     : %s\r\n", ml->subject ? ml->subject : "- No Subject -");
  len += snprintf(mail_text+len, sizeof(mail_text)-len, "Urgency     : %s\r\n", IS_SET_AR(ml->mail_flags, MAIL_URGENT) ? "@RUrgent!@n" : "Normal");
  len += snprintf(mail_text+len, sizeof(mail_text)-len, "Status      : %s\r\n", IS_SET_AR(ml->mail_flags, MAIL_DELETED) ? "@D<Deleted>@n" : (IS_SET_AR(ml->mail_flags, MAIL_READ) ? "Read" : "Unread"));

  if (ml->coins > 0) {
    if (IS_SET_AR(ml->mail_flags, MAIL_COD))
      len += snprintf(mail_text+len, sizeof(mail_text)-len, "COD Payment : %d coins\r\n", ml->coins);
    else
      len += snprintf(mail_text+len, sizeof(mail_text)-len, "Gold Coins  : %d coins\r\n", ml->coins);
  }

  if (count) {
    len += snprintf(mail_text+len, sizeof(mail_text)-len, "Attachments : %s\r\n", att);
    len += snprintf(mail_text+len, sizeof(mail_text)-len, "              (Total: %d attachment%s)\r\n", count, count > 1 ? "s" : "");
  }
  len += snprintf(mail_text+len, sizeof(mail_text)-len, "-------------------------------------------------------------------------------\r\n");
  if (ml->body)
    len += snprintf(mail_text+len, sizeof(mail_text)-len, "%s\r\n", ml->body);
  else
    len += snprintf(mail_text+len, sizeof(mail_text)-len, "- No Message -\r\n");
  len += snprintf(mail_text+len, sizeof(mail_text)-len, "-------------------------------------------------------------------------------\r\n");

  return (mail_text);
}

void show_mail_to_char(struct char_data *ch, struct mail_data *ml)
{
  page_string(ch->desc, get_mail_text(ml), TRUE);
}

void mail_view(struct char_data *ch, long mail_id)
{
  struct mail_data *ml;
  int m_num;

  /* to view, we need to load the actual mudmail from the file */
  ml = create_mail();
  if (!load_mail(mail_id, ml)) {
    send_to_char(ch, "Sorry, that mail cannot be viewed right now.  Please tell an Imm!\r\n");
    return;
  }

  show_mail_to_char(ch, ml);

  m_num = find_mail_by_id(mail_id);
  if (!IS_MAIL_READ(m_num))
  {
    /* Mark as 'read' and save */
    SET_BIT_AR(ml->mail_flags, MAIL_READ);
    save_mail(ml);

    /* And mark the index entry and save the index */
    SET_BIT(mail_table[m_num].flags, MINDEX_READ);
    save_mail_index();
  }
  /* finished with the mail - extract it from memory */
  extract_mail(ml);
}

bool mail_receive(struct char_data *ch, long mail_id)
{
  struct mail_data *ml;
  struct obj_data *obj;
  int i;

  /* first, we need to load the actual mudmail from the file */
  ml = create_mail();
  if (!load_mail(mail_id, ml)) {
    send_to_char(ch, "Sorry, that mail cannot be received right now.  Please tell an Imm!\r\n");
    return FALSE;
  }

  /* Is payment required? */
  if (IS_SET_AR(ml->mail_flags, MAIL_COD) && ml->coins > 0) {
    /* Can player afford the price? */
    if (GET_GOLD(ch) < ml->coins) {
      send_to_char(ch, "That mail requires a COD payment of %d coins, and you can't afford it!\r\n", ml->coins);
      return FALSE;
    }
    /* Create the auto-reply */
    make_cod_payment_mail(ml);
    /* And take the payment */
    GET_GOLD(ch) -= ml->coins;
    ml->coins = 0;
  }

  /* Give any other attachments to the player */
  give_mail_attachments(ch, ml);

  /* All checks passed - let's create the mail object */
  obj = create_obj();
  obj->item_number = 1;
  obj->name = strdup("mail paper letter");
  obj->short_description = strdup("a piece of mail");
  obj->description = strdup("Someone has left a piece of mail here.");

  GET_OBJ_TYPE(obj) = ITEM_NOTE;
  for(i = 0; i < TW_ARRAY_MAX; i++)
    obj->obj_flags.wear_flags[i] = 0;
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  GET_OBJ_WEIGHT(obj) = 1;
  GET_OBJ_COST(obj) = 30;
  GET_OBJ_RENT(obj) = 10;


  obj->action_description = strdup(get_mail_text(ml));

  if (obj->action_description == NULL)
    obj->action_description = strdup("Mail system error - please report.  Error #11.\r\n");

  obj_to_char(obj, ch);

  extract_mail(ml);

  /* And erase the mail from the system */
  delete_mail_index_entry(mail_id);

  return TRUE;
}

bool mail_delmark(struct char_data *ch, long mail_id)
{
  struct mail_data *ml;
  int m_num;

  /* to view, we need to load the actual mudmail from the file */
  ml = create_mail();
  if (!load_mail(mail_id, ml)) {
    send_to_char(ch, "Sorry, that mail cannot be marked for deletion right now.  Please tell an Imm!\r\n");
    return FALSE;
  }

  m_num = find_mail_by_id(mail_id);
  if (!IS_MAIL_DELETED(m_num))
  {
    /* Mark as 'deleted' and save */
    SET_BIT_AR(ml->mail_flags, MAIL_DELETED);
    save_mail(ml);

    /* And mark the index entry and save the index */
    SET_BIT(mail_table[m_num].flags, MINDEX_DELETED);
    save_mail_index();
  } else {
    /* Remove deletion mark flag and save */
    REMOVE_BIT_AR(ml->mail_flags, MAIL_DELETED);
    save_mail(ml);

    /* And unmark the index entry and save the index */
    REMOVE_BIT(mail_table[m_num].flags, MINDEX_DELETED);
    save_mail_index();
  }
  /* finished with the mail - extract it from memory */
  extract_mail(ml);
  return TRUE;
}

bool mail_forward(struct char_data *ch, long mail_id, long recipient)
{
  char fwd_subject[MAX_STRING_LENGTH];
  struct mail_data *ml;

  /* to view, we need to load the actual mudmail from the file */
  ml = create_mail();
  if (!load_mail(mail_id, ml)) {
    send_to_char(ch, "Sorry, that mail cannot be forwarded right now.  Please tell an Imm!\r\n");
    return FALSE;
  }
  /* Remove the ID from the old mail - it will be allocated a new one when saving */
  ml->mail_id = NO_MAIL;

  if (ml->subject) {
    sprintf(fwd_subject, "FW: %s", ml->subject);
    free(ml->subject);
  } else {
    sprintf(fwd_subject, "FW: - No Subject! -");
  }

  ml->subject   = strdup(fwd_subject);
  ml->recipient = recipient;
  ml->sender    = GET_ID(ch);

  save_mail(ml);

  /* finished with the mail - extract it from memory */
  extract_mail(ml);

  notify_if_playing(ch, recipient);

  return TRUE;
}

long get_id_by_inbox_num(struct char_data *ch, int num)
{
  int i, inbox_num=0;

  if (!ch || IS_NPC(ch)) return NO_MAIL;

  for (i = 0; i <= top_of_mail_table; i++) {
    if (mail_table[i].recipient == GET_ID(ch) && !IS_MAIL_DRAFT(i)) {
      if ((++inbox_num) == num)
        return mail_table[i].mail_id;
    }
  }
  /* Not Found */
  return NO_MAIL;
}

long get_id_by_outbox_num(struct char_data *ch, int num)
{
  int i, inbox_num=0;

  if (!ch || IS_NPC(ch)) return NO_MAIL;

  for (i = 0; i <= top_of_mail_table; i++) {
    if (mail_table[i].sender == GET_ID(ch) && IS_MAIL_DRAFT(i)) {
      if ((++inbox_num) == num)
        return mail_table[i].mail_id;
    }
  }
  /* Not Found */
  return NO_MAIL;
}

void mail_view_by_num(struct char_data *ch, int num)
{
  long mail_id;

  if (!ch || IS_NPC(ch)) return;

  if ((mail_id = get_id_by_inbox_num(ch, num)) == NO_MAIL) {
    send_to_char(ch, "That mail exists only in your imaginiation!\r\n");
    return;
  }

  mail_view(ch, mail_id);
}

bool mail_receive_by_num(struct char_data *ch, int num)
{
  long mail_id;
  bool ret;

  if (!ch || IS_NPC(ch)) return FALSE;

  if ((mail_id = get_id_by_inbox_num(ch, num)) == NO_MAIL) {
    send_to_char(ch, "That mail exists only in your imaginiation!\r\n");
    return FALSE;
  }

  ret = mail_receive(ch, mail_id);
  return(ret);
}

bool mail_delmark_by_num(struct char_data *ch, int num)
{
  long mail_id;
  bool ret;

  if (!ch || IS_NPC(ch)) return FALSE;

  if ((mail_id = get_id_by_inbox_num(ch, num)) == NO_MAIL) {
    send_to_char(ch, "That mail exists only in your imaginiation!\r\n");
    return FALSE;
  }

  ret = mail_delmark(ch, mail_id);
  return(ret);
}

/* The 'mail' command parser - can only be used in rooms with the MAIL flag */
ACMD(do_mail)
{
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH], *arg4;
  char **mailwrite;
  struct char_data *mailman;
  long recipient, mail_id;
  int num;

  if (!CONFIG_CAN_MAIL) {
    send_to_char(ch, "Sorry, the mudmail system is currently offline.\r\n");
    return;
  }

  if (IS_NPC(ch)) {
    send_to_char(ch, "Only players can send and receive mails. Get a builder to write you a script!\r\n");
    return;
  }

  /* Get the 'mailman' that gives 'tell' messages */
  if (GET_ADMLEVEL(ch) < CONFIG_MIN_MAIL_ANYWHERE) {
    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_MAIL)) {
      send_to_char(ch, "You can only send an receive mail in a post office\r\n");
      return;
    }

    if ((mailman = find_mailman(ch)) == NULL)
    {
      send_to_char(ch, "The postmaster is currently not here! You can't do this yourself!\r\n");
      return;
    }
  } else {
    /* If the immortal mailman wasn't found, try the current room */
    if ((mailman = find_immortal_mailman()) == NULL) {
      mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Immortal mailman not found in room %d", MAIL_IMMORTAL_ROOM);
      if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_MAIL)) {
        send_to_char(ch, "Sorry, the immortal mailman is broken. You will need to find a postmaster!\r\n");
        return;
      }
      if ((mailman = find_mailman(ch)) == NULL)
      {
        send_to_char(ch, "Sorry, the immortal mailman is broken. You will need to find a postmaster!\r\n");
        return;
      }
	}
  }

  if (GET_ADMLEVEL(ch) < CONFIG_MIN_MAIL_LEVEL) {
    snprintf(buf, sizeof(buf), "$n tells you, 'Sorry, you have to be level %d to send mail!'", CONFIG_MIN_MAIL_LEVEL);
    act(buf, FALSE, mailman, 0, ch, TO_VICT);
    return;
  }

  /* Grab 4 arguments to begin with */
  arg4 = one_argument(two_arguments(argument, arg, arg2), arg3);

  if (!*arg) {
    /* No args entered - enter OLC mode, with inbox visible */
    do_mailedit(ch, argument, cmd, subcmd);
  } else if (is_abbrev(arg, "inbox")) {
    show_inbox_to_char(ch);
    return;
  } else if (is_abbrev(arg, "check")) {
    if (has_mail(ch))
      act("$n tells you, 'You have mail waiting.'", FALSE, mailman, 0, ch, TO_VICT);
    else
      act("$n tells you, 'Sorry, you don't have any mail waiting.'", FALSE, mailman, 0, ch, TO_VICT);
    return;
  } else if (is_abbrev(arg, "view") || is_abbrev(arg, "read")) {
	num = atoi(arg2);
	if (num < 1) {
      act("$n tells you, 'That's not a valid mail number'", FALSE, mailman, 0, ch, TO_VICT);
      return;
	}
	mail_view_by_num(ch, num);
    return;
  } else if (is_abbrev(arg, "receive")) {
    num = atoi(arg2);
    if (num < 1) {
      act("$n tells you, 'That's not a valid mail number'", FALSE, mailman, 0, ch, TO_VICT);
      return;
    }
    if (mail_receive_by_num(ch, num)) {
      act("$n hands you your mail.", FALSE, mailman, 0, ch, TO_VICT);
    }
    return;
  } else if (is_abbrev(arg, "delete")) {
	num = atoi(arg2);
	if (num < 1) {
      act("$n tells you, 'That's not a valid mail number'", FALSE, mailman, 0, ch, TO_VICT);
      return;
	}
	mail_delmark_by_num(ch, num);
    return;
  } else if (is_abbrev(arg, "purge")) {
    /* purge all mails marked as deleted */
    num = purge_marked_mail(ch);
    if (num > 0) {
      snprintf(buf, sizeof(buf), "$n tells you, '%d deleted mail%s ha%s been purged.'", num, num == 1 ? "" : "s", num == 1 ? "s" : "ve");
      act(buf, FALSE, mailman, 0, ch, TO_VICT);
    } else {
      act("$n tells you, 'You don't have any deleted mails to purge.'", FALSE, mailman, 0, ch, TO_VICT);
    }
  } else if (is_abbrev(arg, "reply")) {
    if (CONFIG_STAMP_COST > 0 && GET_GOLD(ch) < CONFIG_STAMP_COST && GET_ADMLEVEL(ch) < CONFIG_FREE_MAIL_LEVEL) {
      snprintf(buf, sizeof(buf), "$n tells you, 'A stamp costs %d coin%s.'\r\n"
                                 "$n tells you, '...which I see you can't afford.'", CONFIG_STAMP_COST,
                                 CONFIG_STAMP_COST == 1 ? "" : "s");
      act(buf, FALSE, mailman, 0, ch, TO_VICT);
      return;
    }
    num = atoi(arg2);
    if (num < 1) {
      act("$n tells you, 'That's not a valid mail number'", FALSE, mailman, 0, ch, TO_VICT);
      return;
    }
    /* Get the mail's ID number */
    if ((mail_id = get_id_by_inbox_num(ch, num)) == NO_MAIL) {
      send_to_char(ch, "That mail exists only in your imaginiation!\r\n");
      return;
    }
    /* And then get the mail index rnum */
    num = find_mail_by_id(mail_id);

    if (IS_SET(mail_table[num].flags, MINDEX_FROM_MOB)) {
      act("That mail was sent from an NPC, not another character!\r\n", FALSE, mailman, 0, ch, TO_VICT);
      return;
    }

    recipient = mail_table[num].sender;

    act("$n starts to write some mail.", TRUE, ch, 0, 0, TO_ROOM);
    if (CONFIG_STAMP_COST > 0 &&  GET_ADMLEVEL(ch) < CONFIG_FREE_MAIL_LEVEL) {
    snprintf(buf, sizeof(buf), "$n tells you, 'I'll take %d coins for the stamp.'\r\n"
         "$n tells you, 'Write your message. (/s saves /h for help).'",
           CONFIG_STAMP_COST);
      GET_GOLD(ch) -= CONFIG_STAMP_COST;
    } else {
      snprintf(buf, sizeof(buf), "$n tells you, 'You don't have to pay for mails'\r\n"
           "$n tells you, 'Write your message. (/s saves /h for help).'");
    }
    act(buf, FALSE, mailman, 0, ch, TO_VICT);


    SET_BIT_AR(PLR_FLAGS(ch), PLR_MAILING);	/* string_write() sets writing. */

    /* Start writing! */
    CREATE(mailwrite, char *, 1);
    string_write(ch->desc, mailwrite, MAX_MAIL_SIZE, recipient, NULL);
  } else if (is_abbrev(arg, "forward")) {
	num = atoi(arg2);
	if (num < 1) {
      act("$n tells you, 'That's not a valid mail number'", FALSE, mailman, 0, ch, TO_VICT);
      return;
	}
	/* Get the mail's ID number */
    if ((mail_id = get_id_by_inbox_num(ch, num)) == NO_MAIL) {
      send_to_char(ch, "That mail exists only in your imaginiation!\r\n");
      return;
    }
    if ((recipient = get_id_by_name(arg3)) < 0 || !mail_recip_ok(arg3)) {
      act("$n tells you, 'No one by that name is registered here!'",
      FALSE, mailman, 0, ch, TO_VICT);
      return;
    }
    /* Now forward it */
    if (!mail_forward(ch, mail_id, recipient)) {
      act("$n tells you, 'Sorry, it was not possible to forward your mail.'", FALSE, mailman, 0, ch, TO_VICT);
    } else {
      act("$n tells you, 'Your mail has been sent!'", FALSE, mailman, 0, ch, TO_VICT);
    }
  } else if (is_abbrev(arg, "list") && (GET_ADMLEVEL(ch) == ADMLVL_IMPL)) {
    mail_list_to_admin(ch);
  } else {  /* Assume the old-style "mail <player>" */
    if (CONFIG_STAMP_COST > 0 && GET_GOLD(ch) < CONFIG_STAMP_COST && GET_ADMLEVEL(ch) < CONFIG_FREE_MAIL_LEVEL) {
      snprintf(buf, sizeof(buf), "$n tells you, 'A stamp costs %d coin%s.'\r\n"
                                 "$n tells you, '...which I see you can't afford.'", CONFIG_STAMP_COST,
                                 CONFIG_STAMP_COST == 1 ? "" : "s");
      act(buf, FALSE, mailman, 0, ch, TO_VICT);
      return;
    }
    if ((recipient = get_id_by_name(arg)) < 0 || !mail_recip_ok(arg)) {
      act("$n tells you, 'No one by that name is registered here!'",
      FALSE, mailman, 0, ch, TO_VICT);
      return;
    }
    act("$n starts to write some mail.", TRUE, ch, 0, 0, TO_ROOM);
    if (CONFIG_STAMP_COST > 0 &&  GET_ADMLEVEL(ch) < CONFIG_FREE_MAIL_LEVEL) {
      snprintf(buf, sizeof(buf), "$n tells you, 'I'll take %d coins for the stamp.'\r\n"
           "$n tells you, 'Write your message. (/s saves /h for help).'",
           CONFIG_STAMP_COST);
      GET_GOLD(ch) -= CONFIG_STAMP_COST;
    } else {
      snprintf(buf, sizeof(buf), "$n tells you, 'You don't have to pay for mails'\r\n"
           "$n tells you, 'Write your message. (/s saves /h for help).'");
    }
    act(buf, FALSE, mailman, 0, ch, TO_VICT);


    SET_BIT_AR(PLR_FLAGS(ch), PLR_MAILING);	/* string_write() sets writing. */

    /* Start writing! */
    CREATE(mailwrite, char *, 1);
    string_write(ch->desc, mailwrite, MAX_MAIL_SIZE, recipient, NULL);
  }
}


