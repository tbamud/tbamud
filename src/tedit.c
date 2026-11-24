/**************************************************************************
*  File: tedit.c                                           Part of tbaMUD *
*  Usage: Oasis OLC - Text files.                                         *
*                                                                         *
* By Michael Scott [Manx].                                                *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "modify.h"

extern time_t motdmod;
extern time_t newsmod;

void tedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  FILE *fl;
  char *storage = d->olc ? OLC_STORAGE(d) : NULL;
  char **target = d->olc ? OLC_TEDIT_TARGET(d) : NULL;

  if (!storage || !target)
    terminator = STRINGADD_ABORT;

  switch (terminator) {
  case STRINGADD_SAVE:
    if (!(fl = fopen(storage, "w")))
      mudlog(CMP, LVL_IMPL, TRUE, "SYSERR: Can't write file '%s'.", storage);
    else {
      char *draft = d->str ? *d->str : OLC_TEDIT_STR(d);
      char *new_live;

      if (draft) {
        char *disk_copy = strdup(draft);
        if (disk_copy) {
          strip_cr(disk_copy);
          fputs(disk_copy, fl);
          free(disk_copy);
        }
      }
      fclose(fl);

      new_live = draft ? draft : strdup("");
      OLC_TEDIT_STR(d) = NULL;

      if (*target) {
        struct descriptor_data *in_use;
        for (in_use = descriptor_list; in_use; in_use = in_use->next) {
          if (!in_use->showstr_count || in_use->showstr_head ||
              !in_use->showstr_vector || *in_use->showstr_vector != *target)
            continue;
          in_use->showstr_head = strdup(*target);
          if (in_use->showstr_head) {
            int temppage = in_use->showstr_page;
            paginate_string(in_use->showstr_head, in_use);
            in_use->showstr_page = temppage;
          }
        }
        free(*target);
      }
      *target = new_live;

      mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE, "OLC: %s saves '%s'.", GET_NAME(d->character), storage);
      write_to_output(d, "Saved.\r\n");
      if (!strcmp(storage, NEWS_FILE))
        newsmod = time(0);
      if (!strcmp(storage, MOTD_FILE))
        motdmod = time(0);
    }
    break;
  case STRINGADD_ABORT:
    write_to_output(d, "Edit aborted.\r\n");
    act("$n stops editing some scrolls.", TRUE, d->character, 0, 0, TO_ROOM);
    break;
  default:
    log("SYSERR: tedit_string_cleanup: Unknown terminator status.");
    break;
  }

  /* Common cleanup code. */
  d->str = NULL;
  cleanup_olc(d, CLEANUP_ALL);
  STATE(d) = CON_PLAYING;
}

ACMD(do_tedit)
{
  int l, i = 0;
  char field[MAX_INPUT_LENGTH];

  struct {
    char *cmd;
    char level;
    char **buffer;
    int  size;
    char *filename;
  } fields[] = {
	/* edit the lvls to your own needs */
	{ "credits",	LVL_IMPL,	&credits,	2400,	CREDITS_FILE},
	{ "news",	LVL_GRGOD,	&news,		8192,	NEWS_FILE},
	{ "motd",	LVL_GRGOD,	&motd,		2400,	MOTD_FILE},
	{ "imotd",	LVL_IMPL,	&imotd,		2400,	IMOTD_FILE},
        { "greetings",  LVL_IMPL,       &GREETINGS,     2400,   GREETINGS_FILE},
        { "help",       LVL_GRGOD,      &help,          2400,   HELP_PAGE_FILE},
	{ "ihelp",      LVL_GRGOD,	&ihelp, 	2400,	IHELP_PAGE_FILE},
	{ "info",	LVL_GRGOD,	&info,		8192,	INFO_FILE},
	{ "background",	LVL_IMPL,	&background,	8192,	BACKGROUND_FILE},
	{ "handbook",   LVL_IMPL,	&handbook,	8192,   HANDBOOK_FILE},
	{ "policies",	LVL_IMPL,	&policies,	8192,	POLICIES_FILE},
        { "wizlist",    LVL_IMPL,       &wizlist,       2400,   WIZLIST_FILE},
        { "immlist",    LVL_GRGOD,      &immlist,       2400,   IMMLIST_FILE},
	{ "\n",		0,		NULL,		0,	NULL }
  };

  if (ch->desc == NULL)
    return;

  one_argument(argument, field);

  if (!*field) {
    send_to_char(ch, "Files available to be edited:\r\n");
    for (l = 0; *fields[l].cmd != '\n'; l++) {
      if (GET_LEVEL(ch) >= fields[l].level) {
	send_to_char(ch, "%-11.11s ", fields[l].cmd);
	if (!(++i % 7))
	  send_to_char(ch, "\r\n");
      }
    }
    if (i % 7)
      send_to_char(ch, "\r\n");
    if (i == 0)
      send_to_char(ch, "None.\r\n");
    return;
  }
  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (*fields[l].cmd == '\n') {
    send_to_char(ch, "Invalid text editor option.\r\n");
    return;
  }

  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return;
  }

  /* set up editor stats */
  clear_screen(ch->desc);
  send_editor_help(ch->desc);
  send_to_char(ch, "Edit file below:\r\n\r\n");

  if (ch->desc->olc) {
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_tedit: Player already had olc structure.");
    cleanup_olc(ch->desc, CLEANUP_ALL);
  }
  CREATE(ch->desc->olc, struct oasis_olc_data, 1);

  if (*fields[l].buffer) {
    send_to_char(ch, "%s", *fields[l].buffer);
    OLC_TEDIT_STR(ch->desc) = strdup(*fields[l].buffer);
  }

  OLC_STORAGE(ch->desc) = strdup(fields[l].filename);
  OLC_TEDIT_TARGET(ch->desc) = fields[l].buffer;
  string_write(ch->desc, &OLC_TEDIT_STR(ch->desc), fields[l].size, 0, NULL);

  act("$n begins editing a scroll.", TRUE, ch, 0, 0, TO_ROOM);
  if (!IS_NPC(ch))
    SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
  STATE(ch->desc) = CON_TEDIT;
}
