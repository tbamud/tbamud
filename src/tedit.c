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
#include <errno.h>

extern time_t motdmod;
extern time_t newsmod;

struct tedit_field {
  const char *cmd;
  char level;
  char **buffer;
  int size;
  const char *filename;
  unsigned long version;
};

static struct tedit_field fields[] = {
  /* edit the lvls to your own needs */
  { "credits",    LVL_IMPL,  &credits,    2400, CREDITS_FILE,    0 },
  { "news",       LVL_GRGOD, &news,       8192, NEWS_FILE,       0 },
  { "motd",       LVL_GRGOD, &motd,       2400, MOTD_FILE,       0 },
  { "imotd",      LVL_IMPL,  &imotd,      2400, IMOTD_FILE,      0 },
  { "greetings",  LVL_IMPL,  &GREETINGS,  2400, GREETINGS_FILE,  0 },
  { "help",       LVL_GRGOD, &help,       2400, HELP_PAGE_FILE,  0 },
  { "ihelp",      LVL_GRGOD, &ihelp,      2400, IHELP_PAGE_FILE, 0 },
  { "info",       LVL_GRGOD, &info,       8192, INFO_FILE,       0 },
  { "background", LVL_IMPL,  &background, 8192, BACKGROUND_FILE, 0 },
  { "handbook",   LVL_IMPL,  &handbook,   8192, HANDBOOK_FILE,   0 },
  { "policies",   LVL_IMPL,  &policies,   8192, POLICIES_FILE,   0 },
  { "wizlist",    LVL_IMPL,  &wizlist,    2400, WIZLIST_FILE,    0 },
  { "immlist",    LVL_GRGOD, &immlist,    2400, IMMLIST_FILE,    0 },
  { "\n",         0,         NULL,           0, NULL,            0 }
};

#define NUM_TEDIT_FIELDS ((int)(sizeof(fields) / sizeof(fields[0]) - 1))

/* Move any reader paging old_text in-place (keep_internal == 0) onto its own
 * private copy while preserving each page pointer's exact byte offset.  If
 * allocating that copy fails, dismiss the reader's pager before freeing. */
static void detach_pagers_from_text(char *old_text)
{
  struct descriptor_data *in_use;

  if (!old_text)
    return;

  for (in_use = descriptor_list; in_use; in_use = in_use->next) {
    char *copy;
    int page;

    if (!in_use->showstr_count || in_use->showstr_head ||
        !in_use->showstr_vector || in_use->showstr_vector[0] != old_text)
      continue;

    copy = strdup(old_text);
    if (!copy) {
      clear_pager(in_use);
      continue;
    }

    in_use->showstr_head = copy;
    for (page = 0; page < in_use->showstr_count; page++) {
      if (in_use->showstr_vector[page])
        in_use->showstr_vector[page] = copy + (in_use->showstr_vector[page] - old_text);
    }
  }
}

void tedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  FILE *fl;
  char *storage = d->olc ? OLC_STORAGE(d) : NULL;
  char **target = d->olc ? OLC_TEDIT_TARGET(d) : NULL;
  int idx = d->olc ? OLC_NUM(d) : -1;

  if (!storage || !target || idx < 0 || idx >= NUM_TEDIT_FIELDS ||
      fields[idx].buffer != target)
    terminator = STRINGADD_ABORT;

  switch (terminator) {
  case STRINGADD_SAVE:
    {
      int same_text = (!*target && !OLC_TEDIT_ORIG(d)) ||
                      (*target && OLC_TEDIT_ORIG(d) && !strcmp(*target, OLC_TEDIT_ORIG(d)));
      char *draft = d->str ? *d->str : OLC_TEDIT_STR(d);
      char *disk_copy = NULL;
      char *empty_live = NULL;
      char tmp_path[MAX_INPUT_LENGTH];
      size_t disk_len = 0;
      int n, saved_errno = 0, write_failed = 0;

      if (OLC_TEDIT_VERSION(d) != fields[idx].version || !same_text) {
        write_to_output(d, "Save failed: '%s' was modified since you opened it. "
                           "Your draft has been kept; use /l to view it or /a to abort.\r\n",
                        storage);
        return;
      }

      n = snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", storage);
      if (n < 0 || (size_t)n >= sizeof(tmp_path)) {
        mudlog(CMP, LVL_IMPL, TRUE, "SYSERR: Path too long for temporary file '%s'.", storage);
        write_to_output(d, "Save failed: file path too long. Your draft has been kept.\r\n");
        return;
      }

      if (draft) {
        disk_copy = strdup(draft);
        if (!disk_copy) {
          mudlog(CMP, LVL_IMPL, TRUE, "SYSERR: Out of memory saving '%s'.", storage);
          write_to_output(d, "Save failed: out of memory. Your draft has been kept.\r\n");
          return;
        }
        strip_cr(disk_copy);
        disk_len = strlen(disk_copy);
      } else {
        empty_live = strdup("");
        if (!empty_live) {
          mudlog(CMP, LVL_IMPL, TRUE, "SYSERR: Out of memory saving '%s'.", storage);
          write_to_output(d, "Save failed: out of memory. Your draft has been kept.\r\n");
          return;
        }
      }

      if (!(fl = fopen(tmp_path, "w"))) {
        saved_errno = errno;
        free(disk_copy);
        free(empty_live);
        mudlog(CMP, LVL_IMPL, TRUE, "SYSERR: Can't open temporary file '%s': %s",
               tmp_path, strerror(saved_errno));
        write_to_output(d, "Save failed: could not open temporary file for '%s'. Your draft has been kept.\r\n",
                        storage);
        return;
      }

      if (disk_len > 0 && fwrite(disk_copy, 1, disk_len, fl) != disk_len) {
        write_failed = 1;
        saved_errno = errno;
      }
      if (fflush(fl) == EOF || ferror(fl)) {
        if (!write_failed)
          saved_errno = errno;
        write_failed = 1;
      }
      if (fclose(fl) == EOF) {
        if (!write_failed)
          saved_errno = errno;
        write_failed = 1;
      }
      free(disk_copy);

      if (write_failed) {
        remove(tmp_path);
        free(empty_live);
        mudlog(CMP, LVL_IMPL, TRUE, "SYSERR: Error writing '%s': %s",
               tmp_path, strerror(saved_errno));
        write_to_output(d, "Save failed: error writing '%s'. Your draft has been kept.\r\n",
                        storage);
        return;
      }

      if (!genolc_install_file(tmp_path, storage)) {
        saved_errno = errno;
        remove(tmp_path);
        free(empty_live);
        mudlog(CMP, LVL_IMPL, TRUE, "SYSERR: Could not put '%s' in place as '%s': %s",
               tmp_path, storage, strerror(saved_errno));
        write_to_output(d, "Save failed: could not install '%s'. Your draft has been kept.\r\n",
                        storage);
        return;
      }

      OLC_TEDIT_STR(d) = NULL;
      if (*target) {
        detach_pagers_from_text(*target);
        free(*target);
      }
      *target = draft ? draft : empty_live;
      fields[idx].version++;

      mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE,
             "OLC: %s saves '%s'.", GET_NAME(d->character), storage);
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

  if (ch->desc->olc) {
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_tedit: Player already had olc structure.");
    cleanup_olc(ch->desc, CLEANUP_ALL);
  }
  CREATE(ch->desc->olc, struct oasis_olc_data, 1);

  OLC_NUM(ch->desc) = l;
  OLC_STORAGE(ch->desc) = strdup(fields[l].filename);
  OLC_TEDIT_TARGET(ch->desc) = fields[l].buffer;
  OLC_TEDIT_VERSION(ch->desc) = fields[l].version;

  if (*fields[l].buffer) {
    OLC_TEDIT_STR(ch->desc) = strdup(*fields[l].buffer);
    OLC_TEDIT_ORIG(ch->desc) = strdup(*fields[l].buffer);
  }

  if (!OLC_STORAGE(ch->desc) ||
      (*fields[l].buffer && (!OLC_TEDIT_STR(ch->desc) || !OLC_TEDIT_ORIG(ch->desc)))) {
    cleanup_olc(ch->desc, CLEANUP_ALL);
    send_to_char(ch, "Unable to open text editor: out of memory.\r\n");
    return;
  }

  /* set up editor stats */
  clear_screen(ch->desc);
  send_editor_help(ch->desc);
  send_to_char(ch, "Edit file below:\r\n\r\n");
  if (*fields[l].buffer)
    send_to_char(ch, "%s", *fields[l].buffer);

  string_write(ch->desc, &OLC_TEDIT_STR(ch->desc), fields[l].size, 0, NULL);

  act("$n begins editing a scroll.", TRUE, ch, 0, 0, TO_ROOM);
  if (!IS_NPC(ch))
    SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
  STATE(ch->desc) = CON_TEDIT;
}
