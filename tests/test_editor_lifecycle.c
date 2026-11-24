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
#include "fight.h"
#include "act.h"
#include <sys/stat.h>

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
int no_mail = 1;
const char *connected_types[] = { "\n" };
const char *class_abbrevs[] = { "Mu", "\n" };

/* Stubs for external helpers referenced by modify.c / genobj.c / interpreter.c / act.comm.c */
obj_rnum real_object(obj_vnum vnum) { return (vnum == 3000 && obj_proto) ? 0 : NOTHING; }
zone_rnum real_zone_by_thing(room_vnum vnum) { (void)vnum; return (zone_table && top_of_zone_table >= 0) ? 0 : NOWHERE; }
int add_to_save_list(zone_vnum zone, int type) { (void)zone; (void)type; return 0; }
int store_mail(long to, long from, char *message_pointer) { (void)to; (void)from; (void)message_pointer; return 1; }
void notify_if_playing(struct char_data *from, int recipient_id) { (void)from; (void)recipient_id; }
int board_abort_write(int board_type, char **writeto) { (void)board_type; (void)writeto; return 0; }
void board_save_board(int board_type) { (void)board_type; }
void cedit_string_cleanup(struct descriptor_data *d, int terminator) { (void)d; (void)terminator; }
void medit_string_cleanup(struct descriptor_data *d, int terminator) { (void)d; (void)terminator; }
void oedit_string_cleanup(struct descriptor_data *d, int terminator) { (void)d; (void)terminator; }
void redit_string_cleanup(struct descriptor_data *d, int terminator) { (void)d; (void)terminator; }
void tedit_string_cleanup(struct descriptor_data *d, int terminator) { (void)d; (void)terminator; }
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
void prefedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
void trigedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
void genolc_free_obj(struct obj_data *obj) { (void)obj; }

struct board_info_type board_info[NUM_OF_BOARDS];
int top_shop = -1;
const char *nrm = "", *grn = "", *cyn = "", *yel = "";
void get_char_colors(struct char_data *ch) { (void)ch; }
void clear_screen(struct descriptor_data *d) { (void)d; }
void cleanup_olc(struct descriptor_data *d, byte cleanup_type) { (void)d; (void)cleanup_type; }
void copy_ex_descriptions(struct extra_descr_data **to, struct extra_descr_data *from)
{ (void)to; (void)from; }
void free_ex_descriptions(struct extra_descr_data *head) { (void)head; }

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
static struct room_data fixture_room;
static struct obj_data fixture_protos[2];
static struct index_data fixture_obj_index[2];
static char orig_cwd[4096];
static char tmp_template[] = "/tmp/tbamud-editor-test-XXXXXX";
static char *active_tmpdir = NULL;

static struct char_data *make_test_char(const char *name, int level, long idnum, struct descriptor_data *d)
{
  struct char_data *ch = calloc(1, sizeof(*ch));
  TEST_ASSERT_NOT_NULL(ch);
  ch->player_specials = calloc(1, sizeof(*ch->player_specials));
  TEST_ASSERT_NOT_NULL(ch->player_specials);
  ch->player.name = strdup(name);
  ch->player.level = level;
  ch->char_specials.saved.idnum = idnum;
  IN_ROOM(ch) = 0;
  d->character = ch;
  d->connected = CON_PLAYING;
  d->next = descriptor_list;
  descriptor_list = d;
  ch->desc = d;
  return ch;
}

static void free_test_char(struct char_data *ch)
{
  if (!ch)
    return;
  while (ch->carrying)
    extract_obj(ch->carrying);
  if (GET_EQ(ch, WEAR_HOLD))
    extract_obj(unequip_char(ch, WEAR_HOLD));
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
  memset(hook_plr_flags, 0, sizeof(hook_plr_flags));

  memset(&config_info, 0, sizeof(config_info));
  config_info.operation.ibt_autosave = 1;

  memset(&fixture_zone, 0, sizeof(fixture_zone));
  fixture_zone.number = 30;
  fixture_zone.bot = 3000;
  fixture_zone.top = 3099;
  zone_table = &fixture_zone;
  top_of_zone_table = 0;

  memset(&fixture_room, 0, sizeof(fixture_room));
  fixture_room.number = 3000;
  fixture_room.name = "Test Room";
  world = &fixture_room;
  top_of_world = 0;

  memset(fixture_protos, 0, sizeof(fixture_protos));
  memset(fixture_obj_index, 0, sizeof(fixture_obj_index));
  fixture_obj_index[0].vnum = 3000;
  fixture_protos[0].item_number = 0;
  obj_proto = fixture_protos;
  obj_index = fixture_obj_index;
  top_of_objt = 0;

  if (!getcwd(orig_cwd, sizeof(orig_cwd)))
    TEST_FAIL_MESSAGE("getcwd failed");
  strcpy(tmp_template, "/tmp/tbamud-editor-test-XXXXXX");
  active_tmpdir = mkdtemp(tmp_template);
  TEST_ASSERT_NOT_NULL(active_tmpdir);
  TEST_ASSERT_EQUAL_INT(0, chdir(active_tmpdir));
  TEST_ASSERT_EQUAL_INT(0, mkdir("misc", 0700));
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

  if (active_tmpdir) {
    (void)remove("misc/bugs");
    (void)remove("misc/ideas");
    (void)remove("misc/typos");
    (void)rmdir("misc");
    if (chdir(orig_cwd) != 0) {}
    (void)rmdir(active_tmpdir);
    active_tmpdir = NULL;
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
  return UNITY_END();
}
