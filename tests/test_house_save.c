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
    remove(HCONTROL_FILE);
  }
  remove(HOUSE_FILE);
  remove(HOUSE_FILE ".tmp");
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

  fail_record = FALSE;
  House_save_all();
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
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

static void test_partial_record_preserves_previous_save(void)
{
  write_file(HOUSE_FILE, "old contents");
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  fail_record = TRUE;
  House_crashsave(3000);
  assert_file(HOUSE_FILE, "old contents");
  TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
}

static void test_save_leaves_existing_scratch_name_alone(void)
{
  write_file(HOUSE_FILE ".tmp", "existing file");
  House_crashsave(3000);
  assert_file(HOUSE_FILE, item.name);
  assert_file(HOUSE_FILE ".tmp", "existing file");
}

int main(void)
{
  char scratch[] = "/tmp/tbamud-house-XXXXXX";
  int result;
  if (!mkdtemp(scratch) || chdir(scratch) || mkdir("house", 0700) || mkdir("etc", 0700))
    return 1;
  umask(0022);
  logfile = stderr;
  UNITY_BEGIN();
  RUN_TEST(test_new_house_retries_after_its_first_save_fails);
  RUN_TEST(test_existing_house_keeps_its_permissions);
  RUN_TEST(test_partial_record_preserves_previous_save);
  RUN_TEST(test_save_leaves_existing_scratch_name_alone);
  result = UNITY_END();
  if (rmdir("house") || rmdir("etc") || chdir("/") || rmdir(scratch))
    return 1;
  return result;
}
