/************************************************************************
* hedit.c 	Hedit version 3.0 for Oasis OLC	 5/5/06			*
* by Steve Wolfe - siv@cyberenet.net					*
* Updated by Scott Meisenholder for Oasis 2.0.6                         *
* **********************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "utils.h"
#include "db.h"
#include "boards.h"
#include "oasis.h"
#include "genolc.h"
#include "genzon.h"
#include "handler.h"
#include "improved-edit.h"

/* external data structures */
extern struct descriptor_data *descriptor_list;
void hedit_disp_menu(struct descriptor_data *d);

/* external variables */
extern struct help_index_element *help_table;
void get_one_line(FILE *fl, char *buf);
int search_help(char *argument, int level);
ACMD(do_reboot);

/* local variables */
int top_of_h_table = 0;         /* ref to top of help table      */

/* local functions */
void hedit_save_internally(struct descriptor_data *d);
void hedit_save_to_disk(struct descriptor_data *d);
void hedit_setup_new(struct descriptor_data *d, char *new_key);
void hedit_setup_existing(struct descriptor_data *d, int rnum);

void load_help(FILE *fl, char *name)
{
  char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];
  size_t entrylen;
  char line[READ_SIZE + 1], hname[READ_SIZE + 1],  *scan;
  struct help_index_element el;

  strlcpy(hname, name, sizeof(hname));
  /* get the first keyword line */
  get_one_line(fl, key);
  while (*key != '$') {
    strcat(key, "\r\n");	/* strcat: OK (READ_SIZE - "\n"  "\r\n" == READ_SIZE  1) */
    entrylen = strlcpy(entry, key, sizeof(entry));
    /* read in the corresponding help entry */
    get_one_line(fl, line);
    while (*line != '#' && entrylen < sizeof(entry) - 1) {
      entrylen += strlcpy(entry + entrylen, line, sizeof(entry) - entrylen);

      if (entrylen + 2 < sizeof(entry) - 1) {
        strcpy(entry + entrylen, "\r\n");	/* strcpy: OK (size checked above) */
        entrylen += 2;
      }
      get_one_line(fl, line);
    }

    if (entrylen >= sizeof(entry) - 1) {
      int keysize;
      const char *truncmsg = "\r\n*TRUNCATED*\r\n";

      strcpy(entry + sizeof(entry) - strlen(truncmsg) - 1, truncmsg);	/* strcpy: OK (assuming sane 'entry' size) */

      keysize = strlen(key) - 2;
      log("SYSERR: Help entry exceeded buffer space: %.*s", keysize, key);

      /* If we ran out of buffer space, eat the rest of the entry. */
      while (*line != '#')
        get_one_line(fl, line);
    }

    if (*line == '#') {
      if (sscanf(line, "#%d", &el.min_level) != 1){
        log("SYSERR: Help entry does not have a min level. %s", key);
        el.min_level = 0;
      }
    }
    el.duplicate = 0;
    el.entry = strdup(entry);
    scan = one_word(key, next_key);
    while (*next_key) {
      el.keywords = strdup(next_key);
      help_table[top_of_h_table++] = el;
      el.duplicate++;
      scan = one_word(scan, next_key);
     }
   /* get next keyword line (or $) */
    get_one_line(fl, key);
  }
}

int hsort(const void *a, const void *b)
{
  const struct help_index_element *a1, *b1;

  a1 = (const struct help_index_element *) a;
  b1 = (const struct help_index_element *) b;

  return (str_cmp(a1->keywords, b1->keywords));
}

ACMD(do_oasis_hedit)
{
  struct descriptor_data *d;
  int i;

  /* No building as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

  if (!can_edit_zone(ch, HEDIT_PERMISSION)) {
    send_to_char(ch, "You don't have access to editing help files.\r\n");
    return;
  }

  for (d = descriptor_list; d; d = d->next)
    if (STATE(d) == CON_HEDIT) {
      send_to_char(ch, "Sorry, only one can person can edit help files at a time.\r\n");
      return;
    }

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Please specify a help entry to edit.\r\n");
    return;
  }

  d = ch->desc;

  if (!str_cmp("save", argument)) {
    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "OLC: %s saves help files.", GET_NAME(ch));
    send_to_char(ch, "Writing help file..\r\n");
    hedit_save_to_disk(d);
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
  OLC_STORAGE(d) = strdup(argument);
  OLC_ZNUM(d) = search_help(OLC_STORAGE(d), LVL_IMPL);

  for(i = 0; i < (int)strlen(argument); i++)
    argument[i] = toupper(argument[i]);

  if (OLC_ZNUM(d) == NOWHERE)
     hedit_setup_new(d, OLC_STORAGE(d));
  else
     hedit_setup_existing(d, OLC_ZNUM(d));


  STATE(d) = CON_HEDIT;
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
  mudlog(CMP, LVL_IMMORT, TRUE, "OLC: %s starts editing help files.", GET_NAME(d->character));
}

/* Utils and exported functions. */
void hedit_setup_new(struct descriptor_data *d, char *new_key)
{
  CREATE(OLC_HELP(d), struct help_index_element, 1);

  OLC_HELP(d)->keywords = strdup(new_key);
  OLC_HELP(d)->entry = strdup("This is an unfinished help entry.\r\n");
  OLC_HELP(d)->min_level = 0;
  hedit_disp_menu(d);
  OLC_VAL(d) = 0;
}

void hedit_setup_existing(struct descriptor_data *d, int rnum)
{
  /* Build a copy of the help entry for editing.*/
  CREATE(OLC_HELP(d), struct help_index_element, 1);
  /* Allocate space for all strings. */
  OLC_HELP(d)->keywords = strdup(help_table[rnum].keywords ?
	help_table[rnum].keywords : "UNDEFINED\r\n");
  OLC_HELP(d)->entry = strdup(help_table[rnum].entry ?
	help_table[rnum].entry : "undefined\r\n");
  OLC_HELP(d)->min_level = help_table[rnum].min_level;

  /* Attach copy of help entry to player's descriptor. */
  OLC_VAL(d) = 0;
  hedit_disp_menu(d);
}

void hedit_save_internally(struct descriptor_data *d)
{
  struct help_index_element *new_help_table = NULL;
  int i;

  if (OLC_ZNUM(d) > top_of_h_table) {
    CREATE(new_help_table, struct help_index_element, top_of_h_table + 2);
    new_help_table[0] = *OLC_HELP(d);
    for (i = 0; i <= top_of_h_table; i++)
      new_help_table[i + 1] = help_table[i];
    free(help_table);
    help_table = new_help_table;
    top_of_h_table ++;
  } else
    help_table[OLC_ZNUM(d)] = *OLC_HELP(d);
    hedit_save_to_disk(d);
}

void hedit_save_to_disk(struct descriptor_data *d)
{
  FILE *fp;
  char buf1[MAX_STRING_LENGTH], index_name[READ_SIZE], buf[READ_SIZE];
  int i;

  snprintf(index_name, sizeof(index_name), "%s%s", HLP_PREFIX, HELP_FILE);
  if (!(fp = fopen(index_name, "w"))) {
    log("SYSERR: Could not write help index file");
    return;
  }

  for (i = 0; i <= top_of_h_table; i++){
    if (help_table[i].entry && help_table[i].duplicate)
      continue;
    strncpy(buf1, help_table[i].entry ? help_table[i].entry : "Empty\n\r", sizeof(buf1) - 1);
    strip_cr(buf1);

    /* Forget making a buffer, lets just write the thing now. */
    fprintf(fp, "%s" "#%d\n", buf1, help_table[i].min_level);
  }
  /* Write final line and close. */
  fprintf(fp, "$~\n");
  fclose(fp);
  do_reboot(d->character, strcpy(buf, "xhelp"), 0, 0);
}

/* Menu functions */
/* The main menu. */
void hedit_disp_menu(struct descriptor_data *d)
{
  clear_screen(d);
  write_to_output(d,

         "-- Help file editor\r\n"
         "1) Keywords    : %s\r\n"
         "2) Entry       :\r\n%s"
	 "@g3@n) Min Level   : @c%d@n\r\n"
	  "@gQ@n) Quit\r\n"
	  "Enter choice : ",

	  OLC_HELP(d)->keywords,
	  OLC_HELP(d)->entry,
	  OLC_HELP(d)->min_level
	  );
   OLC_MODE(d) = HEDIT_MAIN_MENU;
}

/* The main loop */
void hedit_parse(struct descriptor_data *d, char *arg)
{
  char buf[MAX_STRING_LENGTH];
  char *oldtext = '\0';
  int number;

  switch (OLC_MODE(d)) {
  case HEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      snprintf(buf, sizeof(buf), "OLC: %s edits help for %s.", GET_NAME(d->character), OLC_HELP(d)->keywords);
      mudlog(TRUE, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), CMP, buf);
      write_to_output(d, "Help files saved to disk.\r\n");
      hedit_save_internally(d);
      /* Do NOT free strings! Just the help structure. */
      cleanup_olc(d, CLEANUP_STRUCTS);
      break;
    case 'n':
    case 'N':
      /* Free everything up, including strings, etc. */
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      write_to_output(d, "Invalid choice!\r\nDo you wish to save your changes? : \r\n");
      break;
    }
    return;

  case HEDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d)) { /* Something has been modified. */
	write_to_output(d, "Do you wish to save your changes? : ");
	OLC_MODE(d) = HEDIT_CONFIRM_SAVESTRING;
      } else {
        log("this is a test of quit out of hedit");
	write_to_output(d, "No changes made.\r\n");
	cleanup_olc(d, CLEANUP_ALL);
      }
      break;
    case '1':
      write_to_output(d, "Enter keywords:-\r\n] ");
      OLC_MODE(d) = HEDIT_KEYWORDS;
      break;
    case '2':
      OLC_MODE(d) = HEDIT_ENTRY;
      clear_screen(d);
      send_editor_help(d);
      write_to_output(d, "Enter help entry: (/s saves /h for help)\r\n");
      if (OLC_HELP(d)->entry) {
	write_to_output(d, "%s", OLC_HELP(d)->entry);
	oldtext = strdup(OLC_HELP(d)->entry);
      }
      string_write(d, &OLC_HELP(d)->entry, 8052, 0, oldtext);
      OLC_VAL(d) = 1;
      break;
    case '3':
      write_to_output(d, "Enter min level:-\r\n] ");
      OLC_MODE(d) = HEDIT_MIN_LEVEL;
      break;
    default:
      write_to_output(d, "Invalid choice!\r\n");
      hedit_disp_menu(d);
      break;
    }
    return;

  case HEDIT_KEYWORDS:
    if (OLC_HELP(d)->keywords)
      free(OLC_HELP(d)->keywords);
    if (strlen(arg) > MAX_HELP_KEYWORDS)
      arg[MAX_HELP_KEYWORDS - 1] = '\0';
    strip_cr(arg);
    OLC_HELP(d)->keywords = strdup((arg && *arg) ? arg : "UNDEFINED");
    break;

  case HEDIT_ENTRY:
    /* We will NEVER get here, we hope. */
    mudlog(TRUE, LVL_BUILDER, BRF, "SYSERR: Reached HEDIT_ENTRY case in parse_hedit");
    break;

  case HEDIT_MIN_LEVEL:
    number = atoi(arg);
    if ((number < 0) || (number > LVL_IMPL))
      write_to_output(d, "That is not a valid choice!\r\nEnter min level:-\r\n] ");
    else {
      OLC_HELP(d)->min_level = number;
      break;
    }
    return;

  default:
    /* We should never get here. */
    mudlog(TRUE, LVL_BUILDER, BRF, "SYSERR: Reached default case in parse_hedit");
    break;
  }
  /* If we get this far, something has been changed. */
  OLC_VAL(d) = 1;
  hedit_disp_menu(d);
}

void hedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {
    case HEDIT_ENTRY:
    default:
      hedit_disp_menu(d);
      break;
  }
}

ACMD(do_helpcheck)
{
  ACMD(do_action);

  char buf[MAX_STRING_LENGTH];
  int i, count = 0;
  size_t len = 0, nlen;

  send_to_char(ch, "Commands without help entries:\r\n");
  send_to_char(ch, "-------------------------------------------------------------------\r\n");

  for (i = 1; *(complete_cmd_info[i].command) != '\n'; i++) {
    if (complete_cmd_info[i].command_pointer != do_action) {
      if (search_help((char *) complete_cmd_info[i].command, LVL_IMPL) == NOWHERE) {
        nlen = snprintf(buf + len, sizeof(buf) - len, " %-20.20s%s",
            complete_cmd_info[i].command, (++count % 3 ? "|":"\r\n"));
        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;
        len += nlen;
      }
    }
  }
  if (count % 3 && len < sizeof(buf))
    nlen = snprintf(buf + len, sizeof(buf) - len, "\r\n");

  if (ch->desc)
    page_string(ch->desc, buf, TRUE);

  *buf = '\0';
}

ACMD(do_hindex)
{
  int len, count = 0, i;
  char buf[MAX_STRING_LENGTH];

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: hindex <string>\r\n");
    return;
  }

  len = sprintf(buf, "Help index entries based on '%s':\r\n", argument);
  for (i = 0; i <= top_of_h_table; i++)
    if (is_abbrev(argument, help_table[i].keywords) && (GET_LEVEL(ch) >= help_table[i].min_level))
      len += snprintf(buf + len, sizeof(buf) - len, "%-20.20s%s",
    help_table[i].keywords, (++count % 3 ? "" : "\r\n"));

    if (count % 3)
      len += snprintf(buf + len, sizeof(buf) - len, "\r\n");

    if (!count)
      len += snprintf(buf + len, sizeof(buf) - len, "  None.\r\n");

  page_string(ch->desc, buf, TRUE);
}

void free_help(struct help_index_element *help)
{

  if (help->keywords)
    free(help->keywords);
  if (help_table->entry && !help_table->duplicate)
    free(help->entry);

  free(help);
}

void free_help_table(void)
{
  int i;

  if (help_table) {
    for (i = 0; i <= top_of_h_table; i++) {
      if (help_table[i].keywords)
        free(help_table[i].keywords);
      if (help_table[i].entry && !help_table[i].duplicate)
        free(help_table[i].entry);
    }
    free(help_table);
  }
  top_of_h_table = 0;
}
