/**************************************************************************
*  File: modify.c                                          Part of tbaMUD *
*  Usage: Run-time modification of game variables.                        *
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
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "spells.h"
#include "mail.h"
#include "boards.h"
#include "improved-edit.h"
#include "oasis.h"
#include "class.h"
#include "dg_scripts.h" /* for trigedit_string_cleanup */
#include "modify.h"
#include "quest.h"
#include "ibt.h"

/* local (file scope) function prototpyes  */
static char *next_page(char *str, struct char_data *ch);
static int count_pages(char *str, struct char_data *ch);
static void playing_string_cleanup(struct descriptor_data *d, int action);
static void exdesc_string_cleanup(struct descriptor_data *d, int action);

/* Local (file scope) global variables */
/* @deprecated string_fields appears to be no longer be used.
 * Left in but commented out.
static const char *string_fields[] =
{
  "name",
  "short",
  "long",
  "description",
  "title",
  "delete-description",
  "\n"
};
*/

/** maximum length for text field x+1
 *  @deprecated length appears to no longer be used. Left in but commented out.
static int length[] =
{
  15,
  60,
  256,
  240,
  60
};
*/

/* modification of malloc'ed strings */
/* Put '#if 1' here to erase ~, or roll your own method.  A common idea is
 * smash/show tilde to convert the tilde to another innocuous character to
 * save and then back to display it. Whatever you do, at least keep the
 * function around because other MUD packages use it, like mudFTP. -gg */
void smash_tilde(char *str)
{
  /* Erase any _line ending_ tildes inserted in the editor. The load mechanism
   * can't handle those, yet. - Welcor */
  char *p = str;
  for (; *p; p++)
    if (*p == '~' && (*(p+1)=='\r' || *(p+1)=='\n' || *(p+1)=='\0'))
      *p=' ';
}

/* Parse out the @ character and replace it with the '\t' to work with
 * KaVir's protocol snippet */
void parse_at(char *str)
{
  char *p = str;
   for (; *p; p++)
     if (*p == '@') {
       if (*(p+1) != '@')
         *p = '\t';
       else
         p++;
	 }
}

void parse_tab(char *str)
{
  char *p = str;
   for (; *p; p++)
     if (*p == '\t') {
       if (*(p+1) != '\t')
         *p = '@';
       else
         p++;
	 }
}

/* Basic API function to start writing somewhere. 'data' isn't used, but you
 * can use it to pass whatever else you may want through it.  The improved
 * editor patch when updated could use it to pass the old text buffer, for
 * instance. */
void string_write(struct descriptor_data *d, char **writeto, size_t len, long mailto, void *data)
{
  if (d->character && !IS_NPC(d->character))
    SET_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);

  if (using_improved_editor)
    d->backstr = (char *)data;
  else if (data)
    free(data);

  d->str = writeto;
  d->max_str = len;
  d->mail_to = mailto;
}

/* Add user input to the 'current' string (as defined by d->str). This is still
 * overly complex. */
void string_add(struct descriptor_data *d, char *str)
{
  int action;

  /* Determine if this is the terminal string, and truncate if so. Changed to
   * only accept '\t' at the beginning of line. - JE */

  delete_doubledollar(str);
  smash_tilde(str);

  /* Determine if this is the terminal string, and truncate if so. Changed to
   * only accept '\t' if it's by itself. - fnord */
  if ((action = (*str == '\t' && !str[1])))
    *str = '\0';
  else
    if ((action = improved_editor_execute(d, str)) == STRINGADD_ACTION)
      return;

  if (action != STRINGADD_OK)
    /* Do nothing. */ ;
  else if (!(*d->str)) {
    if (strlen(str) + 3 > d->max_str) { /* \r\n\0 */
      send_to_char(d->character, "String too long - Truncated.\r\n");
      strcpy(&str[d->max_str - 3], "\r\n");	/* strcpy: OK (size checked) */
      CREATE(*d->str, char, d->max_str);
      strcpy(*d->str, str);	/* strcpy: OK (size checked) */
      if (!using_improved_editor)
        action = STRINGADD_SAVE;
    } else {
      CREATE(*d->str, char, strlen(str) + 3);
      strcpy(*d->str, str);	/* strcpy: OK (size checked) */
    }
  } else {
    if (strlen(str) + strlen(*d->str) + 3 > d->max_str) { /* \r\n\0 */
      send_to_char(d->character, "String too long.  Last line skipped.\r\n");
      if (!using_improved_editor)
        action = STRINGADD_SAVE;
      else if (action == STRINGADD_OK)
        action = STRINGADD_ACTION;    /* No appending \r\n\0, but still let them save. */
    } else {
      RECREATE(*d->str, char, strlen(*d->str) + strlen(str) + 3); /* \r\n\0 */
      strcat(*d->str, str);	/* strcat: OK (size precalculated) */
    }
  }

  /* Common cleanup code. */
  switch (action) {
    case STRINGADD_ABORT:
      switch (STATE(d)) {
        case CON_CEDIT:
        case CON_TEDIT:
        case CON_REDIT:
        case CON_MEDIT:
        case CON_OEDIT:
        case CON_PLR_DESC:
        case CON_TRIGEDIT:
        case CON_HEDIT:
        case CON_QEDIT:
        case CON_IBTEDIT:
	      free(*d->str);
          *d->str    = d->backstr;
          d->backstr = NULL;
          d->str     = NULL;
          break;
        default:
          log("SYSERR: string_add: Aborting write from unknown origin.");
          break;
      }
      break;
    case STRINGADD_SAVE:
      if (d->str && *d->str && **d->str == '\0') {
        free(*d->str);
        *d->str = strdup("Nothing.\r\n");
      }
      if (d->backstr)
        free(d->backstr);
      d->backstr = NULL;
      break;
    case STRINGADD_ACTION:
      break;
  }

  /* Ok, now final cleanup. */
  if (action == STRINGADD_SAVE || action == STRINGADD_ABORT) {
    int i;
    struct {
      int mode;
      void (*func)(struct descriptor_data *d, int action);
    } cleanup_modes[] = {
      { CON_CEDIT  , cedit_string_cleanup },
      { CON_MEDIT  , medit_string_cleanup },
      { CON_OEDIT  , oedit_string_cleanup },
      { CON_REDIT  , redit_string_cleanup },
      { CON_TEDIT  , tedit_string_cleanup },
      { CON_TRIGEDIT, trigedit_string_cleanup },
      { CON_PLR_DESC , exdesc_string_cleanup },
      { CON_PLAYING, playing_string_cleanup },
      { CON_HEDIT, hedit_string_cleanup },
      { CON_QEDIT  , qedit_string_cleanup },
      { CON_IBTEDIT, ibtedit_string_cleanup },
      { -1, NULL }
    };

    for (i = 0; cleanup_modes[i].func; i++)
      if (STATE(d) == cleanup_modes[i].mode)
        (*cleanup_modes[i].func)(d, action);

    /* Common post cleanup code. */
    d->str = NULL;
    d->mail_to = 0;
    d->max_str = 0;
    if (d->character && !IS_NPC(d->character)) {
      REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_BUG);
      REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_IDEA);
      REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_TYPO);
      REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
      REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
    }
  } else if (action != STRINGADD_ACTION && strlen(*d->str) + 3 <= d->max_str) /* 3 = \r\n\0 */
     strcat(*d->str, "\r\n");
}

static void playing_string_cleanup(struct descriptor_data *d, int action)
{
  if (PLR_FLAGGED(d->character, PLR_MAILING)) {
    if (action == STRINGADD_SAVE && *d->str) {
      store_mail(d->mail_to, GET_IDNUM(d->character), *d->str);
      write_to_output(d, "Message sent!\r\n");
      notify_if_playing(d->character, d->mail_to);
    } else
      write_to_output(d, "Mail aborted.\r\n");
      free(*d->str);
      free(d->str);
    }

  /* We have no way of knowing which slot the post was sent to so we can only
   * give the message.   */
    if (d->mail_to >= BOARD_MAGIC) {
      board_save_board(d->mail_to - BOARD_MAGIC);
      if (action == STRINGADD_ABORT)
        write_to_output(d, "Post not aborted, use REMOVE <post #>.\r\n");
    }
  if (PLR_FLAGGED(d->character, PLR_IDEA)) {
    if (action == STRINGADD_SAVE && *d->str){
      write_to_output(d, "Idea saved!\r\n");
      save_ibt_file(SCMD_IDEA);
    } else {
      write_to_output(d, "Idea aborted!\r\n");
      clean_ibt_list(SCMD_IDEA);
    }
  }
  if (PLR_FLAGGED(d->character, PLR_BUG)) {
    if (action == STRINGADD_SAVE && *d->str){
      write_to_output(d, "Bug saved!\r\n");
      save_ibt_file(SCMD_BUG);
    } else {
      write_to_output(d, "Bug aborted!\r\n");
      clean_ibt_list(SCMD_BUG);
    }
  }
  if (PLR_FLAGGED(d->character, PLR_TYPO)) {
    if (action == STRINGADD_SAVE && *d->str){
      write_to_output(d, "Typo saved!\r\n");
      save_ibt_file(SCMD_TYPO);
    } else {
      write_to_output(d, "Typo aborted!\r\n");
      clean_ibt_list(SCMD_TYPO);
    }
  }
}

static void exdesc_string_cleanup(struct descriptor_data *d, int action)
{
  if (action == STRINGADD_ABORT)
    write_to_output(d, "Description aborted.\r\n");

  write_to_output(d, "%s", CONFIG_MENU);
  STATE(d) = CON_MENU;
}

/* Modification of character skills. */
ACMD(do_skillset)
{
  struct char_data *vict;
  char name[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH], helpbuf[MAX_STRING_LENGTH];
  int skill, value, i, qend, pc, pl;

  argument = one_argument(argument, name);

  if (!*name) {			/* no arguments. print an informative text */
    send_to_char(ch, "Syntax: skillset <name> '<skill>' <value>\r\n"
		"Skill being one of the following:\r\n");
    for (qend = 0, i = 0; i <= TOP_SPELL_DEFINE; i++) {
      if (spell_info[i].name == unused_spellname)	/* This is valid. */
	continue;
      send_to_char(ch, "%18s", spell_info[i].name);
      if (qend++ % 4 == 3)
	send_to_char(ch, "\r\n");
    }
    if (qend % 4 != 0)
      send_to_char(ch, "\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    return;
  }
  skip_spaces(&argument);
  pc = GET_CLASS(vict);
  pl = GET_LEVEL(vict);

  /* If there is no chars in argument */
  if (!*argument) {
    send_to_char(ch, "Skill name expected.\r\n");
    return;
  }
  if (*argument != '\'') {
    send_to_char(ch, "Skill must be enclosed in: ''\r\n");
    return;
  }
  /* Locate the last quote and lowercase the magic words (if any) */

  for (qend = 1; argument[qend] && argument[qend] != '\''; qend++)
    argument[qend] = LOWER(argument[qend]);

  if (argument[qend] != '\'') {
    send_to_char(ch, "Skill must be enclosed in: ''\r\n");
    return;
  }
  strcpy(helpbuf, (argument + 1));	/* strcpy: OK (MAX_INPUT_LENGTH <= MAX_STRING_LENGTH) */
  helpbuf[qend - 1] = '\0';
  if ((skill = find_skill_num(helpbuf)) <= 0) {
    send_to_char(ch, "Unrecognized skill.\r\n");
    return;
  }
  argument += qend + 1;		/* skip to next parameter */
  argument = one_argument(argument, buf);

  if (!*buf) {
    send_to_char(ch, "Learned value expected.\r\n");
    return;
  }
  value = atoi(buf);
  if (value < 0) {
    send_to_char(ch, "Minimum value for learned is 0.\r\n");
    return;
  }
  if (value > 100) {
    send_to_char(ch, "Max value for learned is 100.\r\n");
    return;
  }
  if (IS_NPC(vict)) {
    send_to_char(ch, "You can't set NPC skills.\r\n");
    return;
  }
  if ((spell_info[skill].min_level[(pc)] >= LVL_IMMORT) && (pl < LVL_IMMORT)) {
    send_to_char(ch, "%s cannot be learned by mortals.\r\n", spell_info[skill].name);
    return;
  } else if (spell_info[skill].min_level[(pc)] > pl) {
    send_to_char(ch, "%s is a level %d %s.\r\n", GET_NAME(vict), pl, pc_class_types[pc]);
    send_to_char(ch, "The minimum level for %s is %d for %ss.\r\n", spell_info[skill].name, spell_info[skill].min_level[(pc)], pc_class_types[pc]);
  }

  /* find_skill_num() guarantees a valid spell_info[] index, or -1, and we
   * checked for the -1 above so we are safe here. */
  SET_SKILL(vict, skill, value);
  mudlog(BRF, LVL_IMMORT, TRUE, "%s changed %s's %s to %d.", GET_NAME(ch), GET_NAME(vict), spell_info[skill].name, value);
  send_to_char(ch, "You change %s's %s to %d.\r\n", GET_NAME(vict), spell_info[skill].name, value);
}

/* By Michael Buselli. Traverse down the string until the begining of the next
 * page has been reached.  Return NULL if this is the last page of the string. */
static char *next_page(char *str, struct char_data *ch)
{
  int col = 1, line = 1, count, pw;

  pw = (GET_SCREEN_WIDTH(ch) >= 40 && GET_SCREEN_WIDTH(ch) <= 250) ? GET_SCREEN_WIDTH(ch) : PAGE_WIDTH;

  for (;; str++) {
    /* If end of string, return NULL. */
    if (*str == '\0')
      return (NULL);

    /* If we're at the start of the next page, return this fact. */
    else if (line > (GET_PAGE_LENGTH(ch) - (PRF_FLAGGED(ch, PRF_COMPACT) ? 1 : 2)))
      return (str);

    /* Check for the beginning of an ANSI color code block. */
    else if (*str == '\x1B')  /* Jump to the end of the ANSI code, or max 9 chars */
      for (count=0; *str != 'm' && count < 9; count++)
        str++;

    else if (*str == '\t') {
      if (*(str + 1) != '\t')
        str++;
    }

    /* Check for everything else. */
    else {
      /* Carriage return puts us in column one. */
      if (*str == '\r')
        col = 1;
      /* Newline puts us on the next line. */
      else if (*str == '\n')
        line++;

      /* We need to check here and see if we are over the page width, and if
       * so, compensate by going to the begining of the next line. */
      else if (col++ > pw) {
        col = 1;
        line++;
      }
    }
  }
}

/* Function that returns the number of pages in the string. */
static int count_pages(char *str, struct char_data *ch)
{
  int pages;

  for (pages = 1; (str = next_page(str, ch)); pages++)
    ; /* moved semi-colon to suppress warning */
  return (pages);
}

/* This function assigns all the pointers for showstr_vector for the
 * page_string function, after showstr_vector has been allocated and
 * showstr_count set. */
void paginate_string(char *str, struct descriptor_data *d)
{
  int i;

  if (d->showstr_count)
    *(d->showstr_vector) = str;

  for (i = 1; i < d->showstr_count && str; i++)
    str = d->showstr_vector[i] = next_page(str, d->character);

  d->showstr_page = 0;
}

/* The call that gets the paging ball rolling... */
void page_string(struct descriptor_data *d, char *str, int keep_internal)
{
  char actbuf[MAX_INPUT_LENGTH] = "";

  if (!d)
    return;

  if (!str || !*str)
    return;

  if (GET_PAGE_LENGTH(d->character) < 5)
    GET_PAGE_LENGTH(d->character) = PAGE_LENGTH;
  d->showstr_count = count_pages(str, d->character);
  CREATE(d->showstr_vector, char *, d->showstr_count);

  if (keep_internal) {
    d->showstr_head = strdup(str);
    paginate_string(d->showstr_head, d);
  } else
    paginate_string(str, d);

  show_string(d, actbuf);
}

/* The call that displays the next page. */
void show_string(struct descriptor_data *d, char *input)
{
  char buffer[MAX_STRING_LENGTH], buf[MAX_INPUT_LENGTH];
  int diff;

  any_one_arg(input, buf);

  /* Q is for quit. :) */
  if (LOWER(*buf) == 'q') {
    free(d->showstr_vector);
    d->showstr_vector = NULL;
    d->showstr_count = 0;
    if (d->showstr_head) {
      free(d->showstr_head);
      d->showstr_head = NULL;
    }
    return;
  }
  /* Back up one page internally so we can display it again. */
  else if (LOWER(*buf) == 'r')
    d->showstr_page = MAX(0, d->showstr_page - 1);

  /* Back up two pages internally so we can display the correct page here. */
  else if (LOWER(*buf) == 'b')
    d->showstr_page = MAX(0, d->showstr_page - 2);

  /* Type the number of the page and you are there! */
  else if (isdigit(*buf))
    d->showstr_page = MAX(0, MIN(atoi(buf) - 1, d->showstr_count - 1));

  else if (*buf) {
    send_to_char(d->character, "Valid commands while paging are RETURN, Q, R, B, or a numeric value.\r\n");
    return;
  }
  /* If we're displaying the last page, just send it to the character, and
   * then free up the space we used. Also send a \tn - to make color stop
   * bleeding. - Welcor */
  if (d->showstr_page + 1 >= d->showstr_count) {
    send_to_char(d->character, "%s\tn", d->showstr_vector[d->showstr_page]);
    free(d->showstr_vector);
    d->showstr_vector = NULL;
    d->showstr_count = 0;
    if (d->showstr_head) {
      free(d->showstr_head);
      d->showstr_head = NULL;
    }
  }
  /* Or if we have more to show.... */
  else {
    diff = d->showstr_vector[d->showstr_page + 1] - d->showstr_vector[d->showstr_page];
    if (diff > MAX_STRING_LENGTH - 3) /* 3=\r\n\0 */
      diff = MAX_STRING_LENGTH - 3;
    strncpy(buffer, d->showstr_vector[d->showstr_page], diff);	/* strncpy: OK (size truncated above) */
    /* Fix for prompt overwriting last line in compact mode by Peter Ajamian */
    if (buffer[diff - 2] == '\r' && buffer[diff - 1]=='\n')
      buffer[diff] = '\0';
    else if (buffer[diff - 2] == '\n' && buffer[diff - 1] == '\r')
      /* This is backwards.  Fix it. */
      strcpy(buffer + diff - 2, "\r\n");	/* strcpy: OK (size checked) */
    else if (buffer[diff - 1] == '\r' || buffer[diff - 1] == '\n')
      /* Just one of \r\n.  Overwrite it. */
      strcpy(buffer + diff - 1, "\r\n");	/* strcpy: OK (size checked) */
    else
      /* Tack \r\n onto the end to fix bug with prompt overwriting last line. */
      strcpy(buffer + diff, "\r\n");	/* strcpy: OK (size checked) */
    send_to_char(d->character, "%s", buffer);
    d->showstr_page++;
  }
}
