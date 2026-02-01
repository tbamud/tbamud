/**
* @file set.c
* Builder room/object creation and utility functions.
* 
* This set of code was not originally part of the circlemud distribution.
*/

#include "conf.h"
#include "sysdep.h"

#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "spells.h"
#include "class.h"
#include "species.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "boards.h"
#include "genolc.h"
#include "genwld.h"
#include "genzon.h"
#include "oasis.h"
#include "improved-edit.h"
#include "modify.h"
#include "genobj.h"
#include "genmob.h"
#include "py_triggers.h"
#include "fight.h"
#include "toml.h"

#define PY_SCRIPT_EXT ".py"

static void script_toml_escape(const char *input, char *output, size_t outsz)
{
  size_t len = 0;
  const char *p = input ? input : "";

  if (!outsz)
    return;

  while (*p && len + 2 < outsz) {
    if (*p == '\\' || *p == '"') {
      output[len++] = '\\';
      if (len + 1 >= outsz)
        break;
    }
    output[len++] = *p++;
  }

  output[len] = '\0';
}

static int script_normalize_name(const char *input, char *output, size_t outsz)
{
  const char *ext;

  if (!input || !*input || !output || outsz < 4)
    return 0;

  if (strchr(input, '/'))
    return 0;

  strlcpy(output, input, outsz);
  ext = strrchr(output, '.');
  if (!ext)
    strlcat(output, PY_SCRIPT_EXT, outsz);

  return 1;
}

static int script_exists(const char *script_name)
{
  char path[PATH_MAX];
  struct stat st;

  if (!script_name || !*script_name)
    return 0;

  snprintf(path, sizeof(path), "%s%s", SCRIPTS_PREFIX, script_name);
  if (stat(path, &st) < 0)
    return 0;

  return S_ISREG(st.st_mode);
}

static int script_find_trigger_by_script(const char *script, zone_rnum znum, int attach_type)
{
  int i;
  int bottom, top;

  if (!script || !*script)
    return NOTHING;

  if (znum != NOWHERE) {
    bottom = zone_table[znum].bot;
    top = zone_table[znum].top;
  } else {
    bottom = INT_MIN;
    top = INT_MAX;
  }

  for (i = 0; i < top_of_trigt; i++) {
    if (!trig_index[i] || !trig_index[i]->proto)
      continue;
    if (trig_index[i]->vnum < bottom || trig_index[i]->vnum > top)
      continue;
    if (trig_index[i]->proto->attach_type != attach_type)
      continue;
    if (!trig_index[i]->proto->script)
      continue;
    if (!str_cmp(trig_index[i]->proto->script, script))
      return trig_index[i]->vnum;
  }

  return NOTHING;
}

static int script_find_next_trigger_vnum(zone_rnum znum)
{
  int vnum;
  int bottom;
  int top;

  if (znum == NOWHERE)
    return NOTHING;

  bottom = zone_table[znum].bot;
  top = zone_table[znum].top;

  for (vnum = bottom; vnum <= top; vnum++) {
    if (real_trigger(vnum) == NOTHING)
      return vnum;
  }

  return NOTHING;
}

static int script_default_trigger_type(int attach_type)
{
  switch (attach_type) {
    case MOB_TRIGGER: return MTRIG_LOAD;
    case OBJ_TRIGGER: return OTRIG_LOAD;
    case WLD_TRIGGER: return WTRIG_RESET;
    default: return 0;
  }
}

static int script_default_trigger_narg(int attach_type, long trigger_type)
{
  if (attach_type == MOB_TRIGGER && (trigger_type & MTRIG_LOAD))
    return 100;
  if (attach_type == OBJ_TRIGGER && (trigger_type & OTRIG_LOAD))
    return 100;
  if (attach_type == WLD_TRIGGER && (trigger_type & WTRIG_RESET))
    return 100;
  return 0;
}

static const char *script_default_trigger_name(const char *script)
{
  const char *base;
  const char *dot;

  if (!script || !*script)
    return "script";

  base = script;
  dot = strrchr(base, '.');
  if (dot && dot > base)
    return base;

  return base;
}

static int script_insert_trigger_index(trig_data *trig, int vnum)
{
  struct index_data **new_index;
  struct trig_data *proto;
  struct trig_data *live_trig;
  trig_rnum i, rnum = NOTHING;

  if (!trig || vnum <= 0)
    return NOTHING;

  CREATE(new_index, struct index_data *, top_of_trigt + 1);

  for (i = 0; i < top_of_trigt; i++) {
    if (rnum == NOTHING && trig_index[i]->vnum > vnum) {
      rnum = i;
      CREATE(new_index[rnum], struct index_data, 1);
      new_index[rnum]->vnum = vnum;
      new_index[rnum]->number = 0;
      new_index[rnum]->func = NULL;
      proto = trig;
      new_index[rnum]->proto = proto;
      proto->nr = rnum;
      new_index[i + 1] = trig_index[i];
      trig_index[i]->proto->nr = i + 1;
    } else {
      if (rnum == NOTHING) {
        new_index[i] = trig_index[i];
      } else {
        new_index[i + 1] = trig_index[i];
        trig_index[i]->proto->nr = i + 1;
      }
    }
  }

  if (rnum == NOTHING) {
    rnum = top_of_trigt;
    CREATE(new_index[rnum], struct index_data, 1);
    new_index[rnum]->vnum = vnum;
    new_index[rnum]->number = 0;
    new_index[rnum]->func = NULL;
    proto = trig;
    new_index[rnum]->proto = proto;
    proto->nr = rnum;
  }

  free(trig_index);
  trig_index = new_index;
  top_of_trigt++;

  for (live_trig = trigger_list; live_trig; live_trig = live_trig->next_in_world)
    GET_TRIG_RNUM(live_trig) += (GET_TRIG_RNUM(live_trig) != NOTHING && GET_TRIG_RNUM(live_trig) >= rnum);

  return rnum;
}

static int script_append_trigger_toml(int znum, trig_data *trig, int vnum)
{
  char fname[PATH_MAX];
  FILE *fp;
  char name_buf[MAX_INPUT_LENGTH];
  char arg_buf[MAX_INPUT_LENGTH];
  char script_buf[MAX_INPUT_LENGTH];

  if (!trig || vnum <= 0)
    return 0;

  snprintf(fname, sizeof(fname), "%s/%d.toml", TRG_PREFIX, znum);
  fp = fopen(fname, "a");
  if (!fp)
    return 0;

  script_toml_escape(trig->name ? trig->name : "script", name_buf, sizeof(name_buf));
  script_toml_escape(trig->arglist ? trig->arglist : "", arg_buf, sizeof(arg_buf));
  script_toml_escape(trig->script ? trig->script : "", script_buf, sizeof(script_buf));

  fprintf(fp, "\n[[trigger]]\n");
  fprintf(fp, "vnum = %d\n", vnum);
  fprintf(fp, "name = \"%s\"\n", name_buf);
  fprintf(fp, "attach_type = %d\n", trig->attach_type);
  fprintf(fp, "flags = %ld\n", trig->trigger_type);
  fprintf(fp, "narg = %d\n", trig->narg);
  fprintf(fp, "arglist = \"%s\"\n", arg_buf);
  fprintf(fp, "script = \"%s\"\n", script_buf);

  fclose(fp);
  return 1;
}

static int script_create_trigger_for_zone(zone_rnum znum, int attach_type, const char *script)
{
  trig_data *trig;
  int vnum;
  int rnum;

  if (znum == NOWHERE || !script || !*script)
    return NOTHING;

  vnum = script_find_next_trigger_vnum(znum);
  if (vnum == NOTHING)
    return NOTHING;

  CREATE(trig, trig_data, 1);
  memset(trig, 0, sizeof(*trig));
  trig->name = strdup(script_default_trigger_name(script));
  trig->attach_type = (byte)attach_type;
  trig->trigger_type = (long)script_default_trigger_type(attach_type);
  trig->narg = script_default_trigger_narg(attach_type, trig->trigger_type);
  trig->arglist = strdup("");
  trig->script = strdup(script);

  rnum = script_insert_trigger_index(trig, vnum);
  if (rnum == NOTHING) {
    if (trig->name)
      free(trig->name);
    if (trig->arglist)
      free(trig->arglist);
    if (trig->script)
      free(trig->script);
    free(trig);
    return NOTHING;
  }

  if (!script_append_trigger_toml(zone_table[znum].number, trig, vnum))
    return NOTHING;

  return vnum;
}

static int script_resolve_trigger_vnum(struct char_data *ch, const char *arg,
                                      zone_rnum znum, int attach_type)
{
  char script_name[MAX_INPUT_LENGTH];
  int vnum;

  if (!arg || !*arg)
    return NOTHING;

  if (is_number(arg))
    return atoi(arg);

  if (!script_normalize_name(arg, script_name, sizeof(script_name))) {
    send_to_char(ch, "Script name is invalid.\r\n");
    return NOTHING;
  }

  if (!script_exists(script_name)) {
    send_to_char(ch, "Script file %s not found.\r\n", script_name);
    return NOTHING;
  }

  vnum = script_find_trigger_by_script(script_name, znum, attach_type);
  if (vnum != NOTHING)
    return vnum;

  vnum = script_create_trigger_for_zone(znum, attach_type, script_name);
  if (vnum == NOTHING) {
    send_to_char(ch, "No available trigger vnums in this zone.\r\n");
    return NOTHING;
  }

  send_to_char(ch, "Created trigger %d for script %s.\r\n", vnum, script_name);
  return vnum;
}

static int script_find_attached_trigger_vnum(struct trig_proto_list *list,
                                             const char *script, int attach_type)
{
  struct trig_proto_list *tp;
  int rnum;

  if (!script || !*script)
    return NOTHING;

  for (tp = list; tp; tp = tp->next) {
    rnum = real_trigger(tp->vnum);
    if (rnum == NOTHING || !trig_index[rnum] || !trig_index[rnum]->proto)
      continue;
    if (trig_index[rnum]->proto->attach_type != attach_type)
      continue;
    if (!trig_index[rnum]->proto->script)
      continue;
    if (!str_cmp(trig_index[rnum]->proto->script, script))
      return tp->vnum;
  }

  return NOTHING;
}
#include "quest.h"

#include "set.h"

static int proto_trigger_has(struct trig_proto_list *list, int vnum)
{
  for (; list; list = list->next)
    if (list->vnum == vnum)
      return 1;
  return 0;
}

static int proto_trigger_add(struct trig_proto_list **list, int vnum)
{
  struct trig_proto_list *trig, *tail;

  if (!list)
    return 0;

  if (proto_trigger_has(*list, vnum))
    return 0;

  CREATE(trig, struct trig_proto_list, 1);
  trig->vnum = vnum;
  trig->next = NULL;

  if (!*list) {
    *list = trig;
    return 1;
  }

  tail = *list;
  while (tail->next)
    tail = tail->next;
  tail->next = trig;
  return 1;
}

static int proto_trigger_remove(struct trig_proto_list **list, int vnum)
{
  struct trig_proto_list *cur, *prev, *next;
  int removed = 0;

  if (!list)
    return 0;

  prev = NULL;
  cur = *list;
  while (cur) {
    next = cur->next;
    if (cur->vnum == vnum) {
      if (prev)
        prev->next = next;
      else
        *list = next;
      free(cur);
      removed = 1;
    } else {
      prev = cur;
    }
    cur = next;
  }

  return removed;
}

static void rset_show_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Usage:\r\n"
    "  rset show\r\n"
    "  rset add name <text>\r\n"
    "  rset add sector <sector>\r\n"
    "  rset add flags <flag> [flag ...]\r\n"
    "  rset add exit <direction> <room number>\r\n"
    "  rset add exitdesc <direction> <text>\r\n"
    "  rset add door <direction> <name of door>\r\n"
    "  rset add doorflags <direction> <flag> [flag ...]\r\n"
    "  rset add key <direction> <key number>\r\n"
    "  rset add hidden <direction>\r\n"
    "  rset add forage <object vnum> <dc check>\r\n"
    "  rset add edesc <keyword> <description>\r\n"
    "  rset add script <script name>\r\n"
    "  rset add desc\r\n"
    "  rset del <field>\r\n"
    "  rset clear force\r\n"
    "  rset validate\r\n");
}

static void rset_show_set_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds basic configuration to the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add name <text>\r\n"
    "  rset add sector <sector>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add name \"A wind-scoured alley\"\r\n"
    "  rset add sector desert\r\n");
}

static void rset_show_set_sector_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds room sector type.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add sector <sector>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add sector desert\r\n"
    "\r\n"
    "Sectors:\r\n");
  column_list(ch, 0, sector_types, NUM_ROOM_SECTORS, FALSE);
}

static void rset_show_add_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds specific configuration to the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add name <text>\r\n"
    "  rset add sector <sector>\r\n"
    "  rset add flags <flag> [flag ...]\r\n"
    "  rset add exit <direction> <room number>\r\n"
    "  rset add exitdesc <direction> <text>\r\n"
    "  rset add door <direction> <name of door>\r\n"
    "  rset add doorflags <direction> <flag> [flag ...]\r\n"
    "  rset add key <direction> <key number>\r\n"
    "  rset add hidden <direction>\r\n"
    "  rset add forage <object vnum> <dc check>\r\n"
    "  rset add edesc <keyword> <description>\r\n"
    "  rset add script <script name>\r\n"
    "  rset add desc\r\n");
}

static void rset_show_del_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes specific configuration from the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del flags <flag> [flag ...]\r\n"
    "  rset del exit <direction>\r\n"
    "  rset del exitdesc <direction>\r\n"
    "  rset del door <direction>\r\n"
    "  rset del doorflags <direction> <flag> [flag ...]\r\n"
    "  rset del key <direction>\r\n"
    "  rset del hidden <direction>\r\n"
    "  rset del forage <object vnum>\r\n"
    "  rset del edesc <keyword>\r\n"
    "  rset del script <script name>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del flags INDOORS QUITSAFE\r\n"
    "  rset del exit n\r\n"
    "  rset del door n\r\n"
    "  rset del key n\r\n"
    "  rset del hidden n\r\n"
    "  rset del forage 301\r\n"
    "  rset del edesc mosaic\r\n"
    "  rset del script rat_patrol.py\r\n");
}

static void rset_show_validate_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Verifies your new room meets building standards and looks for any errors. If\r\n"
    "correct, you can use the rsave command to finish building.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset validate\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset validate\r\n");
}

static void rset_show_desc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Enters text editor for editing the main description of the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add desc\r\n");
}

static void rset_show_rcreate_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Creates a new unfinished room which can be entered and configured.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rcreate <vnum>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rcreate 1001\r\n");
}

static void rset_show_add_flags_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds room flags.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add flags <flag> [flag ...]\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add flags INDOORS QUITSAFE\r\n"
    "\r\n"
    "Flags:\r\n");
  column_list(ch, 0, room_bits, NUM_ROOM_FLAGS, FALSE);
}

static void rset_show_del_flags_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes room flags.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del flags <flag> [flag ...]\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del flags INDOORS QUITSAFE\r\n"
    "\r\n"
    "Flags:\r\n");
  column_list(ch, 0, room_bits, NUM_ROOM_FLAGS, FALSE);
}

static void rset_show_add_exit_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds an exit to the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add exit <direction> <room number>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add exit n 101\r\n");
}

static void rset_show_add_exitdesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a description to an existing exit.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add exitdesc <direction> <text>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add exitdesc n A narrow arch leads north.\r\n");
}

static void rset_show_add_door_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a door keyword to an existing exit.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add door <direction> <name of door>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add door n door\r\n");
}

static void rset_show_add_doorflags_usage(struct char_data *ch)
{
  int count = 0;

  while (*exit_bits[count] != '\n')
    count++;

  send_to_char(ch,
    "Adds door flags to an existing exit.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add doorflags <direction> <flag> [flag ...]\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add doorflags n closed locked\r\n"
    "\r\n"
    "Flags:\r\n");
  column_list(ch, 0, exit_bits, count, FALSE);
}

static void rset_show_add_key_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a key to an existing door.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add key <direction> <key number>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add key n 201\r\n");
}

static void rset_show_add_hidden_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds the hidden flag to an existing exit.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add hidden <direction>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add hidden n\r\n");
}

static void rset_show_add_forage_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a forage entry to the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add forage <object vnum> <dc check>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add forage 301 15\r\n");
}

static void rset_show_add_edesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds an extra description to the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add edesc <keyword> <description>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add edesc mosaic A beautiful mosaic is here on the wall.\r\n");
}

static void rset_show_del_exit_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes an exit from the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del exit <direction>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del exit n\r\n");
}

static void rset_show_del_exitdesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes the description from an existing exit.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del exitdesc <direction>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del exitdesc n\r\n");
}

static void rset_show_del_door_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes a door from an existing exit.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del door <direction>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del door n\r\n");
}

static void rset_show_del_doorflags_usage(struct char_data *ch)
{
  int count = 0;

  while (*exit_bits[count] != '\n')
    count++;

  send_to_char(ch,
    "Deletes door flags from an existing exit.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del doorflags <direction> <flag> [flag ...]\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del doorflags n locked\r\n"
    "\r\n"
    "Flags:\r\n");
  column_list(ch, 0, exit_bits, count, FALSE);
}

static void rset_show_del_key_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes a key from an existing door.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del key <direction>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del key n\r\n");
}

static void rset_show_del_hidden_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes the hidden flag from an existing exit.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del hidden <direction>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del hidden n\r\n");
}

static void rset_show_del_forage_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes a forage entry from the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del forage <object vnum>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del forage 301\r\n");
}

static void rset_show_del_edesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes an extra description from the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del edesc <keyword>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del edesc mosaic\r\n");
}

static int rset_find_room_flag(const char *arg)
{
  int i;

  for (i = 0; i < NUM_ROOM_FLAGS; i++)
    if (is_abbrev(arg, room_bits[i]))
      return i;

  return -1;
}

static int rset_find_sector(const char *arg)
{
  int i;

  for (i = 0; i < NUM_ROOM_SECTORS; i++)
    if (is_abbrev(arg, sector_types[i]))
      return i;

  return -1;
}

static int rset_find_dir(const char *arg)
{
  char dirbuf[MAX_INPUT_LENGTH];
  int dir;

  strlcpy(dirbuf, arg, sizeof(dirbuf));
  dir = search_block(dirbuf, dirs, FALSE);
  if (dir == -1) {
    strlcpy(dirbuf, arg, sizeof(dirbuf));
    dir = search_block(dirbuf, autoexits, FALSE);
  }

  if (dir >= DIR_COUNT)
    return -1;

  return dir;
}

static int rset_find_exit_flag(const char *arg)
{
  int i;

  for (i = 0; *exit_bits[i] != '\n'; i++)
    if (is_abbrev(arg, exit_bits[i]))
      return (1 << i);

  return -1;
}

static void rset_mark_room_modified(room_rnum rnum)
{
  if (rnum == NOWHERE || rnum < 0 || rnum > top_of_world)
    return;

  add_to_save_list(zone_table[world[rnum].zone].number, SL_WLD);
}

static void rset_show_room(struct char_data *ch, struct room_data *room)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int i, count = 0;

  send_to_char(ch, "Room [%d]: %s\r\n", room->number, room->name ? room->name : "<None>");
  send_to_char(ch, "Zone [%d]: %s\r\n", zone_table[room->zone].number, zone_table[room->zone].name);

  sprinttype(room->sector_type, sector_types, buf, sizeof(buf));
  send_to_char(ch, "Sector: %s\r\n", buf);

  sprintbitarray(room->room_flags, room_bits, RF_ARRAY_MAX, buf);
  send_to_char(ch, "Flags: %s\r\n", buf);

  send_to_char(ch, "Description:\r\n%s", room->description ? room->description : "  <None>\r\n");

  send_to_char(ch, "Exits:\r\n");
  for (i = 0; i < DIR_COUNT; i++) {
    struct room_direction_data *exit = room->dir_option[i];
    room_rnum to_room;
    const char *dir = dirs[i];
    char keybuf[32];

    if (!exit)
      continue;

    to_room = exit->to_room;
    if (to_room == NOWHERE || to_room < 0 || to_room > top_of_world)
      snprintf(buf2, sizeof(buf2), "NOWHERE");
    else
      snprintf(buf2, sizeof(buf2), "%d", world[to_room].number);

    sprintbit(exit->exit_info, exit_bits, buf, sizeof(buf));
    if (IS_SET(exit->exit_info, EX_HIDDEN))
      strlcat(buf, "HIDDEN ", sizeof(buf));

    if (exit->key == NOTHING)
      strlcpy(keybuf, "None", sizeof(keybuf));
    else
      snprintf(keybuf, sizeof(keybuf), "%d", exit->key);

    send_to_char(ch, "  %-5s -> %s (door: %s, key: %s, flags: %s)\r\n",
      dir,
      buf2,
      exit->keyword ? exit->keyword : "None",
      keybuf,
      buf);
    if (exit->general_description && *exit->general_description)
      send_to_char(ch, "        desc: %s\r\n", exit->general_description);
    count++;
  }
  if (!count)
    send_to_char(ch, "  None.\r\n");

  send_to_char(ch, "Forage:\r\n");
  count = 0;
  for (struct forage_entry *entry = room->forage; entry; entry = entry->next) {
    obj_rnum rnum = real_object(entry->obj_vnum);
    const char *sdesc = (rnum != NOTHING) ? obj_proto[rnum].short_description : "Unknown object";
    send_to_char(ch, "  [%d] DC %d - %s\r\n", entry->obj_vnum, entry->dc, sdesc);
    count++;
  }
  if (!count)
    send_to_char(ch, "  None.\r\n");

  send_to_char(ch, "Extra Descs:\r\n");
  count = 0;
  for (struct extra_descr_data *desc = room->ex_description; desc; desc = desc->next) {
    send_to_char(ch, "  %s\r\n", desc->keyword ? desc->keyword : "<None>");
    count++;
  }
  if (!count)
    send_to_char(ch, "  None.\r\n");
}

static void rset_desc_edit(struct char_data *ch, struct room_data *room)
{
  char *oldtext = NULL;

  send_editor_help(ch->desc);
  write_to_output(ch->desc, "Enter room description:\r\n\r\n");

  if (room->description) {
    write_to_output(ch->desc, "%s", room->description);
    oldtext = strdup(room->description);
  }

  string_write(ch->desc, &room->description, MAX_ROOM_DESC, 0, oldtext);
  rset_mark_room_modified(IN_ROOM(ch));
}

static bool rset_room_has_flags(struct room_data *room)
{
  int i;

  for (i = 0; i < RF_ARRAY_MAX; i++)
    if (room->room_flags[i])
      return TRUE;

  return FALSE;
}

static void rset_validate_room(struct char_data *ch, struct room_data *room)
{
  int errors = 0;
  int i;

  if (!room->name || !*room->name) {
    send_to_char(ch, "Error: room name is not set.\r\n");
    errors++;
  }

  if (!room->description || !*room->description) {
    send_to_char(ch, "Error: room description is not set.\r\n");
    errors++;
  }

  if (room->sector_type < 0 || room->sector_type >= NUM_ROOM_SECTORS) {
    send_to_char(ch, "Error: sector type is invalid.\r\n");
    errors++;
  }

  if (!rset_room_has_flags(room)) {
    send_to_char(ch, "Error: at least one room flag should be set.\r\n");
    errors++;
  }

  for (i = 0; i < DIR_COUNT; i++) {
    struct room_direction_data *exit = room->dir_option[i];
    bool exit_valid;

    if (!exit)
      continue;

    exit_valid = !(exit->to_room == NOWHERE || exit->to_room < 0 || exit->to_room > top_of_world);

    if (!exit_valid) {
      send_to_char(ch, "Error: exit %s does not point to a valid room.\r\n", dirs[i]);
      errors++;
    }

    if (IS_SET(exit->exit_info, EX_ISDOOR) && !exit_valid) {
      send_to_char(ch, "Error: door on %s has no valid exit.\r\n", dirs[i]);
      errors++;
    }

    if (exit->key > 0) {
      if (!IS_SET(exit->exit_info, EX_ISDOOR)) {
        send_to_char(ch, "Error: key on %s is set without a door.\r\n", dirs[i]);
        errors++;
      }
      if (real_object(exit->key) == NOTHING) {
        send_to_char(ch, "Error: key vnum %d on %s does not exist.\r\n", exit->key, dirs[i]);
        errors++;
      }
    }

    if (IS_SET(exit->exit_info, EX_HIDDEN) && !exit_valid) {
      send_to_char(ch, "Error: hidden flag on %s has no valid exit.\r\n", dirs[i]);
      errors++;
    }
  }

  for (struct forage_entry *entry = room->forage; entry; entry = entry->next) {
    if (entry->obj_vnum <= 0 || real_object(entry->obj_vnum) == NOTHING) {
      send_to_char(ch, "Error: forage object vnum %d is invalid.\r\n", entry->obj_vnum);
      errors++;
    }
    if (entry->dc <= 0) {
      send_to_char(ch, "Error: forage object vnum %d is missing a valid DC.\r\n", entry->obj_vnum);
      errors++;
    }
  }

  for (struct extra_descr_data *desc = room->ex_description; desc; desc = desc->next) {
    if (!desc->keyword || !*desc->keyword) {
      send_to_char(ch, "Error: extra description is missing a keyword.\r\n");
      errors++;
    }
    if (!desc->description || !*desc->description) {
      send_to_char(ch, "Error: extra description for %s is missing text.\r\n",
        desc->keyword ? desc->keyword : "<None>");
      errors++;
    }
  }

  if (!errors)
    send_to_char(ch, "Room validates cleanly.\r\n");
  else
    send_to_char(ch, "Validation failed: %d issue%s.\r\n", errors, errors == 1 ? "" : "s");
}

ACMD(do_rcreate)
{
  char arg[MAX_INPUT_LENGTH];
  char namebuf[MAX_INPUT_LENGTH];
  char descbuf[MAX_STRING_LENGTH];
  char timestr[64];
  struct room_data room;
  room_vnum vnum;
  room_rnum rnum;
  zone_rnum znum;
  time_t now;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "rcreate is only usable by connected players.\r\n");
    return;
  }

  argument = one_argument(argument, arg);
  if (!*arg) {
    rset_show_rcreate_usage(ch);
    return;
  }

  if (!is_number(arg)) {
    rset_show_rcreate_usage(ch);
    return;
  }

  vnum = atoi(arg);
  if (vnum <= 0) {
    send_to_char(ch, "That is not a valid room vnum.\r\n");
    return;
  }

  if (real_room(vnum) != NOWHERE) {
    send_to_char(ch, "Room %d already exists.\r\n", vnum);
    return;
  }

  if ((znum = real_zone_by_thing(vnum)) == NOWHERE) {
    send_to_char(ch, "That room number is not in a valid zone.\r\n");
    return;
  }

  if (!can_edit_zone(ch, znum)) {
    send_to_char(ch, "You do not have permission to modify that zone.\r\n");
    return;
  }

  now = time(0);
  strftime(timestr, sizeof(timestr), "%c", localtime(&now));
  snprintf(namebuf, sizeof(namebuf), "Unfinished room made by %s", GET_NAME(ch));
  snprintf(descbuf, sizeof(descbuf),
           "This is an unfinished room created by %s on %s\r\n",
           GET_NAME(ch), timestr);

  memset(&room, 0, sizeof(room));
  room.number = vnum;
  room.zone = znum;
  room.sector_type = SECT_INSIDE;
  room.name = strdup(namebuf);
  room.description = strdup(descbuf);
  room.ex_description = NULL;
  room.forage = NULL;

  rnum = add_room(&room);

  free(room.name);
  free(room.description);

  if (rnum == NOWHERE) {
    send_to_char(ch, "Room creation failed.\r\n");
    return;
  }

  send_to_char(ch, "Room %d created.\r\n", vnum);
}

ACMD(do_rset)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  struct room_data *room;
  room_rnum rnum;
  trig_data *trig;
  int tn, rn;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "rset is only usable by connected players.\r\n");
    return;
  }

  rnum = IN_ROOM(ch);
  if (rnum == NOWHERE || rnum < 0 || rnum > top_of_world) {
    send_to_char(ch, "You are not in a valid room.\r\n");
    return;
  }

  if (!can_edit_zone(ch, world[rnum].zone)) {
    send_to_char(ch, "You do not have permission to modify this zone.\r\n");
    return;
  }

  room = &world[rnum];

  argument = one_argument(argument, arg1);
  if (!*arg1) {
    rset_show_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "show")) {
    rset_show_room(ch, room);
    return;
  }

  if (is_abbrev(arg1, "add") || is_abbrev(arg1, "set")) {
    bool set_alias = is_abbrev(arg1, "set");

    argument = one_argument(argument, arg2);
    if (!*arg2) {
      rset_show_add_usage(ch);
      return;
    }

    if (is_abbrev(arg2, "name")) {
      skip_spaces(&argument);
      if (!*argument) {
        rset_show_set_usage(ch);
        return;
      }
      genolc_checkstring(ch->desc, argument);
      if (count_non_protocol_chars(argument) > MAX_ROOM_NAME / 2) {
        send_to_char(ch, "Size limited to %d non-protocol characters.\r\n", MAX_ROOM_NAME / 2);
        return;
      }
      if (room->name)
        free(room->name);
      argument[MAX_ROOM_NAME - 1] = '\0';
      room->name = str_udup(argument);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Room name set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "sector")) {
      int sector;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_set_sector_usage(ch);
        return;
      }

      if (is_number(arg3))
        sector = atoi(arg3) - 1;
      else
        sector = rset_find_sector(arg3);

      if (sector < 0 || sector >= NUM_ROOM_SECTORS) {
        send_to_char(ch, "Invalid sector type.\r\n");
        return;
      }

      room->sector_type = sector;
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Sector type set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "desc")) {
      if (*argument) {
        rset_show_add_usage(ch);
        return;
      }
      rset_desc_edit(ch, room);
      return;
    }

    if (is_abbrev(arg2, "flags")) {
      bool any = FALSE;

      if (!*argument) {
        rset_show_add_flags_usage(ch);
        return;
      }

      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = rset_find_room_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown room flag: %s\r\n", arg3);
          continue;
        }

        SET_BIT_AR(room->room_flags, flag);
        any = TRUE;
      }

      if (any) {
        rset_mark_room_modified(rnum);
        send_to_char(ch, "Room flags updated.\r\n");
      }
      return;
    }

    if (is_abbrev(arg2, "exit")) {
      int dir;
      room_rnum to_room;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg1);
      if (!*arg3 || !*arg1) {
        rset_show_add_exit_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!is_number(arg1)) {
        send_to_char(ch, "Room number must be numeric.\r\n");
        return;
      }

      to_room = real_room(atoi(arg1));
      if (to_room == NOWHERE) {
        send_to_char(ch, "That room does not exist.\r\n");
        return;
      }

      if (!room->dir_option[dir]) {
        CREATE(room->dir_option[dir], struct room_direction_data, 1);
        room->dir_option[dir]->general_description = NULL;
        room->dir_option[dir]->keyword = NULL;
        room->dir_option[dir]->exit_info = 0;
        room->dir_option[dir]->key = NOTHING;
      }

      room->dir_option[dir]->to_room = to_room;
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Exit %s set to room %d.\r\n", dirs[dir], world[to_room].number);
      return;
    }

    if (is_abbrev(arg2, "exitdesc")) {
      int dir;
      char *desc;

      argument = one_argument(argument, arg3);
      skip_spaces(&argument);
      desc = argument;

      if (!*arg3 || !*desc) {
        rset_show_add_exitdesc_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir] || room->dir_option[dir]->to_room == NOWHERE) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      genolc_checkstring(ch->desc, desc);
      if (room->dir_option[dir]->general_description)
        free(room->dir_option[dir]->general_description);
      room->dir_option[dir]->general_description = str_udup(desc);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Exit description set for %s.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "door")) {
      int dir;
      char *door_name;

      argument = one_argument(argument, arg3);
      skip_spaces(&argument);
      door_name = argument;

      if (!*arg3 || !*door_name) {
        rset_show_add_door_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir] || room->dir_option[dir]->to_room == NOWHERE) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      genolc_checkstring(ch->desc, door_name);
      if (room->dir_option[dir]->keyword)
        free(room->dir_option[dir]->keyword);
      room->dir_option[dir]->keyword = str_udup(door_name);
      SET_BIT(room->dir_option[dir]->exit_info, EX_ISDOOR);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Door added to %s.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "doorflags")) {
      int dir;
      bool any = FALSE;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_add_doorflags_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir] || room->dir_option[dir]->to_room == NOWHERE) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      if (!*argument) {
        rset_show_add_doorflags_usage(ch);
        return;
      }

      while (*argument) {
        int flag;

        argument = one_argument(argument, arg1);
        if (!*arg1)
          break;

        flag = rset_find_exit_flag(arg1);
        if (flag < 0) {
          send_to_char(ch, "Unknown door flag: %s\r\n", arg1);
          continue;
        }

        SET_BIT(room->dir_option[dir]->exit_info, flag);
        any = TRUE;
      }

      if (any) {
        rset_mark_room_modified(rnum);
        send_to_char(ch, "Door flags updated on %s.\r\n", dirs[dir]);
      }
      return;
    }

    if (is_abbrev(arg2, "key")) {
      int dir;
      int key_vnum;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg1);
      if (!*arg3 || !*arg1) {
        rset_show_add_key_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir] || room->dir_option[dir]->to_room == NOWHERE) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      if (!IS_SET(room->dir_option[dir]->exit_info, EX_ISDOOR)) {
        send_to_char(ch, "That exit has no door.\r\n");
        return;
      }

      if (!is_number(arg1)) {
        send_to_char(ch, "Key number must be numeric.\r\n");
        return;
      }

      key_vnum = atoi(arg1);
      if (real_object(key_vnum) == NOTHING) {
        send_to_char(ch, "That key vnum does not exist.\r\n");
        return;
      }

      room->dir_option[dir]->key = key_vnum;
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Key set on %s.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "hidden")) {
      int dir;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_add_hidden_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir] || room->dir_option[dir]->to_room == NOWHERE) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      SET_BIT(room->dir_option[dir]->exit_info, EX_HIDDEN);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Hidden flag set on %s.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "forage")) {
      int vnum, dc;
      struct forage_entry *entry;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg1);
      if (!*arg3 || !*arg1) {
        rset_show_add_forage_usage(ch);
        return;
      }

      if (!is_number(arg3) || !is_number(arg1)) {
        rset_show_add_forage_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      dc = atoi(arg1);

      if (vnum <= 0 || dc <= 0) {
        send_to_char(ch, "Both vnum and DC must be positive.\r\n");
        return;
      }

      if (real_object(vnum) == NOTHING) {
        send_to_char(ch, "That object vnum does not exist.\r\n");
        return;
      }

      CREATE(entry, struct forage_entry, 1);
      entry->obj_vnum = vnum;
      entry->dc = dc;
      entry->next = room->forage;
      room->forage = entry;
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Forage entry added.\r\n");
      return;
    }

    if (is_abbrev(arg2, "edesc")) {
      struct extra_descr_data *desc;
      char *keyword;
      char *edesc;

      argument = one_argument(argument, arg3);
      skip_spaces(&argument);
      keyword = arg3;
      edesc = argument;

      if (!*keyword || !*edesc) {
        rset_show_add_edesc_usage(ch);
        return;
      }

      genolc_checkstring(ch->desc, edesc);
      genolc_checkstring(ch->desc, keyword);

      CREATE(desc, struct extra_descr_data, 1);
      desc->keyword = str_udup(keyword);
      desc->description = str_udup(edesc);
      desc->next = room->ex_description;
      room->ex_description = desc;
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Extra description added.\r\n");
      return;
    }

    if (is_abbrev(arg2, "script")) {
      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_add_usage(ch);
        return;
      }

      tn = script_resolve_trigger_vnum(ch, arg3, room->zone, WLD_TRIGGER);
      if (tn == NOTHING)
        return;
      rn = real_trigger(tn);
      if (rn == NOTHING || trig_index[rn] == NULL || trig_index[rn]->proto == NULL) {
        send_to_char(ch, "That trigger does not exist.\r\n");
        return;
      }
      trig = trig_index[rn]->proto;

      if (!proto_trigger_add(&room->proto_script, tn)) {
        send_to_char(ch, "That trigger is already attached.\r\n");
        return;
      }

      if (SCRIPT(room))
        extract_script(room, WLD_TRIGGER);
      assign_triggers(room, WLD_TRIGGER);
      rset_mark_room_modified(rnum);

      if (!save_rooms(room->zone)) {
        send_to_char(ch, "Failed to write room data to disk.\r\n");
        return;
      }

      send_to_char(ch, "Trigger %d (%s) attached to room %d.\r\n",
                   tn, GET_TRIG_NAME(trig), room->number);
      return;
    }

    if (set_alias) {
      int flag;

      flag = rset_find_room_flag(arg2);
      if (flag >= 0) {
        SET_BIT_AR(room->room_flags, flag);
        rset_mark_room_modified(rnum);
        send_to_char(ch, "Room flag set.\r\n");
        return;
      }
    }

    rset_show_add_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "del")) {
    argument = one_argument(argument, arg2);
    if (!*arg2) {
      rset_show_del_usage(ch);
      return;
    }

    if (is_abbrev(arg2, "flags")) {
      bool any = FALSE;

      if (!*argument) {
        rset_show_del_flags_usage(ch);
        return;
      }

      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = rset_find_room_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown room flag: %s\r\n", arg3);
          continue;
        }

        REMOVE_BIT_AR(room->room_flags, flag);
        any = TRUE;
      }

      if (any) {
        rset_mark_room_modified(rnum);
        send_to_char(ch, "Room flags updated.\r\n");
      }
      return;
    }

    if (is_abbrev(arg2, "exit")) {
      int dir;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_exit_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir]) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      if (room->dir_option[dir]->general_description)
        free(room->dir_option[dir]->general_description);
      if (room->dir_option[dir]->keyword)
        free(room->dir_option[dir]->keyword);
      free(room->dir_option[dir]);
      room->dir_option[dir] = NULL;
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Exit %s removed.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "exitdesc")) {
      int dir;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_exitdesc_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir] || room->dir_option[dir]->to_room == NOWHERE) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      if (room->dir_option[dir]->general_description) {
        free(room->dir_option[dir]->general_description);
        room->dir_option[dir]->general_description = NULL;
      }

      rset_mark_room_modified(rnum);
      send_to_char(ch, "Exit description removed from %s.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "door")) {
      int dir;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_door_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir]) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      if (room->dir_option[dir]->keyword) {
        free(room->dir_option[dir]->keyword);
        room->dir_option[dir]->keyword = NULL;
      }

      REMOVE_BIT(room->dir_option[dir]->exit_info, EX_ISDOOR);
      REMOVE_BIT(room->dir_option[dir]->exit_info, EX_CLOSED);
      REMOVE_BIT(room->dir_option[dir]->exit_info, EX_LOCKED);
      REMOVE_BIT(room->dir_option[dir]->exit_info, EX_PICKPROOF);
      REMOVE_BIT(room->dir_option[dir]->exit_info, EX_HIDDEN);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Door removed from %s.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "doorflags")) {
      int dir;
      bool any = FALSE;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_doorflags_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir] || room->dir_option[dir]->to_room == NOWHERE) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      if (!*argument) {
        rset_show_del_doorflags_usage(ch);
        return;
      }

      while (*argument) {
        int flag;

        argument = one_argument(argument, arg1);
        if (!*arg1)
          break;

        flag = rset_find_exit_flag(arg1);
        if (flag < 0) {
          send_to_char(ch, "Unknown door flag: %s\r\n", arg1);
          continue;
        }

        REMOVE_BIT(room->dir_option[dir]->exit_info, flag);
        any = TRUE;
      }

      if (any) {
        rset_mark_room_modified(rnum);
        send_to_char(ch, "Door flags updated on %s.\r\n", dirs[dir]);
      }
      return;
    }

    if (is_abbrev(arg2, "key")) {
      int dir;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_key_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir]) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      room->dir_option[dir]->key = NOTHING;
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Key removed from %s.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "hidden")) {
      int dir;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_hidden_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir]) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      REMOVE_BIT(room->dir_option[dir]->exit_info, EX_HIDDEN);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Hidden flag removed from %s.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "forage")) {
      int vnum;
      struct forage_entry *entry = room->forage;
      struct forage_entry *prev = NULL;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_forage_usage(ch);
        return;
      }

      if (!is_number(arg3)) {
        rset_show_del_forage_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      while (entry) {
        if (entry->obj_vnum == vnum)
          break;
        prev = entry;
        entry = entry->next;
      }

      if (!entry) {
        send_to_char(ch, "No forage entry found for vnum %d.\r\n", vnum);
        return;
      }

      if (prev)
        prev->next = entry->next;
      else
        room->forage = entry->next;
      free(entry);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Forage entry removed.\r\n");
      return;
    }

    if (is_abbrev(arg2, "edesc")) {
      struct extra_descr_data *desc = room->ex_description;
      struct extra_descr_data *prev = NULL;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_edesc_usage(ch);
        return;
      }

      while (desc) {
        if (desc->keyword && isname(arg3, desc->keyword))
          break;
        prev = desc;
        desc = desc->next;
      }

      if (!desc) {
        send_to_char(ch, "No extra description found for %s.\r\n", arg3);
        return;
      }

      if (prev)
        prev->next = desc->next;
      else
        room->ex_description = desc->next;

      if (desc->keyword)
        free(desc->keyword);
      if (desc->description)
        free(desc->description);
      free(desc);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Extra description removed.\r\n");
      return;
    }

    if (is_abbrev(arg2, "script")) {
      const char *tname = "unknown";
      char script_name[MAX_INPUT_LENGTH];

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_usage(ch);
        return;
      }

      if (is_number(arg3)) {
        tn = atoi(arg3);
      } else {
        if (!script_normalize_name(arg3, script_name, sizeof(script_name))) {
          send_to_char(ch, "Script name is invalid.\r\n");
          return;
        }
        tn = script_find_attached_trigger_vnum(room->proto_script, script_name, WLD_TRIGGER);
        if (tn == NOTHING) {
          send_to_char(ch, "That script is not attached.\r\n");
          return;
        }
      }
      rn = real_trigger(tn);
      if (rn != NOTHING && trig_index[rn] && trig_index[rn]->proto)
        tname = GET_TRIG_NAME(trig_index[rn]->proto);

      if (!proto_trigger_remove(&room->proto_script, tn)) {
        send_to_char(ch, "That trigger is not attached.\r\n");
        return;
      }

      if (SCRIPT(room))
        extract_script(room, WLD_TRIGGER);
      assign_triggers(room, WLD_TRIGGER);
      rset_mark_room_modified(rnum);

      if (!save_rooms(room->zone)) {
        send_to_char(ch, "Failed to write room data to disk.\r\n");
        return;
      }

      send_to_char(ch, "Trigger %d (%s) removed from room %d.\r\n",
                   tn, tname, room->number);
      return;
    }

    rset_show_del_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "desc")) {
    rset_show_desc_usage(ch);
    rset_desc_edit(ch, room);
    return;
  }

  if (is_abbrev(arg1, "clear")) {
    argument = one_argument(argument, arg2);
    if (!*arg2 || !is_abbrev(arg2, "force")) {
      rset_show_usage(ch);
      return;
    }
    if (*argument) {
      rset_show_usage(ch);
      return;
    }

    free_room_strings(room);
    room->name = strdup("An unfinished room");
    room->description = strdup("You are in an unfinished room.\r\n");
    room->sector_type = SECT_INSIDE;
    memset(room->room_flags, 0, sizeof(room->room_flags));
    rset_mark_room_modified(rnum);
    send_to_char(ch, "Room cleared.\r\n");
    return;
  }

  if (is_abbrev(arg1, "validate")) {
    if (*argument) {
      rset_show_validate_usage(ch);
      return;
    }
    rset_validate_room(ch, room);
    return;
  }

  rset_show_usage(ch);
}

static void oset_show_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Usage:\r\n"
    "  oset show <obj>\r\n"
    "  oset add keywords <obj> <keyword> [keywords]\r\n"
    "  oset add sdesc <obj> <text>\r\n"
    "  oset add ldesc <obj> <text>\r\n"
    "  oset add desc <obj>\r\n"
    "  oset add type <obj> <item type>\r\n"
    "  oset add flags <obj> <flags> [flags]\r\n"
    "  oset add wear <obj> <wear type> [wear types]\r\n"
    "  oset add weight <obj> <value>\r\n"
    "  oset add cost <obj> <value>\r\n"
    "  oset add oval <obj> <oval number> <value>\r\n"
    "  oset add edesc <obj> <keyword> <description>\r\n"
    "  oset add script <obj> <script name>\r\n"
    "  oset del <obj> <field>\r\n"
    "  oset clear <obj> force\r\n"
    "  oset validate <obj>\r\n");
}

static void oset_show_add_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds specific configuration to the object.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  oset add keywords <obj> <keyword> [keywords]\r\n"
    "  oset add sdesc <obj> <text>\r\n"
    "  oset add ldesc <obj> <text>\r\n"
    "  oset add desc <obj>\r\n"
    "  oset add type <obj> <item type>\r\n"
    "  oset add flags <obj> <flags> [flags]\r\n"
    "  oset add wear <obj> <wear type> [wear types]\r\n"
    "  oset add weight <obj> <value>\r\n"
    "  oset add cost <obj> <value>\r\n"
    "  oset add oval <obj> <oval number> <value>\r\n"
    "  oset add edesc <obj> <keyword> <description>\r\n"
    "  oset add script <obj> <script name>\r\n");
}

static void oset_show_add_keywords_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds keywords to object. Can add a single keyword or several at once.\r\n"
    "It is always best to use the most specific keyword as the first entry.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add keywords sword steel\r\n"
    "  oset add keywords armor padded\r\n"
    "  oset add keywords staff gnarled oak\r\n");
}

static void oset_show_add_sdesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a short description to the object. This is what is seen in an inventory,\r\n"
    "in a container, on furniture, or worn by someone.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add sdesc sword a wooden sword\r\n"
    "  oset add sdesc cloak a dark, hooded cloak\r\n"
    "  oset add sdesc maul a massive, obsidian-studded maul\r\n");
}

static void oset_show_add_ldesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a long description to the object. This is what everyone sees when an item\r\n"
    "is in a room.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add ldesc cloak A pile of dark fabric is here in a heap.\r\n"
    "  oset add ldesc maul Made of solid wood, a massive, obsidian-studded maul is here.\r\n"
    "  oset add ldesc bread A piece of bread has been discarded here.\r\n");
}

static void oset_show_add_type_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies the object type. Can only be one type.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add type chest container\r\n"
    "  oset add type armor armor\r\n"
    "  oset add type sword weapon\r\n"
    "\r\n"
    "Types:\r\n");
  column_list(ch, 0, item_types, NUM_ITEM_TYPES, FALSE);
}

static void oset_show_add_flags_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies object flags. Can be a single flag or multiples.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add flags sword no_drop\r\n"
    "  oset add flags staff hum glow\r\n"
    "\r\n"
    "Flags:\r\n");
  column_list(ch, 0, extra_bits, NUM_EXTRA_FLAGS, FALSE);
}

static void oset_show_add_wear_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies object wear types. Can be a single type or multiples.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add wear sword wield\r\n"
    "  oset add wear staff wield hold\r\n"
    "\r\n"
    "Wear types:\r\n");
  column_list(ch, 0, wear_bits, NUM_ITEM_WEARS, FALSE);
}

static void oset_show_add_weight_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies object weight. Affects carrying capacity of PC/NPC.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add weight sword 1\r\n");
}

static void oset_show_add_cost_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies object cost. Determines how much it sells for and is bought for.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add cost sword 100\r\n");
}

static void oset_show_add_oval_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Sets an oval property on an object. Each object type has its own respective\r\n"
    "ovals. Some of these influence different code paramters, such as the type\r\n"
    "CONTAINER having oval1 \"capacity\". This sets how much weight the container\r\n"
    "object can hold.\r\n"
    "\r\n"
    "For a list of ovals each item has, check HELP OVALS.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add oval chest capacity 100 (for a container)\r\n"
    "  oset add oval armor piece_ac 2 (for armor)\r\n"
    "  oset add oval sword weapon_type slashing (for weapons)\r\n");
}

static void oset_show_add_edesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds an extra description to the object.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  oset add edesc <obj> <keyword> <description>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add edesc sword rune A rune is etched along the blade.\r\n");
}

static void oset_show_del_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes specific configuration from the object.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  oset del <obj> keywords <keyword> [keywords]\r\n"
    "  oset del <obj> flags <flags> [flags]\r\n"
    "  oset del <obj> wear <wear type> [wear types]\r\n"
    "  oset del <obj> oval <oval number|oval name>\r\n"
    "  oset del <obj> edesc <keyword>\r\n"
    "  oset del <obj> script <script name>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset del sword keywords sword\r\n"
    "  oset del sword flags hum\r\n"
    "  oset del sword wear wield\r\n"
    "  oset del sword oval 2\r\n");
}

static void oset_show_del_flags_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes object flags.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  oset del <obj> flags <flag> [flags]\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset del sword flags hum\r\n"
    "\r\n"
    "Flags:\r\n");
  column_list(ch, 0, extra_bits, NUM_EXTRA_FLAGS, FALSE);
}

static void oset_show_del_wear_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes object wear types.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  oset del <obj> wear <wear type> [wear types]\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset del sword wear wield\r\n"
    "\r\n"
    "Wear types:\r\n");
  column_list(ch, 0, wear_bits, NUM_ITEM_WEARS, FALSE);
}

static void oset_show_clear_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Clears all configuration from an object to start over fresh.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  oset clear <obj> force\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset clear sword force\r\n");
}

static void oset_show_del_edesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes an extra description from the object.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  oset del <obj> edesc <keyword>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset del sword edesc rune\r\n");
}

static struct obj_data *oset_get_target_obj_keyword(struct char_data *ch, char *keyword)
{
  struct obj_data *obj;

  if (!ch || !keyword || !*keyword)
    return NULL;

  obj = get_obj_in_list_vis(ch, keyword, NULL, ch->carrying);
  if (obj)
    return obj;

  return get_obj_in_list_vis(ch, keyword, NULL, world[IN_ROOM(ch)].contents);
}

static void oset_replace_string(struct obj_data *obj, char **field, const char *value, const char *proto_field)
{
  if (*field && (!proto_field || *field != proto_field))
    free(*field);

  *field = strdup(value ? value : "");
}

static int oset_find_item_type(const char *arg)
{
  int i;

  if (!arg || !*arg)
    return -1;

  for (i = 0; *item_types[i] != '\n'; i++) {
    if (is_abbrev(arg, item_types[i]))
      return i;
  }

  return -1;
}

static int oset_find_extra_flag(const char *arg)
{
  int i;

  if (!arg || !*arg)
    return -1;

  for (i = 0; i < NUM_EXTRA_FLAGS; i++) {
    if (is_abbrev(arg, extra_bits[i]))
      return i;
  }

  return -1;
}

static int oset_find_wear_flag(const char *arg)
{
  int i;

  if (!arg || !*arg)
    return -1;

  for (i = 0; i < NUM_ITEM_WEARS; i++) {
    if (is_abbrev(arg, wear_bits[i]))
      return i;
  }

  return -1;
}

static int oset_find_oval_by_name(const char *arg, const char * const *labels)
{
  int i;

  if (!arg || !*arg || !labels)
    return -1;

  for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
    if (labels[i] && *labels[i] && is_abbrev(arg, labels[i]))
      return i;
  }

  return -1;
}

static int oset_find_attack_type(const char *arg)
{
  int i;

  if (!arg || !*arg)
    return -1;

  for (i = 0; i < NUM_ATTACK_TYPES; i++) {
    if (is_abbrev(arg, attack_hit_text[i].singular) ||
        is_abbrev(arg, attack_hit_text[i].plural))
      return i;
  }

  if (!str_cmp(arg, "slashing"))
    return oset_find_attack_type("slash");
  if (!str_cmp(arg, "piercing"))
    return oset_find_attack_type("pierce");
  if (!str_cmp(arg, "bludgeoning"))
    return oset_find_attack_type("bludgeon");

  return -1;
}

static bool oset_add_keywords(struct char_data *ch, struct obj_data *obj, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  bool changed = FALSE;
  obj_rnum rnum = GET_OBJ_RNUM(obj);
  const char *proto_name = (rnum != NOTHING) ? obj_proto[rnum].name : NULL;

  buf[0] = '\0';
  if (obj->name && *obj->name)
    strlcpy(buf, obj->name, sizeof(buf));

  while (*argument) {
    argument = one_argument(argument, arg);
    if (!*arg)
      break;

    if (!isname(arg, buf)) {
      size_t needed = strlen(buf) + strlen(arg) + 2;
      if (needed >= sizeof(buf)) {
        send_to_char(ch, "Keyword list is too long.\r\n");
        return FALSE;
      }
      if (*buf)
        strlcat(buf, " ", sizeof(buf));
      strlcat(buf, arg, sizeof(buf));
      changed = TRUE;
    }
  }

  if (!changed) {
    send_to_char(ch, "Keywords unchanged.\r\n");
    return TRUE;
  }

  oset_replace_string(obj, &obj->name, buf, proto_name);
  send_to_char(ch, "Keywords updated.\r\n");
  return TRUE;
}

static bool oset_del_keywords(struct char_data *ch, struct obj_data *obj, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  char work[MAX_STRING_LENGTH];
  char word[MAX_INPUT_LENGTH];
  bool changed = FALSE;
  obj_rnum rnum = GET_OBJ_RNUM(obj);
  const char *proto_name = (rnum != NOTHING) ? obj_proto[rnum].name : NULL;
  char *ptr;

  buf[0] = '\0';
  if (!obj->name || !*obj->name) {
    send_to_char(ch, "Object has no keywords to remove.\r\n");
    return TRUE;
  }

  strlcpy(work, obj->name, sizeof(work));
  ptr = work;
  while (*ptr) {
    ptr = one_argument(ptr, word);
    if (!*word)
      break;

    if (isname(word, argument)) {
      changed = TRUE;
      continue;
    }

    if (*buf)
      strlcat(buf, " ", sizeof(buf));
    strlcat(buf, word, sizeof(buf));
  }

  if (!changed) {
    send_to_char(ch, "No matching keywords found.\r\n");
    return TRUE;
  }

  oset_replace_string(obj, &obj->name, buf, proto_name);
  send_to_char(ch, "Keywords updated.\r\n");
  return TRUE;
}

static void oset_show_object(struct char_data *ch, struct obj_data *obj)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int i;
  const char * const *labels = obj_value_labels(GET_OBJ_TYPE(obj));

  sprinttype(GET_OBJ_TYPE(obj), item_types, buf, sizeof(buf));
  sprintbitarray(GET_OBJ_EXTRA(obj), extra_bits, EF_ARRAY_MAX, buf2);

  send_to_char(ch, "Object [%d]: %s\r\n",
    GET_OBJ_VNUM(obj), obj->short_description ? obj->short_description : "<None>");
  send_to_char(ch, "Keywords: %s\r\n", obj->name ? obj->name : "<None>");
  send_to_char(ch, "Short desc: %s\r\n", obj->short_description ? obj->short_description : "<None>");
  send_to_char(ch, "Long desc: %s\r\n", obj->description ? obj->description : "<None>");
  send_to_char(ch, "Main desc:\r\n%s", obj->main_description ? obj->main_description : "  <None>\r\n");
  send_to_char(ch, "Type: %s\r\n", buf);
  send_to_char(ch, "Flags: %s\r\n", buf2);
  sprintbitarray(GET_OBJ_WEAR(obj), wear_bits, TW_ARRAY_MAX, buf);
  send_to_char(ch, "Wear: %s\r\n", buf);
  send_to_char(ch, "Weight: %d\r\n", GET_OBJ_WEIGHT(obj));
  send_to_char(ch, "Cost: %d\r\n", GET_OBJ_COST(obj));

  send_to_char(ch, "Ovals:\r\n");
  for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
    const char *label = labels ? labels[i] : "Value";
    send_to_char(ch, "  [%d] %s: %d\r\n", i, label, GET_OBJ_VAL(obj, i));
  }

  send_to_char(ch, "Extra Descs:\r\n");
  i = 0;
  for (struct extra_descr_data *desc = obj->ex_description; desc; desc = desc->next) {
    send_to_char(ch, "  %s\r\n", desc->keyword ? desc->keyword : "<None>");
    i++;
  }
  if (!i)
    send_to_char(ch, "  None.\r\n");
}

static void oset_desc_edit(struct char_data *ch, struct obj_data *obj)
{
  char *oldtext = NULL;

  send_editor_help(ch->desc);
  write_to_output(ch->desc, "Enter object description:\r\n\r\n");

  if (obj->main_description) {
    write_to_output(ch->desc, "%s", obj->main_description);
    oldtext = strdup(obj->main_description);
  }

  string_write(ch->desc, &obj->main_description, MAX_MESSAGE_LENGTH, 0, oldtext);
}

static void oset_clear_object(struct obj_data *obj)
{
  obj_rnum rnum = GET_OBJ_RNUM(obj);

  if (rnum != NOTHING)
    free_object_strings_proto(obj);
  else
    free_object_strings(obj);

  obj->name = NULL;
  obj->description = NULL;
  obj->short_description = NULL;
  obj->main_description = NULL;
  obj->ex_description = NULL;

  obj->name = strdup("unfinished object");
  obj->description = strdup("An unfinished object is lying here.");
  obj->short_description = strdup("an unfinished object");

  GET_OBJ_TYPE(obj) = 0;
  GET_OBJ_WEIGHT(obj) = 0;
  GET_OBJ_COST(obj) = 0;
  GET_OBJ_COST_PER_DAY(obj) = 0;
  GET_OBJ_TIMER(obj) = 0;
  GET_OBJ_LEVEL(obj) = 1;

  memset(obj->obj_flags.extra_flags, 0, sizeof(obj->obj_flags.extra_flags));
  memset(obj->obj_flags.wear_flags, 0, sizeof(obj->obj_flags.wear_flags));
  memset(obj->obj_flags.value, 0, sizeof(obj->obj_flags.value));
  memset(obj->obj_flags.bitvector, 0, sizeof(obj->obj_flags.bitvector));
  memset(obj->affected, 0, sizeof(obj->affected));

  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
}

static void oset_validate_object(struct char_data *ch, struct obj_data *obj)
{
  int errors = 0;

  if (!obj->name || !*obj->name) {
    send_to_char(ch, "Error: keywords are not set.\r\n");
    errors++;
  }

  if (!obj->short_description || !*obj->short_description) {
    send_to_char(ch, "Error: short description is not set.\r\n");
    errors++;
  }

  if (!obj->description || !*obj->description) {
    send_to_char(ch, "Error: long description is not set.\r\n");
    errors++;
  }

  if (!obj->main_description || !*obj->main_description) {
    send_to_char(ch, "Error: main description is not set.\r\n");
    errors++;
  }

  if (GET_OBJ_TYPE(obj) < 1 || GET_OBJ_TYPE(obj) >= NUM_ITEM_TYPES) {
    send_to_char(ch, "Error: object type is not set.\r\n");
    errors++;
  }

  if (GET_OBJ_WEIGHT(obj) <= 0) {
    send_to_char(ch, "Error: weight must be above zero.\r\n");
    errors++;
  }

  if (GET_OBJ_COST(obj) <= 0) {
    send_to_char(ch, "Error: cost must be above zero.\r\n");
    errors++;
  }

  if (GET_OBJ_LEVEL(obj) != 1) {
    send_to_char(ch, "Error: object level must be 1.\r\n");
    errors++;
  }

  if (GET_OBJ_TIMER(obj) != 0) {
    send_to_char(ch, "Error: object timer must be 0.\r\n");
    errors++;
  }

  if (GET_OBJ_COST_PER_DAY(obj) != 0) {
    send_to_char(ch, "Error: cost per day must be 0.\r\n");
    errors++;
  }

  for (struct extra_descr_data *desc = obj->ex_description; desc; desc = desc->next) {
    if (!desc->keyword || !*desc->keyword) {
      send_to_char(ch, "Error: extra description is missing a keyword.\r\n");
      errors++;
    }
    if (!desc->description || !*desc->description) {
      send_to_char(ch, "Error: extra description for %s is missing text.\r\n",
        desc->keyword ? desc->keyword : "<None>");
      errors++;
    }
  }

  if (!errors)
    send_to_char(ch, "Object validates cleanly.\r\n");
  else
    send_to_char(ch, "Validation failed: %d issue%s.\r\n", errors, errors == 1 ? "" : "s");
}

ACMD(do_oset)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  char arg4[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  trig_data *trig;
  int tn, rn;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "oset is only usable by connected players.\r\n");
    return;
  }

  argument = one_argument(argument, arg1);

  if (!*arg1) {
    oset_show_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "show")) {
    char *keyword;

    skip_spaces(&argument);
    keyword = argument;
    if (!*keyword) {
      send_to_char(ch, "Provide a keyword of an object to show.\r\n");
      return;
    }
    obj = oset_get_target_obj_keyword(ch, keyword);
    if (!obj) {
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(keyword), keyword);
      return;
    }
    oset_show_object(ch, obj);
    return;
  }

  if (is_abbrev(arg1, "add")) {
    argument = one_argument(argument, arg2);
    if (!*arg2) {
      oset_show_add_usage(ch);
      return;
    }

    if (is_abbrev(arg2, "keywords")) {
      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_keywords_usage(ch);
        return;
      }
      if (!*argument) {
        oset_show_add_keywords_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      oset_add_keywords(ch, obj, argument);
      return;
    }

    if (is_abbrev(arg2, "sdesc")) {
      static size_t max_len = 64;
      obj_rnum rnum;
      const char *proto_sdesc;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_sdesc_usage(ch);
        return;
      }
      skip_spaces(&argument);
      if (!*argument) {
        oset_show_add_sdesc_usage(ch);
        return;
      }
      if (strlen(argument) > max_len) {
        send_to_char(ch, "Short description is too long.\r\n");
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      rnum = GET_OBJ_RNUM(obj);
      proto_sdesc = (rnum != NOTHING) ? obj_proto[rnum].short_description : NULL;
      oset_replace_string(obj, &obj->short_description, argument, proto_sdesc);
      send_to_char(ch, "Short description set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "ldesc")) {
      static size_t max_len = 128;
      obj_rnum rnum;
      const char *proto_ldesc;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_ldesc_usage(ch);
        return;
      }
      skip_spaces(&argument);
      if (!*argument) {
        oset_show_add_ldesc_usage(ch);
        return;
      }
      if (strlen(argument) > max_len) {
        send_to_char(ch, "Long description is too long.\r\n");
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      rnum = GET_OBJ_RNUM(obj);
      proto_ldesc = (rnum != NOTHING) ? obj_proto[rnum].description : NULL;
      oset_replace_string(obj, &obj->description, argument, proto_ldesc);
      send_to_char(ch, "Long description set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "desc")) {
      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_usage(ch);
        return;
      }
      if (*argument) {
        oset_show_add_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      oset_desc_edit(ch, obj);
      return;
    }

    if (is_abbrev(arg2, "type")) {
      int type;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_type_usage(ch);
        return;
      }
      skip_spaces(&argument);
      if (!*argument) {
        oset_show_add_type_usage(ch);
        return;
      }
      type = oset_find_item_type(argument);
      if (type < 0 || type >= NUM_ITEM_TYPES) {
        send_to_char(ch, "Invalid object type.\r\n");
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      GET_OBJ_TYPE(obj) = type;
      send_to_char(ch, "Object type set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "flags")) {
      bool any = FALSE;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_flags_usage(ch);
        return;
      }
      if (!*argument) {
        oset_show_add_flags_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = oset_find_extra_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown flag: %s\r\n", arg3);
          continue;
        }

        SET_BIT_AR(GET_OBJ_EXTRA(obj), flag);
        any = TRUE;
      }
      if (any)
        send_to_char(ch, "Object flags updated.\r\n");
      return;
    }

    if (is_abbrev(arg2, "wear")) {
      bool any = FALSE;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_wear_usage(ch);
        return;
      }
      if (!*argument) {
        oset_show_add_wear_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = oset_find_wear_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown wear type: %s\r\n", arg3);
          continue;
        }

        SET_BIT_AR(GET_OBJ_WEAR(obj), flag);
        any = TRUE;
      }
      if (any)
        send_to_char(ch, "Wear flags updated.\r\n");
      return;
    }

    if (is_abbrev(arg2, "weight")) {
      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_weight_usage(ch);
        return;
      }
      skip_spaces(&argument);
      if (!*argument || !is_number(argument)) {
        oset_show_add_weight_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      GET_OBJ_WEIGHT(obj) = LIMIT(atoi(argument), 0, MAX_OBJ_WEIGHT);
      send_to_char(ch, "Weight set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "cost")) {
      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_cost_usage(ch);
        return;
      }
      skip_spaces(&argument);
      if (!*argument || !is_number(argument)) {
        oset_show_add_cost_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      GET_OBJ_COST(obj) = LIMIT(atoi(argument), 0, MAX_OBJ_COST);
      send_to_char(ch, "Cost set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "oval")) {
      int pos;
      int value;
      const char * const *labels;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg4);
      if (!*arg3 || !*arg4) {
        oset_show_add_oval_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      argument = one_argument(argument, arg1);
      if (!*arg1) {
        oset_show_add_oval_usage(ch);
        return;
      }

      labels = obj_value_labels(GET_OBJ_TYPE(obj));
      if (is_number(arg4)) {
        pos = atoi(arg4);
      } else {
        pos = oset_find_oval_by_name(arg4, labels);
      }

      if (pos < 0 || pos >= NUM_OBJ_VAL_POSITIONS) {
        send_to_char(ch, "Invalid oval position.\r\n");
        return;
      }

      if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && pos == 2 &&
          !is_number(arg1)) {
        value = oset_find_attack_type(arg1);
        if (value < 0) {
          send_to_char(ch, "Unknown weapon type: %s\r\n", arg1);
          return;
        }
      } else {
        if (!is_number(arg1)) {
          send_to_char(ch, "Oval value must be numeric.\r\n");
          return;
        }
        value = atoi(arg1);
      }

      GET_OBJ_VAL(obj, pos) = value;
      send_to_char(ch, "Oval set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "edesc")) {
      struct extra_descr_data *desc;
      char *keyword;
      char *edesc;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg4);
      skip_spaces(&argument);
      keyword = arg4;
      edesc = argument;

      if (!*arg3 || !*keyword || !*edesc) {
        oset_show_add_edesc_usage(ch);
        return;
      }

      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }

      genolc_checkstring(ch->desc, edesc);
      genolc_checkstring(ch->desc, keyword);

      CREATE(desc, struct extra_descr_data, 1);
      desc->keyword = str_udup(keyword);
      desc->description = str_udup(edesc);
      desc->next = obj->ex_description;
      obj->ex_description = desc;
      send_to_char(ch, "Extra description added.\r\n");
      return;
    }

    if (is_abbrev(arg2, "script")) {
      obj_rnum robj_num;
      obj_vnum vnum;
      zone_rnum znum;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_usage(ch);
        return;
      }
      skip_spaces(&argument);
      if (!*argument) {
        oset_show_add_usage(ch);
        return;
      }

      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }

      robj_num = GET_OBJ_RNUM(obj);
      vnum = GET_OBJ_VNUM(obj);
      if (robj_num == NOTHING || vnum == NOTHING) {
        send_to_char(ch, "That object has no valid vnum.\r\n");
        return;
      }

      if (!can_edit_zone(ch, real_zone_by_thing(vnum))) {
        send_to_char(ch, "You do not have permission to modify that zone.\r\n");
        return;
      }

      znum = real_zone_by_thing(vnum);
      if (znum == NOWHERE) {
        send_to_char(ch, "That object has no valid zone.\r\n");
        return;
      }

      tn = script_resolve_trigger_vnum(ch, argument, znum, OBJ_TRIGGER);
      if (tn == NOTHING)
        return;
      rn = real_trigger(tn);
      if (rn == NOTHING || trig_index[rn] == NULL || trig_index[rn]->proto == NULL) {
        send_to_char(ch, "That trigger does not exist.\r\n");
        return;
      }
      trig = trig_index[rn]->proto;

      if (!proto_trigger_add(&obj_proto[robj_num].proto_script, tn)) {
        send_to_char(ch, "That trigger is already attached.\r\n");
        return;
      }

      {
        struct obj_data *obj_it;
        struct obj_data *target_obj = obj;

        for (obj_it = object_list; obj_it; obj_it = obj_it->next) {
          if (obj_it->item_number != robj_num)
            continue;
          if (SCRIPT(obj_it))
            extract_script(obj_it, OBJ_TRIGGER);
          free_proto_script(obj_it, OBJ_TRIGGER);
          copy_proto_script(&obj_proto[robj_num], obj_it, OBJ_TRIGGER);
          assign_triggers(obj_it, OBJ_TRIGGER);
        }

        if (znum == NOWHERE || !save_objects(znum)) {
          send_to_char(ch, "Failed to write object data to disk.\r\n");
          return;
        }

        send_to_char(ch, "Trigger %d (%s) attached to %s [%d].\r\n",
                     tn, GET_TRIG_NAME(trig),
                     (target_obj->short_description ? target_obj->short_description : target_obj->name),
                     GET_OBJ_VNUM(target_obj));
      }
      return;
    }

    oset_show_add_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "del")) {
    argument = one_argument(argument, arg2);
    if (!*arg2) {
      oset_show_del_usage(ch);
      return;
    }

    argument = one_argument(argument, arg3);
    if (!*arg3) {
      oset_show_del_usage(ch);
      return;
    }

    if (is_abbrev(arg3, "keywords")) {
      if (!*argument) {
        oset_show_del_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg2);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg2), arg2);
        return;
      }
      oset_del_keywords(ch, obj, argument);
      return;
    }

    if (is_abbrev(arg3, "flags")) {
      bool any = FALSE;

      if (!*argument) {
        oset_show_del_flags_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg2);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg2), arg2);
        return;
      }
      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = oset_find_extra_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown flag: %s\r\n", arg3);
          continue;
        }

        REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), flag);
        any = TRUE;
      }
      if (any)
        send_to_char(ch, "Object flags updated.\r\n");
      return;
    }

    if (is_abbrev(arg3, "wear")) {
      bool any = FALSE;

      if (!*argument) {
        oset_show_del_wear_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg2);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg2), arg2);
        return;
      }
      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = oset_find_wear_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown wear type: %s\r\n", arg3);
          continue;
        }

        REMOVE_BIT_AR(GET_OBJ_WEAR(obj), flag);
        any = TRUE;
      }
      if (any)
        send_to_char(ch, "Wear flags updated.\r\n");
      return;
    }

    if (is_abbrev(arg3, "oval")) {
      int pos;
      const char * const *labels;

      argument = one_argument(argument, arg4);
      if (!*arg4) {
        oset_show_del_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg2);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg2), arg2);
        return;
      }

      labels = obj_value_labels(GET_OBJ_TYPE(obj));
      if (is_number(arg4)) {
        pos = atoi(arg4);
      } else {
        pos = oset_find_oval_by_name(arg4, labels);
      }

      if (pos < 0 || pos >= NUM_OBJ_VAL_POSITIONS) {
        send_to_char(ch, "Invalid oval position.\r\n");
        return;
      }

      GET_OBJ_VAL(obj, pos) = 0;
      send_to_char(ch, "Oval cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "edesc")) {
      struct extra_descr_data *desc;
      struct extra_descr_data *prev = NULL;

      argument = one_argument(argument, arg4);
      if (!*arg4) {
        oset_show_del_edesc_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg2);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg2), arg2);
        return;
      }

      for (desc = obj->ex_description; desc; desc = desc->next) {
        if (desc->keyword && isname(arg4, desc->keyword))
          break;
        prev = desc;
      }

      if (!desc) {
        send_to_char(ch, "No extra description found for %s.\r\n", arg4);
        return;
      }

      if (prev)
        prev->next = desc->next;
      else
        obj->ex_description = desc->next;

      if (desc->keyword)
        free(desc->keyword);
      if (desc->description)
        free(desc->description);
      free(desc);
      send_to_char(ch, "Extra description removed.\r\n");
      return;
    }

    if (is_abbrev(arg3, "script")) {
      obj_rnum robj_num;
      obj_vnum vnum;
      zone_rnum znum;
      const char *tname = "unknown";
      char script_name[MAX_INPUT_LENGTH];

      argument = one_argument(argument, arg4);
      if (!*arg4) {
        oset_show_del_usage(ch);
        return;
      }

      obj = oset_get_target_obj_keyword(ch, arg2);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg2), arg2);
        return;
      }

      robj_num = GET_OBJ_RNUM(obj);
      vnum = GET_OBJ_VNUM(obj);
      if (robj_num == NOTHING || vnum == NOTHING) {
        send_to_char(ch, "That object has no valid vnum.\r\n");
        return;
      }

      if (!can_edit_zone(ch, real_zone_by_thing(vnum))) {
        send_to_char(ch, "You do not have permission to modify that zone.\r\n");
        return;
      }

      if (is_number(arg4)) {
        tn = atoi(arg4);
      } else {
        if (!script_normalize_name(arg4, script_name, sizeof(script_name))) {
          send_to_char(ch, "Script name is invalid.\r\n");
          return;
        }
        tn = script_find_attached_trigger_vnum(obj_proto[robj_num].proto_script,
                                               script_name, OBJ_TRIGGER);
        if (tn == NOTHING) {
          send_to_char(ch, "That script is not attached.\r\n");
          return;
        }
      }

      rn = real_trigger(tn);
      if (rn != NOTHING && trig_index[rn] && trig_index[rn]->proto)
        tname = GET_TRIG_NAME(trig_index[rn]->proto);

      if (!proto_trigger_remove(&obj_proto[robj_num].proto_script, tn)) {
        send_to_char(ch, "That trigger is not attached.\r\n");
        return;
      }

      {
        struct obj_data *obj_it;
        struct obj_data *target_obj = obj;

        for (obj_it = object_list; obj_it; obj_it = obj_it->next) {
          if (obj_it->item_number != robj_num)
            continue;
          if (SCRIPT(obj_it))
            extract_script(obj_it, OBJ_TRIGGER);
          free_proto_script(obj_it, OBJ_TRIGGER);
          copy_proto_script(&obj_proto[robj_num], obj_it, OBJ_TRIGGER);
          assign_triggers(obj_it, OBJ_TRIGGER);
        }

        znum = real_zone_by_thing(vnum);
        if (znum == NOWHERE || !save_objects(znum)) {
          send_to_char(ch, "Failed to write object data to disk.\r\n");
          return;
        }

        send_to_char(ch, "Trigger %d (%s) removed from %s [%d].\r\n",
                     tn, tname,
                     (target_obj->short_description ? target_obj->short_description : target_obj->name),
                     GET_OBJ_VNUM(target_obj));
      }
      return;
    }

    oset_show_del_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "clear")) {
    argument = one_argument(argument, arg2);
    if (!*arg2) {
      oset_show_clear_usage(ch);
      return;
    }
    obj = oset_get_target_obj_keyword(ch, arg2);
    if (!obj) {
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg2), arg2);
      return;
    }
    argument = one_argument(argument, arg3);
    if (!*arg3 || !is_abbrev(arg3, "force")) {
      oset_show_clear_usage(ch);
      return;
    }
    if (*argument) {
      oset_show_clear_usage(ch);
      return;
    }
    oset_clear_object(obj);
    send_to_char(ch, "Object cleared.\r\n");
    return;
  }

  if (is_abbrev(arg1, "validate")) {
    char *keyword;

    skip_spaces(&argument);
    keyword = argument;
    if (!*keyword) {
      send_to_char(ch, "Provide a keyword of an object to validate.\r\n");
      return;
    }
    obj = oset_get_target_obj_keyword(ch, keyword);
    if (!obj) {
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(keyword), keyword);
      return;
    }
    oset_validate_object(ch, obj);
    return;
  }

  oset_show_usage(ch);
}

static const char *mset_stat_types[] = {
  "str",
  "dex",
  "con",
  "int",
  "wis",
  "cha"
};

static void mset_show_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Usage:\r\n"
    "  mset show <npc>\r\n"
    "  mset add name <npc> <text>\r\n"
    "  mset add keywords <npc> <keyword> [keywords]\r\n"
    "  mset add sdesc <npc> <text>\r\n"
    "  mset add ldesc <npc> <text>\r\n"
    "  mset add desc <npc> (enters editor)\r\n"
    "  mset add background <npc> (enters editor)\r\n"
    "  mset add edesc <npc> <keyword> <description>\r\n"
    "  mset add attack <npc> <attack type>\r\n"
    "  mset add sex <npc> <male/female/neutral>\r\n"
    "  mset add species <npc> <species name>\r\n"
    "  mset add class <npc> <class name>\r\n"
    "  mset add stat <npc> <stat> <value>\r\n"
    "  mset add save <npc> <stat> <value>\r\n"
    "  mset add skill <npc> <skill name> <skill level>\r\n"
    "  mset add flags <npc> <flags> [flags]\r\n"
    "  mset add affect <npc> <affect> [affects]\r\n"
    "  mset add skinning <npc> <vnum> <dc>\r\n"
    "  mset add script <npc> <script name>\r\n"
    "  mset del <npc>\r\n"
    "  mset clear <npc>\r\n"
    "  mset validate <npc>\r\n");
}

static void mset_show_add_name_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a name to an NPC. Try to be creative when adding a name!\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add name <npc> kinther\r\n"
    "  mset add name <npc> tektolnes\r\n"
    "  mset add name <npc> saddira\r\n");
}

static void mset_show_add_keywords_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds keywords to an NPC. Can add a single keyword or several at once.\r\n"
    "It is always best to use the most specific keyword as the first entry.\r\n"
    "These should be equivalent to what your short description is, minus\r\n"
    "any words like \"the\".\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add keywords <npc> soldier thick burly\r\n"
    "  mset add keywords <npc> templar tall bald\r\n"
    "  mset add keywords <npc> elf lithe feminine\r\n");
}

static void mset_show_add_sdesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a short description to an NPC. This is what is seen when looking\r\n"
    "at the NPC, in combat, when it speaks, and emotes.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add sdesc <npc> the thick, burly soldier\r\n"
    "  mset add sdesc <npc> the tall, bald templar\r\n"
    "  mset add sdesc <npc> the lithe, feminine elf\r\n");
}

static void mset_show_add_ldesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a long description to an NPC. This is what everyone sees when an NPC\r\n"
    "is in a room.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add ldesc <npc> The thick, burly soldier is eyeing the crowd here.\r\n"
    "  mset add ldesc <npc> The tall, bald templar is here barking orders at a work crew.\r\n"
    "  mset add ldesc <npc> The lithe, feminine elf is leaning against a wall here.\r\n");
}

static void mset_show_add_desc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Enters text editor for editing the main description of the NPC.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  mset add desc <npc>\r\n");
}

static void mset_show_add_background_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Enters text editor for editing the NPC background.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  mset add background <npc>\r\n");
}

static void mset_show_add_edesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds an extra description to the NPC.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  mset add edesc <npc> <keyword> <description>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add edesc guard scar A jagged scar cuts across the cheek.\r\n");
}

static void mset_show_add_attack_usage(struct char_data *ch)
{
  const char *types[NUM_ATTACK_TYPES];
  int i;

  send_to_char(ch,
    "Specifies the attack type of the NPC. Can only be one type.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add attack <npc> hit\r\n"
    "  mset add attack <npc> bite\r\n"
    "  mset add attack <npc> claw\r\n"
    "\r\n"
    "Types:\r\n");

  for (i = 0; i < NUM_ATTACK_TYPES; i++)
    types[i] = attack_hit_text[i].singular;
  column_list(ch, 0, types, NUM_ATTACK_TYPES, FALSE);
}

static void mset_show_add_sex_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies NPC sex.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add sex <npc> male\r\n"
    "  mset add sex <npc> female\r\n"
    "  mset add sex <npc> neutral\r\n");
}

static void mset_show_add_species_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies the species of the NPC. Can only be one type.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add species <npc> human\r\n"
    "\r\n"
    "Species:\r\n");
  column_list(ch, 0, species_types, NUM_SPECIES, FALSE);
}

static void mset_show_add_class_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies the class of the NPC. Can only be one type.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add class <npc> fighter\r\n"
    "\r\n"
    "Classes:\r\n");
  column_list(ch, 0, pc_class_types, NUM_CLASSES, FALSE);
}

static void mset_show_add_stat_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies the stat of the NPC. Can only be one type. Defaults to 10.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add stat <npc> str 18\r\n"
    "\r\n"
    "Stats:\r\n");
  column_list(ch, 0, mset_stat_types, NUM_ABILITIES, FALSE);
}

static void mset_show_add_save_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies an additional saving throw bonus to the stat of the NPC.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add save <npc> str 1\r\n"
    "\r\n"
    "Stats:\r\n");
  column_list(ch, 0, mset_stat_types, NUM_ABILITIES, FALSE);
}

static void mset_show_add_skill_usage(struct char_data *ch)
{
  const char *skills[MAX_SKILLS];
  int count = 0;
  int i;

  send_to_char(ch,
    "Specifies a skill the NPC has, and what skill level. For a full list, type \"skills\".\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add skill <npc> kick 40\r\n"
    "  mset add skill <npc> fireball 80\r\n"
    "  mset add skill <npc> hide 10\r\n"
    "\r\n"
    "Skills:\r\n");

  for (i = MAX_SPELLS + 1; i < MAX_SKILLS && i <= TOP_SPELL_DEFINE; i++) {
    if (spell_info[i].name &&
        str_cmp(spell_info[i].name, "UNDEFINED") &&
        str_cmp(spell_info[i].name, "UNUSED")) {
      skills[count++] = spell_info[i].name;
    }
  }

  if (count)
    column_list(ch, 0, skills, count, FALSE);
  else
    send_to_char(ch, "  <None>\r\n");
}

static void mset_show_add_flags_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies a flag to be assigned to the NPC. Can be one or a list of flags.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add flag <npc> SENTINEL\r\n"
    "  mset add flag <npc> SENTINEL MEMORY HELPER\r\n"
    "\r\n"
    "Flags:\r\n");
  column_list(ch, 0, action_bits, NUM_MOB_FLAGS, FALSE);
}

static void mset_show_add_affect_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies a affect to be assigned to the NPC. Can be one or a list of flags.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add affect <npc> FLY\r\n"
    "  mset add affect <npc> BLIND NO_TRACK\r\n"
    "\r\n"
    "Affects:\r\n");
  column_list(ch, 0, affected_bits + 1, NUM_AFF_FLAGS - 1, FALSE);
}

static void mset_show_add_skinning_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies an item vnum that can be skinned from the corpse of the NPC, and DC check for success.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset add skinning <npc> 134 15\r\n");
}

static void mset_show_del_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes specific configuration from the NPC.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  mset del <npc> name\r\n"
    "  mset del <npc> keywords <keyword> [keywords]\r\n"
    "  mset del <npc> sdesc\r\n"
    "  mset del <npc> ldesc\r\n"
    "  mset del <npc> desc\r\n"
    "  mset del <npc> background\r\n"
    "  mset del <npc> edesc <keyword>\r\n"
    "  mset del <npc> attack\r\n"
    "  mset del <npc> sex\r\n"
    "  mset del <npc> species\r\n"
    "  mset del <npc> class\r\n"
    "  mset del <npc> stat <stat>\r\n"
    "  mset del <npc> save <stat>\r\n"
    "  mset del <npc> skill <skill name>\r\n"
    "  mset del <npc> flags <flags> [flags]\r\n"
    "  mset del <npc> affect <affect> [affects]\r\n"
    "  mset del <npc> skinning <vnum>\r\n"
    "  mset del <npc> script <script name>\r\n");
}

static void mset_show_del_edesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes an extra description from the NPC.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  mset del <npc> edesc <keyword>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  mset del guard edesc scar\r\n");
}

static void mset_show_del_flags_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes NPC flags.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  mset del <npc> flags <flag> [flags]\r\n"
    "\r\n"
    "Flags:\r\n");
  column_list(ch, 0, action_bits, NUM_MOB_FLAGS, FALSE);
}

static void mset_show_del_affect_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes NPC affects.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  mset del <npc> affect <affect> [affects]\r\n"
    "\r\n"
    "Affects:\r\n");
  column_list(ch, 0, affected_bits + 1, NUM_AFF_FLAGS - 1, FALSE);
}

static bool mset_illegal_mob_flag(int fl)
{
  int i;
  const int illegal_flags[] = {
    MOB_ISNPC,
    MOB_NOTDEADYET,
  };
  const int num_illegal_flags = sizeof(illegal_flags) / sizeof(int);

  for (i = 0; i < num_illegal_flags; i++)
    if (fl == illegal_flags[i])
      return TRUE;

  return FALSE;
}

static int mset_find_mob_flag(const char *arg)
{
  int i;

  if (!arg || !*arg)
    return -1;

  for (i = 0; i < NUM_MOB_FLAGS; i++)
    if (is_abbrev(arg, action_bits[i]))
      return i;

  return -1;
}

static int mset_find_affect_flag(const char *arg)
{
  int i;

  if (!arg || !*arg)
    return -1;

  for (i = 1; i < NUM_AFF_FLAGS; i++)
    if (is_abbrev(arg, affected_bits[i]))
      return i;

  return -1;
}

static int mset_find_stat(const char *arg)
{
  int i;

  if (!arg || !*arg)
    return -1;

  for (i = 0; i < NUM_ABILITIES; i++)
    if (is_abbrev(arg, mset_stat_types[i]))
      return i;

  return -1;
}

static int mset_find_sex(const char *arg)
{
  char sexbuf[MAX_INPUT_LENGTH];
  int sex;

  if (!arg || !*arg)
    return -1;

  strlcpy(sexbuf, arg, sizeof(sexbuf));
  sex = search_block(sexbuf, genders, FALSE);
  if (sex < 0 || sex >= NUM_GENDERS)
    return -1;

  return sex;
}

static void mset_mark_mob_modified(mob_vnum vnum)
{
  zone_rnum znum;

  if (vnum == NOBODY || vnum <= 0)
    return;

  znum = real_zone_by_thing(vnum);
  if (znum == NOWHERE)
    return;

  add_to_save_list(zone_table[znum].number, SL_MOB);
}

static void mset_update_proto_strings(mob_rnum rnum)
{
  struct char_data *mob;

  if (rnum < 0)
    return;

  for (mob = character_list; mob; mob = mob->next) {
    if (GET_MOB_RNUM(mob) == rnum)
      update_mobile_strings(mob, &mob_proto[rnum]);
  }
}

static void mset_update_proto_keywords(mob_rnum rnum, const char *value)
{
  struct char_data *mob;
  char *old;

  if (rnum < 0)
    return;

  old = mob_proto[rnum].player.keywords;
  mob_proto[rnum].player.keywords = strdup(value ? value : "");

  for (mob = character_list; mob; mob = mob->next) {
    if (GET_MOB_RNUM(mob) == rnum && GET_KEYWORDS(mob) == old)
      GET_KEYWORDS(mob) = mob_proto[rnum].player.keywords;
  }

  if (old)
    free(old);
}

static void mset_update_proto_edesc(mob_rnum rnum, struct extra_descr_data *old)
{
  struct char_data *mob;

  if (rnum < 0)
    return;

  for (mob = character_list; mob; mob = mob->next) {
    if (GET_MOB_RNUM(mob) == rnum && mob->mob_specials.ex_description == old)
      mob->mob_specials.ex_description = mob_proto[rnum].mob_specials.ex_description;
  }
}

static void mset_replace_string(struct char_data *mob, char **field, const char *value, const char *proto_field)
{
  if (*field && (!proto_field || *field != proto_field))
    free(*field);

  *field = strdup(value ? value : "");
}

static void mset_set_stat_value(struct char_data *mob, int stat, int value, bool apply_affects)
{
  switch (stat) {
    case ABIL_STR: mob->real_abils.str = value; break;
    case ABIL_DEX: mob->real_abils.dex = value; break;
    case ABIL_CON: mob->real_abils.con = value; break;
    case ABIL_INT: mob->real_abils.intel = value; break;
    case ABIL_WIS: mob->real_abils.wis = value; break;
    case ABIL_CHA: mob->real_abils.cha = value; break;
    default: return;
  }

  if (apply_affects)
    affect_total(mob);
  else
    mob->aff_abils = mob->real_abils;
}

static void mset_build_ldesc(char *out, size_t outsz, const char *input)
{
  size_t len;

  if (!input || !*input) {
    *out = '\0';
    return;
  }

  strlcpy(out, input, outsz);
  len = strlen(out);
  if (len < 2 || out[len - 2] != '\r' || out[len - 1] != '\n')
    strlcat(out, "\r\n", outsz);
}

static struct char_data *mset_get_target_mob(struct char_data *ch, const char *name)
{
  struct char_data *mob;

  if (!ch || !name || !*name || IN_ROOM(ch) == NOWHERE)
    return NULL;

  mob = get_char_vis(ch, (char *)name, NULL, FIND_CHAR_ROOM);
  if (!mob || !IS_NPC(mob))
    return NULL;

  return mob;
}

static void mset_show_mob(struct char_data *ch, struct char_data *mob)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  mob_rnum rnum = GET_MOB_RNUM(mob);
  int i;

  send_to_char(ch, "NPC [%d]: %s\r\n", GET_MOB_VNUM(mob),
               GET_NAME(mob) ? GET_NAME(mob) : "<None>");
  send_to_char(ch, "Keywords: %s\r\n", GET_KEYWORDS(mob) ? GET_KEYWORDS(mob) : "<None>");
  send_to_char(ch, "Short desc: %s\r\n", GET_SDESC(mob) ? GET_SDESC(mob) : "<None>");
  send_to_char(ch, "Long desc: %s\r\n", GET_LDESC(mob) ? GET_LDESC(mob) : "<None>");
  send_to_char(ch, "Main desc:\r\n%s", GET_DDESC(mob) ? GET_DDESC(mob) : "  <None>\r\n");
  send_to_char(ch, "Background:\r\n%s", GET_BACKGROUND(mob) ? GET_BACKGROUND(mob) : "  <None>\r\n");

  if (GET_ATTACK(mob) >= 0 && GET_ATTACK(mob) < NUM_ATTACK_TYPES)
    send_to_char(ch, "Attack: %s\r\n", attack_hit_text[(int)GET_ATTACK(mob)].singular);
  else
    send_to_char(ch, "Attack: <None>\r\n");

  if (GET_SEX(mob) >= 0 && GET_SEX(mob) < NUM_GENDERS)
    send_to_char(ch, "Sex: %s\r\n", genders[(int)GET_SEX(mob)]);
  else
    send_to_char(ch, "Sex: <None>\r\n");

  send_to_char(ch, "Species: %s\r\n", HAS_VALID_SPECIES(mob) ? species_types[GET_SPECIES(mob)] : "Unassigned");
  send_to_char(ch, "Class: %s\r\n", HAS_VALID_CLASS(mob) ? pc_class_types[GET_CLASS(mob)] : "Unassigned");

  send_to_char(ch, "Stats: Str %d Dex %d Con %d Int %d Wis %d Cha %d\r\n",
               GET_STR(mob), GET_DEX(mob), GET_CON(mob),
               GET_INT(mob), GET_WIS(mob), GET_CHA(mob));

  send_to_char(ch, "Saves: Str %d Dex %d Con %d Int %d Wis %d Cha %d\r\n",
               GET_SAVE(mob, ABIL_STR), GET_SAVE(mob, ABIL_DEX), GET_SAVE(mob, ABIL_CON),
               GET_SAVE(mob, ABIL_INT), GET_SAVE(mob, ABIL_WIS), GET_SAVE(mob, ABIL_CHA));

  sprintbitarray(MOB_FLAGS(mob), action_bits, AF_ARRAY_MAX, buf);
  send_to_char(ch, "Flags: %s\r\n", buf);
  sprintbitarray(AFF_FLAGS(mob), affected_bits, AF_ARRAY_MAX, buf2);
  send_to_char(ch, "Affects: %s\r\n", buf2);

  send_to_char(ch, "Skills:\r\n");
  for (i = 0; i < MAX_SKILLS; i++) {
    if (mob->mob_specials.skills[i] > 0 && i > 0 && i <= TOP_SPELL_DEFINE)
      send_to_char(ch, "  %s %d\r\n", spell_info[i].name, mob->mob_specials.skills[i]);
  }

  send_to_char(ch, "Skinning:\r\n");
  if (rnum != NOBODY && mob_index[rnum].skin_yields) {
    struct skin_yield_entry *e;
    for (e = mob_index[rnum].skin_yields; e; e = e->next) {
      obj_rnum ornum = real_object(e->obj_vnum);
      const char *sdesc = (ornum != NOTHING) ? obj_proto[ornum].short_description : "Unknown object";
      send_to_char(ch, "  [%d] DC %d - %s\r\n", e->obj_vnum, e->dc, sdesc);
    }
  } else {
    send_to_char(ch, "  None.\r\n");
  }

  send_to_char(ch, "Extra Descs:\r\n");
  i = 0;
  for (struct extra_descr_data *desc = mob->mob_specials.ex_description; desc; desc = desc->next) {
    send_to_char(ch, "  %s\r\n", desc->keyword ? desc->keyword : "<None>");
    i++;
  }
  if (!i)
    send_to_char(ch, "  None.\r\n");
}

static void mset_desc_edit(struct char_data *ch, char **field, const char *label)
{
  char *oldtext = NULL;

  send_editor_help(ch->desc);
  write_to_output(ch->desc, "Enter %s:\r\n\r\n", label);

  if (*field) {
    write_to_output(ch->desc, "%s", *field);
    oldtext = strdup(*field);
  }

  string_write(ch->desc, field, MAX_MOB_DESC, 0, oldtext);
}

static void mset_validate_mob(struct char_data *ch, struct char_data *mob)
{
  int errors = 0;

  if (!GET_NAME(mob) || !*GET_NAME(mob)) {
    send_to_char(ch, "Error: name is not set.\r\n");
    errors++;
  }

  if (!GET_KEYWORDS(mob) || !*GET_KEYWORDS(mob)) {
    send_to_char(ch, "Error: keywords are not set.\r\n");
    errors++;
  }

  if (!GET_SDESC(mob) || !*GET_SDESC(mob)) {
    send_to_char(ch, "Error: short description is not set.\r\n");
    errors++;
  }

  if (!GET_LDESC(mob) || !*GET_LDESC(mob)) {
    send_to_char(ch, "Error: long description is not set.\r\n");
    errors++;
  }

  if (!GET_DDESC(mob) || !*GET_DDESC(mob)) {
    send_to_char(ch, "Error: main description is not set.\r\n");
    errors++;
  }

  if (!GET_BACKGROUND(mob) || !*GET_BACKGROUND(mob)) {
    send_to_char(ch, "Error: background is not set.\r\n");
    errors++;
  }

  if (GET_SEX(mob) < 0 || GET_SEX(mob) >= NUM_GENDERS) {
    send_to_char(ch, "Error: sex is not set.\r\n");
    errors++;
  }

  if (!HAS_VALID_SPECIES(mob)) {
    send_to_char(ch, "Error: species is not set.\r\n");
    errors++;
  }

  if (!HAS_VALID_CLASS(mob)) {
    send_to_char(ch, "Error: class is not set.\r\n");
    errors++;
  }

  for (struct extra_descr_data *desc = mob->mob_specials.ex_description; desc; desc = desc->next) {
    if (!desc->keyword || !*desc->keyword) {
      send_to_char(ch, "Error: extra description is missing a keyword.\r\n");
      errors++;
    }
    if (!desc->description || !*desc->description) {
      send_to_char(ch, "Error: extra description for %s is missing text.\r\n",
        desc->keyword ? desc->keyword : "<None>");
      errors++;
    }
  }

  if (GET_LEVEL(mob) != 1) {
    send_to_char(ch, "Error: level must be 1.\r\n");
    errors++;
  }

  if (GET_DEFAULT_POS(mob) != POS_STANDING) {
    send_to_char(ch, "Error: default position must be standing.\r\n");
    errors++;
  }

  if (GET_EXP(mob) != 0) {
    send_to_char(ch, "Error: EXP must be 0.\r\n");
    errors++;
  }

  if (!errors)
    send_to_char(ch, "NPC validates cleanly.\r\n");
  else
    send_to_char(ch, "Validation failed: %d issue%s.\r\n", errors, errors == 1 ? "" : "s");
}

ACMD(do_mset)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  struct char_data *mob;
  mob_rnum rnum;
  mob_vnum vnum;
  trig_data *trig;
  int tn, rn;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "mset is only usable by connected players.\r\n");
    return;
  }

  argument = one_argument(argument, arg1);
  if (!*arg1) {
    mset_show_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "show")) {
    argument = one_argument(argument, arg2);
    if (!*arg2) {
      mset_show_usage(ch);
      return;
    }

    mob = mset_get_target_mob(ch, arg2);
    if (!mob) {
      send_to_char(ch, "Target an NPC in this room: mset show <npc>\r\n");
      return;
    }

    rnum = GET_MOB_RNUM(mob);
    vnum = GET_MOB_VNUM(mob);
    if (vnum == NOBODY) {
      send_to_char(ch, "That NPC has no valid vnum.\r\n");
      return;
    }

    if (!can_edit_zone(ch, real_zone_by_thing(vnum))) {
      send_to_char(ch, "You do not have permission to modify that zone.\r\n");
      return;
    }

    mset_show_mob(ch, mob);
    return;
  }

  if (is_abbrev(arg1, "add")) {
    argument = one_argument(argument, arg2);
    if (!*arg2) {
      mset_show_usage(ch);
      return;
    }

    argument = one_argument(argument, arg3);
    if (!*arg3) {
      mset_show_usage(ch);
      return;
    }

    mob = mset_get_target_mob(ch, arg3);
    if (!mob) {
      send_to_char(ch, "Target an NPC in this room: mset add <field> <npc> ...\r\n");
      return;
    }

    rnum = GET_MOB_RNUM(mob);
    vnum = GET_MOB_VNUM(mob);
    if (vnum == NOBODY) {
      send_to_char(ch, "That NPC has no valid vnum.\r\n");
      return;
    }

    if (!can_edit_zone(ch, real_zone_by_thing(vnum))) {
      send_to_char(ch, "You do not have permission to modify that zone.\r\n");
      return;
    }

    if (is_abbrev(arg2, "name")) {
      skip_spaces(&argument);
      if (!*argument) {
        mset_show_add_name_usage(ch);
        return;
      }

      genolc_checkstring(ch->desc, argument);
      if (rnum != NOBODY) {
        if (mob_proto[rnum].player.name)
          free(mob_proto[rnum].player.name);
        mob_proto[rnum].player.name = str_udup(argument);
        mset_update_proto_strings(rnum);
        mset_mark_mob_modified(vnum);
      } else {
        mset_replace_string(mob, &GET_NAME(mob), argument, NULL);
      }

      send_to_char(ch, "Name set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "script")) {
      zone_rnum znum;

      skip_spaces(&argument);
      if (!*argument) {
        mset_show_usage(ch);
        return;
      }

      znum = real_zone_by_thing(vnum);
      if (znum == NOWHERE) {
        send_to_char(ch, "That NPC has no valid zone.\r\n");
        return;
      }

      tn = script_resolve_trigger_vnum(ch, argument, znum, MOB_TRIGGER);
      if (tn == NOTHING)
        return;
      rn = real_trigger(tn);
      if (rn == NOTHING || trig_index[rn] == NULL || trig_index[rn]->proto == NULL) {
        send_to_char(ch, "That trigger does not exist.\r\n");
        return;
      }
      trig = trig_index[rn]->proto;

      if (!proto_trigger_add(&mob_proto[rnum].proto_script, tn)) {
        send_to_char(ch, "That trigger is already attached.\r\n");
        return;
      }

      for (struct char_data *mob_it = character_list; mob_it; mob_it = mob_it->next) {
        if (GET_MOB_RNUM(mob_it) != rnum)
          continue;
        if (SCRIPT(mob_it))
          extract_script(mob_it, MOB_TRIGGER);
        free_proto_script(mob_it, MOB_TRIGGER);
        copy_proto_script(&mob_proto[rnum], mob_it, MOB_TRIGGER);
        assign_triggers(mob_it, MOB_TRIGGER);
      }

      if (znum == NOWHERE || !save_mobiles(znum)) {
        send_to_char(ch, "Failed to write mobile data to disk.\r\n");
        return;
      }

      send_to_char(ch, "Trigger %d (%s) attached to %s [%d].\r\n",
                   tn, GET_TRIG_NAME(trig), GET_SHORT(mob), GET_MOB_VNUM(mob));
      return;
    }

    if (is_abbrev(arg2, "keywords")) {
      char kwbuf[MAX_STRING_LENGTH];
      char word[MAX_INPUT_LENGTH];
      const char *current;
      bool changed = FALSE;

      if (!*argument) {
        mset_show_add_keywords_usage(ch);
        return;
      }

      current = GET_KEYWORDS(mob);
      kwbuf[0] = '\0';
      if (current && *current)
        strlcpy(kwbuf, current, sizeof(kwbuf));

      while (*argument) {
        argument = one_argument(argument, word);
        if (!*word)
          break;
        if (!isname(word, kwbuf)) {
          if (*kwbuf)
            strlcat(kwbuf, " ", sizeof(kwbuf));
          strlcat(kwbuf, word, sizeof(kwbuf));
          changed = TRUE;
        }
      }

      if (!changed) {
        send_to_char(ch, "Keywords updated.\r\n");
        return;
      }

  if (rnum != NOBODY) {
        mset_update_proto_keywords(rnum, kwbuf);
        mset_mark_mob_modified(vnum);
      } else {
        mset_replace_string(mob, &GET_KEYWORDS(mob), kwbuf, NULL);
      }

      send_to_char(ch, "Keywords updated.\r\n");
      return;
    }

    if (is_abbrev(arg2, "sdesc")) {
      skip_spaces(&argument);
      if (!*argument) {
        mset_show_add_sdesc_usage(ch);
        return;
      }

      genolc_checkstring(ch->desc, argument);
      if (rnum != NOBODY) {
        if (mob_proto[rnum].player.short_descr)
          free(mob_proto[rnum].player.short_descr);
        mob_proto[rnum].player.short_descr = str_udup(argument);
        mset_update_proto_strings(rnum);
        mset_mark_mob_modified(vnum);
      } else {
        mset_replace_string(mob, &GET_SDESC(mob), argument, NULL);
      }

      send_to_char(ch, "Short description set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "ldesc")) {
      skip_spaces(&argument);
      if (!*argument) {
        mset_show_add_ldesc_usage(ch);
        return;
      }

      genolc_checkstring(ch->desc, argument);
      mset_build_ldesc(buf, sizeof(buf), argument);
      if (rnum != NOBODY) {
        if (mob_proto[rnum].player.long_descr)
          free(mob_proto[rnum].player.long_descr);
        mob_proto[rnum].player.long_descr = str_udup(buf);
        mset_update_proto_strings(rnum);
        mset_mark_mob_modified(vnum);
      } else {
        mset_replace_string(mob, &GET_LDESC(mob), buf, NULL);
      }

      send_to_char(ch, "Long description set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "desc")) {
      if (*argument) {
        mset_show_add_desc_usage(ch);
        return;
      }

      if (rnum != NOBODY) {
        mset_desc_edit(ch, &mob_proto[rnum].player.description, "mob description");
        mset_update_proto_strings(rnum);
        mset_mark_mob_modified(vnum);
      } else {
        mset_desc_edit(ch, &mob->player.description, "mob description");
      }

      return;
    }

    if (is_abbrev(arg2, "background")) {
      if (*argument) {
        mset_show_add_background_usage(ch);
        return;
      }

      if (rnum != NOBODY) {
        mset_desc_edit(ch, &mob_proto[rnum].player.background, "mob background");
        mset_update_proto_strings(rnum);
        mset_mark_mob_modified(vnum);
      } else {
        mset_desc_edit(ch, &mob->player.background, "mob background");
      }

      return;
    }

    if (is_abbrev(arg2, "edesc")) {
      struct extra_descr_data *desc;
      struct extra_descr_data *old;
      char *keyword;
      char *edesc;

      argument = one_argument(argument, arg1);
      skip_spaces(&argument);
      keyword = arg1;
      edesc = argument;

      if (!*keyword || !*edesc) {
        mset_show_add_edesc_usage(ch);
        return;
      }

      genolc_checkstring(ch->desc, edesc);
      genolc_checkstring(ch->desc, keyword);

      if (rnum != NOBODY) {
        old = mob_proto[rnum].mob_specials.ex_description;
        CREATE(desc, struct extra_descr_data, 1);
        desc->keyword = str_udup(keyword);
        desc->description = str_udup(edesc);
        desc->next = mob_proto[rnum].mob_specials.ex_description;
        mob_proto[rnum].mob_specials.ex_description = desc;
        mset_update_proto_edesc(rnum, old);
        mset_mark_mob_modified(vnum);
      } else {
        CREATE(desc, struct extra_descr_data, 1);
        desc->keyword = str_udup(keyword);
        desc->description = str_udup(edesc);
        desc->next = mob->mob_specials.ex_description;
        mob->mob_specials.ex_description = desc;
      }

      send_to_char(ch, "Extra description added.\r\n");
      return;
    }

    if (is_abbrev(arg2, "attack")) {
      int atype;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        mset_show_add_attack_usage(ch);
        return;
      }

      atype = oset_find_attack_type(arg3);
      if (atype < 0 || atype >= NUM_ATTACK_TYPES) {
        send_to_char(ch, "Invalid attack type.\r\n");
        return;
      }

      mob->mob_specials.attack_type = (byte)atype;
      if (rnum != NOBODY) {
        mob_proto[rnum].mob_specials.attack_type = (byte)atype;
        mset_mark_mob_modified(vnum);
      }

      send_to_char(ch, "Attack type set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "sex")) {
      int sex;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        mset_show_add_sex_usage(ch);
        return;
      }

      sex = mset_find_sex(arg3);
      if (sex < 0) {
        send_to_char(ch, "Invalid sex.\r\n");
        return;
      }

      GET_SEX(mob) = sex;
      if (rnum != NOBODY) {
        GET_SEX(&mob_proto[rnum]) = sex;
        mset_mark_mob_modified(vnum);
      }

      send_to_char(ch, "Sex set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "species")) {
      int species;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        mset_show_add_species_usage(ch);
        return;
      }

      species = parse_species(arg3);
      if (species == SPECIES_UNDEFINED) {
        send_to_char(ch, "Invalid species.\r\n");
        return;
      }

      GET_SPECIES(mob) = species;
      if (rnum != NOBODY) {
        GET_SPECIES(&mob_proto[rnum]) = species;
        mset_mark_mob_modified(vnum);
      }

      send_to_char(ch, "Species set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "class")) {
      int cls;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        mset_show_add_class_usage(ch);
        return;
      }

      cls = get_class_by_name(arg3);
      if (cls < 0 || cls >= NUM_CLASSES) {
        send_to_char(ch, "Invalid class.\r\n");
        return;
      }

      GET_CLASS(mob) = cls;
      if (rnum != NOBODY) {
        GET_CLASS(&mob_proto[rnum]) = cls;
        mset_mark_mob_modified(vnum);
      }

      send_to_char(ch, "Class set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "stat")) {
      int stat;
      int value;

      argument = one_argument(argument, arg1);
      if (!*arg1) {
        mset_show_add_stat_usage(ch);
        return;
      }

      stat = mset_find_stat(arg1);
      if (stat < 0) {
        mset_show_add_stat_usage(ch);
        return;
      }

      argument = one_argument(argument, arg1);
      if (!*arg1 || !is_number(arg1)) {
        mset_show_add_stat_usage(ch);
        return;
      }

      value = atoi(arg1);
      mset_set_stat_value(mob, stat, value, TRUE);
      if (rnum != NOBODY) {
        mset_set_stat_value(&mob_proto[rnum], stat, value, FALSE);
        mset_mark_mob_modified(vnum);
      }

      send_to_char(ch, "Stat set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "save")) {
      int stat;
      int value;

      argument = one_argument(argument, arg1);
      if (!*arg1) {
        mset_show_add_save_usage(ch);
        return;
      }

      stat = mset_find_stat(arg1);
      if (stat < 0) {
        mset_show_add_save_usage(ch);
        return;
      }

      argument = one_argument(argument, arg1);
      if (!*arg1 || !is_number(arg1)) {
        mset_show_add_save_usage(ch);
        return;
      }

      value = atoi(arg1);
      GET_SAVE(mob, stat) = value;
      if (rnum != NOBODY) {
        GET_SAVE(&mob_proto[rnum], stat) = value;
        mset_mark_mob_modified(vnum);
      }

      send_to_char(ch, "Save set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "skill")) {
      char skillbuf[MAX_INPUT_LENGTH];
      char *skillname;
      char *level_arg;
      char *end;
      int snum;
      int val;

      if (!*argument) {
        mset_show_add_skill_usage(ch);
        return;
      }

      strlcpy(skillbuf, argument, sizeof(skillbuf));
      skillname = skillbuf;
      skip_spaces(&skillname);
      if (!*skillname) {
        mset_show_add_skill_usage(ch);
        return;
      }

      end = skillname + strlen(skillname) - 1;
      while (end > skillname && isspace(*end)) {
        *end = '\0';
        end--;
      }

      level_arg = strrchr(skillname, ' ');
      if (!level_arg) {
        mset_show_add_skill_usage(ch);
        return;
      }

      *level_arg = '\0';
      level_arg++;
      if (!*skillname || !*level_arg || !is_number(level_arg)) {
        mset_show_add_skill_usage(ch);
        return;
      }

      snum = find_skill_num(skillname);
      if (snum <= 0 || snum >= MAX_SKILLS) {
        send_to_char(ch, "Invalid skill.\r\n");
        return;
      }

      val = atoi(level_arg);
      val = MAX(0, MIN(100, val));

      mob->mob_specials.skills[snum] = (byte)val;
      if (rnum != NOBODY) {
        mob_proto[rnum].mob_specials.skills[snum] = (byte)val;
        mset_mark_mob_modified(vnum);
      }

      send_to_char(ch, "Skill set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "flags")) {
      bool any = FALSE;

      if (!*argument) {
        mset_show_add_flags_usage(ch);
        return;
      }

      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = mset_find_mob_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown mob flag: %s\r\n", arg3);
          continue;
        }
        if (mset_illegal_mob_flag(flag)) {
          send_to_char(ch, "Flag %s cannot be set.\r\n", action_bits[flag]);
          continue;
        }

        SET_BIT_AR(MOB_FLAGS(mob), flag);
        if (rnum != NOBODY)
          SET_BIT_AR(MOB_FLAGS(&mob_proto[rnum]), flag);
        any = TRUE;
      }

      if (any) {
        if (rnum != NOBODY)
          mset_mark_mob_modified(vnum);
        send_to_char(ch, "Mob flags updated.\r\n");
      }
      return;
    }

    if (is_abbrev(arg2, "affect")) {
      bool any = FALSE;

      if (!*argument) {
        mset_show_add_affect_usage(ch);
        return;
      }

      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = mset_find_affect_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown affect: %s\r\n", arg3);
          continue;
        }

        SET_BIT_AR(AFF_FLAGS(mob), flag);
        if (rnum != NOBODY)
          SET_BIT_AR(AFF_FLAGS(&mob_proto[rnum]), flag);
        any = TRUE;
      }

      if (any) {
        if (rnum != NOBODY)
          mset_mark_mob_modified(vnum);
        send_to_char(ch, "Affects updated.\r\n");
      }
      return;
    }

    if (is_abbrev(arg2, "skinning")) {
      int skin_vnum;
      int dc;
      struct skin_yield_entry *entry;

      argument = one_argument(argument, arg1);
      if (!*arg1) {
        mset_show_add_skinning_usage(ch);
        return;
      }

      skin_vnum = atoi(arg1);
      argument = one_argument(argument, arg1);
      if (!*arg1 || !is_number(arg1)) {
        mset_show_add_skinning_usage(ch);
        return;
      }

      dc = atoi(arg1);
      if (skin_vnum <= 0 || dc <= 0) {
        send_to_char(ch, "Both vnum and DC must be positive.\r\n");
        return;
      }

      if (real_object(skin_vnum) == NOTHING) {
        send_to_char(ch, "That object vnum does not exist.\r\n");
        return;
      }

      if (rnum == NOBODY) {
        send_to_char(ch, "That NPC has no prototype to attach skinning yields.\r\n");
        return;
      }

      CREATE(entry, struct skin_yield_entry, 1);
      entry->mob_vnum = vnum;
      entry->obj_vnum = skin_vnum;
      entry->dc = dc;
      entry->next = mob_index[rnum].skin_yields;
      mob_index[rnum].skin_yields = entry;
      mset_mark_mob_modified(vnum);
      send_to_char(ch, "Skinning entry added.\r\n");
      return;
    }

    mset_show_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "del")) {
    argument = one_argument(argument, arg2);
    if (!*arg2) {
      mset_show_del_usage(ch);
      return;
    }

    argument = one_argument(argument, arg3);
    if (!*arg3) {
      mset_show_del_usage(ch);
      return;
    }

    mob = mset_get_target_mob(ch, arg2);
    if (!mob) {
      send_to_char(ch, "Target an NPC in this room: mset del <npc> <field> ...\r\n");
      return;
    }

    rnum = GET_MOB_RNUM(mob);
    vnum = GET_MOB_VNUM(mob);
    if (vnum == NOBODY) {
      send_to_char(ch, "That NPC has no valid vnum.\r\n");
      return;
    }

    if (!can_edit_zone(ch, real_zone_by_thing(vnum))) {
      send_to_char(ch, "You do not have permission to modify that zone.\r\n");
      return;
    }

    if (is_abbrev(arg3, "script")) {
      zone_rnum znum;
      const char *tname = "unknown";
      char script_name[MAX_INPUT_LENGTH];

      if (!*argument) {
        mset_show_del_usage(ch);
        return;
      }

      if (is_number(argument)) {
        tn = atoi(argument);
      } else {
        if (!script_normalize_name(argument, script_name, sizeof(script_name))) {
          send_to_char(ch, "Script name is invalid.\r\n");
          return;
        }
        tn = script_find_attached_trigger_vnum(mob_proto[rnum].proto_script,
                                               script_name, MOB_TRIGGER);
        if (tn == NOTHING) {
          send_to_char(ch, "That script is not attached.\r\n");
          return;
        }
      }

      rn = real_trigger(tn);
      if (rn != NOTHING && trig_index[rn] && trig_index[rn]->proto)
        tname = GET_TRIG_NAME(trig_index[rn]->proto);

      if (!proto_trigger_remove(&mob_proto[rnum].proto_script, tn)) {
        send_to_char(ch, "That trigger is not attached.\r\n");
        return;
      }

      for (struct char_data *mob_it = character_list; mob_it; mob_it = mob_it->next) {
        if (GET_MOB_RNUM(mob_it) != rnum)
          continue;
        if (SCRIPT(mob_it))
          extract_script(mob_it, MOB_TRIGGER);
        free_proto_script(mob_it, MOB_TRIGGER);
        copy_proto_script(&mob_proto[rnum], mob_it, MOB_TRIGGER);
        assign_triggers(mob_it, MOB_TRIGGER);
      }

      znum = real_zone_by_thing(vnum);
      if (znum == NOWHERE || !save_mobiles(znum)) {
        send_to_char(ch, "Failed to write mobile data to disk.\r\n");
        return;
      }

      send_to_char(ch, "Trigger %d (%s) removed from %s [%d].\r\n",
                   tn, tname, GET_SHORT(mob), GET_MOB_VNUM(mob));
      return;
    }

    if (is_abbrev(arg3, "name")) {
      if (rnum != NOBODY) {
        if (mob_proto[rnum].player.name)
          free(mob_proto[rnum].player.name);
        mob_proto[rnum].player.name = strdup("");
        mset_update_proto_strings(rnum);
        mset_mark_mob_modified(vnum);
      } else {
        mset_replace_string(mob, &GET_NAME(mob), "", NULL);
      }
      send_to_char(ch, "Name cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "keywords")) {
      char work[MAX_STRING_LENGTH];
      char out[MAX_STRING_LENGTH];
      char word[MAX_INPUT_LENGTH];
      bool changed = FALSE;
      char *ptr;

      if (!*argument) {
        mset_show_add_keywords_usage(ch);
        return;
      }

      if (!GET_KEYWORDS(mob) || !*GET_KEYWORDS(mob)) {
        send_to_char(ch, "NPC has no keywords to remove.\r\n");
        return;
      }

      strlcpy(work, GET_KEYWORDS(mob), sizeof(work));
      out[0] = '\0';
      ptr = work;
      while (*ptr) {
        ptr = one_argument(ptr, word);
        if (!*word)
          break;
        if (isname(word, argument)) {
          changed = TRUE;
          continue;
        }
        if (*out)
          strlcat(out, " ", sizeof(out));
        strlcat(out, word, sizeof(out));
      }

      if (!changed) {
        send_to_char(ch, "No matching keywords found.\r\n");
        return;
      }

      if (rnum != NOBODY) {
        mset_update_proto_keywords(rnum, out);
        mset_mark_mob_modified(vnum);
      } else {
        mset_replace_string(mob, &GET_KEYWORDS(mob), out, NULL);
      }

      send_to_char(ch, "Keywords updated.\r\n");
      return;
    }

    if (is_abbrev(arg3, "sdesc")) {
      if (rnum != NOBODY) {
        if (mob_proto[rnum].player.short_descr)
          free(mob_proto[rnum].player.short_descr);
        mob_proto[rnum].player.short_descr = strdup("");
        mset_update_proto_strings(rnum);
        mset_mark_mob_modified(vnum);
      } else {
        mset_replace_string(mob, &GET_SDESC(mob), "", NULL);
      }
      send_to_char(ch, "Short description cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "ldesc")) {
      if (rnum != NOBODY) {
        if (mob_proto[rnum].player.long_descr)
          free(mob_proto[rnum].player.long_descr);
        mob_proto[rnum].player.long_descr = strdup("");
        mset_update_proto_strings(rnum);
        mset_mark_mob_modified(vnum);
      } else {
        mset_replace_string(mob, &GET_LDESC(mob), "", NULL);
      }
      send_to_char(ch, "Long description cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "desc")) {
      if (rnum != NOBODY) {
        if (mob_proto[rnum].player.description)
          free(mob_proto[rnum].player.description);
        mob_proto[rnum].player.description = strdup("");
        mset_update_proto_strings(rnum);
        mset_mark_mob_modified(vnum);
      } else {
        mset_replace_string(mob, &mob->player.description, "", NULL);
      }
      send_to_char(ch, "Main description cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "background")) {
      if (rnum != NOBODY) {
        if (mob_proto[rnum].player.background)
          free(mob_proto[rnum].player.background);
        mob_proto[rnum].player.background = strdup("");
        mset_update_proto_strings(rnum);
        mset_mark_mob_modified(vnum);
      } else {
        mset_replace_string(mob, &mob->player.background, "", NULL);
      }
      send_to_char(ch, "Background cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "edesc")) {
      struct extra_descr_data *desc;
      struct extra_descr_data *prev = NULL;
      struct extra_descr_data *old;

      argument = one_argument(argument, arg1);
      if (!*arg1) {
        mset_show_del_edesc_usage(ch);
        return;
      }

      if (rnum != NOBODY) {
        old = mob_proto[rnum].mob_specials.ex_description;
        desc = mob_proto[rnum].mob_specials.ex_description;
        while (desc) {
          if (desc->keyword && isname(arg1, desc->keyword))
            break;
          prev = desc;
          desc = desc->next;
        }

        if (!desc) {
          send_to_char(ch, "No extra description found for %s.\r\n", arg1);
          return;
        }

        if (prev)
          prev->next = desc->next;
        else
          mob_proto[rnum].mob_specials.ex_description = desc->next;

        if (desc->keyword)
          free(desc->keyword);
        if (desc->description)
          free(desc->description);
        free(desc);
        mset_update_proto_edesc(rnum, old);
        mset_mark_mob_modified(vnum);
      } else {
        desc = mob->mob_specials.ex_description;
        while (desc) {
          if (desc->keyword && isname(arg1, desc->keyword))
            break;
          prev = desc;
          desc = desc->next;
        }

        if (!desc) {
          send_to_char(ch, "No extra description found for %s.\r\n", arg1);
          return;
        }

        if (prev)
          prev->next = desc->next;
        else
          mob->mob_specials.ex_description = desc->next;

        if (desc->keyword)
          free(desc->keyword);
        if (desc->description)
          free(desc->description);
        free(desc);
      }

      send_to_char(ch, "Extra description removed.\r\n");
      return;
    }

    if (is_abbrev(arg3, "attack")) {
      mob->mob_specials.attack_type = 0;
      if (rnum != NOBODY) {
        mob_proto[rnum].mob_specials.attack_type = 0;
        mset_mark_mob_modified(vnum);
      }
      send_to_char(ch, "Attack type cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "sex")) {
      GET_SEX(mob) = SEX_NEUTRAL;
      if (rnum != NOBODY) {
        GET_SEX(&mob_proto[rnum]) = SEX_NEUTRAL;
        mset_mark_mob_modified(vnum);
      }
      send_to_char(ch, "Sex cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "species")) {
      GET_SPECIES(mob) = SPECIES_UNDEFINED;
      if (rnum != NOBODY) {
        GET_SPECIES(&mob_proto[rnum]) = SPECIES_UNDEFINED;
        mset_mark_mob_modified(vnum);
      }
      send_to_char(ch, "Species cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "class")) {
      GET_CLASS(mob) = CLASS_UNDEFINED;
      if (rnum != NOBODY) {
        GET_CLASS(&mob_proto[rnum]) = CLASS_UNDEFINED;
        mset_mark_mob_modified(vnum);
      }
      send_to_char(ch, "Class cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "stat")) {
      int stat;

      if (!*argument) {
        mset_show_add_stat_usage(ch);
        return;
      }

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        mset_show_add_stat_usage(ch);
        return;
      }

      stat = mset_find_stat(arg3);
      if (stat < 0) {
        mset_show_add_stat_usage(ch);
        return;
      }

      mset_set_stat_value(mob, stat, 10, TRUE);
      if (rnum != NOBODY) {
        mset_set_stat_value(&mob_proto[rnum], stat, 10, FALSE);
        mset_mark_mob_modified(vnum);
      }

      send_to_char(ch, "Stat cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "save")) {
      int stat;

      if (!*argument) {
        mset_show_add_save_usage(ch);
        return;
      }

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        mset_show_add_save_usage(ch);
        return;
      }

      stat = mset_find_stat(arg3);
      if (stat < 0) {
        mset_show_add_save_usage(ch);
        return;
      }

      GET_SAVE(mob, stat) = 10;
      if (rnum != NOBODY) {
        GET_SAVE(&mob_proto[rnum], stat) = 10;
        mset_mark_mob_modified(vnum);
      }

      send_to_char(ch, "Save cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "skill")) {
      char skillbuf[MAX_INPUT_LENGTH];
      char *skillname;
      int snum;

      if (!*argument) {
        mset_show_add_skill_usage(ch);
        return;
      }

      strlcpy(skillbuf, argument, sizeof(skillbuf));
      skillname = skillbuf;
      skip_spaces(&skillname);
      if (!*skillname) {
        mset_show_add_skill_usage(ch);
        return;
      }

      snum = find_skill_num(skillname);
      if (snum <= 0 || snum >= MAX_SKILLS) {
        send_to_char(ch, "Invalid skill.\r\n");
        return;
      }

      mob->mob_specials.skills[snum] = 0;
      if (rnum != NOBODY) {
        mob_proto[rnum].mob_specials.skills[snum] = 0;
        mset_mark_mob_modified(vnum);
      }

      send_to_char(ch, "Skill cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "flags")) {
      bool any = FALSE;

      if (!*argument) {
        mset_show_del_flags_usage(ch);
        return;
      }

      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = mset_find_mob_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown mob flag: %s\r\n", arg3);
          continue;
        }
        if (mset_illegal_mob_flag(flag)) {
          send_to_char(ch, "Flag %s cannot be removed.\r\n", action_bits[flag]);
          continue;
        }

        REMOVE_BIT_AR(MOB_FLAGS(mob), flag);
        if (rnum != NOBODY)
          REMOVE_BIT_AR(MOB_FLAGS(&mob_proto[rnum]), flag);
        any = TRUE;
      }

      if (any) {
        if (rnum != NOBODY)
          mset_mark_mob_modified(vnum);
        send_to_char(ch, "Mob flags updated.\r\n");
      }
      return;
    }

    if (is_abbrev(arg3, "affect")) {
      bool any = FALSE;

      if (!*argument) {
        mset_show_del_affect_usage(ch);
        return;
      }

      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = mset_find_affect_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown affect: %s\r\n", arg3);
          continue;
        }

        REMOVE_BIT_AR(AFF_FLAGS(mob), flag);
        if (rnum != NOBODY)
          REMOVE_BIT_AR(AFF_FLAGS(&mob_proto[rnum]), flag);
        any = TRUE;
      }

      if (any) {
        if (rnum != NOBODY)
          mset_mark_mob_modified(vnum);
        send_to_char(ch, "Affects updated.\r\n");
      }
      return;
    }

    if (is_abbrev(arg3, "skinning")) {
      int skin_vnum;
      struct skin_yield_entry *entry;
      struct skin_yield_entry *prev = NULL;

      if (!*argument) {
        mset_show_add_skinning_usage(ch);
        return;
      }

      argument = one_argument(argument, arg3);
      if (!*arg3 || !is_number(arg3)) {
        mset_show_add_skinning_usage(ch);
        return;
      }

      if (rnum == NOBODY) {
        send_to_char(ch, "That NPC has no prototype to edit skinning yields.\r\n");
        return;
      }

      skin_vnum = atoi(arg3);
      entry = mob_index[rnum].skin_yields;
      while (entry) {
        if (entry->obj_vnum == skin_vnum)
          break;
        prev = entry;
        entry = entry->next;
      }

      if (!entry) {
        send_to_char(ch, "No skinning entry found for vnum %d.\r\n", skin_vnum);
        return;
      }

      if (prev)
        prev->next = entry->next;
      else
        mob_index[rnum].skin_yields = entry->next;
      free(entry);
      mset_mark_mob_modified(vnum);
      send_to_char(ch, "Skinning entry removed.\r\n");
      return;
    }

    mset_show_del_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "clear")) {
    argument = one_argument(argument, arg2);
    if (!*arg2 || *argument) {
      mset_show_usage(ch);
      return;
    }

    mob = mset_get_target_mob(ch, arg2);
    if (!mob) {
      send_to_char(ch, "Target an NPC in this room: mset clear <npc>\r\n");
      return;
    }

    rnum = GET_MOB_RNUM(mob);
    vnum = GET_MOB_VNUM(mob);
    if (vnum == NOBODY) {
      send_to_char(ch, "That NPC has no valid vnum.\r\n");
      return;
    }

    if (!can_edit_zone(ch, real_zone_by_thing(vnum))) {
      send_to_char(ch, "You do not have permission to modify that zone.\r\n");
      return;
    }

    if (rnum != NOBODY) {
      struct extra_descr_data *old_edesc = mob_proto[rnum].mob_specials.ex_description;

      if (mob_proto[rnum].player.name)
        free(mob_proto[rnum].player.name);
      if (mob_proto[rnum].player.short_descr)
        free(mob_proto[rnum].player.short_descr);
      if (mob_proto[rnum].player.long_descr)
        free(mob_proto[rnum].player.long_descr);
      if (mob_proto[rnum].player.description)
        free(mob_proto[rnum].player.description);
      if (mob_proto[rnum].player.background)
        free(mob_proto[rnum].player.background);
      if (mob_proto[rnum].mob_specials.ex_description)
        free_ex_descriptions(mob_proto[rnum].mob_specials.ex_description);

      mob_proto[rnum].player.name = strdup("An unfinished NPC");
      mob_proto[rnum].player.short_descr = strdup("the unfinished npc");
      mob_proto[rnum].player.long_descr = strdup("An unfinished npc stands here.\r\n");
      mob_proto[rnum].player.description = strdup("It looks unfinished.\r\n");
      mob_proto[rnum].player.background = strdup("No background has been recorded.\r\n");
      mob_proto[rnum].mob_specials.ex_description = NULL;
      mset_update_proto_edesc(rnum, old_edesc);

      mob_proto[rnum].mob_specials.attack_type = 0;
      GET_SEX(&mob_proto[rnum]) = SEX_NEUTRAL;
      GET_CLASS(&mob_proto[rnum]) = CLASS_UNDEFINED;
      GET_SPECIES(&mob_proto[rnum]) = SPECIES_UNDEFINED;
      GET_LEVEL(&mob_proto[rnum]) = 1;
      GET_DEFAULT_POS(&mob_proto[rnum]) = POS_STANDING;
      GET_POS(&mob_proto[rnum]) = POS_STANDING;
      GET_EXP(&mob_proto[rnum]) = 0;

      mset_set_stat_value(&mob_proto[rnum], ABIL_STR, 10, FALSE);
      mset_set_stat_value(&mob_proto[rnum], ABIL_DEX, 10, FALSE);
      mset_set_stat_value(&mob_proto[rnum], ABIL_CON, 10, FALSE);
      mset_set_stat_value(&mob_proto[rnum], ABIL_INT, 10, FALSE);
      mset_set_stat_value(&mob_proto[rnum], ABIL_WIS, 10, FALSE);
      mset_set_stat_value(&mob_proto[rnum], ABIL_CHA, 10, FALSE);

      for (int i = 0; i < NUM_ABILITIES; i++)
        GET_SAVE(&mob_proto[rnum], i) = 10;
      for (int i = 0; i < MAX_SKILLS; i++)
        mob_proto[rnum].mob_specials.skills[i] = 0;

      memset(MOB_FLAGS(&mob_proto[rnum]), 0, sizeof(mob_proto[rnum].char_specials.saved.act));
      SET_BIT_AR(MOB_FLAGS(&mob_proto[rnum]), MOB_ISNPC);
      memset(AFF_FLAGS(&mob_proto[rnum]), 0, sizeof(mob_proto[rnum].char_specials.saved.affected_by));

      free_skin_yields(mob_index[rnum].skin_yields);
      mob_index[rnum].skin_yields = NULL;

      mset_update_proto_strings(rnum);
      mset_update_proto_keywords(rnum, "unfinished npc");
      mset_mark_mob_modified(vnum);
    } else {
      mset_replace_string(mob, &GET_NAME(mob), "An unfinished NPC", NULL);
      mset_replace_string(mob, &GET_KEYWORDS(mob), "unfinished npc", NULL);
      mset_replace_string(mob, &GET_SDESC(mob), "the unfinished npc", NULL);
      mset_replace_string(mob, &GET_LDESC(mob), "An unfinished npc stands here.\r\n", NULL);
      mset_replace_string(mob, &mob->player.description, "It looks unfinished.\r\n", NULL);
      mset_replace_string(mob, &mob->player.background, "No background has been recorded.\r\n", NULL);
      if (mob->mob_specials.ex_description) {
        free_ex_descriptions(mob->mob_specials.ex_description);
        mob->mob_specials.ex_description = NULL;
      }

      mob->mob_specials.attack_type = 0;
      GET_SEX(mob) = SEX_NEUTRAL;
      GET_CLASS(mob) = CLASS_UNDEFINED;
      GET_SPECIES(mob) = SPECIES_UNDEFINED;
      GET_LEVEL(mob) = 1;
      GET_DEFAULT_POS(mob) = POS_STANDING;
      GET_POS(mob) = POS_STANDING;
      GET_EXP(mob) = 0;

      mset_set_stat_value(mob, ABIL_STR, 10, TRUE);
      mset_set_stat_value(mob, ABIL_DEX, 10, TRUE);
      mset_set_stat_value(mob, ABIL_CON, 10, TRUE);
      mset_set_stat_value(mob, ABIL_INT, 10, TRUE);
      mset_set_stat_value(mob, ABIL_WIS, 10, TRUE);
      mset_set_stat_value(mob, ABIL_CHA, 10, TRUE);

      for (int i = 0; i < NUM_ABILITIES; i++)
        GET_SAVE(mob, i) = 10;
      for (int i = 0; i < MAX_SKILLS; i++)
        mob->mob_specials.skills[i] = 0;

      memset(MOB_FLAGS(mob), 0, sizeof(mob->char_specials.saved.act));
      SET_BIT_AR(MOB_FLAGS(mob), MOB_ISNPC);
      memset(AFF_FLAGS(mob), 0, sizeof(mob->char_specials.saved.affected_by));
    }

    send_to_char(ch, "NPC cleared.\r\n");
    return;
  }

  if (is_abbrev(arg1, "validate")) {
    argument = one_argument(argument, arg2);
    if (!*arg2 || *argument) {
      mset_show_usage(ch);
      return;
    }

    mob = mset_get_target_mob(ch, arg2);
    if (!mob) {
      send_to_char(ch, "Target an NPC in this room: mset validate <npc>\r\n");
      return;
    }

    rnum = GET_MOB_RNUM(mob);
    vnum = GET_MOB_VNUM(mob);
    if (vnum == NOBODY) {
      send_to_char(ch, "That NPC has no valid vnum.\r\n");
      return;
    }

    if (!can_edit_zone(ch, real_zone_by_thing(vnum))) {
      send_to_char(ch, "You do not have permission to modify that zone.\r\n");
      return;
    }

    mset_validate_mob(ch, mob);
    return;
  }

  mset_show_usage(ch);
}

static struct obj_data *find_obj_vnum_nearby(struct char_data *ch, obj_vnum vnum)
{
  struct obj_data *obj;

  if (!ch || IN_ROOM(ch) == NOWHERE)
    return NULL;

  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj) && GET_OBJ_VNUM(obj) == vnum)
      return obj;

  for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj) && GET_OBJ_VNUM(obj) == vnum)
      return obj;

  return NULL;
}

ACMD(do_mcreate)
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char namebuf[MAX_NAME_LENGTH];
  char timestr[64];
  struct char_data *newmob;
  struct char_data *mob;
  mob_vnum vnum;
  zone_rnum znum;
  time_t ct;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "mcreate is only usable by connected players.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch,
      "Creates a new unfinished NPC which can be configured.\r\n"
      "\r\n"
      "Usage:\r\n"
      "  mcreate <vnum>\r\n"
      "\r\n"
      "Examples:\r\n"
      "  mcreate 1001\r\n");
    return;
  }

  if (!is_number(arg)) {
    send_to_char(ch,
      "Creates a new unfinished NPC which can be configured.\r\n"
      "\r\n"
      "Usage:\r\n"
      "  mcreate <vnum>\r\n"
      "\r\n"
      "Examples:\r\n"
      "  mcreate 1001\r\n");
    return;
  }

  vnum = atoi(arg);
  if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
    send_to_char(ch, "That mobile VNUM can't exist.\r\n");
    return;
  }

  znum = real_zone_by_thing(vnum);
  if (znum == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    return;
  }

  if (!can_edit_zone(ch, znum)) {
    send_cannot_edit(ch, zone_table[znum].number);
    return;
  }

  if (real_mobile(vnum) != NOBODY) {
    mob = read_mobile(vnum, VIRTUAL);
    if (mob == NULL) {
      send_to_char(ch, "mcreate: failed to instantiate NPC %d.\r\n", vnum);
      return;
    }
    char_to_room(mob, IN_ROOM(ch));
    {
      const char *keyword = GET_KEYWORDS(mob);
      if (!keyword || !*keyword)
        keyword = "npc";
      if (!strn_cmp(keyword, "unfinished ", 11))
        keyword += 11;
      act("You form $t from clay.", FALSE, ch, (void *)keyword, 0, TO_CHAR);
      act("$n forms $t from clay.", TRUE, ch, (void *)keyword, 0, TO_ROOM);
    }
    return;
  }

  CREATE(newmob, struct char_data, 1);
  init_mobile(newmob);

  GET_LEVEL(newmob) = 1;
  GET_DEFAULT_POS(newmob) = POS_STANDING;
  GET_POS(newmob) = POS_STANDING;
  GET_EXP(newmob) = 0;

  GET_NAME(newmob) = strdup("An unfinished NPC");
  GET_KEYWORDS(newmob) = strdup("unfinished npc");
  strlcpy(namebuf, GET_NAME(ch), sizeof(namebuf));
  snprintf(buf, sizeof(buf), "An unfinished NPC made by %.*s",
           (int)sizeof(namebuf) - 1, namebuf);
  GET_SDESC(newmob) = strdup(buf);
  snprintf(buf, sizeof(buf), "An unfinished NPC made by %.*s is here.\r\n",
           (int)sizeof(namebuf) - 1, namebuf);
  GET_LDESC(newmob) = strdup(buf);
  ct = time(0);
  strftime(timestr, sizeof(timestr), "%c", localtime(&ct));
  snprintf(buf, sizeof(buf),
    "An unfinished NPC made by %.*s on %.*s\r\n",
    (int)sizeof(namebuf) - 1, namebuf,
    (int)sizeof(timestr) - 1, timestr);
  GET_DDESC(newmob) = strdup(buf);
  GET_BACKGROUND(newmob) = strdup("No background has been recorded.\r\n");

  if (add_mobile(newmob, vnum) == NOBODY) {
    free_mobile_strings(newmob);
    free(newmob);
    send_to_char(ch, "mcreate: failed to add NPC %d.\r\n", vnum);
    return;
  }

  if (in_save_list(zone_table[znum].number, SL_MOB))
    remove_from_save_list(zone_table[znum].number, SL_MOB);

  free_mobile_strings(newmob);
  free(newmob);

  mob = read_mobile(vnum, VIRTUAL);
  if (mob == NULL) {
    send_to_char(ch, "mcreate: failed to instantiate NPC %d.\r\n", vnum);
    return;
  }

  char_to_room(mob, IN_ROOM(ch));
  {
    const char *keyword = GET_KEYWORDS(mob);
    if (!keyword || !*keyword)
      keyword = "npc";
    if (!strn_cmp(keyword, "unfinished ", 11))
      keyword += 11;
    act("You create an unfinished $t from clay.", FALSE, ch, (void *)keyword, 0, TO_CHAR);
    act("$n forms an unfinished $t from clay.", TRUE, ch, (void *)keyword, 0, TO_ROOM);
  }
}

ACMD(do_ocreate)
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char namebuf[MAX_NAME_LENGTH];
  char timestr[MAX_STRING_LENGTH];
  struct obj_data *newobj;
  struct obj_data *obj;
  obj_vnum vnum;
  zone_rnum znum;
  time_t ct;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "ocreate is only usable by connected players.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch,
      "Creates a new unfinished object which can be configured.\r\n"
      "\r\n"
      "Usage:\r\n"
      "  ocreate <vnum>\r\n"
      "\r\n"
      "Examples:\r\n"
      "  ocreate 1001\r\n");
    return;
  }

  if (!is_number(arg)) {
    send_to_char(ch,
      "Creates a new unfinished object which can be configured.\r\n"
      "\r\n"
      "Usage:\r\n"
      "  ocreate <vnum>\r\n"
      "\r\n"
      "Examples:\r\n"
      "  ocreate 1001\r\n");
    return;
  }

  vnum = atoi(arg);
  if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
    send_to_char(ch, "That object VNUM can't exist.\r\n");
    return;
  }

  znum = real_zone_by_thing(vnum);
  if (znum == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    return;
  }

  if (!can_edit_zone(ch, znum)) {
    send_cannot_edit(ch, zone_table[znum].number);
    return;
  }

  if (real_object(vnum) != NOTHING) {
    obj = read_object(vnum, VIRTUAL);
    if (obj == NULL) {
      send_to_char(ch, "ocreate: failed to instantiate object %d.\r\n", vnum);
      return;
    }
    obj_to_char(obj, ch);
    {
      const char *sdesc = obj->short_description;
      if (!sdesc || !*sdesc)
        sdesc = obj->name;
      if (!sdesc || !*sdesc)
        sdesc = "object";
      act("You form $t from clay.", FALSE, ch, (void *)sdesc, 0, TO_CHAR);
      act("$n forms $t from clay.", TRUE, ch, (void *)sdesc, 0, TO_ROOM);
    }
    return;
  }

  CREATE(newobj, struct obj_data, 1);
  clear_object(newobj);

  GET_OBJ_LEVEL(newobj) = 1;
  GET_OBJ_TIMER(newobj) = 0;
  GET_OBJ_COST_PER_DAY(newobj) = 0;

  newobj->name = strdup("unfinished object");
  strlcpy(namebuf, GET_NAME(ch), sizeof(namebuf));
  snprintf(buf, sizeof(buf), "unfinished object made by %s", namebuf);
  newobj->short_description = strdup(buf);
  ct = time(0);
  strftime(timestr, sizeof(timestr), "%c", localtime(&ct));
  snprintf(buf, sizeof(buf),
    "This is an unfinished object created by %s on ", namebuf);
  strlcat(buf, timestr, sizeof(buf));
  newobj->description = strdup(buf);
  SET_BIT_AR(GET_OBJ_WEAR(newobj), ITEM_WEAR_TAKE);

  if (add_object(newobj, vnum) == NOTHING) {
    free_object_strings(newobj);
    free(newobj);
    send_to_char(ch, "ocreate: failed to add object %d.\r\n", vnum);
    return;
  }

  if (in_save_list(zone_table[znum].number, SL_OBJ))
    remove_from_save_list(zone_table[znum].number, SL_OBJ);

  free_object_strings(newobj);
  free(newobj);

  obj = read_object(vnum, VIRTUAL);
  if (obj == NULL) {
    send_to_char(ch, "ocreate: failed to instantiate object %d.\r\n", vnum);
    return;
  }

  obj_to_char(obj, ch);
  {
    const char *sdesc = obj->short_description;
    if (!sdesc || !*sdesc)
      sdesc = obj->name;
    if (!sdesc || !*sdesc)
      sdesc = "object";
    if (!strn_cmp(sdesc, "an ", 3))
      sdesc += 3;
    else if (!strn_cmp(sdesc, "a ", 2))
      sdesc += 2;
    else if (!strn_cmp(sdesc, "the ", 4))
      sdesc += 4;
    if (!strn_cmp(sdesc, "unfinished ", 11))
      sdesc += 11;
    act("You create an unfinished $t from clay.", FALSE, ch, (void *)sdesc, 0, TO_CHAR);
    act("$n forms an unfinished $t from clay.", TRUE, ch, (void *)sdesc, 0, TO_ROOM);
  }
}

ACMD(do_qcreate)
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char namebuf[MAX_NAME_LENGTH];
  char timestr[64];
  struct aq_data *quest;
  qst_vnum vnum;
  zone_rnum znum;
  time_t ct;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "qcreate is only usable by connected players.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch,
      "Creates a new unfinished quest which can be configured.\r\n"
      "\r\n"
      "Usage:\r\n"
      "  qcreate <vnum>\r\n"
      "\r\n"
      "Examples:\r\n"
      "  qcreate 1001\r\n");
    return;
  }

  if (!is_number(arg)) {
    send_to_char(ch,
      "Creates a new unfinished quest which can be configured.\r\n"
      "\r\n"
      "Usage:\r\n"
      "  qcreate <vnum>\r\n"
      "\r\n"
      "Examples:\r\n"
      "  qcreate 1001\r\n");
    return;
  }

  vnum = atoi(arg);
  if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
    send_to_char(ch, "That quest VNUM can't exist.\r\n");
    return;
  }

  if (real_quest(vnum) != NOTHING) {
    send_to_char(ch, "Quest %d already exists.\r\n", vnum);
    return;
  }

  znum = real_zone_by_thing(vnum);
  if (znum == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    return;
  }

  if (!can_edit_zone(ch, znum)) {
    send_cannot_edit(ch, zone_table[znum].number);
    return;
  }

  CREATE(quest, struct aq_data, 1);
  quest->vnum       = vnum;
  quest->qm         = NOBODY;
  quest->flags      = 0;
  quest->type       = AQ_UNDEFINED;
  quest->target     = NOTHING;
  quest->prereq     = NOTHING;
  quest->value[0]   = 0;
  quest->value[1]   = 0;
  quest->value[2]   = 0;
  quest->value[3]   = LVL_IMPL;
  quest->value[4]   = -1;
  quest->value[5]   = NOBODY;
  quest->value[6]   = 1;
  quest->prev_quest = NOTHING;
  quest->next_quest = NOTHING;
  quest->coins_reward = 0;
  quest->exp_reward = 0;
  quest->obj_reward = NOTHING;
  quest->func       = NULL;

  strlcpy(namebuf, GET_NAME(ch), sizeof(namebuf));
  snprintf(buf, sizeof(buf), "unfinished quest made by %.*s",
           (int)sizeof(namebuf) - 1, namebuf);
  quest->name = strdup(buf);

  ct = time(0);
  strftime(timestr, sizeof(timestr), "%c", localtime(&ct));
  snprintf(buf, sizeof(buf),
           "This is an unfinished quest created by %.*s on %.*s",
           (int)sizeof(namebuf) - 1, namebuf,
           (int)sizeof(timestr) - 1, timestr);
  quest->desc = strdup(buf);

  quest->info = strdup("There is no information on this quest.\r\n");
  quest->done = strdup("You have completed the quest.\r\n");
  quest->quit = strdup("You have abandoned the quest.\r\n");

  add_quest(quest);

  if (in_save_list(zone_table[znum].number, SL_QST))
    remove_from_save_list(zone_table[znum].number, SL_QST);

  free_quest(quest);

  send_to_char(ch, "Quest %d created.\r\n", vnum);
}

ACMD(do_osave)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  struct obj_data *proto;
  obj_rnum robj_num;
  obj_vnum vnum;
  zone_rnum znum;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "osave is only usable by connected players.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch,
      "Saves an object and its current properties to disk, which will load upon next boot.\r\n"
      "\r\n"
      "Usage:\r\n"
      "  osave <vnum>\r\n"
      "\r\n"
      "Examples:\r\n"
      "  osave 1001\r\n");
    return;
  }

  if (!is_number(arg)) {
    send_to_char(ch,
      "Saves an object and its current properties to disk, which will load upon next boot.\r\n"
      "\r\n"
      "Usage:\r\n"
      "  osave <vnum>\r\n"
      "\r\n"
      "Examples:\r\n"
      "  osave 1001\r\n");
    return;
  }

  vnum = atoi(arg);
  if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
    send_to_char(ch, "That object VNUM can't exist.\r\n");
    return;
  }

  obj = find_obj_vnum_nearby(ch, vnum);
  if (obj == NULL) {
    send_to_char(ch,
      "osave: object %d is not in your inventory or room.\r\n", vnum);
    return;
  }

  znum = real_zone_by_thing(vnum);
  if (znum == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    return;
  }

  if (!can_edit_zone(ch, znum)) {
    send_cannot_edit(ch, zone_table[znum].number);
    return;
  }

  CREATE(proto, struct obj_data, 1);
  clear_object(proto);
  copy_object(proto, obj);
  proto->in_room = NOWHERE;
  proto->carried_by = NULL;
  proto->worn_by = NULL;
  proto->worn_on = NOWHERE;
  proto->in_obj = NULL;
  proto->contains = NULL;
  proto->next_content = NULL;
  proto->next = NULL;
  proto->sitting_here = NULL;
  proto->events = NULL;
  proto->script = NULL;
  proto->script_id = 0;

  if ((robj_num = add_object(proto, vnum)) == NOTHING) {
    free_object_strings(proto);
    free(proto);
    send_to_char(ch, "osave: failed to update object %d.\r\n", vnum);
    return;
  }

  free_object_strings(proto);
  free(proto);

  for (obj = object_list; obj; obj = obj->next) {
    if (obj->item_number != robj_num)
      continue;
    if (SCRIPT(obj))
      extract_script(obj, OBJ_TRIGGER);
    free_proto_script(obj, OBJ_TRIGGER);
    copy_proto_script(&obj_proto[robj_num], obj, OBJ_TRIGGER);
    assign_triggers(obj, OBJ_TRIGGER);
  }

  save_objects(znum);
  send_to_char(ch, "osave: object %d saved to disk.\r\n", vnum);
}

ACMD(do_qsave)
{
  char arg[MAX_INPUT_LENGTH];
  qst_vnum vnum;
  zone_rnum znum;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "qsave is only usable by connected players.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch,
      "Saves a quest and its current properties to disk, which will load upon next boot.\r\n"
      "\r\n"
      "Usage:\r\n"
      "  qsave <vnum>\r\n"
      "\r\n"
      "Examples:\r\n"
      "  qsave 1001\r\n");
    return;
  }

  if (!is_number(arg)) {
    send_to_char(ch,
      "Saves a quest and its current properties to disk, which will load upon next boot.\r\n"
      "\r\n"
      "Usage:\r\n"
      "  qsave <vnum>\r\n"
      "\r\n"
      "Examples:\r\n"
      "  qsave 1001\r\n");
    return;
  }

  vnum = atoi(arg);
  if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
    send_to_char(ch, "That quest VNUM can't exist.\r\n");
    return;
  }

  if (real_quest(vnum) == NOTHING) {
    send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
    return;
  }

  znum = real_zone_by_thing(vnum);
  if (znum == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    return;
  }

  if (!can_edit_zone(ch, znum)) {
    send_cannot_edit(ch, zone_table[znum].number);
    return;
  }

  if (!save_quests(znum)) {
    send_to_char(ch, "qsave: failed.\r\n");
    return;
  }

  send_to_char(ch, "qsave: quest %d saved to disk.\r\n", vnum);
}

static void qset_show_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Usage:\r\n"
    "  qset show <vnum>\r\n"
    "  qset add name <vnum> <text>\r\n"
    "  qset add desc <vnum> <text>\r\n"
    "  qset add msg <vnum> <type> <text>\r\n"
    "  qset add flags <vnum> <flags> [flags]\r\n"
    "  qset add type <vnum> <type>\r\n"
    "  qset add npc <vnum> <npc vnum>\r\n"
    "  qset add target <vnum> <target vnum>\r\n"
    "  qset add amount <vnum> <amount of coins>\r\n"
    "  qset add reward <vnum> <amount of coins>\r\n"
    "  qset add prereq <vnum> <prerequisite object vnum>\r\n"
    "  qset add prev <vnum> <previous quest vnum>\r\n"
    "  qset del <vnum> <field>\r\n"
    "  qset clear <vnum> force\r\n"
    "  qset validate <vnum>\r\n");
}

static void qset_show_add_name_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a name to the quest.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  qset add name <vnum> <text>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  qset add name 101 Deliver scroll to House Tsalaxa\r\n");
}

static void qset_show_add_desc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a description to the quest. Useful for identifying quest lines or simple quests.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  qset add desc <vnum> <text>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  qset add desc 101 Introduction quest to House Tsalaxa\r\n");
}

static void qset_show_add_msg_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a message to the quest that the player sees. Can be an accept, completion,\r\n"
    "or abandonment/quit message.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  qset add msg <vnum> <type> <text>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  qset add msg 101 accept You have agreed to take the scroll to House Tsalaxa.\r\n"
    "  qset add msg 101 complete You hand over the scroll to House Tsalaxa's courier.\r\n"
    "  qset add msg 101 quit You decide not to deliver the scroll and discard it.\r\n");
}

static void qset_show_add_flags_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds flags to the quest. Useful for repeatable quests or one-offs.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  qset add flags <vnum> repeatable\r\n"
    "\r\n"
    "Flags:\r\n");
  column_list(ch, 0, aq_flags, NUM_AQ_FLAGS, FALSE);
}

static void qset_show_add_type_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Determines the type of quest. Generally is either an object or NPC.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  qset add type <vnum> <type>\r\n"
    "\r\n"
    "Types:\r\n");
  column_list(ch, 0, quest_types, NUM_AQ_TYPES, FALSE);
  send_to_char(ch,
    "\r\n"
    "You can use the number (1-%d) or a name/abbrev: object, room, find,\r\n"
    "kill, save, return, clear.\r\n",
    NUM_AQ_TYPES);
}

static void qset_show_add_npc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Determines the starting NPC of the quest.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  qset add npc <vnum> <npc vnum>\r\n");
}

static void qset_show_add_target_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Determines the target vnum of the quest. Generally is either an object or\r\n"
    "NPC.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  qset add target <vnum> <obj/npc vnum>\r\n");
}

static void qset_show_add_amount_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a currency reward given to a player when they complete the quest.\r\n"
    "Defaults to 0 if not specified.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  qset add amount <vnum> <amount of coins>\r\n");
}

static void qset_show_add_reward_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a currency reward given to a player when they complete the quest.\r\n"
    "Defaults to 0 if not specified.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  qset add reward <vnum> <amount of coins>\r\n");
}

static void qset_show_add_prereq_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds the object required to accept the quest. If not set, the quest\r\n"
    "does not require an item to be accepted.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  qset add prereq <vnum> <prerequisite object vnum>\r\n");
}

static void qset_show_add_prev_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds the previous quest that must be completed before this quest can be\r\n"
    "accepted. If not set, the quest has no prerequisite quest.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  qset add prev <vnum> <previous quest vnum>\r\n");
}

static void qset_mark_quest_modified(qst_vnum vnum)
{
  zone_rnum znum = real_zone_by_thing(vnum);

  if (znum == NOWHERE)
    return;

  add_to_save_list(zone_table[znum].number, SL_QST);
}

static int qset_find_flag(const char *arg)
{
  int i;

  for (i = 0; *aq_flags[i] != '\n'; i++)
    if (is_abbrev(arg, aq_flags[i]))
      return (1 << i);

  return -1;
}

static int qset_find_type(const char *arg)
{
  int type;
  char buf[MAX_INPUT_LENGTH];

  if (!arg || !*arg)
    return -1;

  if (is_number(arg)) {
    type = atoi(arg);
    if (type >= 0 && type < NUM_AQ_TYPES)
      return type;
    if (type > 0 && type <= NUM_AQ_TYPES)
      return type - 1;
    return -1;
  }

  strlcpy(buf, arg, sizeof(buf));
  for (type = 0; type < NUM_AQ_TYPES; type++)
    if (is_abbrev(buf, quest_types[type]))
      return type;

  return -1;
}

static void qset_build_msg(char *out, size_t outsz, const char *input)
{
  size_t len;

  if (!input || !*input) {
    *out = '\0';
    return;
  }

  strlcpy(out, input, outsz);
  len = strlen(out);
  if (len < 2 || out[len - 2] != '\r' || out[len - 1] != '\n')
    strlcat(out, "\r\n", outsz);
}

static void qset_show_quest(struct char_data *ch, qst_rnum rnum)
{
  char flags[MAX_STRING_LENGTH];
  char targetname[MAX_STRING_LENGTH];
  mob_rnum qmrnum;
  const char *type_name = "Unknown";

  sprintbit(QST_FLAGS(rnum), aq_flags, flags, sizeof(flags));

  if (QST_TYPE(rnum) >= 0 && QST_TYPE(rnum) < NUM_AQ_TYPES)
    type_name = quest_types[QST_TYPE(rnum)];

  switch (QST_TYPE(rnum)) {
    case AQ_OBJ_FIND:
    case AQ_OBJ_RETURN:
      snprintf(targetname, sizeof(targetname), "%s",
               real_object(QST_TARGET(rnum)) == NOTHING ?
               "An unknown object" :
               obj_proto[real_object(QST_TARGET(rnum))].short_description);
      break;
    case AQ_ROOM_FIND:
    case AQ_ROOM_CLEAR:
      snprintf(targetname, sizeof(targetname), "%s",
               real_room(QST_TARGET(rnum)) == NOWHERE ?
               "An unknown room" :
               world[real_room(QST_TARGET(rnum))].name);
      break;
    case AQ_MOB_FIND:
    case AQ_MOB_KILL:
    case AQ_MOB_SAVE:
      snprintf(targetname, sizeof(targetname), "%s",
               real_mobile(QST_TARGET(rnum)) == NOBODY ?
               "An unknown mobile" :
               GET_NAME(&mob_proto[real_mobile(QST_TARGET(rnum))]));
      break;
    default:
      snprintf(targetname, sizeof(targetname), "Unknown");
      break;
  }

  qmrnum = real_mobile(QST_MASTER(rnum));
  send_to_char(ch, "Quest [%d]: %s\r\n", QST_NUM(rnum),
               QST_NAME(rnum) ? QST_NAME(rnum) : "<None>");
  send_to_char(ch, "Description: %s\r\n", QST_DESC(rnum) ? QST_DESC(rnum) : "<None>");
  send_to_char(ch, "Accept Message:\r\n%s", QST_INFO(rnum) ? QST_INFO(rnum) : "  <None>\r\n");
  send_to_char(ch, "Completion Message:\r\n%s", QST_DONE(rnum) ? QST_DONE(rnum) : "  <None>\r\n");
  send_to_char(ch, "Quit Message:\r\n%s", QST_QUIT(rnum) ? QST_QUIT(rnum) : "  <None>\r\n");
  send_to_char(ch, "Flags: %s\r\n", flags);
  send_to_char(ch, "Type: %s\r\n", type_name);
  send_to_char(ch, "Quest Master: [%d] %s\r\n",
               QST_MASTER(rnum) == NOBODY ? -1 : QST_MASTER(rnum),
               qmrnum == NOBODY ? "Invalid Mob" : mob_proto[qmrnum].player.short_descr);
  send_to_char(ch, "Target: [%d] %s\r\n",
               QST_TARGET(rnum) == NOTHING ? -1 : QST_TARGET(rnum), targetname);
  send_to_char(ch, "Amount: %d\r\n", QST_QUANTITY(rnum));
  send_to_char(ch, "Reward: %d coin%s\r\n", QST_COINS(rnum),
               QST_COINS(rnum) == 1 ? "" : "s");
  if (QST_PREREQ(rnum) == NOTHING) {
    send_to_char(ch, "Prereq: None\r\n");
  } else {
    obj_rnum ornum = real_object(QST_PREREQ(rnum));
    const char *sdesc = ornum == NOTHING ? "an unknown object" : obj_proto[ornum].short_description;
    send_to_char(ch, "Prereq: [%d] %s\r\n", QST_PREREQ(rnum), sdesc);
  }
}

static void qset_validate_quest(struct char_data *ch, qst_rnum rnum)
{
  int errors = 0;

  if (!QST_NAME(rnum) || !*QST_NAME(rnum)) {
    send_to_char(ch, "Error: quest name is not set.\r\n");
    errors++;
  }

  if (!QST_DESC(rnum) || !*QST_DESC(rnum)) {
    send_to_char(ch, "Error: quest description is not set.\r\n");
    errors++;
  }

  if (!QST_INFO(rnum) || !*QST_INFO(rnum)) {
    send_to_char(ch, "Error: quest accept message is not set.\r\n");
    errors++;
  }

  if (!QST_DONE(rnum) || !*QST_DONE(rnum)) {
    send_to_char(ch, "Error: quest completion message is not set.\r\n");
    errors++;
  }

  if (!QST_QUIT(rnum) || !*QST_QUIT(rnum)) {
    send_to_char(ch, "Error: quest quit message is not set.\r\n");
    errors++;
  }

  if (QST_TYPE(rnum) < 0 || QST_TYPE(rnum) >= NUM_AQ_TYPES) {
    send_to_char(ch, "Error: quest type is invalid.\r\n");
    errors++;
  }

  if (QST_MASTER(rnum) == NOBODY) {
    send_to_char(ch, "Error: quest NPC is not set.\r\n");
    errors++;
  } else if (real_mobile(QST_MASTER(rnum)) == NOBODY) {
    send_to_char(ch, "Error: quest NPC vnum %d is invalid.\r\n", QST_MASTER(rnum));
    errors++;
  }

  if (QST_TARGET(rnum) == NOTHING) {
    send_to_char(ch, "Error: quest target is not set.\r\n");
    errors++;
  } else if (QST_TYPE(rnum) >= 0 && QST_TYPE(rnum) < NUM_AQ_TYPES) {
    if ((QST_TYPE(rnum) == AQ_OBJ_FIND || QST_TYPE(rnum) == AQ_OBJ_RETURN) &&
        real_object(QST_TARGET(rnum)) == NOTHING) {
      send_to_char(ch, "Error: quest target object vnum %d is invalid.\r\n", QST_TARGET(rnum));
      errors++;
    }
    if ((QST_TYPE(rnum) == AQ_ROOM_FIND || QST_TYPE(rnum) == AQ_ROOM_CLEAR) &&
        real_room(QST_TARGET(rnum)) == NOWHERE) {
      send_to_char(ch, "Error: quest target room vnum %d is invalid.\r\n", QST_TARGET(rnum));
      errors++;
    }
    if ((QST_TYPE(rnum) == AQ_MOB_FIND || QST_TYPE(rnum) == AQ_MOB_KILL ||
         QST_TYPE(rnum) == AQ_MOB_SAVE) &&
        real_mobile(QST_TARGET(rnum)) == NOBODY) {
      send_to_char(ch, "Error: quest target mobile vnum %d is invalid.\r\n", QST_TARGET(rnum));
      errors++;
    }
  }

  if (!errors)
    send_to_char(ch, "Quest validates cleanly.\r\n");
  else
    send_to_char(ch, "Validation failed: %d issue%s.\r\n", errors, errors == 1 ? "" : "s");
}

ACMD(do_qset)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  char arg4[MAX_INPUT_LENGTH];
  qst_vnum vnum;
  qst_rnum rnum;
  zone_rnum znum;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "qset is only usable by connected players.\r\n");
    return;
  }

  argument = one_argument(argument, arg1);
  if (!*arg1) {
    qset_show_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "show")) {
    argument = one_argument(argument, arg2);
    if (!*arg2 || !is_number(arg2)) {
      qset_show_usage(ch);
      return;
    }

    vnum = atoi(arg2);
    if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
      send_to_char(ch, "That quest VNUM can't exist.\r\n");
      return;
    }

    if ((rnum = real_quest(vnum)) == NOTHING) {
      send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
      return;
    }

    znum = real_zone_by_thing(vnum);
    if (znum == NOWHERE) {
      send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
      return;
    }

    if (!can_edit_zone(ch, znum)) {
      send_cannot_edit(ch, zone_table[znum].number);
      return;
    }

    qset_show_quest(ch, rnum);
    return;
  }

  if (is_abbrev(arg1, "add")) {
    argument = one_argument(argument, arg2);
    if (!*arg2) {
      qset_show_usage(ch);
      return;
    }

    if (is_abbrev(arg2, "name")) {
      argument = one_argument(argument, arg3);
      skip_spaces(&argument);
      if (!*arg3 || !is_number(arg3) || !*argument) {
        qset_show_add_name_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
        send_to_char(ch, "That quest VNUM can't exist.\r\n");
        return;
      }

      if ((rnum = real_quest(vnum)) == NOTHING) {
        send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
        return;
      }

      znum = real_zone_by_thing(vnum);
      if (znum == NOWHERE) {
        send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
        return;
      }

      if (!can_edit_zone(ch, znum)) {
        send_cannot_edit(ch, zone_table[znum].number);
        return;
      }

      if (!genolc_checkstring(ch->desc, argument))
        return;
      argument[MAX_QUEST_NAME - 1] = '\0';
      if (QST_NAME(rnum))
        free(QST_NAME(rnum));
      QST_NAME(rnum) = str_udup(argument);
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest name set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "desc")) {
      argument = one_argument(argument, arg3);
      skip_spaces(&argument);
      if (!*arg3 || !is_number(arg3) || !*argument) {
        qset_show_add_desc_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
        send_to_char(ch, "That quest VNUM can't exist.\r\n");
        return;
      }

      if ((rnum = real_quest(vnum)) == NOTHING) {
        send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
        return;
      }

      znum = real_zone_by_thing(vnum);
      if (znum == NOWHERE) {
        send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
        return;
      }

      if (!can_edit_zone(ch, znum)) {
        send_cannot_edit(ch, zone_table[znum].number);
        return;
      }

      if (!genolc_checkstring(ch->desc, argument))
        return;
      argument[MAX_QUEST_DESC - 1] = '\0';
      if (QST_DESC(rnum))
        free(QST_DESC(rnum));
      QST_DESC(rnum) = str_udup(argument);
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest description set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "msg")) {
      char msgbuf[MAX_QUEST_MSG];
      int msg_type = -1;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg4);
      skip_spaces(&argument);
      if (!*arg3 || !is_number(arg3) || !*arg4 || !*argument) {
        qset_show_add_msg_usage(ch);
        return;
      }

      if (is_abbrev(arg4, "accept"))
        msg_type = QEDIT_INFO;
      else if (is_abbrev(arg4, "complete") || is_abbrev(arg4, "completion"))
        msg_type = QEDIT_COMPLETE;
      else if (is_abbrev(arg4, "quit") || is_abbrev(arg4, "abandon") || is_abbrev(arg4, "abandonment"))
        msg_type = QEDIT_ABANDON;
      else {
        qset_show_add_msg_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
        send_to_char(ch, "That quest VNUM can't exist.\r\n");
        return;
      }

      if ((rnum = real_quest(vnum)) == NOTHING) {
        send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
        return;
      }

      znum = real_zone_by_thing(vnum);
      if (znum == NOWHERE) {
        send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
        return;
      }

      if (!can_edit_zone(ch, znum)) {
        send_cannot_edit(ch, zone_table[znum].number);
        return;
      }

      if (!genolc_checkstring(ch->desc, argument))
        return;

      qset_build_msg(msgbuf, sizeof(msgbuf), argument);
      msgbuf[MAX_QUEST_MSG - 1] = '\0';

      if (msg_type == QEDIT_INFO) {
        if (QST_INFO(rnum))
          free(QST_INFO(rnum));
        QST_INFO(rnum) = str_udup(msgbuf);
      } else if (msg_type == QEDIT_COMPLETE) {
        if (QST_DONE(rnum))
          free(QST_DONE(rnum));
        QST_DONE(rnum) = str_udup(msgbuf);
      } else {
        if (QST_QUIT(rnum))
          free(QST_QUIT(rnum));
        QST_QUIT(rnum) = str_udup(msgbuf);
      }

      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest message set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "flags")) {
      bool any = FALSE;

      argument = one_argument(argument, arg3);
      if (!*arg3 || !is_number(arg3)) {
        qset_show_add_flags_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
        send_to_char(ch, "That quest VNUM can't exist.\r\n");
        return;
      }

      if ((rnum = real_quest(vnum)) == NOTHING) {
        send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
        return;
      }

      znum = real_zone_by_thing(vnum);
      if (znum == NOWHERE) {
        send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
        return;
      }

      if (!can_edit_zone(ch, znum)) {
        send_cannot_edit(ch, zone_table[znum].number);
        return;
      }

      if (!*argument) {
        qset_show_add_flags_usage(ch);
        return;
      }

      while (*argument) {
        int flag;

        argument = one_argument(argument, arg4);
        if (!*arg4)
          break;

        flag = qset_find_flag(arg4);
        if (flag < 0) {
          send_to_char(ch, "Unknown quest flag: %s\r\n", arg4);
          continue;
        }

        SET_BIT(QST_FLAGS(rnum), flag);
        any = TRUE;
      }

      if (any) {
        qset_mark_quest_modified(vnum);
        send_to_char(ch, "Quest flags updated.\r\n");
      }
      return;
    }

    if (is_abbrev(arg2, "type")) {
      int type;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg4);
      if (!*arg3 || !is_number(arg3) || !*arg4) {
        qset_show_add_type_usage(ch);
        return;
      }

      type = qset_find_type(arg4);
      if (type < 0) {
        qset_show_add_type_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
        send_to_char(ch, "That quest VNUM can't exist.\r\n");
        return;
      }

      if ((rnum = real_quest(vnum)) == NOTHING) {
        send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
        return;
      }

      znum = real_zone_by_thing(vnum);
      if (znum == NOWHERE) {
        send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
        return;
      }

      if (!can_edit_zone(ch, znum)) {
        send_cannot_edit(ch, zone_table[znum].number);
        return;
      }

      QST_TYPE(rnum) = type;
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest type set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "npc")) {
      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg4);
      if (!*arg3 || !is_number(arg3) || !*arg4 || !is_number(arg4)) {
        qset_show_add_npc_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
        send_to_char(ch, "That quest VNUM can't exist.\r\n");
        return;
      }

      if ((rnum = real_quest(vnum)) == NOTHING) {
        send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
        return;
      }

      znum = real_zone_by_thing(vnum);
      if (znum == NOWHERE) {
        send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
        return;
      }

      if (!can_edit_zone(ch, znum)) {
        send_cannot_edit(ch, zone_table[znum].number);
        return;
      }

      if (real_mobile(atoi(arg4)) == NOBODY) {
        send_to_char(ch, "That NPC vnum does not exist.\r\n");
        return;
      }

      QST_MASTER(rnum) = atoi(arg4);
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest NPC set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "target")) {
      int target;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg4);
      if (!*arg3 || !is_number(arg3) || !*arg4 || !is_number(arg4)) {
        qset_show_add_target_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
        send_to_char(ch, "That quest VNUM can't exist.\r\n");
        return;
      }

      if ((rnum = real_quest(vnum)) == NOTHING) {
        send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
        return;
      }

      znum = real_zone_by_thing(vnum);
      if (znum == NOWHERE) {
        send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
        return;
      }

      if (!can_edit_zone(ch, znum)) {
        send_cannot_edit(ch, zone_table[znum].number);
        return;
      }

      target = atoi(arg4);
      if (QST_TYPE(rnum) == AQ_OBJ_FIND || QST_TYPE(rnum) == AQ_OBJ_RETURN) {
        if (real_object(target) == NOTHING) {
          send_to_char(ch, "That object vnum does not exist.\r\n");
          return;
        }
      } else if (QST_TYPE(rnum) == AQ_ROOM_FIND || QST_TYPE(rnum) == AQ_ROOM_CLEAR) {
        if (real_room(target) == NOWHERE) {
          send_to_char(ch, "That room vnum does not exist.\r\n");
          return;
        }
      } else if (QST_TYPE(rnum) == AQ_MOB_FIND || QST_TYPE(rnum) == AQ_MOB_KILL ||
                 QST_TYPE(rnum) == AQ_MOB_SAVE) {
        if (real_mobile(target) == NOBODY) {
          send_to_char(ch, "That NPC vnum does not exist.\r\n");
          return;
        }
      }

      QST_TARGET(rnum) = target;
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest target set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "amount")) {
      int amount;
      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg4);
      if (!*arg3 || !is_number(arg3) || !*arg4 || !is_number(arg4) || *argument) {
        qset_show_add_amount_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
        send_to_char(ch, "That quest VNUM can't exist.\r\n");
        return;
      }

      if ((rnum = real_quest(vnum)) == NOTHING) {
        send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
        return;
      }

      znum = real_zone_by_thing(vnum);
      if (znum == NOWHERE) {
        send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
        return;
      }

      if (!can_edit_zone(ch, znum)) {
        send_cannot_edit(ch, zone_table[znum].number);
        return;
      }

      amount = atoi(arg4);
      QST_COINS(rnum) = LIMIT(amount, 0, 99999);
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest reward set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "reward")) {
      int reward;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg4);
      if (!*arg3 || !is_number(arg3) || !*arg4 || !is_number(arg4)) {
        qset_show_add_reward_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
        send_to_char(ch, "That quest VNUM can't exist.\r\n");
        return;
      }

      if ((rnum = real_quest(vnum)) == NOTHING) {
        send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
        return;
      }

      znum = real_zone_by_thing(vnum);
      if (znum == NOWHERE) {
        send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
        return;
      }

      if (!can_edit_zone(ch, znum)) {
        send_cannot_edit(ch, zone_table[znum].number);
        return;
      }

      reward = atoi(arg4);
      QST_COINS(rnum) = LIMIT(reward, 0, 99999);
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest reward set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "prereq")) {
      int prereq;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg4);
      if (!*arg3 || !is_number(arg3) || !*arg4 || !is_number(arg4)) {
        qset_show_add_prereq_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
        send_to_char(ch, "That quest VNUM can't exist.\r\n");
        return;
      }

      if ((rnum = real_quest(vnum)) == NOTHING) {
        send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
        return;
      }

      znum = real_zone_by_thing(vnum);
      if (znum == NOWHERE) {
        send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
        return;
      }

      if (!can_edit_zone(ch, znum)) {
        send_cannot_edit(ch, zone_table[znum].number);
        return;
      }

      prereq = atoi(arg4);
      if (real_object(prereq) == NOTHING) {
        send_to_char(ch, "That object vnum does not exist.\r\n");
        return;
      }

      QST_PREREQ(rnum) = prereq;
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest prerequisite set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "prev")) {
      qst_vnum prev;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg4);
      if (!*arg3 || !is_number(arg3) || !*arg4 || !is_number(arg4)) {
        qset_show_add_prev_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
        send_to_char(ch, "That quest VNUM can't exist.\r\n");
        return;
      }

      if ((rnum = real_quest(vnum)) == NOTHING) {
        send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
        return;
      }

      znum = real_zone_by_thing(vnum);
      if (znum == NOWHERE) {
        send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
        return;
      }

      if (!can_edit_zone(ch, znum)) {
        send_cannot_edit(ch, zone_table[znum].number);
        return;
      }

      prev = atoi(arg4);
      if (real_quest(prev) == NOTHING) {
        send_to_char(ch, "That previous quest vnum does not exist.\r\n");
        return;
      }

      QST_PREV(rnum) = prev;
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest previous requirement set.\r\n");
      return;
    }

    qset_show_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "del")) {
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);
    if (!*arg2 || !is_number(arg2) || !*arg3) {
      qset_show_usage(ch);
      return;
    }

    vnum = atoi(arg2);
    if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
      send_to_char(ch, "That quest VNUM can't exist.\r\n");
      return;
    }

    if ((rnum = real_quest(vnum)) == NOTHING) {
      send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
      return;
    }

    znum = real_zone_by_thing(vnum);
    if (znum == NOWHERE) {
      send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
      return;
    }

    if (!can_edit_zone(ch, znum)) {
      send_cannot_edit(ch, zone_table[znum].number);
      return;
    }

    if (is_abbrev(arg3, "name")) {
      if (QST_NAME(rnum))
        free(QST_NAME(rnum));
      QST_NAME(rnum) = strdup("");
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest name cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "desc")) {
      if (QST_DESC(rnum))
        free(QST_DESC(rnum));
      QST_DESC(rnum) = strdup("");
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest description cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "msg")) {
      int msg_type = -1;

      argument = one_argument(argument, arg4);
      if (*arg4) {
        if (is_abbrev(arg4, "accept"))
          msg_type = QEDIT_INFO;
        else if (is_abbrev(arg4, "complete") || is_abbrev(arg4, "completion"))
          msg_type = QEDIT_COMPLETE;
        else if (is_abbrev(arg4, "quit") || is_abbrev(arg4, "abandon") || is_abbrev(arg4, "abandonment"))
          msg_type = QEDIT_ABANDON;
        else {
          qset_show_add_msg_usage(ch);
          return;
        }
      }

      if (msg_type == QEDIT_INFO || msg_type == -1) {
        if (QST_INFO(rnum))
          free(QST_INFO(rnum));
        QST_INFO(rnum) = strdup("");
      }
      if (msg_type == QEDIT_COMPLETE || msg_type == -1) {
        if (QST_DONE(rnum))
          free(QST_DONE(rnum));
        QST_DONE(rnum) = strdup("");
      }
      if (msg_type == QEDIT_ABANDON || msg_type == -1) {
        if (QST_QUIT(rnum))
          free(QST_QUIT(rnum));
        QST_QUIT(rnum) = strdup("");
      }

      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest message cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "flags")) {
      bool any = FALSE;

      if (!*argument) {
        if (QST_FLAGS(rnum)) {
          QST_FLAGS(rnum) = 0;
          qset_mark_quest_modified(vnum);
        }
        send_to_char(ch, "Quest flags cleared.\r\n");
        return;
      }

      while (*argument) {
        int flag;

        argument = one_argument(argument, arg4);
        if (!*arg4)
          break;

        flag = qset_find_flag(arg4);
        if (flag < 0) {
          send_to_char(ch, "Unknown quest flag: %s\r\n", arg4);
          continue;
        }

        REMOVE_BIT(QST_FLAGS(rnum), flag);
        any = TRUE;
      }

      if (any) {
        qset_mark_quest_modified(vnum);
        send_to_char(ch, "Quest flags updated.\r\n");
      }
      return;
    }

    if (is_abbrev(arg3, "type")) {
      QST_TYPE(rnum) = AQ_UNDEFINED;
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest type cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "npc")) {
      QST_MASTER(rnum) = NOBODY;
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest NPC cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "target")) {
      QST_TARGET(rnum) = NOTHING;
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest target cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "amount")) {
      QST_COINS(rnum) = 0;
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest reward cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "reward")) {
      QST_COINS(rnum) = 0;
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest reward cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "prereq")) {
      QST_PREREQ(rnum) = NOTHING;
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest prerequisite cleared.\r\n");
      return;
    }

    if (is_abbrev(arg3, "prev")) {
      QST_PREV(rnum) = NOTHING;
      qset_mark_quest_modified(vnum);
      send_to_char(ch, "Quest previous requirement cleared.\r\n");
      return;
    }

    qset_show_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "clear")) {
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);
    if (!*arg2 || !is_number(arg2) || !*arg3 || !is_abbrev(arg3, "force")) {
      qset_show_usage(ch);
      return;
    }

    vnum = atoi(arg2);
    if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
      send_to_char(ch, "That quest VNUM can't exist.\r\n");
      return;
    }

    if ((rnum = real_quest(vnum)) == NOTHING) {
      send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
      return;
    }

    znum = real_zone_by_thing(vnum);
    if (znum == NOWHERE) {
      send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
      return;
    }

    if (!can_edit_zone(ch, znum)) {
      send_cannot_edit(ch, zone_table[znum].number);
      return;
    }

    free_quest_strings(&aquest_table[rnum]);
    QST_NAME(rnum) = strdup("Undefined Quest");
    QST_DESC(rnum) = strdup("Quest definition is incomplete.");
    QST_INFO(rnum) = strdup("There is no information on this quest.\r\n");
    QST_DONE(rnum) = strdup("You have completed the quest.\r\n");
    QST_QUIT(rnum) = strdup("You have abandoned the quest.\r\n");
    QST_FLAGS(rnum) = 0;
    QST_TYPE(rnum) = AQ_UNDEFINED;
    QST_MASTER(rnum) = NOBODY;
    QST_TARGET(rnum) = NOTHING;
    QST_PREREQ(rnum) = NOTHING;
    QST_POINTS(rnum) = 0;
    QST_PENALTY(rnum) = 0;
    QST_MINLEVEL(rnum) = 0;
    QST_MAXLEVEL(rnum) = LVL_IMPL;
    QST_TIME(rnum) = -1;
    QST_RETURNMOB(rnum) = NOBODY;
    QST_QUANTITY(rnum) = 1;
    QST_COINS(rnum) = 0;
    QST_EXP(rnum) = 0;
    QST_OBJ(rnum) = NOTHING;
    QST_PREV(rnum) = NOTHING;
    QST_NEXT(rnum) = NOTHING;

    qset_mark_quest_modified(vnum);
    send_to_char(ch, "Quest cleared.\r\n");
    return;
  }

  if (is_abbrev(arg1, "validate")) {
    argument = one_argument(argument, arg2);
    if (!*arg2 || !is_number(arg2)) {
      qset_show_usage(ch);
      return;
    }

    vnum = atoi(arg2);
    if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
      send_to_char(ch, "That quest VNUM can't exist.\r\n");
      return;
    }

    if ((rnum = real_quest(vnum)) == NOTHING) {
      send_to_char(ch, "Quest %d does not exist.\r\n", vnum);
      return;
    }

    znum = real_zone_by_thing(vnum);
    if (znum == NOWHERE) {
      send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
      return;
    }

    if (!can_edit_zone(ch, znum)) {
      send_cannot_edit(ch, zone_table[znum].number);
      return;
    }

    qset_validate_quest(ch, rnum);
    return;
  }

  qset_show_usage(ch);
}

/* ====== Builder snapshot: save a staged mob's gear as its prototype loadout ====== */
static void msave_loadout_append(struct mob_loadout **head,
                                 struct mob_loadout **tail,
                                 obj_vnum vnum, sh_int wear_pos,
                                 int *eq_count, int *inv_count) {
  struct mob_loadout *e;
  CREATE(e, struct mob_loadout, 1);
  e->vnum = vnum;
  e->wear_pos = wear_pos;
  e->quantity = 1;
  e->next = NULL;

  if (*tail)
    (*tail)->next = e;
  else
    *head = e;
  *tail = e;

  if (wear_pos >= 0)
    (*eq_count)++;
  else
    (*inv_count)++;
}

static void msave_capture_obj_tree(struct mob_loadout **head,
                                   struct mob_loadout **tail,
                                   struct obj_data *obj, sh_int base_wear_pos,
                                   int depth, int *eq_count, int *inv_count) {
  sh_int wear_pos;
  struct obj_data *cont;

  if (!obj || GET_OBJ_VNUM(obj) <= 0)
    return;

  if (depth <= 0)
    wear_pos = base_wear_pos;
  else
    wear_pos = (sh_int)(-(depth + 1));

  msave_loadout_append(head, tail, GET_OBJ_VNUM(obj), wear_pos,
                       eq_count, inv_count);

  for (cont = obj->contains; cont; cont = cont->next_content)
    msave_capture_obj_tree(head, tail, cont, base_wear_pos, depth + 1,
                           eq_count, inv_count);
}

ACMD(do_msave)
{
  char a1[MAX_INPUT_LENGTH], a2[MAX_INPUT_LENGTH];
  char target[MAX_INPUT_LENGTH] = {0}, flags[MAX_INPUT_LENGTH] = {0};
  struct char_data *vict = NULL, *tmp = NULL;
  mob_rnum rnum;
  int include_inv = 0;      /* -all */
  int clear_first = 1;      /* default replace; -append flips this to 0 */
  int equips_added = 0, inv_entries = 0;
  int pos;
  struct obj_data *o;
  struct mob_loadout *lo_head = NULL;
  struct mob_loadout *lo_tail = NULL;

  two_arguments(argument, a1, a2);
  if (*a1 && *a1 == '-') {
    /* user wrote: msave -flags <mob> */
    strcpy(flags, a1);
    strcpy(target, a2);
  } else {
    /* user wrote: msave <mob> [-flags] */
    strcpy(target, a1);
    strcpy(flags, a2);
  }

  /* Parse flags (space-separated, any order) */
  if (*flags) {
    char buf[MAX_INPUT_LENGTH], *p = flags;
    while (*p) {
      p = one_argument(p, buf);
      if (!*buf) break;
      if (!str_cmp(buf, "-all")) include_inv = 1;
      else if (!str_cmp(buf, "-append")) clear_first = 0;
      else if (!str_cmp(buf, "-clear")) clear_first = 1;
      else {
        send_to_char(ch, "Unknown flag '%s'. Try -all, -append, or -clear.\r\n", buf);
        return;
      }
    }
  }

  /* Find target mob in the room */
  if (*target)
    vict = get_char_vis(ch, target, NULL, FIND_CHAR_ROOM);
  else {
    /* No name: pick the first NPC only if exactly one exists */
    for (tmp = world[IN_ROOM(ch)].people; tmp; tmp = tmp->next_in_room) {
      if (IS_NPC(tmp)) {
        if (vict) { vict = NULL; break; } /* more than one — force explicit name */
        vict = tmp;
      }
    }
  }

  if (!vict || !IS_NPC(vict)) {
    send_to_char(ch, "Target an NPC in this room: msave <mob> [-all] [-append|-clear]\r\n");
    return;
  }

  /* Resolve prototype and permission to edit its zone */
  rnum = GET_MOB_RNUM(vict);
  if (rnum < 0) {
    send_to_char(ch, "I can’t resolve that mob's prototype.\r\n");
    return;
  }

#ifdef CAN_EDIT_ZONE
  if (!can_edit_zone(ch, real_zone_by_thing(GET_MOB_VNUM(vict)))) {
    send_to_char(ch, "You don’t have permission to modify that mob’s zone.\r\n");
    return;
  }
#endif

  /* Build the new loadout into the PROTOTYPE */
  if (clear_first) {
    loadout_free_list(&mob_proto[rnum].proto_loadout);
    mob_proto[rnum].proto_loadout = NULL;
  }

  lo_head = mob_proto[rnum].proto_loadout;
  lo_tail = lo_head;
  while (lo_tail && lo_tail->next)
    lo_tail = lo_tail->next;

  /* Capture equipment: one entry per worn slot */
  for (pos = 0; pos < NUM_WEARS; pos++) {
    o = GET_EQ(vict, pos);
    if (!o) continue;
    if (GET_OBJ_VNUM(o) <= 0) continue;
    msave_capture_obj_tree(&lo_head, &lo_tail, o, (sh_int)pos, 0,
                           &equips_added, &inv_entries);
  }

  /* Capture inventory (with nesting) if requested */
  if (include_inv) {
    for (o = vict->carrying; o; o = o->next_content) {
      if (GET_OBJ_VNUM(o) <= 0) continue;
      msave_capture_obj_tree(&lo_head, &lo_tail, o, -1, 0,
                             &equips_added, &inv_entries);
    }
  }

  mob_proto[rnum].proto_loadout = lo_head;

  /* Persist to disk: save the zone owning this mob vnum */
  {
    zone_rnum zr = real_zone_by_thing(GET_MOB_VNUM(vict));
    if (zr == NOWHERE) {
      mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
             "msave: could not resolve zone for mob %d", GET_MOB_VNUM(vict));
      send_to_char(ch, "Saved in memory, but couldn’t resolve zone to write disk.\r\n");
    } else {
      save_mobiles(zr);
      send_to_char(ch,
        "Loadout saved for mob [%d]. Equipped: %d, Inventory lines: %d%s\r\n",
        GET_MOB_VNUM(vict), equips_added, inv_entries, include_inv ? "" : " (use -all to include inventory)");
      mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
             "msave: %s saved loadout for mob %d (eq=%d, inv=%d) in zone %d",
             GET_NAME(ch), GET_MOB_VNUM(vict), equips_added, inv_entries,
             zone_table[zr].number);
    }
  }

}

ACMD(do_rsave)
{
  room_rnum rnum;
  zone_rnum znum;
  int ok;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Mobiles can’t use this.\r\n");
    return;
  }

  /* IN_ROOM(ch) is already a room_rnum (index into world[]). Do NOT pass it to real_room(). */
  rnum = IN_ROOM(ch);

  if (rnum == NOWHERE || rnum < 0 || rnum > top_of_world) {
    send_to_char(ch, "You are not in a valid room.\r\n");
    return;
  }

  znum = world[rnum].zone;
  if (znum < 0 || znum > top_of_zone_table) {
    send_to_char(ch, "This room is not attached to a valid zone.\r\n");
    return;
  }

  /* Optional: permission check */
  if (!can_edit_zone(ch, znum)) {
    send_to_char(ch, "You don’t have permission to modify zone %d.\r\n",
                 zone_table[znum].number);
    return;
  }

  /* Save the owning zone's .toml file so the room data persists */
  ok = save_rooms(znum);
  if (ok)
    ok = RoomSave_now(rnum);

  if (!ok) {
    send_to_char(ch, "rsave: failed.\r\n");
    mudlog(BRF, GET_LEVEL(ch), TRUE,
           "RSAVE FAIL: %s room %d (rnum=%d) zone %d (znum=%d)",
           GET_NAME(ch), GET_ROOM_VNUM(rnum), rnum,
           zone_table[znum].number, znum);
    return;
  }

  send_to_char(ch, "rsave: room %d saved to world file for zone %d.\r\n",
               GET_ROOM_VNUM(rnum), zone_table[znum].number);

  mudlog(CMP, GET_LEVEL(ch), TRUE,
         "RSAVE OK: %s room %d (rnum=%d) -> world/wld/%d.toml",
         GET_NAME(ch), GET_ROOM_VNUM(rnum), rnum, zone_table[znum].number);
}

/* Write saved rooms under lib/world/rsv/<vnum>.toml (like wld/ zon/ obj/). */
#ifndef ROOMSAVE_PREFIX
#define ROOMSAVE_PREFIX  LIB_WORLD "rsv/"
#endif
#ifndef ROOMSAVE_EXT
#define ROOMSAVE_EXT     ".toml"
#endif

static unsigned char *roomsave_dirty = NULL;

void RoomSave_init_dirty(void) {
  free(roomsave_dirty);
  roomsave_dirty = calloc((size_t)top_of_world + 1, 1);
}

void RoomSave_mark_dirty_room(room_rnum rnum) {
  if (!roomsave_dirty) return;
  if (rnum != NOWHERE && rnum >= 0 && rnum <= top_of_world)
    roomsave_dirty[rnum] = 1;
}

/* Where does an object “live” (topmost location -> room)? */
room_rnum RoomSave_room_of_obj(struct obj_data *o) {
  if (!o) return NOWHERE;
  while (o->in_obj) o = o->in_obj;
  if (o->carried_by) return IN_ROOM(o->carried_by);
  if (o->worn_by)    return IN_ROOM(o->worn_by);
  return o->in_room;
}

static struct obj_data *RS_create_obj_by_vnum(obj_vnum ov);
static struct char_data *RS_create_mob_by_vnum(mob_vnum mv);
static void RS_apply_inventory_loadout(struct char_data *mob);

/* --- RoomSave TOML helpers --- */
static int roomsave_toml_get_int_default(toml_table_t *tab, const char *key, int def)
{
  toml_datum_t v = toml_int_in(tab, key);
  if (v.ok)
    return (int)v.u.i;
  return def;
}

static int roomsave_toml_get_int_array(toml_table_t *tab, const char *key, int *out, int out_len)
{
  toml_array_t *arr;
  int i, n;

  if (!tab || !out || out_len <= 0)
    return 0;

  arr = toml_array_in(tab, key);
  if (!arr)
    return 0;
  n = toml_array_nelem(arr);
  if (n > out_len)
    n = out_len;
  for (i = 0; i < n; i++) {
    toml_datum_t v = toml_int_at(arr, i);
    if (v.ok)
      out[i] = (int)v.u.i;
  }
  return n;
}

struct roomsave_obj {
  int vnum;
  int timer;
  int weight;
  int cost;
  int cost_per_day;
  int extra_flags[EF_ARRAY_MAX];
  int wear_flags[TW_ARRAY_MAX];
  int values[NUM_OBJ_VAL_POSITIONS];
  int full;
  struct roomsave_obj *contents;
  struct roomsave_obj *next;
};

struct roomsave_mob_item {
  int vnum;
  int wear_pos;
  struct roomsave_obj *contents;
  struct roomsave_mob_item *next;
};

struct roomsave_mob {
  int vnum;
  struct roomsave_mob_item *equipment;
  struct roomsave_mob_item *inventory;
  struct roomsave_mob *next;
};

struct roomsave_room {
  int vnum;
  long saved_at;
  struct roomsave_obj *objects;
  struct roomsave_mob *mobs;
  struct roomsave_room *next;
};

static void roomsave_free_obj_list(struct roomsave_obj *obj)
{
  while (obj) {
    struct roomsave_obj *next = obj->next;
    if (obj->contents)
      roomsave_free_obj_list(obj->contents);
    free(obj);
    obj = next;
  }
}

static void roomsave_free_mob_items(struct roomsave_mob_item *item)
{
  while (item) {
    struct roomsave_mob_item *next = item->next;
    if (item->contents)
      roomsave_free_obj_list(item->contents);
    free(item);
    item = next;
  }
}

static void roomsave_free_mobs(struct roomsave_mob *mob)
{
  while (mob) {
    struct roomsave_mob *next = mob->next;
    roomsave_free_mob_items(mob->equipment);
    roomsave_free_mob_items(mob->inventory);
    free(mob);
    mob = next;
  }
}

static void roomsave_free_rooms(struct roomsave_room *room)
{
  while (room) {
    struct roomsave_room *next = room->next;
    roomsave_free_obj_list(room->objects);
    roomsave_free_mobs(room->mobs);
    free(room);
    room = next;
  }
}

static struct roomsave_obj *roomsave_parse_obj_table(toml_table_t *tab, int full)
{
  struct roomsave_obj *obj;
  toml_array_t *arr;
  int i;

  if (!tab)
    return NULL;

  CREATE(obj, struct roomsave_obj, 1);
  obj->vnum = roomsave_toml_get_int_default(tab, "vnum", -1);
  obj->full = full;
  if (obj->vnum <= 0) {
    free(obj);
    return NULL;
  }

  if (full) {
    obj->timer = roomsave_toml_get_int_default(tab, "timer", 0);
    obj->weight = roomsave_toml_get_int_default(tab, "weight", 0);
    obj->cost = roomsave_toml_get_int_default(tab, "cost", 0);
    obj->cost_per_day = roomsave_toml_get_int_default(tab, "cost_per_day", 0);
    for (i = 0; i < EF_ARRAY_MAX; i++)
      obj->extra_flags[i] = 0;
    for (i = 0; i < TW_ARRAY_MAX; i++)
      obj->wear_flags[i] = 0;
    for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
      obj->values[i] = 0;
    roomsave_toml_get_int_array(tab, "extra_flags", obj->extra_flags, EF_ARRAY_MAX);
    roomsave_toml_get_int_array(tab, "wear_flags", obj->wear_flags, TW_ARRAY_MAX);
    roomsave_toml_get_int_array(tab, "values", obj->values, NUM_OBJ_VAL_POSITIONS);
  }

  arr = toml_array_in(tab, "contents");
  if (arr) {
    struct roomsave_obj *tail = NULL;
    int n = toml_array_nelem(arr);
    for (i = 0; i < n; i++) {
      toml_table_t *ctab = toml_table_at(arr, i);
      struct roomsave_obj *child = roomsave_parse_obj_table(ctab, full);
      if (!child)
        continue;
      child->next = NULL;
      if (!obj->contents) {
        obj->contents = tail = child;
      } else {
        tail->next = child;
        tail = child;
      }
    }
  }

  return obj;
}

static struct roomsave_mob_item *roomsave_parse_mob_item(toml_table_t *tab, int with_wear)
{
  struct roomsave_mob_item *item;
  toml_array_t *arr;
  int i;

  if (!tab)
    return NULL;

  CREATE(item, struct roomsave_mob_item, 1);
  item->vnum = roomsave_toml_get_int_default(tab, "vnum", -1);
  item->wear_pos = with_wear ? roomsave_toml_get_int_default(tab, "wear_pos", 0) : -1;
  if (item->vnum <= 0) {
    free(item);
    return NULL;
  }

  arr = toml_array_in(tab, "contents");
  if (arr) {
    struct roomsave_obj *tail = NULL;
    int n = toml_array_nelem(arr);
    for (i = 0; i < n; i++) {
      toml_table_t *ctab = toml_table_at(arr, i);
      struct roomsave_obj *child = roomsave_parse_obj_table(ctab, 0);
      if (!child)
        continue;
      child->next = NULL;
      if (!item->contents) {
        item->contents = tail = child;
      } else {
        tail->next = child;
        tail = child;
      }
    }
  }

  return item;
}

static struct roomsave_mob *roomsave_parse_mob_table(toml_table_t *tab)
{
  struct roomsave_mob *mob;
  toml_array_t *arr;
  int i;

  if (!tab)
    return NULL;

  CREATE(mob, struct roomsave_mob, 1);
  mob->vnum = roomsave_toml_get_int_default(tab, "vnum", -1);
  if (mob->vnum <= 0) {
    free(mob);
    return NULL;
  }

  arr = toml_array_in(tab, "equipment");
  if (arr) {
    struct roomsave_mob_item *tail = NULL;
    int n = toml_array_nelem(arr);
    for (i = 0; i < n; i++) {
      toml_table_t *itab = toml_table_at(arr, i);
      struct roomsave_mob_item *item = roomsave_parse_mob_item(itab, 1);
      if (!item)
        continue;
      item->next = NULL;
      if (!mob->equipment) {
        mob->equipment = tail = item;
      } else {
        tail->next = item;
        tail = item;
      }
    }
  }

  arr = toml_array_in(tab, "inventory");
  if (arr) {
    struct roomsave_mob_item *tail = NULL;
    int n = toml_array_nelem(arr);
    for (i = 0; i < n; i++) {
      toml_table_t *itab = toml_table_at(arr, i);
      struct roomsave_mob_item *item = roomsave_parse_mob_item(itab, 0);
      if (!item)
        continue;
      item->next = NULL;
      if (!mob->inventory) {
        mob->inventory = tail = item;
      } else {
        tail->next = item;
        tail = item;
      }
    }
  }

  return mob;
}

static struct roomsave_room *roomsave_parse_room_table(toml_table_t *tab)
{
  struct roomsave_room *room;
  toml_array_t *arr;
  int i;

  if (!tab)
    return NULL;

  CREATE(room, struct roomsave_room, 1);
  room->vnum = roomsave_toml_get_int_default(tab, "vnum", -1);
  room->saved_at = (long)roomsave_toml_get_int_default(tab, "saved_at", 0);
  if (room->vnum <= 0) {
    free(room);
    return NULL;
  }

  arr = toml_array_in(tab, "object");
  if (arr) {
    struct roomsave_obj *tail = NULL;
    int n = toml_array_nelem(arr);
    for (i = 0; i < n; i++) {
      toml_table_t *otab = toml_table_at(arr, i);
      struct roomsave_obj *obj = roomsave_parse_obj_table(otab, 1);
      if (!obj)
        continue;
      obj->next = NULL;
      if (!room->objects) {
        room->objects = tail = obj;
      } else {
        tail->next = obj;
        tail = obj;
      }
    }
  }

  arr = toml_array_in(tab, "mob");
  if (arr) {
    struct roomsave_mob *tail = NULL;
    int n = toml_array_nelem(arr);
    for (i = 0; i < n; i++) {
      toml_table_t *mtab = toml_table_at(arr, i);
      struct roomsave_mob *mob = roomsave_parse_mob_table(mtab);
      if (!mob)
        continue;
      mob->next = NULL;
      if (!room->mobs) {
        room->mobs = tail = mob;
      } else {
        tail->next = mob;
        tail = mob;
      }
    }
  }

  return room;
}

static struct roomsave_room *roomsave_load_file_toml(const char *path)
{
  FILE *fp;
  toml_table_t *tab;
  toml_array_t *arr;
  char errbuf[200];
  struct roomsave_room *head = NULL, *tail = NULL;
  int i, n;

  fp = fopen(path, "r");
  if (!fp)
    return NULL;

  tab = toml_parse_file(fp, errbuf, sizeof(errbuf));
  fclose(fp);
  if (!tab) {
    mudlog(NRM, LVL_IMMORT, TRUE, "RoomSave: parsing %s: %s", path, errbuf);
    return NULL;
  }

  arr = toml_array_in(tab, "room");
  if (!arr) {
    toml_free(tab);
    return NULL;
  }

  n = toml_array_nelem(arr);
  for (i = 0; i < n; i++) {
    toml_table_t *rtab = toml_table_at(arr, i);
    struct roomsave_room *room = roomsave_parse_room_table(rtab);
    if (!room)
      continue;
    room->next = NULL;
    if (!head) {
      head = tail = room;
    } else {
      tail->next = room;
      tail = room;
    }
  }

  toml_free(tab);
  return head;
}

static void roomsave_write_inline_int_array(FILE *fp, const int *vals, int len)
{
  int i;
  fputc('[', fp);
  for (i = 0; i < len; i++) {
    if (i)
      fputs(", ", fp);
    fprintf(fp, "%d", vals[i]);
  }
  fputc(']', fp);
}

static void roomsave_write_int_array(FILE *fp, const char *key, const int *vals, int len)
{
  fprintf(fp, "%s = ", key);
  roomsave_write_inline_int_array(fp, vals, len);
  fputc('\n', fp);
}

static void roomsave_write_inline_obj(FILE *fp, struct roomsave_obj *obj)
{
  fputs("{ vnum = ", fp);
  fprintf(fp, "%d", obj->vnum);
  if (obj->full) {
    fprintf(fp, ", timer = %d, weight = %d, cost = %d, cost_per_day = %d",
            obj->timer, obj->weight, obj->cost, obj->cost_per_day);
    fputs(", extra_flags = ", fp);
    roomsave_write_inline_int_array(fp, obj->extra_flags, EF_ARRAY_MAX);
    fputs(", wear_flags = ", fp);
    roomsave_write_inline_int_array(fp, obj->wear_flags, TW_ARRAY_MAX);
    fputs(", values = ", fp);
    roomsave_write_inline_int_array(fp, obj->values, NUM_OBJ_VAL_POSITIONS);
  }
  fputs(", contents = [", fp);
  if (obj->contents) {
    struct roomsave_obj *child;
    int first = 1;
    for (child = obj->contents; child; child = child->next) {
      if (!first)
        fputs(", ", fp);
      roomsave_write_inline_obj(fp, child);
      first = 0;
    }
  }
  fputs("] }", fp);
}

static void roomsave_write_room_objects(FILE *fp, struct roomsave_obj *obj)
{
  for (; obj; obj = obj->next) {
    fprintf(fp, "\n[[room.object]]\n");
    fprintf(fp, "vnum = %d\n", obj->vnum);
    fprintf(fp, "timer = %d\n", obj->timer);
    fprintf(fp, "weight = %d\n", obj->weight);
    fprintf(fp, "cost = %d\n", obj->cost);
    fprintf(fp, "cost_per_day = %d\n", obj->cost_per_day);
    roomsave_write_int_array(fp, "extra_flags", obj->extra_flags, EF_ARRAY_MAX);
    roomsave_write_int_array(fp, "wear_flags", obj->wear_flags, TW_ARRAY_MAX);
    roomsave_write_int_array(fp, "values", obj->values, NUM_OBJ_VAL_POSITIONS);
    fputs("contents = [", fp);
    if (obj->contents) {
      struct roomsave_obj *child;
      int first = 1;
      for (child = obj->contents; child; child = child->next) {
        if (!first)
          fputs(", ", fp);
        roomsave_write_inline_obj(fp, child);
        first = 0;
      }
    }
    fputs("]\n", fp);
  }
}

static void roomsave_write_mob_items(FILE *fp, const char *table_name, struct roomsave_mob_item *item)
{
  for (; item; item = item->next) {
    fprintf(fp, "\n[[room.mob.%s]]\n", table_name);
    if (!strcmp(table_name, "equipment"))
      fprintf(fp, "wear_pos = %d\n", item->wear_pos);
    fprintf(fp, "vnum = %d\n", item->vnum);
    fputs("contents = [", fp);
    if (item->contents) {
      struct roomsave_obj *child;
      int first = 1;
      for (child = item->contents; child; child = child->next) {
        if (!first)
          fputs(", ", fp);
        roomsave_write_inline_obj(fp, child);
        first = 0;
      }
    }
    fputs("]\n", fp);
  }
}

static void roomsave_write_rooms(FILE *fp, struct roomsave_room *room)
{
  if (!room) {
    fputs("room = []\n", fp);
    return;
  }

  for (; room; room = room->next) {
    fprintf(fp, "[[room]]\n");
    fprintf(fp, "vnum = %d\n", room->vnum);
    fprintf(fp, "saved_at = %ld\n", room->saved_at);
    roomsave_write_room_objects(fp, room->objects);
    if (room->mobs) {
      struct roomsave_mob *mob;
      for (mob = room->mobs; mob; mob = mob->next) {
        fprintf(fp, "\n[[room.mob]]\n");
        fprintf(fp, "vnum = %d\n", mob->vnum);
        roomsave_write_mob_items(fp, "equipment", mob->equipment);
        roomsave_write_mob_items(fp, "inventory", mob->inventory);
      }
    }
    fputs("\n", fp);
  }
}

static void roomsave_apply_obj_fields(struct obj_data *obj, struct roomsave_obj *rs)
{
  int i;

  if (!obj || !rs || !rs->full)
    return;

  GET_OBJ_TIMER(obj) = rs->timer;
  GET_OBJ_WEIGHT(obj) = rs->weight;
  GET_OBJ_COST(obj) = rs->cost;
  GET_OBJ_COST_PER_DAY(obj) = rs->cost_per_day;

#if defined(EF_ARRAY_MAX) && defined(GET_OBJ_EXTRA_AR)
  for (i = 0; i < EF_ARRAY_MAX; i++)
    GET_OBJ_EXTRA_AR(obj, i) = rs->extra_flags[i];
#elif defined(EF_ARRAY_MAX)
  for (i = 0; i < EF_ARRAY_MAX; i++)
    GET_OBJ_EXTRA(obj)[i] = rs->extra_flags[i];
#else
  GET_OBJ_EXTRA(obj) = rs->extra_flags[0];
#endif

#ifdef TW_ARRAY_MAX
  for (i = 0; i < TW_ARRAY_MAX; i++)
    GET_OBJ_WEAR(obj)[i] = rs->wear_flags[i];
#else
  GET_OBJ_WEAR(obj) = rs->wear_flags[0];
#endif

#ifdef NUM_OBJ_VAL_POSITIONS
  for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
    GET_OBJ_VAL(obj, i) = rs->values[i];
#else
  for (i = 0; i < 6; i++)
    GET_OBJ_VAL(obj, i) = rs->values[i];
#endif
}

static void roomsave_restore_obj_contents(struct obj_data *parent, struct roomsave_obj *list)
{
  struct roomsave_obj *it;

  if (!parent)
    return;

  for (it = list; it; it = it->next) {
    struct obj_data *obj = RS_create_obj_by_vnum((obj_vnum)it->vnum);
    if (!obj)
      continue;
    roomsave_apply_obj_fields(obj, it);
    obj_to_obj(obj, parent);
    if (it->contents)
      roomsave_restore_obj_contents(obj, it->contents);
    if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
      update_money_obj(obj);
  }
}

static int roomsave_restore_room_objects(struct roomsave_room *room, room_rnum rnum)
{
  struct roomsave_obj *it;
  int count = 0;

  for (it = room->objects; it; it = it->next) {
    struct obj_data *obj = RS_create_obj_by_vnum((obj_vnum)it->vnum);
    if (!obj)
      continue;
    roomsave_apply_obj_fields(obj, it);
    if (it->contents)
      roomsave_restore_obj_contents(obj, it->contents);
    obj_to_room(obj, rnum);
    if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
      update_money_obj(obj);
    count++;
  }

  return count;
}

static void roomsave_restore_mob_items(struct char_data *mob, struct roomsave_mob_item *items, int is_equipment)
{
  struct roomsave_mob_item *it;

  for (it = items; it; it = it->next) {
    struct obj_data *obj = RS_create_obj_by_vnum((obj_vnum)it->vnum);
    if (!obj)
      continue;

    if (is_equipment) {
      int pos = it->wear_pos;
      if (pos < 0 || pos >= NUM_WEARS)
        pos = WEAR_HOLD;
      equip_char(mob, obj, pos);
    } else {
      obj_to_char(obj, mob);
    }

    if (it->contents)
      roomsave_restore_obj_contents(obj, it->contents);
  }
}

static int roomsave_restore_room_mobs(struct roomsave_room *room, room_rnum rnum)
{
  struct roomsave_mob *it;
  int count = 0;

  for (it = room->mobs; it; it = it->next) {
    struct char_data *mob = RS_create_mob_by_vnum((mob_vnum)it->vnum);
    int saw_inventory = 0;

    if (!mob)
      continue;

    char_to_room(mob, rnum);
    if (IN_ROOM(mob) != rnum)
      char_to_room(mob, rnum);

    roomsave_restore_mob_items(mob, it->equipment, 1);
    roomsave_restore_mob_items(mob, it->inventory, 0);

    if (it->inventory)
      saw_inventory = 1;

    if (!saw_inventory)
      RS_apply_inventory_loadout(mob);

    load_mtrigger(mob);

    count++;
  }

  return count;
}

static struct roomsave_obj *roomsave_build_obj(struct obj_data *obj, int full)
{
  struct roomsave_obj *rs;
  int i;

  if (!obj || GET_OBJ_VNUM(obj) <= 0)
    return NULL;

  CREATE(rs, struct roomsave_obj, 1);
  rs->vnum = (int)GET_OBJ_VNUM(obj);
  rs->full = full;

  if (full) {
    rs->timer = GET_OBJ_TIMER(obj);
    rs->weight = GET_OBJ_WEIGHT(obj);
    rs->cost = GET_OBJ_COST(obj);
    rs->cost_per_day = GET_OBJ_COST_PER_DAY(obj);
#if defined(EF_ARRAY_MAX) && defined(GET_OBJ_EXTRA_AR)
    for (i = 0; i < EF_ARRAY_MAX; i++)
      rs->extra_flags[i] = GET_OBJ_EXTRA_AR(obj, i);
#elif defined(EF_ARRAY_MAX)
    for (i = 0; i < EF_ARRAY_MAX; i++)
      rs->extra_flags[i] = GET_OBJ_EXTRA(obj)[i];
#else
    rs->extra_flags[0] = GET_OBJ_EXTRA(obj);
#endif
#ifdef TW_ARRAY_MAX
    for (i = 0; i < TW_ARRAY_MAX; i++)
      rs->wear_flags[i] = GET_OBJ_WEAR(obj)[i];
#else
    rs->wear_flags[0] = GET_OBJ_WEAR(obj);
#endif
#ifdef NUM_OBJ_VAL_POSITIONS
    for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
      rs->values[i] = GET_OBJ_VAL(obj, i);
#else
    for (i = 0; i < 6; i++)
      rs->values[i] = GET_OBJ_VAL(obj, i);
#endif
  }

  if (obj->contains) {
    struct roomsave_obj *tail = NULL;
    struct obj_data *child;
    for (child = obj->contains; child; child = child->next_content) {
      struct roomsave_obj *child_rs = roomsave_build_obj(child, full);
      if (!child_rs)
        continue;
      child_rs->next = NULL;
      if (!rs->contents) {
        rs->contents = tail = child_rs;
      } else {
        tail->next = child_rs;
        tail = child_rs;
      }
    }
  }

  return rs;
}

static struct roomsave_mob_item *roomsave_build_mob_item(struct obj_data *obj, int wear_pos)
{
  struct roomsave_mob_item *item;
  struct obj_data *child;
  struct roomsave_obj *tail = NULL;

  if (!obj || GET_OBJ_VNUM(obj) <= 0)
    return NULL;

  CREATE(item, struct roomsave_mob_item, 1);
  item->vnum = (int)GET_OBJ_VNUM(obj);
  item->wear_pos = wear_pos;

  for (child = obj->contains; child; child = child->next_content) {
    struct roomsave_obj *child_rs = roomsave_build_obj(child, 0);
    if (!child_rs)
      continue;
    child_rs->next = NULL;
    if (!item->contents) {
      item->contents = tail = child_rs;
    } else {
      tail->next = child_rs;
      tail = child_rs;
    }
  }

  return item;
}

static struct roomsave_mob *roomsave_build_mob(struct char_data *mob)
{
  struct roomsave_mob *rs;
  int w;
  struct obj_data *obj;
  struct roomsave_mob_item *tail;

  if (!mob || !IS_NPC(mob) || GET_MOB_VNUM(mob) <= 0)
    return NULL;

  CREATE(rs, struct roomsave_mob, 1);
  rs->vnum = (int)GET_MOB_VNUM(mob);

  tail = NULL;
  for (w = 0; w < NUM_WEARS; w++) {
    obj = GET_EQ(mob, w);
    if (!obj)
      continue;
    {
      struct roomsave_mob_item *item = roomsave_build_mob_item(obj, w);
      if (!item)
        continue;
      item->next = NULL;
      if (!rs->equipment) {
        rs->equipment = tail = item;
      } else {
        tail->next = item;
        tail = item;
      }
    }
  }

  tail = NULL;
  for (obj = mob->carrying; obj; obj = obj->next_content) {
    struct roomsave_mob_item *item = roomsave_build_mob_item(obj, -1);
    if (!item)
      continue;
    item->next = NULL;
    if (!rs->inventory) {
      rs->inventory = tail = item;
    } else {
      tail->next = item;
      tail = item;
    }
  }

  return rs;
}

static struct roomsave_room *roomsave_build_room(room_rnum rnum)
{
  struct roomsave_room *room;
  struct roomsave_obj *tail_obj = NULL;
  struct roomsave_mob *tail_mob = NULL;
  struct obj_data *obj;
  struct char_data *mob;

  if (rnum == NOWHERE)
    return NULL;

  CREATE(room, struct roomsave_room, 1);
  room->vnum = world[rnum].number;
  room->saved_at = time(0);

  for (obj = world[rnum].contents; obj; obj = obj->next_content) {
    struct roomsave_obj *rs = roomsave_build_obj(obj, 1);
    if (!rs)
      continue;
    rs->next = NULL;
    if (!room->objects) {
      room->objects = tail_obj = rs;
    } else {
      tail_obj->next = rs;
      tail_obj = rs;
    }
  }

  for (mob = world[rnum].people; mob; mob = mob->next_in_room) {
    struct roomsave_mob *rs = roomsave_build_mob(mob);
    if (!rs)
      continue;
    rs->next = NULL;
    if (!room->mobs) {
      room->mobs = tail_mob = rs;
    } else {
      tail_mob->next = rs;
      tail_mob = rs;
    }
  }

  return room;
}

static void ensure_dir_exists(const char *path) {
  if (mkdir(path, 0775) == -1 && errno != EEXIST) {
    mudlog(CMP, LVL_IMMORT, TRUE, "SYSERR: roomsave mkdir(%s): %s", path, strerror(errno));
  }
}

/* zone vnum for a given room rnum (e.g., 134 -> zone 1) */
static int roomsave_zone_for_rnum(room_rnum rnum) {
  if (rnum == NOWHERE || rnum < 0 || rnum > top_of_world) return 0;
  zone_rnum znum = world[rnum].zone;
  if (znum < 0 || znum > top_of_zone_table) return 0;
  return zone_table[znum].number; /* e.g., 1 for rooms 100–199, 2 for 200–299, etc. */
}

/* lib/world/rsv/<zone>.toml */
static void roomsave_zone_filename(int zone_vnum, char *out, size_t outsz) {
  snprintf(out, outsz, "%s%d%s", ROOMSAVE_PREFIX, zone_vnum, ROOMSAVE_EXT);
}

/* Public: write the entire room’s contents */
int RoomSave_now(room_rnum rnum) {
  char path[PATH_MAX], tmp[PATH_MAX];
  FILE *out = NULL;
  struct roomsave_room *rooms = NULL, *it = NULL, *prev = NULL;
  struct roomsave_room *new_room = NULL;
  room_vnum rvnum;
  int zvnum;

  if (rnum == NOWHERE)
    return 0;

  rvnum = world[rnum].number;
  zvnum = roomsave_zone_for_rnum(rnum);
  if (zvnum < 0)
    return 0;

  ensure_dir_exists(ROOMSAVE_PREFIX);
  roomsave_zone_filename(zvnum, path, sizeof(path));

  {
    int n = snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    if (n < 0 || n >= (int)sizeof(tmp)) {
      mudlog(NRM, LVL_IMMORT, TRUE,
             "SYSERR: RoomSave: temp path too long for %s", path);
      return 0;
    }
  }

  rooms = roomsave_load_file_toml(path);
  new_room = roomsave_build_room(rnum);
  if (!new_room)
    return 0;

  for (it = rooms; it; it = it->next) {
    if (it->vnum == (int)rvnum)
      break;
    prev = it;
  }

  if (it) {
    roomsave_free_obj_list(it->objects);
    roomsave_free_mobs(it->mobs);
    it->saved_at = new_room->saved_at;
    it->objects = new_room->objects;
    it->mobs = new_room->mobs;
    free(new_room);
  } else {
    if (!rooms) {
      rooms = new_room;
    } else if (prev) {
      prev->next = new_room;
    }
  }

  if (!(out = fopen(tmp, "w"))) {
    mudlog(NRM, LVL_IMMORT, TRUE,
           "SYSERR: RoomSave: fopen(%s) failed: %s",
           tmp, strerror(errno));
    roomsave_free_rooms(rooms);
    return 0;
  }

  roomsave_write_rooms(out, rooms);

  if (fclose(out) != 0) {
    mudlog(NRM, LVL_IMMORT, TRUE,
           "SYSERR: RoomSave: fclose(%s) failed: %s",
           tmp, strerror(errno));
    roomsave_free_rooms(rooms);
    return 0;
  }
  if (rename(tmp, path) != 0) {
    mudlog(NRM, LVL_IMMORT, TRUE,
           "SYSERR: RoomSave: rename(%s -> %s) failed: %s",
           tmp, path, strerror(errno));
    roomsave_free_rooms(rooms);
    return 0;
  }

  roomsave_free_rooms(rooms);
  return 1;
}

static struct obj_data *RS_create_obj_by_vnum(obj_vnum ov) {
  obj_rnum ornum;
  if (ov <= 0) return NULL;
  ornum = real_object(ov);
  if (ornum == NOTHING) return NULL;
  return read_object(ornum, REAL);
}

static struct char_data *RS_create_mob_by_vnum(mob_vnum mv) {
  mob_rnum mrnum;
  if (mv <= 0) return NULL;
  mrnum = real_mobile(mv);
  if (mrnum == NOBODY) return NULL;
  return read_mobile(mrnum, REAL);
}

static void RS_apply_inventory_loadout(struct char_data *mob) {
  mob_rnum rnum;
  const struct mob_loadout *e;
  struct obj_data *stack[16];
  int i;

  if (!mob || !IS_NPC(mob)) return;
  rnum = GET_MOB_RNUM(mob);
  if (rnum < 0) return;

  for (i = 0; i < (int)(sizeof(stack) / sizeof(stack[0])); i++)
    stack[i] = NULL;

  for (e = mob_proto[rnum].proto_loadout; e; e = e->next) {
    int qty, n;
    if (e->wear_pos >= 0)
      continue;
    qty = (e->quantity > 0) ? e->quantity : 1;
    for (n = 0; n < qty; n++) {
      struct obj_data *obj = RS_create_obj_by_vnum(e->vnum);
      if (!obj) {
        log("SYSERR: RS_apply_inventory_loadout: bad obj vnum %d on mob %d",
            e->vnum, GET_MOB_VNUM(mob));
        continue;
      }
      if (e->wear_pos == -1) {
        for (i = 0; i < (int)(sizeof(stack) / sizeof(stack[0])); i++)
          stack[i] = NULL;
        obj_to_char(obj, mob);
        if (obj_is_storage(obj) || GET_OBJ_TYPE(obj) == ITEM_FURNITURE)
          stack[0] = obj;
        continue;
      }

      {
        int depth = -(e->wear_pos) - 1;
        if (depth <= 0 ||
            depth >= (int)(sizeof(stack) / sizeof(stack[0])) ||
            !stack[depth - 1]) {
          obj_to_char(obj, mob);
          continue;
        }
        obj_to_obj(obj, stack[depth - 1]);
        if (obj_is_storage(obj) || GET_OBJ_TYPE(obj) == ITEM_FURNITURE) {
          stack[depth] = obj;
          for (i = depth + 1; i < (int)(sizeof(stack) / sizeof(stack[0])); i++)
            stack[i] = NULL;
        }
      }
    }
  }
}

/* Optional autosave hook (invoked by limits.c:point_update). */
void RoomSave_autosave_tick(void) {
  /* Iterate all rooms; only save flagged ones. */
  for (room_rnum rnum = 0; rnum <= top_of_world; ++rnum) {
    if (ROOM_FLAGGED(rnum, ROOM_SAVE))
      RoomSave_now(rnum);
  }
}

void RoomSave_boot(void)
{
  DIR *dirp;
  struct dirent *dp;

  ensure_dir_exists(ROOMSAVE_PREFIX);

  dirp = opendir(ROOMSAVE_PREFIX);
  if (!dirp) {
    mudlog(NRM, LVL_IMMORT, TRUE,
           "SYSERR: RoomSave_boot: cannot open %s", ROOMSAVE_PREFIX);
    return;
  }

  log("RoomSave: scanning %s for *.toml", ROOMSAVE_PREFIX);

  while ((dp = readdir(dirp))) {
    size_t n = strlen(dp->d_name);
    size_t extlen = strlen(ROOMSAVE_EXT);
    if (n <= extlen) continue; /* skip . and .. */
    if (strcmp(dp->d_name + n - extlen, ROOMSAVE_EXT) != 0) continue;

    {
      char path[PATH_MAX];
      struct roomsave_room *rooms;
      struct roomsave_room *room;
      int wn = snprintf(path, sizeof(path), "%s%s", ROOMSAVE_PREFIX, dp->d_name);
      if (wn < 0 || wn >= (int)sizeof(path)) {
        mudlog(NRM, LVL_IMMORT, TRUE,
               "SYSERR: RoomSave_boot: path too long: %s%s",
               ROOMSAVE_PREFIX, dp->d_name);
        continue;
      }

      log("RoomSave: reading %s", path);

      rooms = roomsave_load_file_toml(path);
      if (!rooms)
        continue;

      int blocks = 0;
      int restored_objs_total = 0;
      int restored_mobs_total = 0;

      for (room = rooms; room; room = room->next) {
        room_rnum rnum = real_room((room_vnum)room->vnum);
        int count_objs = 0;
        int count_mobs = 0;

        blocks++;

        if (rnum == NOWHERE) {
          mudlog(NRM, LVL_IMMORT, FALSE,
                 "RoomSave: unknown room vnum %d in %s (skipping)",
                 room->vnum, path);
          continue;
        }

        while (world[rnum].contents)
          extract_obj(world[rnum].contents);

        count_objs = roomsave_restore_room_objects(room, rnum);
        count_mobs = roomsave_restore_room_mobs(room, rnum);

        restored_objs_total += count_objs;
        restored_mobs_total += count_mobs;

        if (count_mobs > 0)
          log("RoomSave: room %d <- %d object(s) and %d mob(s)",
              room->vnum, count_objs, count_mobs);
        else
          log("RoomSave: room %d <- %d object(s)", room->vnum, count_objs);
      }

      log("RoomSave: finished %s (blocks=%d, objects=%d, mobs=%d)",
          path, blocks, restored_objs_total, restored_mobs_total);
      roomsave_free_rooms(rooms);
    }
  }

  closedir(dirp);
}

/* ======== MOB SAVE: write NPCs and their equipment/inventory ========== */
