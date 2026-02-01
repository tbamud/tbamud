/**************************************************************************
*  File: players.c                                         Part of tbaMUD *
*  Usage: Player loading/saving and utility routines.                     *
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
#include "db.h"
#include "handler.h"
#include "pfdefaults.h"
#include "py_triggers.h"
#include "comm.h"
#include "interpreter.h"
#include "genolc.h" /* for strip_cr */
#include "config.h" /* for pclean_criteria[] */
#include "py_triggers.h" /* To enable saving of player variables to disk */
#include "quest.h"
#include "toml.h"
#include "toml_utils.h"

#define LOAD_HIT	0
#define LOAD_MANA	1
#define LOAD_STAMINA	2
#define LOAD_STRENGTH	3

#define PT_PNAME(i) (player_table[(i)].name)
#define PT_IDNUM(i) (player_table[(i)].id)
#define PT_LEVEL(i) (player_table[(i)].level)
#define PT_FLAGS(i) (player_table[(i)].flags)
#define PT_LLAST(i) (player_table[(i)].last)

/* local functions */
#if 0
static void load_affects(FILE *fl, struct char_data *ch);
static void load_skills(FILE *fl, struct char_data *ch);
static void load_quests(FILE *fl, struct char_data *ch);
static void load_HMVS(struct char_data *ch, const char *line, int mode);
static void write_aliases_ascii(FILE *file, struct char_data *ch);
static void read_aliases_ascii(FILE *file, struct char_data *ch, int count);
#endif
static int toml_get_int_default(toml_table_t *tab, const char *key, int def);
static long toml_get_long_default(toml_table_t *tab, const char *key, long def);
static char *toml_get_string_dup(toml_table_t *tab, const char *key);
static void toml_read_int_array(toml_array_t *arr, int *out, int out_count, int def);
static void toml_read_long_array(toml_array_t *arr, long *out, int out_count, long def);
static void toml_write_int_array(FILE *fp, const char *key, const int *values, int count);
static void toml_write_long_array(FILE *fp, const char *key, const long *values, int count);

/* New version to build player index for TOML Player Files. Generate index
 * table for the player file. */
void build_player_index(void)
{
  int rec_count = 0, i;
  FILE *plr_index;
  char index_name[40];
  char errbuf[256];
  toml_table_t *tab = NULL;
  toml_array_t *arr = NULL;

  sprintf(index_name, "%s%s", LIB_PLRFILES, INDEX_FILE);
  if (!(plr_index = fopen(index_name, "r"))) {
    top_of_p_table = -1;
    log("No player index file!  First new char will be IMP!");
    return;
  }

  tab = toml_parse_file(plr_index, errbuf, sizeof(errbuf));
  fclose(plr_index);
  if (!tab) {
    top_of_p_table = -1;
    log("SYSERR: Could not parse player index file %s: %s", index_name, errbuf);
    return;
  }

  arr = toml_array_in(tab, "player");
  if (!arr) {
    toml_free(tab);
    player_table = NULL;
    top_of_p_table = -1;
    return;
  }

  rec_count = toml_array_nelem(arr);
  if (rec_count == 0) {
    toml_free(tab);
    player_table = NULL;
    top_of_p_table = -1;
    return;
  }

  CREATE(player_table, struct player_index_element, rec_count);
  for (i = 0; i < rec_count; i++) {
    toml_table_t *entry = toml_table_at(arr, i);
    char *name = NULL;
    int j;

    if (!entry)
      continue;
    player_table[i].id = toml_get_long_default(entry, "id", 0);
    player_table[i].level = toml_get_int_default(entry, "level", 0);
    player_table[i].flags = toml_get_int_default(entry, "flags", 0);
    player_table[i].last = toml_get_long_default(entry, "last", 0);

    name = toml_get_string_dup(entry, "name");
    if (!name)
      name = strdup("");
    CREATE(player_table[i].name, char, strlen(name) + 1);
    for (j = 0; (player_table[i].name[j] = LOWER(name[j])); j++)
      /* Nothing */;
    free(name);
    top_idnum = MAX(top_idnum, player_table[i].id);
  }
  toml_free(tab);
  top_of_p_file = top_of_p_table = i - 1;
}

/* Create a new entry in the in-memory index table for the player file. If the
 * name already exists, by overwriting a deleted character, then we re-use the
 * old position. */
int create_entry(char *name)
{
  int i, pos;

  if (top_of_p_table == -1) {	/* no table */
    pos = top_of_p_table = 0;
    CREATE(player_table, struct player_index_element, 1);
  } else if ((pos = get_ptable_by_name(name)) == -1) {	/* new name */
    i = ++top_of_p_table + 1;

    RECREATE(player_table, struct player_index_element, i);
    pos = top_of_p_table;
  }

  CREATE(player_table[pos].name, char, strlen(name) + 1);

  /* copy lowercase equivalent of name to table field */
  for (i = 0; (player_table[pos].name[i] = LOWER(name[i])); i++)
    /* Nothing */;

  /* clear the bitflag in case we have garbage data */
  player_table[pos].flags = 0;

  return (pos);
}


/* Remove an entry from the in-memory player index table.               *
 * Requires the 'pos' value returned by the get_ptable_by_name function */
static void remove_player_from_index(int pos)
{
  int i;

  if (pos < 0 || pos > top_of_p_table)
    return;

  /* We only need to free the name string */
  free(PT_PNAME(pos));

  /* Move every other item in the list down the index */
  for (i = pos+1; i <= top_of_p_table; i++) {
    PT_PNAME(i-1) = PT_PNAME(i);
    PT_IDNUM(i-1) = PT_IDNUM(i);
    PT_LEVEL(i-1) = PT_LEVEL(i);
    PT_FLAGS(i-1) = PT_FLAGS(i);
    PT_LLAST(i-1) = PT_LLAST(i);
  }
  PT_PNAME(top_of_p_table) = NULL;

  /* Reduce the index table counter */
  top_of_p_table--;

  /* And reduce the size of the table */
  if (top_of_p_table >= 0)
    RECREATE(player_table, struct player_index_element, (top_of_p_table+1));
  else {
    free(player_table);
    player_table = NULL;
  }
}

/* This function necessary to save a seperate TOML player index */
void save_player_index(void)
{
  int i;
  char index_name[50];
  FILE *index_file;

  sprintf(index_name, "%s%s", LIB_PLRFILES, INDEX_FILE);
  if (!(index_file = fopen(index_name, "w"))) {
    log("SYSERR: Could not write player index file");
    return;
  }

  for (i = 0; i <= top_of_p_table; i++)
    if (*player_table[i].name) {
      fprintf(index_file, "[[player]]\n");
      fprintf(index_file, "id = %ld\n", player_table[i].id);
      toml_write_kv_string(index_file, "name", player_table[i].name);
      fprintf(index_file, "level = %d\n", player_table[i].level);
      fprintf(index_file, "flags = %d\n", player_table[i].flags);
      fprintf(index_file, "last = %ld\n\n", (long)player_table[i].last);
    }

  fclose(index_file);
}

void free_player_index(void)
{
  int tp;

  if (!player_table)
    return;

  for (tp = 0; tp <= top_of_p_table; tp++)
    if (player_table[tp].name)
      free(player_table[tp].name);

  free(player_table);
  player_table = NULL;
  top_of_p_table = 0;
}

long get_ptable_by_name(const char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (!str_cmp(player_table[i].name, name))
      return (i);

  return (-1);
}

long get_id_by_name(const char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (!str_cmp(player_table[i].name, name))
      return (player_table[i].id);

  return (-1);
}

char *get_name_by_id(long id)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (player_table[i].id == id)
      return (player_table[i].name);

  return (NULL);
}

static void update_roleplay_age(struct char_data *ch)
{
  if (GET_ROLEPLAY_AGE(ch) == 0)
    GET_ROLEPLAY_AGE(ch) = MIN_CHAR_AGE;

  if (GET_ROLEPLAY_AGE_YEAR(ch) == 0)
    GET_ROLEPLAY_AGE_YEAR(ch) = time_info.year;

  if (time_info.year > GET_ROLEPLAY_AGE_YEAR(ch)) {
    GET_ROLEPLAY_AGE(ch) += (time_info.year - GET_ROLEPLAY_AGE_YEAR(ch));
    GET_ROLEPLAY_AGE_YEAR(ch) = time_info.year;
  }
}

static int toml_get_int_default(toml_table_t *tab, const char *key, int def)
{
  toml_datum_t d = toml_int_in(tab, key);

  if (!d.ok)
    return def;

  return (int)d.u.i;
}

static long toml_get_long_default(toml_table_t *tab, const char *key, long def)
{
  toml_datum_t d = toml_int_in(tab, key);

  if (!d.ok)
    return def;

  return (long)d.u.i;
}

static char *toml_get_string_dup(toml_table_t *tab, const char *key)
{
  toml_datum_t d = toml_string_in(tab, key);

  if (!d.ok)
    return NULL;

  return d.u.s;
}

static void toml_read_int_array(toml_array_t *arr, int *out, int out_count, int def)
{
  int i;

  for (i = 0; i < out_count; i++)
    out[i] = def;

  if (!arr)
    return;

  for (i = 0; i < out_count; i++) {
    toml_datum_t d = toml_int_at(arr, i);
    if (d.ok)
      out[i] = (int)d.u.i;
  }
}

static void toml_read_long_array(toml_array_t *arr, long *out, int out_count, long def)
{
  int i;

  for (i = 0; i < out_count; i++)
    out[i] = def;

  if (!arr)
    return;

  for (i = 0; i < out_count; i++) {
    toml_datum_t d = toml_int_at(arr, i);
    if (d.ok)
      out[i] = (long)d.u.i;
  }
}

static void toml_write_int_array(FILE *fp, const char *key, const int *values, int count)
{
  int i;

  fprintf(fp, "%s = [", key);
  for (i = 0; i < count; i++) {
    if (i > 0)
      fputs(", ", fp);
    fprintf(fp, "%d", values[i]);
  }
  fprintf(fp, "]\n");
}

static void toml_write_long_array(FILE *fp, const char *key, const long *values, int count)
{
  int i;

  fprintf(fp, "%s = [", key);
  for (i = 0; i < count; i++) {
    if (i > 0)
      fputs(", ", fp);
    fprintf(fp, "%ld", values[i]);
  }
  fprintf(fp, "]\n");
}

/* Stuff related to the save/load player system. */
/* New load_char reads TOML Player Files. Load a char, TRUE if loaded, FALSE
 * if not. */
int load_char(const char *name, struct char_data *ch)
{
  int id, i;
  FILE *fl;
  char filename[40];
  char errbuf[256];
  toml_table_t *tab = NULL;
  toml_table_t *sub = NULL;
  toml_array_t *arr = NULL;
  char *str = NULL;
  long long_values[MAX_SKILLS];
  int int_values[6];
  trig_data *t = NULL;
  trig_rnum t_rnum = NOTHING;

  if ((id = get_ptable_by_name(name)) < 0)
    return (-1);
  else {
    if (!get_filename(filename, sizeof(filename), PLR_FILE, player_table[id].name))
      return (-1);
    if (!(fl = fopen(filename, "r"))) {
      mudlog(NRM, LVL_GOD, TRUE, "SYSERR: Couldn't open player file %s", filename);
      return (-1);
    }

    /* Character initializations. Necessary to keep some things straight. */
    ch->affected = NULL;
    ch->player.short_descr = NULL;   /* ensure a clean start */
    for (i = 1; i <= MAX_SKILLS; i++)
      if (IS_NPC(ch))
        ch->mob_specials.skills[i] = 0;
      else
        ch->player_specials->saved.skills[i] = 0;
    GET_SEX(ch) = PFDEF_SEX;
    GET_CLASS(ch) = PFDEF_CLASS;
    GET_SPECIES(ch) = PFDEF_SPECIES;
    GET_LEVEL(ch) = PFDEF_LEVEL;
    GET_HEIGHT(ch) = PFDEF_HEIGHT;
    GET_WEIGHT(ch) = PFDEF_WEIGHT;
    GET_ROLEPLAY_AGE(ch) = 0;
    GET_ROLEPLAY_AGE_YEAR(ch) = 0;
    for (i = 0; i < NUM_OF_SAVING_THROWS; i++)
      GET_SAVE(ch, i) = PFDEF_SAVETHROW;
    GET_LOADROOM(ch) = PFDEF_LOADROOM;
    GET_INVIS_LEV(ch) = PFDEF_INVISLEV;
    GET_FREEZE_LEV(ch) = PFDEF_FREEZELEV;
    GET_WIMP_LEV(ch) = PFDEF_WIMPLEV;
    GET_COND(ch, HUNGER) = PFDEF_HUNGER;
    GET_COND(ch, THIRST) = PFDEF_THIRST;
    GET_COND(ch, DRUNK) = PFDEF_DRUNK;
    GET_BAD_PWS(ch) = PFDEF_BADPWS;
    GET_COINS(ch) = PFDEF_COINS;
    GET_BANK_COINS(ch) = PFDEF_BANK_COINS;
    GET_EXP(ch) = PFDEF_EXP;
    GET_AC(ch) = PFDEF_AC;
    ch->real_abils.str = PFDEF_STR;
    ch->real_abils.dex = PFDEF_DEX;
    ch->real_abils.intel = PFDEF_INT;
    ch->real_abils.wis = PFDEF_WIS;
    ch->real_abils.con = PFDEF_CON;
    ch->real_abils.cha = PFDEF_CHA;
    GET_HIT(ch) = PFDEF_HIT;
    GET_MAX_HIT(ch) = PFDEF_MAXHIT;
    GET_MANA(ch) = PFDEF_MANA;
    GET_MAX_MANA(ch) = PFDEF_MAXMANA;
    GET_STAMINA(ch) = PFDEF_STAMINA;
    GET_MAX_STAMINA(ch) = PFDEF_MAXSTAMINA;
    GET_OLC_ZONE(ch) = PFDEF_OLC;
    GET_PAGE_LENGTH(ch) = PFDEF_PAGELENGTH;
    GET_SCREEN_WIDTH(ch) = PFDEF_SCREENWIDTH;
    GET_ALIASES(ch) = NULL;
    SITTING(ch) = NULL;
    NEXT_SITTING(ch) = NULL;
    GET_QUESTPOINTS(ch) = PFDEF_QUESTPOINTS;
    GET_QUEST_COUNTER(ch) = PFDEF_QUESTCOUNT;
    GET_QUEST(ch) = PFDEF_CURRQUEST;
    GET_NUM_QUESTS(ch) = PFDEF_COMPQUESTS;
    GET_LAST_MOTD(ch) = PFDEF_LASTMOTD;
    GET_LAST_NEWS(ch) = PFDEF_LASTNEWS;
    GET_REROLL_USED(ch) = PFDEF_REROLL_USED;
    GET_REROLL_EXPIRES(ch) = PFDEF_REROLL_EXPIRES;
    memset(&GET_REROLL_OLD_ABILS(ch), 0, sizeof(struct char_ability_data));
    if (GET_ACCOUNT(ch)) {
      free(GET_ACCOUNT(ch));
      GET_ACCOUNT(ch) = NULL;
    }

    for (i = 0; i < AF_ARRAY_MAX; i++)
      AFF_FLAGS(ch)[i] = PFDEF_AFFFLAGS;
    for (i = 0; i < PM_ARRAY_MAX; i++)
      PLR_FLAGS(ch)[i] = PFDEF_PLRFLAGS;
    for (i = 0; i < PR_ARRAY_MAX; i++)
      PRF_FLAGS(ch)[i] = PFDEF_PREFFLAGS;

    tab = toml_parse_file(fl, errbuf, sizeof(errbuf));
    fclose(fl);
    if (!tab) {
      mudlog(NRM, LVL_GOD, TRUE, "SYSERR: Couldn't parse player file %s: %s", filename, errbuf);
      return (-1);
    }

    str = toml_get_string_dup(tab, "name");
    if (str) {
      if (GET_PC_NAME(ch))
        free(GET_PC_NAME(ch));
      GET_PC_NAME(ch) = str;
    } else if (!GET_PC_NAME(ch))
      GET_PC_NAME(ch) = strdup(name);

    str = toml_get_string_dup(tab, "short_desc");
    if (str) {
      if (GET_SHORT_DESC(ch))
        free(GET_SHORT_DESC(ch));
      GET_SHORT_DESC(ch) = str;
    }

    str = toml_get_string_dup(tab, "password");
    if (str) {
      strlcpy(GET_PASSWD(ch), str, MAX_PWD_LENGTH + 1);
      free(str);
    }

    str = toml_get_string_dup(tab, "account");
    if (str) {
      if (GET_ACCOUNT(ch))
        free(GET_ACCOUNT(ch));
      GET_ACCOUNT(ch) = str;
    }

    str = toml_get_string_dup(tab, "description");
    if (str) {
      if (ch->player.description)
        free(ch->player.description);
      ch->player.description = str;
    }

    str = toml_get_string_dup(tab, "background");
    if (str) {
      if (ch->player.background)
        free(ch->player.background);
      ch->player.background = str;
    }

    str = toml_get_string_dup(tab, "poofin");
    if (str) {
      if (POOFIN(ch))
        free(POOFIN(ch));
      POOFIN(ch) = str;
    }

    str = toml_get_string_dup(tab, "poofout");
    if (str) {
      if (POOFOUT(ch))
        free(POOFOUT(ch));
      POOFOUT(ch) = str;
    }

    GET_SEX(ch) = toml_get_int_default(tab, "sex", GET_SEX(ch));
    GET_CLASS(ch) = toml_get_int_default(tab, "class", GET_CLASS(ch));
    GET_SPECIES(ch) = toml_get_int_default(tab, "species", GET_SPECIES(ch));
    GET_LEVEL(ch) = toml_get_int_default(tab, "level", GET_LEVEL(ch));
    GET_IDNUM(ch) = toml_get_long_default(tab, "id", GET_IDNUM(ch));
    ch->player.time.birth = toml_get_long_default(tab, "birth", ch->player.time.birth);
    GET_ROLEPLAY_AGE(ch) = toml_get_int_default(tab, "age", GET_ROLEPLAY_AGE(ch));
    GET_ROLEPLAY_AGE_YEAR(ch) = toml_get_int_default(tab, "age_year", GET_ROLEPLAY_AGE_YEAR(ch));
    ch->player.time.played = toml_get_int_default(tab, "played", ch->player.time.played);
    ch->player.time.logon = toml_get_long_default(tab, "logon", ch->player.time.logon);
    GET_LAST_MOTD(ch) = toml_get_int_default(tab, "last_motd", GET_LAST_MOTD(ch));
    GET_LAST_NEWS(ch) = toml_get_int_default(tab, "last_news", GET_LAST_NEWS(ch));
    GET_REROLL_USED(ch) = toml_get_int_default(tab, "reroll_used", GET_REROLL_USED(ch));
    GET_REROLL_EXPIRES(ch) = (time_t)toml_get_long_default(tab, "reroll_expires", (long)GET_REROLL_EXPIRES(ch));

    arr = toml_array_in(tab, "reroll_old_abils");
    if (arr) {
      toml_read_int_array(arr, int_values, 6, 0);
      GET_REROLL_OLD_ABILS(ch).str = int_values[0];
      GET_REROLL_OLD_ABILS(ch).intel = int_values[1];
      GET_REROLL_OLD_ABILS(ch).wis = int_values[2];
      GET_REROLL_OLD_ABILS(ch).dex = int_values[3];
      GET_REROLL_OLD_ABILS(ch).con = int_values[4];
      GET_REROLL_OLD_ABILS(ch).cha = int_values[5];
    }

    str = toml_get_string_dup(tab, "host");
    if (str) {
      if (GET_HOST(ch))
        free(GET_HOST(ch));
      GET_HOST(ch) = str;
    }

    GET_HEIGHT(ch) = toml_get_int_default(tab, "height", GET_HEIGHT(ch));
    GET_WEIGHT(ch) = toml_get_int_default(tab, "weight", GET_WEIGHT(ch));

    arr = toml_array_in(tab, "act_flags");
    toml_read_int_array(arr, int_values, PM_ARRAY_MAX, PFDEF_PLRFLAGS);
    for (i = 0; i < PM_ARRAY_MAX; i++)
      PLR_FLAGS(ch)[i] = int_values[i];

    arr = toml_array_in(tab, "aff_flags");
    toml_read_int_array(arr, int_values, AF_ARRAY_MAX, PFDEF_AFFFLAGS);
    for (i = 0; i < AF_ARRAY_MAX; i++)
      AFF_FLAGS(ch)[i] = int_values[i];

    arr = toml_array_in(tab, "pref_flags");
    toml_read_int_array(arr, int_values, PR_ARRAY_MAX, PFDEF_PREFFLAGS);
    for (i = 0; i < PR_ARRAY_MAX; i++)
      PRF_FLAGS(ch)[i] = int_values[i];

    arr = toml_array_in(tab, "saving_throws");
    toml_read_int_array(arr, int_values, NUM_OF_SAVING_THROWS, PFDEF_SAVETHROW);
    for (i = 0; i < NUM_OF_SAVING_THROWS; i++)
      GET_SAVE(ch, i) = int_values[i];

    GET_WIMP_LEV(ch) = toml_get_int_default(tab, "wimp", GET_WIMP_LEV(ch));
    GET_FREEZE_LEV(ch) = toml_get_int_default(tab, "freeze", GET_FREEZE_LEV(ch));
    GET_INVIS_LEV(ch) = toml_get_int_default(tab, "invis", GET_INVIS_LEV(ch));
    GET_LOADROOM(ch) = toml_get_int_default(tab, "load_room", GET_LOADROOM(ch));
    GET_BAD_PWS(ch) = toml_get_int_default(tab, "bad_passwords", GET_BAD_PWS(ch));

    sub = toml_table_in(tab, "conditions");
    if (sub) {
      GET_COND(ch, HUNGER) = toml_get_int_default(sub, "hunger", GET_COND(ch, HUNGER));
      GET_COND(ch, THIRST) = toml_get_int_default(sub, "thirst", GET_COND(ch, THIRST));
      GET_COND(ch, DRUNK) = toml_get_int_default(sub, "drunk", GET_COND(ch, DRUNK));
    }

    sub = toml_table_in(tab, "hmv");
    if (sub) {
      GET_HIT(ch) = toml_get_int_default(sub, "hit", GET_HIT(ch));
      GET_MAX_HIT(ch) = toml_get_int_default(sub, "max_hit", GET_MAX_HIT(ch));
      GET_MANA(ch) = toml_get_int_default(sub, "mana", GET_MANA(ch));
      GET_MAX_MANA(ch) = toml_get_int_default(sub, "max_mana", GET_MAX_MANA(ch));
      GET_STAMINA(ch) = toml_get_int_default(sub, "stamina", GET_STAMINA(ch));
      GET_MAX_STAMINA(ch) = toml_get_int_default(sub, "max_stamina", GET_MAX_STAMINA(ch));
    }

    sub = toml_table_in(tab, "abilities");
    if (sub) {
      ch->real_abils.str = toml_get_int_default(sub, "str", ch->real_abils.str);
      ch->real_abils.intel = toml_get_int_default(sub, "int", ch->real_abils.intel);
      ch->real_abils.wis = toml_get_int_default(sub, "wis", ch->real_abils.wis);
      ch->real_abils.dex = toml_get_int_default(sub, "dex", ch->real_abils.dex);
      ch->real_abils.con = toml_get_int_default(sub, "con", ch->real_abils.con);
      ch->real_abils.cha = toml_get_int_default(sub, "cha", ch->real_abils.cha);
    }

    GET_AC(ch) = toml_get_int_default(tab, "ac", GET_AC(ch));
    GET_COINS(ch) = toml_get_int_default(tab, "coins", GET_COINS(ch));
    GET_BANK_COINS(ch) = toml_get_int_default(tab, "bank_coins", GET_BANK_COINS(ch));
    GET_EXP(ch) = toml_get_int_default(tab, "exp", GET_EXP(ch));
    GET_OLC_ZONE(ch) = toml_get_int_default(tab, "olc_zone", GET_OLC_ZONE(ch));
    GET_PAGE_LENGTH(ch) = toml_get_int_default(tab, "page_length", GET_PAGE_LENGTH(ch));
    GET_SCREEN_WIDTH(ch) = toml_get_int_default(tab, "screen_width", GET_SCREEN_WIDTH(ch));
    GET_QUESTPOINTS(ch) = toml_get_int_default(tab, "quest_points", GET_QUESTPOINTS(ch));
    GET_QUEST_COUNTER(ch) = toml_get_int_default(tab, "quest_counter", GET_QUEST_COUNTER(ch));
    GET_QUEST(ch) = toml_get_int_default(tab, "current_quest", GET_QUEST(ch));

    arr = toml_array_in(tab, "completed_quests");
    if (arr) {
      int count = toml_array_nelem(arr);
      for (i = 0; i < count; i++) {
        toml_datum_t d = toml_int_at(arr, i);
        if (d.ok)
          add_completed_quest(ch, (int)d.u.i);
      }
    }

    arr = toml_array_in(tab, "triggers");
    if (arr && CONFIG_SCRIPT_PLAYERS) {
      int count = toml_array_nelem(arr);
      for (i = 0; i < count; i++) {
        toml_datum_t d = toml_int_at(arr, i);
        if (!d.ok)
          continue;
        t_rnum = real_trigger((int)d.u.i);
        if (t_rnum == NOTHING)
          continue;
        t = read_trigger(t_rnum);
        if (!SCRIPT(ch))
          CREATE(SCRIPT(ch), struct script_data, 1);
        add_trigger(SCRIPT(ch), t, -1);
      }
    }

    arr = toml_array_in(tab, "skill_gain_next");
    toml_read_long_array(arr, long_values, MAX_SKILLS, 0);
    for (i = 0; i < MAX_SKILLS; i++)
      GET_SKILL_NEXT_GAIN(ch, i + 1) = (time_t)long_values[i];

    arr = toml_array_in(tab, "skill");
    if (arr) {
      int count = toml_array_nelem(arr);
      for (i = 0; i < count; i++) {
        toml_table_t *skill = toml_table_at(arr, i);
        int skill_id, skill_level;

        if (!skill)
          continue;
        skill_id = toml_get_int_default(skill, "id", 0);
        skill_level = toml_get_int_default(skill, "level", 0);
        if (skill_id < 1 || skill_id > MAX_SKILLS)
          continue;
        if (IS_NPC(ch))
          ch->mob_specials.skills[skill_id] = skill_level;
        else
          ch->player_specials->saved.skills[skill_id] = skill_level;
      }
    }

    arr = toml_array_in(tab, "affect");
    if (arr) {
      int count = toml_array_nelem(arr);
      for (i = 0; i < count; i++) {
        struct affected_type af;
        toml_table_t *aff_tab = toml_table_at(arr, i);

        if (!aff_tab)
          continue;

        new_affect(&af);
        af.spell = toml_get_int_default(aff_tab, "spell", 0);
        af.duration = toml_get_int_default(aff_tab, "duration", 0);
        af.modifier = toml_get_int_default(aff_tab, "modifier", 0);
        af.location = toml_get_int_default(aff_tab, "location", 0);
        toml_read_int_array(toml_array_in(aff_tab, "bitvector"), int_values, AF_ARRAY_MAX, 0);
        af.bitvector[0] = int_values[0];
        af.bitvector[1] = int_values[1];
        af.bitvector[2] = int_values[2];
        af.bitvector[3] = int_values[3];
        if (af.spell > 0)
          affect_to_char(ch, &af);
      }
    }

    arr = toml_array_in(tab, "alias");
    if (arr) {
      int count = toml_array_nelem(arr);
      for (i = 0; i < count; i++) {
        toml_table_t *alias_tab = toml_table_at(arr, i);
        char *alias = NULL;
        char *replacement = NULL;
        int type;
        struct alias_data *temp;

        if (!alias_tab)
          continue;
        alias = toml_get_string_dup(alias_tab, "alias");
        replacement = toml_get_string_dup(alias_tab, "replacement");
        type = toml_get_int_default(alias_tab, "type", 0);
        if (!alias || !replacement) {
          if (alias)
            free(alias);
          if (replacement)
            free(replacement);
          continue;
        }

        CREATE(temp, struct alias_data, 1);
        temp->alias = alias;
        temp->replacement = replacement;
        temp->type = type;
        temp->next = GET_ALIASES(ch);
        GET_ALIASES(ch) = temp;
      }
    }

    arr = toml_array_in(tab, "var");
    if (arr) {
      int count = toml_array_nelem(arr);
      for (i = 0; i < count; i++) {
        toml_table_t *var_tab = toml_table_at(arr, i);
        char *varname = NULL;
        char *value = NULL;
        long context;

        if (!var_tab)
          continue;
        varname = toml_get_string_dup(var_tab, "name");
        value = toml_get_string_dup(var_tab, "value");
        context = toml_get_long_default(var_tab, "context", 0);
        if (!varname || !value) {
          if (varname)
            free(varname);
          if (value)
            free(value);
          continue;
        }
        if (!SCRIPT(ch))
          CREATE(SCRIPT(ch), struct script_data, 1);
        add_var(&(SCRIPT(ch)->global_vars), varname, value, context);
        free(varname);
        free(value);
      }
    }

    toml_free(tab);
  }

  update_roleplay_age(ch);
  ch->player.time.birth = time(0) - get_total_played_seconds(ch);
  affect_total(ch);
  MOUNT(ch) = NULL;
  RIDDEN_BY(ch) = NULL;
  HITCHED_TO(ch) = NULL;
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_MOUNTED);

  /* initialization for imms */
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    for (i = 1; i <= MAX_SKILLS; i++)
      if (IS_NPC(ch))
        ch->mob_specials.skills[i] = 100;
      else
        ch->player_specials->saved.skills[i] = 100;
    GET_COND(ch, HUNGER) = -1;
    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, DRUNK) = -1;
  }
  return(id);
}

/* Write the vital data of a player to the player file. */
/* This is the TOML Player Files save routine. */
void save_char(struct char_data * ch)
{
  FILE *fl;
  char filename[40], buf[MAX_STRING_LENGTH];
  int i, j, id, save_index = FALSE;
  int save_throws[NUM_OF_SAVING_THROWS];
  long gain_times[MAX_SKILLS];
  struct affected_type *aff, tmp_aff[MAX_AFFECT];
  struct obj_data *char_eq[NUM_WEARS];
  trig_data *t;

  if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
    return;

  /* If ch->desc is not null, then update session data before saving. */
  if (ch->desc) {
    if (*ch->desc->host) {
      if (!GET_HOST(ch))
        GET_HOST(ch) = strdup(ch->desc->host);
      else if (GET_HOST(ch) && strcmp(GET_HOST(ch), ch->desc->host)) {
        free(GET_HOST(ch));
        GET_HOST(ch) = strdup(ch->desc->host);
      }
    }

    /* Only update the time.played and time.logon if the character is playing. */
    if (STATE(ch->desc) == CON_PLAYING) {
      ch->player.time.played += time(0) - ch->player.time.logon;
      ch->player.time.logon = time(0);
    }
  }

  update_roleplay_age(ch);
  ch->player.time.birth = time(0) - get_total_played_seconds(ch);

  if (!get_filename(filename, sizeof(filename), PLR_FILE, GET_NAME(ch)))
    return;
  if (!(fl = fopen(filename, "w"))) {
    mudlog(NRM, LVL_GOD, TRUE, "SYSERR: Couldn't open player file %s for write", filename);
    return;
  }

  /* Unaffect everything a character can be affected by. */
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      char_eq[i] = unequip_char(ch, i);
#ifndef NO_EXTRANEOUS_TRIGGERS
      remove_otrigger(char_eq[i], ch);
#endif
    }
    else
      char_eq[i] = NULL;
  }

  for (aff = ch->affected, i = 0; i < MAX_AFFECT; i++) {
    if (aff) {
      tmp_aff[i] = *aff;
      for (j=0; j<AF_ARRAY_MAX; j++)
        tmp_aff[i].bitvector[j] = aff->bitvector[j];
      tmp_aff[i].next = 0;
      aff = aff->next;
    } else {
      new_affect(&(tmp_aff[i]));
      tmp_aff[i].next = 0;
    }
  }

  /* Remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in. */

  while (ch->affected)
    affect_remove(ch, ch->affected);

  if ((i >= MAX_AFFECT) && aff && aff->next)
    log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

  ch->aff_abils = ch->real_abils;
  /* end char_to_store code */

  if (GET_NAME(ch))
    toml_write_kv_string(fl, "name", GET_NAME(ch));
  toml_write_kv_string_opt(fl, "short_desc", GET_SHORT_DESC(ch));
  if (GET_PASSWD(ch) && *GET_PASSWD(ch))
    toml_write_kv_string(fl, "password", GET_PASSWD(ch));
  toml_write_kv_string_opt(fl, "account", GET_ACCOUNT(ch));
  if (ch->player.description && *ch->player.description) {
    strlcpy(buf, ch->player.description, sizeof(buf));
    strip_cr(buf);
    toml_write_kv_string(fl, "description", buf);
  }
  if (ch->player.background && *ch->player.background) {
    strlcpy(buf, ch->player.background, sizeof(buf));
    strip_cr(buf);
    toml_write_kv_string(fl, "background", buf);
  }
  toml_write_kv_string_opt(fl, "poofin", POOFIN(ch));
  toml_write_kv_string_opt(fl, "poofout", POOFOUT(ch));

  fprintf(fl, "sex = %d\n", GET_SEX(ch));
  fprintf(fl, "class = %d\n", GET_CLASS(ch));
  fprintf(fl, "species = %d\n", GET_SPECIES(ch));
  fprintf(fl, "level = %d\n", GET_LEVEL(ch));

  fprintf(fl, "id = %ld\n", GET_IDNUM(ch));
  fprintf(fl, "birth = %ld\n", (long)ch->player.time.birth);
  fprintf(fl, "age = %d\n", GET_ROLEPLAY_AGE(ch));
  fprintf(fl, "age_year = %d\n", GET_ROLEPLAY_AGE_YEAR(ch));
  fprintf(fl, "played = %d\n", ch->player.time.played);
  fprintf(fl, "logon = %ld\n", (long)ch->player.time.logon);
  fprintf(fl, "last_motd = %d\n", (int)GET_LAST_MOTD(ch));
  fprintf(fl, "last_news = %d\n", (int)GET_LAST_NEWS(ch));
  fprintf(fl, "reroll_used = %d\n", (int)GET_REROLL_USED(ch));
  fprintf(fl, "reroll_expires = %ld\n", (long)GET_REROLL_EXPIRES(ch));
  fprintf(fl, "reroll_old_abils = [%d, %d, %d, %d, %d, %d]\n",
          GET_REROLL_OLD_ABILS(ch).str,
          GET_REROLL_OLD_ABILS(ch).intel,
          GET_REROLL_OLD_ABILS(ch).wis,
          GET_REROLL_OLD_ABILS(ch).dex,
          GET_REROLL_OLD_ABILS(ch).con,
          GET_REROLL_OLD_ABILS(ch).cha);

  toml_write_kv_string_opt(fl, "host", GET_HOST(ch));
  fprintf(fl, "height = %d\n", GET_HEIGHT(ch));
  fprintf(fl, "weight = %d\n", GET_WEIGHT(ch));

  toml_write_int_array(fl, "act_flags", PLR_FLAGS(ch), PM_ARRAY_MAX);
  toml_write_int_array(fl, "aff_flags", AFF_FLAGS(ch), AF_ARRAY_MAX);
  toml_write_int_array(fl, "pref_flags", PRF_FLAGS(ch), PR_ARRAY_MAX);

  for (i = 0; i < NUM_OF_SAVING_THROWS; i++)
    save_throws[i] = GET_SAVE(ch, i);
  toml_write_int_array(fl, "saving_throws", save_throws, NUM_OF_SAVING_THROWS);

  fprintf(fl, "wimp = %d\n", GET_WIMP_LEV(ch));
  fprintf(fl, "freeze = %d\n", GET_FREEZE_LEV(ch));
  fprintf(fl, "invis = %d\n", GET_INVIS_LEV(ch));
  fprintf(fl, "load_room = %d\n", GET_LOADROOM(ch));
  fprintf(fl, "bad_passwords = %d\n", GET_BAD_PWS(ch));
  fprintf(fl, "conditions = { hunger = %d, thirst = %d, drunk = %d }\n",
          GET_COND(ch, HUNGER), GET_COND(ch, THIRST), GET_COND(ch, DRUNK));
  fprintf(fl, "hmv = { hit = %d, max_hit = %d, mana = %d, max_mana = %d, stamina = %d, max_stamina = %d }\n",
          GET_HIT(ch), GET_MAX_HIT(ch), GET_MANA(ch), GET_MAX_MANA(ch),
          GET_STAMINA(ch), GET_MAX_STAMINA(ch));
  fprintf(fl, "abilities = { str = %d, int = %d, wis = %d, dex = %d, con = %d, cha = %d }\n",
          GET_STR(ch), GET_INT(ch), GET_WIS(ch), GET_DEX(ch), GET_CON(ch), GET_CHA(ch));

  fprintf(fl, "ac = %d\n", GET_AC(ch));
  fprintf(fl, "coins = %d\n", GET_COINS(ch));
  fprintf(fl, "bank_coins = %d\n", GET_BANK_COINS(ch));
  fprintf(fl, "exp = %d\n", GET_EXP(ch));
  fprintf(fl, "olc_zone = %d\n", GET_OLC_ZONE(ch));
  fprintf(fl, "page_length = %d\n", GET_PAGE_LENGTH(ch));
  fprintf(fl, "screen_width = %d\n", GET_SCREEN_WIDTH(ch));
  fprintf(fl, "quest_points = %d\n", GET_QUESTPOINTS(ch));
  fprintf(fl, "quest_counter = %d\n", GET_QUEST_COUNTER(ch));
  fprintf(fl, "current_quest = %d\n", GET_QUEST(ch));

  if (GET_NUM_QUESTS(ch) > 0) {
    fprintf(fl, "completed_quests = [");
    for (i = 0; i < GET_NUM_QUESTS(ch); i++) {
      if (i > 0)
        fputs(", ", fl);
      fprintf(fl, "%d", ch->player_specials->saved.completed_quests[i]);
    }
    fprintf(fl, "]\n");
  }

  if (SCRIPT(ch)) {
    int trig_count = 0;
    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
      trig_count++;
    if (trig_count > 0) {
      fprintf(fl, "triggers = [");
      for (t = TRIGGERS(SCRIPT(ch)), i = 0; t; t = t->next, i++) {
        if (i > 0)
          fputs(", ", fl);
        fprintf(fl, "%d", GET_TRIG_VNUM(t));
      }
      fprintf(fl, "]\n");
    }
  }

  for (i = 1; i <= MAX_SKILLS; i++)
    gain_times[i - 1] = (long)GET_SKILL_NEXT_GAIN(ch, i);
  toml_write_long_array(fl, "skill_gain_next", gain_times, MAX_SKILLS);

  if (GET_LEVEL(ch) < LVL_IMMORT) {
    for (i = 1; i <= MAX_SKILLS; i++) {
      if (!GET_SKILL(ch, i))
        continue;
      fprintf(fl, "\n[[skill]]\n");
      fprintf(fl, "id = %d\n", i);
      fprintf(fl, "level = %d\n", GET_SKILL(ch, i));
    }
  }

  if (tmp_aff[0].spell > 0) {
    for (i = 0; i < MAX_AFFECT; i++) {
      aff = &tmp_aff[i];
      if (!aff->spell)
        continue;
      fprintf(fl, "\n[[affect]]\n");
      fprintf(fl, "spell = %d\n", aff->spell);
      fprintf(fl, "duration = %d\n", aff->duration);
      fprintf(fl, "modifier = %d\n", aff->modifier);
      fprintf(fl, "location = %d\n", aff->location);
      fprintf(fl, "bitvector = [%d, %d, %d, %d]\n",
              aff->bitvector[0], aff->bitvector[1], aff->bitvector[2], aff->bitvector[3]);
    }
  }

  for (struct alias_data *temp = GET_ALIASES(ch); temp; temp = temp->next) {
    fprintf(fl, "\n[[alias]]\n");
    toml_write_kv_string(fl, "alias", temp->alias);
    toml_write_kv_string(fl, "replacement", temp->replacement);
    fprintf(fl, "type = %d\n", temp->type);
  }

  if (SCRIPT(ch) && ch->script->global_vars) {
    struct trig_var_data *vars;

    for (vars = ch->script->global_vars; vars; vars = vars->next) {
      if (*vars->name == '-')
        continue;
      fprintf(fl, "\n[[var]]\n");
      toml_write_kv_string(fl, "name", vars->name);
      fprintf(fl, "context = %ld\n", vars->context);
      toml_write_kv_string(fl, "value", vars->value);
    }
  }

  fclose(fl);

  /* More char_to_store code to add spell and eq affections back in. */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (tmp_aff[i].spell)
      affect_to_char(ch, &tmp_aff[i]);
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_eq[i])
#ifndef NO_EXTRANEOUS_TRIGGERS
        if (wear_otrigger(char_eq[i], ch, i))
#endif
    equip_char(ch, char_eq[i], i);
#ifndef NO_EXTRANEOUS_TRIGGERS
          else
          obj_to_char(char_eq[i], ch);
#endif
  }
  /* end char_to_store code */

  if ((id = get_ptable_by_name(GET_NAME(ch))) < 0)
    return;

  /* update the player in the player index */
  if (player_table[id].level != GET_LEVEL(ch)) {
    save_index = TRUE;
    player_table[id].level = GET_LEVEL(ch);
  }
  if (player_table[id].last != ch->player.time.logon) {
    save_index = TRUE;
    player_table[id].last = ch->player.time.logon;
  }
  i = player_table[id].flags;
  if (PLR_FLAGGED(ch, PLR_DELETED))
    SET_BIT(player_table[id].flags, PINDEX_DELETED);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_DELETED);
  if (PLR_FLAGGED(ch, PLR_NODELETE) || PLR_FLAGGED(ch, PLR_CRYO))
    SET_BIT(player_table[id].flags, PINDEX_NODELETE);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_NODELETE);

  if (PLR_FLAGGED(ch, PLR_FROZEN) || PLR_FLAGGED(ch, PLR_NOWIZLIST))
    SET_BIT(player_table[id].flags, PINDEX_NOWIZLIST);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_NOWIZLIST);

  if (player_table[id].flags != i || save_index)
    save_player_index();
}

/* Separate a 4-character id tag from the data it precedes */
void tag_argument(char *argument, char *tag)
{
  char *tmp = argument, *ttag = tag, *wrt = argument;
  int i;

  for (i = 0; i < 4; i++)
    *(ttag++) = *(tmp++);
  *ttag = '\0';

  while (*tmp == ':' || *tmp == ' ')
    tmp++;

  while (*tmp)
    *(wrt++) = *(tmp++);
  *wrt = '\0';
}

/* Stuff related to the player file cleanup system. */

/* remove_player() removes all files associated with a player who is self-deleted,
 * deleted by an immortal, or deleted by the auto-wipe system (if enabled). */
void remove_player(int pfilepos)
{
  char filename[MAX_STRING_LENGTH], timestr[25];
  int i;

  if (!*player_table[pfilepos].name)
    return;

  /* Unlink all player-owned files */
  for (i = 0; i < MAX_FILES; i++) {
    if (get_filename(filename, sizeof(filename), i, player_table[pfilepos].name))
      unlink(filename);
  }

  strftime(timestr, sizeof(timestr), "%c", localtime(&(player_table[pfilepos].last)));
  log("PCLEAN: %s Lev: %d Last: %s",
	player_table[pfilepos].name, player_table[pfilepos].level,
	timestr);
  player_table[pfilepos].name[0] = '\0';

  /* Update index table. */
  remove_player_from_index(pfilepos);

  save_player_index();
}

void clean_pfiles(void)
{
  int i, ci;

  for (i = 0; i <= top_of_p_table; i++) {
    /* We only want to go further if the player isn't protected from deletion
     * and hasn't already been deleted. */
    if (!IS_SET(player_table[i].flags, PINDEX_NODELETE) &&
        *player_table[i].name) {
      /* If the player is already flagged for deletion, then go ahead and get
       * rid of him. */
      if (IS_SET(player_table[i].flags, PINDEX_DELETED)) {
	remove_player(i);
      } else {
        /* Check to see if the player has overstayed his welcome based on level. */
	for (ci = 0; pclean_criteria[ci].level > -1; ci++) {
	  if (player_table[i].level <= pclean_criteria[ci].level &&
	      ((time(0) - player_table[i].last) >
	       (pclean_criteria[ci].days * SECS_PER_REAL_DAY))) {
	    remove_player(i);
	    break;
	  }
	}
        /* If we got this far and the players hasn't been kicked out, then he
	 * can stay a little while longer. */
      }
    }
  }
  /* After everything is done, we should rebuild player_index and remove the
   * entries of the players that were just deleted. */
}

/* load_affects function now handles both 32-bit and
   128-bit affect bitvectors for backward compatibility */
#if 0
static void load_affects(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0, num6 = 0, num7 = 0, num8 = 0, i, n_vars;
  char line[MAX_INPUT_LENGTH + 1];
  struct affected_type af;

  i = 0;
  do {
    new_affect(&af);
    get_line(fl, line);
    n_vars = sscanf(line, "%d %d %d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5, &num6, &num7, &num8);
    if (num > 0) {
      af.spell = num;
      af.duration = num2;
      af.modifier = num3;
      af.location = num4;
      if (n_vars == 8) {              /* New 128-bit version */
          af.bitvector[0] =  num5;
          af.bitvector[1] =  num6;
          af.bitvector[2] =  num7;
          af.bitvector[3] =  num8;
      } else if (n_vars == 5) {       /* Old 32-bit conversion version */
        if (num5 > 0 && num5 < NUM_AFF_FLAGS)  /* Ignore invalid values */
          SET_BIT_AR(af.bitvector, num5);
      } else {
        log("SYSERR: Invalid affects in pfile (%s), expecting 5 or 8 values", GET_NAME(ch));
      }
      affect_to_char(ch, &af);
      i++;
    }
  } while (num != 0);
}

static void load_skills(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);

    if (num != 0) {
      if (IS_NPC(ch))
        ch->mob_specials.skills[num] = num2;
      else
        ch->player_specials->saved.skills[num] = num2;
    }
  } while (num != 0);
}

void load_quests(FILE *fl, struct char_data *ch)
{
  int num = NOTHING;
  char line[MAX_INPUT_LENGTH + 1];

  do {
    get_line(fl, line);
    sscanf(line, "%d", &num);
    if (num != NOTHING)
      add_completed_quest(ch, num);
  } while (num != NOTHING);
}

static void load_HMVS(struct char_data *ch, const char *line, int mode)
{
  int num = 0, num2 = 0;

  sscanf(line, "%d/%d", &num, &num2);

  switch (mode) {
  case LOAD_HIT:
    GET_HIT(ch) = num;
    GET_MAX_HIT(ch) = num2;
    break;

  case LOAD_MANA:
    GET_MANA(ch) = num;
    GET_MAX_MANA(ch) = num2;
    break;

  case LOAD_STAMINA:
    GET_STAMINA(ch) = num;
    GET_MAX_STAMINA(ch) = num2;
    break;

  case LOAD_STRENGTH:
    ch->real_abils.str = num;
    break;
  }
}

static void write_aliases_ascii(FILE *file, struct char_data *ch)
{
  struct alias_data *temp;
  int count = 0;

  if (GET_ALIASES(ch) == NULL)
    return;

  for (temp = GET_ALIASES(ch); temp; temp = temp->next)
    count++;

  fprintf(file, "Alis: %d\n", count);

  for (temp = GET_ALIASES(ch); temp; temp = temp->next)
    fprintf(file, " %s\n"   /* Alias: prepend a space in order to avoid issues with aliases beginning
                             * with * (get_line treats lines beginning with * as comments and ignores them */
                  "%s\n"    /* Replacement: always prepended with a space in memory anyway */
                  "%d\n",   /* Type */
                  temp->alias,
                  temp->replacement,
                  temp->type);
}

static void read_aliases_ascii(FILE *file, struct char_data *ch, int count)
{
  int i;

  if (count == 0) {
    GET_ALIASES(ch) = NULL;
    return; /* No aliases in the list. */
  }

  /* This code goes both ways for the old format (where alias and replacement start at the
   * first character on the line) and the new (where they are prepended by a space in order
   * to avoid the possibility of a * at the start of the line */
  for (i = 0; i < count; i++) {
    char abuf[MAX_INPUT_LENGTH+1], rbuf[MAX_INPUT_LENGTH+1], tbuf[MAX_INPUT_LENGTH];

    /* Read the aliased command. */
    get_line(file, abuf);

    /* Read the replacement. This needs to have a space prepended before placing in
     * the in-memory struct. The space may be there already, but we can't be certain! */
    rbuf[0] = ' ';
    get_line(file, rbuf+1);

    /* read the type */
    get_line(file, tbuf);

    if (abuf[0] && rbuf[1] && *tbuf) {
      struct alias_data *temp;
      CREATE(temp, struct alias_data, 1);
      temp->alias       = strdup(abuf[0] == ' ' ? abuf+1 : abuf);
      temp->replacement = strdup(rbuf[1] == ' ' ? rbuf+1 : rbuf);
      temp->type        = atoi(tbuf);
      temp->next        = GET_ALIASES(ch);
      GET_ALIASES(ch)   = temp;
    }
  }
}
#endif
