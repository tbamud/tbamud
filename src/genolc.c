/**************************************************************************
*  File: genolc.c                                          Part of tbaMUD *
*  Usage: Generic OLC Library - General.                                  *
*                                                                         *
*  Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer.            *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "comm.h"
#include "shop.h"
#include "oasis.h"
#include "genolc.h"
#include "archive.h"
#include "genwld.h"
#include "genmob.h"
#include "genshp.h"
#include "genzon.h"
#include "genobj.h"
#include "dg_olc.h"
#include "constants.h"
#include "interpreter.h"
#include "act.h"        /* for the space_to_minus function */
#include "modify.h"      /* for smash_tilde */
#include "quest.h"

/* Global variables defined here, used elsewhere */
/* List of zones to be saved. */
struct save_list_data *save_list;

/* Local (file scope) variables */
/* Structure defining all known save types. */
static struct {
  int save_type;
  int (*func)(IDXTYPE rnum);
  const char *message;
} save_types[] = {
  { SL_MOB, save_mobiles , "mobile" },
  { SL_OBJ, save_objects, "object" },
  { SL_SHP, save_shops, "shop" },
  { SL_WLD, save_rooms, "room" },
  { SL_ZON, save_zone, "zone" },
  { SL_CFG, save_config, "config" },
  { SL_QST, save_quests, "quest" },
  { SL_ACT, NULL, "social" },
  { SL_HLP, NULL, "help" },
  { -1, NULL, NULL },
};
/* for Zone Export */

/* Local (file scope) functions */
/* Zone export functions */
/* How an export renders the vnums it writes.
 *
 * `export <zone>` keeps the QQ scheme the info file documents: a vnum the
 * zone owns becomes QQnn, and the recipient replaces QQ with their own
 * zone number. `export <zone> <target>` rewrites those vnums into the
 * target zone's range instead, so the files can be dropped straight in.
 *
 * Either way a reference that leaves the zone becomes ZZnn, because it
 * cannot come with the zone: the destination refuses to load it until
 * someone points it somewhere real, which is the intended outcome. */
struct export_fmt {
  zone_vnum bot;        /* the zone's own vnum window */
  zone_vnum top;
  int target;           /* target zone number, or -1 for the QQ scheme */
  const char *stem;     /* "qq", or the target number, for file names */
  int as_zip;
};

static int export_save_shops(zone_rnum zrnum, const struct export_fmt *fmt);
static int export_save_mobiles(zone_rnum rznum, const struct export_fmt *fmt);
static int export_save_zone(zone_rnum zrnum, const struct export_fmt *fmt);
static int export_save_objects(zone_rnum zrnum, const struct export_fmt *fmt);
static int export_save_rooms(zone_rnum zrnum, const struct export_fmt *fmt);
static int export_save_triggers(zone_rnum zrnum, const struct export_fmt *fmt);
static int export_save_quests(zone_rnum zrnum, const struct export_fmt *fmt);
static int export_archive(const char *filename, const struct export_fmt *fmt);
static int export_mobile_record(mob_vnum mvnum, struct char_data *mob, FILE *fd,
                                const struct export_fmt *fmt);
static void export_script_save_to_disk(FILE *fp, void *item, int type,
                                       const struct export_fmt *fmt);
static int export_info_file(zone_rnum zrnum, const struct export_fmt *fmt);
static int count_zone_exits(zone_rnum zrnum);
static int count_zone_keys(zone_rnum zrnum);
static int key_in_zone(obj_vnum key, zone_rnum zrnum);

int genolc_checkstring(struct descriptor_data *d, char *arg)
{
  smash_tilde(arg);
  parse_at(arg);
  return TRUE;
}

char *str_udup(const char *txt)
{
  return strdup((txt && *txt) ? txt : "undefined");
}

char *str_udupnl(const char *txt)
{
  char *str = NULL, undef[] = "undefined";
  const char *ptr = NULL;
  size_t n;

  ptr = (txt && *txt) ? txt : undef;
  n = strlen(ptr) + 3;

  CREATE(str, char, n);
  strlcpy(str, ptr, n);
  strcat(str, "\r\n");

  return str;
}

/* Original use: to be called at shutdown time. */
int save_all(void)
{
  struct save_list_data *entry, *next;
  int all_saved = TRUE;

  /* A saver takes its own entry off the list when it succeeds and leaves
   * it there when it does not, so the next entry is taken before the call
   * and the walk uses its own pointer rather than the list head.  The old
   * loop re-read the head each time and moved on only when a saver
   * answered a negative number -- which none of them do: every one
   * reports failure as FALSE.  So a zone that would not save was retried
   * without end, and the whole game stalled in saveall, the autosave tick
   * or shutdown.  Now the entry stays for the next attempt and the loop
   * carries on to the rest. */
  for (entry = save_list; entry; entry = next) {
    next = entry->next;

    if (entry->type < 0 || entry->type > SL_MAX) {
      switch (entry->type) {
        case SL_ACT:
          log("Actions not saved - can not autosave. Use 'aedit save'.");
          break;
        case SL_HLP:
          log("Help not saved - can not autosave. Use 'hedit save'.");
          break;
        default:
          log("SYSERR: GenOLC: Invalid save type %d in save list.\n", entry->type);
          break;
      }
      /* Nothing here can save it, so it is dropped rather than reported
       * again on every pass; the old loop dropped it too, by walking past
       * it and leaking the entry. */
      remove_from_save_list(entry->zone, entry->type);
    } else if (!(*save_types[entry->type].func) (real_zone(entry->zone)))
      all_saved = FALSE;
  }

  return all_saved;
}

/* NOTE: This changes the buffer passed in. */
void strip_cr(char *buffer)
{
  int rpos, wpos;

  if (buffer == NULL)
    return;

  for (rpos = 0, wpos = 0; buffer[rpos]; rpos++) {
    buffer[wpos] = buffer[rpos];
    wpos += (buffer[rpos] != '\r');
  }
  buffer[wpos] = '\0';
}

void copy_ex_descriptions(struct extra_descr_data **to, struct extra_descr_data *from)
{
  struct extra_descr_data *wpos;

  CREATE(*to, struct extra_descr_data, 1);
  wpos = *to;

  for (; from; from = from->next, wpos = wpos->next) {
    wpos->keyword = str_udup(from->keyword);
    wpos->description = str_udup(from->description);
    if (from->next)
      CREATE(wpos->next, struct extra_descr_data, 1);
  }
}

void free_ex_descriptions(struct extra_descr_data *head)
{
  struct extra_descr_data *thised, *next_one;

  if (!head) {
    log("free_ex_descriptions: NULL pointer or NULL data.");
    return;
  }

  for (thised = head; thised; thised = next_one) {
    next_one = thised->next;
    if (thised->keyword)
      free(thised->keyword);
    if (thised->description)
      free(thised->description);
    free(thised);
  }
}

int remove_from_save_list(zone_vnum zone, int type)
{
  struct save_list_data *ritem, *temp;

  for (ritem = save_list; ritem; ritem = ritem->next)
    if (ritem->zone == zone && ritem->type == type)
      break;

  if (ritem == NULL) {
    log("SYSERR: remove_from_save_list: Saved item not found. (%d/%d)", zone, type);
    return FALSE;
  }
  REMOVE_FROM_LIST(ritem, save_list, next);
  free(ritem);
  return TRUE;
}

int add_to_save_list(zone_vnum zone, int type)
{
  struct save_list_data *nitem;
  zone_rnum rznum;

  if (type == SL_CFG)
    return FALSE;

  rznum = real_zone(zone);
  if (rznum == NOWHERE || rznum > top_of_zone_table) {
    if (zone != AEDIT_PERMISSION && zone != HEDIT_PERMISSION) {
      log("SYSERR: add_to_save_list: Invalid zone number passed. (%d => %d, 0-%d)", zone, rznum, top_of_zone_table);
      return FALSE;
    }
  }

  for (nitem = save_list; nitem; nitem = nitem->next)
    if (nitem->zone == zone && nitem->type == type)
      return FALSE;

  CREATE(nitem, struct save_list_data, 1);
  nitem->zone = zone;
  nitem->type = type;
  nitem->next = save_list;
  save_list = nitem;
  return TRUE;
}

int in_save_list(zone_vnum zone, int type)
{
  struct save_list_data *nitem;

  for (nitem = save_list; nitem; nitem = nitem->next)
    if (nitem->zone == zone && nitem->type == type)
      return TRUE;

  return FALSE;
}

void free_save_list(void)
{
  struct save_list_data *sld, *next_sld;

  for (sld = save_list; sld; sld = next_sld) {
    next_sld = sld->next;
    free(sld);
  }
}

/* Used from do_show(), ideally. */
ACMD(do_show_save_list)
{
  if (save_list == NULL)
    send_to_char(ch, "All world files are up to date.\r\n");
  else {
    struct save_list_data *item;

    send_to_char(ch, "The following files need saving:\r\n");
    for (item = save_list; item; item = item->next) {
      if (item->type != SL_CFG)
        send_to_char(ch, " - %s data for zone %d.\r\n", save_types[item->type].message, item->zone);
      else
        send_to_char(ch, " - Game configuration data.\r\n");
    }
  }
}

room_vnum genolc_zonep_bottom(struct zone_data *zone)
{
  return zone->bot;
}

zone_vnum genolc_zone_bottom(zone_rnum rznum)
{
  return zone_table[rznum].bot;
}

int sprintascii(char *out, bitvector_t bits)
{
  int i, j = 0;
  /* 32 bits, don't just add letters to try to get more unless your bitvector_t is also as large. */
  char *flags = "abcdefghijklmnopqrstuvwxyzABCDEF";

  for (i = 0; flags[i] != '\0'; i++)
    if (bits & (1 << i))
      out[j++] = flags[i];

  if (j == 0) /* Didn't write anything. */
    out[j++] = '0';

  /* NUL terminate the output string. */
  out[j++] = '\0';
  return j;
}

/* converts illegal filename chars into appropriate equivalents */ 
static void fix_filename(const char *str, char *outbuf, size_t maxlen)
{
  const char *in = str;
  char *out = outbuf;
  size_t count = 0;

  /* The result names a file under world/export/, so it has to be only a
   * file name: a zone called "../../bin/circle" must not send the archive
   * anywhere but into that directory. Anything outside the set below is
   * dropped rather than passed through. */
  while (*in && count + 1 < maxlen) {
    switch (*in) {
      case ' ': *out++ = '_'; count++; break;
      case '(': *out++ = '{'; count++; break;
      case ')': *out++ = '}'; count++; break;

      default:
        if (isalnum((unsigned char) *in) || *in == '_' || *in == '-') {
          *out++ = *in;
          count++;
        }
        break;
    }
    in++;
  }
  *out = '\0';

  /* A name made entirely of characters we dropped would leave an empty
   * string, and "world/export/.tar.gz" is not what anyone meant. */
  if (*outbuf == '\0')
    strlcpy(outbuf, "zone", maxlen);
}

/* How many rendered vnums one record can have in flight at once.  Every
 * argument to an fprintf is evaluated before the call, so a record that
 * emits N vnums holds N of these pointers live together; the widest is the
 * quest record in export_save_quests(), at eight.  A ring sized exactly to
 * its worst case does not fail by breaking, it fails by handing the same
 * buffer out twice and printing one vnum where two were meant, so leave
 * room: the number to raise is this one. */
#define XV_RING 16

/* Render one vnum. Several of these appear in a single fprintf, so the
 * results rotate through a ring of buffers. */
static const char *xv(const struct export_fmt *fmt, int vnum)
{
  static char ring[XV_RING][32];
  static int next = 0;
  char *out = ring[next];
  size_t len = sizeof(ring[0]);

  next = (next + 1) % XV_RING;

  /* The NOTHING/NOWHERE sentinel is not a vnum and must not be marked. */
  if (vnum == NOTHING || vnum == NOWHERE)
    snprintf(out, len, "%d", vnum);
  else if (vnum < fmt->bot || vnum > fmt->top)
    snprintf(out, len, "ZZ%02d", vnum % 100);
  else if (fmt->target < 0)
    snprintf(out, len, "QQ%02d", vnum % 100);
  else
    snprintf(out, len, "%d", fmt->target * 100 + (vnum - fmt->bot));

  return out;
}

/* The same, for a field that may be unset.  "Unset" is spelled three ways:
 * -1, the NOTHING/NOBODY sentinel, and 0 -- which is how most of the stock
 * world writes a doorway with no lock and a container with no key.  db.c
 * folds -1 and the sentinel into NOTHING and leaves 0 alone, so a 0 has to
 * come back as a 0.  None of the three names an object in another zone, so
 * none of them is ZZ'd: a ZZ there would be a cross-zone dependency the
 * recipient cannot resolve, and setup_dir() and parse_object() both exit(1)
 * on a numeric line they cannot read. */
static const char *xv_opt(const struct export_fmt *fmt, int vnum)
{
  if (vnum == NOTHING || vnum == NOBODY || vnum < 0)
    return "-1";
  if (vnum == 0)                     /* no object 0 exists to point at */
    return "0";
  return xv(fmt, vnum);
}

/* The zone's own number: the QQ scheme writes the bare marker, which is
 * why "#QQ" has no digits after it. */
static const char *xz(const struct export_fmt *fmt)
{
  static char buf[16];

  if (fmt->target < 0)
    return "QQ";
  snprintf(buf, sizeof(buf), "%d", fmt->target);
  return buf;
}

/* Open one of the export's files: qq.wld, or 400.wld when renumbering. */
static FILE *export_open(const struct export_fmt *fmt, const char *ext)
{
  char path[READ_SIZE];

  snprintf(path, sizeof(path), "world/export/%s.%s", fmt->stem, ext);
  return fopen(path, "w");
}

/* Export command by Kyle */ 
ACMD(do_export_zone)
{
  zone_rnum zrnum;
  zone_vnum zvnum;
  char zone_name[READ_SIZE], fixed_file_name[READ_SIZE];
  /* Room for the directory prefix and the extension on top of a
   * full-length zone name. */
  char filename[READ_SIZE * 2];
  char arg1[MAX_INPUT_LENGTH], rest[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  char stem[16];
  const char *keyword;
  struct export_fmt fmt;
  int success, target, number;

  /* The MUD chdir()s to its data directory at boot, so every path here is
   * relative to that -- the same paths the export writers below use. */
  const char *path = "world/export/";

  if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMPL) 
    return; 

  skip_spaces(&argument);
  if (!*argument) {
    send_to_char(ch, "Syntax: export <zone vnum> [<target zone>] [zip]\r\n");
    return;
  }

  half_chop(argument, arg1, rest);
  half_chop(rest, arg2, arg3);

  zvnum = atoi(arg1);
  zrnum = real_zone(zvnum);

  if (zrnum == NOWHERE) {
    send_to_char(ch, "Export which zone?\r\n");
    return;
  }

  /* A numeric second argument is the zone to renumber into; anything else
   * is the format keyword, which may also be given third. */
  if (*arg2 && is_number(arg2)) {
    target = atoi(arg2);
    keyword = arg3;
  } else {
    target = -1;
    keyword = arg2;
  }

  if (*keyword && str_cmp(keyword, "zip")) {
    send_to_char(ch, "Syntax: export <zone vnum> [<target zone>] [zip]\r\n");
    return;
  }

  /* A target renumbers the zone into target*100 .. target*100 + its width,
   * and that highest vnum has to be one the recipient can store: inside
   * IDXTYPE, and not IDXTYPE_MAX itself, which is NOWHERE and NOTHING on an
   * unsigned-index build.  Nothing on the load path range-checks a vnum --
   * parse_room() takes an int and truncates at world[room_nr].number, and
   * load_zones() reads the header with %hd -- so a target that does not fit
   * yields files that are quietly wrong rather than files that are refused.
   *
   * Derived from IDXTYPE_MAX rather than written as a literal, so the two
   * cannot drift apart.  The literal this replaces, 655, was already one
   * too many: zone 655 reaches vnum 65599 and only its first 35 slots fit
   * in an unsigned short.  With CIRCLE_UNSIGNED_INDEX 0 it was out by a
   * factor of two. */
  if (*arg2 && is_number(arg2) && target < 0) {
    send_to_char(ch, "A target zone cannot be negative.\r\n");
    return;
  }

  /* Zero fits every bound above and still cannot be used: setup_dir() reads
   * a to_room of 0 as NOWHERE, so every exit leading into the zone's first
   * room would arrive as no exit at all, and nothing on either side would
   * say so.  It is also the one zone every stock world already has. */
  if (target == 0) {
    send_to_char(ch, "Zone 0 cannot be a target: an exit to room 0 loads as "
                     "no exit at all.\r\n");
    return;
  }

  if (target >= 0) {
    long highest = (long)target * 100 +
                   ((long)zone_table[zrnum].top - genolc_zone_bottom(zrnum));

    if (highest >= (long)IDXTYPE_MAX) {
      send_to_char(ch, "Zone %d would put this zone's highest vnum at %ld, "
                       "and no vnum above %ld can be stored.\r\n",
                   target, highest, (long)IDXTYPE_MAX - 1);
      return;
    }
  }

  fmt.bot = genolc_zone_bottom(zrnum);
  fmt.top = zone_table[zrnum].top;
  fmt.target = target;
  fmt.as_zip = (*keyword != '\0');
  if (target < 0)
    strlcpy(stem, "qq", sizeof(stem));
  else
    snprintf(stem, sizeof(stem), "%d", target);
  fmt.stem = stem;

  if (zone_table[zrnum].top - genolc_zone_bottom(zrnum) >= 100)
    send_to_char(ch, "Note: this zone is wider than the 100-vnum grid the "
                     "export scheme assumes.\r\n"); 

  /* If we fail, it might just be because the directory didn't exist.  Can't 
   * hurt to try again. Do it silently though ( no logs ). */ 
  if (!export_info_file(zrnum, &fmt) && archive_mkdir("world/export") != 0) {
    send_to_char(ch, "Failed to create export directory.\r\n");
    return;
  }

  /* Every writer's result is kept: assigning to one `success` meant only
   * the LAST one decided whether the archive was built, so a failure
   * anywhere else was reported and then packaged anyway. */
  success = TRUE;
  if (!export_info_file(zrnum, &fmt)) {
    send_to_char(ch, "Info file not saved!\r\n");
    success = FALSE;
  }
  if (!export_save_shops(zrnum, &fmt)) {
    send_to_char(ch, "Shops not saved!\r\n");
    success = FALSE;
  }
  if (!export_save_mobiles(zrnum, &fmt)) {
    send_to_char(ch, "Mobiles not saved!\r\n");
    success = FALSE;
  }
  if (!export_save_objects(zrnum, &fmt)) {
    send_to_char(ch, "Objects not saved!\r\n");
    success = FALSE;
  }
  if (!export_save_zone(zrnum, &fmt)) {
    send_to_char(ch, "Zone info not saved!\r\n");
    success = FALSE;
  }
  if (!export_save_rooms(zrnum, &fmt)) {
    send_to_char(ch, "Rooms not saved!\r\n");
    success = FALSE;
  }
  if (!export_save_triggers(zrnum, &fmt)) {
    send_to_char(ch, "Triggers not saved!\r\n");
    success = FALSE;
  }
  if (!export_save_quests(zrnum, &fmt)) {
    send_to_char(ch, "Quests not saved!\r\n");
    success = FALSE;
  }

  /* If anything went wrong, don't try to tar the files. */ 
  if (success) { 
    send_to_char(ch, "Individual files saved to /lib/world/export.\r\n"); 
    snprintf(zone_name, sizeof(zone_name), "%s", zone_table[zrnum].name); 
  } else { 
    send_to_char(ch, "Ran into problems writing to files.\r\n"); 
    return; 
  }
  /* Make sure the name of the zone doesn't make the filename illegal. */
  fix_filename(zone_name, fixed_file_name, sizeof(fixed_file_name));

  /* "<zone #>_<zone name>.tgz" is what the help has promised since it was
   * written; the code wrote "<zone name>.tar.gz", so two zones sharing a
   * name overwrote each other in world/export/. The number is the target
   * when renumbering, so the two forms do not collide either. */
  number = (target < 0) ? zone_table[zrnum].number : target;
  snprintf(filename, sizeof(filename), "%s%d_%s.%s", path, number,
           fixed_file_name, fmt.as_zip ? "zip" : "tar.gz");
  if (!export_archive(filename, &fmt)) {
    send_to_char(ch, "Failed to write the archive.\r\n");
    return;
  }

  send_to_char(ch, "Archive written to \"%s\"\r\n", filename);
}

/* Build <filename> from the files export just wrote. Nothing here runs a
 * shell: archive.c emits the tar and gzip bytes directly, which is what
 * lets the command work on every platform rather than being #ifdef'd out
 * of the Windows port, and leaves no command line for a zone name to be
 * interpolated into. */
static int export_archive(const char *filename, const struct export_fmt *fmt)
{
  static const char *parts[] = { "info", "wld", "zon", "mob", "obj", "trg", "shp", "qst" };
  struct archive_member members[8];
  struct byte_buf tarred, gzipped;
  char member_path[READ_SIZE];
  unsigned char *bodies[8];
  time_t now = time(0);
  int i, count = 0, ok = TRUE;
  FILE *fp;

  for (i = 0; i < 8; i++) {
    long size;

    snprintf(member_path, sizeof(member_path), "world/export/%s.%s", fmt->stem, parts[i]);
    if (!(fp = fopen(member_path, "rb"))) {
      mudlog(BRF, LVL_GOD, TRUE, "SYSERR: export_archive: cannot read %s", member_path);
      ok = FALSE;
      break;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    CREATE(bodies[count], unsigned char, size ? size : 1);
    members[count].len = fread(bodies[count], 1, size, fp);
    fclose(fp);

    snprintf(member_path, sizeof(member_path), "%s.%s", fmt->stem, parts[i]);
    members[count].name = strdup(member_path);
    members[count].data = bodies[count];
    count++;
  }

  if (ok) {
    buf_init(&tarred);
    buf_init(&gzipped);
    if (fmt->as_zip)
      archive_zip(&gzipped, members, count, now);
    else {
      archive_tar(&tarred, members, count, now);
      archive_gzip(&gzipped, tarred.data, tarred.len, now);
    }

    if (!(fp = fopen(filename, "wb")))
      ok = FALSE;
    else {
      ok = (fwrite(gzipped.data, 1, gzipped.len, fp) == gzipped.len);
      fclose(fp);
    }
    buf_free(&tarred);
    buf_free(&gzipped);
  }

  for (i = 0; i < count; i++) {
    free(bodies[i]);
    free((char *) members[i].name);
  }
  return ok;
}

/* An exit that leaves the zone is something the recipient has to reattach
 * by hand, so the info file has a section listing them -- but that section
 * was gated on a file-static export_save_rooms only set afterwards, which
 * meant it never ran and every info file ever produced claimed the zone
 * was self-contained. Work the answer out here instead. */
/* Is this object vnum one of the zone's own? */
static int key_in_zone(obj_vnum key, zone_rnum zrnum)
{
  return key >= genolc_zone_bottom(zrnum) && key <= zone_table[zrnum].top;
}

/* Does this key name an object the zone cannot take with it?  Only those
 * have to be ZZ'd and reported.  NOTHING and 0 are both "no lock" rather
 * than a reference somewhere else -- most of the stock world spells it 0 --
 * and counting those would list every ordinary doorway in the zone. */
static int key_is_foreign(obj_vnum key, zone_rnum zrnum)
{
  return key != NOTHING && key > 0 && !key_in_zone(key, zrnum);
}

/* Doors locked with a key from another zone are ZZ'd like an exit that
 * leaves the zone, and are just as fatal to the recipient's boot, so the
 * info file has to list them too. */
static int count_zone_keys(zone_rnum zrnum)
{
  int i, j, found = 0;

  for (i = genolc_zone_bottom(zrnum); i <= zone_table[zrnum].top; i++) {
    room_rnum rnum = real_room(i);

    if (rnum == NOWHERE)
      continue;

    for (j = 0; j < DIR_COUNT; j++) {
      struct room_direction_data *pexit = R_EXIT(&world[rnum], j);

      if (pexit && key_is_foreign(pexit->key, zrnum))
        found++;
    }
  }
  return found;
}

static int count_zone_exits(zone_rnum zrnum)
{
  int i, j, found = 0;

  for (i = genolc_zone_bottom(zrnum); i <= zone_table[zrnum].top; i++) {
    room_rnum rnum = real_room(i);

    if (rnum == NOWHERE)
      continue;

    for (j = 0; j < DIR_COUNT; j++) {
      struct room_direction_data *pexit = R_EXIT(&world[rnum], j);

      if (!pexit || pexit->to_room == NOWHERE)
        continue;
      if (world[pexit->to_room].zone != zrnum)
        found++;
    }
  }
  return found;
}

static int export_info_file(zone_rnum zrnum, const struct export_fmt *fmt)
{
  int i;
  FILE *info_file;

  if (!(info_file = export_open(fmt, "info"))) {
    mudlog(BRF, LVL_GOD, TRUE, "SYSERR: export_info_file : Cannot open file!");
    return FALSE;
  } else if (fprintf(info_file, "tbaMUD Area file.\n") < 0) {
    mudlog(BRF, LVL_GOD, TRUE, "SYSERR: export_info_file: Cannot write to file!");
    fclose(info_file);
    return FALSE;
  }

  fprintf(info_file, "The files accompanying this info file contain the area: %s\n", zone_table[zrnum].name);
  fprintf(info_file, "It was written by: %s.\n\n", zone_table[zrnum].builders);
  fprintf(info_file, "The author has given permission to distribute the area, provided credit is\n");
  fprintf(info_file, "given. The area may be modified as you see fit, except you are not allowed to\n");
  fprintf(info_file, "remove the builder name or credits.\n\n");
  fprintf(info_file, "Implementation:\n");
  if (fmt->target < 0) {
    fprintf(info_file, "1. All the files have been QQ'ed. This means all occurences of the zone number\n");
    fprintf(info_file, "   have been changed to QQ. In other words, if you decide to have this zone as\n");
    fprintf(info_file, "   zone 123, replace all occurences of QQ with 123 and rename the qq.zon file\n");
    fprintf(info_file, "   to 123.zon (etc.). And of course add 123.zon to the respective index file.\n");
  } else {
    /* Renumbered: there is no QQ to replace and no qq.zon to rename, so the
     * step above would send the recipient looking for neither. */
    fprintf(info_file, "1. The files are numbered for zone %d already. The vnums inside them and the\n", fmt->target);
    fprintf(info_file, "   file names are the ones this zone will use, so there is nothing to find\n");
    fprintf(info_file, "   and replace and nothing to rename -- add %d.zon to the respective index\n", fmt->target);
    fprintf(info_file, "   file (etc.) and the zone is in place.\n");
  }
  if (count_zone_exits(zrnum)) {
    fprintf(info_file, "2. Exits out of this zone have been ZZ'd. So all doors leading out have ZZ??\n");
    fprintf(info_file, "   instead of the room vnum (?? are numbers 00 - 99).\n");
    fprintf(info_file, "   In this zone, the exit rooms in question are:\n");

    for (i = genolc_zone_bottom(zrnum); i <= zone_table[zrnum].top; i++) {
      room_rnum rnum = real_room(i);
      struct room_data *room;
      int j;

      if (rnum == NOWHERE)
        continue;

      room = &world[rnum];

      for (j = 0; j < DIR_COUNT; j++) {
        if (!R_EXIT(room, j))
          continue;

        if (R_EXIT(room, j)->to_room == NOWHERE || world[R_EXIT(room, j)->to_room].zone == zrnum)
          continue;

        fprintf(info_file, "      Room %s : Exit to the %s\n",
                           xv(fmt, room->number), dirs[j]);
      }
    }
  } else {
    fprintf(info_file, "2. This area doesn't have any exits _out_ of the zone.\n");
    fprintf(info_file, "   More info on connections can be found in the zone description room (%s).\n",
            xv(fmt, genolc_zone_bottom(zrnum)));
  }

  if (count_zone_keys(zrnum)) {
    fprintf(info_file, "\n3. Some doors here are locked with keys that belong to other zones.\n");
    fprintf(info_file, "   Those key vnums have been ZZ'd for the same reason as the exits\n");
    fprintf(info_file, "   above, and need pointing at real objects before the zone will load:\n");

    for (i = genolc_zone_bottom(zrnum); i <= zone_table[zrnum].top; i++) {
      room_rnum rnum = real_room(i);
      int j;

      if (rnum == NOWHERE)
        continue;

      for (j = 0; j < DIR_COUNT; j++) {
        struct room_direction_data *pexit = R_EXIT(&world[rnum], j);

        if (!pexit || !key_is_foreign(pexit->key, zrnum))
          continue;

        fprintf(info_file, "      Room %s : %s door, key was object %d\n",
                xv(fmt, world[rnum].number), dirs[j], pexit->key);
      }
    }
  }

  fprintf(info_file, "\nAdditional zone information is available in the zone description room %s.\n",
          xv(fmt, genolc_zone_bottom(zrnum)));
  fprintf(info_file, "The Builder's Academy is maintaining and improving these zones. Any typo or\n");
  fprintf(info_file, "bug reports should be reported to rumble@tbamud.com or stop by The Builder Academy\n");
  fprintf(info_file, "port telnet://tbamud.com:9091\n");
  fprintf(info_file, "\nAnyone interested in submitting areas or helping improve the existing ones\n");
  fprintf(info_file, "please stop by TBA and talk to Rumble.\n\n");
  fprintf(info_file, "We at The Builder's Academy hope you will enjoy using the area.\n\n");

  fprintf(info_file, "Rumble - Admin of TBA\n");
  fprintf(info_file, "Welcor - Coder of TBA\n");
  fprintf(info_file, "\ntelnet://tbamud.com:9091/\n");

  fclose(info_file);
  return TRUE;
}

static int export_save_shops(zone_rnum zrnum, const struct export_fmt *fmt)
{
  int i, j, rshop;
  FILE *shop_file;
  struct shop_data *shop;

  if (!(shop_file = export_open(fmt, "shp"))) {
    mudlog(BRF, LVL_GOD, TRUE, "SYSERR: export_save_shops : Cannot open shop file!");
    return FALSE;
  } else if (fprintf(shop_file, "CircleMUD v3.0 Shop File~\n") < 0) {
    mudlog(BRF, LVL_GOD, TRUE, "SYSERR: export_save_shops: Cannot write to shop file!");
    fclose(shop_file);
    return FALSE;
  }
  /* Search database for shops in this zone. */
  for (i = genolc_zone_bottom(zrnum); i <= zone_table[zrnum].top; i++) {
    if ((rshop = real_shop(i)) != NOWHERE) {
      fprintf(shop_file, "#%s~\n", xv(fmt, i));
      shop = &shop_index[rshop];

      /* Save the products. */
      for (j = 0; S_PRODUCT(shop, j) != NOTHING; j++) {
        if (obj_index[S_PRODUCT(shop, j)].vnum < genolc_zone_bottom(zrnum) ||
            obj_index[S_PRODUCT(shop, j)].vnum > zone_table[zrnum].top)
          continue;

	fprintf(shop_file, "%s\n", xv(fmt, obj_index[S_PRODUCT(shop, j)].vnum));
      }
      fprintf(shop_file, "-1\n");

      /* Save the rates. */
      fprintf(shop_file, "%1.2f\n"
                         "%1.2f\n",
                         S_BUYPROFIT(shop),
                         S_SELLPROFIT(shop));

      /* Save the buy types and namelists. */
      for (j = 0;S_BUYTYPE(shop, j) != NOTHING; j++)
        fprintf(shop_file, "%d%s\n",
                S_BUYTYPE(shop, j),
		S_BUYWORD(shop, j) ? S_BUYWORD(shop, j) : "");
      fprintf(shop_file, "-1\n");

      /* Save messages. Added some defaults as sanity checks. */
      fprintf(shop_file,
	      "%s~\n"
	      "%s~\n"
	      "%s~\n"
	      "%s~\n"
	      "%s~\n"
	      "%s~\n"
	      "%s~\n"
	      "%d\n"
	      "%ld\n"
	      "%s\n"
	      "%d\n",
	      S_NOITEM1(shop) ? S_NOITEM1(shop) : "%s Ke?!",
	      S_NOITEM2(shop) ? S_NOITEM2(shop) : "%s Ke?!",
	      S_NOBUY(shop) ? S_NOBUY(shop) : "%s Ke?!",
	      S_NOCASH1(shop) ? S_NOCASH1(shop) : "%s Ke?!",
	      S_NOCASH2(shop) ? S_NOCASH2(shop) : "%s Ke?!",
	      S_BUY(shop) ? S_BUY(shop) : "%s Ke?! %d?",
	      S_SELL(shop) ? S_SELL(shop) : "%s Ke?! %d?",
	      S_BROKE_TEMPER(shop),
	      S_BITVECTOR(shop),
	      S_KEEPER(shop) == NOBODY ? "-1"
	                               : xv(fmt, mob_index[S_KEEPER(shop)].vnum),
	      S_NOTRADE(shop)
	      );

      /* Save the rooms. */
      for (j = 0;S_ROOM(shop, j) != NOWHERE; j++) {
        if (S_ROOM(shop, j) < genolc_zone_bottom(zrnum) ||
            S_ROOM(shop, j) > zone_table[zrnum].top)
          continue;

        fprintf(shop_file, "%s\n", xv(fmt, S_ROOM(shop, j)));
      }
      fprintf(shop_file, "-1\n");

      /* Save open/closing times. */
      fprintf(shop_file, "%d\n%d\n%d\n%d\n", S_OPEN1(shop), S_CLOSE1(shop),
		S_OPEN2(shop), S_CLOSE2(shop));
    }
  }
  fprintf(shop_file, "$~\n");
  fclose(shop_file);

  return TRUE;
}

static int export_save_mobiles(zone_rnum rznum, const struct export_fmt *fmt)
{
  FILE *mob_file;
  mob_vnum i;
  mob_rnum rmob;

  if (!(mob_file = export_open(fmt, "mob"))) {
    mudlog(BRF, LVL_GOD, TRUE, "SYSERR: export_save_mobiles : Cannot open file!");
    return FALSE;
  }

  for (i = genolc_zone_bottom(rznum); i <= zone_table[rznum].top; i++) {
    if ((rmob = real_mobile(i)) == NOBODY)
      continue;
    check_mobile_strings(&mob_proto[rmob]);
    if (export_mobile_record(i, &mob_proto[rmob], mob_file, fmt) < 0)
      log("SYSERR: export_save_mobiles: Error writing mobile #%d.", i);
  }
  fputs("$\n", mob_file);
  fclose(mob_file);

  return TRUE;
}

static int export_mobile_record(mob_vnum mvnum, struct char_data *mob, FILE *fd,
                                const struct export_fmt *fmt)
{

  char ldesc[MAX_STRING_LENGTH];
  char ddesc[MAX_STRING_LENGTH];

  ldesc[MAX_STRING_LENGTH - 1] = '\0';
  ddesc[MAX_STRING_LENGTH - 1] = '\0';
  strip_cr(strncpy(ldesc, GET_LDESC(mob), MAX_STRING_LENGTH - 1));
  strip_cr(strncpy(ddesc, GET_DDESC(mob), MAX_STRING_LENGTH - 1));

  fprintf(fd,	"#%s\n"
		"%s%c\n"
		"%s%c\n"
		"%s%c\n"
		"%s%c\n",
	xv(fmt, mvnum),
	GET_ALIAS(mob), STRING_TERMINATOR,
	GET_SDESC(mob), STRING_TERMINATOR,
	ldesc, STRING_TERMINATOR,
	ddesc, STRING_TERMINATOR
  );

  fprintf(fd, "%d %d %d %d %d %d %d %d %d E\n"
      "%d %d %d %dd%d+%d %dd%d+%d\n",
      MOB_FLAGS(mob)[0], MOB_FLAGS(mob)[1],
      MOB_FLAGS(mob)[2], MOB_FLAGS(mob)[3],
      AFF_FLAGS(mob)[0], AFF_FLAGS(mob)[1],
      AFF_FLAGS(mob)[2], AFF_FLAGS(mob)[3],
      GET_ALIGNMENT(mob),
      GET_LEVEL(mob), 20 - GET_HITROLL(mob), GET_AC(mob) / 10, GET_HIT(mob),
      GET_MANA(mob), GET_MOVE(mob), GET_NDD(mob), GET_SDD(mob),
      GET_DAMROLL(mob));

  fprintf(fd, 	"%d %d\n"
		"%d %d %d\n",
		GET_GOLD(mob), GET_EXP(mob),
		GET_POS(mob), GET_DEFAULT_POS(mob), GET_SEX(mob)
  );

  if (write_mobile_espec(mvnum, mob, fd) < 0)
    log("SYSERR: GenOLC: Error writing E-specs for mobile #%d.", mvnum);

  export_script_save_to_disk(fd, mob, MOB_TRIGGER, fmt);

  return TRUE;
}

static int export_save_zone(zone_rnum zrnum, const struct export_fmt *fmt)
{
  int subcmd;
  FILE *zone_file;

  if (!(zone_file = export_open(fmt, "zon"))) {
    mudlog(BRF, LVL_GOD, TRUE, "SYSERR: export_save_zone : Cannot open file!");
    return FALSE;
  }

  /* Print zone header to file. */
  fprintf(zone_file, "#%s\n"
                 "%s~\n"
                 "%s~\n"
                 "%s %s %d %d\n",
	  xz(fmt),
	  (zone_table[zrnum].builders && *zone_table[zrnum].builders)
		? zone_table[zrnum].builders : "None.",
	  (zone_table[zrnum].name && *zone_table[zrnum].name)
		? zone_table[zrnum].name : "undefined",
          xv(fmt, genolc_zone_bottom(zrnum)),
	  xv(fmt, zone_table[zrnum].top),
	  zone_table[zrnum].lifespan,
	  zone_table[zrnum].reset_mode
	  );

	/* Handy Quick Reference Chart for Zone Values.
	 *
	 * Field #1    Field #3   Field #4  Field #5
	 * -------------------------------------------------
	 * M (Mobile)  Mob-Vnum   Wld-Max   Room-Vnum
	 * O (Object)  Obj-Vnum   Wld-Max   Room-Vnum
	 * G (Give)    Obj-Vnum   Wld-Max   Unused
	 * E (Equip)   Obj-Vnum   Wld-Max   EQ-Position
	 * P (Put)     Obj-Vnum   Wld-Max   Target-Obj-Vnum
	 * D (Door)    Room-Vnum  Door-Dir  Door-State
	 * R (Remove)  Room-Vnum  Obj-Vnum  Unused
         * T (Trigger) Trig-type  Trig-Vnum Room-Vnum
         * V (var)     Trig-type  Context   Room-Vnum Varname Value
	 * ------------------------------------------------- */

  for (subcmd = 0; ZCMD(zrnum, subcmd).command != 'S'; subcmd++) {
    switch (ZCMD(zrnum, subcmd).command) {
    case 'M':
      fprintf(zone_file, "M %d %s %d %s \t(%s)\n",
		ZCMD(zrnum, subcmd).if_flag,
		xv(fmt, mob_index[ZCMD(zrnum, subcmd).arg1].vnum),
		ZCMD(zrnum, subcmd).arg2,
		xv(fmt, world[ZCMD(zrnum, subcmd).arg3].number),
		mob_proto[ZCMD(zrnum, subcmd).arg1].player.short_descr);
      break;
    case 'O':
      fprintf(zone_file, "O %d %s %d %s \t(%s)\n",
		ZCMD(zrnum, subcmd).if_flag,
		xv(fmt, obj_index[ZCMD(zrnum, subcmd).arg1].vnum),
		ZCMD(zrnum, subcmd).arg2,
		xv(fmt, world[ZCMD(zrnum, subcmd).arg3].number),
		obj_proto[ZCMD(zrnum, subcmd).arg1].short_description);
      break;
    case 'G':
      fprintf(zone_file, "G %d %s %d -1 \t(%s)\n",
		ZCMD(zrnum, subcmd).if_flag,
                xv(fmt, obj_index[ZCMD(zrnum, subcmd).arg1].vnum),
                ZCMD(zrnum, subcmd).arg2,
                obj_proto[ZCMD(zrnum, subcmd).arg1].short_description);
      break;
    case 'E':
      fprintf(zone_file, "E %d %s %d %d \t(%s)\n",
		ZCMD(zrnum, subcmd).if_flag,
		 xv(fmt, obj_index[ZCMD(zrnum, subcmd).arg1].vnum),
		 ZCMD(zrnum, subcmd).arg2,
		 ZCMD(zrnum, subcmd).arg3,
		 obj_proto[ZCMD(zrnum, subcmd).arg1].short_description);
      break;
    case 'P':
      fprintf(zone_file, "P %d %s %d %s \t(%s)\n",
		ZCMD(zrnum, subcmd).if_flag,
		xv(fmt, obj_index[ZCMD(zrnum, subcmd).arg1].vnum),
		ZCMD(zrnum, subcmd).arg2,
		xv(fmt, obj_index[ZCMD(zrnum, subcmd).arg3].vnum),
		obj_proto[ZCMD(zrnum, subcmd).arg1].short_description);
      break;
    case 'D':
      fprintf(zone_file, "D %d %s %d %d \t(%s)\n",
		ZCMD(zrnum, subcmd).if_flag,
		xv(fmt, world[ZCMD(zrnum, subcmd).arg1].number),
		ZCMD(zrnum, subcmd).arg2,
		ZCMD(zrnum, subcmd).arg3,
		world[ZCMD(zrnum, subcmd).arg1].name);
      break;
    case 'R':
      fprintf(zone_file, "R %d %s %s -1 \t(%s)\n",
		ZCMD(zrnum, subcmd).if_flag,
		xv(fmt, world[ZCMD(zrnum, subcmd).arg1].number),
		xv(fmt, obj_index[ZCMD(zrnum, subcmd).arg2].vnum),
		obj_proto[ZCMD(zrnum, subcmd).arg2].short_description);
      break;
    case 'T':
      fprintf(zone_file, "T %d %d %s %s \t(%s)\n",
		ZCMD(zrnum, subcmd).if_flag,
		ZCMD(zrnum, subcmd).arg1,
		xv(fmt, trig_index[ZCMD(zrnum, subcmd).arg2]->vnum),
		xv(fmt, world[ZCMD(zrnum, subcmd).arg3].number),
		GET_TRIG_NAME(trig_index[ZCMD(zrnum, subcmd).arg2]->proto));
      break;
    case 'V':
      fprintf(zone_file, "V %d %d %d %s %s %s\n",
              ZCMD(zrnum, subcmd).if_flag,
              ZCMD(zrnum, subcmd).arg1,
              ZCMD(zrnum, subcmd).arg2,
              xv(fmt, world[ZCMD(zrnum, subcmd).arg3].number),
              ZCMD(zrnum, subcmd).sarg1,
              ZCMD(zrnum, subcmd).sarg2);
      break;
    case '*':
      /* Invalid commands are replaced with '*' - Ignore them. */
      continue;
    default:
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: export_save_zone(): Unknown cmd '%c' - NOT saving", ZCMD(zrnum, subcmd).command);
      continue;
    }
  }
  fputs("S\n$\n", zone_file);
  fclose(zone_file);

  return TRUE;
}

static int export_save_objects(zone_rnum zrnum, const struct export_fmt *fmt)
{
  char buf[MAX_STRING_LENGTH];
  char ebuf1[MAX_STRING_LENGTH], ebuf2[MAX_STRING_LENGTH], ebuf3[MAX_STRING_LENGTH], ebuf4[MAX_STRING_LENGTH];
  char wbuf1[MAX_STRING_LENGTH], wbuf2[MAX_STRING_LENGTH], wbuf3[MAX_STRING_LENGTH], wbuf4[MAX_STRING_LENGTH];
  char pbuf1[MAX_STRING_LENGTH], pbuf2[MAX_STRING_LENGTH], pbuf3[MAX_STRING_LENGTH], pbuf4[MAX_STRING_LENGTH];
  obj_rnum ornum;
  obj_vnum ovnum;
  int i;
  FILE *obj_file;
  struct obj_data *obj;
  struct extra_descr_data *ex_desc;

  if (!(obj_file = export_open(fmt, "obj"))) {
    mudlog(BRF, LVL_GOD, TRUE, "SYSERR: export_save_objects : Cannot open file!");
    return FALSE;
  }
  /* Start running through all objects in this zone. */
  for (ovnum = genolc_zone_bottom(zrnum); ovnum <= zone_table[zrnum].top; ovnum++) {
    if ((ornum = real_object(ovnum)) != NOTHING) {
      if ((obj = &obj_proto[ornum])->action_description) {
	strncpy(buf, obj->action_description, sizeof(buf) - 1);
	strip_cr(buf);
      } else
	*buf = '\0';

      fprintf(obj_file,
	      "#%s\n"
	      "%s~\n"
	      "%s~\n"
	      "%s~\n"
	      "%s~\n",

	      xv(fmt, GET_OBJ_VNUM(obj)),
	      (obj->name && *obj->name) ? obj->name : "undefined",
	      (obj->short_description && *obj->short_description) ? obj->short_description : "undefined",
	      (obj->description && *obj->description) ?	obj->description : "undefined",
	      buf);

      sprintascii(ebuf1, GET_OBJ_EXTRA(obj)[0]);
      sprintascii(ebuf2, GET_OBJ_EXTRA(obj)[1]);
      sprintascii(ebuf3, GET_OBJ_EXTRA(obj)[2]);
      sprintascii(ebuf4, GET_OBJ_EXTRA(obj)[3]);
      sprintascii(wbuf1, GET_OBJ_WEAR(obj)[0]);
      sprintascii(wbuf2, GET_OBJ_WEAR(obj)[1]);
      sprintascii(wbuf3, GET_OBJ_WEAR(obj)[2]);
      sprintascii(wbuf4, GET_OBJ_WEAR(obj)[3]);
      sprintascii(pbuf1, GET_OBJ_AFFECT(obj)[0]);
      sprintascii(pbuf2, GET_OBJ_AFFECT(obj)[1]);
      sprintascii(pbuf3, GET_OBJ_AFFECT(obj)[2]);
      sprintascii(pbuf4, GET_OBJ_AFFECT(obj)[3]);

      fprintf(obj_file,
          "%d %s %s %s %s %s %s %s %s %s %s %s %s\n",
          GET_OBJ_TYPE(obj),
          ebuf1, ebuf2, ebuf3, ebuf4,
          wbuf1, wbuf2, wbuf3, wbuf4,
          pbuf1, pbuf2, pbuf3, pbuf4);

      if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
        fprintf(obj_file,
                "%d %d %d %d\n",
	        GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3));
      else
        fprintf(obj_file,
                "%d %d %s %d\n",
	        GET_OBJ_VAL(obj, 0),
	        GET_OBJ_VAL(obj, 1),
	        xv_opt(fmt, GET_OBJ_VAL(obj, 2)), /* key */
	        GET_OBJ_VAL(obj, 3));

      fprintf(obj_file,
	      "%d %d %d %d %d\n",
	      GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_LEVEL(obj),
	      GET_OBJ_TIMER(obj));

      /* Do we have script(s) attached? */
      export_script_save_to_disk(obj_file, obj, OBJ_TRIGGER, fmt);

      /* Do we have extra descriptions? */
      if (obj->ex_description) {	/* Yes, save them too. */
	for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next) {
	  /* Sanity check to prevent nasty protection faults. */
	  if (!ex_desc->keyword || !ex_desc->description || !*ex_desc->keyword || !*ex_desc->description) {
	    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: OLC: export_save_objects: Corrupt ex_desc!");
	    continue;
	  }
	  strncpy(buf, ex_desc->description, sizeof(buf) - 1);
	  strip_cr(buf);
	  fprintf(obj_file, "E\n"
		  "%s~\n"
		  "%s~\n", ex_desc->keyword, buf);
	}
      }
      /* Do we have affects? */
      for (i = 0; i < MAX_OBJ_AFFECT; i++)
	if (obj->affected[i].modifier)
	  fprintf(obj_file, "A\n"
		            "%d %d\n",
		  obj->affected[i].location,
		  obj->affected[i].modifier);
    }
  }

  /* Write the final line, close the file. */
  fprintf(obj_file, "$~\n");
  fclose(obj_file);

  return TRUE;
}

static int export_save_rooms(zone_rnum zrnum, const struct export_fmt *fmt)
{
  int i;
  const char *key_tag;
  int key_num;
  struct room_data *room;
  FILE *room_file;
  char buf[MAX_STRING_LENGTH];
  char buf1[MAX_STRING_LENGTH];

  if (!(room_file = export_open(fmt, "wld"))) {
    mudlog(BRF, LVL_GOD, TRUE, "SYSERR: export_save_rooms : Cannot open file!");
    return FALSE;
  }

  for (i = genolc_zone_bottom(zrnum); i <= zone_table[zrnum].top; i++) {
    room_rnum rnum;

    if ((rnum = real_room(i)) != NOWHERE) {
      int j;

      room = &world[rnum];

      /* Copy the description and strip off trailing newlines. */
      strncpy(buf, room->description ? room->description : "Empty room.", sizeof(buf)-1 );
      strip_cr(buf);

      /* Save the numeric and string section of the file. */
      fprintf(room_file, 	"#%s\n"
			"%s%c\n"
			"%s%c\n"
			"%s %d %d %d %d %d\n",
		xv(fmt, room->number),
		room->name ? room->name : "Untitled", STRING_TERMINATOR,
		buf, STRING_TERMINATOR,
		xz(fmt),
		room->room_flags[0], room->room_flags[1],
                room->room_flags[2], room->room_flags[3], room->sector_type
      );

      /* Now you write out the exits for the room. */
      for (j = 0; j < DIR_COUNT; j++) {
	if (R_EXIT(room, j)) {
	  int dflag;
	  if (R_EXIT(room, j)->general_description) {
	    strncpy(buf, R_EXIT(room, j)->general_description, sizeof(buf)-1);
	    strip_cr(buf);
	  } else
	    *buf = '\0';

	  /* Figure out door flag. */
	  if (IS_SET(R_EXIT(room, j)->exit_info, EX_ISDOOR)) {
	    if (IS_SET(R_EXIT(room, j)->exit_info, EX_PICKPROOF))
	      dflag = 2;
	    else
	      dflag = 1;

	    if (IS_SET(R_EXIT(room, j)->exit_info, EX_HIDDEN))
	      dflag += 2;
	  } else
	    dflag = 0;

	  if (R_EXIT(room, j)->keyword)
	    strncpy(buf1, R_EXIT(room, j)->keyword, sizeof(buf1)-1 );
	  else
	    *buf1 = '\0';

	  /* A key from another zone cannot come with the zone, so mark it
	   * the way an exit leaving the zone is marked rather than mapping
	   * it onto whatever object holds that slot in the recipient's
	   * copy. */
	  if (R_EXIT(room, j)->key == NOTHING) {
	    key_tag = "";
	    key_num = -1;
	  } else {
	    key_tag = "";
	    key_num = R_EXIT(room, j)->key;
	  }

	  /* Now write the exit to the file. */
          if (R_EXIT(room, j)->to_room == NOWHERE || world[R_EXIT(room, j)->to_room].zone == zrnum)
	    fprintf(room_file,"D%d\n"
		              "%s~\n"
			      "%s~\n"
        		      "%d %s%s %s\n",
			      j,
			      buf,
			      buf1,
			      dflag,
			      key_tag,
			      xv_opt(fmt, key_num),
			      R_EXIT(room, j)->to_room == NOTHING ? "-1" :
			        xv(fmt, world[R_EXIT(room, j)->to_room].number));
          else {
	    fprintf(room_file,"D%d\n"
		              "%s~\n"
			      "%s~\n"
        		      "%d %s%s %s\n",
			      j,
			      buf,
			      buf1,
			      dflag,
			      key_tag,
			      xv_opt(fmt, key_num),
			      xv(fmt, world[R_EXIT(room, j)->to_room].number));
          }
	}
      }

      if (room->ex_description) {
        struct extra_descr_data *xdesc;

	for (xdesc = room->ex_description; xdesc; xdesc = xdesc->next) {
	  strncpy(buf, xdesc->description, sizeof(buf) - 1);
	  buf[sizeof(buf) - 1] = '\0';
	  strip_cr(buf);
	  fprintf(room_file,	"E\n"
			"%s~\n"
			"%s~\n", xdesc->keyword, buf);
	}
      }
      fprintf(room_file, "S\n");
      export_script_save_to_disk(room_file, room, WLD_TRIGGER, fmt);
    }
  }

  /* Write the final line and close it. */
  fprintf(room_file, "$~\n");
  fclose(room_file);

  return TRUE;
}

static void export_script_save_to_disk(FILE *fp, void *item, int type,
                                       const struct export_fmt *fmt)
{
  struct trig_proto_list *t;

  if (type==MOB_TRIGGER)
    t = ((struct char_data *)item)->proto_script;
  else if (type==OBJ_TRIGGER)
    t = ((struct obj_data *)item)->proto_script;
  else if (type==WLD_TRIGGER)
    t = ((struct room_data *)item)->proto_script;
  else {
    log("SYSERR: Invalid type passed to export_script_save_to_disk()");
    return;
  }

  while (t)
  {
    fprintf(fp, "T %s\n", xv(fmt, t->vnum));
    t = t->next;
  }
}

/* save the zone's triggers to internal memory and to disk */
/* The help text lists .qst among the files that make up a zone, but the
 * exporter never wrote one, so a zone's autoquests did not travel with
 * it. Same record layout as genqst.c's save_quests, with the vnums this
 * zone owns QQ'd like every other exported file.
 *
 * An unset field may hold the NOBODY/NOTHING sentinel or a plain -1
 * depending on which editor last wrote the quest -- the stock 1.qst
 * has -1 in its return-mob slot -- so both count as unset. */
static int export_save_quests(zone_rnum zrnum, const struct export_fmt *fmt)
{
  FILE *quest_file;
  char quest_flags[MAX_STRING_LENGTH];
  char quest_desc[MAX_STRING_LENGTH], quest_info[MAX_STRING_LENGTH];
  char quest_done[MAX_STRING_LENGTH], quest_quit[MAX_STRING_LENGTH];
  qst_vnum i;

  if (!(quest_file = export_open(fmt, "qst"))) {
    mudlog(BRF, LVL_GOD, TRUE, "SYSERR: export_save_quests : Cannot open file!");
    return FALSE;
  }

  for (i = genolc_zone_bottom(zrnum); i <= zone_table[zrnum].top; i++) {
    qst_rnum rnum = real_quest(i);

    if (rnum == NOTHING)
      continue;

    strncpy(quest_desc, QST_DESC(rnum) ? QST_DESC(rnum) : "undefined", sizeof(quest_desc) - 1);
    strncpy(quest_info, QST_INFO(rnum) ? QST_INFO(rnum) : "undefined", sizeof(quest_info) - 1);
    strncpy(quest_done, QST_DONE(rnum) ? QST_DONE(rnum) : "undefined", sizeof(quest_done) - 1);
    strncpy(quest_quit, QST_QUIT(rnum) ? QST_QUIT(rnum) : "undefined", sizeof(quest_quit) - 1);
    strip_cr(quest_desc);
    strip_cr(quest_info);
    strip_cr(quest_done);
    strip_cr(quest_quit);
    sprintascii(quest_flags, QST_FLAGS(rnum));

    /* Eight vnums in one call: the widest record there is, and what
     * XV_RING is sized against. */
    fprintf(quest_file,
      "#%s\n"
      "%s%c\n"
      "%s%c\n"
      "%s%c\n"
      "%s%c\n"
      "%s%c\n"
      "%d %s %s %s %s %s %s\n"
      "%d %d %d %d %d %s %d\n"
      "%d %d %s\n"
      "S\n",
      xv(fmt, QST_NUM(rnum)),
      QST_NAME(rnum) ? QST_NAME(rnum) : "Untitled", STRING_TERMINATOR,
      quest_desc, STRING_TERMINATOR,
      quest_info, STRING_TERMINATOR,
      quest_done, STRING_TERMINATOR,
      quest_quit, STRING_TERMINATOR,
      QST_TYPE(rnum),
      xv_opt(fmt, QST_MASTER(rnum)),
      quest_flags,
      xv_opt(fmt, QST_TARGET(rnum)),
      xv_opt(fmt, QST_PREV(rnum)),
      xv_opt(fmt, QST_NEXT(rnum)),
      xv_opt(fmt, QST_PREREQ(rnum)),
      QST_POINTS(rnum), QST_PENALTY(rnum), QST_MINLEVEL(rnum),
      QST_MAXLEVEL(rnum), QST_TIME(rnum),
      xv_opt(fmt, QST_RETURNMOB(rnum)),
      QST_QUANTITY(rnum),
      QST_GOLD(rnum), QST_EXP(rnum),
      xv(fmt, QST_OBJ(rnum))
    );
  }

  fprintf(quest_file, "$~\n");
  fclose(quest_file);
  return TRUE;
}

static int export_save_triggers(zone_rnum zrnum, const struct export_fmt *fmt)
{
  int i;
  trig_data *trig;
  struct cmdlist_element *cmd;
  FILE *trig_file;
  char bitBuf[MAX_INPUT_LENGTH];

  if (!(trig_file = export_open(fmt, "trg"))) {
    mudlog(BRF, LVL_GOD, TRUE, "SYSERR: export_save_triggers : Cannot open file!");
    return FALSE;
  }

  for (i = genolc_zone_bottom(zrnum); i <= zone_table[zrnum].top; i++) {
    trig_rnum rnum;

    if ((rnum = real_trigger(i)) != NOTHING) {
      trig = trig_index[rnum]->proto;

      fprintf(trig_file, "#%s\n", xv(fmt, i));

      sprintascii(bitBuf, GET_TRIG_TYPE(trig));
      fprintf(trig_file,      "%s%c\n"
                              "%d %s %d\n"
                              "%s%c\n",
           (GET_TRIG_NAME(trig)) ? (GET_TRIG_NAME(trig)) : "unknown trigger", STRING_TERMINATOR,
           trig->attach_type,
           *bitBuf ? bitBuf : "0", GET_TRIG_NARG(trig),
           GET_TRIG_ARG(trig) ? GET_TRIG_ARG(trig) : "", STRING_TERMINATOR);

      fprintf(trig_file, "* This trigger has been exported 'as is'. This means that vnums\n"
                         "* in this file are not changed, and will have to be edited by hand.\n"
                         "* This zone was number %d on The Builder Academy, so you\n"
                         "* should be looking for %dxx, where xx is 00-99.\n",
                         zone_table[zrnum].number, zone_table[zrnum].number);
        for (cmd = trig->cmdlist; cmd; cmd = cmd->next) {
          fprintf(trig_file, "%s\n", cmd->cmd);
        }
      fprintf(trig_file, "%c\n", STRING_TERMINATOR);
    }
  }

  fprintf(trig_file, "$%c\n", STRING_TERMINATOR);
  fclose(trig_file);
  return TRUE;
}
