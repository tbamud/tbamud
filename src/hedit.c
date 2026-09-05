/**************************************************************************
*  File: hedit.c                                           Part of tbaMUD *
*  Usage: Oasis OLC Help Editor.                                          *
* Author: Steve Wolfe, Scott Meisenholder, Rhade                          *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "boards.h"
#include "oasis.h"
#include "genolc.h"
#include "genzon.h"
#include "handler.h"
#include "improved-edit.h"
#include "act.h"
#include "hedit.h"
#include "modify.h"

/* local functions */
static void hedit_disp_menu(struct descriptor_data *);
static void hedit_setup_new(struct descriptor_data *);
static void hedit_setup_existing(struct descriptor_data *, int);
static void hedit_save_to_disk(struct descriptor_data *);
static int hedit_save_internally(struct descriptor_data *);
static int hedit_same_keyword_line(const char *, const char *);


ACMD(do_oasis_hedit)
{
  char arg[MAX_INPUT_LENGTH];
  struct descriptor_data *d;
  int i;

  /* No building as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

  if (!can_edit_zone(ch, HEDIT_PERMISSION)) {
    send_to_char(ch, "You don't have access to editing help files.\r\n");
    return;
  }

  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_HEDIT) {
      send_to_char(ch, "Sorry, only one can person can edit help files at a time.\r\n");
      return;
    }
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Please specify a help entry to edit.\r\n");
    return;
  }

  d = ch->desc;

  if (!str_cmp("save", arg)) {
    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "OLC: %s saves help files.",
           GET_NAME(ch));
    /* The delete path pairs its add_to_save_list with the removal inside
     * hedit_save_to_disk. This one never adds, so the removal finds nothing
     * and logs "remove_from_save_list: Saved item not found." on every
     * `hedit save`. Pairing it here silences that without changing what
     * reaches disk -- it is exactly the noise this commit complains about
     * elsewhere, and fixing it only for the delete was inconsistent. */
    add_to_save_list(HEDIT_PERMISSION, SL_HLP);
    hedit_save_to_disk(d);
    send_to_char(ch, "Saving help files.\r\n");
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
  
  OLC_ZNUM(d) = search_help(OLC_STORAGE(d), LVL_IMPL);
  /* Remember which table this index belongs to; the delete refuses if it
   * is rebuilt underneath the editor. */
  OLC_HELP_VERSION(d) = help_table_version;

  /* Bound the row before reading it. search_help() answers NOWHERE whenever
   * the keyword has no entry, which is every `hedit <something new>` -- the
   * ordinary way to create one -- and NOWHERE is an unsigned 65535, so the
   * read lands 2MB from the base of the table (65535 rows of 32 bytes), well
   * past the end of it.
   *
   * What that costs depends on the build. Under a sanitiser it is a reliable
   * SEGV. On a plain build the address has so far always been mapped, and
   * what comes back is garbage: measured once as duplicate = 593, non-zero,
   * which sends the loop below scanning every row for an entry pointer that
   * came from nowhere. That one had a NULL entry so nothing matched, but a
   * value that collided with a live row would open an unrelated help file.
   *
   * `< top_of_helpt` rather than `!= NOWHERE`: search_help can only answer
   * NOWHERE or an in-range row, so the two are equivalent today, but a bound
   * stays right if that ever stops being true. */
  if (OLC_ZNUM(d) < top_of_helpt && help_table[OLC_ZNUM(d)].duplicate) {
    for (i = 0; i < top_of_helpt; i++)
      if (help_table[i].duplicate == 0 && help_table[i].entry == help_table[OLC_ZNUM(d)].entry) {
        OLC_ZNUM(d) = i;
        break;
      }
  }

  if (OLC_ZNUM(d) == NOWHERE) {
    send_to_char(ch, "Do you wish to add the '%s' help file? ", OLC_STORAGE(d));
    OLC_MODE(d) = HEDIT_CONFIRM_ADD;
  } else {
    send_to_char(ch, "Do you wish to edit the '%s' help file?", help_table[OLC_ZNUM(d)].keywords);
    OLC_MODE(d) = HEDIT_CONFIRM_EDIT;
  }

  STATE(d) = CON_HEDIT;
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
  mudlog(CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), 
    TRUE, "OLC: %s starts editing help files.", GET_NAME(d->character));
}

static void hedit_setup_new(struct descriptor_data *d)
{
  CREATE(OLC_HELP(d), struct help_index_element, 1);

  OLC_HELP(d)->keywords		= strdup(OLC_STORAGE(d));
  OLC_HELP(d)->entry		= strdup("KEYWORDS\r\n\r\nThis help file is unfinished.\r\n");
  OLC_HELP(d)->min_level	= 0;
  OLC_HELP(d)->duplicate	= 0;
  OLC_VAL(d) = 0;

  hedit_disp_menu(d);
}

static void hedit_setup_existing(struct descriptor_data *d, int rnum)
{
  CREATE(OLC_HELP(d), struct help_index_element, 1);

  OLC_HELP(d)->keywords		= str_udup(help_table[rnum].keywords);
  OLC_HELP(d)->entry		= str_udup(help_table[rnum].entry);
  OLC_HELP(d)->duplicate	= help_table[rnum].duplicate;
  OLC_HELP(d)->min_level	= help_table[rnum].min_level;
  OLC_VAL(d) = 0;

  /* How to find this row again if the table is rebuilt underneath the
   * editor. Both are taken now, before the builder can edit either field.
   *
   * The keyword alone is not an identity: a row's keyword is one word, and
   * the first word of a multi-keyword entry can be another entry's only
   * keyword. The shipped help file has eleven first keywords shared by two
   * or more entries, three of them the first word of a multi-keyword entry
   * that is another entry's only keyword -- `spells`
   * names both its own entry and the first keyword of the magic entry --
   * and matching on the word alone lands on whichever comes first.
   *
   * The text is the entry's identity: load_help strdups it once per entry,
   * so rows of one entry share a pointer and rows of different entries do
   * not. Together they separate the two -- as strings, once the table has
   * been rebuilt and the pointers mean nothing, which is why two entries
   * with the same keyword line AND the same body (the shipped file has one
   * such pair, `Empty`) cannot be told apart at all and are refused. */
  OLC_HELP_KEY(d) = str_udup(help_table[rnum].keywords);
  OLC_HELP_TEXT(d) = str_udup(help_table[rnum].entry);

  /* And whether the keyword line itself names this entry alone. It does not
   * always: the shipped file has four keyword lines each carried by two or
   * three separate entries with different bodies (SUMMON, CONTROL-WEATHER,
   * the AC-CONFIDENCE line, and TOGGLES CONFIGURE SETTINGS -- that last pair
   * differs only by a trailing blank, which the compare ignores), plus the
   * `Empty` pair whose bodies match too. For those the line cannot stand in
   * for the
   * text once the text has changed, and hedit_relocate has to know that
   * before a reload has removed the twins it would need to notice it. */
  {
    int i, n;

    for (i = 0, n = 0; i < top_of_helpt; i++)
      if (!help_table[i].duplicate && help_table[i].entry &&
          hedit_same_keyword_line(OLC_HELP_TEXT(d), help_table[i].entry))
        n++;
    OLC_HELP_LINE_SHARED(d) = (n > 1);
  }

  hedit_disp_menu(d);
}

/* A write has to land on the entry's primary row: hedit_save_to_disk skips
 * duplicates, so one that lands on a duplicate never reaches the file. */
static int hedit_primary_of(int i)
{
  int j;

  if (!help_table[i].duplicate)
    return i;
  for (j = 0; j < top_of_helpt; j++)
    if (!help_table[j].duplicate && help_table[j].entry == help_table[i].entry)
      return j;
  /* A duplicate with no primary. The table is already wrong; writing here
   * at least does not reach into another entry. */
  return i;
}

/* Find the row this editor opened, in a table that has been rebuilt since it
 * opened.
 *
 * Not by what the builder typed: OLC_STORAGE is a word, and answering 'n' at
 * the confirm prompt walks forward to the next row that word abbreviates, so
 * after a walk it names a different entry than the one being edited.
 *
 * And not by the row's keyword alone, which was the first thing tried and is
 * worse than the bug it fixes. A row's keyword is one word, and the first
 * word of a multi-keyword entry can be another entry's only keyword -- the
 * shipped file has eleven first keywords shared by two or more entries.
 * Relocating on the word alone took whichever came first, so editing the
 * magic entry through `magics` had the save destroy the separate `spells`
 * entry instead.
 *
 * Nor by the keyword alone even where it now names exactly one row. That was
 * the second thing tried, as the rule for "somebody edited the text", and it
 * has the same failure from the other side: cut the opened entry from the
 * file, reload, and its collision twin is the one row left carrying the word.
 * The save landed on the twin and reported success.
 *
 * So: the pair; then the keyword together with the entry's own keyword line,
 * which load_help writes as the first line of the text and an edit to the
 * body leaves alone; and otherwise no answer at all rather than a guess.
 *
 * The keyword line is only an identity where it was one when the editor
 * opened. Four lines in the shipped file are each carried by two or three
 * entries with different bodies, and for those a lone surviving row with
 * the right line is indistinguishable from a twin whose sibling was cut:
 * review staged SUMMON that way and the save landed on the other SUMMON.
 * hedit_setup_existing records whether the line was shared, and the second
 * rule is simply not available to such an entry -- it refuses instead.
 *
 * HEDIT_RELOC_AMBIGUOUS is a refusal for the caller to report;
 * HEDIT_RELOC_NOTFOUND means nothing carries the keyword any more -- the
 * entry was removed, or renamed out from under the word, which look the same
 * from here -- and the save becomes an add. Appending is the answer that
 * destroys nothing either way; if it was a rename the file then holds the
 * entry twice, under each name, which the builder can see and fix. */
#define HEDIT_RELOC_NOTFOUND	(-1)
#define HEDIT_RELOC_AMBIGUOUS	(-2)

/* Whether two entry texts open with the same keyword line. Trailing blanks
 * are ignored: 108 keyword lines in the shipped file end in a space, and an
 * editor that trims it on the way through must not turn a body edit into a
 * refusal. */
static int hedit_same_keyword_line(const char *a, const char *b)
{
  size_t na = strcspn(a, "\r\n"), nb = strcspn(b, "\r\n");

  while (na > 0 && isspace((unsigned char) a[na - 1]))
    na--;
  while (nb > 0 && isspace((unsigned char) b[nb - 1]))
    nb--;

  return na == nb && !strncmp(a, b, na);
}

static int hedit_relocate(struct descriptor_data *d)
{
  const char *key = OLC_HELP_KEY(d), *text = OLC_HELP_TEXT(d);
  int i, n, found, matched = FALSE;

  /* 1. Both. The reload that changed nothing, and every reload that changed
   *    something else, land here.
   *
   *    Counted, not taken on first sight. Two entries can have the same
   *    keyword line and the same body -- the shipped file has a pair, both
   *    called `Empty` -- and then the pair names both. Taking the first
   *    wrote a builder's change to the twin they had walked past with 'n'
   *    to avoid, and left the one on their screen untouched. Rows of one
   *    entry all resolve to its primary, so a keyword repeated on one line
   *    counts once. */
  if (key && text) {
    for (i = 0, n = 0, found = -1; i < top_of_helpt; i++)
      if (help_table[i].keywords && !strcmp(help_table[i].keywords, key) &&
          help_table[i].entry && !strcmp(help_table[i].entry, text) &&
          hedit_primary_of(i) != found) {
        n++;
        found = hedit_primary_of(i);
      }
    if (n == 1)
      return found;
    if (n > 1)
      return HEDIT_RELOC_AMBIGUOUS;
  }

  /* 2. The keyword, if exactly one row carrying it also carries the entry's
   *    keyword line, and that line named this entry alone when it opened:
   *    somebody edited the body of the text. A row with the word but a
   *    different keyword line is a different entry, however alone it now
   *    stands, and counts only towards the refusal; so does every row when
   *    the line was shared at open, since a lone survivor then proves
   *    nothing about which entry it is.
   *
   *    There is deliberately no rule between these two matching on the
   *    text alone. load_help puts the keyword line INTO the entry text, so
   *    text that still matches exactly is text whose keyword line is
   *    unchanged -- which means the captured keyword is still one of that
   *    entry's rows, and step 1 has already answered. A rename that keeps
   *    the word changes the keyword line and lands here as a refusal; one
   *    that drops the word lands nowhere, and is treated as a removal. */
  if (key && text) {
    for (i = 0, n = 0, found = -1; i < top_of_helpt; i++)
      if (help_table[i].keywords && !strcmp(help_table[i].keywords, key)) {
        matched = TRUE;
        if (!OLC_HELP_LINE_SHARED(d) && help_table[i].entry &&
            hedit_same_keyword_line(text, help_table[i].entry)) {
          n++;
          found = i;
        }
      }
    if (n == 1)
      return hedit_primary_of(found);
  }

  return matched ? HEDIT_RELOC_AMBIGUOUS : HEDIT_RELOC_NOTFOUND;
}

/* FALSE means nothing was written and nothing was discarded; the caller says
 * why and leaves the builder in the editor. */
static int hedit_save_internally(struct descriptor_data *d)
{
  struct help_index_element *new_help_table = NULL;

  /* An index into a table that has been rebuilt since the editor opened names
   * whatever now sits in that slot, so writing through it overwrites an entry
   * the builder never asked for. Only a help reload can do that while hedit is
   * open, since hedit refuses a second editor -- `reload xhelp`, `reload all`
   * and `reload *` all reach free_help_table() + index_boot(DB_BOOT_HLP).
   *
   * Take the row again rather than refusing outright: this is the last thing
   * that runs before the editor is torn down, so a flat refusal would throw
   * the builder's work away to protect somebody else's. Treating it as new is
   * not an option either -- a reload that changes nothing still bumps the
   * counter, and appending then puts a second entry in help.hlp under the same
   * keyword, which search_help resolves to the older of the two, so the builder
   * could not reach their own work even by reopening it.
   *
   * Only where the row genuinely cannot be identified is the save refused, and
   * then the version is deliberately left stale: the builder is put back in the
   * editor, and their next attempt has to come through here again rather than
   * sail past a guard that has already been satisfied. */
  if (OLC_HELP_VERSION(d) != help_table_version) {
    int row = hedit_relocate(d);

    if (row == HEDIT_RELOC_AMBIGUOUS)
      return FALSE;

    OLC_ZNUM(d) = (row == HEDIT_RELOC_NOTFOUND) ? NOWHERE : row;
    OLC_HELP_VERSION(d) = help_table_version;
  }

  /* The write always lands on the entry's primary row, stale index or not.
   * hedit_save_to_disk skips duplicates, so a builder who reached one with 'n'
   * at the confirm prompt had their min_level change written nowhere at all --
   * only the entry text survived, and only because the pass below carries it
   * to the primary by hand. */
  if (OLC_ZNUM(d) != NOWHERE && OLC_ZNUM(d) < top_of_helpt)
    OLC_ZNUM(d) = hedit_primary_of(OLC_ZNUM(d));
  OLC_HELP(d)->duplicate = 0;

  /* Out of range counts as new, not as a row to overwrite. NOWHERE is not
   * the only value that gets here: answering 'n' at "Do you wish to edit
   * the '<keyword>' help file?" steps OLC_ZNUM to the next row so a
   * builder can page through duplicate-keyword twins, which works because
   * duplicates sort adjacently. On the first non-match it sets
   * top_of_helpt + 1, which the walk's own increment then takes to
   * top_of_helpt + 2 before routing to the add prompt. Saving there wrote two elements past the
   * end of help_table -- a 32-byte heap write, silent on a normal build,
   * six keystrokes after an ordinary `hedit <existing keyword>`.
   *
   * The bound closes a second route as well: the walk can leave the index
   * at exactly top_of_helpt, which is equally out of range and equally
   * written through here. */
  if (OLC_ZNUM(d) == NOWHERE || OLC_ZNUM(d) >= top_of_helpt) {
    int i;
    CREATE(new_help_table, struct help_index_element, top_of_helpt + 2);

    for (i = 0; i < top_of_helpt; i++)
      new_help_table[i] = help_table[i];
      
    new_help_table[top_of_helpt++] = *OLC_HELP(d);
    free(help_table);
    help_table = new_help_table;
    help_table_version++;
  } else {
    /* The row is overwritten wholesale, so whatever it was holding has to go
     * first -- the caller hands this function ownership of OLC_HELP's strings
     * (CLEANUP_STRUCTS, not CLEANUP_ALL, with a comment saying so) and the
     * old ones are then owned by nobody at all.
     *
     * keywords belongs to this row alone. entry is shared with the row's
     * duplicates -- that is how one entry answers to several keywords -- so
     * it can only be freed once nothing points at it, and the duplicates are
     * pointed at the replacement in the same pass. They have to be: without
     * that they would spend the rest of the function holding a pointer that
     * has just been freed, and `help <the other keyword>` would read it. */
    int i;
    char *oldkey = help_table[OLC_ZNUM(d)].keywords;
    char *oldentry = help_table[OLC_ZNUM(d)].entry;

    help_table[OLC_ZNUM(d)] = *OLC_HELP(d);

    if (oldentry)
      for (i = 0; i < top_of_helpt; i++)
        if (i != OLC_ZNUM(d) && help_table[i].entry == oldentry)
          help_table[i].entry = OLC_HELP(d)->entry;

    if (oldkey)
      free(oldkey);
    if (oldentry)
      free(oldentry);
  }

  add_to_save_list(HEDIT_PERMISSION, SL_HLP);
  hedit_save_to_disk(d);
  return TRUE;
}

static void hedit_save_to_disk(struct descriptor_data *d)
{
  FILE *fp;
  char buf1[MAX_STRING_LENGTH], index_name[READ_SIZE], tmp_name[READ_SIZE];
  int i;

  snprintf(index_name, sizeof(index_name), "%s%s", HLP_PREFIX, HELP_FILE);

  /* Build the new help file beside the old one and put it in place only
   * once it is whole.  Opening the real file with "w" truncated it before
   * a single entry had been written, and nothing looked at the result of a
   * write or of the close -- which is where a full disk reports itself,
   * the entries before it having only reached the stream's buffer.
   *
   * That is worse here than in the other savers, because of the two lines
   * at the foot of this function: the table is thrown away and read back
   * from the file just written, and index_boot() calls exit(1) when it
   * finds no records.  A failed save therefore took the running MUD down
   * and left behind a help file that would not boot the next one either. */
  if (snprintf(tmp_name, sizeof(tmp_name), "%s.tmp", index_name) >= (int)sizeof(tmp_name)) {
    log("SYSERR: Help file name too long to write beside: %s", index_name);
    return;
  }

  if (!(fp = fopen(tmp_name, "w"))) {
    log("SYSERR: Could not write help index file: %s", strerror(errno));
    return;
  }

  for (i = 0; i < top_of_helpt; i++) {
    if (help_table[i].duplicate)
      continue;
    strncpy(buf1, help_table[i].entry ? help_table[i].entry : "Empty\r\n", sizeof(buf1) - 1);
    strip_cr(buf1);

    /* Forget making a buffer, lets just write the thing now. */
    fprintf(fp, "%s#%d\n", convert_from_tabs(buf1), help_table[i].min_level);
  }
  /* Write final line and close. */
  fprintf(fp, "$~\n");

  if (fflush(fp) == EOF || ferror(fp) || fclose(fp) == EOF) {
    log("SYSERR: Could not write help index file: %s", strerror(errno));
    remove(tmp_name);
    return;
  }

  /* rename() replaces the destination outright on POSIX; the Windows C
   * runtime refuses a name that already exists, which is what the second
   * attempt is for.  objsave.c:548-556 installs rent files the same way. */
  if (rename(tmp_name, index_name)) {
    remove(index_name);
    if (rename(tmp_name, index_name)) {
      log("SYSERR: Could not put the help file in place: %s", strerror(errno));
      remove(tmp_name);
      return;
    }
  }

  remove_from_save_list(HEDIT_PERMISSION, SL_HLP);

  /* Reboot the help files. */
  free_help_table();     
  index_boot(DB_BOOT_HLP);
}

/* The row this editor opened, provided the table it was opened against is
 * still the one in memory.
 *
 * OLC_ZNUM is a help_table index captured when the editor opened, and the
 * table is rebuilt and re-sorted by `reload xhelp` and `reload all` -- which
 * hedit's one-editor lock does not cover, because they are not hedit. Acting
 * on the stale index removes whatever now occupies that slot: a different
 * entry entirely, with nothing in the log to say which one. So this is a
 * version check and a bounds check and nothing more; the reasoning for why
 * that is enough, and why re-deriving the row was not, is inside.
 *
 * Returns -1 if the table has been rebuilt since the editor opened or the
 * index is out of range, which is the honest answer either way. */
static int hedit_find_row(struct descriptor_data *d)
{
  /* OLC_ZNUM is the row hedit_setup_existing read to fill the editor, and
   * nothing moves it afterwards -- the CONFIRM_EDIT 'n' walk happens before
   * setup. So once the table is known not to have been rebuilt, that index
   * still names the entry on the builder's screen, and there is nothing
   * left for a re-resolution to add.
   *
   * This used to re-derive the row from the keyword and compare. That is
   * worse than redundant. Both halves came from the CURRENT table, so it
   * could not tell "unchanged" from "something else slid into this slot":
   * a reload that removed the open entry let another entry's canonical row
   * land on the captured index, and the delete took seven rows the builder
   * never saw. It caught the reload that cost nothing and missed the one
   * that cost an entry -- while also refusing the legitimate case of a
   * builder who walked past a twin with 'n' to reach the one they wanted.
   *
   * The version counter asks the question that was actually being asked.
   * Everything now rests on bumping it wherever help_table is rebuilt or
   * replaced; see its declaration in db.c. */
  if (OLC_HELP_VERSION(d) != help_table_version)
    return -1;

  if (OLC_ZNUM(d) == NOWHERE || OLC_ZNUM(d) >= top_of_helpt)
    return -1;

  return OLC_ZNUM(d);
}

/* Remove a help entry, and every row that shares its text.
 *
 * An entry with N keywords is stored as N rows, all sharing one `entry`
 * pointer -- the loader strdups the text once and copies the struct per
 * keyword. help_table is then sorted by keyword (db.c, hsort), so those rows
 * are NOT adjacent and cannot be found by walking the `duplicate` counter.
 * They are found by the shared pointer instead; deleting one row alone would
 * leave the others pointing at freed text. */
static int hedit_delete_entry(int rnum)
{
  char *text;
  int i, w = 0, removed = 0, keep;

  if (rnum < 0 || rnum >= top_of_helpt)
    return FALSE;

  text = help_table[rnum].entry;

  /* Never leave the table empty. hedit_save_to_disk writes help.hlp and
   * index_boot reads it straight back; on a file with no entries at all
   * that is 'boot error - 0 records counted' and exit(1) -- which takes
   * the running server down mid-command AND fails every boot after it,
   * until somebody edits the file by hand. */
  for (i = 0, keep = 0; i < top_of_helpt; i++)
    if (!(text ? (help_table[i].entry == text) : (i == rnum)))
      keep++;
  if (keep == 0)
    return FALSE;

  for (i = 0; i < top_of_helpt; i++) {
    /* A NULL text would match every empty row, so that case takes only the
     * row it was actually asked for. */
    if (text ? (help_table[i].entry == text) : (i == rnum)) {
      if (help_table[i].keywords)
        free(help_table[i].keywords);
      removed++;
      continue;
    }
    if (w != i)
      help_table[w] = help_table[i];
    w++;
  }

  if (text)
    free(text);		/* shared by the whole set; freed once, after the sweep */

  top_of_helpt = w;
  /* The table changed shape, so anyone holding an index into it is
   * holding a stale one. Nothing outlives this command today, but the
   * counter's whole value is that it is bumped without needing to know
   * that. */
  help_table_version++;
  return removed > 0;
}

/* The main menu. */
static void hedit_disp_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);

  write_to_output(d,
      "%s-- Help file editor\r\n"
      "%s1%s) Entry       :\r\n%s%s"
      "%s2%s) Min Level   : %s%d\r\n"
      "%sX%s) Delete this help entry\r\n"
      "%sQ%s) Quit\r\n"
      "Enter choice : ",
       nrm,
       grn, nrm, yel, OLC_HELP(d)->entry,
       grn, nrm, yel, OLC_HELP(d)->min_level,
       grn, nrm,
       grn, nrm
  );
  OLC_MODE(d) = HEDIT_MAIN_MENU;
}

void hedit_parse(struct descriptor_data *d, char *arg)
{
  char buf[MAX_STRING_LENGTH];
  char *oldtext = NULL;
  int number;

  switch (OLC_MODE(d)) {
  case HEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      /* Formatted before the save, emitted after it. Both halves matter.
       *
       * After, because the save can decline: the old order logged the edit
       * and told the builder it had reached disk before the write was even
       * attempted.
       *
       * Before, because by the time the save returns this string is gone.
       * hedit_save_internally hands OLC_HELP's strings to the table and
       * hedit_save_to_disk ends by rebooting the table from the file, and
       * free_help_table() frees every row's keywords on the way -- these
       * among them. Reading them afterwards formats freed heap, with
       * index_boot reallocating in between.
       *
       * The refusal path returns before the mudlog, so nothing is logged
       * for a save that did not happen. Nor can the builder do anything about
       * it from here: the version is left stale on purpose, so every later
       * save comes back through the same check, and nothing on the menu
       * touches the keyword and text captured at open, which are what the
       * check reads. Only another reload that brings the entry back clears
       * it -- review staged that and the next save landed. The message says
       * both, because "your work is still here" on its own reads as an
       * invitation to try again. */
      snprintf(buf, sizeof(buf), "OLC: %s edits help for %s.", GET_NAME(d->character),
               OLC_HELP(d)->keywords);
      if (!hedit_save_internally(d)) {
        write_to_output(d, "The help files were reloaded while you were editing, and the "
                           "entry you opened can no longer be picked out with certainty "
                           "from what is there now. Writing to the wrong one would "
                           "destroy an entry you never touched, so nothing has been saved. "
                           "Your work is still here, and nothing in this editor can clear "
                           "that. If whoever reloaded can put the entry back and reload "
                           "again, the save will go through; otherwise copy your text out "
                           "before you quit, and add it again.\r\n");
        hedit_disp_menu(d);
        return;
      }
      mudlog(TRUE, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), CMP, "%s", buf);
      write_to_output(d, "Help saved to disk.\r\n");

      /* Do not free strings, just the help structure. */
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

  case HEDIT_CONFIRM_EDIT:
    /* Above the switch, not inside one arm of it. All three arms read
     * help_table[OLC_ZNUM(d)] -- 'y' to fill the editor, 'n' to walk to the
     * next match, and the reprompt to name the entry -- and the index they
     * share was taken before a reload could move it. The reprompt is the one
     * a builder is most likely to reach, since a bare RETURN lands there.
     *
     * Refusing costs nothing here: nothing has been typed yet. That is why
     * this says so and stops, where the save -- which runs after the work is
     * done -- goes looking for the row instead. */
    if (OLC_HELP_VERSION(d) != help_table_version) {
      write_to_output(d, "The help files were reloaded while you were deciding, so "
                         "that is not necessarily the entry you asked for any more. "
                         "Nothing has been changed; run hedit again.\r\n");
      cleanup_olc(d, CLEANUP_ALL);
      return;
    }
    switch (*arg)  {
    case 'y': case 'Y':
      hedit_setup_existing(d, OLC_ZNUM(d));
      break;
    case 'q': case 'Q': 
      cleanup_olc(d, CLEANUP_ALL);
      break;       
    case 'n': case 'N':
      OLC_ZNUM(d)++;
      for (; OLC_ZNUM(d) < top_of_helpt; OLC_ZNUM(d)++)
        if (is_abbrev(OLC_STORAGE(d), help_table[OLC_ZNUM(d)].keywords))
          break;
        else
          OLC_ZNUM(d) = top_of_helpt + 1;

      /* >=, not >. The loop can also leave OLC_ZNUM at exactly top_of_helpt,
       * by running to its own bound rather than through the else -- which is
       * what happens when the last row in the table is a primary keyword and
       * the builder declines it. `>` is false there, so this took the edit
       * branch and read help_table[top_of_helpt]: one element past the end,
       * and a segfault on an ordinary build. The break path always leaves a
       * value below the bound, so this only changes that case, and the add
       * prompt below prints OLC_STORAGE rather than the row. */
      if (OLC_ZNUM(d) >= top_of_helpt) {
        write_to_output(d, "Do you wish to add the '%s' help file? ",
            OLC_STORAGE(d));
        OLC_MODE(d) = HEDIT_CONFIRM_ADD;
      } else {
        write_to_output(d, "Do you wish to edit the '%s' help file? ",
            help_table[OLC_ZNUM(d)].keywords);
        OLC_MODE(d) = HEDIT_CONFIRM_EDIT;
      }     
      break;
    default:
      write_to_output(d, "Invalid choice!\r\n"
                         "Do you wish to edit the '%s' help file? ",
                         help_table[OLC_ZNUM(d)].keywords);
      break;
    }
    return;

  case HEDIT_CONFIRM_ADD:
    switch (*arg)  {
      case 'y': case 'Y':
      hedit_setup_new(d);
      break;
    case 'n': case 'N': case 'q': case 'Q':
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      write_to_output(d, "Invalid choice!\r\n"
                         "Do you wish to add the '%s' help file? ",
                         OLC_STORAGE(d));
      break;
    }
    return;

  case HEDIT_CONFIRM_DELETE: {
    int row;
    switch (*arg) {
    case 'y':
    case 'Y':
      row = hedit_find_row(d);
      if (row >= 0 && hedit_delete_entry(row)) {
        /* hedit_save_to_disk ends by removing this from the save list, so
         * it has to be on it -- hedit_save_internally adds it immediately
         * before saving for exactly this reason. Without the add, every
         * deletion logs "remove_from_save_list: Saved item not found." */
        add_to_save_list(HEDIT_PERMISSION, SL_HLP);
        mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE,
               "OLC: %s deletes help entry '%s'", GET_NAME(d->character),
               OLC_HELP(d)->keywords ? OLC_HELP(d)->keywords : "unnamed");
        write_to_output(d, "Help entry deleted.\r\n");
        /* Rewrites help.hlp from the table and reboots it, which is how
         * every other hedit change reaches disk. */
        hedit_save_to_disk(d);
        cleanup_olc(d, CLEANUP_ALL);
        return;
      }
      /* Nothing was removed, so nothing is thrown away either --
       * cleanup_olc here would discard the builder's unsaved work on top
       * of refusing the delete. */
      if (hedit_find_row(d) >= 0)
        /* Found, so the refusal came from the last-entry guard. Saying it
         * was reloaded would be false twice over, with the entry still on
         * screen underneath. */
        write_to_output(d, "That is the last help entry left. The MUD cannot boot from a help file with none, so it will not be deleted.\r\n");
      else
        write_to_output(d, "That entry is no longer in the help table. It may have been reloaded while you were editing it. Nothing was deleted.\r\n");
      hedit_disp_menu(d);
      return;
    case 'n':
    case 'N':
      hedit_disp_menu(d);
      return;
    default:
      write_to_output(d, "Invalid choice!\r\n");
      write_to_output(d, "Delete this help entry, and every keyword that reaches it? : ");
      return;
    }
  }

  case HEDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d)) {
        /* Something has been modified. */
        write_to_output(d, "Do you wish to save your changes? : ");
        OLC_MODE(d) = HEDIT_CONFIRM_SAVESTRING;
      } else {
        write_to_output(d, "No changes made.\r\n");
        cleanup_olc(d, CLEANUP_ALL);
      }
      break;
    case 'x':
    case 'X':
      if (hedit_find_row(d) < 0) {
        write_to_output(d, "That entry is not in the help table -- either it was never saved, or the table was reloaded while you were editing. Quit without saving.\r\n");
        hedit_disp_menu(d);
        return;
      }
      write_to_output(d, "Delete this help entry, and every keyword that reaches it? : ");
      OLC_MODE(d) = HEDIT_CONFIRM_DELETE;
      return;
    case '1':
      OLC_MODE(d) = HEDIT_ENTRY;
      clear_screen(d);
      send_editor_help(d);
      write_to_output(d, "Enter help entry: (/s saves /h for help)\r\n");
      if (OLC_HELP(d)->entry) {
        write_to_output(d, "%s", OLC_HELP(d)->entry);
        oldtext = strdup(OLC_HELP(d)->entry);
      }
      string_write(d, &OLC_HELP(d)->entry, MAX_MESSAGE_LENGTH, 0, oldtext);
      OLC_VAL(d) = 1;
      break;
    case '2':
      write_to_output(d, "Enter min level : ");
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
    OLC_HELP(d)->keywords = str_udup(arg);
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
    hedit_disp_menu(d);
    break;
  }
}

ACMD(do_helpcheck)
{

  char buf[MAX_STRING_LENGTH];
  int i, count = 0;
  size_t len = 0, nlen;

  for (i = 1; *(complete_cmd_info[i].command) != '\n'; i++) {
    if (complete_cmd_info[i].command_pointer != do_action && complete_cmd_info[i].minimum_level >= 0) {
      if (search_help(complete_cmd_info[i].command, LVL_IMPL) == NOWHERE) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%-20.20s%s", complete_cmd_info[i].command,
                        (++count % 3 ? "" : "\r\n"));
        if (len + nlen >= sizeof(buf))
          break;
        len += nlen;
      }
    }
  }
  if (count % 3 && len < sizeof(buf))
    snprintf(buf + len, sizeof(buf) - len, "\r\n");

  if (ch->desc) {
	if (len == 0)
	 send_to_char(ch, "All commands have help entries.\r\n");
	else {
	 send_to_char(ch, "Commands without help entries:\r\n");
	 page_string(ch->desc, buf, TRUE);
	}
  }
}

ACMD(do_hindex)
{
  int len, len2, count = 0, count2=0, i;
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: hindex <string>\r\n");
    return;
  }

  len = sprintf(buf, "\t1Help index entries beginning with '%s':\t2\r\n", argument);
  len2 = sprintf(buf2, "\t1Help index entries containing '%s':\t2\r\n", argument);
  for (i = 0; i < top_of_helpt; i++) {
    if (is_abbrev(argument, help_table[i].keywords)
        && (GET_LEVEL(ch) >= help_table[i].min_level))
      len +=
          snprintf(buf + len, sizeof(buf) - len, "%-20.20s%s", help_table[i].keywords,
                   (++count % 3 ? "" : "\r\n"));
    else if (strstr(help_table[i].keywords, argument)
        && (GET_LEVEL(ch) >= help_table[i].min_level))
      len2 +=
          snprintf(buf2 + len2, sizeof(buf2) - len2, "%-20.20s%s", help_table[i].keywords,
                   (++count2 % 3 ? "" : "\r\n"));
  }
  if (count % 3)
    len += snprintf(buf + len, sizeof(buf) - len, "\r\n");
  if (count2 % 3)
    len2 += snprintf(buf2 + len2, sizeof(buf2) - len2, "\r\n");

  if (!count)
    len += snprintf(buf + len, sizeof(buf) - len, "  None.\r\n");
  if (!count2)
    snprintf(buf2 + len2, sizeof(buf2) - len2, "  None.\r\n");

  // Join the two strings
  len += snprintf(buf + len, sizeof(buf) - len, "%s", buf2);

  snprintf(buf + len, sizeof(buf) - len, "\t1Applicable Index Entries: \t3%d\r\n"
                                                 "\t1Total Index Entries: \t3%d\tn\r\n", count + count2, top_of_helpt);

  page_string(ch->desc, buf, TRUE);
}
