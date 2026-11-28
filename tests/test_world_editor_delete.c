/* Regression coverage for world editor (redit, oedit, medit) delete targeting,
 * unsaved/failed delete guards, concurrent medit rnum updates / free_mobile()
 * bounds checking, OLC_SCRIPT cleanup on delete, and medit W) Copy + save. */
#include "conf.h"
#include "sysdep.h"
#include "unity.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "shop.h"
#include "boards.h"
#include "genolc.h"
#include "genmob.h"
#include "genobj.h"
#include "genwld.h"
#include "genzon.h"
#include "oasis.h"
#include "dg_scripts.h"
#include "dg_olc.h"
#include "constants.h"
#include "fight.h"
#include "spells.h"
#include "modify.h"
#include "improved-edit.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Globals not defined in test_stubs.c */
struct zone_data *zone_table = NULL;
zone_rnum top_of_zone_table = 0;
struct char_data *mob_proto = NULL;
struct shop_data *shop_index = NULL;
int top_shop = -1;
struct board_info_type board_info[NUM_OF_BOARDS];
struct index_data **trig_index = NULL;
int top_of_trigt = 0;
struct trig_data *trigger_list = NULL;
struct attack_hit_type attack_hit_text[] = {
  { "hit", "hits" },
  { "sting", "stings" }
};
struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];

/* Output capture for write_to_output() */
static char out_buf[8192];
static size_t out_len = 0;

static void clear_output(void)
{
  out_buf[0] = '\0';
  out_len = 0;
}

size_t write_to_output(struct descriptor_data *d, const char *txt, ...)
{
  va_list args;
  int n;
  (void)d;
  if (!txt || out_len + 1 >= sizeof(out_buf))
    return 0;
  va_start(args, txt);
  n = vsnprintf(out_buf + out_len, sizeof(out_buf) - out_len, txt, args);
  va_end(args);
  if (n > 0) {
    size_t added = (size_t)n;
    if (out_len + added >= sizeof(out_buf))
      out_len = sizeof(out_buf) - 1;
    else
      out_len += added;
  }
  return n > 0 ? (size_t)n : 0;
}

/* Real lookup helpers over the test tables */
room_rnum real_room(room_vnum vnum)
{
  room_rnum i;
  if (!world)
    return NOWHERE;
  for (i = 0; i <= top_of_world; i++)
    if (world[i].number == vnum)
      return i;
  return NOWHERE;
}

obj_rnum real_object(obj_vnum vnum)
{
  obj_rnum i;
  if (!obj_index || top_of_objt == NOTHING)
    return NOTHING;
  for (i = 0; i <= top_of_objt; i++)
    if (obj_index[i].vnum == vnum)
      return i;
  return NOTHING;
}

mob_rnum real_mobile(mob_vnum vnum)
{
  mob_rnum i;
  if (!mob_index || top_of_mobt == NOBODY)
    return NOBODY;
  for (i = 0; i <= top_of_mobt; i++)
    if (mob_index[i].vnum == vnum)
      return i;
  return NOBODY;
}

zone_rnum real_zone(zone_vnum vnum)
{
  zone_rnum i;
  if (!zone_table)
    return NOWHERE;
  for (i = 0; i <= top_of_zone_table; i++)
    if (zone_table[i].number == vnum)
      return i;
  return NOWHERE;
}

trig_rnum real_trigger(trig_vnum vnum)
{
  (void)vnum;
  return NOTHING;
}

/* Minimal stubs for linked OLC modules */
void clear_object(struct obj_data *obj)
{
  if (!obj)
    return;
  memset(obj, 0, sizeof(*obj));
  obj->item_number = NOTHING;
  obj->in_room = NOWHERE;
  obj->worn_on = NOWHERE;
}

void smash_tilde(char *str) { (void)str; }
void string_write(struct descriptor_data *d, char **writeto, size_t len, long mailto, void *data)
{ (void)d; (void)writeto; (void)len; (void)mailto; (void)data; }
void dg_script_menu(struct descriptor_data *d) { (void)d; }
int dg_script_edit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; return 0; }
void assign_triggers(void *i, int type) { (void)i; (void)type; }
void script_save_to_disk(FILE *fp, void *item, int type) { (void)fp; (void)item; (void)type; }
int save_triggers(zone_rnum zone_num) { (void)zone_num; return TRUE; }
int save_shops(zone_rnum zone_num) { (void)zone_num; return TRUE; }
int save_quests(zone_rnum zone_num) { (void)zone_num; return TRUE; }
int save_config(zone_rnum nowhere) { (void)nowhere; return TRUE; }
void free_shop(struct shop_data *shop) { free(shop); }
void free_quest(struct aq_data *quest) { free(quest); }
void free_action(struct social_messg *mess) { free(mess); }
void free_help(struct help_index_element *h) { free(h); }
void free_olc_ibt(struct ibt_data *ibt) { free(ibt); }
void free_message_list(struct message_list *mlist) { free(mlist); }
void affect_remove(struct char_data *ch, struct affected_type *af) { (void)ch; (void)af; }
void obj_from_room(struct obj_data *object) { (void)object; }
void obj_to_room(struct obj_data *object, room_rnum room) { (void)object; (void)room; }
void obj_from_obj(struct obj_data *obj) { (void)obj; }
void obj_to_obj(struct obj_data *obj, struct obj_data *obj_to) { (void)obj; (void)obj_to; }
void obj_to_char(struct obj_data *object, struct char_data *ch) { (void)object; (void)ch; }
void extract_obj(struct obj_data *obj) { (void)obj; }

/* Minimal dg_olc_script_copy matching src/dg_olc.c so tests exercise OLC_SCRIPT
 * allocation and freeing without pulling all of dg_olc.c. */
void dg_olc_script_copy(struct descriptor_data *d)
{
  struct trig_proto_list *origscript, *editscript;

  if (OLC_ITEM_TYPE(d) == MOB_TRIGGER)
    origscript = OLC_MOB(d)->proto_script;
  else if (OLC_ITEM_TYPE(d) == OBJ_TRIGGER)
    origscript = OLC_OBJ(d)->proto_script;
  else
    origscript = OLC_ROOM(d)->proto_script;

  if (origscript) {
    CREATE(editscript, struct trig_proto_list, 1);
    OLC_SCRIPT(d) = editscript;
    while (origscript) {
      editscript->vnum = origscript->vnum;
      origscript = origscript->next;
      if (origscript)
        CREATE(editscript->next, struct trig_proto_list, 1);
      editscript = editscript->next;
    }
  } else {
    OLC_SCRIPT(d) = NULL;
  }
}

/* Helper to attach a proto trigger node */
static struct trig_proto_list *make_proto_trig(trig_vnum vnum)
{
  struct trig_proto_list *node;
  CREATE(node, struct trig_proto_list, 1);
  node->vnum = vnum;
  node->next = NULL;
  return node;
}

static struct descriptor_data *make_builder(const char *name)
{
  struct descriptor_data *d;
  struct char_data *ch;

  CREATE(d, struct descriptor_data, 1);
  CREATE(ch, struct char_data, 1);
  CREATE(ch->player_specials, struct player_special_data, 1);
  ch->player.name = strdup(name);
  ch->player.level = LVL_IMPL;
  GET_OLC_ZONE(ch) = ALL_PERMISSION;
  ch->desc = d;
  d->character = ch;
  STATE(d) = CON_PLAYING;
  d->next = descriptor_list;
  descriptor_list = d;
  return d;
}

static void free_builder(struct descriptor_data *d)
{
  struct descriptor_data **pp;
  if (!d)
    return;
  if (d->olc)
    cleanup_olc(d, CLEANUP_ALL);
  for (pp = &descriptor_list; *pp; pp = &(*pp)->next) {
    if (*pp == d) {
      *pp = d->next;
      break;
    }
  }
  if (d->character) {
    free(d->character->player_specials);
    free(d->character->player.name);
    free(d->character);
  }
  free(d);
}

static void open_redit(struct descriptor_data *d, room_vnum vnum)
{
  room_rnum rnum;
  CREATE(d->olc, struct oasis_olc_data, 1);
  OLC_ZNUM(d) = real_zone_by_thing(vnum);
  OLC_NUM(d) = vnum;
  if ((rnum = real_room(vnum)) != NOWHERE) {
    redit_setup_existing(d, rnum);
  } else {
    CREATE(OLC_ROOM(d), struct room_data, 1);
    OLC_ROOM(d)->name = strdup("An unfinished room");
    OLC_ROOM(d)->description = strdup("You are in an unfinished room.\r\n");
    OLC_ROOM(d)->number = NOWHERE;
    OLC_ITEM_TYPE(d) = WLD_TRIGGER;
    OLC_ROOM(d)->proto_script = OLC_SCRIPT(d) = NULL;
    OLC_VAL(d) = 0;
  }
  OLC_MODE(d) = REDIT_MAIN_MENU;
  STATE(d) = CON_REDIT;
}

static void open_oedit(struct descriptor_data *d, obj_vnum vnum)
{
  obj_rnum rnum;
  CREATE(d->olc, struct oasis_olc_data, 1);
  OLC_ZNUM(d) = real_zone_by_thing(vnum);
  OLC_NUM(d) = vnum;
  if ((rnum = real_object(vnum)) != NOTHING) {
    oedit_setup_existing(d, rnum);
  } else {
    CREATE(OLC_OBJ(d), struct obj_data, 1);
    clear_object(OLC_OBJ(d));
    OLC_OBJ(d)->name = strdup("unfinished object");
    OLC_OBJ(d)->description = strdup("An unfinished object is lying here.");
    OLC_OBJ(d)->short_description = strdup("an unfinished object");
    OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
    OLC_OBJ(d)->proto_script = OLC_SCRIPT(d) = NULL;
    OLC_VAL(d) = 0;
  }
  OLC_MODE(d) = OEDIT_MAIN_MENU;
  STATE(d) = CON_OEDIT;
}

static void open_medit(struct descriptor_data *d, mob_vnum vnum)
{
  mob_rnum rnum;
  CREATE(d->olc, struct oasis_olc_data, 1);
  OLC_ZNUM(d) = real_zone_by_thing(vnum);
  OLC_NUM(d) = vnum;
  if ((rnum = real_mobile(vnum)) != NOBODY) {
    medit_setup_existing(d, rnum);
  } else {
    CREATE(OLC_MOB(d), struct char_data, 1);
    clear_char(OLC_MOB(d));
    GET_MOB_RNUM(OLC_MOB(d)) = NOBODY;
    GET_ALIAS(OLC_MOB(d)) = strdup("mob unfinished");
    GET_SDESC(OLC_MOB(d)) = strdup("the unfinished mob");
    GET_LDESC(OLC_MOB(d)) = strdup("An unfinished mob stands here.\r\n");
    GET_DDESC(OLC_MOB(d)) = strdup("It looks unfinished.\r\n");
    OLC_MOB(d)->player_specials = &dummy_mob;
    SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_ISNPC);
    OLC_ITEM_TYPE(d) = MOB_TRIGGER;
    OLC_MOB(d)->proto_script = OLC_SCRIPT(d) = NULL;
    OLC_VAL(d) = 0;
  }
  OLC_MODE(d) = MEDIT_MAIN_MENU;
  STATE(d) = CON_MEDIT;
}

static void feed_redit(struct descriptor_data *d, const char *input)
{
  char buf[MAX_INPUT_LENGTH];
  strlcpy(buf, input, sizeof(buf));
  redit_parse(d, buf);
}

static void feed_oedit(struct descriptor_data *d, const char *input)
{
  char buf[MAX_INPUT_LENGTH];
  strlcpy(buf, input, sizeof(buf));
  oedit_parse(d, buf);
}

static void feed_medit(struct descriptor_data *d, const char *input)
{
  char buf[MAX_INPUT_LENGTH];
  strlcpy(buf, input, sizeof(buf));
  medit_parse(d, buf);
}

static void read_log(char *buf, size_t cap)
{
  size_t n;
  fflush(logfile);
  rewind(logfile);
  n = fread(buf, 1, cap - 1, logfile);
  buf[n] = '\0';
}

void setUp(void)
{
  int i;

  logfile = tmpfile();
  TEST_ASSERT_NOT_NULL(logfile);

  memset(&config_info, 0, sizeof(config_info));
  CONFIG_OLC_SAVE = FALSE;
  descriptor_list = NULL;
  character_list = NULL;
  object_list = NULL;
  save_list = NULL;
  r_mortal_start_room = 0;
  r_immort_start_room = 0;
  r_frozen_start_room = 0;
  top_shop = -1;
  shop_index = NULL;
  for (i = 0; i < NUM_OF_BOARDS; i++)
    BOARD_RNUM(i) = NOTHING;

  /* Two zones: zone 0 (vnums 0-99) and zone 30 (vnums 3000-3099) */
  top_of_zone_table = 1;
  CREATE(zone_table, struct zone_data, 2);
  zone_table[0].number = 0;
  zone_table[0].bot = 0;
  zone_table[0].top = 99;
  CREATE(zone_table[0].cmd, struct reset_com, 1);
  zone_table[0].cmd[0].command = 'S';

  zone_table[1].number = 30;
  zone_table[1].bot = 3000;
  zone_table[1].top = 3099;
  CREATE(zone_table[1].cmd, struct reset_com, 1);
  zone_table[1].cmd[0].command = 'S';

  /* Rooms: 0 (The Void), 3000, 3001, 3002 */
  top_of_world = 3;
  CREATE(world, struct room_data, 4);
  {
    room_vnum vnums[4] = { 0, 3000, 3001, 3002 };
    for (i = 0; i < 4; i++) {
      char name[64];
      world[i].number = vnums[i];
      world[i].zone = (i == 0) ? 0 : 1;
      snprintf(name, sizeof(name), "Room %d", vnums[i]);
      world[i].name = strdup(name);
      world[i].description = strdup("A room.\r\n");
      if (vnums[i] == 3002)
        world[i].proto_script = make_proto_trig(3000);
    }
  }

  /* Objects: 3000, 3001, 3002 */
  top_of_objt = 2;
  CREATE(obj_index, struct index_data, 3);
  CREATE(obj_proto, struct obj_data, 3);
  for (i = 0; i < 3; i++) {
    char sdesc[64];
    clear_object(&obj_proto[i]);
    obj_index[i].vnum = 3000 + i;
    obj_proto[i].item_number = i;
    snprintf(sdesc, sizeof(sdesc), "object %d", 3000 + i);
    obj_proto[i].name = strdup(sdesc);
    obj_proto[i].short_description = strdup(sdesc);
    obj_proto[i].description = strdup("An object lies here.");
    if (i == 2)
      obj_proto[i].proto_script = make_proto_trig(3001);
  }

  /* Mobiles: 3000, 3001, 3002 */
  top_of_mobt = 2;
  CREATE(mob_index, struct index_data, 3);
  CREATE(mob_proto, struct char_data, 3);
  for (i = 0; i < 3; i++) {
    char sdesc[64];
    clear_char(&mob_proto[i]);
    mob_index[i].vnum = 3000 + i;
    mob_proto[i].nr = i;
    mob_proto[i].player_specials = &dummy_mob;
    SET_BIT_AR(MOB_FLAGS(&mob_proto[i]), MOB_ISNPC);
    snprintf(sdesc, sizeof(sdesc), "mobile %d", 3000 + i);
    GET_ALIAS(&mob_proto[i]) = strdup(sdesc);
    GET_SDESC(&mob_proto[i]) = strdup(sdesc);
    GET_LDESC(&mob_proto[i]) = strdup("A mobile stands here.\r\n");
    GET_DDESC(&mob_proto[i]) = strdup("A mobile.\r\n");
    if (i == 2)
      mob_proto[i].proto_script = make_proto_trig(3002);
  }

  clear_output();
}

void tearDown(void)
{
  int i;

  while (descriptor_list)
    free_builder(descriptor_list);

  free_save_list();
  save_list = NULL;

  if (world) {
    for (i = 0; i <= (int)top_of_world; i++) {
      free_room_strings(&world[i]);
      free_proto_script(&world[i], WLD_TRIGGER);
    }
    free(world);
    world = NULL;
  }

  if (obj_proto) {
    for (i = 0; i <= (int)top_of_objt; i++) {
      free_object_strings(&obj_proto[i]);
      free_proto_script(&obj_proto[i], OBJ_TRIGGER);
    }
    free(obj_proto);
    obj_proto = NULL;
  }
  free(obj_index);
  obj_index = NULL;

  if (mob_proto) {
    for (i = 0; i <= (int)top_of_mobt; i++) {
      free_mobile_strings(&mob_proto[i]);
      free_proto_script(&mob_proto[i], MOB_TRIGGER);
    }
    free(mob_proto);
    mob_proto = NULL;
  }
  free(mob_index);
  mob_index = NULL;

  if (zone_table) {
    for (i = 0; i <= (int)top_of_zone_table; i++)
      free(zone_table[i].cmd);
    free(zone_table);
    zone_table = NULL;
  }

  if (logfile) {
    fclose(logfile);
    logfile = NULL;
  }
}

/* 1. redit: W) Copy followed by X) Delete deletes the edited room, not source */
static void test_redit_delete_after_copy_targets_edited_room(void)
{
  struct descriptor_data *d = make_builder("Builder");
  open_redit(d, 3001);

  feed_redit(d, "w");
  feed_redit(d, "3002"); /* copies room 3002 (which also has a trigger) */
  TEST_ASSERT_NOT_NULL(OLC_SCRIPT(d));

  clear_output();
  feed_redit(d, "x");
  TEST_ASSERT_NOT_NULL(strstr(out_buf, "Are you sure you want to delete room 3001? "));
  TEST_ASSERT_EQUAL_INT(REDIT_DELETE, OLC_MODE(d));

  clear_output();
  feed_redit(d, "y");
  TEST_ASSERT_NOT_NULL(strstr(out_buf, "Room deleted."));
  TEST_ASSERT_NULL(d->olc);
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(d));
  TEST_ASSERT_EQUAL_UINT16(NOWHERE, real_room(3001));
  TEST_ASSERT_NOT_EQUAL_UINT16(NOWHERE, real_room(3002));
}

/* 2. oedit: W) Copy followed by X) Delete deletes the edited object, not source */
static void test_oedit_delete_after_copy_targets_edited_object(void)
{
  struct descriptor_data *d = make_builder("Builder");
  open_oedit(d, 3001);

  feed_oedit(d, "w");
  feed_oedit(d, "3002"); /* copies object 3002 (which also has a trigger) */
  TEST_ASSERT_NOT_NULL(OLC_SCRIPT(d));

  clear_output();
  feed_oedit(d, "x");
  TEST_ASSERT_NOT_NULL(strstr(out_buf, "Are you sure you want to delete object 3001? "));
  TEST_ASSERT_EQUAL_INT(OEDIT_DELETE, OLC_MODE(d));

  clear_output();
  feed_oedit(d, "y");
  TEST_ASSERT_NOT_NULL(strstr(out_buf, "Object deleted."));
  TEST_ASSERT_NULL(d->olc);
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(d));
  TEST_ASSERT_EQUAL_UINT16(NOTHING, real_object(3001));
  TEST_ASSERT_NOT_EQUAL_UINT16(NOTHING, real_object(3002));
}

/* 3. oedit: deleting after a lower-vnum object was removed still targets OLC_NUM */
static void test_oedit_delete_after_table_shift_targets_edited_object(void)
{
  struct descriptor_data *d = make_builder("Builder");
  open_oedit(d, 3002);

  /* Another builder deletes 3000; 3002 shifts from rnum 2 to rnum 1. */
  TEST_ASSERT_NOT_EQUAL_UINT16(NOTHING, delete_object(real_object(3000)));
  TEST_ASSERT_EQUAL_UINT16(1, real_object(3002));

  clear_output();
  feed_oedit(d, "x");
  TEST_ASSERT_NOT_NULL(strstr(out_buf, "Are you sure you want to delete object 3002? "));

  clear_output();
  feed_oedit(d, "y");
  TEST_ASSERT_NOT_NULL(strstr(out_buf, "Object deleted."));
  TEST_ASSERT_EQUAL_UINT16(NOTHING, real_object(3002));
  TEST_ASSERT_NOT_EQUAL_UINT16(NOTHING, real_object(3001));
}

/* 4. medit: W) Copy of last mobile + X) Delete deletes edited mobile safely */
static void test_medit_delete_after_copy_targets_edited_mobile(void)
{
  struct descriptor_data *d = make_builder("Builder");
  open_medit(d, 3001);

  feed_medit(d, "w");
  feed_medit(d, "3002"); /* rnum == top_of_mobt, also has a trigger */
  TEST_ASSERT_NOT_NULL(OLC_SCRIPT(d));

  clear_output();
  feed_medit(d, "x");
  TEST_ASSERT_NOT_NULL(strstr(out_buf, "Are you sure you want to delete mobile 3001? "));
  TEST_ASSERT_EQUAL_INT(MEDIT_DELETE, OLC_MODE(d));

  clear_output();
  feed_medit(d, "y");
  TEST_ASSERT_NOT_NULL(strstr(out_buf, "Mobile deleted."));
  TEST_ASSERT_NULL(d->olc);
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(d));
  TEST_ASSERT_EQUAL_UINT16(NOBODY, real_mobile(3001));
  TEST_ASSERT_NOT_EQUAL_UINT16(NOBODY, real_mobile(3002));
}

/* 5. medit: deleting a mobile keeps other open medit sessions safe on cleanup */
static void test_medit_delete_updates_other_open_medit_rnums(void)
{
  struct descriptor_data *d_last = make_builder("BuilderLast");
  struct descriptor_data *d_copy = make_builder("BuilderCopy");
  struct descriptor_data *d_del  = make_builder("BuilderDel");

  open_medit(d_last, 3002); /* holds rnum 2 == top_of_mobt */
  open_medit(d_copy, 3000);
  feed_medit(d_copy, "w");
  feed_medit(d_copy, "3001"); /* holds rnum 1 == the mobile about to be deleted */
  open_medit(d_del, 3001);

  feed_medit(d_del, "x");
  feed_medit(d_del, "y");

  TEST_ASSERT_EQUAL_UINT16(NOBODY, real_mobile(3001));
  TEST_ASSERT_EQUAL_UINT16(1, top_of_mobt);
  /* d_last shifted down to 3002's new rnum; d_copy cleared to NOBODY */
  TEST_ASSERT_EQUAL_UINT16(1, GET_MOB_RNUM(OLC_MOB(d_last)));
  TEST_ASSERT_EQUAL_UINT16(NOBODY, GET_MOB_RNUM(OLC_MOB(d_copy)));

  /* Both surviving editors clean up without OOB reads into mob_proto[] */
  OLC_MOB(d_last)->proto_script = OLC_SCRIPT(d_last);
  OLC_SCRIPT(d_last) = NULL;
  cleanup_olc(d_last, CLEANUP_ALL);
  OLC_MOB(d_copy)->proto_script = OLC_SCRIPT(d_copy);
  OLC_SCRIPT(d_copy) = NULL;
  cleanup_olc(d_copy, CLEANUP_ALL);
}

/* 6. free_mobile(): handles out-of-range cached rnum safely after direct delete */
static void test_free_mobile_handles_stale_oob_rnum(void)
{
  struct char_data *mob;
  CREATE(mob, struct char_data, 1);
  copy_mobile(mob, &mob_proto[2]); /* cached nr == 2 == top_of_mobt */
  mob->proto_script = NULL;
  SCRIPT(mob) = NULL;

  /* Direct delete shrinks top_of_mobt to 1 without updating external copies */
  TEST_ASSERT_NOT_EQUAL_UINT16(NOBODY, delete_mobile(real_mobile(3001)));
  TEST_ASSERT_EQUAL_UINT16(1, top_of_mobt);
  TEST_ASSERT_EQUAL_UINT16(2, GET_MOB_RNUM(mob));

  TEST_ASSERT_TRUE(free_mobile(mob));
}

/* 7. Unsaved records refuse X) Delete up front without touching copied source */
static void test_unsaved_records_refuse_delete_upfront(void)
{
  struct descriptor_data *dr = make_builder("RoomBuilder");
  struct descriptor_data *do_ = make_builder("ObjBuilder");
  struct descriptor_data *dm = make_builder("MobBuilder");

  open_redit(dr, 3099);
  feed_redit(dr, "w");
  feed_redit(dr, "3001");
  clear_output();
  feed_redit(dr, "x");
  TEST_ASSERT_NOT_NULL(strstr(out_buf, "never been saved"));
  TEST_ASSERT_NOT_NULL(dr->olc);
  TEST_ASSERT_EQUAL_INT(REDIT_MAIN_MENU, OLC_MODE(dr));
  TEST_ASSERT_NOT_EQUAL_UINT16(NOWHERE, real_room(3001));

  open_oedit(do_, 3099);
  feed_oedit(do_, "w");
  feed_oedit(do_, "3001");
  clear_output();
  feed_oedit(do_, "x");
  TEST_ASSERT_NOT_NULL(strstr(out_buf, "never been saved"));
  TEST_ASSERT_NOT_NULL(do_->olc);
  TEST_ASSERT_EQUAL_INT(OEDIT_MAIN_MENU, OLC_MODE(do_));
  TEST_ASSERT_NOT_EQUAL_UINT16(NOTHING, real_object(3001));

  open_medit(dm, 3099);
  feed_medit(dm, "w");
  feed_medit(dm, "3001");
  clear_output();
  feed_medit(dm, "x");
  TEST_ASSERT_NOT_NULL(strstr(out_buf, "never been saved"));
  TEST_ASSERT_NOT_NULL(dm->olc);
  TEST_ASSERT_EQUAL_INT(MEDIT_MAIN_MENU, OLC_MODE(dm));
  TEST_ASSERT_NOT_EQUAL_UINT16(NOBODY, real_mobile(3001));

  /* Clean up detached script lists before tearDown's cleanup_olc */
  OLC_ROOM(dr)->proto_script = OLC_SCRIPT(dr);
  OLC_SCRIPT(dr) = NULL;
  OLC_OBJ(do_)->proto_script = OLC_SCRIPT(do_);
  free_proto_script(OLC_OBJ(do_), OBJ_TRIGGER);
  OLC_SCRIPT(do_) = NULL;
  OLC_MOB(dm)->proto_script = OLC_SCRIPT(dm);
  OLC_SCRIPT(dm) = NULL;
}

/* 8. Failed confirmation keeps editor open and logs no SYSERR in medit */
static void test_failed_delete_keeps_session_open_and_no_syserr(void)
{
  char logbuf[4096];
  struct descriptor_data *dr = make_builder("RoomBuilder");
  struct descriptor_data *do_ = make_builder("ObjBuilder");
  struct descriptor_data *dm = make_builder("MobBuilder");

  /* Room 0 (The Void): exists so X prompts, but delete_room(0) refuses. */
  open_redit(dr, 0);
  feed_redit(dr, "x");
  TEST_ASSERT_EQUAL_INT(REDIT_DELETE, OLC_MODE(dr));
  clear_output();
  feed_redit(dr, "y");
  TEST_ASSERT_NOT_NULL(strstr(out_buf, "Couldn't delete the room!"));
  TEST_ASSERT_NOT_NULL(dr->olc);
  TEST_ASSERT_EQUAL_INT(REDIT_MAIN_MENU, OLC_MODE(dr));
  TEST_ASSERT_EQUAL_INT(CON_REDIT, STATE(dr));

  /* Object disappears between X prompt and 'y' confirmation. */
  open_oedit(do_, 3001);
  feed_oedit(do_, "x");
  TEST_ASSERT_EQUAL_INT(OEDIT_DELETE, OLC_MODE(do_));
  TEST_ASSERT_NOT_EQUAL_UINT16(NOTHING, delete_object(real_object(3001)));
  clear_output();
  feed_oedit(do_, "y");
  TEST_ASSERT_NOT_NULL(strstr(out_buf, "Couldn't delete the object!"));
  TEST_ASSERT_NOT_NULL(do_->olc);
  TEST_ASSERT_EQUAL_INT(OEDIT_MAIN_MENU, OLC_MODE(do_));
  TEST_ASSERT_EQUAL_INT(CON_OEDIT, STATE(do_));

  /* Mobile disappears between X prompt and 'y' confirmation: no SYSERR. */
  open_medit(dm, 3001);
  feed_medit(dm, "x");
  TEST_ASSERT_EQUAL_INT(MEDIT_DELETE, OLC_MODE(dm));
  TEST_ASSERT_NOT_EQUAL_UINT16(NOBODY, delete_mobile(real_mobile(3001)));
  clear_output();
  feed_medit(dm, "y");
  TEST_ASSERT_NOT_NULL(strstr(out_buf, "Couldn't delete the mobile!"));
  TEST_ASSERT_NOT_NULL(dm->olc);
  TEST_ASSERT_EQUAL_INT(MEDIT_MAIN_MENU, OLC_MODE(dm));
  TEST_ASSERT_EQUAL_INT(CON_MEDIT, STATE(dm));

  read_log(logbuf, sizeof(logbuf));
  TEST_ASSERT_NULL_MESSAGE(strstr(logbuf, "SYSERR"), logbuf);
}

/* 9. medit: W) Copy + save keeps mob_proto[rnum].nr aligned with rnum */
static void test_medit_copy_and_save_preserves_proto_rnum(void)
{
  struct descriptor_data *d = make_builder("Builder");
  open_medit(d, 3001);

  feed_medit(d, "w");
  feed_medit(d, "3002");
  TEST_ASSERT_EQUAL_UINT16(2, GET_MOB_RNUM(OLC_MOB(d)));

  medit_save_internally(d);
  cleanup_olc(d, CLEANUP_ALL);

  TEST_ASSERT_EQUAL_UINT16(1, real_mobile(3001));
  TEST_ASSERT_EQUAL_UINT16(1, mob_proto[1].nr);
  TEST_ASSERT_EQUAL_UINT16(3001, mob_index[1].vnum);
  TEST_ASSERT_EQUAL_STRING("mobile 3002", GET_SDESC(&mob_proto[1]));
  /* 3002 itself remains intact */
  TEST_ASSERT_EQUAL_UINT16(2, real_mobile(3002));
  TEST_ASSERT_EQUAL_UINT16(2, mob_proto[2].nr);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_redit_delete_after_copy_targets_edited_room);
  RUN_TEST(test_oedit_delete_after_copy_targets_edited_object);
  RUN_TEST(test_oedit_delete_after_table_shift_targets_edited_object);
  RUN_TEST(test_medit_delete_after_copy_targets_edited_mobile);
  RUN_TEST(test_medit_delete_updates_other_open_medit_rnums);
  RUN_TEST(test_free_mobile_handles_stale_oob_rnum);
  RUN_TEST(test_unsaved_records_refuse_delete_upfront);
  RUN_TEST(test_failed_delete_keeps_session_open_and_no_syserr);
  RUN_TEST(test_medit_copy_and_save_preserves_proto_rnum);
  return UNITY_END();
}
