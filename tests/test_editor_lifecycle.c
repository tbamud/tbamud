/* Regression coverage for note buffer isolation during prototype updates and
 * object removal, IBT disconnect cleanup + OLC index shifting + remove guard,
 * and login/reconnect clearing of stale report-writing flags. */
#include "unity.h"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "modify.h"
#include "improved-edit.h"
#include "ibt.h"
#include "genobj.h"
#include "genolc.h"
#include "oasis.h"
#include "boards.h"
#include "mail.h"
#include "quest.h"
#include "dg_scripts.h"
#include "hedit.h"
#include "prefedit.h"
#include "shop.h"
#include "genmob.h"
#include "genshp.h"
#include "msgedit.h"
#include "house.h"
#include "limits.h"
#include "fight.h"
#include "act.h"
#if defined(CIRCLE_WINDOWS) || defined(_WIN32)
#include <direct.h>
#include <windows.h>
#define test_mkdir(path) _mkdir(path)
#define test_chdir(path) _chdir(path)
#define test_rmdir(path) _rmdir(path)
#define test_getcwd(buf, size) _getcwd(buf, size)
#else
#include <sys/stat.h>
#include <sys/resource.h>
#include <signal.h>
#define test_mkdir(path) mkdir(path, 0700)
#define test_chdir(path) chdir(path)
#define test_rmdir(path) rmdir(path)
#define test_getcwd(buf, size) getcwd(buf, size)
#endif

/* Globals defined strongly in test_stubs.c */
extern FILE *logfile;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern room_rnum top_of_world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct obj_data *obj_proto;
extern struct index_data *obj_index;
extern obj_rnum top_of_objt;
extern struct config_data config_info;
extern char *motd;

/* Additional globals needed by linked production modules */
struct zone_data *zone_table = NULL;
zone_rnum top_of_zone_table = -1;
struct char_data *mob_proto = NULL;
struct shop_data *shop_index = NULL;
int top_of_shopt = -1;
struct attack_hit_type attack_hit_text[] = { { "hit", "hits" } };
const char *ibt_bits[] = { "RESOLVED", "VNUM", "DARK", "INPROGRESS", "\n" };
const char *MENU = "";
const char *WELC_MESSG = "";
const char *START_MESSG = "";
char *credits = NULL;
char *news = NULL;
char *info = NULL;
char *wizlist = NULL;
char *immlist = NULL;
char *policies = NULL;
char *handbook = NULL;
char *help = NULL;
char *ihelp = NULL;
int no_mail = 1;
const char *connected_types[35] = {
  "Playing", "Disconnecting", "Get name", "Confirm name", "Get password",
  "Get new PW", "Confirm new PW", "Select sex", "Select class", "Reading MOTD",
  "Main menu", "Enter descript.", "Changing PW 1", "Changing PW 2", "Changing PW 3",
  "Self-Delete 1", "Self-Delete 2", "Disconnecting", "Object edit", "Room edit",
  "Zone edit", "Mobile edit", "Shop edit", "Text edit", "Config edit",
  "Social edit", "Trigger edit", "Help edit", "Quest edit", "Preferences edit",
  "IBT edit", "Message edit", "Protocol", "\n"
};
const char *class_abbrevs[] = { "Mu", "\n" };
const char *syslog_types[] = { "Off", "Brief", "Normal", "Complete", "\n" };

/* Stubs for external helpers referenced by linked production modules */
obj_rnum real_object(obj_vnum vnum) { return (vnum == 3000 && obj_proto) ? 0 : NOTHING; }
zone_rnum real_zone(zone_vnum vnum) { return (zone_table && top_of_zone_table >= 0 && vnum == 30) ? 0 : NOWHERE; }
zone_rnum real_zone_by_thing(room_vnum vnum) { (void)vnum; return (zone_table && top_of_zone_table >= 0) ? 0 : NOWHERE; }
int store_mail(long to, long from, char *message_pointer) { (void)to; (void)from; (void)message_pointer; return 1; }
void notify_if_playing(struct char_data *from, int recipient_id) { (void)from; (void)recipient_id; }
int board_abort_write(int board_type, char **writeto) { (void)board_type; (void)writeto; return 0; }
void board_save_board(int board_type) { (void)board_type; }
void cedit_string_cleanup(struct descriptor_data *d, int terminator) { (void)d; (void)terminator; }
void medit_string_cleanup(struct descriptor_data *d, int terminator) { (void)d; (void)terminator; }
void oedit_string_cleanup(struct descriptor_data *d, int terminator) { (void)d; (void)terminator; }
void redit_string_cleanup(struct descriptor_data *d, int terminator) { (void)d; (void)terminator; }
void trigedit_string_cleanup(struct descriptor_data *d, int terminator) { (void)d; (void)terminator; }
void hedit_string_cleanup(struct descriptor_data *d, int terminator) { (void)d; (void)terminator; }
void qedit_string_cleanup(struct descriptor_data *d, int terminator) { (void)d; (void)terminator; }
void cedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
void medit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
void oedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
void redit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
void tedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
void zedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
void sedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
void aedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
void hedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
void qedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
void msgedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
void trigedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
void genolc_free_obj(struct obj_data *obj) { (void)obj; }
void free_room(struct room_data *room) { free(room); }
int free_mobile(struct char_data *mob) { free(mob); return 1; }
void free_shop(struct shop_data *shop) { free(shop); }
void free_quest(struct aq_data *quest) { free(quest); }
void free_action(struct social_messg *mess) { free(mess); }
void free_help(struct help_index_element *h) { free(h); }
void free_message_list(struct message_list *mlist) { free(mlist); }
void free_trigger(trig_data *trig) { free(trig); }
void set_default_prefs(struct char_data *ch, int flags[]) { (void)ch; (void)flags; }
int House_can_enter(struct char_data *ch, room_vnum house) { (void)ch; (void)house; return 1; }
void reboot_wizlists(void) {}
void update_pos(struct char_data *victim) { (void)victim; }
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype)
{ (void)ch; (void)victim; (void)dam; (void)attacktype; return 0; }
void timer_otrigger(obj_data *obj) { (void)obj; }
void game_info(const char *messg, ...) { (void)messg; }
int free_strings(void *data, int type) { (void)data; (void)type; return 1; }
int Crash_rentsave(struct char_data *ch, int cost) { (void)ch; (void)cost; return 1; }
int Crash_idlesave(struct char_data *ch) { (void)ch; return 1; }

struct board_info_type board_info[NUM_OF_BOARDS];
int top_shop = -1;

char *fread_word(FILE *fp)
{
  static char buf[MAX_INPUT_LENGTH];
  size_t i = 0;
  int c;

  while ((c = getc(fp)) != EOF && isspace(c))
    ;
  if (c == EOF) {
    buf[0] = '\0';
    return buf;
  }
  do {
    if (i + 1 < sizeof(buf))
      buf[i++] = (char)c;
    c = getc(fp);
  } while (c != EOF && !isspace(c));
  if (c != EOF)
    ungetc(c, fp);
  buf[i] = '\0';
  return buf;
}

void fread_to_eol(FILE *fp)
{
  int c;
  while ((c = getc(fp)) != EOF && c != '\n')
    ;
}

int fread_number(FILE *fp)
{
  int val = 0;
  if (fscanf(fp, "%d", &val) != 1)
    return 0;
  return val;
}

int fread_flags(FILE *fp, int *fg, int fg_size)
{
  int i;
  for (i = 0; i < fg_size; i++)
    fg[i] = fread_number(fp);
  fread_to_eol(fp);
  return 0;
}

char *fread_line(FILE *fp)
{
  static char buf[MAX_STRING_LENGTH];
  size_t i = 0;
  int c;

  while ((c = getc(fp)) != EOF && (c == ' ' || c == '\t'))
    ;
  while (c != EOF && c != '\n') {
    if (c != '\r' && i + 1 < sizeof(buf))
      buf[i++] = (char)c;
    c = getc(fp);
  }
  if (i > 0 && buf[i - 1] == '~')
    i--;
  buf[i] = '\0';
  return buf;
}

char *fread_clean_string(FILE *fl, const char *error)
{
  char buf[MAX_STRING_LENGTH];
  size_t i = 0;
  int c;
  (void)error;

  while ((c = getc(fl)) != EOF && isspace(c))
    ;
  if (c != EOF && c != '~') {
    ungetc(c, fl);
    while ((c = getc(fl)) != EOF && c != '~') {
      if (c == '\r')
        continue;
      if (c == '\n') {
        if (i + 2 < sizeof(buf)) {
          buf[i++] = '\r';
          buf[i++] = '\n';
        }
      } else if (i + 1 < sizeof(buf)) {
        buf[i++] = (char)c;
      }
    }
  }
  buf[i] = '\0';
  return strdup(buf);
}

/* Test-local hooks overriding weak symbols in test_stubs.c */
static int hook_load_char = 0;
static bitvector_t hook_plr_flags[PM_ARRAY_MAX];
static int free_char_count = 0;
static struct char_data *last_freed_char = NULL;

void free_char(struct char_data *ch)
{
  free_char_count++;
  last_freed_char = ch;
}

int load_char(const char *name, struct char_data *ch)
{
  size_t i;
  (void)name;
  if (!hook_load_char)
    return (-1);
  for (i = 0; i < PM_ARRAY_MAX; i++)
    PLR_FLAGS(ch)[i] = hook_plr_flags[i];
  return (0);
}

void free_obj(struct obj_data *obj)
{
  if (!obj)
    return;
  if (obj->action_description &&
      (GET_OBJ_RNUM(obj) == NOTHING || !obj_proto ||
       obj->action_description != obj_proto[GET_OBJ_RNUM(obj)].action_description))
    free(obj->action_description);
  if (obj->name &&
      (GET_OBJ_RNUM(obj) == NOTHING || !obj_proto ||
       obj->name != obj_proto[GET_OBJ_RNUM(obj)].name))
    free(obj->name);
  if (obj->description &&
      (GET_OBJ_RNUM(obj) == NOTHING || !obj_proto ||
       obj->description != obj_proto[GET_OBJ_RNUM(obj)].description))
    free(obj->description);
  if (obj->short_description &&
      (GET_OBJ_RNUM(obj) == NOTHING || !obj_proto ||
       obj->short_description != obj_proto[GET_OBJ_RNUM(obj)].short_description))
    free(obj->short_description);
  free(obj);
}

static struct command_info test_cmds[] = {
  { "bug", "bug", POS_DEAD, do_ibt, 0, SCMD_BUG },
  { "\n", "", 0, NULL, 0, 0 }
};

static struct zone_data fixture_zone;
static struct room_data fixture_rooms[4];
static struct obj_data fixture_protos[2];
static struct index_data fixture_obj_index[2];
static char orig_cwd[4096];
static char active_tmpdir[4096];
static int has_tmpdir = 0;

static struct char_data *make_test_char(const char *name, int level, long idnum, struct descriptor_data *d)
{
  struct char_data *ch = calloc(1, sizeof(*ch));
  TEST_ASSERT_NOT_NULL(ch);
  ch->player_specials = calloc(1, sizeof(*ch->player_specials));
  TEST_ASSERT_NOT_NULL(ch->player_specials);
  ch->player.name = strdup(name);
  ch->player.level = level;
  ch->char_specials.saved.idnum = idnum;
  IN_ROOM(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = GET_MAX_HIT(ch) = 100;
  GET_MANA(ch) = GET_MAX_MANA(ch) = 100;
  GET_MOVE(ch) = GET_MAX_MOVE(ch) = 100;
  GET_WAS_IN(ch) = NOWHERE;
  ch->next = character_list;
  character_list = ch;
  char_to_room(ch, 0);
  d->character = ch;
  d->connected = CON_PLAYING;
  d->next = descriptor_list;
  descriptor_list = d;
  ch->desc = d;
  return ch;
}

static void free_test_char(struct char_data *ch)
{
  struct char_data **cur;
  if (!ch)
    return;
  for (cur = &character_list; *cur; ) {
    if (*cur == ch)
      *cur = ch->next;
    else
      cur = &(*cur)->next;
  }
  if (ch->desc)
    clear_pager(ch->desc);
  while (ch->carrying)
    extract_obj(ch->carrying);
  if (GET_EQ(ch, WEAR_HOLD))
    extract_obj(unequip_char(ch, WEAR_HOLD));
  if (IN_ROOM(ch) != NOWHERE)
    char_from_room(ch);
  if (ch->desc && ch->desc->note_buf) {
    free(ch->desc->note_buf);
    ch->desc->note_buf = NULL;
  }
  if (ch->player.name)
    free(ch->player.name);
  if (GET_HOST(ch))
    free(GET_HOST(ch));
  if (ch->player_specials && ch->player_specials != &dummy_mob)
    free(ch->player_specials);
  free(ch);
}

static void unlink_descriptor(struct descriptor_data *d)
{
  struct descriptor_data **cur = &descriptor_list;
  while (*cur) {
    if (*cur == d) {
      *cur = d->next;
      d->next = NULL;
      return;
    }
    cur = &(*cur)->next;
  }
}

static void attach_pen_and_paper(struct char_data *ch, const char *proto_text,
                                 struct obj_data **out_paper)
{
  struct obj_data *pen = calloc(1, sizeof(*pen));
  struct obj_data *paper = calloc(1, sizeof(*paper));

  TEST_ASSERT_NOT_NULL(pen);
  TEST_ASSERT_NOT_NULL(paper);

  if (fixture_protos[0].name) free(fixture_protos[0].name);
  if (fixture_protos[0].short_description) free(fixture_protos[0].short_description);
  if (fixture_protos[0].description) free(fixture_protos[0].description);
  if (fixture_protos[0].action_description) free(fixture_protos[0].action_description);

  fixture_protos[0].item_number = 0;
  fixture_protos[0].obj_flags.type_flag = ITEM_NOTE;
  fixture_protos[0].name = strdup("paper note");
  fixture_protos[0].short_description = strdup("a sheet of paper");
  fixture_protos[0].description = strdup("A sheet of paper lies here.");
  fixture_protos[0].action_description = proto_text ? strdup(proto_text) : NULL;

  paper->item_number = 0;
  paper->obj_flags.type_flag = ITEM_NOTE;
  paper->name = fixture_protos[0].name;
  paper->short_description = fixture_protos[0].short_description;
  paper->description = fixture_protos[0].description;
  paper->action_description = fixture_protos[0].action_description;
  paper->in_room = NOWHERE;
  paper->worn_on = NOWHERE;
  paper->next = object_list;
  object_list = paper;
  obj_to_char(paper, ch);

  pen->item_number = NOTHING;
  pen->obj_flags.type_flag = ITEM_PEN;
  pen->name = strdup("pen quill");
  pen->short_description = strdup("a quill pen");
  pen->description = strdup("A quill pen lies here.");
  pen->in_room = NOWHERE;
  pen->worn_on = NOWHERE;
  pen->next = object_list;
  object_list = pen;
  equip_char(ch, pen, WEAR_HOLD);

  *out_paper = paper;
}

static void refresh_note_proto(const char *new_text)
{
  struct obj_data updated;
  memset(&updated, 0, sizeof(updated));
  updated.item_number = 0;
  updated.obj_flags.type_flag = ITEM_NOTE;
  updated.name = strdup("paper note");
  updated.short_description = strdup("a sheet of paper");
  updated.description = strdup("A sheet of paper lies here.");
  updated.action_description = strdup(new_text);

  TEST_ASSERT_EQUAL_INT(0, add_object(&updated, 3000));

  free(updated.name);
  free(updated.short_description);
  free(updated.description);
  free(updated.action_description);
}

void setUp(void)
{
  logfile = stderr;
  descriptor_list = NULL;
  complete_cmd_info = test_cmds;
  object_list = NULL;
  character_list = NULL;
  hook_load_char = 0;
  free_char_count = 0;
  last_freed_char = NULL;
  memset(hook_plr_flags, 0, sizeof(hook_plr_flags));

  memset(&config_info, 0, sizeof(config_info));
  config_info.operation.ibt_autosave = 1;
  config_info.operation.use_new_socials = 0;
  CONFIG_MENU = "0) Exit\r\n1) Enter\r\n";
  CONFIG_OK = "Okay.\r\n";
  CONFIG_FREE_RENT = 1;
  CONFIG_IDLE_VOID = 5;
  CONFIG_IDLE_RENT_TIME = 10;
  CONFIG_IDLE_MAX_LEVEL = LVL_IMMORT;

  memset(&fixture_zone, 0, sizeof(fixture_zone));
  fixture_zone.number = 30;
  fixture_zone.bot = 3000;
  fixture_zone.top = 3099;
  zone_table = &fixture_zone;
  top_of_zone_table = 0;

  memset(fixture_rooms, 0, sizeof(fixture_rooms));
  fixture_rooms[0].number = 3000;
  fixture_rooms[0].name = "Test Room";
  fixture_rooms[1].number = 1;
  fixture_rooms[1].name = "The Void";
  fixture_rooms[2].number = 2;
  fixture_rooms[2].name = "Limbo";
  fixture_rooms[3].number = 3;
  fixture_rooms[3].name = "Idle Storage";
  world = fixture_rooms;
  top_of_world = 3;

  memset(fixture_protos, 0, sizeof(fixture_protos));
  memset(fixture_obj_index, 0, sizeof(fixture_obj_index));
  fixture_obj_index[0].vnum = 3000;
  fixture_protos[0].item_number = 0;
  obj_proto = fixture_protos;
  obj_index = fixture_obj_index;
  top_of_objt = 0;

  if (!test_getcwd(orig_cwd, sizeof(orig_cwd)))
    TEST_FAIL_MESSAGE("getcwd failed");
#if defined(CIRCLE_WINDOWS) || defined(_WIN32)
  {
    char root[MAX_PATH];
    if (!GetTempPathA(sizeof(root), root) ||
        !GetTempFileNameA(root, "edt", 0, active_tmpdir))
      TEST_FAIL_MESSAGE("GetTempFileNameA failed");
    if (!DeleteFileA(active_tmpdir) || test_mkdir(active_tmpdir) != 0)
      TEST_FAIL_MESSAGE("failed to create temp dir");
  }
#else
  strcpy(active_tmpdir, "/tmp/tbamud-editor-test-XXXXXX");
  TEST_ASSERT_NOT_NULL(mkdtemp(active_tmpdir));
#endif
  if (test_chdir(active_tmpdir) != 0) {
    (void)test_rmdir(active_tmpdir);
    TEST_FAIL_MESSAGE("chdir to temp dir failed");
  }
  has_tmpdir = 1;
  TEST_ASSERT_EQUAL_INT(0, test_mkdir("misc"));
  TEST_ASSERT_EQUAL_INT(0, test_mkdir("text"));
}

void tearDown(void)
{
  size_t i;

  free_ibt_lists();
  while (object_list)
    extract_obj(object_list);

  for (i = 0; i < 2; i++) {
    if (fixture_protos[i].name) free(fixture_protos[i].name);
    if (fixture_protos[i].short_description) free(fixture_protos[i].short_description);
    if (fixture_protos[i].description) free(fixture_protos[i].description);
    if (fixture_protos[i].action_description) free(fixture_protos[i].action_description);
    fixture_protos[i].name = NULL;
    fixture_protos[i].short_description = NULL;
    fixture_protos[i].description = NULL;
    fixture_protos[i].action_description = NULL;
  }

  descriptor_list = NULL;
  obj_proto = NULL;
  obj_index = NULL;
  zone_table = NULL;
  top_of_zone_table = -1;

  if (motd) {
    free(motd);
    motd = NULL;
  }
  if (news) {
    free(news);
    news = NULL;
  }

  if (has_tmpdir) {
    (void)remove("misc/bugs");
    (void)remove("misc/ideas");
    (void)remove("misc/typos");
    (void)remove("text/motd");
    (void)remove("text/motd.tmp");
    (void)remove("text/news");
    (void)remove("text/news.tmp");
    (void)test_rmdir("misc");
    (void)test_rmdir("text");
    if (test_chdir(orig_cwd) != 0) {}
    (void)test_rmdir(active_tmpdir);
    has_tmpdir = 0;
  }
  logfile = NULL;
}

static void send_editor_line(struct descriptor_data *d, const char *line)
{
  char buf[MAX_INPUT_LENGTH];
  snprintf(buf, sizeof(buf), "%s", line);
  string_add(d, buf);
}

static void test_note_abort_after_prototype_refresh_preserves_proto(void)
{
  struct descriptor_data d;
  struct char_data *ch;
  struct obj_data *paper = NULL;
  char arg[MAX_INPUT_LENGTH];

  memset(&d, 0, sizeof(d));
  ch = make_test_char("Writer", 10, 1001, &d);
  attach_pen_and_paper(ch, "Initial proto text\r\n", &paper);

  snprintf(arg, sizeof(arg), "paper");
  do_write(ch, arg, 0, 0);

  TEST_ASSERT_EQUAL_PTR(&d.note_buf, d.str);
  TEST_ASSERT_EQUAL_PTR(paper, d.note_obj);
  TEST_ASSERT_NOT_NULL(d.note_buf);

  send_editor_line(&d, "Player draft line");
  refresh_note_proto("Refreshed proto text\r\n");

  TEST_ASSERT_EQUAL_PTR(fixture_protos[0].action_description, paper->action_description);
  TEST_ASSERT_EQUAL_STRING("Refreshed proto text\r\n", fixture_protos[0].action_description);

  send_editor_line(&d, "/a");

  TEST_ASSERT_NULL(d.str);
  TEST_ASSERT_NULL(d.note_buf);
  TEST_ASSERT_NULL(d.note_obj);
  TEST_ASSERT_FALSE(PLR_FLAGGED(ch, PLR_WRITING));
  TEST_ASSERT_EQUAL_STRING("Refreshed proto text\r\n", fixture_protos[0].action_description);
  TEST_ASSERT_EQUAL_STRING("Refreshed proto text\r\n", paper->action_description);

  unlink_descriptor(&d);
  free_test_char(ch);
}

static void test_note_save_after_prototype_refresh_preserves_proto(void)
{
  struct descriptor_data d;
  struct char_data *ch;
  struct obj_data *paper = NULL;
  char arg[MAX_INPUT_LENGTH];

  memset(&d, 0, sizeof(d));
  ch = make_test_char("Writer", 10, 1002, &d);
  attach_pen_and_paper(ch, "Initial proto text\r\n", &paper);

  snprintf(arg, sizeof(arg), "paper");
  do_write(ch, arg, 0, 0);
  send_editor_line(&d, "Saved note addition");
  refresh_note_proto("Refreshed proto text\r\n");

  REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CRASH);
  send_editor_line(&d, "/s");

  TEST_ASSERT_NULL(d.str);
  TEST_ASSERT_NULL(d.note_buf);
  TEST_ASSERT_NULL(d.note_obj);
  TEST_ASSERT_FALSE(PLR_FLAGGED(ch, PLR_WRITING));
  TEST_ASSERT_TRUE(PLR_FLAGGED(ch, PLR_CRASH));
  TEST_ASSERT_EQUAL_STRING("Refreshed proto text\r\n", fixture_protos[0].action_description);
  TEST_ASSERT_TRUE(paper->action_description != fixture_protos[0].action_description);
  TEST_ASSERT_NOT_NULL(strstr(paper->action_description, "Saved note addition"));

  unlink_descriptor(&d);
  free_test_char(ch);
}

static void test_note_save_and_abort_after_extract_obj(void)
{
  struct descriptor_data d;
  struct char_data *ch;
  struct obj_data *paper1 = NULL, *paper2 = NULL;
  char arg[MAX_INPUT_LENGTH];

  memset(&d, 0, sizeof(d));
  ch = make_test_char("Writer", 10, 1003, &d);

  /* Case 1: save after paper extracted */
  attach_pen_and_paper(ch, "Initial note\r\n", &paper1);
  snprintf(arg, sizeof(arg), "paper");
  do_write(ch, arg, 0, 0);
  send_editor_line(&d, "Orphaned draft");
  extract_obj(paper1);
  TEST_ASSERT_NULL(d.note_obj);
  send_editor_line(&d, "/s");
  TEST_ASSERT_NULL(d.str);
  TEST_ASSERT_NULL(d.note_buf);
  TEST_ASSERT_FALSE(PLR_FLAGGED(ch, PLR_WRITING));

  /* Case 2: abort after paper extracted */
  extract_obj(unequip_char(ch, WEAR_HOLD));
  attach_pen_and_paper(ch, "Initial note\r\n", &paper2);
  snprintf(arg, sizeof(arg), "paper");
  do_write(ch, arg, 0, 0);
  send_editor_line(&d, "Orphaned abort");
  extract_obj(paper2);
  TEST_ASSERT_NULL(d.note_obj);
  send_editor_line(&d, "/a");
  TEST_ASSERT_NULL(d.str);
  TEST_ASSERT_NULL(d.note_buf);
  TEST_ASSERT_FALSE(PLR_FLAGGED(ch, PLR_WRITING));

  unlink_descriptor(&d);
  free_test_char(ch);
}

static void test_disconnected_ibt_writer_aborted_before_save_and_renumbers_editor(void)
{
  struct descriptor_data d1, d2, d_imm;
  struct char_data *ch1, *ch2, *imm;
  char cmd1[MAX_INPUT_LENGTH], cmd2[MAX_INPUT_LENGTH];

  memset(&d1, 0, sizeof(d1));
  memset(&d2, 0, sizeof(d2));
  memset(&d_imm, 0, sizeof(d_imm));

  ch1 = make_test_char("AuthorOne", 10, 2001, &d1);
  ch2 = make_test_char("AuthorTwo", 10, 2002, &d2);
  imm = make_test_char("ImmEditor", LVL_IMPL, 2003, &d_imm);

  d_imm.connected = CON_IBTEDIT;
  d_imm.olc = calloc(1, sizeof(*d_imm.olc));
  TEST_ASSERT_NOT_NULL(d_imm.olc);
  OLC_VAL(&d_imm) = SCMD_BUG;
  OLC_NUM(&d_imm) = 2;

  /* Author 1 opens bug #1 and disconnects before finishing */
  snprintf(cmd1, sizeof(cmd1), "submit First unfinished bug");
  do_ibt(ch1, cmd1, 0, SCMD_BUG);
  TEST_ASSERT_NOT_NULL(d1.str);
  send_editor_line(&d1, "Half typed line");

  /* Author 2 opens and saves bug #2 */
  snprintf(cmd2, sizeof(cmd2), "submit Second completed bug");
  do_ibt(ch2, cmd2, 0, SCMD_BUG);
  TEST_ASSERT_NOT_NULL(d2.str);
  send_editor_line(&d2, "Actual bug body");
  send_editor_line(&d2, "/s");
  TEST_ASSERT_NULL(d2.str);

  /* Verify concurrent save wrote only bug #2 and skipped in-progress bug #1 */
  {
    FILE *fp = fopen("misc/bugs", "rb");
    char filebuf[MAX_STRING_LENGTH];
    size_t n;
    TEST_ASSERT_NOT_NULL(fp);
    n = fread(filebuf, 1, sizeof(filebuf) - 1, fp);
    filebuf[n] = '\0';
    fclose(fp);
    TEST_ASSERT_NOT_NULL(strstr(filebuf, "Second completed bug"));
    TEST_ASSERT_NULL(strstr(filebuf, "First unfinished bug"));
  }

  /* Close author 1's socket path */
  TEST_ASSERT_EQUAL_INT(1, ibt_abort_write(d1.str));
  d1.str = NULL;
  unlink_descriptor(&d1);

  /* Open CON_IBTEDIT on bug #2 shifted down to #1 */
  TEST_ASSERT_EQUAL_INT(1, OLC_NUM(&d_imm));
  TEST_ASSERT_EQUAL_INT(0, ibt_abort_write(NULL));

  /* Subsequent category save/reload contains only bug #2 */
  save_ibt_file(SCMD_BUG);
  free_ibt_lists();
  load_ibt_file(SCMD_BUG);

  TEST_ASSERT_NOT_NULL(first_bug);
  TEST_ASSERT_EQUAL_PTR(first_bug, last_bug);
  TEST_ASSERT_NULL(first_bug->next);
  TEST_ASSERT_EQUAL_STRING("Second completed bug", first_bug->text);

  unlink_descriptor(&d2);
  unlink_descriptor(&d_imm);
  free(d_imm.olc);
  free_test_char(ch1);
  free_test_char(ch2);
  free_test_char(imm);
}

static void test_ibt_remove_only_blocks_matching_edited_number(void)
{
  struct descriptor_data d1, d2, d_imm;
  struct char_data *ch1, *ch2, *imm;
  char cmd[MAX_INPUT_LENGTH];

  memset(&d1, 0, sizeof(d1));
  memset(&d2, 0, sizeof(d2));
  memset(&d_imm, 0, sizeof(d_imm));

  ch1 = make_test_char("Reporter1", 10, 3001, &d1);
  ch2 = make_test_char("Reporter2", 10, 3002, &d2);
  imm = make_test_char("Admin", LVL_IMPL, 3003, &d_imm);

  snprintf(cmd, sizeof(cmd), "submit Bug one");
  do_ibt(ch1, cmd, 0, SCMD_BUG);
  send_editor_line(&d1, "Body one");
  send_editor_line(&d1, "/s");

  snprintf(cmd, sizeof(cmd), "submit Bug two");
  do_ibt(ch2, cmd, 0, SCMD_BUG);
  send_editor_line(&d2, "Body two");
  send_editor_line(&d2, "/s");

  d_imm.connected = CON_IBTEDIT;
  d_imm.olc = calloc(1, sizeof(*d_imm.olc));
  TEST_ASSERT_NOT_NULL(d_imm.olc);
  OLC_VAL(&d_imm) = SCMD_BUG;
  OLC_NUM(&d_imm) = 1;

  /* Removing the currently edited entry #1 is blocked */
  snprintf(cmd, sizeof(cmd), "remove 1");
  do_ibt(imm, cmd, 0, SCMD_BUG);
  TEST_ASSERT_NOT_NULL(first_bug);
  TEST_ASSERT_NOT_NULL(first_bug->next);

  /* Removing #2 succeeds while #1 is open in CON_IBTEDIT */
  snprintf(cmd, sizeof(cmd), "remove 2");
  do_ibt(imm, cmd, 0, SCMD_BUG);
  TEST_ASSERT_NOT_NULL(first_bug);
  TEST_ASSERT_EQUAL_PTR(first_bug, last_bug);
  TEST_ASSERT_NULL(first_bug->next);

  unlink_descriptor(&d1);
  unlink_descriptor(&d2);
  unlink_descriptor(&d_imm);
  free(d_imm.olc);
  free_test_char(ch1);
  free_test_char(ch2);
  free_test_char(imm);
}

static void test_login_clears_report_flags_and_note_abort_ignores_stale_flags(void)
{
  struct descriptor_data d_login, d_note, d_bug;
  struct char_data *ch_note, *ch_bug;
  struct obj_data *paper = NULL;
  char input[MAX_INPUT_LENGTH];

  /* Part A: CON_GET_NAME clears stale writing/mail/report flags on load */
  memset(&d_login, 0, sizeof(d_login));
  snprintf(d_login.host, sizeof(d_login.host), "localhost");
  STATE(&d_login) = CON_GET_NAME;
  hook_load_char = 1;
  SET_BIT_AR(hook_plr_flags, PLR_WRITING);
  SET_BIT_AR(hook_plr_flags, PLR_MAILING);
  SET_BIT_AR(hook_plr_flags, PLR_BUG);
  SET_BIT_AR(hook_plr_flags, PLR_IDEA);
  SET_BIT_AR(hook_plr_flags, PLR_TYPO);

  snprintf(input, sizeof(input), "Tester");
  nanny(&d_login, input);

  TEST_ASSERT_EQUAL_INT(CON_PASSWORD, STATE(&d_login));
  TEST_ASSERT_NOT_NULL(d_login.character);
  TEST_ASSERT_FALSE(PLR_FLAGGED(d_login.character, PLR_WRITING));
  TEST_ASSERT_FALSE(PLR_FLAGGED(d_login.character, PLR_MAILING));
  TEST_ASSERT_FALSE(PLR_FLAGGED(d_login.character, PLR_BUG));
  TEST_ASSERT_FALSE(PLR_FLAGGED(d_login.character, PLR_IDEA));
  TEST_ASSERT_FALSE(PLR_FLAGGED(d_login.character, PLR_TYPO));
  free_test_char(d_login.character);
  d_login.character = NULL;
  hook_load_char = 0;

  /* Part B: stale report flag set while writing a note still aborts as note */
  memset(&d_bug, 0, sizeof(d_bug));
  memset(&d_note, 0, sizeof(d_note));
  ch_bug = make_test_char("Bugger", 10, 4001, &d_bug);
  snprintf(input, sizeof(input), "submit Existing bug");
  do_ibt(ch_bug, input, 0, SCMD_BUG);
  send_editor_line(&d_bug, "Existing body");
  send_editor_line(&d_bug, "/s");

  ch_note = make_test_char("NoteWriter", 10, 4002, &d_note);
  attach_pen_and_paper(ch_note, "Original paper text\r\n", &paper);
  snprintf(input, sizeof(input), "paper");
  do_write(ch_note, input, 0, 0);
  send_editor_line(&d_note, "Cancelled edit");
  SET_BIT_AR(PLR_FLAGS(ch_note), PLR_BUG);
  send_editor_line(&d_note, "/a");

  TEST_ASSERT_NULL(d_note.str);
  TEST_ASSERT_NULL(d_note.note_buf);
  TEST_ASSERT_FALSE(PLR_FLAGGED(ch_note, PLR_BUG));
  TEST_ASSERT_EQUAL_STRING("Original paper text\r\n", paper->action_description);
  TEST_ASSERT_NOT_NULL(first_bug);

  unlink_descriptor(&d_bug);
  unlink_descriptor(&d_note);
  free_test_char(ch_bug);
  free_test_char(ch_note);
}

static void test_note_save_rejects_transformed_or_transferred_paper(void)
{
  struct descriptor_data d1, d2;
  struct char_data *ch1, *ch2;
  struct obj_data *paper1 = NULL, *paper2 = NULL;
  char arg[MAX_INPUT_LENGTH];

  memset(&d1, 0, sizeof(d1));
  memset(&d2, 0, sizeof(d2));
  ch1 = make_test_char("Writer1", 10, 5001, &d1);
  ch2 = make_test_char("Holder2", 10, 5002, &d2);

  /* Case 1: in-place transformation away from ITEM_NOTE */
  attach_pen_and_paper(ch1, "Original note\r\n", &paper1);
  snprintf(arg, sizeof(arg), "paper");
  do_write(ch1, arg, 0, 0);
  send_editor_line(&d1, "Transformed draft");
  GET_OBJ_TYPE(paper1) = ITEM_OTHER;
  REMOVE_BIT_AR(PLR_FLAGS(ch1), PLR_CRASH);
  send_editor_line(&d1, "/s");

  TEST_ASSERT_NULL(d1.str);
  TEST_ASSERT_NULL(d1.note_buf);
  TEST_ASSERT_NULL(d1.note_obj);
  TEST_ASSERT_EQUAL_STRING("Original note\r\n", paper1->action_description);
  TEST_ASSERT_FALSE(PLR_FLAGGED(ch1, PLR_CRASH));

  /* Case 2: paper transferred to another holder mid-edit */
  extract_obj(paper1);
  extract_obj(unequip_char(ch1, WEAR_HOLD));
  attach_pen_and_paper(ch1, "Original note\r\n", &paper2);
  snprintf(arg, sizeof(arg), "paper");
  do_write(ch1, arg, 0, 0);
  send_editor_line(&d1, "Transferred draft");
  obj_from_char(paper2);
  obj_to_char(paper2, ch2);
  REMOVE_BIT_AR(PLR_FLAGS(ch1), PLR_CRASH);
  REMOVE_BIT_AR(PLR_FLAGS(ch2), PLR_CRASH);
  send_editor_line(&d1, "/s");

  TEST_ASSERT_NULL(d1.str);
  TEST_ASSERT_NULL(d1.note_buf);
  TEST_ASSERT_NULL(d1.note_obj);
  TEST_ASSERT_EQUAL_STRING("Original note\r\n", paper2->action_description);
  TEST_ASSERT_FALSE(PLR_FLAGGED(ch1, PLR_CRASH));
  TEST_ASSERT_FALSE(PLR_FLAGGED(ch2, PLR_CRASH));

  unlink_descriptor(&d1);
  unlink_descriptor(&d2);
  free_test_char(ch1);
  free_test_char(ch2);
}

static void test_extract_char_aborts_editor_and_dismisses_pager(void)
{
  struct descriptor_data d;
  struct char_data *ch;
  char cmd[MAX_INPUT_LENGTH];
  int i;

  memset(&d, 0, sizeof(d));
  ch = make_test_char("PagerVictim", 10, 5001, &d);
  GET_PAGE_LENGTH(ch) = 10;

  snprintf(cmd, sizeof(cmd), "submit Multi-page draft bug");
  do_ibt(ch, cmd, 0, SCMD_BUG);
  TEST_ASSERT_NOT_NULL(d.str);
  for (i = 0; i < 25; i++) {
    char line[64];
    snprintf(line, sizeof(line), "Report line %d", i + 1);
    send_editor_line(&d, line);
  }

  /* Open /l pager inside the editor */
  send_editor_line(&d, "/l");
  TEST_ASSERT_TRUE(d.showstr_count > 1);
  TEST_ASSERT_NOT_NULL(d.showstr_vector);
  TEST_ASSERT_NOT_NULL(d.showstr_head);

  extract_char_final(ch);

  TEST_ASSERT_NULL(d.str);
  TEST_ASSERT_NULL(first_bug);
  TEST_ASSERT_FALSE(PLR_FLAGGED(ch, PLR_BUG));
  TEST_ASSERT_FALSE(PLR_FLAGGED(ch, PLR_WRITING));
  TEST_ASSERT_EQUAL_INT(0, d.showstr_count);
  TEST_ASSERT_EQUAL_INT(0, d.showstr_page);
  TEST_ASSERT_NULL(d.showstr_vector);
  TEST_ASSERT_NULL(d.showstr_head);
  TEST_ASSERT_EQUAL_INT(CON_MENU, STATE(&d));

  unlink_descriptor(&d);
  free_test_char(ch);
}

static void test_extract_switched_prefedit_and_switched_original_body(void)
{
  struct descriptor_data d_imp, d_vict, d_imp2, d_rvict;
  struct char_data *imp, *vict, *vict2, *imp2, *rvict;
  char arg[MAX_INPUT_LENGTH];

  /* Case A: controlled player extracted during prefedit */
  memset(&d_imp, 0, sizeof(d_imp));
  memset(&d_vict, 0, sizeof(d_vict));
  imp = make_test_char("ImpOne", LVL_IMPL, 6001, &d_imp);
  vict = make_test_char("ControlledPC", 10, 6002, &d_vict);
  /* Make vict linkless so do_switch can possess vict */
  unlink_descriptor(&d_vict);
  vict->desc = NULL;

  snprintf(arg, sizeof(arg), "ControlledPC");
  do_switch(imp, arg, 0, 0);
  TEST_ASSERT_EQUAL_PTR(vict, d_imp.character);
  TEST_ASSERT_EQUAL_PTR(imp, d_imp.original);

  arg[0] = '\0';
  do_oasis_prefedit(vict, arg, 0, 0);
  TEST_ASSERT_EQUAL_INT(CON_PREFEDIT, STATE(&d_imp));
  TEST_ASSERT_NOT_NULL(d_imp.olc);
  TEST_ASSERT_EQUAL_PTR(vict, OLC_PREFS(&d_imp)->ch);

  extract_char_final(vict);

  TEST_ASSERT_NULL(d_imp.olc);
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(&d_imp));
  TEST_ASSERT_EQUAL_PTR(imp, d_imp.character);
  TEST_ASSERT_NULL(d_imp.original);
  TEST_ASSERT_EQUAL_PTR(&d_imp, imp->desc);
  TEST_ASSERT_NULL(vict->desc);

  /* Case B: implementor's original body extracted while switched body writes IBT */
  vict2 = make_test_char("SwitchedWriter", 10, 6003, &d_vict);
  unlink_descriptor(&d_vict);
  vict2->desc = NULL;

  snprintf(arg, sizeof(arg), "SwitchedWriter");
  do_switch(imp, arg, 0, 0);
  TEST_ASSERT_EQUAL_PTR(vict2, d_imp.character);
  TEST_ASSERT_EQUAL_PTR(imp, d_imp.original);

  snprintf(arg, sizeof(arg), "submit Unfinished switched bug");
  do_ibt(vict2, arg, 0, SCMD_BUG);
  send_editor_line(&d_imp, "Draft body from switched writer");
  TEST_ASSERT_NOT_NULL(first_bug);
  TEST_ASSERT_TRUE(PLR_FLAGGED(vict2, PLR_BUG));

  extract_char_final(imp);

  TEST_ASSERT_NULL(first_bug);
  TEST_ASSERT_NULL(d_imp.str);
  TEST_ASSERT_FALSE(PLR_FLAGGED(vict2, PLR_BUG));
  TEST_ASSERT_FALSE(PLR_FLAGGED(vict2, PLR_WRITING));
  TEST_ASSERT_NULL(vict2->desc);
  TEST_ASSERT_EQUAL_INT(CON_MENU, STATE(&d_imp));

  /* Case C: remote prefedit <player> without switch closes when target extracts */
  memset(&d_imp2, 0, sizeof(d_imp2));
  memset(&d_rvict, 0, sizeof(d_rvict));
  imp2 = make_test_char("ImpTwo", LVL_IMPL, 6004, &d_imp2);
  rvict = make_test_char("RemoteVict", 10, 6005, &d_rvict);

  snprintf(arg, sizeof(arg), "RemoteVict");
  do_oasis_prefedit(imp2, arg, 0, 0);
  TEST_ASSERT_EQUAL_INT(CON_PREFEDIT, STATE(&d_imp2));
  TEST_ASSERT_NOT_NULL(d_imp2.olc);

  extract_char_final(rvict);

  TEST_ASSERT_NULL(d_imp2.olc);
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(&d_imp2));

  unlink_descriptor(&d_imp);
  unlink_descriptor(&d_imp2);
  unlink_descriptor(&d_rvict);
  free_test_char(imp);
  free_test_char(vict);
  free_test_char(vict2);
  free_test_char(imp2);
  free_test_char(rvict);
}

static void test_player_purge_and_idle_rent_cleanup_editors_and_olc(void)
{
  struct descriptor_data d_imp, d_mail, d_olc, d_idle1, d_idle2;
  struct char_data *imp, *p_mail, *p_olc, *p_idle1, *p_idle2;
  char **mail_slot;
  char arg[MAX_INPUT_LENGTH];
  int i;

  memset(&d_imp, 0, sizeof(d_imp));
  memset(&d_mail, 0, sizeof(d_mail));
  memset(&d_olc, 0, sizeof(d_olc));
  imp = make_test_char("PurgerImp", LVL_IMPL, 7001, &d_imp);
  p_mail = make_test_char("MailPurge", 10, 7002, &d_mail);
  p_olc = make_test_char("OlcPurge", 10, 7003, &d_olc);

  /* Case A1: purge player mid-mail with active pager */
  GET_PAGE_LENGTH(p_mail) = 6;
  SET_BIT_AR(PLR_FLAGS(p_mail), PLR_MAILING);
  mail_slot = calloc(1, sizeof(char *));
  TEST_ASSERT_NOT_NULL(mail_slot);
  string_write(&d_mail, mail_slot, MAX_MAIL_SIZE, 999, NULL);
  for (i = 0; i < 15; i++)
    send_editor_line(&d_mail, "Unsent mail line");
  send_editor_line(&d_mail, "/l");
  TEST_ASSERT_TRUE(d_mail.showstr_count > 1);

  snprintf(arg, sizeof(arg), "MailPurge");
  do_purge(imp, arg, 0, 0);
  extract_pending_chars();

  TEST_ASSERT_NULL(d_mail.str);
  TEST_ASSERT_EQUAL_INT(0, d_mail.showstr_count);
  TEST_ASSERT_NULL(d_mail.showstr_vector);
  TEST_ASSERT_NULL(d_mail.showstr_head);
  TEST_ASSERT_EQUAL_INT(CON_CLOSE, STATE(&d_mail));
  TEST_ASSERT_NULL(d_mail.character);

  /* Case A2: purge player in CON_PREFEDIT */
  arg[0] = '\0';
  do_oasis_prefedit(p_olc, arg, 0, 0);
  TEST_ASSERT_NOT_NULL(d_olc.olc);
  TEST_ASSERT_EQUAL_INT(CON_PREFEDIT, STATE(&d_olc));

  snprintf(arg, sizeof(arg), "OlcPurge");
  do_purge(imp, arg, 0, 0);
  extract_pending_chars();

  TEST_ASSERT_NULL(d_olc.olc);
  TEST_ASSERT_EQUAL_INT(CON_CLOSE, STATE(&d_olc));
  TEST_ASSERT_NULL(d_olc.character);

  /* Case B: idle rent via point_update() cleans up active IBT+pager and OLC */
  memset(&d_idle1, 0, sizeof(d_idle1));
  memset(&d_idle2, 0, sizeof(d_idle2));
  p_idle1 = make_test_char("IdleWriter", 10, 7004, &d_idle1);
  p_idle2 = make_test_char("IdlePrefs", 10, 7005, &d_idle2);

  GET_PAGE_LENGTH(p_idle1) = 6;
  snprintf(arg, sizeof(arg), "submit Idle unfinished bug");
  do_ibt(p_idle1, arg, 0, SCMD_BUG);
  for (i = 0; i < 15; i++)
    send_editor_line(&d_idle1, "Idle bug line");
  send_editor_line(&d_idle1, "/l");
  TEST_ASSERT_NOT_NULL(first_bug);
  TEST_ASSERT_TRUE(d_idle1.showstr_count > 1);

  arg[0] = '\0';
  do_oasis_prefedit(p_idle2, arg, 0, 0);
  TEST_ASSERT_NOT_NULL(d_idle2.olc);

  GET_WAS_IN(p_idle1) = 0;
  p_idle1->char_specials.timer = CONFIG_IDLE_RENT_TIME + 1;
  GET_WAS_IN(p_idle2) = 0;
  p_idle2->char_specials.timer = CONFIG_IDLE_RENT_TIME + 1;

  point_update();
  extract_pending_chars();

  TEST_ASSERT_NULL(first_bug);
  TEST_ASSERT_NULL(d_idle1.str);
  TEST_ASSERT_EQUAL_INT(0, d_idle1.showstr_count);
  TEST_ASSERT_EQUAL_INT(CON_DISCONNECT, STATE(&d_idle1));
  TEST_ASSERT_NULL(d_idle1.character);
  TEST_ASSERT_NULL(d_idle2.olc);
  TEST_ASSERT_EQUAL_INT(CON_DISCONNECT, STATE(&d_idle2));
  TEST_ASSERT_NULL(d_idle2.character);

  unlink_descriptor(&d_imp);
  unlink_descriptor(&d_mail);
  unlink_descriptor(&d_olc);
  unlink_descriptor(&d_idle1);
  unlink_descriptor(&d_idle2);
  free_test_char(imp);
  free_test_char(p_mail);
  free_test_char(p_olc);
  free_test_char(p_idle1);
  free_test_char(p_idle2);
}

static void test_tedit_private_draft_concurrent_save_and_basic_editor_extraction(void)
{
  struct descriptor_data d1, d2, d3;
  struct char_data *b1, *b2, *b3;
  char arg[MAX_INPUT_LENGTH];
  FILE *fp;
  char diskbuf[256];
  size_t n;

  memset(&d1, 0, sizeof(d1));
  memset(&d2, 0, sizeof(d2));
  memset(&d3, 0, sizeof(d3));
  b1 = make_test_char("BuilderOne", LVL_IMPL, 8001, &d1);
  b2 = make_test_char("BuilderTwo", LVL_IMPL, 8002, &d2);
  b3 = make_test_char("BuilderThree", LVL_IMPL, 8003, &d3);

  motd = strdup("Initial MOTD\r\n");

  /* Builder 1 opens MOTD and edits draft; live MOTD must stay untouched */
  snprintf(arg, sizeof(arg), "motd");
  do_tedit(b1, arg, 0, 0);
  TEST_ASSERT_EQUAL_INT(CON_TEDIT, STATE(&d1));
  send_editor_line(&d1, "Builder 1 unsaved line");
  TEST_ASSERT_EQUAL_STRING("Initial MOTD\r\n", motd);

  /* Builder 2 opens MOTD, replaces content, and saves */
  snprintf(arg, sizeof(arg), "motd");
  do_tedit(b2, arg, 0, 0);
  TEST_ASSERT_EQUAL_INT(CON_TEDIT, STATE(&d2));
  send_editor_line(&d2, "/c");
  send_editor_line(&d2, "Builder 2 saved MOTD");
  send_editor_line(&d2, "/s");
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(&d2));
  TEST_ASSERT_NULL(d2.olc);
  TEST_ASSERT_EQUAL_STRING("Builder 2 saved MOTD\r\n", motd);

  fp = fopen("text/motd", "r");
  TEST_ASSERT_NOT_NULL(fp);
  n = fread(diskbuf, 1, sizeof(diskbuf) - 1, fp);
  diskbuf[n] = '\0';
  fclose(fp);
  TEST_ASSERT_EQUAL_STRING("Builder 2 saved MOTD\n", diskbuf);

  /* Extracting Builder 1 must NOT roll live MOTD back to Initial MOTD */
  extract_char_final(b1);
  TEST_ASSERT_EQUAL_INT(CON_MENU, STATE(&d1));
  TEST_ASSERT_NULL(d1.olc);
  TEST_ASSERT_NULL(d1.str);
  TEST_ASSERT_EQUAL_STRING("Builder 2 saved MOTD\r\n", motd);

  /* Extracting a builder who typed nothing (with NULL backstr, as in basic
   * editor mode) must keep the live MOTD intact. */
  snprintf(arg, sizeof(arg), "motd");
  do_tedit(b3, arg, 0, 0);
  TEST_ASSERT_EQUAL_INT(CON_TEDIT, STATE(&d3));
  TEST_ASSERT_NULL(d3.backstr);

  extract_char_final(b3);
  TEST_ASSERT_EQUAL_INT(CON_MENU, STATE(&d3));
  TEST_ASSERT_NULL(d3.olc);
  TEST_ASSERT_NULL(d3.str);
  TEST_ASSERT_NOT_NULL(motd);
  TEST_ASSERT_EQUAL_STRING("Builder 2 saved MOTD\r\n", motd);

  unlink_descriptor(&d1);
  unlink_descriptor(&d2);
  unlink_descriptor(&d3);
  free_test_char(b1);
  free_test_char(b2);
  free_test_char(b3);
}

static void test_tedit_save_preserves_pager_byte_offsets_after_pagelength_change(void)
{
  struct descriptor_data d_reader, d_builder;
  struct char_data *reader, *builder;
  char old_buf[2048];
  char arg[MAX_INPUT_LENGTH], empty_in[4];
  ptrdiff_t offsets[32];
  int orig_count, orig_page, i;
  size_t old_len;

  memset(&d_reader, 0, sizeof(d_reader));
  memset(&d_builder, 0, sizeof(d_builder));
  reader = make_test_char("MotdReader", 10, 9001, &d_reader);
  builder = make_test_char("MotdBuilder", LVL_IMPL, 9002, &d_builder);

  old_buf[0] = '\0';
  for (i = 0; i < 30; i++) {
    char line[64];
    snprintf(line, sizeof(line), "Old MOTD line %02d\r\n", i + 1);
    strcat(old_buf, line);
  }
  motd = strdup(old_buf);
  old_len = strlen(motd);

  /* Reader starts paging motd in-place (keep_internal == 0) at page_length 6 */
  GET_PAGE_LENGTH(reader) = 6;
  page_string(&d_reader, motd, 0);
  TEST_ASSERT_TRUE(d_reader.showstr_count >= 4);
  TEST_ASSERT_TRUE(d_reader.showstr_count < 32);
  TEST_ASSERT_NULL(d_reader.showstr_head);
  TEST_ASSERT_EQUAL_PTR(motd, d_reader.showstr_vector[0]);

  /* Advance one page and record every page pointer's byte offset */
  empty_in[0] = '\0';
  show_string(&d_reader, empty_in);
  orig_count = d_reader.showstr_count;
  orig_page = d_reader.showstr_page;
  for (i = 0; i < orig_count; i++) {
    TEST_ASSERT_NOT_NULL(d_reader.showstr_vector[i]);
    offsets[i] = d_reader.showstr_vector[i] - motd;
    TEST_ASSERT_TRUE(offsets[i] >= 0 && (size_t)offsets[i] <= old_len);
  }

  /* Reader increases page length mid-read so recomputing pages would leave
   * trailing showstr_vector entries pointing into freed old motd. */
  GET_PAGE_LENGTH(reader) = 50;

  snprintf(arg, sizeof(arg), "motd");
  do_tedit(builder, arg, 0, 0);
  send_editor_line(&d_builder, "/c");
  send_editor_line(&d_builder, "Brand new short MOTD");
  send_editor_line(&d_builder, "/s");

  TEST_ASSERT_NOT_NULL(d_reader.showstr_head);
  TEST_ASSERT_EQUAL_STRING(old_buf, d_reader.showstr_head);
  TEST_ASSERT_EQUAL_INT(orig_count, d_reader.showstr_count);
  TEST_ASSERT_EQUAL_INT(orig_page, d_reader.showstr_page);
  for (i = 0; i < orig_count; i++) {
    TEST_ASSERT_EQUAL_PTR(d_reader.showstr_head + offsets[i], d_reader.showstr_vector[i]);
  }

  /* Stepping through remaining pages succeeds cleanly and dismisses at end */
  while (d_reader.showstr_count > 0) {
    empty_in[0] = '\0';
    show_string(&d_reader, empty_in);
  }
  TEST_ASSERT_NULL(d_reader.showstr_head);
  TEST_ASSERT_NULL(d_reader.showstr_vector);

  unlink_descriptor(&d_reader);
  unlink_descriptor(&d_builder);
  free_test_char(reader);
  free_test_char(builder);
}

static void test_extract_occupied_original_body_resolves_both_descriptors(void)
{
  struct descriptor_data d_owner, d_occ, d_mob;
  struct char_data *owner, *occupant, *mob;
  char arg[MAX_INPUT_LENGTH];

  memset(&d_owner, 0, sizeof(d_owner));
  memset(&d_occ, 0, sizeof(d_occ));
  memset(&d_mob, 0, sizeof(d_mob));

  owner = make_test_char("OrigOwner", LVL_IMPL, 9101, &d_owner);
  occupant = make_test_char("BodyOccupant", LVL_IMPL, 9102, &d_occ);
  mob = make_test_char("TargetBody", 10, 9103, &d_mob);
  unlink_descriptor(&d_mob);
  mob->desc = NULL;

  /* Owner switches into mob, leaving owner->desc == NULL */
  snprintf(arg, sizeof(arg), "TargetBody");
  do_switch(owner, arg, 0, 0);
  TEST_ASSERT_EQUAL_PTR(mob, d_owner.character);
  TEST_ASSERT_EQUAL_PTR(owner, d_owner.original);
  TEST_ASSERT_NULL(owner->desc);

  /* Occupant switches into owner's unoccupied original body */
  snprintf(arg, sizeof(arg), "OrigOwner");
  do_switch(occupant, arg, 0, 0);
  TEST_ASSERT_EQUAL_PTR(&d_occ, owner->desc);
  TEST_ASSERT_EQUAL_PTR(owner, d_occ.character);
  TEST_ASSERT_EQUAL_PTR(occupant, d_occ.original);
  TEST_ASSERT_NULL(occupant->desc);

  /* Put both descriptors into active editors/OLC before extracting owner */
  arg[0] = '\0';
  do_oasis_prefedit(owner, arg, 0, 0);
  TEST_ASSERT_EQUAL_INT(CON_PREFEDIT, STATE(&d_occ));

  snprintf(arg, sizeof(arg), "submit Owner switched bug");
  do_ibt(mob, arg, 0, SCMD_BUG);
  send_editor_line(&d_owner, "Unfinished body");
  TEST_ASSERT_NOT_NULL(first_bug);

  free_char_count = 0;
  extract_char_final(owner);

  /* Occupant returned cleanly to their own body */
  TEST_ASSERT_NULL(d_occ.olc);
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(&d_occ));
  TEST_ASSERT_EQUAL_PTR(occupant, d_occ.character);
  TEST_ASSERT_NULL(d_occ.original);
  TEST_ASSERT_EQUAL_PTR(&d_occ, occupant->desc);

  /* Owner's switched draft was aborted, owner restored to d_owner at CON_MENU,
   * and owner was NOT freed while d_owner still references it. */
  TEST_ASSERT_NULL(first_bug);
  TEST_ASSERT_NULL(d_owner.str);
  TEST_ASSERT_NULL(d_owner.original);
  TEST_ASSERT_EQUAL_PTR(owner, d_owner.character);
  TEST_ASSERT_EQUAL_PTR(&d_owner, owner->desc);
  TEST_ASSERT_NULL(mob->desc);
  TEST_ASSERT_EQUAL_INT(CON_MENU, STATE(&d_owner));
  TEST_ASSERT_EQUAL_INT(0, free_char_count);

  unlink_descriptor(&d_owner);
  unlink_descriptor(&d_occ);
  free_test_char(owner);
  free_test_char(occupant);
  free_test_char(mob);
}

static void test_tedit_detects_stale_draft_conflict_and_keeps_draft(void)
{
  struct descriptor_data d1, d2;
  struct char_data *b1, *b2;
  char arg[MAX_INPUT_LENGTH];
  char diskbuf[256];
  FILE *fp;
  size_t n;

  memset(&d1, 0, sizeof(d1));
  memset(&d2, 0, sizeof(d2));
  b1 = make_test_char("FirstSaver", LVL_IMPL, 9201, &d1);
  b2 = make_test_char("StaleSaver", LVL_IMPL, 9202, &d2);

  motd = strdup("Baseline MOTD\r\n");

  /* Both builders open MOTD from the same baseline */
  snprintf(arg, sizeof(arg), "motd");
  do_tedit(b1, arg, 0, 0);
  snprintf(arg, sizeof(arg), "motd");
  do_tedit(b2, arg, 0, 0);

  /* First builder saves an update */
  send_editor_line(&d1, "/c");
  send_editor_line(&d1, "First builder saved MOTD");
  send_editor_line(&d1, "/s");
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(&d1));
  TEST_ASSERT_EQUAL_STRING("First builder saved MOTD\r\n", motd);

  /* Second builder tries to save untouched stale draft: must be refused,
   * preserving first builder's save on disk/memory and keeping b2's draft. */
  send_editor_line(&d2, "/s");
  TEST_ASSERT_EQUAL_INT(CON_TEDIT, STATE(&d2));
  TEST_ASSERT_NOT_NULL(d2.olc);
  TEST_ASSERT_NOT_NULL(d2.str);
  TEST_ASSERT_NOT_NULL(*d2.str);
  TEST_ASSERT_EQUAL_STRING("Baseline MOTD\r\n", *d2.str);
  TEST_ASSERT_TRUE(PLR_FLAGGED(b2, PLR_WRITING));
  TEST_ASSERT_EQUAL_STRING("First builder saved MOTD\r\n", motd);

  fp = fopen("text/motd", "r");
  TEST_ASSERT_NOT_NULL(fp);
  n = fread(diskbuf, 1, sizeof(diskbuf) - 1, fp);
  diskbuf[n] = '\0';
  fclose(fp);
  TEST_ASSERT_EQUAL_STRING("First builder saved MOTD\n", diskbuf);

  /* Second builder can still edit their kept draft and cleanly abort */
  send_editor_line(&d2, "Second builder extra line");
  TEST_ASSERT_NOT_NULL(strstr(*d2.str, "Second builder extra line"));
  send_editor_line(&d2, "/a");
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(&d2));
  TEST_ASSERT_NULL(d2.olc);
  TEST_ASSERT_NULL(d2.str);
  TEST_ASSERT_EQUAL_STRING("First builder saved MOTD\r\n", motd);

  unlink_descriptor(&d1);
  unlink_descriptor(&d2);
  free_test_char(b1);
  free_test_char(b2);
}

static void test_tedit_confirms_disk_write_and_rename_before_publishing(void)
{
  struct descriptor_data d;
  struct char_data *b;
  char arg[MAX_INPUT_LENGTH];
  char diskbuf[256];
  FILE *fp;
  size_t n;

  memset(&d, 0, sizeof(d));
  b = make_test_char("DiskBuilder", LVL_IMPL, 9301, &d);

  /* Seed existing MOTD both in memory and on disk */
  motd = strdup("Existing MOTD on disk\r\n");
  fp = fopen("text/motd", "w");
  TEST_ASSERT_NOT_NULL(fp);
  fputs("Existing MOTD on disk\n", fp);
  fclose(fp);

  snprintf(arg, sizeof(arg), "motd");
  do_tedit(b, arg, 0, 0);
  send_editor_line(&d, "/c");
  send_editor_line(&d, "Updated MOTD text requiring disk confirmation");

  /* 1. Block temp file creation by placing a directory at text/motd.tmp */
  TEST_ASSERT_EQUAL_INT(0, test_mkdir("text/motd.tmp"));
  send_editor_line(&d, "/s");
  TEST_ASSERT_EQUAL_INT(0, test_rmdir("text/motd.tmp"));

  TEST_ASSERT_EQUAL_INT(CON_TEDIT, STATE(&d));
  TEST_ASSERT_NOT_NULL(d.str);
  TEST_ASSERT_EQUAL_STRING("Updated MOTD text requiring disk confirmation\r\n", *d.str);
  TEST_ASSERT_EQUAL_STRING("Existing MOTD on disk\r\n", motd);
  fp = fopen("text/motd", "r");
  TEST_ASSERT_NOT_NULL(fp);
  n = fread(diskbuf, 1, sizeof(diskbuf) - 1, fp);
  diskbuf[n] = '\0';
  fclose(fp);
  TEST_ASSERT_EQUAL_STRING("Existing MOTD on disk\n", diskbuf);

  /* 2. Verify existing file survives a failed genolc_install_file replacement */
#if defined(CIRCLE_WINDOWS) || defined(_WIN32)
  {
    HANDLE hScratch, hDest;
    fp = fopen("text/locked_scratch.tmp", "w");
    TEST_ASSERT_NOT_NULL(fp);
    fputs("Replacement scratch\n", fp);
    fclose(fp);

    hScratch = CreateFileA("text/locked_scratch.tmp", GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    TEST_ASSERT_TRUE(hScratch != INVALID_HANDLE_VALUE);
    TEST_ASSERT_FALSE(genolc_install_file("text/locked_scratch.tmp", "text/motd"));
    CloseHandle(hScratch);
    (void)remove("text/locked_scratch.tmp");

    fp = fopen("text/motd", "r");
    TEST_ASSERT_NOT_NULL(fp);
    n = fread(diskbuf, 1, sizeof(diskbuf) - 1, fp);
    diskbuf[n] = '\0';
    fclose(fp);
    TEST_ASSERT_EQUAL_STRING("Existing MOTD on disk\n", diskbuf);

    /* Also verify end-to-end tedit /s preserves locked existing destination */
    hDest = CreateFileA("text/motd", GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    TEST_ASSERT_TRUE(hDest != INVALID_HANDLE_VALUE);
    send_editor_line(&d, "/s");
    CloseHandle(hDest);

    TEST_ASSERT_EQUAL_INT(CON_TEDIT, STATE(&d));
    TEST_ASSERT_NOT_NULL(d.str);
    TEST_ASSERT_NULL(fopen("text/motd.tmp", "r"));
    fp = fopen("text/motd", "r");
    TEST_ASSERT_NOT_NULL(fp);
    n = fread(diskbuf, 1, sizeof(diskbuf) - 1, fp);
    diskbuf[n] = '\0';
    fclose(fp);
    TEST_ASSERT_EQUAL_STRING("Existing MOTD on disk\n", diskbuf);
  }
#else
  if (geteuid() != 0) {
    TEST_ASSERT_EQUAL_INT(0, test_mkdir("text/ro_src"));
    fp = fopen("text/ro_src/locked_scratch.tmp", "w");
    TEST_ASSERT_NOT_NULL(fp);
    fputs("Replacement scratch\n", fp);
    fclose(fp);
    TEST_ASSERT_EQUAL_INT(0, chmod("text/ro_src", 0500));

    TEST_ASSERT_FALSE(genolc_install_file("text/ro_src/locked_scratch.tmp", "text/motd"));

    TEST_ASSERT_EQUAL_INT(0, chmod("text/ro_src", 0700));
    TEST_ASSERT_EQUAL_INT(0, remove("text/ro_src/locked_scratch.tmp"));
    TEST_ASSERT_EQUAL_INT(0, test_rmdir("text/ro_src"));

    fp = fopen("text/motd", "r");
    TEST_ASSERT_NOT_NULL(fp);
    n = fread(diskbuf, 1, sizeof(diskbuf) - 1, fp);
    diskbuf[n] = '\0';
    fclose(fp);
    TEST_ASSERT_EQUAL_STRING("Existing MOTD on disk\n", diskbuf);
  }

  /* 2b. Simulate disk-full / quota mid-write failure via RLIMIT_FSIZE */
  {
    struct rlimit old_lim, small_lim;
    void (*old_handler)(int) = signal(SIGXFSZ, SIG_IGN);
    TEST_ASSERT_EQUAL_INT(0, getrlimit(RLIMIT_FSIZE, &old_lim));
    small_lim = old_lim;
    small_lim.rlim_cur = 4;
    TEST_ASSERT_EQUAL_INT(0, setrlimit(RLIMIT_FSIZE, &small_lim));
    send_editor_line(&d, "/s");
    TEST_ASSERT_EQUAL_INT(0, setrlimit(RLIMIT_FSIZE, &old_lim));

    TEST_ASSERT_EQUAL_INT(CON_TEDIT, STATE(&d));
    TEST_ASSERT_NOT_NULL(d.str);
    TEST_ASSERT_EQUAL_STRING("Updated MOTD text requiring disk confirmation\r\n", *d.str);
    TEST_ASSERT_EQUAL_STRING("Existing MOTD on disk\r\n", motd);
    TEST_ASSERT_NULL(fopen("text/motd.tmp", "r"));

    fp = fopen("text/motd", "r");
    TEST_ASSERT_NOT_NULL(fp);
    n = fread(diskbuf, 1, sizeof(diskbuf) - 1, fp);
    diskbuf[n] = '\0';
    fclose(fp);
    TEST_ASSERT_EQUAL_STRING("Existing MOTD on disk\n", diskbuf);
    if (old_handler != SIG_ERR)
      signal(SIGXFSZ, old_handler);
  }
#endif

  /* 3. Once disk is writable, retrying /s from the preserved draft succeeds */
  send_editor_line(&d, "/s");
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(&d));
  TEST_ASSERT_NULL(d.olc);
  TEST_ASSERT_NULL(d.str);
  TEST_ASSERT_EQUAL_STRING("Updated MOTD text requiring disk confirmation\r\n", motd);

  fp = fopen("text/motd", "r");
  TEST_ASSERT_NOT_NULL(fp);
  n = fread(diskbuf, 1, sizeof(diskbuf) - 1, fp);
  diskbuf[n] = '\0';
  fclose(fp);
  TEST_ASSERT_EQUAL_STRING("Updated MOTD text requiring disk confirmation\n", diskbuf);

  /* 4. Simulate rename (genolc_install_file) failure after temp file write succeeds */
  news = strdup("Live news in memory\r\n");
  TEST_ASSERT_EQUAL_INT(0, test_mkdir("text/news"));
  fp = fopen("text/news/lock", "wb");
  TEST_ASSERT_NOT_NULL(fp);
  fclose(fp);

  snprintf(arg, sizeof(arg), "news");
  do_tedit(b, arg, 0, 0);
  send_editor_line(&d, "/c");
  send_editor_line(&d, "News draft blocked at rename");
  send_editor_line(&d, "/s");

  TEST_ASSERT_EQUAL_INT(CON_TEDIT, STATE(&d));
  TEST_ASSERT_NOT_NULL(d.str);
  TEST_ASSERT_EQUAL_STRING("News draft blocked at rename\r\n", *d.str);
  TEST_ASSERT_EQUAL_STRING("Live news in memory\r\n", news);
  TEST_ASSERT_NULL(fopen("text/news.tmp", "rb"));

  TEST_ASSERT_EQUAL_INT(0, remove("text/news/lock"));
  TEST_ASSERT_EQUAL_INT(0, test_rmdir("text/news"));
  send_editor_line(&d, "/s");
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(&d));
  TEST_ASSERT_NULL(d.olc);
  TEST_ASSERT_NULL(d.str);
  TEST_ASSERT_EQUAL_STRING("News draft blocked at rename\r\n", news);

  unlink_descriptor(&d);
  free_test_char(b);
}


static void login_duplicate_via_nanny(struct descriptor_data *d_new,
                                      const char *name, int level, long idnum)
{
  struct char_data *login_ch = calloc(1, sizeof(*login_ch));
  const char *crypted;
  char pw_in[MAX_INPUT_LENGTH];

  TEST_ASSERT_NOT_NULL(login_ch);
  login_ch->player_specials = calloc(1, sizeof(*login_ch->player_specials));
  TEST_ASSERT_NOT_NULL(login_ch->player_specials);
  login_ch->player.name = strdup(name);
  login_ch->player.level = level;
  login_ch->char_specials.saved.idnum = idnum;
  IN_ROOM(login_ch) = NOWHERE;

  crypted = CRYPT("secret", "aa");
  strncpy(GET_PASSWD(login_ch), crypted, MAX_PWD_LENGTH);
  GET_PASSWD(login_ch)[MAX_PWD_LENGTH] = '\0';

  memset(d_new, 0, sizeof(*d_new));
  snprintf(d_new->host, sizeof(d_new->host), "localhost");
  d_new->character = login_ch;
  login_ch->desc = d_new;
  STATE(d_new) = CON_PASSWORD;
  d_new->next = descriptor_list;
  descriptor_list = d_new;

  last_freed_char = NULL;
  snprintf(pw_in, sizeof(pw_in), "secret");
  nanny(d_new, pw_in);

  if (last_freed_char == login_ch) {
    if (login_ch->player.name)
      free(login_ch->player.name);
    if (GET_HOST(login_ch))
      free(GET_HOST(login_ch));
    if (login_ch->player_specials && login_ch->player_specials != &dummy_mob)
      free(login_ch->player_specials);
    free(login_ch);
    last_freed_char = NULL;
  }
}

static void test_dupe_login_closes_editors_and_usurps_olc_builder(void)
{
  struct descriptor_data d_old, d_new, d_sw, d_mob, d_re1, d_imp, d_vict, d_re2;
  struct char_data *b, *imp_sw, *mob, *imp2, *vict;
  char arg[MAX_INPUT_LENGTH];
  int i;

  /* Case A: duplicate login while builder is in CON_TEDIT with draft + pager */
  memset(&d_old, 0, sizeof(d_old));
  b = make_test_char("DupeBuilder", LVL_IMPL, 9401, &d_old);
  GET_PAGE_LENGTH(b) = 6;
  motd = strdup("Live MOTD before dupe\r\n");

  snprintf(arg, sizeof(arg), "motd");
  do_tedit(b, arg, 0, 0);
  TEST_ASSERT_EQUAL_INT(CON_TEDIT, STATE(&d_old));
  TEST_ASSERT_NOT_NULL(d_old.olc);
  for (i = 0; i < 15; i++)
    send_editor_line(&d_old, "Unsaved tedit line before duplicate login");
  send_editor_line(&d_old, "/l");
  TEST_ASSERT_TRUE(d_old.showstr_count > 1);

  login_duplicate_via_nanny(&d_new, "DupeBuilder", LVL_IMPL, 9401);

  TEST_ASSERT_NULL(d_old.olc);
  TEST_ASSERT_NULL(d_old.str);
  TEST_ASSERT_NULL(d_old.backstr);
  TEST_ASSERT_EQUAL_INT(0, d_old.showstr_count);
  TEST_ASSERT_NULL(d_old.showstr_vector);
  TEST_ASSERT_NULL(d_old.showstr_head);
  TEST_ASSERT_EQUAL_INT(CON_CLOSE, STATE(&d_old));
  TEST_ASSERT_NULL(d_old.character);

  /* cleanup_olc restored CON_PLAYING before usurp check so d_new usurped b */
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(&d_new));
  TEST_ASSERT_EQUAL_PTR(b, d_new.character);
  TEST_ASSERT_EQUAL_PTR(&d_new, b->desc);
  TEST_ASSERT_FALSE(PLR_FLAGGED(b, PLR_WRITING));
  TEST_ASSERT_EQUAL_STRING("Live MOTD before dupe\r\n", motd);

  unlink_descriptor(&d_old);
  unlink_descriptor(&d_new);
  free_test_char(b);

  /* Case B: switched original descriptor writing IBT when owner logs in again */
  memset(&d_sw, 0, sizeof(d_sw));
  memset(&d_mob, 0, sizeof(d_mob));
  imp_sw = make_test_char("SwitchedDupeImp", LVL_IMPL, 9402, &d_sw);
  mob = make_test_char("DupeTargetBody", 10, 9403, &d_mob);
  unlink_descriptor(&d_mob);
  mob->desc = NULL;

  snprintf(arg, sizeof(arg), "DupeTargetBody");
  do_switch(imp_sw, arg, 0, 0);
  snprintf(arg, sizeof(arg), "submit Unfinished switched dupe bug");
  do_ibt(mob, arg, 0, SCMD_BUG);
  send_editor_line(&d_sw, "Draft line on switched body");
  TEST_ASSERT_NOT_NULL(first_bug);

  login_duplicate_via_nanny(&d_re1, "SwitchedDupeImp", LVL_IMPL, 9402);

  TEST_ASSERT_NULL(first_bug);
  TEST_ASSERT_NULL(d_sw.str);
  TEST_ASSERT_EQUAL_INT(CON_CLOSE, STATE(&d_sw));
  TEST_ASSERT_NULL(d_sw.character);
  TEST_ASSERT_NULL(d_sw.original);
  TEST_ASSERT_NULL(mob->desc);
  TEST_ASSERT_FALSE(PLR_FLAGGED(mob, PLR_BUG));
  TEST_ASSERT_FALSE(PLR_FLAGGED(mob, PLR_WRITING));
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(&d_re1));
  TEST_ASSERT_EQUAL_PTR(imp_sw, d_re1.character);
  TEST_ASSERT_EQUAL_PTR(&d_re1, imp_sw->desc);

  unlink_descriptor(&d_sw);
  unlink_descriptor(&d_re1);
  free_test_char(imp_sw);
  free_test_char(mob);

  /* Case C: player reconnects while an immortal is switched into them in prefedit */
  memset(&d_imp, 0, sizeof(d_imp));
  memset(&d_vict, 0, sizeof(d_vict));
  imp2 = make_test_char("OccupyingImp", LVL_IMPL, 9404, &d_imp);
  vict = make_test_char("ReconnectingPC", 10, 9405, &d_vict);
  unlink_descriptor(&d_vict);
  vict->desc = NULL;

  snprintf(arg, sizeof(arg), "ReconnectingPC");
  do_switch(imp2, arg, 0, 0);
  arg[0] = '\0';
  do_oasis_prefedit(vict, arg, 0, 0);
  TEST_ASSERT_EQUAL_INT(CON_PREFEDIT, STATE(&d_imp));
  TEST_ASSERT_NOT_NULL(d_imp.olc);

  login_duplicate_via_nanny(&d_re2, "ReconnectingPC", 10, 9405);

  TEST_ASSERT_NULL(d_imp.olc);
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(&d_imp));
  TEST_ASSERT_EQUAL_PTR(imp2, d_imp.character);
  TEST_ASSERT_NULL(d_imp.original);
  TEST_ASSERT_EQUAL_PTR(&d_imp, imp2->desc);
  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(&d_re2));
  TEST_ASSERT_EQUAL_PTR(vict, d_re2.character);
  TEST_ASSERT_EQUAL_PTR(&d_re2, vict->desc);

  unlink_descriptor(&d_imp);
  unlink_descriptor(&d_re2);
  free_test_char(imp2);
  free_test_char(vict);
}

static void test_dc_on_olc_builder_and_menu_editor(void)
{
  struct descriptor_data d_imp, d_bld, d_menu;
  struct char_data *imp, *bld, *menu_ch;
  char arg[MAX_INPUT_LENGTH];
  int i;

  memset(&d_imp, 0, sizeof(d_imp));
  memset(&d_bld, 0, sizeof(d_bld));
  d_imp.desc_num = 1;
  d_bld.desc_num = 2;
  imp = make_test_char("DcImp", LVL_IMPL, 9501, &d_imp);
  bld = make_test_char("DcBuilder", LVL_GRGOD, 9502, &d_bld);
  GET_PAGE_LENGTH(bld) = 6;

  motd = strdup("MOTD before dc\r\n");
  snprintf(arg, sizeof(arg), "motd");
  do_tedit(bld, arg, 0, 0);
  TEST_ASSERT_EQUAL_INT(CON_TEDIT, STATE(&d_bld));
  TEST_ASSERT_NOT_NULL(d_bld.olc);
  for (i = 0; i < 15; i++)
    send_editor_line(&d_bld, "Unsaved line before dc");
  send_editor_line(&d_bld, "/l");
  TEST_ASSERT_TRUE(d_bld.showstr_count > 1);

  free_char_count = 0;
  snprintf(arg, sizeof(arg), "2");
  do_dc(imp, arg, 0, 0);

  TEST_ASSERT_NULL(d_bld.olc);
  TEST_ASSERT_NULL(d_bld.str);
  TEST_ASSERT_NULL(d_bld.backstr);
  TEST_ASSERT_EQUAL_INT(0, d_bld.showstr_count);
  TEST_ASSERT_NULL(d_bld.showstr_vector);
  TEST_ASSERT_NULL(d_bld.showstr_head);
  TEST_ASSERT_FALSE(PLR_FLAGGED(bld, PLR_WRITING));
  TEST_ASSERT_EQUAL_INT(CON_DISCONNECT, STATE(&d_bld));
  TEST_ASSERT_EQUAL_INT(0, free_char_count);
  TEST_ASSERT_EQUAL_STRING("MOTD before dc\r\n", motd);

  /* Non-playing descriptor in CON_PLR_DESC gets aborted and set to CON_CLOSE */
  memset(&d_menu, 0, sizeof(d_menu));
  d_menu.desc_num = 3;
  menu_ch = calloc(1, sizeof(*menu_ch));
  TEST_ASSERT_NOT_NULL(menu_ch);
  menu_ch->player_specials = calloc(1, sizeof(*menu_ch->player_specials));
  TEST_ASSERT_NOT_NULL(menu_ch->player_specials);
  menu_ch->player.name = strdup("MenuDescUser");
  menu_ch->player.level = 10;
  menu_ch->player.description = strdup("Half-edited desc\r\n");
  d_menu.character = menu_ch;
  menu_ch->desc = &d_menu;
  d_menu.backstr = strdup("Original desc\r\n");
  d_menu.str = &menu_ch->player.description;
  d_menu.max_str = PLR_DESC_LENGTH;
  STATE(&d_menu) = CON_PLR_DESC;
  d_menu.next = descriptor_list;
  descriptor_list = &d_menu;

  snprintf(arg, sizeof(arg), "3");
  do_dc(imp, arg, 0, 0);

  TEST_ASSERT_NULL(d_menu.str);
  TEST_ASSERT_NULL(d_menu.backstr);
  TEST_ASSERT_EQUAL_STRING("Original desc\r\n", menu_ch->player.description);
  TEST_ASSERT_EQUAL_INT(CON_CLOSE, STATE(&d_menu));

  unlink_descriptor(&d_menu);
  free(menu_ch->player.description);
  free(menu_ch->player.name);
  free(menu_ch->player_specials);
  free(menu_ch);

  unlink_descriptor(&d_imp);
  unlink_descriptor(&d_bld);
  free_test_char(imp);
  free_test_char(bld);
}


static struct char_data *make_creation_char(const char *name,
                                            struct descriptor_data *d,
                                            int state)
{
  struct char_data *ch = calloc(1, sizeof(*ch));
  TEST_ASSERT_NOT_NULL(ch);
  ch->player_specials = calloc(1, sizeof(*ch->player_specials));
  TEST_ASSERT_NOT_NULL(ch->player_specials);
  ch->player.name = strdup(name);
  IN_ROOM(ch) = NOWHERE;

  memset(d, 0, sizeof(*d));
  snprintf(d->host, sizeof(d->host), "localhost");
  d->character = ch;
  ch->desc = d;
  STATE(d) = state;
  d->next = descriptor_list;
  descriptor_list = d;
  return ch;
}

static void free_creation_char(struct char_data *ch)
{
  if (!ch)
    return;
  if (ch->player.name)
    free(ch->player.name);
  if (GET_HOST(ch))
    free(GET_HOST(ch));
  if (ch->player_specials && ch->player_specials != &dummy_mob)
    free(ch->player_specials);
  free(ch);
}

static void test_new_char_dupe_check_older_creation_and_ingame_collision(void)
{
  struct descriptor_data d_unnamed, d_older, d_qclass, d_new1, d_trailing, d_ingame, d_new2;
  struct char_data *ch_unnamed, *ch_older, *ch_qclass, *ch_new1, *bystander, *builder, *ch_new2;
  char arg[MAX_INPUT_LENGTH];

  /* Case A: unnamed CON_GET_NAME descriptor is skipped; older connections in
   * CON_NEWPASSWD and at the CON_QCLASS boundary (matched case-insensitively)
   * are booted with characters kept attached while the new session proceeds. */
  ch_unnamed = make_creation_char("temp", &d_unnamed, CON_GET_NAME);
  free(ch_unnamed->player.name);
  ch_unnamed->player.name = NULL;

  ch_older = make_creation_char("McNewbie", &d_older, CON_NEWPASSWD);
  ch_qclass = make_creation_char("MCNEWBIE", &d_qclass, CON_QCLASS);
  ch_new1 = make_creation_char("Mcnewbie", &d_new1, CON_NAME_CNFRM);

  snprintf(arg, sizeof(arg), "Y");
  nanny(&d_new1, arg);

  TEST_ASSERT_EQUAL_INT(CON_GET_NAME, STATE(&d_unnamed));
  TEST_ASSERT_EQUAL_PTR(ch_unnamed, d_unnamed.character);
  TEST_ASSERT_EQUAL_INT(CON_CLOSE, STATE(&d_older));
  TEST_ASSERT_EQUAL_PTR(ch_older, d_older.character);
  TEST_ASSERT_EQUAL_PTR(&d_older, ch_older->desc);
  TEST_ASSERT_EQUAL_INT(CON_CLOSE, STATE(&d_qclass));
  TEST_ASSERT_EQUAL_PTR(ch_qclass, d_qclass.character);
  TEST_ASSERT_EQUAL_PTR(&d_qclass, ch_qclass->desc);
  TEST_ASSERT_EQUAL_INT(CON_NEWPASSWD, STATE(&d_new1));
  TEST_ASSERT_EQUAL_PTR(ch_new1, d_new1.character);

  unlink_descriptor(&d_new1);
  unlink_descriptor(&d_qclass);
  unlink_descriptor(&d_older);
  unlink_descriptor(&d_unnamed);
  free_creation_char(ch_unnamed);
  free_creation_char(ch_older);
  free_creation_char(ch_qclass);
  free_creation_char(ch_new1);

  /* Case B: first connection is in-game (in CON_TEDIT) and a trailing
   * descriptor follows it in descriptor_list -> close editors on k, close both
   * k and d immediately without scanning trailing descriptors or prompting d */
  memset(&d_trailing, 0, sizeof(d_trailing));
  memset(&d_ingame, 0, sizeof(d_ingame));
  bystander = make_test_char("Bystander", 10, 9601, &d_trailing);
  builder = make_test_char("CollisionBuilder", LVL_IMPL, 9602, &d_ingame);
  motd = strdup("MOTD before creation collision\r\n");

  snprintf(arg, sizeof(arg), "motd");
  do_tedit(builder, arg, 0, 0);
  TEST_ASSERT_EQUAL_INT(CON_TEDIT, STATE(&d_ingame));
  send_editor_line(&d_ingame, "Unsaved collision draft");

  ch_new2 = make_creation_char("Collisionbuilder", &d_new2, CON_NAME_CNFRM);
  /* Verify list order: d_new2 -> d_ingame -> d_trailing */
  TEST_ASSERT_EQUAL_PTR(&d_ingame, d_new2.next);
  TEST_ASSERT_EQUAL_PTR(&d_trailing, d_ingame.next);

  snprintf(arg, sizeof(arg), "Y");
  nanny(&d_new2, arg);

  TEST_ASSERT_NULL(d_ingame.olc);
  TEST_ASSERT_NULL(d_ingame.str);
  TEST_ASSERT_NULL(d_ingame.backstr);
  TEST_ASSERT_EQUAL_INT(CON_CLOSE, STATE(&d_ingame));
  TEST_ASSERT_NULL(d_ingame.character);
  TEST_ASSERT_NULL(builder->desc);
  TEST_ASSERT_FALSE(PLR_FLAGGED(builder, PLR_WRITING));
  TEST_ASSERT_EQUAL_STRING("MOTD before creation collision\r\n", motd);

  TEST_ASSERT_EQUAL_INT(CON_CLOSE, STATE(&d_new2));
  TEST_ASSERT_EQUAL_PTR(ch_new2, d_new2.character);
  TEST_ASSERT_EQUAL_PTR(&d_new2, ch_new2->desc);

  TEST_ASSERT_EQUAL_INT(CON_PLAYING, STATE(&d_trailing));
  TEST_ASSERT_EQUAL_PTR(bystander, d_trailing.character);

  unlink_descriptor(&d_new2);
  unlink_descriptor(&d_ingame);
  unlink_descriptor(&d_trailing);
  free_creation_char(ch_new2);
  free_test_char(builder);
  free_test_char(bystander);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_note_abort_after_prototype_refresh_preserves_proto);
  RUN_TEST(test_note_save_after_prototype_refresh_preserves_proto);
  RUN_TEST(test_note_save_and_abort_after_extract_obj);
  RUN_TEST(test_disconnected_ibt_writer_aborted_before_save_and_renumbers_editor);
  RUN_TEST(test_ibt_remove_only_blocks_matching_edited_number);
  RUN_TEST(test_login_clears_report_flags_and_note_abort_ignores_stale_flags);
  RUN_TEST(test_note_save_rejects_transformed_or_transferred_paper);
  RUN_TEST(test_extract_char_aborts_editor_and_dismisses_pager);
  RUN_TEST(test_extract_switched_prefedit_and_switched_original_body);
  RUN_TEST(test_player_purge_and_idle_rent_cleanup_editors_and_olc);
  RUN_TEST(test_tedit_private_draft_concurrent_save_and_basic_editor_extraction);
  RUN_TEST(test_tedit_save_preserves_pager_byte_offsets_after_pagelength_change);
  RUN_TEST(test_extract_occupied_original_body_resolves_both_descriptors);
  RUN_TEST(test_tedit_detects_stale_draft_conflict_and_keeps_draft);
  RUN_TEST(test_tedit_confirms_disk_write_and_rename_before_publishing);
  RUN_TEST(test_dupe_login_closes_editors_and_usurps_olc_builder);
  RUN_TEST(test_dc_on_olc_builder_and_menu_editor);
  RUN_TEST(test_new_char_dupe_check_older_creation_and_ingame_collision);
  return UNITY_END();
}
