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
static void hedit_save_internally(struct descriptor_data *);


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

  hedit_disp_menu(d);
}

static void hedit_save_internally(struct descriptor_data *d)
{
  struct help_index_element *new_help_table = NULL;

  if (OLC_ZNUM(d) == NOWHERE) {
    int i;
    CREATE(new_help_table, struct help_index_element, top_of_helpt + 2);

    for (i = 0; i < top_of_helpt; i++)
      new_help_table[i] = help_table[i];
      
    new_help_table[top_of_helpt++] = *OLC_HELP(d);
    free(help_table);
    help_table = new_help_table;
    help_table_version++;
  } else
    help_table[OLC_ZNUM(d)] = *OLC_HELP(d);

  add_to_save_list(HEDIT_PERMISSION, SL_HLP);
  hedit_save_to_disk(d);
}

static void hedit_save_to_disk(struct descriptor_data *d)
{
  FILE *fp;
  char buf1[MAX_STRING_LENGTH], index_name[READ_SIZE];
  int i;

  snprintf(index_name, sizeof(index_name), "%s%s", HLP_PREFIX, HELP_FILE);
  if (!(fp = fopen(index_name, "w"))) {
    log("SYSERR: Could not write help index file");
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
  fclose(fp);

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
      snprintf(buf, sizeof(buf), "OLC: %s edits help for %s.", GET_NAME(d->character),
               OLC_HELP(d)->keywords);
      mudlog(TRUE, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), CMP, "%s", buf);
      write_to_output(d, "Help saved to disk.\r\n");
      hedit_save_internally(d);

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

      if (OLC_ZNUM(d) > top_of_helpt) {
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
