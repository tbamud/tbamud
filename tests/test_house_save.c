/* Exercise house creation and saving against an isolated house directory.
 * The record writer can fail after writing bytes to its stream. */
#include "conf.h"
#include "sysdep.h"
#include "unity.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "house.h"

#include <sys/stat.h>

#define HOUSE_FILE LIB_HOUSE "3000.house"
#define ELSEWHERE_FILE "elsewhere/3000.house"
static struct room_data rooms[2];
static struct obj_data item;
static int fail_record;

room_rnum real_room(room_vnum vnum)
{
  return vnum == 3000 || vnum == 3001 ? vnum - 3000 : NOWHERE;
}

long get_id_by_name(const char *name) { return !strcmp(name, "owner") ? 1 : -1; }

/* The other hcontrol subcommands share the command handler. */
char *get_name_by_id(long id) { return NULL; }
obj_save_data *objsave_parse_objects(FILE *fp) { return NULL; }
void extract_obj(struct obj_data *obj) {}
obj_rnum real_object(obj_vnum vnum) { return NOTHING; }
struct obj_data *read_object(obj_vnum vnum, int type) { return NULL; }

int objsave_save_obj_record(struct obj_data *obj, FILE *fp, int location)
{
  return fputs(obj->name, fp) >= 0 && !fail_record;
}

void setUp(void)
{
  static struct room_direction_data north, south;
  memset(rooms, 0, sizeof(rooms));
  world = rooms;
  top_of_world = 1;
  rooms[0].number = 3000;
  rooms[1].number = 3001;
  north.to_room = 1;
  south.to_room = 0;
  rooms[0].dir_option[NORTH] = &north;
  rooms[1].dir_option[SOUTH] = &south;
  rooms[0].contents = &item;
  item.name = "current contents";
  fail_record = FALSE;
}

void tearDown(void)
{
  if (ROOM_FLAGGED(0, ROOM_HOUSE)) {
    char command[] = "destroy 3000";
    do_hcontrol(NULL, command, 0, 0);
  }
  umask(0022);
  remove(HCONTROL_FILE);
  remove(HOUSE_FILE);
  remove(HOUSE_FILE ".tmp");
  remove(ELSEWHERE_FILE);
  remove(ELSEWHERE_FILE ".tmp");
  remove("bystander");
}

/* lstat(), so a dangling link still counts as something being there. */
static int exists(const char *path)
{
  struct stat st;
  return lstat(path, &st) == 0;
}

static void write_file(const char *path, const char *contents)
{
  FILE *fp = fopen(path, "wb");
  TEST_ASSERT_NOT_NULL(fp);
  int wrote = fputs(contents, fp);
  int closed = fclose(fp);
  TEST_ASSERT_TRUE(wrote >= 0);
  TEST_ASSERT_EQUAL_INT(0, closed);
}

static void assert_file(const char *path, const char *expected)
{
  char bytes[64];
  FILE *fp = fopen(path, "rb");
  TEST_ASSERT_NOT_NULL(fp);
  size_t length = fread(bytes, 1, sizeof(bytes), fp);
  int error = ferror(fp);
  fclose(fp);
  TEST_ASSERT_EQUAL_INT(0, error);
  TEST_ASSERT_EQUAL_UINT(strlen(expected), length);
  TEST_ASSERT_EQUAL_MEMORY(expected, bytes, length);
}

static void test_new_house_retries_after_its_first_save_fails(void)
{
  char command[] = "build 3000 north owner";
  fail_record = TRUE;
  do_hcontrol(NULL, command, 0, 0);
  TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE));
  TEST_ASSERT_TRUE(access(HCONTROL_FILE, F_OK) == 0);
  TEST_ASSERT_TRUE(access(HOUSE_FILE, F_OK) != 0);
  TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));

  TEST_ASSERT_FALSE(exists(HOUSE_FILE ".tmp"));

  fail_record = FALSE;
  House_save_all();
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
}

/* The scratch file is created as fopen() creates one, so a new house's file
 * gets the mode it always did rather than mkstemp()'s 0600. */
static void test_new_house_file_follows_the_umask(void)
{
  const mode_t masks[] = {0022, 0027, 0077};
  struct stat st;
  for (size_t i = 0; i < sizeof(masks) / sizeof(masks[0]); ++i) {
    remove(HOUSE_FILE);
    umask(masks[i]);
    House_crashsave(3000);
    umask(0022);
    assert_file(HOUSE_FILE, item.name);
    TEST_ASSERT_EQUAL_INT(0, stat(HOUSE_FILE, &st));
    TEST_ASSERT_EQUAL_UINT(0666 & ~masks[i], st.st_mode & 0777);
  }
}

static void test_existing_house_keeps_its_permissions(void)
{
  const mode_t modes[] = {0600, 0660};
  struct stat st;
  for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
    write_file(HOUSE_FILE, "old contents");
    TEST_ASSERT_EQUAL_INT(0, chmod(HOUSE_FILE, modes[i]));
    House_crashsave(3000);
    assert_file(HOUSE_FILE, item.name);
    TEST_ASSERT_EQUAL_INT(0, stat(HOUSE_FILE, &st));
    TEST_ASSERT_EQUAL_UINT(modes[i], st.st_mode & 0777);
  }
}

/* Any group this process is in can be given back.  The case it cannot give
 * back -- a group it is not in -- takes root to set up, so it is not here. */
static void test_existing_house_keeps_a_group_the_game_is_in(void)
{
  gid_t groups[64];
  int count = getgroups(64, groups), i;
  struct stat st;

  for (i = 0; i < count && groups[i] == getegid(); i++)
    ;
  if (i >= count)
    TEST_IGNORE_MESSAGE("this user is in no group but its own");
  write_file(HOUSE_FILE, "old contents");
  TEST_ASSERT_EQUAL_INT(0, chown(HOUSE_FILE, (uid_t)-1, groups[i]));
  TEST_ASSERT_EQUAL_INT(0, chmod(HOUSE_FILE, 0640));
  House_crashsave(3000);
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(0, stat(HOUSE_FILE, &st));
  TEST_ASSERT_EQUAL_UINT(groups[i], st.st_gid);
  TEST_ASSERT_EQUAL_UINT(0640, st.st_mode & 0777);
}

static void test_partial_record_preserves_previous_save(void)
{
  write_file(HOUSE_FILE, "old contents");
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  fail_record = TRUE;
  House_crashsave(3000);
  assert_file(HOUSE_FILE, "old contents");
  TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  TEST_ASSERT_FALSE(exists(HOUSE_FILE ".tmp"));
}

static void test_stale_scratch_file_is_replaced(void)
{
  write_file(HOUSE_FILE ".tmp", "left by a crash");
  House_crashsave(3000);
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_FALSE(exists(HOUSE_FILE ".tmp"));
}

/* Whatever a link at the scratch name points at is left as it was. */
static void test_save_never_writes_through_the_scratch_name(void)
{
  for (int symbolic = 0; symbolic < 2; ++symbolic) {
    write_file("bystander", "bystander");
    TEST_ASSERT_EQUAL_INT(0, symbolic ? symlink("../bystander", HOUSE_FILE ".tmp")
                                      : link("bystander", HOUSE_FILE ".tmp"));
    House_crashsave(3000);
    assert_file(HOUSE_FILE, item.name);
    assert_file("bystander", "bystander");
    TEST_ASSERT_FALSE(exists(HOUSE_FILE ".tmp"));
  }
}

static void test_symlinked_house_file_is_saved_where_it_points(void)
{
  struct stat st;
  write_file(ELSEWHERE_FILE, "old contents");
  TEST_ASSERT_EQUAL_INT(0, chmod(ELSEWHERE_FILE, 0640));
  TEST_ASSERT_EQUAL_INT(0, symlink("../" ELSEWHERE_FILE, HOUSE_FILE));
  House_crashsave(3000);
  TEST_ASSERT_EQUAL_INT(0, lstat(HOUSE_FILE, &st));
  TEST_ASSERT_TRUE(S_ISLNK(st.st_mode));
  assert_file(ELSEWHERE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(0, stat(ELSEWHERE_FILE, &st));
  TEST_ASSERT_EQUAL_UINT(0640, st.st_mode & 0777);
  TEST_ASSERT_FALSE(exists(ELSEWHERE_FILE ".tmp"));
}

static void test_destroying_a_house_removes_its_stale_scratch_file(void)
{
  char build[] = "build 3000 north owner", destroy[] = "destroy 3000";
  do_hcontrol(NULL, build, 0, 0);
  write_file(HOUSE_FILE ".tmp", "left by a crash");
  do_hcontrol(NULL, destroy, 0, 0);
  TEST_ASSERT_FALSE(exists(HOUSE_FILE));
  TEST_ASSERT_FALSE(exists(HOUSE_FILE ".tmp"));
}

int main(void)
{
  char scratch[] = "/tmp/tbamud-house-XXXXXX";
  int result;
  if (!mkdtemp(scratch) || chdir(scratch) || mkdir("house", 0700) || mkdir("etc", 0700) ||
      mkdir("elsewhere", 0700))
    return 1;
  umask(0022);
  logfile = stderr;
  UNITY_BEGIN();
  RUN_TEST(test_new_house_retries_after_its_first_save_fails);
  RUN_TEST(test_new_house_file_follows_the_umask);
  RUN_TEST(test_existing_house_keeps_its_permissions);
  RUN_TEST(test_existing_house_keeps_a_group_the_game_is_in);
  RUN_TEST(test_partial_record_preserves_previous_save);
  RUN_TEST(test_stale_scratch_file_is_replaced);
  RUN_TEST(test_save_never_writes_through_the_scratch_name);
  RUN_TEST(test_symlinked_house_file_is_saved_where_it_points);
  RUN_TEST(test_destroying_a_house_removes_its_stale_scratch_file);
  result = UNITY_END();
  if (rmdir("house") || rmdir("etc") || rmdir("elsewhere") || chdir("/") || rmdir(scratch))
    return 1;
  return result;
}
