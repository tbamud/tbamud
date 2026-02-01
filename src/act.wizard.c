/**************************************************************************
*  File: act.wizard.c                                      Part of tbaMUD *
*  Usage: Player-level god commands and other goodies.                    *
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
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "screen.h"
#include "constants.h"
#include "oasis.h"
#include "py_triggers.h"
#include "shop.h"
#include "act.h"
#include "genzon.h" /* for real_zone_by_thing */
#include "class.h"
#include "species.h"
#include "genmob.h"
#include "genolc.h"
#include "genobj.h"
#include "genshp.h"
#include "fight.h"
#include "house.h"
#include "modify.h"
#include "quest.h"
#include "ban.h"
#include "screen.h"

/* local utility functions with file scope */
static int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg);
static void perform_immort_invis(struct char_data *ch, int level);
static void do_stat_room(struct char_data *ch, struct room_data *rm);
static void do_stat_object(struct char_data *ch, struct obj_data *j);
static void do_stat_character(struct char_data *ch, struct char_data *k);
static void do_stat_shop(struct char_data *ch, shop_rnum shop_nr);
static void stop_snooping(struct char_data *ch);
static struct obj_data *find_inventory_coin(struct char_data *ch);
static void remove_other_coins_from_list(struct obj_data *list, struct obj_data *keep);
static size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone, int listall);
static struct char_data *is_in_game(long idnum);
static void mob_checkload(struct char_data *ch, mob_vnum mvnum);
static void obj_checkload(struct char_data *ch, obj_vnum ovnum);
static void trg_checkload(struct char_data *ch, trig_vnum tvnum);
static void mod_llog_entry(struct last_entry *llast,int type);
static int  get_max_recent(void);
static void clear_recent(struct recent_player *this);
static struct recent_player *create_recent(void);
const char *get_spec_func_name(SPECIAL(*func));
bool zedit_get_levels(struct descriptor_data *d, char *buf);

/* Modern stat table helpers */
#define STAT_LABEL_WIDTH 18
#define STAT_VALUE_WIDTH 55

static const char *stat_border_line(void);
static void stat_table_border(struct char_data *ch);
static void stat_table_row(struct char_data *ch, const char *label, const char *value);
static void stat_table_row_fmt(struct char_data *ch, const char *label, const char *fmt, ...) __attribute__((format(printf,3,4)));
static void stat_table_border_buf(char *buf, size_t buf_size, size_t *len);
static void stat_table_row_buf(char *buf, size_t buf_size, size_t *len, const char *label, const char *value);
static void stat_table_row_fmt_buf(char *buf, size_t buf_size, size_t *len, const char *label, const char *fmt, ...) __attribute__((format(printf,5,6)));
static void stat_appendf(char *buf, size_t buf_size, size_t *len, const char *fmt, ...) __attribute__((format(printf,4,5)));
static void stat_append_list_item(char *buf, size_t buf_size, size_t *len, size_t *line_len, const char *item);
static void stat_format_notrade_classes(bitvector_t flags, char *out, size_t outsz);
static void stat_format_script_triggers(struct script_data *sc, char *buf, size_t buf_size);
static void stat_format_script_globals(struct script_data *sc, char *buf, size_t buf_size);
static void stat_format_script_memory(struct script_memory *mem, char *buf, size_t buf_size);
static void stat_format_character_list(struct char_data *viewer, struct char_data *list, char *buf, size_t buf_size);
static void stat_format_object_list(struct char_data *viewer, struct obj_data *list, char *buf, size_t buf_size);
static void stat_format_exit_summary(struct room_data *rm, char *buf, size_t buf_size);
static void stat_format_char_effects(struct char_data *k, char *buf, size_t buf_size);
static void stat_format_obj_affects(struct obj_data *j, char *buf, size_t buf_size);
static void stat_format_obj_special(struct obj_data *j, char *buf, size_t buf_size);
static void stat_format_zone_cmds_room(room_vnum rvnum, char *buf, size_t buf_size);

/* Local Globals */
static struct recent_player *recent_list = NULL;  /** Global list of recent players */

static const char *stat_border_line(void)
{
  static char border[STAT_LABEL_WIDTH + STAT_VALUE_WIDTH + 16];
  if (!*border) {
    size_t pos = 0;
    border[pos++] = '+';
    for (int i = 0; i < STAT_LABEL_WIDTH + 2 && pos < sizeof(border) - 1; i++)
      border[pos++] = '-';
    border[pos++] = '+';
    for (int i = 0; i < STAT_VALUE_WIDTH + 2 && pos < sizeof(border) - 1; i++)
      border[pos++] = '-';
    border[pos++] = '+';
    border[pos++] = '\r';
    border[pos++] = '\n';
    border[pos] = '\0';
  }
  return border;
}

static void stat_table_border(struct char_data *ch)
{
  send_to_char(ch, "%s", stat_border_line());
}

static const char *stat_next_chunk(const char *text, char *chunk, size_t width)
{
  const char *ptr = text;
  size_t chunk_len = 0;

  chunk[0] = '\0';

  while (*ptr == ' ')
    ptr++;

  while (*ptr && *ptr != '\n' && *ptr != '\r') {
    const char *word_start = ptr;
    size_t word_len = 0;

    while (word_start[word_len] &&
           word_start[word_len] != ' ' &&
           word_start[word_len] != '\n' &&
           word_start[word_len] != '\r')
      word_len++;

    if (word_len == 0)
      break;

    if (chunk_len == 0 && word_len > width) {
      word_len = width;
      memcpy(chunk, word_start, word_len);
      chunk_len = word_len;
      chunk[chunk_len] = '\0';
      ptr = word_start + word_len;
      return ptr;
    }

    size_t needed = word_len + (chunk_len ? 1 : 0);
    if (chunk_len + needed > width)
      break;

    if (chunk_len)
      chunk[chunk_len++] = ' ';
    memcpy(chunk + chunk_len, word_start, word_len);
    chunk_len += word_len;
    chunk[chunk_len] = '\0';

    ptr = word_start + word_len;

    while (*ptr == ' ')
      ptr++;
  }

  while (*ptr == '\r' || *ptr == '\n')
    ptr++;

  return ptr;
}

static void stat_table_row(struct char_data *ch, const char *label, const char *value)
{
  const char *text = (value && *value) ? value : "<None>";
  bool printed = FALSE;

  while (*text) {
    while (*text == '\r' || *text == '\n')
      text++;
    if (!*text)
      break;

    char chunk[STAT_VALUE_WIDTH + 1];
    const char *next = stat_next_chunk(text, chunk, STAT_VALUE_WIDTH);

    if (!*chunk) {
      text = next;
      continue;
    }

    send_to_char(ch, "| %-*s | %-*s |\r\n",
      STAT_LABEL_WIDTH,
      printed || !label ? "" : label,
      STAT_VALUE_WIDTH,
      chunk);

    printed = TRUE;
    text = next;
  }

  if (!printed)
    send_to_char(ch, "| %-*s | %-*s |\r\n",
      STAT_LABEL_WIDTH,
      label ? label : "",
      STAT_VALUE_WIDTH,
      (value && *value) ? value : "<None>");
}

static void stat_table_row_fmt(struct char_data *ch, const char *label, const char *fmt, ...)
{
  char buf[MAX_STRING_LENGTH];
  va_list args;

  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  stat_table_row(ch, label, buf);
}

static void stat_table_border_buf(char *buf, size_t buf_size, size_t *len)
{
  stat_appendf(buf, buf_size, len, "%s", stat_border_line());
}

static void stat_table_row_buf(char *buf, size_t buf_size, size_t *len, const char *label, const char *value)
{
  const char *text = (value && *value) ? value : "<None>";
  bool printed = FALSE;

  while (*text) {
    while (*text == '\r' || *text == '\n')
      text++;
    if (!*text)
      break;

    char chunk[STAT_VALUE_WIDTH + 1];
    const char *next = stat_next_chunk(text, chunk, STAT_VALUE_WIDTH);

    if (!*chunk) {
      text = next;
      continue;
    }

    stat_appendf(buf, buf_size, len, "| %-*s | %-*s |\r\n",
      STAT_LABEL_WIDTH,
      printed || !label ? "" : label,
      STAT_VALUE_WIDTH,
      chunk);

    printed = TRUE;
    text = next;
  }

  if (!printed)
    stat_appendf(buf, buf_size, len, "| %-*s | %-*s |\r\n",
      STAT_LABEL_WIDTH,
      label ? label : "",
      STAT_VALUE_WIDTH,
      (value && *value) ? value : "<None>");
}

static void stat_table_row_fmt_buf(char *buf, size_t buf_size, size_t *len, const char *label, const char *fmt, ...)
{
  char row[MAX_STRING_LENGTH];
  va_list args;

  va_start(args, fmt);
  vsnprintf(row, sizeof(row), fmt, args);
  va_end(args);

  stat_table_row_buf(buf, buf_size, len, label, row);
}

static void stat_appendf(char *buf, size_t buf_size, size_t *len, const char *fmt, ...)
{
  if (*len >= buf_size)
    return;

  va_list args;
  va_start(args, fmt);
  int wrote = vsnprintf(buf + *len, buf_size - *len, fmt, args);
  va_end(args);

  if (wrote < 0)
    return;

  if ((size_t)wrote >= buf_size - *len)
    *len = buf_size - 1;
  else
    *len += wrote;
}

static void stat_append_list_item(char *buf, size_t buf_size, size_t *len, size_t *line_len, const char *item)
{
  size_t item_len = strlen(item);

  if (*line_len && *line_len + 2 + item_len > STAT_VALUE_WIDTH) {
    stat_appendf(buf, buf_size, len, "\n");
    *line_len = 0;
  }

  if (*line_len) {
    stat_appendf(buf, buf_size, len, ", ");
    *line_len += 2;
  }

  stat_appendf(buf, buf_size, len, "%s", item);
  *line_len += item_len;
}

static void stat_format_notrade_classes(bitvector_t flags, char *out, size_t outsz)
{
  size_t len = 0;
  int i, found = 0;

  if (!out || outsz == 0)
    return;

  out[0] = '\0';

  for (i = TRADE_CLASS_START; i < NUM_TRADERS; i++) {
    if (IS_SET(flags, 1 << i)) {
      int n = snprintf(out + len, outsz - len, "%s%s", found ? " " : "", trade_letters[i]);

      if (n < 0 || (size_t)n >= outsz - len) {
        out[outsz - 1] = '\0';
        return;
      }

      len += (size_t)n;
      found = 1;
    }
  }

  if (!found)
    strlcpy(out, "NOBITS", outsz);
}

static void stat_format_script_triggers(struct script_data *sc, char *buf, size_t buf_size)
{
  size_t len = 0;
  buf[0] = '\0';

  if (!sc || !TRIGGERS(sc)) {
    strlcpy(buf, "None", buf_size);
    return;
  }

  for (trig_data *t = TRIGGERS(sc); t; t = t->next) {
    const char *attach = (t->attach_type == OBJ_TRIGGER) ? "obj" :
                         (t->attach_type == WLD_TRIGGER) ? "room" : "mob";
    const char **type_names = (t->attach_type == OBJ_TRIGGER) ? otrig_types :
                              (t->attach_type == WLD_TRIGGER) ? wtrig_types : trig_types;
    char type_buf[MAX_STRING_LENGTH];
    sprintbit(GET_TRIG_TYPE(t), type_names, type_buf, sizeof(type_buf));

    if (len)
      stat_appendf(buf, buf_size, &len, "\n");

    stat_appendf(buf, buf_size, &len, "#%d %s (%s %s) narg=%d arg=%s script=%s",
      GET_TRIG_VNUM(t),
      GET_TRIG_NAME(t),
      attach,
      type_buf,
      GET_TRIG_NARG(t),
      (GET_TRIG_ARG(t) && *GET_TRIG_ARG(t)) ? GET_TRIG_ARG(t) : "None",
      (t->script && *t->script) ? t->script : "None");

    if (GET_TRIG_WAIT(t))
      stat_appendf(buf, buf_size, &len, " [wait]");
  }
}

static void stat_format_script_globals(struct script_data *sc, char *buf, size_t buf_size)
{
  size_t len = 0;
  buf[0] = '\0';

  if (!sc || !sc->global_vars) {
    strlcpy(buf, "None", buf_size);
    return;
  }

  for (struct trig_var_data *tv = sc->global_vars; tv; tv = tv->next) {
    char value[MAX_INPUT_LENGTH];

    if (*(tv->value) == UID_CHAR)
      find_uid_name(tv->value, value, sizeof(value));
    else
      strlcpy(value, tv->value, sizeof(value));

    if (len)
      stat_appendf(buf, buf_size, &len, "\n");

    if (tv->context)
      stat_appendf(buf, buf_size, &len, "%s:%ld = %s", tv->name, tv->context, value);
    else
      stat_appendf(buf, buf_size, &len, "%s = %s", tv->name, value);
  }
}

static void stat_format_script_memory(struct script_memory *mem, char *buf, size_t buf_size)
{
  size_t len = 0;
  buf[0] = '\0';

  if (!mem) {
    strlcpy(buf, "None", buf_size);
    return;
  }

  for (struct script_memory *cur = mem; cur; cur = cur->next) {
    struct char_data *mc = find_char(cur->id);
    const char *name = mc ? GET_NAME(mc) : "**Corrupted**";

    if (len)
      stat_appendf(buf, buf_size, &len, "\n");

    stat_appendf(buf, buf_size, &len, "%-15s %s",
      name,
      cur->cmd ? cur->cmd : "<default>");
  }
}

static void stat_format_character_list(struct char_data *viewer, struct char_data *list, char *buf, size_t buf_size)
{
  size_t len = 0;
  buf[0] = '\0';

  for (struct char_data *k = list; k; k = k->next_in_room) {
    if (viewer && !CAN_SEE(viewer, k))
      continue;

    if (len)
      stat_appendf(buf, buf_size, &len, ", ");

    stat_appendf(buf, buf_size, &len, "%s (%s)",
      GET_NAME(k),
      !IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB"));
  }

  if (!len)
    strlcpy(buf, "None", buf_size);
}

static void stat_format_object_list(struct char_data *viewer, struct obj_data *list, char *buf, size_t buf_size)
{
  size_t len = 0;
  buf[0] = '\0';

  for (struct obj_data *j = list; j; j = j->next_content) {
    if (viewer && !CAN_SEE_OBJ(viewer, j))
      continue;

    if (len)
      stat_appendf(buf, buf_size, &len, ", ");

    stat_appendf(buf, buf_size, &len, "%s", j->short_description ? j->short_description : "<Unnamed>");
  }

  if (!len)
    strlcpy(buf, "None", buf_size);
}

static void stat_format_exit_summary(struct room_data *rm, char *buf, size_t buf_size)
{
  size_t len = 0;
  buf[0] = '\0';

  for (int i = 0; i < DIR_COUNT; i++) {
    if (!rm->dir_option[i])
      continue;

    const char *dir = dirs[i];
    char exit_buf[MAX_STRING_LENGTH];
    sprintbit(rm->dir_option[i]->exit_info, exit_bits, exit_buf, sizeof(exit_buf));

    room_rnum to = rm->dir_option[i]->to_room;
    if (len)
      stat_appendf(buf, buf_size, &len, "\n");

    if (to == NOWHERE)
      stat_appendf(buf, buf_size, &len, "%s -> NOWHERE (flags: %s)", dir, exit_buf);
    else
      stat_appendf(buf, buf_size, &len, "%s -> #%d %s (flags: %s, key: %d, keywords: %s)",
        dir,
        GET_ROOM_VNUM(to),
        world[to].name,
        exit_buf,
        rm->dir_option[i]->key == NOTHING ? -1 : rm->dir_option[i]->key,
        rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "None");
  }

  if (!len)
    strlcpy(buf, "None", buf_size);
}

static void stat_format_char_effects(struct char_data *k, char *buf, size_t buf_size)
{
  size_t len = 0;
  buf[0] = '\0';

  for (struct affected_type *aff = k->affected; aff; aff = aff->next) {
    char flags[MAX_STRING_LENGTH];
    bool has_flags = FALSE;

    for (int i = 0; i < NUM_AFF_FLAGS; i++) {
      if (IS_SET_AR(aff->bitvector, i)) {
        if (!has_flags) {
          flags[0] = '\0';
          has_flags = TRUE;
        }
        size_t f_len = strlen(flags);
        snprintf(flags + f_len, sizeof(flags) - f_len, "%s%s",
          f_len ? " " : "",
          affected_bits[i]);
      }
    }

    if (len)
      stat_appendf(buf, buf_size, &len, "\n");

    const char *dur_unit = "hr";
    int display_dur = aff->duration + 1;

    if (aff->spell == SKILL_PERCEPTION ||
        aff->spell == SPELL_SCAN_AFFECT ||
        aff->spell == SPELL_LISTEN_AFFECT) {
      /* convert mud-hours -> real minutes (round up) */
      int total_seconds = display_dur * SECS_PER_MUD_HOUR;
      display_dur = (total_seconds + SECS_PER_REAL_MIN - 1) / SECS_PER_REAL_MIN;
      dur_unit = "min";
    }

    stat_appendf(buf, buf_size, &len, "%s (%d%s)",
      skill_name(aff->spell),
      display_dur,
      dur_unit);

    if (aff->modifier)
      stat_appendf(buf, buf_size, &len, " %+d %s",
        aff->modifier,
        apply_types[(int)aff->location]);

    if (has_flags)
      stat_appendf(buf, buf_size, &len, " [sets %s]", flags);
  }

  if (!len)
    strlcpy(buf, "None", buf_size);
}

static void stat_format_obj_affects(struct obj_data *j, char *buf, size_t buf_size)
{
  size_t len = 0;
  buf[0] = '\0';

  for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
    if (!j->affected[i].modifier)
      continue;

    if (len)
      stat_appendf(buf, buf_size, &len, "\n");

    stat_appendf(buf, buf_size, &len, "%+d %s",
      j->affected[i].modifier,
      apply_types[j->affected[i].location]);
  }

  if (!len)
    strlcpy(buf, "None", buf_size);
}

static void stat_format_obj_special(struct obj_data *j, char *buf, size_t buf_size)
{
  size_t len = 0;
  buf[0] = '\0';
  struct char_data *tempch;

  switch (GET_OBJ_TYPE(j)) {
  case ITEM_LIGHT:
    if (GET_OBJ_VAL(j, 2) == -1)
      stat_appendf(buf, buf_size, &len, "Infinite light");
    else
      stat_appendf(buf, buf_size, &len, "Hours left: %d", GET_OBJ_VAL(j, 2));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    stat_appendf(buf, buf_size, &len, "Level %d spells: %s, %s, %s",
      GET_OBJ_VAL(j, 0),
      skill_name(GET_OBJ_VAL(j, 1)),
      skill_name(GET_OBJ_VAL(j, 2)),
      skill_name(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    stat_appendf(buf, buf_size, &len, "%s at level %d, %d/%d charges",
      skill_name(GET_OBJ_VAL(j, 3)),
      GET_OBJ_VAL(j, 0),
      GET_OBJ_VAL(j, 2),
      GET_OBJ_VAL(j, 1));
    break;
  case ITEM_WEAPON:
    stat_appendf(buf, buf_size, &len, "Damage %dd%d (avg %.1f), type %s",
      GET_OBJ_VAL(j, 1),
      GET_OBJ_VAL(j, 2),
      ((GET_OBJ_VAL(j, 2) + 1) / 2.0) * GET_OBJ_VAL(j, 1),
      attack_hit_text[GET_OBJ_VAL(j, 3)].singular);
    break;
  case ITEM_ARMOR:
    stat_appendf(buf, buf_size, &len, "Piece AC %d, bulk %d, magic %+d, stealth %s, durability %d, str req %d",
      GET_OBJ_VAL(j, VAL_ARMOR_PIECE_AC),
      GET_OBJ_VAL(j, VAL_ARMOR_BULK),
      GET_OBJ_VAL(j, VAL_ARMOR_MAGIC_BONUS),
      YESNO(GET_OBJ_VAL(j, VAL_ARMOR_STEALTH_DISADV)),
      GET_OBJ_VAL(j, VAL_ARMOR_DURABILITY),
      GET_OBJ_VAL(j, VAL_ARMOR_STR_REQ));
    break;
  case ITEM_CONTAINER: {
    char flags[MAX_STRING_LENGTH];
    sprintbit(GET_OBJ_VAL(j, 1), container_bits, flags, sizeof(flags));
    stat_appendf(buf, buf_size, &len, "Capacity %d, Flags %s, Key %d, Corpse %s",
      GET_OBJ_VAL(j, 0),
      flags,
      GET_OBJ_VAL(j, 2),
      YESNO(GET_OBJ_VAL(j, 3)));
    break;
  }
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN: {
    char drink[MAX_INPUT_LENGTH];
    sprinttype(GET_OBJ_VAL(j, 2), drinks, drink, sizeof(drink));
    stat_appendf(buf, buf_size, &len, "Capacity %d, Contains %d, Liquid %s, Poisoned %s",
      GET_OBJ_VAL(j, 0),
      GET_OBJ_VAL(j, 1),
      drink,
      YESNO(GET_OBJ_VAL(j, 3)));
    break;
  }
  case ITEM_NOTE:
    stat_appendf(buf, buf_size, &len, "Tongue %d", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_FOOD:
    stat_appendf(buf, buf_size, &len, "Bites %d/%d, Hours full %d, Poisoned %s",
      GET_OBJ_VAL(j, 1),
      GET_OBJ_VAL(j, 0),
      GET_OBJ_VAL(j, 2),
      YESNO(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_MONEY:
    stat_appendf(buf, buf_size, &len, "Coins: %d", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_FURNITURE:
    stat_appendf(buf, buf_size, &len, "Seats %d/%d, Allowed pos %d",
      GET_OBJ_VAL(j, 1),
      GET_OBJ_VAL(j, 0),
      GET_OBJ_VAL(j, 2));
    if (OBJ_SAT_IN_BY(j)) {
      stat_appendf(buf, buf_size, &len, ", Occupied by:");
      for (tempch = OBJ_SAT_IN_BY(j); tempch; tempch = NEXT_SITTING(tempch))
        stat_appendf(buf, buf_size, &len, " %s", GET_NAME(tempch));
    }
    break;
  default:
    break;
  }

  if (!len)
    strlcpy(buf, "None", buf_size);
}

static int purge_room(room_rnum room)
{
  int j;
  struct char_data *vict;

  if (room == NOWHERE || room > top_of_world) return 0;

  for (vict = world[room].people; vict; vict = vict->next_in_room) {
    if (!IS_NPC(vict))
      continue;

    /* Dump inventory. */
    while (vict->carrying)
      extract_obj(vict->carrying);

    /* Dump equipment. */
    for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(vict, j))
        extract_obj(GET_EQ(vict, j));

    /* Dump character. */
    extract_char(vict);
  }

  /* Clear the ground. */
  while (world[room].contents)
    extract_obj(world[room].contents);

  return 1;
}

/* ===================== Emote engine ===================== */

/* Operators:
 *
 *   ~ (name)                   / target sees "you"
 *   ! him/her/them             / target sees "you"
 *   % (name)'s                 / target sees "your"
 *   ^ his/her/their            / target sees "your"
 *   # he/she/they              / target sees "you"
 *   & himself/herself/themself / target sees "yourself"
 *   = (name)'s                 / target sees "yours"
 *   + his/hers/theirs          / target sees "yours"
 *   @ moves actor name (or actor's possessive for pemote) to that position
 */

/* --- Pronoun & string helpers --- */
static const char *pron_obj(struct char_data *tch) { /* him/her/them */
  switch (GET_SEX(tch)) { case SEX_MALE: return "him"; case SEX_FEMALE: return "her"; default: return "them"; }
}
static const char *pron_pos_adj(struct char_data *tch) { /* his/her/their */
  switch (GET_SEX(tch)) { case SEX_MALE: return "his"; case SEX_FEMALE: return "her"; default: return "their"; }
}
static const char *pron_pos_pron(struct char_data *tch) { /* his/hers/theirs */
  switch (GET_SEX(tch)) { case SEX_MALE: return "his"; case SEX_FEMALE: return "hers"; default: return "theirs"; }
}
static const char *pron_subj(struct char_data *tch) { /* he/she/they */
  switch (GET_SEX(tch)) { case SEX_MALE: return "he"; case SEX_FEMALE: return "she"; default: return "they"; }
}
static const char *pron_refl(struct char_data *tch) { /* himself/herself/themself */
  switch (GET_SEX(tch)) { case SEX_MALE: return "himself"; case SEX_FEMALE: return "herself"; default: return "themself"; }
}

static void make_possessive(const char *name, char *out, size_t outsz) {
  size_t n = strlcpy(out, name, outsz);
  if (n > 0 && out[n-1] == 's') strlcat(out, "'", outsz);
  else                          strlcat(out, "'s", outsz);
}

static bool is_end_punct(char c) { return (c == '.' || c == '!' || c == '?'); }

static void collapse_spaces(char *s) {
  char *src = s, *dst = s; bool last_space = false;
  while (*src) {
    if (isspace((unsigned char)*src)) { if (!last_space) *dst++ = ' '; last_space = true; }
    else { *dst++ = *src; last_space = false; }
    src++;
  }
  *dst = '\0';
}

static void build_actor_name(struct char_data *actor,
                             bool possessive,
                             char *out, size_t outsz)
{
  /* Prefer short description if present; fall back to personal name */
  const char *base =
    (GET_SHORT_DESC(actor) && *GET_SHORT_DESC(actor))
      ? GET_SHORT_DESC(actor)
      : GET_NAME(actor);

  if (!possessive)
    strlcpy(out, base, outsz);
  else
    make_possessive(base, out, outsz);
}

/* Replace all occurrences of 'needle' in 'hay' with 'repl'. */
static void replace_all_tokens(char *hay, size_t haysz, const char *needle, const char *repl) {
  char work[MAX_STRING_LENGTH]; work[0] = '\0';
  const char *src = hay; size_t nlen = strlen(needle);
  while (*src) {
    const char *pos = strstr(src, needle);
    if (!pos) { strlcat(work, src, sizeof(work)); break; }
    size_t head = (size_t)(pos - src);
    if (head) {
      char chunk[2048];
      if (head >= sizeof(chunk)) head = sizeof(chunk) - 1;
      memcpy(chunk, src, head); chunk[head] = '\0';
      strlcat(work, chunk, sizeof(work));
    }
    strlcat(work, repl, sizeof(work));
    src = pos + nlen;
  }
  strlcpy(hay, work, haysz);
}

static bool is_token_operator(char c) {
  switch (c) {
    case '~': case '!': case '%': case '^': case '#':
    case '&': case '=': case '+': case '@':
      return TRUE;
  }
  return FALSE;
}

/* Capitalize the first alphabetic character of every sentence (start and after .?!).
   Skips any number of spaces and common closers (quotes/brackets) between sentences. */
static void capitalize_sentences(char *s) {
  bool new_sent = true;
  for (char *p = s; *p; ++p) {
    unsigned char c = (unsigned char)*p;
    if (new_sent) {
      if (isalpha(c)) {
        *p = toupper(c);
        new_sent = false;
      } else if (!isspace(c) && c != '"' && c != '\'' && c != ')' && c != ']' && c != '}') {
        /* Non-space symbol at sentence start; don't flip flag yet unless it's a letter later. */
      }
    }
    if (*p == '.' || *p == '!' || *p == '?')
      new_sent = true;
  }
}

/* --- Numbered target parsing --- */
/* Parse "2.corpse" => out_num=2, out_name="corpse". "corpse" => out_num=1, out_name="corpse". */
static void split_numbered_name(const char *raw, int *out_num, char *out_name, size_t outsz) {
  char tmp[MAX_NAME_LENGTH];
  strlcpy(tmp, raw, sizeof(tmp));
  char *p = tmp;
  int num = get_number(&p);   /* advances p if leading "N." is present */
  if (num <= 0) num = 1;
  if (*p) strlcpy(out_name, p, outsz);
  else    strlcpy(out_name, tmp, outsz);
  *out_num = num;
}

/* Case-insensitive strcmp helper (tiny) */
static bool ieq(const char *a, const char *b) {
  while (*a && *b) {
    char ca = tolower((unsigned char)*a++);
    char cb = tolower((unsigned char)*b++);
    if (ca != cb) return false;
  }
  return *a == '\0' && *b == '\0';
}

/* Resolve a char/obj with support for numbered refs, searching room chars, room objs, inventory, equipment.
   Special: "me"/"self" (any operator) resolves to the actor. */
static bool resolve_reference(struct char_data *actor,
                              const char *raw,
                              struct char_data **out_ch,
                              struct obj_data **out_obj) {
  char name[MAX_NAME_LENGTH];
  int ordinal = 1;

  *out_ch = NULL; *out_obj = NULL;

  if (ieq(raw, "me") || ieq(raw, "self")) { *out_ch = actor; return true; }

  split_numbered_name(raw, &ordinal, name, sizeof(name));

  /* Characters in room (ordinal applies to this list) */
  {
    int num = ordinal;
    struct char_data *tch = get_char_room_vis(actor, name, &num);
    if (tch) { *out_ch = tch; return true; }
  }

  /* Objects in room */
  {
    int num = ordinal;
    struct obj_data *obj = get_obj_in_list_vis(actor, name, &num, world[IN_ROOM(actor)].contents);
    if (obj) { *out_obj = obj; return true; }
  }
  /* Objects carried */
  {
    int num = ordinal;
    struct obj_data *obj = get_obj_in_list_vis(actor, name, &num, actor->carrying);
    if (obj) { *out_obj = obj; return true; }
  }
  /* Objects equipped */
  {
    int num = ordinal;
    struct obj_data *obj = get_obj_in_equip_vis(actor, name, &num, actor->equipment);
    if (obj) { *out_obj = obj; return true; }
  }

  return false;
}

/* Build replacement text for a token as seen by 'viewer'. */
static void build_replacement(const struct emote_token *tok,
                              struct char_data *actor,
                              struct char_data *viewer,
                              bool actor_possessive_for_at,
                              char *out, size_t outsz) {
  out[0] = '\0';

  if (tok->op == '@') {
    /* Use actor's sdesc if present, otherwise their name */
    const char *ref = (GET_SHORT_DESC(actor) && *GET_SHORT_DESC(actor))
                      ? GET_SHORT_DESC(actor)
                      : GET_NAME(actor);

    if (!actor_possessive_for_at)
      strlcpy(out, ref, outsz);
    else
      make_possessive(ref, out, outsz);
    return;
  }

  if (tok->tch) {
    bool you = (viewer == tok->tch);
    /* For non-you views, prefer sdesc over name */
    const char *ref = (GET_SHORT_DESC(tok->tch) && *GET_SHORT_DESC(tok->tch))
                      ? GET_SHORT_DESC(tok->tch)
                      : GET_NAME(tok->tch);

    switch (tok->op) {
      case '~':
        if (you) strlcpy(out, "you", outsz);
        else     strlcpy(out, ref, outsz);
        break;

      case '!':
        if (you) strlcpy(out, "you", outsz);
        else     strlcpy(out, pron_obj(tok->tch), outsz);
        break;

      case '%':
        if (you) strlcpy(out, "your", outsz);
        else     make_possessive(ref, out, outsz);
        break;

      case '^':
        if (you) strlcpy(out, "your", outsz);
        else     strlcpy(out, pron_pos_adj(tok->tch), outsz);
        break;

      case '#':
        if (you) strlcpy(out, "you", outsz);
        else     strlcpy(out, pron_subj(tok->tch), outsz);
        break;

      case '&':
        if (you) strlcpy(out, "yourself", outsz);
        else     strlcpy(out, pron_refl(tok->tch), outsz);
        break;

      case '=':
        if (you) strlcpy(out, "yours", outsz);
        else     make_possessive(ref, out, outsz);
        break;

      case '+':
        if (you) strlcpy(out, "yours", outsz);
        else     strlcpy(out, pron_pos_pron(tok->tch), outsz);
        break;

      default:
        strlcpy(out, ref, outsz);
        break;
    }
    return;
  }

  if (tok->tobj) {
    const char *sdesc = (tok->tobj->short_description && *tok->tobj->short_description)
                        ? tok->tobj->short_description : "something";
    char posbuf[MAX_INPUT_LENGTH];
    switch (tok->op) {
      case '~': strlcpy(out, sdesc, outsz); break;
      case '!': strlcpy(out, "it", outsz);  break;
      case '#': strlcpy(out, "it", outsz);  break;
      case '%':
      case '=': make_possessive(sdesc, posbuf, sizeof(posbuf)); strlcpy(out, posbuf, outsz); break;
      case '^':
      case '+': strlcpy(out, "its", outsz); break;
      case '&': strlcpy(out, "itself", outsz); break;
      default:  strlcpy(out, sdesc, outsz); break;
    }
    return;
  }

  strlcpy(out, "something", outsz);
}

bool build_targeted_phrase(struct char_data *ch, const char *input, bool allow_actor_at, struct targeted_phrase *phrase) {
  struct emote_token tokens[MAX_EMOTE_TOKENS];
  int tokc = 0;
  char out[MAX_STRING_LENGTH];
  char working[MAX_STRING_LENGTH];
  const char *p;

  if (!phrase)
    return FALSE;

  phrase->template[0] = '\0';
  phrase->token_count = 0;

  if (!input || !*input)
    return TRUE;

  strlcpy(working, input, sizeof(working));
  out[0] = '\0';
  p = working;

  while (*p) {
    if (is_token_operator(*p)) {
      char op = *p++;
      char name[MAX_NAME_LENGTH];
      int ni = 0;

      if (op == '@' && !allow_actor_at) {
        send_to_char(ch, "You can't use '@' in that phrase.\r\n");
        return FALSE;
      }

      if (op != '@') {
        const char *q = p;

        while (*q && isdigit((unsigned char)*q) && ni < (int)sizeof(name) - 1)
          name[ni++] = *q++;

        if (ni > 0 && *q == '.' && ni < (int)sizeof(name) - 1)
          name[ni++] = *q++;

        while (*q && (isalnum((unsigned char)*q) || *q == '_') && ni < (int)sizeof(name) - 1)
          name[ni++] = *q++;

        name[ni] = '\0';
        p = q;
      } else {
        name[0] = '\0';
      }

      if (tokc >= MAX_EMOTE_TOKENS) {
        send_to_char(ch, "That's too many references for one phrase.\r\n");
        return FALSE;
      }

      tokens[tokc].op = op;
      tokens[tokc].name[0] = '\0';
      tokens[tokc].tch = NULL;
      tokens[tokc].tobj = NULL;

      if (op != '@') {
        if (!*name) {
          send_to_char(ch, "You need to specify who or what you're referencing.\r\n");
          return FALSE;
        }
        strlcpy(tokens[tokc].name, name, sizeof(tokens[tokc].name));
        if (!resolve_reference(ch, name, &tokens[tokc].tch, &tokens[tokc].tobj)) {
          send_to_char(ch, "You can't find one of the references here.\r\n");
          return FALSE;
        }
      }

      char ph[16];
      snprintf(ph, sizeof(ph), "$T%d", tokc + 1);
      strlcat(out, ph, sizeof(out));
      tokc++;
      continue;
    }

    char buf[2] = { *p++, '\0' };
    strlcat(out, buf, sizeof(out));
  }

  strlcpy(phrase->template, out, sizeof(phrase->template));
  phrase->token_count = tokc;
  for (int i = 0; i < tokc; i++)
    phrase->tokens[i] = tokens[i];

  return TRUE;
}

void render_targeted_phrase(struct char_data *actor,
                            const struct targeted_phrase *phrase,
                            bool actor_possessive_for_at,
                            struct char_data *viewer,
                            char *out,
                            size_t outsz)
{
  char msg[MAX_STRING_LENGTH];

  if (!out || !phrase) {
    if (out && outsz > 0)
      *out = '\0';
    return;
  }

  if (!phrase->template[0]) {
    if (outsz > 0)
      *out = '\0';
    return;
  }

  strlcpy(msg, phrase->template, sizeof(msg));

  for (int i = 0; i < phrase->token_count; i++) {
    char token[16], repl[MAX_INPUT_LENGTH];
    snprintf(token, sizeof(token), "$T%d", i + 1);
    build_replacement(&phrase->tokens[i], actor, viewer, actor_possessive_for_at, repl, sizeof(repl));
    replace_all_tokens(msg, sizeof(msg), token, repl);
  }

  collapse_spaces(msg);
  strlcpy(out, msg, outsz);
}

static bool hidden_emote_can_view(struct char_data *actor,
                                  struct char_data *viewer,
                                  int stealth_total) {
  int opposed;

  if (!viewer || viewer == actor)
    return TRUE;
  if (GET_LEVEL(viewer) >= LVL_IMMORT)
    return TRUE;
  if (!AWAKE(viewer))
    return FALSE;

  if (can_scan_for_sneak(viewer))
    opposed = roll_scan_perception(viewer);
  else
    opposed = rand_number(1, 20);

  return opposed >= stealth_total;
}

/* ===================== Main entry ===================== */
void perform_emote(struct char_data *ch, char *argument, bool possessive, bool hidden) {
  char base[MAX_STRING_LENGTH];
  char with_placeholders[MAX_STRING_LENGTH];
  int at_count = 0;
  int stealth_total = 0;

  struct emote_token toks[MAX_EMOTE_TOKENS];
  int tokc = 0;

  skip_spaces(&argument);
  if (!*argument) { send_to_char(ch, "Yes... but what?\r\n"); return; }

  if (hidden)
    stealth_total = roll_stealth_check(ch);

  /* Only one '@' allowed (inserts actor name/possessive at that spot) */
  for (const char *c = argument; *c; ++c) if (*c == '@') at_count++;
  if (at_count > 1) { send_to_char(ch, "You can only use '@' once in an emote.\r\n"); return; }
  bool has_at = (at_count == 1);

  /* Prefix actor name unless '@' is used */
  if (!has_at) {
    char who[MAX_INPUT_LENGTH];
    build_actor_name(ch, possessive, who, sizeof(who));
    snprintf(base, sizeof(base), "%s %s", who, argument);
  } else {
    strlcpy(base, argument, sizeof(base));
  }

  /* Parse operators to placeholders ($Tn) and capture tokens in order */
  {
    char out[MAX_STRING_LENGTH]; out[0] = '\0';
    const char *p = base;

    while (*p && tokc < MAX_EMOTE_TOKENS) {
      if (*p == '@' || *p == '~' || *p == '!' || *p == '%' ||
          *p == '^' || *p == '#' || *p == '&' || *p == '=' || *p == '+') {

        char op = *p++;
        char name[MAX_NAME_LENGTH]; int ni = 0;

        if (op == '@') {
          name[0] = '\0';
        } else {
          /* Allow numbered refs like "2.corpse" but strip trailing punctuation. */
          const char *q = p;

          /* optional leading digits */
          while (*q && isdigit((unsigned char)*q) && ni < (int)sizeof(name) - 1)
            name[ni++] = *q++;

          /* include a single '.' only if we had digits (for "2.corpse") */
          if (ni > 0 && *q == '.' && ni < (int)sizeof(name) - 1)
            name[ni++] = *q++;

          /* base name (letters/digits/underscore only) */
          while (*q && (isalnum((unsigned char)*q) || *q == '_') && ni < (int)sizeof(name) - 1)
            name[ni++] = *q++;

          name[ni] = '\0';
          p = q;
        }

        toks[tokc].op = op;
        toks[tokc].name[0] = '\0';
        toks[tokc].tch = NULL;
        toks[tokc].tobj = NULL;

        if (op != '@') {
          strlcpy(toks[tokc].name, name, sizeof(toks[tokc].name));
          if (!resolve_reference(ch, name, &toks[tokc].tch, &toks[tokc].tobj)) {
            send_to_char(ch, "You can't find one of the references here.\r\n");
            return;
          }
        }

        char ph[16]; snprintf(ph, sizeof(ph), "$T%d", tokc + 1);
        strlcat(out, ph, sizeof(out));
        tokc++;
        continue;
      }

      char buf[2] = { *p++, '\0' };
      strlcat(out, buf, sizeof(out));
    }
    strlcat(out, p, sizeof(out));
    strlcpy(with_placeholders, out, sizeof(with_placeholders));
  }

  /* Replace literal '@' with a placeholder if present */
  if (has_at && tokc < MAX_EMOTE_TOKENS) {
    toks[tokc].op = '@'; toks[tokc].name[0] = '\0'; toks[tokc].tch = NULL; toks[tokc].tobj = NULL;
    char ph[16]; snprintf(ph, sizeof(ph), "$T%d", tokc + 1);
    replace_all_tokens(with_placeholders, sizeof(with_placeholders), "@", ph);
    tokc++;
  }

  /* Polish: ensure final punctuation (only if the string lacks any .?! at the very end), collapse spaces */
  {
    size_t n = strlen(with_placeholders);
    bool ends_with_punct = (n > 0 && is_end_punct(with_placeholders[n - 1]));
    if (!ends_with_punct) strlcat(with_placeholders, ".", sizeof(with_placeholders));
    collapse_spaces(with_placeholders);
  }

  struct targeted_phrase phrase;
  strlcpy(phrase.template, with_placeholders, sizeof(phrase.template));
  phrase.token_count = tokc;
  for (int i = 0; i < tokc && i < MAX_EMOTE_TOKENS; i++)
    phrase.tokens[i] = toks[i];

  /* Deliver personalized message to everyone in the room (including actor) */
  for (struct descriptor_data *d = descriptor_list; d; d = d->next) {
    if (STATE(d) != CON_PLAYING || !d->character) continue;
    if (IN_ROOM(d->character) != IN_ROOM(ch))     continue;
    if (hidden && !hidden_emote_can_view(ch, d->character, stealth_total))
      continue;

    char msg[MAX_STRING_LENGTH];
    render_targeted_phrase(ch, &phrase, possessive, d->character, msg, sizeof(msg));

    /* Final per-viewer cleanup: spaces + multi-sentence capitalization */
    collapse_spaces(msg);
    capitalize_sentences(msg);

    if (d->character == ch) {
      act(msg, FALSE, ch, NULL, NULL, TO_CHAR);
    } else if (hidden) {
      send_to_char(d->character, "You notice:\r\n%s\r\n", msg);
    } else {
      act(msg, FALSE, ch, NULL, d->character, TO_VICT);
    }
  }
}
/* =================== End emote engine =================== */

ACMD(do_wizhelp) 
{ 
  extern int *cmd_sort_info; 
  int no = 1, i, cmd_num; 
  int level;

  if (!ch->desc)
    return;

  send_to_char(ch, "The following privileged commands are available:\r\n"); 
  
  for (level = LVL_IMPL; level >= LVL_IMMORT; level--) { 
    send_to_char(ch, "%sLevel %d%s:\r\n", CCCYN(ch, C_NRM), level, CCNRM(ch, C_NRM)); 
    for (no = 1, cmd_num = 1; complete_cmd_info[cmd_sort_info[cmd_num]].command[0] != '\n'; cmd_num++) { 
      i = cmd_sort_info[cmd_num]; 
  
      if (complete_cmd_info[i].minimum_level != level) 
        continue;            
  
      send_to_char(ch, "%-14s%s", complete_cmd_info[i].command, no++ % 7 == 0 ? "\r\n" : ""); 
    } 
    if (no % 7 != 1) 
      send_to_char(ch, "\r\n"); 
    if (level != LVL_IMMORT) 
      send_to_char(ch, "\r\n"); 
  } 
}

ACMD(do_echo)
{
  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "Yes.. but what?\r\n");
  else {
    char buf[MAX_INPUT_LENGTH + 4];

    if (subcmd == SCMD_EMOTE)
      snprintf(buf, sizeof(buf), "$n %s", argument);
    else {
      strlcpy(buf, argument, sizeof(buf));
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "(GC) %s echoed: %s", GET_NAME(ch), buf);
      }
    act(buf, FALSE, ch, 0, 0, TO_ROOM);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
  }
}

ACMD(do_emote)
{
  perform_emote(ch, argument, FALSE, FALSE);
}

ACMD(do_pemote)
{
  perform_emote(ch, argument, TRUE, FALSE);
}

ACMD(do_hemote)
{
  perform_emote(ch, argument, FALSE, TRUE);
}

ACMD(do_phemote)
{
  perform_emote(ch, argument, TRUE, TRUE);
}

ACMD(do_send)
{
  char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char(ch, "Send what to who?\r\n");
    return;
  }
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    return;
  }
  send_to_char(vict, "%s\r\n", buf);
  mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s sent %s: %s", GET_NAME(ch), GET_NAME(vict), buf);

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "Sent.\r\n");
  else
    send_to_char(ch, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
}

/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(struct char_data *ch, char *rawroomstr)
{
  room_rnum location = NOWHERE;
  char roomstr[MAX_INPUT_LENGTH];

  one_argument(rawroomstr, roomstr);

  if (!*roomstr) {
    send_to_char(ch, "You must supply a room number or name.\r\n");
    return (NOWHERE);
  }

  if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
    if ((location = real_room((room_vnum)atoi(roomstr))) == NOWHERE) {
      send_to_char(ch, "No room exists with that number.\r\n");
      return (NOWHERE);
    }
  } else {
    struct char_data *target_mob;
    struct obj_data *target_obj;
    char *mobobjstr = roomstr;
    int num;

    num = get_number(&mobobjstr);
    if ((target_mob = get_char_vis(ch, mobobjstr, &num, FIND_CHAR_WORLD)) != NULL) {
      if ((location = IN_ROOM(target_mob)) == NOWHERE) {
        send_to_char(ch, "That character is currently lost.\r\n");
        return (NOWHERE);
      }
    } else if ((target_obj = get_obj_vis(ch, mobobjstr, &num)) != NULL) {
      if (IN_ROOM(target_obj) != NOWHERE)
        location = IN_ROOM(target_obj);
      else if (target_obj->carried_by && IN_ROOM(target_obj->carried_by) != NOWHERE)
        location = IN_ROOM(target_obj->carried_by);
      else if (target_obj->worn_by && IN_ROOM(target_obj->worn_by) != NOWHERE)
        location = IN_ROOM(target_obj->worn_by);

      if (location == NOWHERE) {
        send_to_char(ch, "That object is currently not in a room.\r\n");
        return (NOWHERE);
      }
    }

    if (location == NOWHERE) {
      send_to_char(ch, "Nothing exists by that name.\r\n");
      return (NOWHERE);
    }
  }

  /* A location has been found -- if you're >= GRGOD, no restrictions. */
  if (GET_LEVEL(ch) >= LVL_GRGOD)
    return (location);

  if (ROOM_FLAGGED(location, ROOM_GODROOM))
    send_to_char(ch, "You are not godly enough to use that room!\r\n");
  else if (ROOM_FLAGGED(location, ROOM_PRIVATE) && world[location].people && world[location].people->next_in_room)
    send_to_char(ch, "There's a private conversation going on in that room.\r\n");
  else if (ROOM_FLAGGED(location, ROOM_HOUSE) && !House_can_enter(ch, GET_ROOM_VNUM(location)))
    send_to_char(ch, "That's private property -- no trespassing!\r\n");
  else
    return (location);

  return (NOWHERE);
}

ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  room_rnum location, original_loc;

  half_chop(argument, buf, command);
  if (!*buf) {
    send_to_char(ch, "You must supply a room number or a name.\r\n");
    return;
  }

  if (!*command) {
    send_to_char(ch, "What do you want to do there?\r\n");
    return;
  }

  if ((location = find_target_room(ch, buf)) == NOWHERE)
    return;

  /* a location has been found. */
  original_loc = IN_ROOM(ch);
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);

  /* check if the char is still there */
  if (IN_ROOM(ch) == location) {
    char_from_room(ch);
    char_to_room(ch, original_loc);
  }
}

ACMD(do_goto)
{
  char buf[MAX_STRING_LENGTH];
  room_rnum location;

  if ((location = find_target_room(ch, argument)) == NOWHERE)
    return;

  if (ZONE_FLAGGED(GET_ROOM_ZONE(location), ZONE_NOIMMORT) && (GET_LEVEL(ch) >= LVL_IMMORT) && (GET_LEVEL(ch) < LVL_GRGOD)) {
    send_to_char(ch, "Sorry, that zone is off-limits for immortals!");
    return;
  }

  snprintf(buf, sizeof(buf), "$n %s", POOFOUT(ch) ? POOFOUT(ch) : "disappears in a puff of smoke.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);

  char_from_room(ch);
  char_to_room(ch, location);

  snprintf(buf, sizeof(buf), "$n %s", POOFIN(ch) ? POOFIN(ch) : "appears with an ear-splitting bang.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);

  look_at_room(ch, 0);
  enter_wtrigger(&world[IN_ROOM(ch)], ch, -1);
}

ACMD(do_trans)
{
  char buf[MAX_INPUT_LENGTH];
  struct descriptor_data *i;
  struct char_data *victim;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char(ch, "Whom do you wish to transfer?\r\n");
  else if (str_cmp("all", buf)) {
    if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (victim == ch)
      send_to_char(ch, "That doesn't make much sense, does it?\r\n");
    else {
      if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
	send_to_char(ch, "Go transfer someone your own size.\r\n");
	return;
      }
      act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
      char_from_room(victim);
      char_to_room(victim, IN_ROOM(ch));
      act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
      look_at_room(victim, 0);

      enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
    }
  } else {			/* Trans All */
    if (GET_LEVEL(ch) < LVL_GRGOD) {
      send_to_char(ch, "I think not.\r\n");
      return;
    }

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i->character && i->character != ch) {
	victim = i->character;
	if (GET_LEVEL(victim) >= GET_LEVEL(ch))
	  continue;
	act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, IN_ROOM(ch));
	act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
	act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
	look_at_room(victim, 0);
        enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
      }
    send_to_char(ch, "%s", CONFIG_OK);
  }
}

ACMD(do_teleport)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *victim;
  room_rnum target;

  two_arguments(argument, buf, buf2);

  if (!*buf)
    send_to_char(ch, "Whom do you wish to teleport?\r\n");
  else if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (victim == ch)
    send_to_char(ch, "Use 'goto' to teleport yourself.\r\n");
  else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
    send_to_char(ch, "Maybe you shouldn't do that.\r\n");
  else if (!*buf2)
    send_to_char(ch, "Where do you wish to send this person?\r\n");
  else if ((target = find_target_room(ch, buf2)) != NOWHERE) {
    send_to_char(ch, "%s", CONFIG_OK);
    act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, target);
    act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    act("$n has teleported you!", FALSE, ch, 0, (char *) victim, TO_VICT);
    look_at_room(victim, 0);
    enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
  }
}

ACMD(do_vnum)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  int good_arg = 0;

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2) {
    send_to_char(ch, "Usage: vnum { obj | mob | room | trig } <name>\r\n");
    return;
  }
  if (is_abbrev(buf, "mob") && (good_arg = 1))
    if (!vnum_mobile(buf2, ch))
      send_to_char(ch, "No mobiles by that name.\r\n");

  if (is_abbrev(buf, "obj") && (good_arg =1 ))
    if (!vnum_object(buf2, ch))
      send_to_char(ch, "No objects by that name.\r\n");

  if (is_abbrev(buf, "room") && (good_arg = 1))
    if (!vnum_room(buf2, ch))
      send_to_char(ch, "No rooms by that name.\r\n");

  if (is_abbrev(buf, "trig") && (good_arg = 1))
    if (!vnum_trig(buf2, ch))
      send_to_char(ch, "No triggers by that name.\r\n");

  if (!good_arg)
     send_to_char(ch, "Usage: vnum { obj | mob | room | trig } <name>\r\n");
 }

#define ZOCMD zone_table[zrnum].cmd[subcmd]

static void stat_format_zone_cmds_room(room_vnum rvnum, char *buf, size_t buf_size)
{
  zone_rnum zrnum = real_zone_by_thing(rvnum);
  room_rnum rrnum = real_room(rvnum), cmd_room = NOWHERE;
  int subcmd = 0, count = 0;
  size_t len = 0;

  buf[0] = '\0';

  if (zrnum == NOWHERE || rrnum == NOWHERE) {
    strlcpy(buf, "None", buf_size);
    return;
  }

#define ZOCMD zone_table[zrnum].cmd[subcmd]
  while (ZOCMD.command != 'S') {
    switch (ZOCMD.command) {
      case 'M':
      case 'O':
      case 'T':
      case 'V':
        cmd_room = ZOCMD.arg3;
        break;
      case 'D':
      case 'R':
        cmd_room = ZOCMD.arg1;
        break;
      default:
        cmd_room = NOWHERE;
        break;
    }

    if (cmd_room == rrnum) {
      if (len)
        stat_appendf(buf, buf_size, &len, "\n");

      count++;
      switch (ZOCMD.command) {
        case 'M':
          stat_appendf(buf, buf_size, &len, "%sLoad mob %s (#%d) max %d",
            ZOCMD.if_flag ? "then " : "",
            mob_proto[ZOCMD.arg1].player.short_descr,
            mob_index[ZOCMD.arg1].vnum,
            ZOCMD.arg2);
          break;
        case 'G':
          stat_appendf(buf, buf_size, &len, "%sGive %s (#%d) max %d",
            ZOCMD.if_flag ? "then " : "",
            obj_proto[ZOCMD.arg1].short_description,
            obj_index[ZOCMD.arg1].vnum,
            ZOCMD.arg2);
          break;
        case 'O':
          stat_appendf(buf, buf_size, &len, "%sLoad obj %s (#%d) max %d",
            ZOCMD.if_flag ? "then " : "",
            obj_proto[ZOCMD.arg1].short_description,
            obj_index[ZOCMD.arg1].vnum,
            ZOCMD.arg2);
          break;
        case 'E':
          stat_appendf(buf, buf_size, &len, "%sEquip %s (#%d) %s max %d",
            ZOCMD.if_flag ? "then " : "",
            obj_proto[ZOCMD.arg1].short_description,
            obj_index[ZOCMD.arg1].vnum,
            equipment_types[ZOCMD.arg3],
            ZOCMD.arg2);
          break;
        case 'P':
          stat_appendf(buf, buf_size, &len, "%sPut %s (#%d) in %s (#%d) max %d",
            ZOCMD.if_flag ? "then " : "",
            obj_proto[ZOCMD.arg1].short_description,
            obj_index[ZOCMD.arg1].vnum,
            obj_proto[ZOCMD.arg3].short_description,
            obj_index[ZOCMD.arg3].vnum,
            ZOCMD.arg2);
          break;
        case 'R':
          stat_appendf(buf, buf_size, &len, "%sRemove %s (#%d)",
            ZOCMD.if_flag ? "then " : "",
            obj_proto[ZOCMD.arg2].short_description,
            obj_index[ZOCMD.arg2].vnum);
          break;
        case 'D':
          stat_appendf(buf, buf_size, &len, "%sSet door %s %s",
            ZOCMD.if_flag ? "then " : "",
            dirs[ZOCMD.arg2],
            ZOCMD.arg3 ? ((ZOCMD.arg3 == 1) ? "closed" : "locked") : "open");
          break;
        case 'T':
          stat_appendf(buf, buf_size, &len, "%sAttach trig %s (#%d) to %s",
            ZOCMD.if_flag ? "then " : "",
            trig_index[ZOCMD.arg2]->proto->name,
            trig_index[ZOCMD.arg2]->vnum,
            (ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" :
            (ZOCMD.arg1 == OBJ_TRIGGER) ? "object" :
            (ZOCMD.arg1 == WLD_TRIGGER) ? "room" : "unknown");
          break;
        case 'V':
          stat_appendf(buf, buf_size, &len, "%sAssign global %s:%d to %s = %s",
            ZOCMD.if_flag ? "then " : "",
            ZOCMD.sarg1,
            ZOCMD.arg2,
            (ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" :
            (ZOCMD.arg1 == OBJ_TRIGGER) ? "object" :
            (ZOCMD.arg1 == WLD_TRIGGER) ? "room" : "unknown",
            ZOCMD.sarg2 ? ZOCMD.sarg2 : "");
          break;
        default:
          stat_appendf(buf, buf_size, &len, "%s<Unknown command %c>",
            ZOCMD.if_flag ? "then " : "",
            ZOCMD.command);
          break;
      }
    }
    subcmd++;
  }
#undef ZOCMD

  if (!count)
    strlcpy(buf, "None", buf_size);
}

static void do_stat_room(struct char_data *ch, struct room_data *rm)
{
  char buf[MAX_STRING_LENGTH];
  char chars[MAX_STRING_LENGTH];
  char objs[MAX_STRING_LENGTH];
  char exits[MAX_STRING_LENGTH];
  char triggers[MAX_STRING_LENGTH];
  char globals[MAX_STRING_LENGTH];
  char zone_cmds[MAX_STRING_LENGTH];
  size_t len = 0;

  stat_table_border(ch);
  stat_table_row(ch, "Name", rm->name ? rm->name : "<None>");

  stat_table_row_fmt(ch, "Identifiers", "Zone %d (%s), VNum #%d (RNum %d), ID %ld",
    zone_table[rm->zone].number, zone_table[rm->zone].name,
    rm->number, real_room(rm->number), room_script_id(rm));

  sprinttype(rm->sector_type, sector_types, buf, sizeof(buf));
  stat_table_row_fmt(ch, "Type", "Sector %s, SpecProc %s",
    buf,
    rm->func ? get_spec_func_name(rm->func) : "None");

  sprintbitarray(rm->room_flags, room_bits, RF_ARRAY_MAX, buf);
  stat_table_row(ch, "Room Flags", buf);

  buf[0] = '\0';
  len = 0;
  for (struct extra_descr_data *desc = rm->ex_description; desc; desc = desc->next)
    stat_appendf(buf, sizeof(buf), &len, "%s%s", len ? ", " : "", desc->keyword ? desc->keyword : "<None>");
  if (!len)
    strlcpy(buf, "None", sizeof(buf));
  stat_table_row(ch, "Extra Descs", buf);

  stat_format_character_list(ch, rm->people, chars, sizeof(chars));
  stat_table_row(ch, "Characters", chars);

  stat_format_object_list(ch, rm->contents, objs, sizeof(objs));
  stat_table_row(ch, "Contents", objs);

  stat_format_exit_summary(rm, exits, sizeof(exits));
  stat_table_row(ch, "Exits", exits);

  stat_format_script_triggers(SCRIPT(rm), triggers, sizeof(triggers));
  stat_table_row(ch, "Triggers", triggers);

  stat_format_script_globals(SCRIPT(rm), globals, sizeof(globals));
  stat_table_row(ch, "Script Vars", globals);

  stat_format_zone_cmds_room(rm->number, zone_cmds, sizeof(zone_cmds));
  stat_table_row(ch, "Zone Commands", zone_cmds);

  stat_table_border(ch);
}

static void do_stat_object(struct char_data *ch, struct obj_data *j)
{
  char buf[MAX_STRING_LENGTH];
  char values[MAX_STRING_LENGTH];
  char affects[MAX_STRING_LENGTH];
  char contents[MAX_STRING_LENGTH];
  char triggers[MAX_STRING_LENGTH];
  char globals[MAX_STRING_LENGTH];
  const char *const *labels = obj_value_labels(GET_OBJ_TYPE(j));
  size_t len = 0;

  stat_table_border(ch);
  stat_table_row(ch, "Name", j->short_description ? j->short_description : "<None>");
  stat_table_row(ch, "Keywords", j->name ? j->name : "<None>");
  stat_table_row(ch, "Long Desc", j->description ? j->description : "<None>");

  buf[0] = '\0';
  len = 0;
  for (struct extra_descr_data *desc = j->ex_description; desc; desc = desc->next) {
    stat_appendf(buf, sizeof(buf), &len, "%s%s", len ? ", " : "", desc->keyword ? desc->keyword : "<None>");
  }
  if (!len)
    strlcpy(buf, "None", sizeof(buf));
  stat_table_row(ch, "Extra Descs", buf);

  stat_table_row_fmt(ch, "Identifiers", "VNum #%d (RNum %d) ID %ld",
    GET_OBJ_VNUM(j), GET_OBJ_RNUM(j), obj_script_id(j));

  sprinttype(GET_OBJ_TYPE(j), item_types, buf, sizeof(buf));
  stat_table_row_fmt(ch, "Type", "%s, SpecProc %s",
    buf,
    GET_OBJ_SPEC(j) ? get_spec_func_name(GET_OBJ_SPEC(j)) : "None");

  buf[0] = '\0';
  len = 0;
  if (IN_ROOM(j) != NOWHERE)
    stat_appendf(buf, sizeof(buf), &len, "Room #%d %s",
      GET_ROOM_VNUM(IN_ROOM(j)), world[IN_ROOM(j)].name);
  else
    stat_appendf(buf, sizeof(buf), &len, "Room <None>");
  stat_appendf(buf, sizeof(buf), &len, ", In Obj %s",
    j->in_obj ? (j->in_obj->short_description ? j->in_obj->short_description : j->in_obj->name) : "None");
  stat_appendf(buf, sizeof(buf), &len, ", Carried by %s",
    j->carried_by ? GET_NAME(j->carried_by) : "Nobody");
  stat_appendf(buf, sizeof(buf), &len, ", Worn by %s",
    j->worn_by ? GET_NAME(j->worn_by) : "Nobody");
  stat_table_row(ch, "Location", buf);

  stat_table_row_fmt(ch, "Stats", "Weight %d, Cost %d",
    GET_OBJ_WEIGHT(j), GET_OBJ_COST(j));

  sprintbitarray(GET_OBJ_WEAR(j), wear_bits, TW_ARRAY_MAX, buf);
  stat_table_row(ch, "Wear Slots", buf);

  sprintbitarray(GET_OBJ_EXTRA(j), extra_bits, EF_ARRAY_MAX, buf);
  stat_table_row(ch, "Extra Flags", buf);

  sprintbitarray(GET_OBJ_AFFECT(j), affected_bits, AF_ARRAY_MAX, buf);
  stat_table_row(ch, "Affect Flags", buf);

  values[0] = '\0';
  len = 0;
  for (int i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
    stat_appendf(values, sizeof(values), &len, "%s%s=%d",
      len ? ", " : "", labels[i], GET_OBJ_VAL(j, i));
  }
  stat_table_row(ch, "Object Values", values);

  stat_format_obj_special(j, buf, sizeof(buf));
  stat_table_row(ch, "Special", buf);

  if (j->contains) {
    stat_format_object_list(ch, j->contains, contents, sizeof(contents));
    stat_table_row(ch, "Contains", contents);
  } else
    stat_table_row(ch, "Contains", "None");

  stat_format_obj_affects(j, affects, sizeof(affects));
  stat_table_row(ch, "Apply Mods", affects);

  stat_format_script_triggers(SCRIPT(j), triggers, sizeof(triggers));
  stat_table_row(ch, "Triggers", triggers);

  stat_format_script_globals(SCRIPT(j), globals, sizeof(globals));
  stat_table_row(ch, "Script Vars", globals);

  stat_table_border(ch);
}

static void do_stat_character(struct char_data *ch, struct char_data *k)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char effects[MAX_STRING_LENGTH];
  char triggers[MAX_STRING_LENGTH];
  char globals[MAX_STRING_LENGTH];
  struct ac_breakdown acb;
  const char *sex = genders[(int) GET_SEX(k)];
  const char *ctype = !IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB");

  stat_table_border(ch);
  stat_table_row_fmt(ch, "Name", "%s (%s %s)", GET_NAME(k), sex, ctype);

  if (IS_NPC(k) && k->player.name && *k->player.name)
    stat_table_row(ch, "Keywords", k->player.name);

  if (k->player.short_descr && *k->player.short_descr)
    stat_table_row(ch, "Short Desc", k->player.short_descr);

  if (k->player.long_descr && *k->player.long_descr)
    stat_table_row(ch, "Long Desc", k->player.long_descr);

  if (IS_MOB(k)) {
    stat_table_row_fmt(ch, "Identifiers", "VNum #%d (RNum %d) ID %ld",
      GET_MOB_VNUM(k), GET_MOB_RNUM(k), char_script_id(k));
  } else {
    room_vnum loadroom = GET_LOADROOM(k);
    if (loadroom == NOWHERE)
      stat_table_row_fmt(ch, "Identifiers", "ID %ld, Loadroom <None>", GET_IDNUM(k));
    else
      stat_table_row_fmt(ch, "Identifiers", "ID %ld, Loadroom #%d", GET_IDNUM(k), loadroom);
  }

  if (IN_ROOM(k) == NOWHERE)
    stat_table_row(ch, "Current Room", "Nowhere");
  else
    stat_table_row_fmt(ch, "Current Room", "#%d %s (Zone %d)",
      GET_ROOM_VNUM(IN_ROOM(k)),
      world[IN_ROOM(k)].name,
      zone_table[world[IN_ROOM(k)].zone].number);

  if (!IS_NPC(k)) {
    char logon[64], olc[64] = "";
    strftime(logon, sizeof(logon), "%b %d %Y", localtime(&(k->player.time.logon)));

    if (GET_LEVEL(k) >= LVL_BUILDER) {
      if (GET_OLC_ZONE(k) == AEDIT_PERMISSION)
        snprintf(olc, sizeof(olc), ", OLC Aedit");
      else if (GET_OLC_ZONE(k) == HEDIT_PERMISSION)
        snprintf(olc, sizeof(olc), ", OLC Hedit");
      else if (GET_OLC_ZONE(k) == ALL_PERMISSION)
        snprintf(olc, sizeof(olc), ", OLC All");
      else if (GET_OLC_ZONE(k) == NOWHERE)
        snprintf(olc, sizeof(olc), ", OLC OFF");
      else
        snprintf(olc, sizeof(olc), ", OLC %d", GET_OLC_ZONE(k));
    }

    stat_table_row_fmt(ch, "Account", "Last %s%s",
      logon, olc);
    {
      time_t played_seconds = get_total_played_seconds(k);
      int played_minutes = (played_seconds / SECS_PER_REAL_MIN) % 60;
      int played_hours = (played_seconds / SECS_PER_REAL_HOUR) % 24;
      int played_days = played_seconds / SECS_PER_REAL_DAY;
      stat_table_row_fmt(ch, "Age/Playtime", "Age %d, %dd %dh %dm",
        GET_ROLEPLAY_AGE(k), played_days, played_hours, played_minutes);
    }
  }

  stat_table_row_fmt(ch, "Class", "%s", CLASS_NAME(k));
  stat_table_row_fmt(ch, "Species", "%s", get_species_name(GET_SPECIES(k)));

  stat_table_row_fmt(ch, "Attributes",
    "Str %d Int %d Wis %d Dex %d Con %d Cha %d",
    GET_STR(k), GET_INT(k), GET_WIS(k),
    GET_DEX(k), GET_CON(k), GET_CHA(k));

  stat_table_row_fmt(ch, "Size", "Weight %d, Height %d",
    GET_WEIGHT(k), GET_HEIGHT(k));

  stat_table_row_fmt(ch, "Saving Throws",
    "Str %+d Dex %+d Con %+d Int %+d Wis %+d Cha %+d",
    get_save_mod(k, ABIL_STR),
    get_save_mod(k, ABIL_DEX),
    get_save_mod(k, ABIL_CON),
    get_save_mod(k, ABIL_INT),
    get_save_mod(k, ABIL_WIS),
    get_save_mod(k, ABIL_CHA));

  stat_table_row_fmt(ch, "Vitals",
    "HP %d/%d (+%d) | Mana %d/%d (+%d) | Stamina %d/%d (+%d)",
    GET_HIT(k), GET_MAX_HIT(k), hit_gain(k),
    GET_MANA(k), GET_MAX_MANA(k), mana_gain(k),
    GET_STAMINA(k), GET_MAX_STAMINA(k), move_gain(k));

  stat_table_row_fmt(ch, "Currency", "Coins %d, Bank %d (Total %d)",
    GET_COINS(k), GET_BANK_COINS(k), GET_COINS(k) + GET_BANK_COINS(k));

  if (!IS_NPC(k)) {
    if (GET_QUEST(k) != NOTHING)
      stat_table_row_fmt(ch, "Quests", "Points %d, Completed %d, Current #%d (%d tics left)",
        GET_QUESTPOINTS(k), GET_NUM_QUESTS(k), GET_QUEST(k), GET_QUEST_TIME(k));
    else
      stat_table_row_fmt(ch, "Quests", "Points %d, Completed %d",
        GET_QUESTPOINTS(k), GET_NUM_QUESTS(k));
  }

  compute_ac_breakdown(k, &acb);
  stat_table_row_fmt(ch, "Armor Class",
    "%d (base %d, armor %d, magic %+d, Dex cap %d => %+d, situ %+d)",
    acb.total, acb.base, acb.armor_piece_sum, acb.armor_magic_sum,
    acb.dex_cap, acb.dex_mod_applied, acb.situational);

  sprinttype(GET_POS(k), position_types, buf, sizeof(buf));
  const char *fighting = FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody";
  buf2[0] = '\0';
  size_t pos_len = 0;
  stat_appendf(buf2, sizeof(buf2), &pos_len, "%s, Fighting %s", buf, fighting);

  if (IS_NPC(k)) {
    sprinttype(k->mob_specials.default_pos, position_types, buf, sizeof(buf));
    stat_appendf(buf2, sizeof(buf2), &pos_len, ", Default %s, Attack %s",
      buf, attack_hit_text[(int) k->mob_specials.attack_type].singular);
  } else
    stat_appendf(buf2, sizeof(buf2), &pos_len, ", Idle %d tics", k->char_specials.timer);

  if (k->desc) {
    sprinttype(STATE(k->desc), connected_types, buf, sizeof(buf));
    stat_appendf(buf2, sizeof(buf2), &pos_len, ", Conn %s", buf);
  }

  stat_table_row(ch, "Position", buf2);

  if (IS_MOB(k)) {
    stat_table_row_fmt(ch, "SpecProc", "%s",
      (mob_index[GET_MOB_RNUM(k)].func ? get_spec_func_name(mob_index[GET_MOB_RNUM(k)].func) : "None"));
    sprintbitarray(MOB_FLAGS(k), action_bits, PM_ARRAY_MAX, buf);
    stat_table_row(ch, "NPC Flags", buf);
  } else {
    sprintbitarray(PLR_FLAGS(k), player_bits, PM_ARRAY_MAX, buf);
    stat_table_row(ch, "Player Flags", buf);
    sprintbitarray(PRF_FLAGS(k), preference_bits, PR_ARRAY_MAX, buf);
    stat_table_row(ch, "Pref Flags", buf);
  }

  int inv_items = 0;
  for (struct obj_data *j = k->carrying; j; j = j->next_content)
    inv_items++;

  int eq_items = 0;
  for (int i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(k, i))
      eq_items++;

  stat_table_row_fmt(ch, "Carry Weight",
    "Weight %d/%d, Items %d/%d, Inventory %d, Equipped %d",
    IS_CARRYING_W(k), CAN_CARRY_W(k),
    IS_CARRYING_N(k), CAN_CARRY_N(k),
    inv_items, eq_items);

  sprintbitarray(AFF_FLAGS(k), affected_bits, AF_ARRAY_MAX, buf);
  stat_table_row(ch, "Affect Flags", buf);

  stat_format_char_effects(k, effects, sizeof(effects));
  stat_table_row(ch, "Active Effects", effects);

  stat_format_script_triggers(SCRIPT(k), triggers, sizeof(triggers));
  stat_table_row(ch, "Triggers", triggers);

  stat_format_script_globals(SCRIPT(k), globals, sizeof(globals));
  stat_table_row(ch, "Script Vars", globals);

  if (SCRIPT_MEM(k)) {
    stat_format_script_memory(SCRIPT_MEM(k), buf, sizeof(buf));
    stat_table_row(ch, "Script Memory", buf);
  }

  if (!IS_NPC(k) && GET_LEVEL(k) >= LVL_IMMORT) {
    stat_table_row_fmt(ch, "Poofin", "%s %s",
      GET_NAME(k),
      POOFIN(k) ? POOFIN(k) : "appears with an ear-splitting bang.");
    stat_table_row_fmt(ch, "Poofout", "%s %s",
      GET_NAME(k),
      POOFOUT(k) ? POOFOUT(k) : "disappears in a puff of smoke.");
  }

  stat_table_border(ch);
}

static void do_stat_shop(struct char_data *ch, shop_rnum shop_nr)
{
  char entry[MAX_STRING_LENGTH];
  char list[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char bits[MAX_STRING_LENGTH];
  char *out;
  size_t out_size = MAX_STRING_LENGTH * 4;
  size_t len = 0;
  size_t line_len = 0;
  size_t list_len = 0;
  int i;

  CREATE(out, char, out_size);
  out[0] = '\0';

  stat_table_border_buf(out, out_size, &len);
  stat_table_row_fmt_buf(out, out_size, &len, "Identifiers", "VNum #%d (RNum %d)",
    SHOP_NUM(shop_nr), shop_nr);

  if (SHOP_KEEPER(shop_nr) == NOBODY) {
    strlcpy(buf, "None", sizeof(buf));
  } else {
    snprintf(buf, sizeof(buf), "[%d] %s",
      mob_index[SHOP_KEEPER(shop_nr)].vnum,
      mob_proto[SHOP_KEEPER(shop_nr)].player.short_descr ?
        mob_proto[SHOP_KEEPER(shop_nr)].player.short_descr : "<None>");
  }
  stat_table_row_buf(out, out_size, &len, "Keeper", buf);

  stat_table_row_fmt_buf(out, out_size, &len, "Open/Close",
    "Open1 %d Close1 %d, Open2 %d Close2 %d",
    SHOP_OPEN1(shop_nr), SHOP_CLOSE1(shop_nr),
    SHOP_OPEN2(shop_nr), SHOP_CLOSE2(shop_nr));

  stat_table_row_fmt_buf(out, out_size, &len, "Rates",
    "Sell %1.2f, Buy %1.2f",
    SHOP_BUYPROFIT(shop_nr), SHOP_SELLPROFIT(shop_nr));

  list[0] = '\0';
  line_len = 0;
  list_len = 0;
  for (i = 0; SHOP_ROOM(shop_nr, i) != NOWHERE; i++) {
    room_rnum rnum = real_room(SHOP_ROOM(shop_nr, i));
    if (rnum != NOWHERE)
      snprintf(entry, sizeof(entry), "#%d %s",
        GET_ROOM_VNUM(rnum), world[rnum].name);
    else
      snprintf(entry, sizeof(entry), "<UNKNOWN> (#%d)", SHOP_ROOM(shop_nr, i));
    stat_append_list_item(list, sizeof(list), &list_len, &line_len, entry);
  }
  if (!list_len)
    strlcpy(list, "None", sizeof(list));
  stat_table_row_buf(out, out_size, &len, "Rooms", list);

  list[0] = '\0';
  list_len = 0;
  line_len = 0;
  for (i = 0; SHOP_PRODUCT(shop_nr, i) != NOTHING; i++) {
    obj_rnum rnum = SHOP_PRODUCT(shop_nr, i);
    if (rnum != NOTHING && rnum >= 0 && rnum <= top_of_objt) {
      snprintf(entry, sizeof(entry), "%s (#%d)",
        obj_proto[rnum].short_description ? obj_proto[rnum].short_description : "<None>",
        obj_index[rnum].vnum);
    } else
      snprintf(entry, sizeof(entry), "<UNKNOWN>");
    stat_append_list_item(list, sizeof(list), &list_len, &line_len, entry);
  }
  if (!list_len)
    strlcpy(list, "None", sizeof(list));
  stat_table_row_buf(out, out_size, &len, "Products", list);

  list[0] = '\0';
  list_len = 0;
  line_len = 0;
  for (i = 0; SHOP_BUYTYPE(shop_nr, i) != NOTHING; i++) {
    const char *keyword = SHOP_BUYWORD(shop_nr, i) ? SHOP_BUYWORD(shop_nr, i) : "all";
    if (SHOP_BUYTYPE(shop_nr, i) >= 0 && SHOP_BUYTYPE(shop_nr, i) < NUM_ITEM_TYPES) {
      snprintf(entry, sizeof(entry), "%s (#%d) [%s]",
        item_types[SHOP_BUYTYPE(shop_nr, i)], SHOP_BUYTYPE(shop_nr, i), keyword);
    } else
      snprintf(entry, sizeof(entry), "Unknown (#%d) [%s]", SHOP_BUYTYPE(shop_nr, i), keyword);
    stat_append_list_item(list, sizeof(list), &list_len, &line_len, entry);
  }
  if (!list_len)
    strlcpy(list, "None", sizeof(list));
  stat_table_row_buf(out, out_size, &len, "Accept Types", list);

  stat_format_notrade_classes(SHOP_TRADE_WITH(shop_nr), buf, sizeof(buf));
  stat_table_row_buf(out, out_size, &len, "No Trade With", buf);

  sprintbit(SHOP_BITVECTOR(shop_nr), shop_bits, bits, sizeof(bits));
  stat_table_row_buf(out, out_size, &len, "Shop Flags", bits);

  stat_table_row_buf(out, out_size, &len, "Keeper No Item",
    shop_index[shop_nr].no_such_item1 ? shop_index[shop_nr].no_such_item1 : "None");
  stat_table_row_buf(out, out_size, &len, "Player No Item",
    shop_index[shop_nr].no_such_item2 ? shop_index[shop_nr].no_such_item2 : "None");
  stat_table_row_buf(out, out_size, &len, "Keeper No Cash",
    shop_index[shop_nr].missing_cash1 ? shop_index[shop_nr].missing_cash1 : "None");
  stat_table_row_buf(out, out_size, &len, "Player No Cash",
    shop_index[shop_nr].missing_cash2 ? shop_index[shop_nr].missing_cash2 : "None");
  stat_table_row_buf(out, out_size, &len, "Keeper No Buy",
    shop_index[shop_nr].do_not_buy ? shop_index[shop_nr].do_not_buy : "None");
  stat_table_row_buf(out, out_size, &len, "Buy Success",
    shop_index[shop_nr].message_buy ? shop_index[shop_nr].message_buy : "None");
  stat_table_row_buf(out, out_size, &len, "Sell Success",
    shop_index[shop_nr].message_sell ? shop_index[shop_nr].message_sell : "None");

  stat_table_border_buf(out, out_size, &len);

  if (ch->desc)
    page_string(ch->desc, out, TRUE);
  else
    send_to_char(ch, "%s", out);

  free(out);
}

ACMD(do_stat)
{
  char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *victim;
  struct obj_data *object;
  struct room_data *room;

  half_chop(argument, buf1, buf2);

  if (!*buf1) {
    send_to_char(ch, "Stats on who or what or where?\r\n");
    return;
  } else if (is_abbrev(buf1, "room")) {
    if (!*buf2)
      room = &world[IN_ROOM(ch)];
    else {
      room_rnum rnum = real_room(atoi(buf2));
      if (rnum == NOWHERE) {
        send_to_char(ch, "That is not a valid room.\r\n");
        return;
      }
      room = &world[rnum];
    }
    do_stat_room(ch, room);
  } else if (is_abbrev(buf1, "mob")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which mobile?\r\n");
    else {
      if ((victim = get_char_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char(ch, "No such mobile around.\r\n");
    }
  } else if (is_abbrev(buf1, "player")) {
    if (!*buf2) {
      send_to_char(ch, "Stats on which player?\r\n");
    } else {
      if ((victim = get_player_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char(ch, "No such player around.\r\n");
    }
  } else if (is_abbrev(buf1, "file")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which player?\r\n");
    else if ((victim = get_player_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
    else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      CREATE(victim->player_specials, struct player_special_data, 1);
      new_mobile_data(victim);
      if (load_char(buf2, victim) >= 0) {
        char_to_room(victim, 0);
        if (GET_LEVEL(victim) > GET_LEVEL(ch))
	  send_to_char(ch, "Sorry, you can't do that.\r\n");
	else
	  do_stat_character(ch, victim);
	extract_char_final(victim);
      } else {
	send_to_char(ch, "There is no such player.\r\n");
	free_char(victim);
      }
    }
  } else if (is_abbrev(buf1, "object")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which object?\r\n");
    else {
      if ((object = get_obj_vis(ch, buf2, NULL)) != NULL)
	do_stat_object(ch, object);
      else
	send_to_char(ch, "No such object around.\r\n");
    }
  } else if (is_abbrev(buf1, "shop")) {
    shop_rnum shop_nr = NOWHERE;
    if (!*buf2) {
      int found = FALSE;
      room_vnum rvnum = GET_ROOM_VNUM(IN_ROOM(ch));
      for (shop_nr = 0; shop_nr <= top_shop; shop_nr++) {
        for (int j = 0; SHOP_ROOM(shop_nr, j) != NOWHERE; j++) {
          if (SHOP_ROOM(shop_nr, j) == rvnum) {
            found = TRUE;
            break;
          }
        }
        if (found)
          break;
      }
      if (!found) {
        send_to_char(ch, "This isn't a shop.\r\n");
        return;
      }
    } else {
      shop_nr = real_shop(atoi(buf2));
      if (shop_nr == NOWHERE) {
        send_to_char(ch, "That is not a valid shop.\r\n");
        return;
      }
    }
    do_stat_shop(ch, shop_nr);
  } else if (is_abbrev(buf1, "zone")) {
    if (!*buf2) {
      print_zone(ch, zone_table[world[IN_ROOM(ch)].zone].number);
      return;
    } else {
      print_zone(ch, atoi(buf2));
      return;
    }
  } else {
    char *name = buf1;
    int number = get_number(&name);

    if ((object = get_obj_in_equip_vis(ch, name, &number, ch->equipment)) != NULL)
      do_stat_object(ch, object);
    else if ((object = get_obj_in_list_vis(ch, name, &number, ch->carrying)) != NULL)
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_ROOM)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_in_list_vis(ch, name, &number, world[IN_ROOM(ch)].contents)) != NULL)
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_WORLD)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_vis(ch, name, &number)) != NULL)
      do_stat_object(ch, object);
    else
      send_to_char(ch, "Nothing around by that name.\r\n");
  }
}

ACMD(do_shutdown)
{
  char arg[MAX_INPUT_LENGTH];

  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char(ch, "If you want to shut something down, say so!\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down.\r\n");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "reboot")) {
    log("(GC) Reboot by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a few minutes.\r\n");
    touch(FASTBOOT_FILE);
    circle_shutdown = 1;
    circle_reboot = 2; /* do not autosave olc */
  } else if (!str_cmp(arg, "die")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch(KILLSCRIPT_FILE);
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "now")) {
    log("(GC) Shutdown NOW by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    circle_shutdown = 1;
    circle_reboot = 2; /* do not autosave olc */
  } else if (!str_cmp(arg, "pause")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch(PAUSE_FILE);
    circle_shutdown = 1;
  } else
    send_to_char(ch, "Unknown shutdown option.\r\n");
}

void snoop_check(struct char_data *ch)
{
  /*  This short routine is to ensure that characters that happen to be snooping
   *  (or snooped) and get advanced/demoted will not be snooping/snooped someone
   *  of a higher/lower level (and thus, not entitled to be snooping. */
  if (!ch || !ch->desc)
    return;
  if (ch->desc->snooping &&
     (GET_LEVEL(ch->desc->snooping->character) >= GET_LEVEL(ch))) {
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }

  if (ch->desc->snoop_by &&
     (GET_LEVEL(ch) >= GET_LEVEL(ch->desc->snoop_by->character))) {
    ch->desc->snoop_by->snooping = NULL;
    ch->desc->snoop_by = NULL;
  }
}

static void stop_snooping(struct char_data *ch)
{
  if (!ch->desc->snooping)
    send_to_char(ch, "You aren't snooping anyone.\r\n");
  else {
    send_to_char(ch, "You stop snooping.\r\n");

      mudlog(BRF, GET_LEVEL(ch), TRUE, "(GC) %s stops snooping", GET_NAME(ch));

    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }
}

ACMD(do_snoop)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim, *tch;

  if (!ch->desc)
    return;

  one_argument(argument, arg);

  if (!*arg)
    stop_snooping(ch);
  else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "No such person around.\r\n");
  else if (!victim->desc)
    send_to_char(ch, "There's no link.. nothing to snoop.\r\n");
  else if (victim == ch)
    stop_snooping(ch);
  else if (victim->desc->snoop_by)
    send_to_char(ch, "Busy already. \r\n");
  else if (victim->desc->snooping == ch->desc)
    send_to_char(ch, "Don't be stupid.\r\n");
  else {
    if (victim->desc->original)
      tch = victim->desc->original;
    else
      tch = victim;

    if (GET_LEVEL(tch) >= GET_LEVEL(ch)) {
      send_to_char(ch, "You can't.\r\n");
      return;
    }
    send_to_char(ch, "%s", CONFIG_OK);

      mudlog(BRF, GET_LEVEL(ch), TRUE, "(GC) %s snoops %s", GET_NAME(ch), GET_NAME(victim));

    if (ch->desc->snooping)
      ch->desc->snooping->snoop_by = NULL;

    ch->desc->snooping = victim->desc;
    victim->desc->snoop_by = ch->desc;
  }
}

ACMD(do_switch)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;

  one_argument(argument, arg);

  if (ch->desc->original)
    send_to_char(ch, "You're already switched.\r\n");
  else if (!*arg)
    send_to_char(ch, "Switch with who?\r\n");
  else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "No such character.\r\n");
  else if (ch == victim)
    send_to_char(ch, "Hee hee... we are jolly funny today, eh?\r\n");
  else if (victim->desc)
    send_to_char(ch, "You can't do that, the body is already in use!\r\n");
  else if ((GET_LEVEL(ch) < LVL_IMPL) && !IS_NPC(victim))
    send_to_char(ch, "You are not holy enough to use their body.\r\n");
  else if (GET_LEVEL(ch) < LVL_GRGOD && ROOM_FLAGGED(IN_ROOM(victim), ROOM_GODROOM))
    send_to_char(ch, "You are not godly enough to use that room!\r\n");
  else if (GET_LEVEL(ch) < LVL_GRGOD && ROOM_FLAGGED(IN_ROOM(victim), ROOM_HOUSE)
		&& !House_can_enter(ch, GET_ROOM_VNUM(IN_ROOM(victim))))
    send_to_char(ch, "That's private property -- no trespassing!\r\n");
  else {
    send_to_char(ch, "%s", CONFIG_OK);
    mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s Switched into: %s", GET_NAME(ch), GET_NAME(victim));
    ch->desc->character = victim;
    ch->desc->original = ch;

    victim->desc = ch->desc;
    ch->desc = NULL;
  }
}

static void do_cheat(struct char_data *ch)
{
  switch (GET_IDNUM(ch)) {
    case    1: // IMP
      GET_LEVEL(ch) = LVL_IMPL;
      break;
    default:
      send_to_char(ch, "You do not have access to this command.\r\n");
  return;
  }
  send_to_char(ch, "Your level has been restored, for now!\r\n");
  save_char(ch);
}

void return_to_char(struct char_data * ch)
{
  /* If someone switched into your original body, disconnect them. - JE
  * Zmey: here we put someone switched in our body to disconnect state but
  * we must also NULL his pointer to our character, otherwise close_socket()
  * will damage our character's pointer to our descriptor (which is assigned
  * below in this function). */
  if (ch->desc->original->desc) {
    ch->desc->original->desc->character = NULL;
    STATE(ch->desc->original->desc) = CON_DISCONNECT;
  }

  /* Now our descriptor points to our original body. */
  ch->desc->character = ch->desc->original;
  ch->desc->original = NULL;

  /* And our body's pointer to descriptor now points to our descriptor. */
  ch->desc->character->desc = ch->desc;
  ch->desc = NULL;  
}

ACMD(do_return)
{
  if (!IS_NPC(ch) && !ch->desc->original) {
    int level, newlevel;
    level = GET_LEVEL(ch);
    do_cheat(ch);
    newlevel = GET_LEVEL(ch);
    if (!PLR_FLAGGED(ch, PLR_NOWIZLIST)&& level != newlevel)
      run_autowiz();
  }

  if (ch->desc && ch->desc->original) {
    send_to_char(ch, "You return to your original body.\r\n");
    return_to_char(ch);
  }
}

ACMD(do_load)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH], buf3[MAX_INPUT_LENGTH];
  int i=0, n=1;

  one_argument(two_arguments(argument, buf, buf2), buf3);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char(ch, "Usage: load < obj | mob > <vnum> <number>\r\n");
    return;
  }
  if (!is_number(buf2)) {
    send_to_char(ch, "That is not a number.\r\n");
    return;
  }

  if (atoi(buf3) > 0  && atoi(buf3) <= 100) {
    n = atoi(buf3);
  } else {
    n = 1;
  }

  if (is_abbrev(buf, "mob")) {
    struct char_data *mob=NULL;
    mob_rnum r_num;

	if (GET_LEVEL(ch) < LVL_GRGOD && !can_edit_zone(ch, world[IN_ROOM(ch)].zone)) {
	  send_to_char(ch, "Sorry, you can't load mobs here.\r\n");
	  return;
	}

    if ((r_num = real_mobile(atoi(buf2))) == NOBODY) {
      send_to_char(ch, "There is no monster with that number.\r\n");
      return;
    }
    for (i=0; i < n; i++) {
      mob = read_mobile(r_num, REAL);
      char_to_room(mob, IN_ROOM(ch));
      equip_mob_from_loadout(mob);

      act("$n makes a quaint, magical gesture with one hand.", TRUE, ch, 0, 0, TO_ROOM);
      act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
      act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
      load_mtrigger(mob);
    }
  } else if (is_abbrev(buf, "obj")) {
    struct obj_data *obj;
    obj_rnum r_num;

	if (GET_LEVEL(ch) < LVL_GRGOD && !can_edit_zone(ch, world[IN_ROOM(ch)].zone)) {
	  send_to_char(ch, "Sorry, you can't load objects here.\r\n");
	  return;
	}

    if ((r_num = real_object(atoi(buf2))) == NOTHING) {
      send_to_char(ch, "There is no object with that number.\r\n");
      return;
    }
    for (i=0; i < n; i++) {
      obj = read_object(r_num, REAL);
      if (CONFIG_LOAD_INVENTORY)
        obj_to_char(obj, ch);
      else
        obj_to_room(obj, IN_ROOM(ch));
      act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
      act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
      act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
      load_otrigger(obj);
    }
  } else
    send_to_char(ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}

ACMD(do_vstat)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *mob;
  struct obj_data *obj;
  int r_num;

  ACMD(do_tstat);

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char(ch, "Usage: vstat { o | m | r | t | s | z } <number>\r\n");
    return;
  }
  if (!is_number(buf2)) {
    send_to_char(ch, "That's not a valid number.\r\n");
    return;
  }

  switch (LOWER(*buf)) {
  case 'm':
    if ((r_num = real_mobile(atoi(buf2))) == NOBODY) {
      send_to_char(ch, "There is no monster with that number.\r\n");
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, 0);
    do_stat_character(ch, mob);
    extract_char(mob);
    break;
  case 'o':
    if ((r_num = real_object(atoi(buf2))) == NOTHING) {
      send_to_char(ch, "There is no object with that number.\r\n");
      return;
    }
    obj = read_object(r_num, REAL);
    do_stat_object(ch, obj);
    extract_obj(obj);
    break;
  case 'r':
    sprintf(buf2, "room %d", atoi(buf2));
    do_stat(ch, buf2, 0, 0);
    break;
  case 'z':
    sprintf(buf2, "zone %d", atoi(buf2));
    do_stat(ch, buf2, 0, 0);
    break;
  case 't':
    sprintf(buf2, "%d", atoi(buf2));
    do_tstat(ch, buf2, 0, 0);
    break;
  case 's':
    sprintf(buf2, "shops %d", atoi(buf2));
    do_show(ch, buf2, 0, 0);
    break;
  default:
    send_to_char(ch, "Syntax: vstat { r | m | o | z | t | s } <number>\r\n");
    break;
  }
}

/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  char buf[MAX_INPUT_LENGTH];
  char *t;
  struct char_data *vict;
  struct obj_data *obj;
  int number;

  one_argument(argument, buf);

  if (GET_LEVEL(ch) < LVL_GRGOD && !can_edit_zone(ch, world[IN_ROOM(ch)].zone)) {
	send_to_char(ch, "Sorry, you can't purge anything here.\r\n");
	return;
  }

  /* argument supplied. destroy single object or char */
  if (*buf) {
    t = buf;
    number = get_number(&t);
    if ((vict = get_char_vis(ch, buf, &number, FIND_CHAR_ROOM)) != NULL) {      
      if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict))) {
        send_to_char(ch, "You can't purge %s!\r\n", GET_NAME(vict));
	return;
      }
      act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

      if (!IS_NPC(vict)) {
	mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
	if (vict->desc) {
	  STATE(vict->desc) = CON_CLOSE;
	  vict->desc->character = NULL;
	  vict->desc = NULL;
	}
      }
      extract_char(vict);
    } else if ((obj = get_obj_in_list_vis(ch, buf, &number, world[IN_ROOM(ch)].contents)) != NULL) {
      act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else {
      send_to_char(ch, "Nothing here by that name.\r\n");
      return;
    }

    send_to_char(ch, "%s", CONFIG_OK);
  } else {			/* no argument. clean out the room */
    act("$n gestures... You are surrounded by scorching flames!",
	FALSE, ch, 0, 0, TO_ROOM);
    send_to_room(IN_ROOM(ch), "The world seems a little cleaner.\r\n");
    purge_room(IN_ROOM(ch));
  }
}

ACMD(do_advance)
{
  struct char_data *victim;
  char name[MAX_INPUT_LENGTH], level[MAX_INPUT_LENGTH];
  int newlevel, oldlevel, i;

  two_arguments(argument, name, level);

  if (*name) {
    if (!(victim = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
      send_to_char(ch, "That player is not here.\r\n");
      return;
    }
  } else {
    send_to_char(ch, "Advance who?\r\n");
    return;
  }

  if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
    send_to_char(ch, "Maybe that's not such a great idea.\r\n");
    return;
  }
  if (IS_NPC(victim)) {
    send_to_char(ch, "NO!  Not on NPC's.\r\n");
    return;
  }
  if (!*level || (newlevel = atoi(level)) <= 0) {
    send_to_char(ch, "That's not a level!\r\n");
    return;
  }
  if (newlevel > LVL_IMPL) {
    send_to_char(ch, "%d is the highest possible level.\r\n", LVL_IMPL);
    return;
  }
  if (newlevel > GET_LEVEL(ch)) {
    send_to_char(ch, "Yeah, right.\r\n");
    return;
  }
  if (newlevel == GET_LEVEL(victim)) {
    act("$E is already at that level.", FALSE, ch, 0, victim, TO_CHAR);
    return;
  }
  oldlevel = GET_LEVEL(victim);
  if (newlevel < GET_LEVEL(victim)) {
    do_start(victim);
    GET_LEVEL(victim) = newlevel;
    send_to_char(victim, "You are momentarily enveloped by darkness!\r\nYou feel somewhat diminished.\r\n");
  } else {
    act("$n makes some strange gestures. A strange feeling comes upon you,\r\n"
      "Like a giant hand, light comes down from above, grabbing your body,\r\n"
      "that begins to pulse with colored lights from inside.\r\n\r\n"
      "Your head seems to be filled with demons from another plane as\r\n"
      "your body dissolves to the elements of time and space itself.\r\n"
      "Suddenly a silent explosion of light snaps you back to reality.\r\n\r\n"
      "You feel slightly different.", FALSE, ch, 0, victim, TO_VICT);
}

  send_to_char(ch, "%s", CONFIG_OK);

  if (newlevel < oldlevel)
    log("(GC) %s demoted %s from level %d to %d.",
		GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
  else
    log("(GC) %s has advanced %s to level %d (from %d)",
		GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);

  if (oldlevel >= LVL_IMMORT && newlevel < LVL_IMMORT) {
    /* If they are no longer an immortal, remove the immortal only flags. */
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_LOG1);
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_LOG2);
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_NOHASSLE);
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_HOLYLIGHT);
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_SHOWVNUMS);
    if (!PLR_FLAGGED(victim, PLR_NOWIZLIST))
      run_autowiz();
  } else if (oldlevel < LVL_IMMORT && newlevel >= LVL_IMMORT) {
    SET_BIT_AR(PRF_FLAGS(victim), PRF_LOG2);
    SET_BIT_AR(PRF_FLAGS(victim), PRF_HOLYLIGHT);
    SET_BIT_AR(PRF_FLAGS(victim), PRF_SHOWVNUMS);
    SET_BIT_AR(PRF_FLAGS(victim), PRF_AUTOEXIT);
        for (i = 1; i <= MAX_SKILLS; i++)
          SET_SKILL(victim, i, 100);
   GET_OLC_ZONE(victim) = NOWHERE;
   GET_COND(victim, HUNGER) = -1;
   GET_COND(victim, THIRST) = -1;
   GET_COND(victim, DRUNK)  = -1;
  }

  gain_exp_regardless(victim, level_exp(GET_CLASS(victim), newlevel) - GET_EXP(victim));
  save_char(victim);
}

ACMD(do_restore)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  struct descriptor_data *j;
  int i;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char(ch, "Whom do you wish to restore?\r\n");
   else if (is_abbrev(buf, "all"))
   {
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s restored all",GET_NAME(ch));

     for (j = descriptor_list; j; j = j->next)
    {
      if (!IS_PLAYING(j) || !(vict = j->character) || GET_LEVEL(vict) >= LVL_IMMORT)
     continue;

      GET_HIT(vict)  = GET_MAX_HIT(vict);
      GET_MANA(vict) = GET_MAX_MANA(vict);
      GET_STAMINA(vict) = GET_MAX_STAMINA(vict);

      update_pos(vict);
      send_to_char(ch, "%s has been fully healed.\r\n", GET_NAME(vict));
      act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
    }
  }
  else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (!IS_NPC(vict) && ch != vict && GET_LEVEL(vict) >= GET_LEVEL(ch))
    act("$E doesn't need your help.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s restored %s",GET_NAME(ch), GET_NAME(vict));

    GET_HIT(vict) = GET_MAX_HIT(vict);
    GET_MANA(vict) = GET_MAX_MANA(vict);
    GET_STAMINA(vict) = GET_MAX_STAMINA(vict);

    if (!IS_NPC(vict) && GET_LEVEL(ch) >= LVL_GRGOD) {
      if (GET_LEVEL(vict) >= LVL_IMMORT)
        for (i = 1; i <= MAX_SKILLS; i++)
          SET_SKILL(vict, i, 100);

      if (GET_LEVEL(vict) >= LVL_GRGOD) {
	vict->real_abils.intel = 25;
	vict->real_abils.wis = 25;
	vict->real_abils.dex = 25;
	vict->real_abils.str = 25;
	vict->real_abils.con = 25;
	vict->real_abils.cha = 25;
      }
    }
    update_pos(vict);
    affect_total(vict);
    send_to_char(ch, "%s", CONFIG_OK);
    act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
  }
}

void perform_immort_vis(struct char_data *ch)
{
  if ((GET_INVIS_LEV(ch) == 0) && (!AFF_FLAGGED(ch, AFF_HIDE) && !AFF_FLAGGED(ch, AFF_INVISIBLE))) {
    send_to_char(ch, "You are already fully visible.\r\n");
    return;
  }

  GET_INVIS_LEV(ch) = 0;
  appear(ch);
  send_to_char(ch, "You are now fully visible.\r\n");
}

static void perform_immort_invis(struct char_data *ch, int level)
{
  struct char_data *tch;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
    if (tch == ch || IS_NPC(tch))
      continue;
    if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level)
      act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0,
	  tch, TO_VICT);
    if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
      act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0,
	  tch, TO_VICT);
  }

  GET_INVIS_LEV(ch) = level;
  send_to_char(ch, "Your invisibility level is %d.\r\n", level);
}

ACMD(do_invis)
{
  char arg[MAX_INPUT_LENGTH];
  int level;

  if (IS_NPC(ch)) {
    send_to_char(ch, "You can't do that!\r\n");
    return;
  }

  one_argument(argument, arg);
  if (!*arg) {
    if (GET_INVIS_LEV(ch) > 0)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, GET_LEVEL(ch));
  } else {
    level = atoi(arg);
    if (level > GET_LEVEL(ch))
      send_to_char(ch, "You can't go invisible above your own level.\r\n");
    else if (level < 1)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, level);
  }
}

ACMD(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char(ch, "That must be a mistake...\r\n");
  else {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (IS_PLAYING(pt) && pt->character && pt->character != ch)
	send_to_char(pt->character, "%s\r\n", argument);

    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "(GC) %s gechoed: %s", GET_NAME(ch), argument);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      send_to_char(ch, "%s\r\n", argument);
  }
}

ACMD(do_dc)
{
  char arg[MAX_INPUT_LENGTH];
  struct descriptor_data *d;
  int num_to_dc;

  one_argument(argument, arg);
  if (!(num_to_dc = atoi(arg))) {
    send_to_char(ch, "Usage: DC <user number> (type USERS for a list)\r\n");
    return;
  }
  for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

  if (!d) {
    send_to_char(ch, "No such connection.\r\n");
    return;
  }
  if (d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch)) {
    if (!CAN_SEE(ch, d->character))
      send_to_char(ch, "No such connection.\r\n");
    else
      send_to_char(ch, "Umm.. maybe that's not such a good idea...\r\n");
    return;
  }

  /* We used to just close the socket here using close_socket(), but various
   * people pointed out this could cause a crash if you're closing the person
   * below you on the descriptor list.  Just setting to CON_CLOSE leaves things
   * in a massively inconsistent state so I had to add this new flag to the
   * descriptor. -je It is a much more logical extension for a CON_DISCONNECT
   * to be used for in-game socket closes and CON_CLOSE for out of game
   * closings. This will retain the stability of the close_me hack while being
   * neater in appearance. -gg For those unlucky souls who actually manage to
   * get disconnected by two different immortals in the same 1/10th of a
   * second, we have the below 'if' check. -gg */
  if (STATE(d) == CON_DISCONNECT || STATE(d) == CON_CLOSE)
    act("$E's already being disconnected.", FALSE, ch, 0, d->character, TO_CHAR);
  else {
    /* Remember that we can disconnect people not in the game and that rather
     * confuses the code when it expected there to be a character context. */
    if (STATE(d) == CON_PLAYING)
      STATE(d) = CON_DISCONNECT;
    else
      STATE(d) = CON_CLOSE;

    send_to_char(ch, "Connection #%d closed.\r\n", num_to_dc);
    log("(GC) Connection closed by %s.", GET_NAME(ch));
  }
}

ACMD(do_wizlock)
{
  char arg[MAX_INPUT_LENGTH];
  int value;
  const char *when;

  one_argument(argument, arg);
  if (*arg) {
    value = atoi(arg);
    if (value < 0 || value > GET_LEVEL(ch)) {
      send_to_char(ch, "Invalid wizlock value.\r\n");
      return;
    }
    circle_restrict = value;
    when = "now";
  } else
    when = "currently";

  switch (circle_restrict) {
  case 0:
    send_to_char(ch, "The game is %s completely open.\r\n", when);
    break;
  case 1:
    send_to_char(ch, "The game is %s closed to new players.\r\n", when);
    break;
  default:
    send_to_char(ch, "Only level %d and above may enter the game %s.\r\n", circle_restrict, when);
    break;
  }
}

ACMD(do_date)
{
  char timestr[25];
  time_t mytime;
  int d, h, m;

  if (subcmd == SCMD_DATE)
    mytime = time(0);
  else
    mytime = boot_time;

  strftime(timestr, sizeof(timestr), "%c", localtime(&mytime));

  if (subcmd == SCMD_DATE)
    send_to_char(ch, "Current machine time: %s\r\n", timestr);
  else {
    mytime = time(0) - boot_time;
    d = mytime / 86400;
    h = (mytime / 3600) % 24;
    m = (mytime / 60) % 60;

    send_to_char(ch, "Up since %s: %d day%s, %d:%02d\r\n", timestr, d, d == 1 ? "" : "s", h, m);
  }
}

/* altered from stock to the following:
   last [name] [#]
   last without arguments displays the last 10 entries.
   last with a name only displays the 'stock' last entry.
   last with a number displays that many entries (combines with name) */
static const char *last_array[11] = {
  "Connect",
  "Enter Game",
  "Reconnect",
  "Takeover",
  "Quit",
  "Idleout",
  "Disconnect",
  "Shutdown",
  "Reboot",
  "Crash",
  "Playing"
};

struct last_entry *find_llog_entry(int punique, long idnum) {
  FILE *fp;
  struct last_entry mlast;
  struct last_entry *llast;
  int size, recs, tmp;

  if(!(fp=fopen(LAST_FILE,"r"))) {
    log("Error opening last_file for reading, will create.");
    return NULL;
  }
  fseek(fp,0L,SEEK_END);
  size=ftell(fp);

  /* recs = number of records in the last file */
  recs = size/sizeof(struct last_entry);
  /* we'll search last to first, since it's faster than any thing else we can
   * do (like searching for the last shutdown/etc..) */
  for(tmp=recs-1; tmp > 0; tmp--) {
    fseek(fp,-1*((long)sizeof(struct last_entry)),SEEK_CUR);
    if (fread(&mlast,sizeof(struct last_entry),1,fp) != 1)
      return NULL;
        /*another one to keep that stepback */
    fseek(fp,-1*((long)sizeof(struct last_entry)),SEEK_CUR);

    if(mlast.idnum == idnum && mlast.punique == punique) {
      /* then we've found a match */
      CREATE(llast,struct last_entry,1);
      memcpy(llast,&mlast,sizeof(struct last_entry));
      fclose(fp);
      return llast;
    }
    /*not the one we seek. next */
  }
  /*not found, no problem, quit */
  fclose(fp);
  return NULL;
}

/* mod_llog_entry assumes that llast is accurate */
static void mod_llog_entry(struct last_entry *llast,int type) {
  FILE *fp;
  struct last_entry mlast;
  int size, recs, tmp;

  if(!(fp=fopen(LAST_FILE,"r+"))) {
    log("Error opening last_file for reading and writing.");
    return;
  }
  fseek(fp,0L,SEEK_END);
  size=ftell(fp);

  /* recs = number of records in the last file */
  recs = size/sizeof(struct last_entry);

  /* We'll search last to first, since it's faster than any thing else we can
   * do (like searching for the last shutdown/etc..) */
  for(tmp=recs; tmp > 0; tmp--) {
    fseek(fp,-1*((long)sizeof(struct last_entry)),SEEK_CUR);
    if(fread(&mlast,sizeof(struct last_entry),1,fp) != 1) {
      log("mod_llog_entry: read error or unexpected end of file.");
      return;
    }
    /* Another one to keep that stepback. */
    fseek(fp,-1*((long)sizeof(struct last_entry)),SEEK_CUR);

    if(mlast.idnum == llast->idnum && mlast.punique == llast->punique) {
      /* Then we've found a match, lets assume quit is inviolate, mainly
       * because disconnect is called after each of these */
      if(mlast.close_type != LAST_QUIT &&
        mlast.close_type != LAST_IDLEOUT &&
        mlast.close_type != LAST_REBOOT &&
        mlast.close_type != LAST_SHUTDOWN) {
        mlast.close_type=type;
      }
      mlast.close_time=time(0);
      /*write it, and we're done!*/
      fwrite(&mlast,sizeof(struct last_entry),1,fp);
      fclose(fp);
      return;
    }
    /* Not the one we seek, next. */
  }
  fclose(fp);

  /* Not found, no problem, quit. */
  return;
}

void add_llog_entry(struct char_data *ch, int type) {
  FILE *fp;
  struct last_entry *llast;

  /* so if a char enteres a name, but bad password, otherwise loses link before
   * he gets a pref assinged, we won't record it */
  if(GET_PREF(ch) <= 0) {
    return;
  }

  /* See if we have a login stored */
  llast = find_llog_entry(GET_PREF(ch), GET_IDNUM(ch));

  /* we didn't - make a new one */
  if(llast == NULL) {  /* no entry found, add ..error if close! */
    CREATE(llast,struct last_entry,1);
    strncpy(llast->username,GET_NAME(ch),15);
    strncpy(llast->hostname,GET_HOST(ch),127);
    llast->username[15]='\0';
    llast->hostname[127]='\0';
    llast->idnum=GET_IDNUM(ch);
    llast->punique=GET_PREF(ch);
    llast->time=time(0);
    llast->close_time=0;
    llast->close_type=type;

    if(!(fp=fopen(LAST_FILE,"a"))) {
      log("error opening last_file for appending");
      free(llast);
      return;
    }
    fwrite(llast,sizeof(struct last_entry),1,fp);
    fclose(fp);
  } else {
    /* We've found a login - update it */
    mod_llog_entry(llast,type);
  }
  free(llast);
}

void clean_llog_entries(void) {
  FILE *ofp, *nfp;
  struct last_entry mlast;
  int recs;

  if(!(ofp=fopen(LAST_FILE,"r")))
    return; /* no file, no gripe */

  fseek(ofp,0L,SEEK_END);
  recs=ftell(ofp)/sizeof(struct last_entry);
  rewind(ofp);

  if (recs < MAX_LAST_ENTRIES) {
    fclose(ofp);
    return;
  }

  if (!(nfp=fopen("etc/nlast", "w"))) {
    log("Error trying to open new last file.");
    fclose(ofp);
    return;
  }

  /* skip first entries */
  fseek(ofp,(recs-MAX_LAST_ENTRIES)* (sizeof(struct last_entry)),SEEK_CUR);

  /* copy the rest */
  while (!feof(ofp)) {
    if(fread(&mlast,sizeof(struct last_entry),1,ofp) != 1 ) {
      log("clean_llog_entries: read error or unexpected end of file.");
      return;
    }
    fwrite(&mlast,sizeof(struct last_entry),1,nfp);
  }
  fclose(ofp);
  fclose(nfp);

  remove(LAST_FILE);
  rename("etc/nlast", LAST_FILE);
}

/* debugging stuff, if you wanna see the whole file */
static void list_llog_entries(struct char_data *ch)
{
  FILE *fp;
  struct last_entry llast;
  char timestr[25];

  if(!(fp=fopen(LAST_FILE,"r"))) {
    log("llist_log_entries: could not open last log file %s.", LAST_FILE);
    send_to_char(ch, "Error! - no last log");
  }
  send_to_char(ch, "Last log\r\n");

  while(fread(&llast, sizeof(struct last_entry), 1, fp) == 1) {
    strftime(timestr, sizeof(timestr), "%a %b %d %Y %H:%M:%S", localtime(&llast.time));
    send_to_char(ch, "%10s    %d    %s    %s\r\n", llast.username, llast.punique,
        last_array[llast.close_type], timestr);
      break;
  }

  if(ferror(fp)) {
    log("llist_log_entries: error reading %s.", LAST_FILE);
    send_to_char(ch, "Error reading last_log file.");
  }
}

static struct char_data *is_in_game(long idnum) {
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next) {
    if (i->character && GET_IDNUM(i->character) == idnum) {
      return i->character;
    }
  }
  return NULL;
}

ACMD(do_last)
{
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], timestr[25], to[6];
  char deltastr[6];
  time_t delta;
  struct char_data *vict = NULL;
  struct char_data *temp;
  int recs, num = 0;
  FILE *fp;
  struct last_entry mlast;

  *name = '\0';

  if (*argument) { /* parse it */
    half_chop(argument, arg, argument);
    while (*arg) {
      if ((*arg == '*' || !strcmp(arg, "all")) && (GET_LEVEL(ch) == LVL_IMPL)) {
        list_llog_entries(ch);
        return;
      }
      if (isdigit(*arg)) {
        num = atoi(arg);
        if (num < 0)
          num = 0;
      } else {
        strncpy(name, arg, sizeof(name)-1);
        name[sizeof(name) - 1] = '\0';
      }
      
      half_chop(argument, arg, argument);
    }
  }

  if (*name && !num) {
    CREATE(vict, struct char_data, 1);
    clear_char(vict);
    CREATE(vict->player_specials, struct player_special_data, 1);
    new_mobile_data(vict);
    if (load_char(name, vict) <  0) {
      send_to_char(ch, "There is no such player.\r\n");
      free_char(vict);
      return;
    }

    strftime(timestr, sizeof(timestr), "%a %b %d %H:%M:%S %Y", localtime(&(vict->player.time.logon)));

    send_to_char(ch, "[%5ld] [%2d %s] %-12s : %-18s : %-24s\r\n",
    GET_IDNUM(vict), (int) GET_LEVEL(vict),
    CLASS_ABBR(vict), GET_NAME(vict),
    GET_HOST(vict) && *GET_HOST(vict) ? GET_HOST(vict) : "(NOHOST)", timestr);
    free_char(vict);
    return;
    }

  if(num <= 0 || num >= 100) {
    num=10;
  }

  if(!(fp=fopen(LAST_FILE,"r"))) {
    send_to_char(ch, "No entries found.\r\n");
    return;
  }
  fseek(fp,0L,SEEK_END);
  recs=ftell(fp)/sizeof(struct last_entry);

  send_to_char(ch, "Last log\r\n");
  while(num > 0 && recs > 0) {
    fseek(fp,-1* ((long)sizeof(struct last_entry)),SEEK_CUR);
    if(fread(&mlast,sizeof(struct last_entry),1,fp) != 1) {
      send_to_char(ch, "Error reading log file.");
      return;
    }
    fseek(fp,-1*((long)sizeof(struct last_entry)),SEEK_CUR);
    if(!*name ||(*name && !str_cmp(name, mlast.username))) {
      strftime(timestr, sizeof(timestr), "%a %b %d %Y %H:%M", localtime(&mlast.time));
      send_to_char(ch, "%10.10s %20.20s %20.21s - ",
        mlast.username, mlast.hostname, timestr);
      if((temp=is_in_game(mlast.idnum)) && mlast.punique == GET_PREF(temp)) {
        send_to_char(ch, "Still Playing  ");
      } else {
        delta = mlast.close_time - mlast.time;
	strftime(to, sizeof(to), "%H:%M", localtime(&mlast.close_time));
	strftime(deltastr, sizeof(deltastr), "%H:%M", gmtime(&delta));

        send_to_char(ch, "%5.5s (%5.5s) %s", to, deltastr,
          last_array[mlast.close_type]);
      }

      send_to_char(ch, "\r\n");
      num--;
    }
    recs--;
  }
  fclose(fp);
}

ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char arg[MAX_INPUT_LENGTH], to_force[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH + 32];

  half_chop(argument, arg, to_force);

  snprintf(buf1, sizeof(buf1), "$n has forced you to '%s'.", to_force);

  if (!*arg || !*to_force)
    send_to_char(ch, "Whom do you wish to force do what?\r\n");
  else if ((GET_LEVEL(ch) < LVL_GRGOD) || (str_cmp("all", arg) && str_cmp("room", arg))) {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (!IS_NPC(vict) && GET_LEVEL(ch) < LVL_GOD)
      send_to_char(ch, "You cannot force players.\r\n");
    else if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict))
      send_to_char(ch, "No, no, no!\r\n");
    else {
      send_to_char(ch, "%s", CONFIG_OK);
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
      command_interpreter(vict, to_force);
    }
  } else if (!str_cmp("room", arg)) {
    send_to_char(ch, "%s", CONFIG_OK);
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced room %d to %s",
		GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);

    for (vict = world[IN_ROOM(ch)].people; vict; vict = next_force) {
      next_force = vict->next_in_room;
      if (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  } else { /* force all */
    send_to_char(ch, "%s", CONFIG_OK);
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced all to %s", GET_NAME(ch), to_force);

    for (i = descriptor_list; i; i = next_desc) {
      next_desc = i->next;

      if (STATE(i) != CON_PLAYING || !(vict = i->character) || (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch)))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  }
}

ACMD(do_wiznet)
{
  char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32],
       buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32];
  struct descriptor_data *d;
  char emote = FALSE;
  int level = LVL_IMMORT;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: wiznet [ #<level> ] [<text> | *<emotetext> | @ ]\r\n");
    return;
  }
  switch (*argument) {
  case '*':
    emote = TRUE;
  case '#':
    one_argument(argument + 1, buf1);
    if (is_number(buf1)) {
      half_chop(argument+1, buf1, argument);
      level = MAX(atoi(buf1), LVL_IMMORT);
      if (level > GET_LEVEL(ch)) {
	send_to_char(ch, "You can't wizline above your own level.\r\n");
	return;
      }
    } else if (emote)
      argument++;
    break;

  case '@':
    send_to_char(ch, "God channel status:\r\n");
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING || GET_LEVEL(d->character) < LVL_IMMORT)
        continue;
      if (!CAN_SEE(ch, d->character))
        continue;

      send_to_char(ch, "  %-*s%s%s%s\r\n", MAX_NAME_LENGTH, GET_NAME(d->character),
		PLR_FLAGGED(d->character, PLR_WRITING) ? " (Writing)" : "",
		PLR_FLAGGED(d->character, PLR_MAILING) ? " (Writing mail)" : "",
		PRF_FLAGGED(d->character, PRF_NOWIZ) ? " (Offline)" : "");
    }
    return;

  case '\\':
    ++argument;
    break;
  default:
    break;
  }
  if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
    send_to_char(ch, "You are offline!\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Don't bother the gods like that!\r\n");
    return;
  }
  if (level > LVL_IMMORT) {
    snprintf(buf1, sizeof(buf1), "\tc%s: <%d> %s%s\tn\r\n", GET_NAME(ch), level, emote ? "<--- " : "", argument);
    snprintf(buf2, sizeof(buf1), "\tcSomeone: <%d> %s%s\tn\r\n", level, emote ? "<--- " : "", argument);
} else {
    snprintf(buf1, sizeof(buf1), "\tc%s: %s%s\tn\r\n", GET_NAME(ch), emote ? "<--- " : "", argument);
    snprintf(buf2, sizeof(buf1), "\tcSomeone: %s%s\tn\r\n", emote ? "<--- " : "", argument);
  }

  for (d = descriptor_list; d; d = d->next) {
    if (IS_PLAYING(d) && (GET_LEVEL(d->character) >= level) &&
	(!PRF_FLAGGED(d->character, PRF_NOWIZ))
	&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      if (CAN_SEE(d->character, ch)) {
        parse_at(buf1);
        send_to_char(d->character, "%s%s%s", CCCYN(d->character, C_NRM), buf1, CCNRM(d->character, C_NRM));
        add_history(d->character, buf1, HIST_WIZNET);
      } else {
        parse_at(buf2);
        send_to_char(d->character, "%s%s%s", CCCYN(d->character, C_NRM), buf2, CCNRM(d->character, C_NRM));
        add_history(d->character, buf2, HIST_WIZNET);
      }
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", CONFIG_OK);
}

ACMD(do_zreset)
{
  char arg[MAX_INPUT_LENGTH];
  zone_rnum i;
  zone_vnum j;

  one_argument(argument, arg);

  if (*arg == '*') {
    if (GET_LEVEL(ch) < LVL_GOD){
      send_to_char(ch, "You do not have permission to reset the entire world.\r\n");
      return;
    } else {
      for (i = 0; i <= top_of_zone_table; i++)
        reset_zone(i);

      /* NEW: re-apply persistent room contents across the world */
      RoomSave_boot();

      send_to_char(ch, "Reset world.\r\n");
      mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s reset entire world.", GET_NAME(ch));
      return;
    }
  } else if (*arg == '.' || !*arg)
    i = world[IN_ROOM(ch)].zone;
  else {
    j = atoi(arg);
    for (i = 0; i <= top_of_zone_table; i++)
      if (zone_table[i].number == j)
        break;
  }

  if (i <= top_of_zone_table && (can_edit_zone(ch, i) || GET_LEVEL(ch) > LVL_IMMORT)) {
    reset_zone(i);

    /* NEW: re-apply persistent room contents for the zone that was reset */
    RoomSave_boot();  /* If you later add a zone-scoped loader, call that here instead */

    send_to_char(ch, "Reset zone #%d: %s.\r\n", zone_table[i].number, zone_table[i].name);
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
           "(GC) %s reset zone %d (%s)", GET_NAME(ch), zone_table[i].number, zone_table[i].name);
  } else
    send_to_char(ch, "You do not have permission to reset this zone. Try %d.\r\n", GET_OLC_ZONE(ch));
}

/*  General fn for wizcommands of the sort: cmd <player> */
ACMD(do_wizutil)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int taeller;
  long result;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Yes, but for whom?!?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "There is no such player.\r\n");
  else if (IS_NPC(vict))
    send_to_char(ch, "You can't do that to a mob!\r\n");
  else if (GET_LEVEL(vict) >= GET_LEVEL(ch) && vict != ch)
    send_to_char(ch, "Hmmm...you'd better not.\r\n");
  else {
    switch (subcmd) {
    case SCMD_REROLL:
      send_to_char(ch, "Rerolled...\r\n");
      roll_real_abils(vict);
      log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
      send_to_char(ch, "New stats: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
	      GET_STR(vict), GET_INT(vict), GET_WIS(vict),
	      GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
      break;
    case SCMD_PARDON:
      send_to_char(ch, "Criminal flags are currently disabled.\r\n");
      return;
    case SCMD_MUTE:
      result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
      mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) Mute %s for %s by %s.",
		ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      send_to_char(ch, "(GC) Mute %s for %s by %s.\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_FREEZE:
      if (ch == vict) {
	send_to_char(ch, "Oh, yeah, THAT'S real smart...\r\n");
	return;
      }
      if (PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char(ch, "Your victim is already pretty cold.\r\n");
	return;
      }
      SET_BIT_AR(PLR_FLAGS(vict), PLR_FROZEN);
      GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
      send_to_char(vict, "A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n");
      send_to_char(ch, "Frozen.\r\n");
      act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
      mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_THAW:
      if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char(ch, "Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
	return;
      }
      if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
	send_to_char(ch, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
		GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
	return;
      }
      mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_FROZEN);
      send_to_char(vict, "A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
      send_to_char(ch, "Thawed.\r\n");
      act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
      break;
    case SCMD_UNAFFECT:
      if (vict->affected || ANY_AFF_FLAGS(vict)) {
	while (vict->affected)
	  affect_remove(vict, vict->affected);
    for(taeller=0; taeller < AF_ARRAY_MAX; taeller++)
      AFF_FLAGS(vict)[taeller] = 0;
    send_to_char(vict, "There is a brief flash of light!\r\nYou feel slightly different.\r\n");
	send_to_char(ch, "All spells removed.\r\n");
      } else {
	send_to_char(ch, "Your victim does not have any affections!\r\n");
	return;
      }
      break;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
      /*  SYSERR_DESC: This is the same as the unhandled case in do_gen_ps(),
       *  but this function handles 'reroll', 'pardon', 'freeze', etc. */
      break;
    }
    save_char(vict);
  }
}

/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 FIXME: overflow possible */
static size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone, int listall)
{
  size_t tmp;

  if (listall) {
    int i, j, k, l, m, n, o;
    char buf[MAX_STRING_LENGTH];

    sprintbitarray(zone_table[zone].zone_flags, zone_bits, ZN_ARRAY_MAX, buf);

    tmp = snprintf(bufptr, left,
	"%3d %-30.30s%s By: %-10.10s%s Age: %3d; Reset: %3d (%s); Range: %5d-%5d\r\n",
	zone_table[zone].number, zone_table[zone].name, KNRM, zone_table[zone].builders, KNRM,
	zone_table[zone].age, zone_table[zone].lifespan,
        zone_table[zone].reset_mode ? ((zone_table[zone].reset_mode == 1) ? "Reset when no players are in zone" : "Normal reset") : "Never reset",
	zone_table[zone].bot, zone_table[zone].top);
        j = k = l = m = n = o = 0;

        for (i = 0; i < top_of_world; i++)
          if (world[i].number >= zone_table[zone].bot && world[i].number <= zone_table[zone].top)
            j++;

        for (i = 0; i < top_of_objt; i++)
          if (obj_index[i].vnum >= zone_table[zone].bot && obj_index[i].vnum <= zone_table[zone].top)
            k++;

        for (i = 0; i < top_of_mobt; i++)
          if (mob_index[i].vnum >= zone_table[zone].bot && mob_index[i].vnum <= zone_table[zone].top)
            l++;

        for (i = 0; i<= top_shop; i++)
          if (SHOP_NUM(i) >= zone_table[zone].bot && SHOP_NUM(i) <= zone_table[zone].top)
            m++;

        for (i = 0; i < top_of_trigt; i++)
          if (trig_index[i]->vnum >= zone_table[zone].bot && trig_index[i]->vnum <= zone_table[zone].top)
            n++;

        o = count_quests(zone_table[zone].bot, zone_table[zone].top);

	tmp += snprintf(bufptr + tmp, left - tmp,
                        "       Zone stats:\r\n"
                        "       ---------------\r\n"
                        "         Flags:    %s\r\n"
                        "         Min Lev:  %2d\r\n"
                        "         Max Lev:  %2d\r\n"
                        "         Rooms:    %2d\r\n"
                        "         Objects:  %2d\r\n"
                        "         Mobiles:  %2d\r\n"
                        "         Shops:    %2d\r\n"
                        "         Triggers: %2d\r\n"
                        "         Quests:   %2d\r\n",
			buf, zone_table[zone].min_level, zone_table[zone].max_level,
                        j, k, l, m, n, o);

    return tmp;
  }

    return snprintf(bufptr, left,
        "%3d %-*s%s By: %-10.10s%s Range: %5d-%5d\r\n", zone_table[zone].number,
	count_color_chars(zone_table[zone].name)+30, zone_table[zone].name, KNRM,
	zone_table[zone].builders, KNRM, zone_table[zone].bot, zone_table[zone].top);
}

ACMD(do_show)
{
  int i, j, k, l, con, builder =0;		/* i, j, k to specifics? */
  size_t len, nlen;
  zone_rnum zrn;
  zone_vnum zvn;
  byte self = FALSE;
  struct char_data *vict = NULL;
  struct obj_data *obj;
  struct descriptor_data *d;
  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH],
	arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  int r, g, b;
  char colour[16];

  struct show_struct {
    const char *cmd;
    const char level;
  } fields[] = {
    { "nothing",	0  },				/* 0 */
    { "zones",		LVL_IMMORT },			/* 1 */
    { "player",		LVL_IMMORT },
    { "stats",		LVL_IMMORT },
    { "errors",		LVL_IMMORT },			/* 5 */
    { "death",		LVL_IMMORT },
    { "godrooms",	LVL_IMMORT },
    { "shops",		LVL_IMMORT },
    { "houses",		LVL_IMMORT },
    { "snoop",		LVL_IMMORT },			/* 10 */
    { "exp",        LVL_IMMORT },
    { "colour",     LVL_IMMORT },
    { "\n", 0 }
  };

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Show options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_LEVEL(ch))
	send_to_char(ch, "%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
    send_to_char(ch, "\r\n");
    return;
  }

  strcpy(arg, two_arguments(argument, field, value));	/* strcpy: OK (argument <= MAX_INPUT_LENGTH == arg) */

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return;
  }
  if (!strcmp(value, "."))
    self = TRUE;
  buf[0] = '\0';

  switch (l) {
  /* show zone */
  case 1:
    /* tightened up by JE 4/6/93 */
    if (self)
      print_zone_to_buf(buf, sizeof(buf), world[IN_ROOM(ch)].zone, 1);
    else if (*value && is_number(value)) {
      for (zvn = atoi(value), zrn = 0; zone_table[zrn].number != zvn && zrn <= top_of_zone_table; zrn++);
      if (zrn <= top_of_zone_table)
	print_zone_to_buf(buf, sizeof(buf), zrn, 1);
      else {
	send_to_char(ch, "That is not a valid zone.\r\n");
	return;
      }
    } else {
      char *buf2;
      if (*value)
        builder = 1;
      for (len = zrn = 0; zrn <= top_of_zone_table; zrn++) {
        if (*value) {
          buf2 = strtok(strdup(zone_table[zrn].builders), " ");
          while (buf2) {
            if (!str_cmp(buf2, value)) {
              if (builder == 1)
                builder++;
              break;
          }
            buf2 = strtok(NULL, " ");
          }
          if (!buf2)
	    continue;
	}
	nlen = print_zone_to_buf(buf + len, sizeof(buf) - len, zrn, 0);
        if (len + nlen >= sizeof(buf))
          break;
        len += nlen;
      }
    }
    if (builder == 1)
      send_to_char(ch, "%s has not built any zones here.\r\n", CAP(value));
    else if (builder == 2)
      send_to_char(ch, "The following zones have been built by: %s\r\n", CAP(value));
    page_string(ch->desc, buf, TRUE);
    break;

  /* show player */
  case 2: {
    char buf1[64], buf2[64];

    if (!*value) {
      send_to_char(ch, "A name would help.\r\n");
      return;
    }

    CREATE(vict, struct char_data, 1);
    clear_char(vict);
    CREATE(vict->player_specials, struct player_special_data, 1);
    new_mobile_data(vict);
    if (load_char(value, vict) < 0) {
      send_to_char(ch, "There is no such player.\r\n");
      free_char(vict);
      return;
    }

    strftime(buf1, sizeof(buf1), "%a %b %d %H:%M:%S %Y", localtime(&(vict->player.time.birth)));
    strftime(buf2, sizeof(buf2), "%a %b %d %H:%H:%S %Y", localtime(&(vict->player.time.logon)));

    send_to_char(ch, "Player: %-12s (%s) [%2d %s]\r\n", GET_NAME(vict),
      genders[(int) GET_SEX(vict)], GET_LEVEL(vict), CLASS_ABBR(vict));
    send_to_char(ch, "Coins: %-8d  Bal: %-8d Exp: %-8d\r\n",
      GET_COINS(vict), GET_BANK_COINS(vict), GET_EXP(vict));
    send_to_char(ch, "Started: %-25.25s  Last: %-25.25s\r\n", buf1, buf2);
    send_to_char(ch, "Played: %dh %dm\r\n",
      (int) (vict->player.time.played / 3600),
      (int) (vict->player.time.played / 60 % 60));

    free_char(vict);
    break;
  }
  /* show stats */
  case 3:
    i = 0;
    j = 0;
    k = 0;
    con = 0;
    for (vict = character_list; vict; vict = vict->next) {
      if (IS_NPC(vict))
	j++;
      else if (CAN_SEE(ch, vict)) {
	i++;
	if (vict->desc)
	  con++;
      }
    }
    for (obj = object_list; obj; obj = obj->next)
      k++;
    send_to_char(ch,
	"Current stats:\r\n"
	"  %5d players in game  %5d connected\r\n"
	"  %5d registered\r\n"
	"  %5d mobiles          %5d prototypes\r\n"
	"  %5d objects          %5d prototypes\r\n"
	"  %5d rooms            %5d zones\r\n"
  "  %5d triggers         %5d shops\r\n"
  "  %5d large bufs       %5d autoquests\r\n"
	"  %5d buf switches     %5d overflows\r\n"
	"  %5d lists\r\n",
	i, con,
	top_of_p_table + 1,
	j, top_of_mobt + 1,
	k, top_of_objt + 1,
	top_of_world + 1, top_of_zone_table + 1,
	top_of_trigt + 1, top_shop + 1,
	buf_largecount, total_quests,
	buf_switches, buf_overflows, global_lists->iSize
	);
    break;

  /* show errors */
  case 4:
    len = strlcpy(buf, "Errant Rooms\r\n------------\r\n", sizeof(buf));
    for (i = 0, k = 0; i <= top_of_world; i++)
      for (j = 0; j < DIR_COUNT; j++) {
      	if (!W_EXIT(i,j))
      	  continue;
        if (W_EXIT(i,j)->to_room == 0) {
	    nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (void   ) [%5d] %-*s%s (%s)\r\n", ++k, GET_ROOM_VNUM(i), count_color_chars(world[i].name)+40, world[i].name, QNRM, dirs[j]);
            if (len + nlen >= sizeof(buf))
              break;
            len += nlen;
        }
        if (W_EXIT(i,j)->to_room == NOWHERE && !W_EXIT(i,j)->general_description) {
	    nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (Nowhere) [%5d] %-*s%s (%s)\r\n", ++k, GET_ROOM_VNUM(i), count_color_chars(world[i].name)+ 40, world[i].name, QNRM, dirs[j]);
            if (len + nlen >= sizeof(buf))
              break;
            len += nlen;
        }
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show death */
  case 5:
    len = strlcpy(buf, "Death Traps\r\n-----------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s%s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name, QNRM);
        if (len + nlen >= sizeof(buf))
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show godrooms */
  case 6:
    len = strlcpy(buf, "Godrooms\r\n--------------------------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_GODROOM)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s%s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name, QNRM);
        if (len + nlen >= sizeof(buf))
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show shops */
  case 7:
    show_shops(ch, value);
    break;

  /* show houses */
  case 8:
    hcontrol_list_houses(ch, value);
    break;

  /* show snoop */
  case 9:
    i = 0;
    send_to_char(ch, "People currently snooping:\r\n--------------------------\r\n");
    for (d = descriptor_list; d; d = d->next) {
      if (d->snooping == NULL || d->character == NULL)
	continue;
      if (STATE(d) != CON_PLAYING || GET_LEVEL(ch) < GET_LEVEL(d->character))
	continue;
      if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
	continue;
      i++;
      send_to_char(ch, "%-10s%s - snooped by %s%s.\r\n", GET_NAME(d->snooping->character), QNRM, GET_NAME(d->character), QNRM);
    }
    if (i == 0)
      send_to_char(ch, "No one is currently snooping.\r\n");
    break;

  /* show experience tables */
  case 10:
    len = strlcpy(buf, "LvL - Mu     Cl     Th     Wa     BA     Ra     Br     Dr\r\n--------------------------\r\n", sizeof(buf));

    for (i = 1; i < LVL_IMMORT; i++) { 
      nlen = snprintf(buf + len, sizeof(buf) - len,  "%-3d - %-6d %-6d %-6d %-6d %-6d %-6d %-6d %-6d\r\n", i,  
				level_exp(CLASS_SORCEROR, i) - level_exp(CLASS_SORCEROR, i - 1),
				level_exp(CLASS_CLERIC, i) - level_exp(CLASS_CLERIC, i - 1),
				level_exp(CLASS_ROGUE, i) - level_exp(CLASS_ROGUE, i - 1),
				level_exp(CLASS_FIGHTER, i) - level_exp(CLASS_FIGHTER, i - 1),
				level_exp(CLASS_BARBARIAN, i) - level_exp(CLASS_BARBARIAN, i - 1),
				level_exp(CLASS_RANGER, i) - level_exp(CLASS_RANGER, i - 1),
				level_exp(CLASS_BARD, i) - level_exp(CLASS_BARD, i - 1),
				level_exp(CLASS_DRUID, i) - level_exp(CLASS_DRUID, i - 1));
      if (len + nlen >= sizeof(buf))
        break;
      len += nlen;
    }

    page_string(ch->desc, buf, TRUE);
    break;

  case 11:
    len = strlcpy(buf, "Colours\r\n--------------------------\r\n", sizeof(buf));
    k = 0;
    for (r = 0; r < 6; r++)
      for (g = 0; g < 6; g++)
        for (b = 0; b < 6; b++) {
          sprintf(colour, "F%d%d%d", r, g, b);
          nlen = snprintf(buf + len, sizeof(buf) - len,  "%s%s%s", ColourRGB(ch->desc, colour), colour, ++k % 6 == 0 ? "\tn\r\n" : "    ");
          if (len + nlen >= sizeof(buf))
            break;
          len += nlen;
        }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show what? */
  default:
    send_to_char(ch, "Sorry, I don't understand that.\r\n");
    break;
  }
}

/* The do_set function */

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC	0
#define BINARY	1
#define NUMBER	2

#define SET_OR_REMOVE(flagset, flags) { \
        if (on) SET_BIT_AR(flagset, flags); \
        else if (off) REMOVE_BIT_AR(flagset, flags); }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

/* The set options available */
static struct set_struct {
    const char *cmd;
    const char level;
    const char pcnpc;
    const char type;
  } set_fields[] = {
   { "ac",		LVL_BUILDER, 	BOTH, 	NUMBER },  /* 0  */
   { "afk",             LVL_BUILDER,	PC,	BINARY },  /* 1  */
   { "age",		LVL_GOD,	BOTH,	NUMBER },
   { "bank",		LVL_BUILDER, 	PC, 	NUMBER },
   { "brief",		LVL_GOD, 	PC, 	BINARY },  /* 4  */
   { "cha",		LVL_BUILDER, 	BOTH, 	NUMBER },
   { "class",		LVL_BUILDER, 	BOTH, 	MISC },
   { "color",		LVL_GOD, 	PC, 	BINARY },
   { "con", 		LVL_BUILDER, 	BOTH, 	NUMBER },
   { "deleted",		LVL_IMPL, 	PC, 	BINARY },
   { "dex", 		LVL_BUILDER, 	BOTH, 	NUMBER },
   { "drunk",		LVL_BUILDER, 	BOTH, 	MISC },
   { "exp", 		LVL_GOD, 	BOTH, 	NUMBER },
   { "frozen",		LVL_GRGOD, 	PC,	BINARY },  /* 13 */
   { "coins",		LVL_BUILDER, 	BOTH, 	NUMBER },
   { "height",		LVL_BUILDER,	BOTH,	NUMBER },
   { "hitpoints",       LVL_BUILDER, 	BOTH, 	NUMBER },
   { "hunger",		LVL_BUILDER, 	BOTH, 	MISC },    /* 17 */
   { "int", 		LVL_BUILDER, 	BOTH, 	NUMBER },
   { "invis",		LVL_GOD, 	PC, 	NUMBER },
   { "invstart",        LVL_BUILDER,	PC, 	BINARY },
   { "level",		LVL_GRGOD, 	BOTH, 	NUMBER },  /* 21 */
   { "loadroom",	LVL_BUILDER, 	PC, 	MISC },
   { "mana",		LVL_BUILDER, 	BOTH, 	NUMBER },
   { "maxhit",	        LVL_BUILDER, 	BOTH, 	NUMBER },
   { "maxmana",       	LVL_BUILDER, 	BOTH, 	NUMBER },
   { "maxstam",		LVL_BUILDER, 	BOTH, 	NUMBER },  /* 26 */
   { "name",	LVL_IMMORT, 	PC, 	MISC },
   { "nodelete",	LVL_GOD, 	PC, 	BINARY },
   { "nohassle",	LVL_GOD, 	PC, 	BINARY },
   { "nosummon",	LVL_BUILDER,	PC,	BINARY },
   { "nowizlist", 	LVL_GRGOD, 	PC, 	BINARY },
   { "olc",		LVL_GRGOD,	PC,	MISC },
   { "password",	LVL_GRGOD,	PC,	MISC },
   { "poofin",		LVL_IMMORT,	PC,	MISC },
   { "poofout",         LVL_IMMORT,	PC,	MISC },
   { "quest",		LVL_GOD, 	PC, 	BINARY },
   { "room",		LVL_BUILDER, 	BOTH, 	NUMBER },
   { "screenwidth", LVL_GOD,  PC,   NUMBER }, /* 38 */
   { "sex", 		LVL_GOD, 	BOTH, 	MISC },
   { "showvnums",  LVL_BUILDER,  PC, BINARY },
   { "siteok",   LVL_GOD,  PC,   BINARY },
   { "skill",   LVL_GOD,  BOTH,   NUMBER },
   { "stam",		LVL_BUILDER, 	BOTH, 	NUMBER },  /* 43 */
   { "str",		LVL_BUILDER, 	BOTH, 	NUMBER },
   { "thirst",		LVL_BUILDER, 	BOTH, 	MISC },
   { "variable",        LVL_GRGOD,	PC,	MISC },
   { "weight",		LVL_BUILDER,	BOTH,	NUMBER },
   { "wis", 		LVL_BUILDER, 	BOTH, 	NUMBER }, 
   { "questpoints",     LVL_GOD,        PC,     NUMBER },  /* 49 */
   { "questhistory",    LVL_GOD,        PC,   NUMBER },
   { "species",         LVL_BUILDER,    BOTH, MISC },
   { "\n", 0, BOTH, MISC }
  };

static int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg)
{
  int i, on = 0, off = 0, value = 0, qvnum;
  room_rnum rnum;
  room_vnum rvnum;

  /* Check to make sure all the levels are correct */
  if (GET_LEVEL(ch) != LVL_IMPL) {
    if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch) {
      send_to_char(ch, "Maybe that's not such a great idea...\r\n");
      return (0);
    }
  }
  if (GET_LEVEL(ch) < set_fields[mode].level) {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return (0);
  }

  /* Make sure the PC/NPC is correct */
  if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC)) {
    send_to_char(ch, "You can't do that to a beast!\r\n");
    return (0);
  } else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC)) {
    send_to_char(ch, "That can only be done to a beast!\r\n");
    return (0);
  }

  /* Find the value of the argument */
  if (set_fields[mode].type == BINARY) {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
      on = 1;
    else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
      off = 1;
    if (!(on || off)) {
      send_to_char(ch, "Value must be 'on' or 'off'.\r\n");
      return (0);
    }
  } else if (set_fields[mode].type == NUMBER) {
    value = atoi(val_arg);
  }
  switch (mode) {
    case 0: /* ac */
      vict->points.armor = RANGE(-100, 100);
      affect_total(vict);
      break;
    case 1: /* afk */
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_AFK);
      break;
    case 2: /* age */
      if (value < 2 || value > 200) {	/* Arbitrary limits. */
        send_to_char(ch, "Ages 2 to 200 accepted.\r\n");
        return (0);
      }
      GET_ROLEPLAY_AGE(vict) = LIMIT(value, MIN_CHAR_AGE, MAX_CHAR_AGE);
      GET_ROLEPLAY_AGE_YEAR(vict) = time_info.year;
      break;
    case 3: /* bank */
      GET_BANK_COINS(vict) = RANGE(0, 100000000);
      break;
    case 4: /* brief */
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
      break;
    case 5:  /* cha */
      if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
        RANGE(3, 25);
      else
        RANGE(3, 18);
      vict->real_abils.cha = value;
      affect_total(vict);
      break;
    case 6: /* class */
      if ((i = parse_class(*val_arg)) == CLASS_UNDEFINED) {
        send_to_char(ch, "That is not a class.\r\n");
        return (0);
      }
      GET_CLASS(vict) = i;
      break;
    case 7:  /* color */
      SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1));
      SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_2));
      break;
    case 8: /* con */
      if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
        RANGE(3, 25);
      else
        RANGE(3, 18);
      vict->real_abils.con = value;
      affect_total(vict);
      break;
    case 9: /* delete */
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
      break;
    case 10: /* dex */
      if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
        RANGE(3, 25);
      else
        RANGE(3, 18);
      vict->real_abils.dex = value;
      affect_total(vict);
      break;
    case 11: /* drunk */
      if (!str_cmp(val_arg, "off")) {
        GET_COND(vict, DRUNK) = -1;
        send_to_char(ch, "%s's drunkenness is now off.\r\n", GET_NAME(vict));
      } else if (is_number(val_arg)) {
        value = atoi(val_arg);
        RANGE(0, 24);
        GET_COND(vict, DRUNK) = value;
        send_to_char(ch, "%s's drunkenness set to %d.\r\n", GET_NAME(vict), value);
      } else {
        send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
        return (0);
      }
      break;
    case 12: /* exp */
      vict->points.exp = RANGE(0, 50000000);
      break;
    case 13: /* frozen */
      if (ch == vict && on) {
        send_to_char(ch, "Better not -- could be a long winter!\r\n");
        return (0);
      }
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
      break;
    case 14: { /* coins */
      struct obj_data *coin_obj;
      int i;

      value = MIN(MAX(value, 0), 100000000);
      coin_obj = find_inventory_coin(vict);

      if (value == 0) {
        remove_other_coins_from_list(vict->carrying, NULL);
        for (i = 0; i < NUM_WEARS; i++)
          remove_other_coins_from_list(GET_EQ(vict, i), NULL);
        GET_COINS(vict) = 0;
        send_to_char(ch, "Ok.\r\n");
        return (1);
      }

      if (coin_obj) {
        GET_OBJ_VAL(coin_obj, 0) = value;
        update_money_obj(coin_obj);
      } else {
        coin_obj = create_money(value);
        if (!coin_obj) {
          send_to_char(ch, "Ok.\r\n");
          return (1);
        }
        obj_to_char(coin_obj, vict);
      }

      remove_other_coins_from_list(vict->carrying, coin_obj);
      for (i = 0; i < NUM_WEARS; i++)
        remove_other_coins_from_list(GET_EQ(vict, i), coin_obj);

      GET_COINS(vict) = value;
      send_to_char(ch, "Ok.\r\n");
      return (1);
    }
    case 15: /* height */
      GET_HEIGHT(vict) = value;
      affect_total(vict);
      break;
    case 16: /* hit */
      vict->points.hit = RANGE(-9, vict->points.max_hit);
      affect_total(vict);
      break;
    case 17: /* hunger */
      if (!str_cmp(val_arg, "off")) {
        GET_COND(vict, HUNGER) = -1;
        send_to_char(ch, "%s's hunger is now off.\r\n", GET_NAME(vict));
      } else if (is_number(val_arg)) {
        value = atoi(val_arg);
        RANGE(0, 24);
        GET_COND(vict, HUNGER) = value;
        send_to_char(ch, "%s's hunger set to %d.\r\n", GET_NAME(vict), value);
      } else {
        send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
        return (0);
       }
       break;
   case 18: /* int */
      if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
        RANGE(3, 25);
      else
        RANGE(3, 18);
      vict->real_abils.intel = value;
      affect_total(vict);
      break;
    case 19: /* invis */
      if (GET_LEVEL(ch) < LVL_IMPL && ch != vict) {
        send_to_char(ch, "You aren't godly enough for that!\r\n");
        return (0);
      }
      GET_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
      break;
    case 20: /* invistart */
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
      break;
    case 21: /* level */
      if ((!IS_NPC(vict) && value > GET_LEVEL(ch)) || value > LVL_IMPL) {
        send_to_char(ch, "You can't do that.\r\n");
        return (0);
      }
      RANGE(1, LVL_IMPL);
      vict->player.level = value;
      break;
    case 22: /* loadroom */
      if (!str_cmp(val_arg, "off")) {
        REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_LOADROOM);
      } else if (is_number(val_arg)) {
        rvnum = atoi(val_arg);
        if (real_room(rvnum) != NOWHERE) {
          SET_BIT_AR(PLR_FLAGS(vict), PLR_LOADROOM);
          GET_LOADROOM(vict) = rvnum;
          send_to_char(ch, "%s will enter at room #%d.\r\n", GET_NAME(vict), GET_LOADROOM(vict));
        } else {
        send_to_char(ch, "That room does not exist!\r\n");
	  return (0);
        }
      } else {
        send_to_char(ch, "Must be 'off' or a room's virtual number.\r\n");
        return (0);
      }
      break;
    case 23: /* mana */
      vict->points.mana = RANGE(0, vict->points.max_mana);
      affect_total(vict);
      break;
    case 24: /* maxhit */
      vict->points.max_hit = RANGE(1, 5000);
      affect_total(vict);
      break;
    case 25: /* maxmana */
      vict->points.max_mana = RANGE(1, 5000);
      affect_total(vict);
      break;
    case 26: /* maxstam */
      vict->points.max_stamina = RANGE(1, 5000);
      affect_total(vict);
      break;
    case 27: /* name */
      if (ch != vict && GET_LEVEL(ch) < LVL_IMPL) {
        send_to_char(ch, "Only Imps can change the name of other players.\r\n");
        return (0);
      }
      if (!change_player_name(ch, vict, val_arg)) {
        send_to_char(ch, "Name has not been changed!\r\n");
        return (0);
      }
      break;
    case 28: /* nodelete */
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
      break;
    case 29: /* nohassle */
      if (GET_LEVEL(ch) < LVL_GOD && ch != vict) {
        send_to_char(ch, "You aren't godly enough for that!\r\n");
        return (0);
      }
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
      break;
    case 30: /* nosummon */
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
      send_to_char(ch, "Nosummon %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
      break;
    case 31: /* nowiz */
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
      break;
    case 32: /* olc */
      if (is_abbrev(val_arg, "socials") || is_abbrev(val_arg, "actions") || is_abbrev(val_arg, "aedit"))
        GET_OLC_ZONE(vict) = AEDIT_PERMISSION;
      else if (is_abbrev(val_arg, "hedit") || is_abbrev(val_arg, "help"))
        GET_OLC_ZONE(vict) = HEDIT_PERMISSION;
      else if (*val_arg == '*' || is_abbrev(val_arg, "all"))
        GET_OLC_ZONE(vict) = ALL_PERMISSION;
      else if (is_abbrev(val_arg, "off"))
        GET_OLC_ZONE(vict) = NOWHERE;
      else if (!is_number(val_arg))  {
        send_to_char(ch, "Value must be a zone number, 'aedit', 'hedit', 'off' or 'all'.\r\n");
        return (0);
      } else
        GET_OLC_ZONE(vict) = atoi(val_arg);
      break;
    case 33: /* password */
      if (GET_LEVEL(vict) >= LVL_GRGOD) {
        send_to_char(ch, "You cannot change that.\r\n");
        return (0);
      }
      strncpy(GET_PASSWD(vict), CRYPT(val_arg, GET_NAME(vict)), MAX_PWD_LENGTH);	/* strncpy: OK (G_P:MAX_PWD_LENGTH) */
      *(GET_PASSWD(vict) + MAX_PWD_LENGTH) = '\0';
      send_to_char(ch, "Password changed to '%s'.\r\n", val_arg);
      break;
    case 34: /* poofin */
      if ((vict == ch) || (GET_LEVEL(ch) == LVL_IMPL)) {
        skip_spaces(&val_arg);
        parse_at(val_arg);

        if (POOFIN(vict))
          free(POOFIN(vict));

      if (!*val_arg)
          POOFIN(vict) = NULL;
        else
          POOFIN(vict) = strdup(val_arg);
        }
      break;
    case 35: /* poofout */
      if ((vict == ch) || (GET_LEVEL(ch) == LVL_IMPL)) {
        skip_spaces(&val_arg);
        parse_at(val_arg);

        if (POOFOUT(vict))
          free(POOFOUT(vict));

	if (!*val_arg)
          POOFOUT(vict) = NULL;
        else
          POOFOUT(vict) = strdup(val_arg);
        }
      break;
    case 36: /* quest */
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
      break;
    case 37: /* room */
      if ((rnum = real_room(value)) == NOWHERE) {
        send_to_char(ch, "No room exists with that number.\r\n");
        return (0);
      }
      if (IN_ROOM(vict) != NOWHERE)
        char_from_room(vict);
      char_to_room(vict, rnum);
      break;
    case 38: /* screenwidth */
      GET_SCREEN_WIDTH(vict) = RANGE(40, 200);
      break;
    case 39: /* sex */
      if ((i = search_block(val_arg, genders, FALSE)) < 0) {
        send_to_char(ch, "Must be 'male', 'female', or 'neutral'.\r\n");
        return (0);
      }
      GET_SEX(vict) = i;
      break;
    case 40: /* showvnums */
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SHOWVNUMS);
      break;
    case 41: /* siteok */
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
      break;
    case 42: /* skills/spells */
    {
      char local_buf[MAX_INPUT_LENGTH], *value_arg, *name_end;
      char skill_name[MAX_INPUT_LENGTH];
      int snum;

      /* Make a writable copy and trim trailing spaces */
      strlcpy(local_buf, val_arg, sizeof(local_buf));
      {
        int len = strlen(local_buf);
        while (len > 0 && isspace((unsigned char)local_buf[len - 1])) {
          local_buf[--len] = '\0';
        }
      }

      if (!*local_buf) {
        send_to_char(ch, "Usage: set <player> skill <skill-name> <0-100>\r\n");
        return (0);
      }

      /* Find last word (the numeric value) by walking backwards */
      name_end = local_buf + strlen(local_buf) - 1;
      while (name_end > local_buf && !isspace((unsigned char)*name_end))
        name_end--;

      if (name_end <= local_buf) {
        send_to_char(ch, "Usage: set <player> skill <skill-name> <0-100>\r\n");
        return (0);
      }

      *name_end = '\0';             /* terminate skill name string */
      value_arg = name_end + 1;     /* point to numeric string */

      /* Trim trailing whitespace from skill name */
      while (name_end > local_buf && isspace((unsigned char)name_end[-1])) {
        *--name_end = '\0';
      }

      char *skill_ptr = local_buf;
      skip_spaces(&skill_ptr);
      strlcpy(skill_name, skill_ptr, sizeof(skill_name));

      if (!*skill_name || !*value_arg) {
        send_to_char(ch, "Usage: set <player> skill <skill-name> <0-100>\r\n");
        return (0);
      }

      if (IS_NPC(vict)) {
        send_to_char(ch, "You can only set skills on player characters.\r\n");
        return (0);
      }

      if (!is_number(value_arg)) {
        send_to_char(ch, "The skill value must be a number from 0 to 100.\r\n");
        return (0);
      }

      value = atoi(value_arg);
      if (value < 0)   value = 0;
      if (value > 100) value = 100;

      snum = find_skill_num(skill_name); /* handles case-insensitive, abbrev match */
      if (snum <= 0) {
        send_to_char(ch, "That skill or spell doesn't exist.\r\n");
        return (0);
      }

      SET_SKILL(vict, snum, value);

      send_to_char(ch, "Set %s's %s to %d%%.\r\n",
                  GET_NAME(vict), spell_info[snum].name, value);

      if (vict != ch)
        send_to_char(vict, "%s has set your %s to %d%%.\r\n",
                    GET_NAME(ch), spell_info[snum].name, value);

      save_char(vict);

      mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
            "%s set %s's %s skill to %d.",
            GET_NAME(ch), GET_NAME(vict), spell_info[snum].name, value);

    }
    break;

    case 43: /* stam */
      vict->points.stamina = RANGE(0, vict->points.max_stamina);
      affect_total(vict);
      break;

    case 44: /* str */
      if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
        RANGE(3, 25);
      else
        RANGE(3, 18);
      vict->real_abils.str = value;
      affect_total(vict);
      break;
    case 45: /* thirst */
      if (!str_cmp(val_arg, "off")) {
        GET_COND(vict, THIRST) = -1;
        send_to_char(ch, "%s's thirst is now off.\r\n", GET_NAME(vict));
      } else if (is_number(val_arg)) {
        value = atoi(val_arg);
        RANGE(0, 24);
        GET_COND(vict, THIRST) = value;
        send_to_char(ch, "%s's thirst set to %d.\r\n", GET_NAME(vict), value);
      } else {
        send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
        return (0);
      }
      break;
    case 46: /* variable */
      return perform_set_dg_var(ch, vict, val_arg);
    case 47: /* weight */
      GET_WEIGHT(vict) = value;
      affect_total(vict);
      break;
    case 48: /* wis */
      if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
        RANGE(3, 25);
      else
        RANGE(3, 18);
      vict->real_abils.wis = value;
      affect_total(vict);
      break;
    case 49: /* questpoints */
      GET_QUESTPOINTS(vict) = RANGE(0, 100000000);
      break;
    case 50: /* questhistory */
      qvnum = atoi(val_arg);
      if (real_quest(qvnum) == NOTHING) {
        send_to_char(ch, "That quest doesn't exist.\r\n");
        return FALSE;
      } else {
        if (is_complete(vict, qvnum)) {
          remove_completed_quest(vict, qvnum);
          send_to_char(ch, "Quest %d removed from history for player %s.\r\n",
     qvnum, GET_NAME(vict));
        } else {
          add_completed_quest(vict, qvnum);
          send_to_char(ch, "Quest %d added to history for player %s.\r\n",
     qvnum, GET_NAME(vict));
        }
        break;
      }
    case 51: /* species */
      if ((i = parse_species(val_arg)) == SPECIES_UNDEFINED) {
        send_to_char(ch, "That is not a species.\r\n");
        return (0);
      }
      update_species(vict, i);
      affect_total(vict);
      send_to_char(ch, "You set %s's species to %s.\r\n",
                   get_char_sdesc(vict), get_species_name(GET_SPECIES(vict)));
      break;
    default:
      send_to_char(ch, "Can't set that!\r\n");
      return (0);
    }
  /* Show the new value of the variable */
  if (set_fields[mode].type == BINARY) {
    send_to_char(ch, "%s %s for %s.\r\n", set_fields[mode].cmd, ONOFF(on), GET_NAME(vict));
  } else if (set_fields[mode].type == NUMBER) {
    send_to_char(ch, "%s's %s set to %d.\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
  } else
    send_to_char(ch, "%s", CONFIG_OK);

  return (1);
}

static void show_set_help(struct char_data *ch)
{
  const char *set_levels[] = {"Imm", "God", "GrGod", "IMP"};
  const char *set_targets[] = {"PC", "NPC", "BOTH"};
  const char *set_types[] = {"MISC", "BINARY", "NUMBER"};
  char buf[MAX_STRING_LENGTH];
  int i, len=0, add_len=0;

  len = snprintf(buf, sizeof(buf), "%sCommand             Lvl    Who?  Type%s\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
  for (i = 0; *(set_fields[i].cmd) != '\n'; i++) {
	if (set_fields[i].level <= GET_LEVEL(ch)) {
      add_len = snprintf(buf+len, sizeof(buf)-len, "%-20s%-5s  %-4s  %-6s\r\n", set_fields[i].cmd,
                                        set_levels[((int)(set_fields[i].level) - LVL_IMMORT)],
                                        set_targets[(int)(set_fields[i].pcnpc)-1],
                                        set_types[(int)(set_fields[i].type)]);
      len += add_len;
    }
  }
  page_string(ch->desc, buf, TRUE);
}

static struct obj_data *find_inventory_coin(struct char_data *ch)
{
  struct obj_data *obj;

  if (!ch)
    return NULL;

  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
      return obj;

  return NULL;
}

static void remove_other_coins_from_list(struct obj_data *list, struct obj_data *keep)
{
  struct obj_data *obj, *next_obj;

  for (obj = list; obj; obj = next_obj) {
    next_obj = obj->next_content;

    if (obj->contains)
      remove_other_coins_from_list(obj->contains, keep);

    if (obj != keep && GET_OBJ_TYPE(obj) == ITEM_MONEY)
      extract_obj(obj);
  }
}

ACMD(do_set)
{
  struct char_data *vict = NULL, *cbuf = NULL;
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  int mode, len, player_i = 0, retval;
  char is_file = 0, is_player = 0;

  half_chop(argument, name, buf);

  if (!strcmp(name, "file")) {
    is_file = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "help")) {
    show_set_help(ch);
    return;
  } else if (!str_cmp(name, "player")) {
    is_player = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "mob"))
    half_chop(buf, name, buf);

  if (!is_file && !is_player && !str_cmp(name, "species")) {
    char val[MAX_INPUT_LENGTH];
    char target[MAX_INPUT_LENGTH];

    half_chop(buf, val, target);
    if (!*val || !*target) {
      send_to_char(ch, "Usage: set species <type> <target>\r\n");
      return;
    }
    strlcpy(field, "species", sizeof(field));
    strlcpy(name, target, sizeof(name));
    strlcpy(buf, val, sizeof(buf));
  } else {
    half_chop(buf, field, buf);
  }

  if (!*name || !*field) {
    send_to_char(ch, "Usage: set <victim> <field> <value>\r\n");
    send_to_char(ch, "       %sset help%s will display valid fields\r\n", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
    return;
  }

  /* find the target */
  if (!is_file) {
    if (is_player) {
      if (!(vict = get_player_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
	send_to_char(ch, "There is no such player.\r\n");
	return;
      }
    } else { /* is_mob */
      if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
	send_to_char(ch, "There is no such creature.\r\n");
	return;
      }
    }
  } else if (is_file) {
    /* try to load the player off disk */
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    CREATE(cbuf->player_specials, struct player_special_data, 1);
    new_mobile_data(cbuf);
    if ((player_i = load_char(name, cbuf)) > -1) {
      if (GET_LEVEL(cbuf) > GET_LEVEL(ch)) {
	free_char(cbuf);
	send_to_char(ch, "Sorry, you can't do that.\r\n");
	return;
      }
      vict = cbuf;
    } else {
      free_char(cbuf);
      send_to_char(ch, "There is no such player.\r\n");
      return;
    }
  }

  /* find the command in the list */
  len = strlen(field);
  for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
    if (!strncmp(field, set_fields[mode].cmd, len))
      break;

  if (*(set_fields[mode].cmd) == '\n') {
    retval = 0; /* skips saving below */
    send_to_char(ch, "Can't set that!\r\n");
  } else
  /* perform the set */
  retval = perform_set(ch, vict, mode, buf);

  /* save the character if a change was made */
  if (retval) {
    if (!is_file && !IS_NPC(vict))
      save_char(vict);
    if (is_file) {
      GET_PFILEPOS(cbuf) = player_i;
      save_char(cbuf);
      send_to_char(ch, "Saved in file.\r\n");
    }
  }

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
}

ACMD(do_saveall)
{
 if (GET_LEVEL(ch) < LVL_BUILDER)
    send_to_char (ch, "You are not holy enough to use this privelege.\n\r");
 else {
    save_all();
    House_save_all();
    send_to_char(ch, "World and house files saved.\n\r");
 }
}

ACMD(do_links)
{
  zone_rnum zrnum;
  zone_vnum zvnum;
  room_rnum nr, to_room;
  int first, last, j;
  char arg[MAX_INPUT_LENGTH];


  skip_spaces(&argument);
  one_argument(argument, arg);

  if (!is_number(arg)) {
    zrnum = world[IN_ROOM(ch)].zone;
    zvnum = zone_table[zrnum].number;
  } else {
    zvnum = atoi(arg);
    zrnum = real_zone(zvnum);
  }

  if (zrnum == NOWHERE || zvnum == NOWHERE) {
    send_to_char(ch, "No zone was found with that number.\n\r");
    return;
  }

  last  = zone_table[zrnum].top;
  first = zone_table[zrnum].bot;

  send_to_char(ch, "Zone %d is linked to the following zones:\r\n", zvnum);
  for (nr = 0; nr <= top_of_world && (GET_ROOM_VNUM(nr) <= last); nr++) {
    if (GET_ROOM_VNUM(nr) >= first) {
      for (j = 0; j < DIR_COUNT; j++) {
        if (world[nr].dir_option[j]) {
          to_room = world[nr].dir_option[j]->to_room;
          if (to_room != NOWHERE && (zrnum != world[to_room].zone))
          send_to_char(ch, "%3d %-30s at %5d (%-5s) ---> %5d\r\n",
                       zone_table[world[to_room].zone].number,
                       zone_table[world[to_room].zone].name,
                       GET_ROOM_VNUM(nr), dirs[j], world[to_room].number);
        }
      }
    }
  }
}

/* Zone Checker Code below */
/*mob limits*/
#define MAX_EXP_ALLOWED          GET_LEVEL(mob)*GET_LEVEL(mob) * 120
#define MAX_LEVEL_ALLOWED        LVL_IMPL
#define GET_OBJ_AVG_DAM(obj)     (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1))
/* arbitrary limit for per round dam */
#define MAX_MOB_DAM_ALLOWED      500

#define ZCMD2 zone_table[zone].cmd[cmd_no]  /*fom DB.C*/

/*item limits*/
#define MAX_DAM_ALLOWED            50    /* for weapons  - avg. dam*/
#define MAX_AFFECTS_ALLOWED        3
#define MAX_OBJ_COINS_ALLOWED       1000000

/* Armor class limits*/
#define TOTAL_WEAR_CHECKS  (NUM_ITEM_WEARS-2)  /*minus Wield and Take*/
static struct zcheck_armor {
  bitvector_t bitvector;          /* from Structs.h                       */
  int ac_allowed;                 /* Max. AC allowed for this body part  */
  char *message;                  /* phrase for error message            */
} zarmor[] = {
  {ITEM_WEAR_FINGER, 10, "Ring"},
  {ITEM_WEAR_NECK,   10, "Necklace"},
  {ITEM_WEAR_BACK,   10, "Backpack"},
  {ITEM_WEAR_BODY,   10, "Body armor"},
  {ITEM_WEAR_HEAD,   10, "Head gear"},
  {ITEM_WEAR_LEGS,   10, "Legwear"},
  {ITEM_WEAR_FEET,   10, "Footwear"},
  {ITEM_WEAR_HANDS,  10, "Glove"},
  {ITEM_WEAR_ARMS,   10, "Armwear"},
  {ITEM_WEAR_SHIELD, 10, "Shield"},
  {ITEM_WEAR_ABOUT,  10, "Cloak"},
  {ITEM_WEAR_WAIST,  10, "Belt"},
  {ITEM_WEAR_WRIST,  10, "Wristwear"},
  {ITEM_WEAR_HOLD,   10, "Held item"}
};

/* Applies limits !! Very Important:  Keep these in the same order as in Structs.h.
 * To ignore an apply, set max_aff to -99. These will be ignored if MAX_APPLIES_LIMIT = 0 */
static struct zcheck_affs {
  int aff_type;    /*from Structs.h*/
  int min_aff;     /*min. allowed value*/
  int max_aff;     /*max. allowed value*/
  char *message;   /*phrase for error message*/
} zaffs[] = {
  {APPLY_NONE,         0, -99, "unused0"},
  {APPLY_STR,         -5,   3, "strength"},
  {APPLY_DEX,         -5,   3, "dexterity"},
  {APPLY_INT,         -5,   3, "intelligence"},
  {APPLY_WIS,         -5,   3, "wisdom"},
  {APPLY_CON,         -5,   3, "constitution"},
  {APPLY_CHA,         -5,   3, "charisma"},
  {APPLY_CLASS,        0,   0, "class"},
  {APPLY_LEVEL,        0,   0, "level"},
  {APPLY_AGE,        -10,  10, "age"},
  {APPLY_CHAR_WEIGHT,-50,  50, "character weight"},
  {APPLY_CHAR_HEIGHT,-50,  50, "character height"},
  {APPLY_MANA,       -50,  50, "mana"},
  {APPLY_HIT,        -50,  50, "hit points"},
  {APPLY_STAMINA,       -50,  50, "stamina"},
  {APPLY_COINS,         0,   0, "coins"},
  {APPLY_EXP,          0,   0, "experience"},
  {APPLY_AC,         -10,  10, "magical AC"},
  {APPLY_SAVE_STR, -2, 2, "saving throw (Strength)"},
  {APPLY_SAVE_DEX, -2, 2, "saving throw (Dexterity)"},
  {APPLY_SAVE_CON, -2, 2, "saving throw (Constitution)"},
  {APPLY_SAVE_INT, -2, 2, "saving throw (Intelligence)"},
  {APPLY_SAVE_WIS, -2, 2, "saving throw (Wisdom)"},
  {APPLY_SAVE_CHA, -2, 2, "saving throw (Charisma)"}
};

/*room limits*/
/* Off limit zones are any zones a player should NOT be able to walk to (ex. Limbo) */
static const int offlimit_zones[] = {0,12,13,14,-1};  /*what zones can no room connect to (virtual num) */
#define MIN_ROOM_DESC_LENGTH   80       /* at least one line - set to 0 to not care. */
#define MAX_COLOUMN_WIDTH      80       /* at most 80 chars per line */

ACMD (do_zcheck)
{
  zone_rnum zrnum;
  struct obj_data *obj;
  struct char_data *mob = NULL;
  room_vnum exroom=0;
  int ac=0;
  int affs=0, value;
  int i = 0, j = 0, k = 0, l = 0, m = 0, found = 0; /* found is used as a 'send now' flag*/
  char buf[MAX_STRING_LENGTH];
  float avg_dam;
  size_t len = 0;
  struct extra_descr_data *ext, *ext2;
  one_argument(argument, buf);

  if (!is_number(buf) || !strcmp(buf, "."))
    zrnum = world[IN_ROOM(ch)].zone;
  else
    zrnum = real_zone(atoi(buf));

  if (zrnum == NOWHERE) {
    send_to_char(ch, "Check what zone ?\r\n");
    return;
  } else
    send_to_char(ch, "Checking zone %d!\r\n", zone_table[zrnum].number);

 /* Check mobs */

  send_to_char(ch, "Checking Mobs for limits...\r\n");
  /*check mobs first*/
  for (i=0; i<top_of_mobt;i++) {
      if (real_zone_by_thing(mob_index[i].vnum) == zrnum) {  /*is mob in this zone?*/
        mob = &mob_proto[i];
        if (!strcmp(mob->player.name, "mob unfinished") && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Alias hasn't been set.\r\n");

        if (!strcmp(mob->player.short_descr, "the unfinished mob") && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Short description hasn't been set.\r\n");

        if (!strncmp(mob->player.long_descr, "An unfinished mob stands here.", 30) && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Long description hasn't been set.\r\n");

        if (mob->player.description && *mob->player.description) {
          if (!strncmp(mob->player.description, "It looks unfinished.", 20) && (found=1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Description hasn't been set.\r\n");
          else if (strncmp(mob->player.description, "   ", 3) && (found=1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Description hasn't been formatted. (/fi)\r\n");
        }

        if (GET_LEVEL(mob)>MAX_LEVEL_ALLOWED && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Is level %d (limit: 1-%d)\r\n",
                          GET_LEVEL(mob), MAX_LEVEL_ALLOWED);

        /* --- 5e-style average unarmed damage check --- */
        {
          int prof = 0;
          int str_mod = GET_ABILITY_MOD(GET_STR(mob));
          int die_size;

          /* derive proficiency tier; mobs use their stored unarmed skill */
          prof = GET_PROFICIENCY(GET_SKILL(mob, SKILL_UNARMED));

          switch (prof) {
            case 0:  die_size = 4; break; /* initial unarmed skill die size */
            case 1:  die_size = 4; break;
            case 2:  die_size = 6; break; /* ~40 skill level */
            case 3:  die_size = 6; break;
            case 4:  die_size = 8; break; /* ~80 skill level */
            case 5:  die_size = 10; break; /* max skill level */
            default: die_size = 12; break;
          }

          /* expected average damage = average roll + STR + proficiency */
          avg_dam = ((die_size + 1) / 2.0) + str_mod + prof;

          if (avg_dam > MAX_MOB_DAM_ALLOWED && (found=1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- average unarmed damage of %4.1f is too high (limit: %d)\r\n",
                            avg_dam, MAX_MOB_DAM_ALLOWED);

          if (prof == 0 && str_mod <= 0 && (found=1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- No unarmed combat proficiency set (add skill or weapon)\r\n");
        }

        if (GET_EXP(mob)>MAX_EXP_ALLOWED && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Has %d experience (limit: %d)\r\n",
                              GET_EXP(mob), MAX_EXP_ALLOWED);
        if ((AFF_FLAGGED(mob, AFF_CHARM) || AFF_FLAGGED(mob, AFF_POISON)) && (found = 1))
	  len += snprintf(buf + len, sizeof(buf) - len,
                          "- Has illegal affection bits set (%s %s)\r\n",
                              AFF_FLAGGED(mob, AFF_CHARM) ? "CHARM" : "",
                              AFF_FLAGGED(mob, AFF_POISON) ? "POISON" : "");


        if (!MOB_FLAGGED(mob, MOB_SENTINEL) && !MOB_FLAGGED(mob, MOB_STAY_ZONE) && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                            "- Neither SENTINEL nor STAY_ZONE bits set.\r\n");

        if (MOB_FLAGGED(mob, MOB_SPEC) && (found = 1))
          snprintf(buf + len, sizeof(buf) - len,
                            "- SPEC flag needs to be removed.\r\n");

        /* Additional mob checks.*/
        if (found) {
          send_to_char(ch,
                  "%s[%5d]%s %-30s: %s\r\n",
                  CCCYN(ch, C_NRM), GET_MOB_VNUM(mob),
                  CCYEL(ch, C_NRM), GET_NAME(mob),
                  CCNRM(ch, C_NRM));
          send_to_char(ch, "%s", buf);
        }
        /* reset buffers and found flag */
        strcpy(buf, "");
        found = 0;
        len = 0;
      }   /* mob is in zone */
    }  /* check mobs */

 /* Check objects */
  send_to_char(ch, "\r\nChecking Objects for limits...\r\n");
  for (i=0; i<top_of_objt; i++) {
    if (real_zone_by_thing(obj_index[i].vnum) == zrnum) { /*is object in this zone?*/
      obj = &obj_proto[i];
      switch (GET_OBJ_TYPE(obj)) {
        case ITEM_MONEY:
          if ((value = GET_OBJ_VAL(obj, 0))>MAX_OBJ_COINS_ALLOWED && (found=1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Is worth %d (money limit %d coins).\r\n",
                                 value, MAX_OBJ_COINS_ALLOWED);
          break;
        case ITEM_WEAPON:
          if (GET_OBJ_VAL(obj, 3) >= NUM_ATTACK_TYPES && (found=1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- has out of range attack type %d.\r\n",
                                 GET_OBJ_VAL(obj, 3));

          if (GET_OBJ_AVG_DAM(obj)>MAX_DAM_ALLOWED && (found=1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Damroll is %2.1f (limit %d)\r\n",
                                 GET_OBJ_AVG_DAM(obj), MAX_DAM_ALLOWED);
          break;
        case ITEM_ARMOR:
          ac=GET_OBJ_VAL(obj,0);
          for (j=0; j<TOTAL_WEAR_CHECKS;j++) {
            if (CAN_WEAR(obj,zarmor[j].bitvector) && (ac>zarmor[j].ac_allowed) && (found=1))
              len += snprintf(buf + len, sizeof(buf) - len,
                                   "- Has AC %d (%s limit is %d)\r\n",
                                   ac, zarmor[j].message, zarmor[j].ac_allowed);
          }
          break;

      }  /*switch on Item_Type*/

      if (!CAN_WEAR(obj, ITEM_WEAR_TAKE)) {
        if ((GET_OBJ_COST(obj) || (GET_OBJ_WEIGHT(obj) && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN)) &&
           (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- is NO_TAKE, but has cost (%d) or weight (%d) set.\r\n",
                          GET_OBJ_COST(obj), GET_OBJ_WEIGHT(obj));
      } else {
        if (GET_OBJ_COST(obj) == 0 && (found=1) && GET_OBJ_TYPE(obj) != ITEM_TRASH)
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- has 0 cost (min. 1).\r\n");

        if (GET_OBJ_WEIGHT(obj) == 0 && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- has 0 weight (min. 1).\r\n");

        if (GET_OBJ_WEIGHT(obj) > MAX_OBJ_WEIGHT && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "  Weight is too high: %d (limit  %d).\r\n",
                          GET_OBJ_WEIGHT(obj), MAX_OBJ_WEIGHT);


        if (GET_OBJ_COST(obj) > MAX_OBJ_COST && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- has %d cost (max %d).\r\n",
                          GET_OBJ_COST(obj), MAX_OBJ_COST);
      }

      if (GET_OBJ_LEVEL(obj) > LVL_IMMORT-1 && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
                          "- has min level set to %d (max %d).\r\n",
                          GET_OBJ_LEVEL(obj), LVL_IMMORT-1);

      if (obj->main_description && *obj->main_description &&
          GET_OBJ_TYPE(obj) != ITEM_STAFF &&
          GET_OBJ_TYPE(obj) != ITEM_WAND &&
          GET_OBJ_TYPE(obj) != ITEM_SCROLL &&
          GET_OBJ_TYPE(obj) != ITEM_NOTE && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
                          "- has main_description set, but is inappropriate type.\r\n");

      /*first check for over-all affections*/
      for (affs=0, j = 0; j < MAX_OBJ_AFFECT; j++)
        if (obj->affected[j].modifier) affs++;

      if (affs>MAX_AFFECTS_ALLOWED && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
                          "- has %d affects (limit %d).\r\n",
                             affs, MAX_AFFECTS_ALLOWED);

      /*check for out of range affections. */
      for (j=0;j<MAX_OBJ_AFFECT;j++)
        if (zaffs[(int)obj->affected[j].location].max_aff != -99 && /* only care if a range is set */
            (obj->affected[j].modifier > zaffs[(int)obj->affected[j].location].max_aff ||
             obj->affected[j].modifier < zaffs[(int)obj->affected[j].location].min_aff ||
             zaffs[(int)obj->affected[j].location].min_aff == zaffs[(int)obj->affected[j].location].max_aff) && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- apply to %s is %d (limit %d - %d).\r\n",
                               zaffs[(int)obj->affected[j].location].message,
                               obj->affected[j].modifier,
                               zaffs[(int)obj->affected[j].location].min_aff,
                               zaffs[(int)obj->affected[j].location].max_aff);


     for (ext2 = NULL, ext = obj->ex_description; ext; ext = ext->next)
       if (strncmp(ext->description, "   ", 3))
         ext2 = ext;

     if (ext2 && (found = 1))
       snprintf(buf + len, sizeof(buf) - len,
                       "- has unformatted extra description\r\n");
     /* Additional object checks. */
     if (found) {
        send_to_char(ch, "[%5d] %-30s: \r\n", GET_OBJ_VNUM(obj), obj->short_description);
        send_to_char(ch, "%s", buf);
      }
      strcpy(buf, "");
      len = 0;
      found = 0;
    }   /*object is in zone*/
  } /*check objects*/

  /* Check rooms */
  send_to_char(ch, "\r\nChecking Rooms for limits...\r\n");
  for (i=0; i<top_of_world;i++) {
    if (world[i].zone==zrnum) {
      for (j = 0; j < DIR_COUNT; j++) {
        /*check for exit, but ignore off limits if you're in an offlimit zone*/
        if (!world[i].dir_option[j])
          continue;
        exroom=world[i].dir_option[j]->to_room;
        if (exroom==NOWHERE)
          continue;
        if (world[exroom].zone == zrnum)
          continue;
        if (world[exroom].zone == world[i].zone)
          continue;

        for (k=0;offlimit_zones[k] != -1;k++) {
          if (world[exroom].zone == real_zone(offlimit_zones[k]) && (found = 1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Exit %s cannot connect to %d (zone off limits).\r\n",
                            dirs[j], world[exroom].number);
        } /* for (k.. */
      } /* cycle directions */

     if (ROOM_FLAGGED(i, ROOM_ATRIUM) || ROOM_FLAGGED(i, ROOM_HOUSE) || ROOM_FLAGGED(i, ROOM_HOUSE_CRASH) || ROOM_FLAGGED(i, ROOM_OLC) || ROOM_FLAGGED(i, ROOM_BFS_MARK))
         len += snprintf(buf + len, sizeof(buf) - len,
         "- Has illegal affection bits set (%s %s %s %s %s)\r\n",
                            ROOM_FLAGGED(i, ROOM_ATRIUM) ? "ATRIUM" : "",
                            ROOM_FLAGGED(i, ROOM_HOUSE) ? "HOUSE" : "",
                            ROOM_FLAGGED(i, ROOM_HOUSE_CRASH) ? "HCRSH" : "",
                            ROOM_FLAGGED(i, ROOM_OLC) ? "OLC" : "",
                            ROOM_FLAGGED(i, ROOM_BFS_MARK) ? "*" : "");

      if ((MIN_ROOM_DESC_LENGTH) && strlen(world[i].description)<MIN_ROOM_DESC_LENGTH && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
          "- Room description is too short. (%4.4d of min. %d characters).\r\n",
          (int)strlen(world[i].description), MIN_ROOM_DESC_LENGTH);

      if (strncmp(world[i].description, "   ", 3) && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Room description not formatted with indent (/fi in the editor).\r\n");

      /* strcspan = size of text in first arg before any character in second arg */
      if ((strcspn(world[i].description, "\r\n")>MAX_COLOUMN_WIDTH) && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Room description not wrapped at %d chars (/fi in the editor).\r\n",
                             MAX_COLOUMN_WIDTH);

     for (ext2 = NULL, ext = world[i].ex_description; ext; ext = ext->next)
       if (strncmp(ext->description, "   ", 3))
         ext2 = ext;

     if (ext2 && (found = 1))
       len += snprintf(buf + len, sizeof(buf) - len,
                       "- has unformatted extra description\r\n");

      if (found) {
        send_to_char(ch, "[%5d] %-30s: \r\n",
                       world[i].number, world[i].name ? world[i].name : "An unnamed room");
        send_to_char(ch, "%s", buf);
        strcpy(buf, "");
        len = 0;
        found = 0;
      }
    } /*is room in this zone?*/
  } /*checking rooms*/

  for (i=0; i<top_of_world;i++) {
    if (world[i].zone==zrnum) {
      m++;
      for (j = 0, k = 0; j < DIR_COUNT; j++)
        if (!world[i].dir_option[j])
          k++;

      if (k == DIR_COUNT)
        l++;
    }
  }
  if (l * 3 > m)
    send_to_char(ch, "More than 1/3 of the rooms are not linked.\r\n");

}

static void mob_checkload(struct char_data *ch, mob_vnum mvnum)
{
  int cmd_no;
  zone_rnum zone;
  mob_rnum mrnum = real_mobile(mvnum);

  if (mrnum == NOBODY) {
      send_to_char(ch, "That mob does not exist.\r\n");
      return;
  }

  send_to_char(ch, "Checking load info for the mob %s...\r\n",
                    mob_proto[mrnum].player.short_descr);

  for (zone=0; zone <= top_of_zone_table; zone++) {
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      if (ZCMD2.command != 'M')
        continue;

      /* read a mobile */
      if (ZCMD2.arg1 == mrnum) {
        send_to_char(ch, "  [%5d] %s (%d MAX)\r\n",
                         world[ZCMD2.arg3].number,
                         world[ZCMD2.arg3].name,
                         ZCMD2.arg2);
      }
    }
  }
}

static void obj_checkload(struct char_data *ch, obj_vnum ovnum)
{
  int cmd_no;
  zone_rnum zone;
  obj_rnum ornum = real_object(ovnum);
  room_vnum lastroom_v = 0;
  room_rnum lastroom_r = 0;
  mob_rnum lastmob_r = 0;

  if (ornum ==NOTHING) {
    send_to_char(ch, "That object does not exist.\r\n");
    return;
  }

  send_to_char(ch, "Checking load info for the obj %s...\r\n",
                   obj_proto[ornum].short_description);

  for (zone=0; zone <= top_of_zone_table; zone++) {
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      switch (ZCMD2.command) {
        case 'M':
          lastroom_v = world[ZCMD2.arg3].number;
          lastroom_r = ZCMD2.arg3;
          lastmob_r = ZCMD2.arg1;
          break;
        case 'O':                   /* read an object */
          lastroom_v = world[ZCMD2.arg3].number;
          lastroom_r = ZCMD2.arg3;
          if (ZCMD2.arg1 == ornum)
            send_to_char(ch, "  [%5d] %s (%d Max)\r\n",
                             lastroom_v,
                             world[lastroom_r].name,
                             ZCMD2.arg2);
          break;
        case 'P':                   /* object to object */
          if (ZCMD2.arg1 == ornum)
            send_to_char(ch, "  [%5d] %s (Put in another object [%d Max])\r\n",
                             lastroom_v,
                             world[lastroom_r].name,
                             ZCMD2.arg2);
          break;
        case 'G':                   /* obj_to_char */
          if (ZCMD2.arg1 == ornum)
            send_to_char(ch, "  [%5d] %s (Given to %s [%d][%d Max])\r\n",
                             lastroom_v,
                             world[lastroom_r].name,
                             mob_proto[lastmob_r].player.short_descr,
                             mob_index[lastmob_r].vnum,
                             ZCMD2.arg2);
          break;
        case 'E':                   /* object to equipment list */
          if (ZCMD2.arg1 == ornum)
            send_to_char(ch, "  [%5d] %s (Equipped to %s [%d][%d Max])\r\n",
                             lastroom_v,
                             world[lastroom_r].name,
                             mob_proto[lastmob_r].player.short_descr,
                             mob_index[lastmob_r].vnum,
                             ZCMD2.arg2);
          break;
        case 'R': /* rem obj from room */
          lastroom_v = world[ZCMD2.arg1].number;
          lastroom_r = ZCMD2.arg1;
          if (ZCMD2.arg2 == ornum)
            send_to_char(ch, "  [%5d] %s (Removed from room)\r\n",
                             lastroom_v,
                             world[lastroom_r].name);
          break;
      }/* switch */
    } /*for cmd_no......*/
  }  /*for zone...*/
}

static void trg_checkload(struct char_data *ch, trig_vnum tvnum)
{
  int cmd_no, found = 0;
  zone_rnum zone;
  trig_rnum trnum = real_trigger(tvnum);
  room_vnum lastroom_v = 0;
  room_rnum lastroom_r = 0, k;
  mob_rnum lastmob_r = 0, i;
  obj_rnum lastobj_r = 0, j;
  struct trig_proto_list *tpl;

  if (trnum == NOTHING) {
    send_to_char(ch, "That trigger does not exist.\r\n");
    return;
  }

  send_to_char(ch, "Checking load info for the %s trigger '%s':\r\n",
                    trig_index[trnum]->proto->attach_type == MOB_TRIGGER ? "mobile" :
                    (trig_index[trnum]->proto->attach_type == OBJ_TRIGGER ? "object" : "room"),
                    trig_index[trnum]->proto->name);

  for (zone=0; zone <= top_of_zone_table; zone++) {
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      switch (ZCMD2.command) {
        case 'M':
          lastroom_v = world[ZCMD2.arg3].number;
          lastroom_r = ZCMD2.arg3;
          lastmob_r = ZCMD2.arg1;
          break;
        case 'O':                   /* read an object */
          lastroom_v = world[ZCMD2.arg3].number;
          lastroom_r = ZCMD2.arg3;
          lastobj_r = ZCMD2.arg1;
          break;
        case 'P':                   /* object to object */
          lastobj_r = ZCMD2.arg1;
          break;
        case 'G':                   /* obj_to_char */
          lastobj_r = ZCMD2.arg1;
          break;
        case 'E':                   /* object to equipment list */
          lastobj_r = ZCMD2.arg1;
          break;
        case 'R':                   /* rem obj from room */
          lastroom_v = 0;
          lastroom_r = 0;
          lastobj_r = 0;
          lastmob_r = 0;
        case 'T':                   /* trigger to something */
          if (ZCMD2.arg2 != trnum)
            break;
          if (ZCMD2.arg1 == MOB_TRIGGER) {
            send_to_char(ch, "mob [%5d] %-60s (zedit room %5d)\r\n",
                               mob_index[lastmob_r].vnum,
                               mob_proto[lastmob_r].player.short_descr,
                               lastroom_v);
            found = 1;
          } else if (ZCMD2.arg1 == OBJ_TRIGGER) {
            send_to_char(ch, "obj [%5d] %-60s  (zedit room %d)\r\n",
                               obj_index[lastobj_r].vnum,
                               obj_proto[lastobj_r].short_description,
                               lastroom_v);
            found = 1;
          } else if (ZCMD2.arg1==WLD_TRIGGER) {
            send_to_char(ch, "room [%5d] %-60s (zedit)\r\n",
                               lastroom_v,
                               world[lastroom_r].name);
            found = 1;
          }
        break;
      } /* switch */
    } /*for cmd_no......*/
  }  /*for zone...*/

  for (i = 0; i < top_of_mobt; i++) {
    if (!mob_proto[i].proto_script)
      continue;

    for (tpl = mob_proto[i].proto_script;tpl;tpl = tpl->next)
      if (tpl->vnum == tvnum) {
        send_to_char(ch, "mob [%5d] %s\r\n",
                         mob_index[i].vnum,
                         mob_proto[i].player.short_descr);
        found = 1;
      }
  }

  for (j = 0; j < top_of_objt; j++) {
    if (!obj_proto[j].proto_script)
      continue;

    for (tpl = obj_proto[j].proto_script;tpl;tpl = tpl->next)
      if (tpl->vnum == tvnum) {
        send_to_char(ch, "obj [%5d] %s\r\n",
                         obj_index[j].vnum,
                         obj_proto[j].short_description);
        found = 1;
      }
  }

  for (k = 0;k < top_of_world; k++) {
    if (!world[k].proto_script)
      continue;

    for (tpl = world[k].proto_script;tpl;tpl = tpl->next)
      if (tpl->vnum == tvnum) {
        send_to_char(ch, "room[%5d] %s\r\n",
                         world[k].number,
                         world[k].name);
        found = 1;
      }
  }

  if (!found)
    send_to_char(ch, "This trigger is not attached to anything.\r\n");
}

ACMD(do_checkloadstatus)
{
  char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  two_arguments(argument, buf1, buf2);

  if ((!*buf1) || (!*buf2) || (!isdigit(*buf2))) {
    send_to_char(ch, "Checkload <M | O | T> <vnum>\r\n");
    return;
  }

  if (LOWER(*buf1) == 'm') {
    mob_checkload(ch, atoi(buf2));
    return;
  }

  if (LOWER(*buf1) == 'o') {
    obj_checkload(ch, atoi(buf2));
    return;
  }

  if (LOWER(*buf1) == 't') {
    trg_checkload(ch, atoi(buf2));
    return;
  }
}
/* Zone Checker code above. */

/* (c) 1996-97 Erwin S. Andreasen. */
ACMD(do_copyover)
{
  FILE *fp;
  struct descriptor_data *d, *d_next;
  char buf [100], buf2[100];

  fp = fopen (COPYOVER_FILE, "w");
    if (!fp) {
      send_to_char (ch, "Copyover file not writeable, aborted.\n\r");
      return;
    }

   sprintf (buf, "\n\r *** COPYOVER by %s - please remain seated!\n\r", GET_NAME(ch));

   /* write boot_time as first line in file */
   fprintf(fp, "%ld\n", (long)boot_time);

   /* For each playing descriptor, save its state */
   for (d = descriptor_list; d ; d = d_next) {
     struct char_data * och = d->character;
   
   /* If d is currently in someone else's body, return them. */  
   if (och && d->original)
     return_to_char(och);
        
   /* We delete from the list , so need to save this */
     d_next = d->next;

  /* drop those logging on */
   if (!d->character || d->connected > CON_PLAYING) {
     write_to_descriptor (d->descriptor, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r");
     close_socket (d); /* throw'em out */
   } else {
      fprintf (fp, "%d %ld %s %s %s\n", d->descriptor, GET_PREF(och), GET_NAME(och), d->host, CopyoverGet(d));
      /* save och */
      GET_LOADROOM(och) = GET_ROOM_VNUM(IN_ROOM(och));
      Crash_rentsave(och,0);
      save_char(och);
      write_to_descriptor (d->descriptor, buf);
    }
  }

  fprintf (fp, "-1\n");
  fclose (fp);

  /* exec - descriptors are inherited */
  sprintf (buf, "%d", port);
  sprintf (buf2, "-C%d", mother_desc);

  /* Ugh, seems it is expected we are 1 step above lib - this may be dangerous! */
  if(chdir ("..") != 0) {
    log("Error changing working directory: %s", strerror(errno));
    send_to_char(ch, "Error changing working directory: %s.", strerror(errno));
    exit(1);
  }

  /* Close reserve and other always-open files and release other resources */
  execl (EXE_FILE, "circle", buf2, buf, (char *) NULL);

  /* Failed - successful exec will not return */
  perror ("do_copyover: execl");
  send_to_char (ch, "Copyover FAILED!\n\r");

  exit (1); /* too much trouble to try to recover! */
}

ACMD(do_peace)
{
  struct char_data *vict, *next_v;

  act ("As $n makes a strange arcane gesture, a golden light descends\r\n"
       "from the heavens stopping all the fighting.\r\n",FALSE, ch, 0, 0, TO_ROOM);
  send_to_room(IN_ROOM(ch), "Everything is quite peaceful now.\r\n");
  for(vict=world[IN_ROOM(ch)].people; vict; vict=next_v) {
    next_v = vict->next_in_room;
    if (FIGHTING(vict))
      stop_fighting(vict);
    if (IS_NPC(vict))
      clearMemory(vict);
  }
}

ACMD(do_zpurge)
{
  int vroom, room, zone = 0;
  char arg[MAX_INPUT_LENGTH];
  int purge_all = FALSE;
  one_argument(argument, arg);
  if (*arg == '.' || !*arg) {
    zone = world[IN_ROOM(ch)].zone;
  }
  else if (is_number(arg)) {
    zone = real_zone(atoi(arg));
    if (zone == NOWHERE || zone > top_of_zone_table) {
      send_to_char(ch, "That zone doesn't exist!\r\n");
      return;
    }
  }
  else if (*arg == '*') {
    purge_all = TRUE;
  }
  else {
    send_to_char(ch, "That isn't a valid zone number!\r\n");
    return;
  }
  if (GET_LEVEL(ch) < LVL_GOD && !can_edit_zone(ch, zone)) {
    send_to_char(ch, "You can only purge your own zone!\r\n");
    return;
  }
  if (!purge_all) {
    for (vroom = zone_table[zone].bot; vroom <= zone_table[zone].top; vroom++) {
      purge_room(real_room(vroom));
    }
    send_to_char(ch, "Purged zone #%d: %s.\r\n", zone_table[zone].number, zone_table[zone].name);
    mudlog(NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s purged zone %d (%s)", GET_NAME(ch), zone_table[zone].number, zone_table[zone].name);
  }
  else {
    for (room = 0; room <= top_of_world; room++) {
      purge_room(room);
    }
    send_to_char(ch, "Purged world.\r\n");
    mudlog(NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s purged entire world.", GET_NAME(ch));
  }
}

/** Used to read and gather a bit of information about external log files while
 * in game.
 * Makes use of the '\t' color codes in the file status information.
 * Some of the methods used are a bit wasteful (reading through the file
 * multiple times to gather diagnostic information), but it is
 * assumed that the files read with this function will never be very large.
 * Files to be read are assumed to exist and be readable and if they aren't,
 * log the name of the missing file.
 */
ACMD(do_file)
{
  /* Local variables */
  int def_lines_to_read = 15;  /* Set the default num lines to be read. */
  int max_lines_to_read = 300; /* Maximum number of lines to read. */
  FILE *req_file;              /* Pointer to file to be read. */
  size_t req_file_size = 0;    /* Size of file to be read. */
  int req_file_lines = 0;      /* Number of total lines in file to be read. */
  int lines_read = 0;     /* Counts total number of lines read from the file. */
  int req_lines = 0;      /* Number of lines requested to be displayed. */
  int i;                  /* Generic loop counter. */
  int l;                  /* Marks choice of file in fields array. */
  char field[MAX_INPUT_LENGTH];  /* Holds users choice of file to be read. */
  char value[MAX_INPUT_LENGTH];  /* Holds # lines to be read, if requested. */
  char buf[MAX_STRING_LENGTH];   /* Display buffer for req_file. */

  /* Defines which files are available to read. */
  struct file_struct {
    char *cmd;          /* The 'name' of the file to view */
    char level;         /* Minimum level needed to view. */
    char *file;         /* The file location, relative to the working dir. */
    int read_backwards; /* Should the file be read backwards by default? */
  } fields[] = {
    { "xnames",         LVL_GOD,    XNAME_FILE,          TRUE},
    { "levels",         LVL_GOD,    LEVELS_LOGFILE,      TRUE},
    { "rip",            LVL_GOD,    RIP_LOGFILE,         TRUE},
    { "players",        LVL_GOD,    NEWPLAYERS_LOGFILE,  TRUE},
    { "errors",         LVL_GOD,    ERRORS_LOGFILE,      TRUE},
    { "godcmds",        LVL_GOD,    GODCMDS_LOGFILE,     TRUE},
    { "syslog",         LVL_GOD,    SYSLOG_LOGFILE,      TRUE},
    { "crash",          LVL_GOD,    CRASH_LOGFILE,       TRUE},
    { "help",           LVL_GOD,    HELP_LOGFILE,        TRUE},
    { "changelog",      LVL_GOD,    CHANGE_LOG_FILE,     FALSE},
    { "deletes",        LVL_GOD,    DELETES_LOGFILE,     TRUE},
    { "restarts",       LVL_GOD,    RESTARTS_LOGFILE,    TRUE},
    { "usage",          LVL_GOD,    USAGE_LOGFILE,       TRUE},
    { "badpws",         LVL_GOD,    BADPWS_LOGFILE,      TRUE},
    { "olc",            LVL_GOD,    OLC_LOGFILE,         TRUE},
    { "trigger",        LVL_GOD,    TRIGGER_LOGFILE,     TRUE},
    { "\n", 0, "\n", FALSE } /* This must be the last entry */
  };

   /* Initialize buffer */
   buf[0] = '\0';

   /**/
   /* End function variable set-up and initialization. */

   skip_spaces(&argument);

   /* Display usage if no argument. */
   if (!*argument) {
     send_to_char(ch, "USAGE: file <filename> <num lines>\r\n\r\nFile options:\r\n");
     for (i = 0; fields[i].level; i++)
       if (fields[i].level <= GET_LEVEL(ch))
         send_to_char(ch, "%-15s%s\r\n", fields[i].cmd, fields[i].file);
     return;
   }

   /* Begin validity checks. Is the file choice valid and accessible? */
   /**/
   /* There are some arguments, deal with them. */
   two_arguments(argument, field, value);

   for (l = 0; *(fields[l].cmd) != '\n'; l++)
   {
     if (!strncmp(field, fields[l].cmd, strlen(field)))
       break;
   }

   if(*(fields[l].cmd) == '\n') {
     send_to_char(ch, "'%s' is not a valid file.\r\n", field);
     return;
   }

   if (GET_LEVEL(ch) < fields[l].level) {
     send_to_char(ch, "You have not achieved a high enough level to view '%s'.\r\n",
         fields[l].cmd);
     return;
   }

   /* Number of lines to view. Default is 15. */
   if(!*value)
     req_lines = def_lines_to_read;
   else if (!isdigit(*value))
   {
     /* This check forces the requisite positive digit and prevents negative
      * numbers of lines from being read. */
     send_to_char(ch, "'%s' is not a valid number of lines to view.\r\n", value);
     return;
   }
   else
   {
     req_lines = atoi(value);
     /* Limit the maximum number of lines */
     req_lines = MIN( req_lines, max_lines_to_read );
   }

   /* Must be able to access the file on disk. */
   if (!(req_file=fopen(fields[l].file,"r"))) {
     send_to_char(ch, "The file %s can not be opened.\r\n", fields[l].file);
     mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: Error opening file %s using 'file' command.", fields[l].file);
     return;
   }
   /**/
   /* End validity checks. From here on, the file should be viewable. */

   /* Diagnostic information about the file */
   req_file_size = file_sizeof(req_file);
   req_file_lines = file_numlines(req_file);

   snprintf( buf, sizeof(buf),
       "\tgFile:\tn %s\tg; Min. Level to read:\tn %d\tg; File Location:\tn %s\tg\r\n"
       "File size (bytes):\tn %ld\tg; Total num lines:\tn %d\r\n",
       fields[l].cmd, fields[l].level, fields[l].file, (long) req_file_size,
       req_file_lines);

   /* Should the file be 'headed' or 'tailed'? */
   if ( (fields[l].read_backwards == TRUE) && (req_lines < req_file_lines) )
   {
     snprintf( buf + strlen(buf), sizeof(buf) - strlen(buf),
               "\tgReading from the tail of the file.\tn\r\n\r\n" );
     lines_read = file_tail( req_file, buf, sizeof(buf), req_lines );
   }
   else
   {
     snprintf( buf + strlen(buf), sizeof(buf) - strlen(buf),
              "\tgReading from the head of the file.\tn\r\n\r\n" );
     lines_read = file_head( req_file, buf, sizeof(buf), req_lines );
   }

   /* Since file_head and file_tail will add the overflow message, we
    * don't check for status here. */
   if ( lines_read == req_file_lines )
   {
     /* We're reading the entire file */
     snprintf( buf + strlen(buf), sizeof(buf) - strlen(buf),
         "\r\n\tgEntire file returned (\tn%d \tglines).\tn\r\n",
         lines_read );
   }
   else if ( lines_read == max_lines_to_read )
   {
     snprintf( buf + strlen(buf), sizeof(buf) - strlen(buf),
         "\r\n\tgMaximum number of \tn%d \tglines returned.\tn\r\n",
         lines_read );
   }
   else
   {
     snprintf( buf + strlen(buf), sizeof(buf) - strlen(buf),
         "\r\n%d \tglines returned.\tn\r\n",
         lines_read );
   }

   /* Clean up before return */
   fclose(req_file);

   page_string(ch->desc, buf, 1);
}

ACMD(do_changelog)
{
  time_t rawtime;
  char timestr[12], line[READ_SIZE], last_buf[READ_SIZE],
      buf[READ_SIZE];
  FILE *fl, *new;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: changelog <change>\r\n");
    return;
  }

  sprintf(buf, "%s.bak", CHANGE_LOG_FILE);
  if (rename(CHANGE_LOG_FILE, buf)) {
    mudlog(BRF, LVL_IMPL, TRUE,
           "SYSERR: Error making backup changelog file (%s)", buf);
    return;
  }

  if (!(fl = fopen(buf, "r"))) {
    mudlog(BRF, LVL_IMPL, TRUE,
           "SYSERR: Error opening backup changelog file (%s)", buf);
    return;
  }

  if (!(new = fopen(CHANGE_LOG_FILE, "w"))) {
    mudlog(BRF, LVL_IMPL, TRUE,
           "SYSERR: Error opening new changelog file (%s)", CHANGE_LOG_FILE);
    return;
  }

  while (get_line(fl, line)) {
    if (*line != '[')
      fprintf(new, "%s\n", line);
    else {
      strcpy(last_buf, line);
      break;
    }
  }

  rawtime = time(0);
  strftime(timestr, sizeof(timestr), "%b %d %Y", localtime(&rawtime));

  sprintf(buf, "[%s] - %s", timestr, GET_NAME(ch));

  fprintf(new, "%s\n", buf);
  fprintf(new, "  %s\n", argument);

  if (strcmp(buf, last_buf))
    fprintf(new, "%s\n", line);

  while (get_line(fl, line))
    fprintf(new, "%s\n", line);

  fclose(fl);
  fclose(new);
  send_to_char(ch, "Change added.\r\n");
}

#define PLIST_FORMAT \
  "Usage: plist [minlev[-maxlev]] [-n name] [-d days] [-h hours] [-i] [-m]"

ACMD(do_plist)
{
  int i, len = 0, count = 0;
  char mode, buf[MAX_STRING_LENGTH * 20], name_search[MAX_NAME_LENGTH], timestr[MAX_STRING_LENGTH];
  struct time_info_data time_away;
  int low = 0, high = LVL_IMPL, low_day = 0, high_day = 10000, low_hr = 0, high_hr = 24;

  skip_spaces(&argument);
  strcpy(buf, argument);        /* strcpy: OK (sizeof: argument == buf) */
  name_search[0] = '\0';

  while (*buf) {
    char arg[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH];

    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      if (sscanf(arg, "%d-%d", &low, &high) == 1)
        high = low;
      strcpy(buf, buf1);        /* strcpy: OK (sizeof: buf1 == buf) */
    } else if (*arg == '-') {
      mode = *(arg + 1);        /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'l':
        half_chop(buf1, arg, buf);
        sscanf(arg, "%d-%d", &low, &high);
        break;
      case 'n':
        half_chop(buf1, name_search, buf);
        break;
      case 'i':
        strcpy(buf, buf1);
        low = LVL_IMMORT;
        break;
      case 'm':
        strcpy(buf, buf1);
        high = LVL_IMMORT - 1;
        break;
      case 'd':
        half_chop(buf1, arg, buf);
        if (sscanf(arg, "%d-%d", &low_day, &high_day) == 1)
          high_day = low_day;
        break;
      case 'h':
        half_chop(buf1, arg, buf);
        if (sscanf(arg, "%d-%d", &low_hr, &high_hr) == 1)
          high_hr = low_hr;
        break;
      default:
        send_to_char(ch, "%s\r\n", PLIST_FORMAT);
        return;
      }
    } else {
      send_to_char(ch, "%s\r\n", PLIST_FORMAT);
      return;
    }
  }

  len = 0;
  len += snprintf(buf + len, sizeof(buf) - len, "\tW[ Id] (Lv) Name         Last\tn\r\n"
                  "%s-------------------------------------%s\r\n", CCCYN(ch, C_NRM),
                  CCNRM(ch, C_NRM));

  for (i = 0; i <= top_of_p_table; i++) {
    if (player_table[i].level < low || player_table[i].level > high)
      continue;

    time_away = *real_time_passed(time(0), player_table[i].last);

    if (*name_search && str_cmp(name_search, player_table[i].name))
      continue;

    if (time_away.day > high_day || time_away.day < low_day)
      continue;
    if (time_away.hours > high_hr || time_away.hours < low_hr)
      continue;

    strftime(timestr, sizeof(timestr), "%c", localtime(&player_table[i].last));

    len += snprintf(buf + len, sizeof(buf) - len, "[%3ld] (%2d) %c%-15s %s\r\n",
                    player_table[i].id, player_table[i].level,
                    UPPER(*player_table[i].name), player_table[i].name + 1, timestr);
    count++;
  }
  snprintf(buf + len, sizeof(buf) - len, "%s-------------------------------------%s\r\n"
           "%d players listed.\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), count);
  page_string(ch->desc, buf, TRUE);
}

ACMD(do_wizupdate)
{
  run_autowiz();
  send_to_char(ch, "Wizlists updated.\n\r");
}

/* NOTE: This is called from perform_set */
bool change_player_name(struct char_data *ch, struct char_data *vict, char *new_name)
{
  struct char_data *temp_ch=NULL;
  int plr_i = 0, i, k;
  char old_name[MAX_NAME_LENGTH], old_pfile[50], new_pfile[50], buf[MAX_STRING_LENGTH];

  if (!ch)
  {
    log("SYSERR: No char passed to change_player_name.");
    return FALSE;
  }

  if (!vict)
  {
    log("SYSERR: No victim passed to change_player_name.");
    send_to_char(ch, "Invalid victim.\r\n");
    return FALSE;
  }

  if (!new_name || !(*new_name) || strlen(new_name) < 2 ||
      strlen(new_name) > MAX_NAME_LENGTH || !valid_name(new_name) ||
      fill_word(new_name) || reserved_word(new_name) ) {
    send_to_char(ch, "Invalid new name.\r\n");
    return FALSE;
  }

  /* Check that someone with new_name isn't already logged in */
  if ((temp_ch = get_player_vis(ch, new_name, NULL, FIND_CHAR_WORLD)) != NULL) {
    send_to_char(ch, "Sorry, the new name already exists.\r\n");
    return FALSE;
  } else  {
    /* try to load the player off disk */
    CREATE(temp_ch, struct char_data, 1);
    clear_char(temp_ch);
    CREATE(temp_ch->player_specials, struct player_special_data, 1);
    new_mobile_data(temp_ch);
    if ((plr_i = load_char(new_name, temp_ch)) > -1) {
      free_char(temp_ch);
      send_to_char(ch, "Sorry, the new name already exists.\r\n");
      return FALSE;
    }
  }

  /* New playername is OK - find the entry in the index */
  for (i = 0; i <= top_of_p_table; i++)
    if (player_table[i].id == GET_IDNUM(vict))
      break;

  if (player_table[i].id != GET_IDNUM(vict))
  {
    send_to_char(ch, "Your target was not found in the player index.\r\n");
    log("SYSERR: Player %s, with ID %ld, could not be found in the player index.", GET_NAME(vict), GET_IDNUM(vict));
    return FALSE;
  }

  /* Set up a few variables that will be needed */
  sprintf(old_name, "%s", GET_NAME(vict));
  if (!get_filename(old_pfile, sizeof(old_pfile), PLR_FILE, old_name))
  {
    send_to_char(ch, "Unable to ascertain player's old pfile name.\r\n");
    return FALSE;
  }
  if (!get_filename(new_pfile, sizeof(new_pfile), PLR_FILE, new_name))
  {
    send_to_char(ch, "Unable to ascertain player's new pfile name.\r\n");
    return FALSE;
  }

  /* Now start changing the name over - all checks and setup have passed */
  free(player_table[i].name);              // Free the old name in the index
  player_table[i].name = strdup(new_name); // Insert the new name into the index
  for (k=0; (*(player_table[i].name+k) = LOWER(*(player_table[i].name+k))); k++);

  free(GET_PC_NAME(vict));
  GET_PC_NAME(vict) = strdup(CAP(new_name));    // Change the name in the victims char struct

  /* Rename the player's pfile */
  sprintf(buf, "mv %s %s", old_pfile, new_pfile);

  /* Save the changed player index - the pfile is saved by perform_set */
  save_player_index();

  mudlog(BRF, LVL_IMMORT, TRUE, "(GC) %s changed the name of %s to %s", GET_NAME(ch), old_name, new_name);

  if (vict->desc)  /* Descriptor is set if the victim is logged in */
    send_to_char(vict, "Your login name has changed from %s%s%s to %s%s%s.\r\n", CCYEL(vict, C_NRM), old_name, CCNRM(vict, C_NRM),
                                                                                 CCYEL(vict, C_NRM), new_name, CCNRM(vict, C_NRM));

  return TRUE;
}

ACMD(do_zlock)
{
  zone_vnum znvnum;
  zone_rnum zn;
  char      arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int       counter = 0;
  bool      fail = FALSE;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Usage: %szlock <zone number>%s\r\n", QYEL, QNRM);
    send_to_char(ch, "%s       zlock list%s\r\n\r\n", QYEL, QNRM);
    send_to_char(ch, "Locks a zone so that building or editing is not possible.\r\n");
    send_to_char(ch, "The 'list' shows all currently locked zones.\r\n");
    send_to_char(ch, "'zlock all' will lock every zone with the GRID flag set.\r\n");
    send_to_char(ch, "'zlock all all' will lock every zone in the MUD.\r\n");
    return;
  }
  if (is_abbrev(arg, "all")) {
    if (GET_LEVEL(ch) < LVL_GRGOD) {
      send_to_char(ch, "You do not have sufficient access to lock all zones.\r\n");
      return;
    }
    if (!*arg2) {
      for (zn = 0; zn <= top_of_zone_table; zn++) {
        if (!ZONE_FLAGGED(zn, ZONE_NOBUILD) && ZONE_FLAGGED(zn, ZONE_GRID)) {
          counter++;
          SET_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
          if (save_zone(zn)) {
            log("(GC) %s has locked zone %d", GET_NAME(ch), zone_table[zn].number);
          } else {
            fail = TRUE;
          }
        }
      }
    } else if (is_abbrev(arg2, "all")) {
      for (zn = 0; zn <= top_of_zone_table; zn++) {
        if (!ZONE_FLAGGED(zn, ZONE_NOBUILD)) {
          counter++;
          SET_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
          if (save_zone(zn)) {
            log("(GC) %s has locked zone %d", GET_NAME(ch), zone_table[zn].number);
          } else {
            fail = TRUE;
          }
        }
      }
    }
    if (counter == 0) {
      send_to_char(ch, "There are no unlocked zones to lock!\r\n");
      return;
    }
    if (fail) {
      send_to_char(ch, "Unable to save zone changes.  Check syslog!\r\n");
      return;
    }
    send_to_char(ch, "%d zones have now been locked.\r\n", counter);
    mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has locked ALL zones!", GET_NAME(ch));
    return;
  }
  if (is_abbrev(arg, "list")) {
    /* Show all locked zones */
    for (zn = 0; zn <= top_of_zone_table; zn++) {
      if (ZONE_FLAGGED(zn, ZONE_NOBUILD)) {
        if (!counter) send_to_char(ch, "Locked Zones\r\n");

        send_to_char(ch, "[%s%3d%s] %s%-*s %s%-1s%s\r\n",
          QGRN, zone_table[zn].number, QNRM, QCYN, count_color_chars(zone_table[zn].name)+30, zone_table[zn].name,
          QYEL, zone_table[zn].builders ? zone_table[zn].builders : "None.", QNRM);
        counter++;
      }
    }
    if (counter == 0) {
      send_to_char(ch, "There are currently no locked zones!\r\n");
    }
    return;
  }
  else if ((znvnum = atoi(arg)) == 0) {
    send_to_char(ch, "Usage: %szlock <zone number>%s\r\n", QYEL, QNRM);
    return;
  }

  if ((zn = real_zone(znvnum)) == NOWHERE) {
    send_to_char(ch, "That zone does not exist!\r\n");
    return;
  }

  /* Check the builder list */
  if (GET_LEVEL(ch) < LVL_GRGOD && !is_name(GET_NAME(ch), zone_table[zn].builders) && GET_OLC_ZONE(ch) != znvnum) {
    send_to_char(ch, "You do not have sufficient access to lock that zone!\r\n");
    return;
  }

  /* If we get here, player has typed 'zlock <num>' */
  if (ZONE_FLAGGED(zn, ZONE_NOBUILD)) {
    send_to_char(ch, "Zone %d is already locked!\r\n", znvnum);
    return;
  }
  SET_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
  if (save_zone(zn)) {
    mudlog(NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has locked zone %d", GET_NAME(ch), znvnum);
  }
  else
  {
    send_to_char(ch, "Unable to save zone changes.  Check syslog!\r\n");
  }
}

ACMD(do_zunlock)
{
  zone_vnum znvnum;
  zone_rnum zn;
  char      arg[MAX_INPUT_LENGTH];
  int       counter = 0;
  bool      fail = FALSE;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Usage: %szunlock <zone number>%s\r\n", QYEL, QNRM);
    send_to_char(ch, "%s       zunlock list%s\r\n\r\n", QYEL, QNRM);
    send_to_char(ch, "Unlocks a 'locked' zone to allow building or editing.\r\n");
    send_to_char(ch, "The 'list' shows all currently unlocked zones.\r\n");
    send_to_char(ch, "'zunlock all' will unlock every zone in the MUD.\r\n");
    return;
  }
  if (is_abbrev(arg, "all")) {
    if (GET_LEVEL(ch) < LVL_GRGOD) {
      send_to_char(ch, "You do not have sufficient access to lock zones.\r\n");
      return;
    }
    for (zn = 0; zn <= top_of_zone_table; zn++) {
      if (ZONE_FLAGGED(zn, ZONE_NOBUILD)) {
        counter++;
        REMOVE_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
        if (save_zone(zn)) {
          log("(GC) %s has unlocked zone %d", GET_NAME(ch), zone_table[zn].number);
        } else {
          fail = TRUE;
        }
      }
    }
    if (counter == 0) {
      send_to_char(ch, "There are no locked zones to unlock!\r\n");
      return;
    }
    if (fail) {
      send_to_char(ch, "Unable to save zone changes.  Check syslog!\r\n");
      return;
    }
    send_to_char(ch, "%d zones have now been unlocked.\r\n", counter);
    mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has unlocked ALL zones!", GET_NAME(ch));
    return;
  }
  if (is_abbrev(arg, "list")) {
    /* Show all unlocked zones */
    for (zn = 0; zn <= top_of_zone_table; zn++) {
      if (!ZONE_FLAGGED(zn, ZONE_NOBUILD)) {
        if (!counter) send_to_char(ch, "Unlocked Zones\r\n");

        send_to_char(ch, "[%s%3d%s] %s%-*s %s%-1s%s\r\n",
          QGRN, zone_table[zn].number, QNRM, QCYN, count_color_chars(zone_table[zn].name)+30, zone_table[zn].name,
          QYEL, zone_table[zn].builders ? zone_table[zn].builders : "None.", QNRM);
        counter++;
      }
    }
    if (counter == 0) {
      send_to_char(ch, "There are currently no unlocked zones!\r\n");
    }
    return;
  }
  else if ((znvnum = atoi(arg)) == 0) {
    send_to_char(ch, "Usage: %szunlock <zone number>%s\r\n", QYEL, QNRM);
    return;
  }

  if ((zn = real_zone(znvnum)) == NOWHERE) {
    send_to_char(ch, "That zone does not exist!\r\n");
    return;
  }

  /* Check the builder list */
  if (GET_LEVEL(ch) < LVL_GRGOD && !is_name(GET_NAME(ch), zone_table[zn].builders) && GET_OLC_ZONE(ch) != znvnum) {
    send_to_char(ch, "You do not have sufficient access to unlock that zone!\r\n");
    return;
  }

  /* If we get here, player has typed 'zunlock <num>' */
  if (!ZONE_FLAGGED(zn, ZONE_NOBUILD)) {
    send_to_char(ch, "Zone %d is already unlocked!\r\n", znvnum);
    return;
  }
  REMOVE_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
  if (save_zone(zn)) {
    mudlog(NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has unlocked zone %d", GET_NAME(ch), znvnum);
  }
  else
  {
    send_to_char(ch, "Unable to save zone changes.  Check syslog!\r\n");
  }
}

/* get highest vnum in recent player list  */
static int get_max_recent(void)
{
  struct recent_player *this;
  int iRet=0;

  this = recent_list;

  while (this)
  {
   if (this->vnum > iRet) iRet = this->vnum;
   this = this->next;
  }

  return iRet;
}

/* clear an item in recent player list */
static void clear_recent(struct recent_player *this)
{
  this->vnum = 0;
  this->time = 0;
  strcpy(this->name, "");
  strcpy(this->host, "");
  this->next = NULL;
}

/* create new blank player in recent players list */
static struct recent_player *create_recent(void)
{
  struct recent_player *newrecent;

  CREATE(newrecent, struct recent_player, 1);
  clear_recent(newrecent);
  newrecent->next = recent_list;
  recent_list = newrecent;

  newrecent->vnum = get_max_recent();
  newrecent->vnum++;
  return newrecent;
}

/* Add player to recent player list */
bool AddRecentPlayer(char *chname, char *chhost, bool newplr, bool cpyplr)
{
  struct recent_player *this;
  time_t ct;
  int max_vnum;

  if (!chname || !*chname) // dropped connection with no name given
       return FALSE;

  ct = time(0);  /* Grab the current time */

  this = create_recent();

  if (!this) return FALSE;

  this->time = ct;
  this->new_player = newplr;
  this->copyover_player = cpyplr;
  strcpy(this->host, chhost);
  strcpy(this->name, chname);
  max_vnum = get_max_recent();
  this->vnum = max_vnum;   /* Possibly should be +1 ? */

  return TRUE;
}

void free_recent_players(void) 
{
  struct recent_player *this;
  struct recent_player *temp;
  
  this = recent_list;
  
  while((temp = this) != NULL)
  {
	this = this->next;
	free(temp);  
  }  	
}

ACMD(do_recent)
{
  time_t ct;
  char timestr[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  int hits = 0, limit = 0, count = 0;
  struct recent_player *this;
  bool loc;

  one_argument(argument, arg);
  if (!*arg) {
    limit = 0;
  } else {
    limit = atoi(arg);
  }

  if (GET_LEVEL(ch) >= LVL_GRGOD) {  /* If High-Level Imm, then show Host IP */
    send_to_char(ch, " ID | DATE/TIME                | HOST IP                          | Player Name\r\n");
  } else {
    send_to_char(ch, " ID | DATE/TIME                | Player Name\r\n");
  }

  this = recent_list;
  while(this)
  {
    loc = FALSE;
    hits++;
    ct = this->time;
    strftime(timestr, sizeof(timestr), "%a %b %d %H:%M:%S %Y", localtime(&ct));

    if (*(this->host)) {
      if (!strcmp(this->host, "localhost")) loc = TRUE;
    }

    if ((limit == 0) || (count < limit))
    {
      if (GET_LEVEL(ch) >= LVL_GRGOD)   /* If High-Level Imm, then show Host IP */
      {
        if (this->new_player == TRUE) {
          send_to_char(ch, "%3d | %-24.24s | %s%-32s%s | %s %s(New Player)%s\r\n", this->vnum, timestr, loc ? QRED : "", this->host, QNRM, this->name, QYEL, QNRM);
        } else if (this->copyover_player == TRUE) {
          send_to_char(ch, "%3d | %-24.24s | %s%-32s%s | %s %s(Copyover)%s\r\n", this->vnum, timestr, loc ? QRED : "", this->host, QNRM, this->name, QCYN, QNRM);
        } else {
          send_to_char(ch, "%3d | %-24.24s | %s%-32s%s | %s\r\n", this->vnum, timestr, loc ? QRED : "", this->host, QNRM, this->name);
        }
      }
      else
      {
        if (this->new_player == TRUE) {
          send_to_char(ch, "%3d | %-24.24s | %s %s(New Player)%s\r\n", this->vnum, timestr, this->name, QYEL, QNRM);
        } else if (this->copyover_player == TRUE) {
          send_to_char(ch, "%3d | %-24.24s | %s %s(Copyover)%s\r\n", this->vnum, timestr, this->name, QCYN, QNRM);
        } else {
          send_to_char(ch, "%3d | %-24.24s | %s\r\n", this->vnum, timestr, this->name);
        }
      }
      count++;

      this = this->next;
    }
    else
    {
      this = NULL;
    }
  }

  ct = time(0);  /* Grab the current time */
  strftime(timestr, sizeof(timestr), "%c", localtime(&ct));
  send_to_char(ch, "Current Server Time: %s\r\nShowing %d players since last copyover/reboot\r\n", timestr, hits);
}


/* 5e system helpers */

/* Helper: map wear flags to our armor_slots[] index (-1 if not an armor slot) */
static int armor_slot_index_from_wear(const struct obj_data *obj) {
  if (!obj) return -1;

  /* IMPORTANT: use your project's wear flag macros.
     Typical tbaMUD macros: CAN_WEAR(obj, ITEM_WEAR_*) */
  if (CAN_WEAR(obj, ITEM_WEAR_HEAD))   return 0; /* "head"  */
  if (CAN_WEAR(obj, ITEM_WEAR_BODY))   return 1; /* "body"  */
  if (CAN_WEAR(obj, ITEM_WEAR_LEGS))   return 2; /* "legs"  */
  if (CAN_WEAR(obj, ITEM_WEAR_ARMS))   return 3; /* "arms"  */
  if (CAN_WEAR(obj, ITEM_WEAR_HANDS))  return 4; /* "hands" */
  if (CAN_WEAR(obj, ITEM_WEAR_FEET))   return 5; /* "feet"  */

  /* Shield is audited separately in AC compute; skip it here */
  if (CAN_WEAR(obj, ITEM_WEAR_SHIELD)) return -2; /* special */

  return -1;
}

/* Pretty: slot name (matches armor_slots[] order) */
static const char *slot_name_from_index(int idx) {
  switch (idx) {
    case 0: return "head";
    case 1: return "body";
    case 2: return "legs";
    case 3: return "arms";
    case 4: return "hands";
    case 5: return "feet";
    default: return "unknown";
  }
}

/* Wizard command: scan armor prototypes, validate per-piece fields (compact, paged, 25 lines) */
ACMD(do_audit)
{
  int found = 0, warned = 0;
  char arg[MAX_INPUT_LENGTH];
  bool do_armor = FALSE;
  bool do_weapons = FALSE;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char(ch, "Audit what?\r\n");
    return;
  }
  if (is_abbrev(arg, "armor"))
    do_armor = TRUE;
  else if (is_abbrev(arg, "melee"))
    do_weapons = TRUE;
  else {
    send_to_char(ch, "Usage: audit armor | audit melee\r\n");
    return;
  }

  if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char(ch, "You lack the authority to use this.\r\n");
    return;
  }

  /* --- dynamic buffer builder --- */
  size_t cap = 8192, len = 0;
  char *out = (char *)malloc(cap);
  if (!out) { send_to_char(ch, "Memory error.\r\n"); return; }
  out[0] = '\0';

#define APPEND_FMT(...) do {                                               \
    int need = snprintf(NULL, 0, __VA_ARGS__);                              \
    if (need < 0) need = 0;                                                 \
    if (len + (size_t)need + 1 > cap) {                                     \
      size_t ncap = cap * 2;                                                \
      if (ncap < len + (size_t)need + 1) ncap = len + (size_t)need + 1;     \
      char *tmp = (char *)realloc(out, ncap);                               \
      if (!tmp) { free(out); send_to_char(ch, "Memory error.\r\n"); return; } \
      out = tmp; cap = ncap;                                                \
    }                                                                       \
    len += (size_t)snprintf(out + len, cap - len, __VA_ARGS__);             \
  } while (0)

  if (do_armor) {
    /* Header (short so it won’t wrap) */
    APPEND_FMT("\r\n\tY[Armor Audit]\tn ITEM_ARMOR scan\r\n");
    APPEND_FMT("Legend: \tR!\tn over-cap, \tY?\tn warn, S stealth-disadv\r\n");

    for (obj_rnum r = 0; r <= top_of_objt; r++) {
      struct obj_data *obj = &obj_proto[r];
      char namebuf[128] = {0};
      int idx, vnum, piece_ac, bulk, magic, stealth, strreq;

      if (GET_OBJ_TYPE(obj) != ITEM_ARMOR)
        continue;

      /* Identify slot (skip shields here) */
      idx = armor_slot_index_from_wear(obj);
      if (idx == -2) continue; /* shield handled in AC; skip */
      if (idx < 0)  continue;  /* not a supported armor slot */

      vnum     = GET_OBJ_VNUM(obj);
      piece_ac = GET_OBJ_VAL(obj, VAL_ARMOR_PIECE_AC);
      bulk     = GET_OBJ_VAL(obj, VAL_ARMOR_BULK);
      magic    = GET_OBJ_VAL(obj, VAL_ARMOR_MAGIC_BONUS);
      stealth  = GET_OBJ_VAL(obj, VAL_ARMOR_STEALTH_DISADV); /* 1/0, yes or no */
      strreq   = GET_OBJ_VAL(obj, VAL_ARMOR_STR_REQ);        /* 0 or 13/15/16 typically */

      /* Display name (trim to keep line width < ~78 cols) */
      if (obj->short_description)
        snprintf(namebuf, sizeof(namebuf), "%s", obj->short_description);
      else if (obj->name)
        snprintf(namebuf, sizeof(namebuf), "%s", obj->name);
      else
        snprintf(namebuf, sizeof(namebuf), "object");

      /* Slot caps */
      const int max_piece_ac = armor_slots[idx].max_piece_ac;
      const int max_magic    = armor_slots[idx].max_magic;

      /* Validations */
      bool over_ac    = (piece_ac > max_piece_ac);
      bool over_magic = (magic    > max_magic);
      bool bad_ac     = (piece_ac < 0 || piece_ac > 3);
      bool bad_bulk   = (bulk     < 0 || bulk     > 3);
      bool bad_magic  = (magic    < 0 || magic    > 3);

      found++;

      /* Compact, non-wrapping row (~70 cols worst case) */
      APPEND_FMT("\tc[#%5d]\tn %-24.24s sl=%-5.5s ac=%2d%s b=%d%s m=%+d%s sd=%d str=%d\r\n",
        vnum,
        namebuf,
        slot_name_from_index(idx),
        piece_ac,  over_ac    ? " \tR!\tn" : (bad_ac   ? " \tY?\tn" : ""),
        bulk,      bad_bulk   ? " \tY?\tn" : "",
        magic,     over_magic ? " \tR!\tn" : (bad_magic? " \tY?\tn" : ""),
        stealth, strreq);

      if (over_ac || over_magic || bad_ac || bad_bulk || bad_magic)
        warned++;
    }

    if (!found) {
      free(out);
      send_to_char(ch, "No ITEM_ARMOR prototypes found for the audited slots.\r\n");
      return;
    }

    /* Footer */
    APPEND_FMT("\r\nScanned: %d items, %d with issues. Armor magic cap +%d (shield separate).\r\n",
               found, warned, MAX_TOTAL_ARMOR_MAGIC);
  } else if (do_weapons) {
    APPEND_FMT("\r\n\tY[Weapon Audit]\tn ITEM_WEAPON scan\r\n");
    APPEND_FMT("Legend: \tR!\tn error, \tY?\tn warn, dmg=1d4/1d6/1d8/1d10/1d12\r\n");

    for (obj_rnum r = 0; r <= top_of_objt; r++) {
      struct obj_data *obj = &obj_proto[r];
      char namebuf[128] = {0};
      int vnum, ndice, sdice, atype, weight;
      bool bad_type, bad_dmg, bad_weight;
      const char *atype_name;

      if (GET_OBJ_TYPE(obj) != ITEM_WEAPON)
        continue;

      vnum   = GET_OBJ_VNUM(obj);
      ndice  = GET_OBJ_VAL(obj, 0);
      sdice  = GET_OBJ_VAL(obj, 1);
      atype  = GET_OBJ_VAL(obj, 2);
      weight = GET_OBJ_WEIGHT(obj);

      /* Display name (trim to keep line width < ~78 cols) */
      if (obj->short_description)
        snprintf(namebuf, sizeof(namebuf), "%s", obj->short_description);
      else if (obj->name)
        snprintf(namebuf, sizeof(namebuf), "%s", obj->name);
      else
        snprintf(namebuf, sizeof(namebuf), "object");

      bad_type = (atype < 0 || atype >= NUM_ATTACK_TYPES);
      bad_dmg = !(ndice == 1 &&
                  (sdice == 4 || sdice == 6 || sdice == 8 || sdice == 10 || sdice == 12));
      bad_weight = (weight <= 0);

      if (bad_type)
        atype_name = "invalid";
      else
        atype_name = attack_hit_text[atype].singular;

      found++;

      APPEND_FMT("\tc[#%5d]\tn %-24.24s at=%-7.7s%s dmg=%dd%d%s wt=%d%s\r\n",
        vnum,
        namebuf,
        atype_name, bad_type ? " \tR!\tn" : "",
        ndice, sdice, bad_dmg ? " \tR!\tn" : "",
        weight, bad_weight ? " \tY?\tn" : "");

      if (bad_type || bad_dmg || bad_weight)
        warned++;
    }

    if (!found) {
      free(out);
      send_to_char(ch, "No ITEM_WEAPON prototypes found.\r\n");
      return;
    }

    APPEND_FMT("\r\nScanned: %d items, %d with issues.\r\n", found, warned);
  }

  /* Page it (copy mode) and try to force 25-line pages */
  if (ch->desc) {
    int old_len = 0; bool changed = false;
#if defined(GET_SCREEN_HEIGHT)
    old_len = GET_SCREEN_HEIGHT(ch); GET_SCREEN_HEIGHT(ch) = 25; changed = true;
#elif defined(GET_PAGE_LENGTH)
    old_len = GET_PAGE_LENGTH(ch);   GET_PAGE_LENGTH(ch)   = 25; changed = true;
#endif
    page_string(ch->desc, out, 0); /* copy; we free out */
    free(out);
    if (changed) {
#if defined(GET_SCREEN_HEIGHT)
      GET_SCREEN_HEIGHT(ch) = old_len;
#elif defined(GET_PAGE_LENGTH)
      GET_PAGE_LENGTH(ch)   = old_len;
#endif
    }
  } else {
    send_to_char(ch, "%s", out);
    free(out);
  }

#undef APPEND_FMT
}
