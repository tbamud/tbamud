/* Exercise clean_pfiles(), remove_player(), and player index compaction
 * against an isolated player-file directory tree. */
#include "conf.h"
#include "sysdep.h"
#include "unity.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "config.h"
#include "genolc.h"

#if defined(CIRCLE_WINDOWS) || defined(_WIN32)
#include <direct.h>
#include <io.h>
#include <windows.h>
#define test_mkdir(path) _mkdir(path)
#define test_chdir(path) _chdir(path)
#define test_rmdir(path) _rmdir(path)
#define test_getcwd(buf, size) _getcwd(buf, size)
#define test_access(path) _access((path), 0)
#else
#include <sys/stat.h>
#include <unistd.h>
#define test_mkdir(path) mkdir((path), 0700)
#define test_chdir(path) chdir(path)
#define test_rmdir(path) rmdir(path)
#define test_getcwd(buf, size) getcwd((buf), (size))
#define test_access(path) access((path), F_OK)
#endif

#define PLAYER_INDEX_PATH LIB_PLRFILES INDEX_FILE

/* Globals referenced by src/players.c (player_table and logfile come from
 * test_stubs.c). */
int top_of_p_table = -1;
int top_of_p_file = -1;
long top_idnum = 0;

/* Minimal flag codecs matching src/db.c and src/genolc.c so the test links
 * cleanly under ASan/UBSan without pulling in global OLC/DB tables. */
bitvector_t asciiflag_conv(char *flag)
{
  bitvector_t flags = 0;
  int is_num = TRUE;
  char *p;

  for (p = flag; *p; p++) {
    if (islower((unsigned char)*p))
      flags |= 1U << (*p - 'a');
    else if (isupper((unsigned char)*p))
      flags |= 1U << (26 + (*p - 'A'));

    if (!isdigit((unsigned char)*p) && (*p != '-' || p != flag))
      is_num = FALSE;
  }

  if (is_num)
    flags = (bitvector_t)atol(flag);

  return flags;
}

int sprintascii(char *out, bitvector_t bits)
{
  int i, j = 0;
  const char *flags = "abcdefghijklmnopqrstuvwxyzABCDEF";

  for (i = 0; flags[i] != '\0'; i++)
    if (bits & (1U << i))
      out[j++] = flags[i];

  if (j == 0)
    out[j++] = '0';

  out[j++] = '\0';
  return j;
}

/* Stock autowipe thresholds matching src/config.c. */
struct pclean_criteria_data pclean_criteria[] = {
  { 0, 0 },
  { 1, 4 },
  { 4, 7 },
  { 10, 30 },
  { LVL_IMMORT - 1, 60 },
  { LVL_IMPL, 90 },
  { -1, 0 }
};

static const char *const test_names[] = {
  "adam", "alice", "beth", "bob", "cara", "dave", "edna", "erin"
};

static void remove_player_files(const char *name)
{
  char filename[MAX_STRING_LENGTH];
  int mode;

  for (mode = 0; mode < MAX_FILES; mode++) {
    if (get_filename(filename, sizeof(filename), mode, name))
      remove(filename);
  }
}

static void create_player_file(int mode, const char *name)
{
  char filename[MAX_STRING_LENGTH];
  FILE *fp;

  TEST_ASSERT_TRUE(get_filename(filename, sizeof(filename), mode, name));
  fp = fopen(filename, "wb");
  TEST_ASSERT_NOT_NULL(fp);
  TEST_ASSERT_TRUE(fputs("stub\n", fp) >= 0);
  TEST_ASSERT_EQUAL_INT(0, fclose(fp));
}

static void create_all_player_files(const char *name)
{
  int mode;

  for (mode = 0; mode < MAX_FILES; mode++)
    create_player_file(mode, name);
}

static void assert_player_files_exist(const char *name, int expected)
{
  char filename[MAX_STRING_LENGTH];
  int mode;

  for (mode = 0; mode < MAX_FILES; mode++) {
    TEST_ASSERT_TRUE(get_filename(filename, sizeof(filename), mode, name));
    if (expected)
      TEST_ASSERT_EQUAL_INT(0, test_access(filename));
    else
      TEST_ASSERT_NOT_EQUAL(0, test_access(filename));
  }
}

struct index_spec {
  long id;
  const char *name;
  int level;
  bitvector_t flags;
  time_t last;
};

static void load_index_specs(const struct index_spec *specs, size_t count)
{
  FILE *fp;
  size_t i;
  char bits[64];

  fp = fopen(PLAYER_INDEX_PATH, "wb");
  TEST_ASSERT_NOT_NULL(fp);
  for (i = 0; i < count; i++) {
    sprintascii(bits, specs[i].flags);
    TEST_ASSERT_TRUE(fprintf(fp, "%ld %s %d %s %ld\n",
                             specs[i].id, specs[i].name, specs[i].level,
                             *bits ? bits : "0", (long)specs[i].last) > 0);
  }
  TEST_ASSERT_TRUE(fputs("~\n", fp) >= 0);
  TEST_ASSERT_EQUAL_INT(0, fclose(fp));

  build_player_index();
  TEST_ASSERT_EQUAL_INT((int)count - 1, top_of_p_table);
}

static void read_file_contents(const char *path, char *buf, size_t size)
{
  FILE *fp = fopen(path, "rb");
  size_t len;

  TEST_ASSERT_NOT_NULL(fp);
  len = fread(buf, 1, size - 1, fp);
  TEST_ASSERT_EQUAL_INT(EOF, fgetc(fp));
  TEST_ASSERT_EQUAL_INT(0, ferror(fp));
  TEST_ASSERT_EQUAL_INT(0, fclose(fp));
  buf[len] = '\0';
}

void setUp(void)
{
  free_player_index();
  top_of_p_table = -1;
  top_of_p_file = -1;
  top_idnum = 0;
  remove(PLAYER_INDEX_PATH);
}

void tearDown(void)
{
  size_t i;

  free_player_index();
  top_of_p_table = -1;
  top_of_p_file = -1;
  top_idnum = 0;
  remove(PLAYER_INDEX_PATH);
  for (i = 0; i < sizeof(test_names) / sizeof(test_names[0]); i++)
    remove_player_files(test_names[i]);
}

static void test_clean_pfiles_removes_consecutive_expired_and_deleted_players(void)
{
  time_t now = time(0);
  const struct index_spec specs[] = {
    { 1, "alice", 1,  0,              now - 10 * SECS_PER_REAL_DAY },
    { 2, "bob",   1,  0,              now - 10 * SECS_PER_REAL_DAY },
    { 3, "cara",  10, PINDEX_DELETED, now },
    { 4, "dave",  4,  0,              now - 10 * SECS_PER_REAL_DAY },
    { 5, "erin",  10, 0,              now - 1 * SECS_PER_REAL_DAY }
  };
  size_t i;

  load_index_specs(specs, sizeof(specs) / sizeof(specs[0]));
  for (i = 0; i < sizeof(specs) / sizeof(specs[0]); i++)
    create_all_player_files(specs[i].name);

  clean_pfiles();

  TEST_ASSERT_EQUAL_INT(0, top_of_p_table);
  TEST_ASSERT_NOT_NULL(player_table);
  TEST_ASSERT_EQUAL_STRING("erin", player_table[0].name);
  TEST_ASSERT_EQUAL_INT(5, (int)player_table[0].id);

  assert_player_files_exist("alice", FALSE);
  assert_player_files_exist("bob", FALSE);
  assert_player_files_exist("cara", FALSE);
  assert_player_files_exist("dave", FALSE);
  assert_player_files_exist("erin", TRUE);

  /* Reloading the saved index should match the in-memory table. */
  free_player_index();
  top_of_p_table = -1;
  build_player_index();
  TEST_ASSERT_EQUAL_INT(0, top_of_p_table);
  TEST_ASSERT_NOT_NULL(player_table);
  TEST_ASSERT_EQUAL_STRING("erin", player_table[0].name);
  TEST_ASSERT_EQUAL_INT(5, (int)player_table[0].id);
}

static void test_clean_pfiles_preserves_nodelete_active_and_order(void)
{
  time_t now = time(0);
  const struct index_spec specs[] = {
    { 1, "adam",  5,        0,                               now - 2 * SECS_PER_REAL_DAY },
    { 2, "alice", 1,        0,                               now - 10 * SECS_PER_REAL_DAY },
    { 3, "beth",  1,        PINDEX_NODELETE | PINDEX_DELETED, now - 100 * SECS_PER_REAL_DAY },
    { 4, "bob",   20,       PINDEX_DELETED,                  now },
    { 5, "cara",  0,        0,                               now - 10 },
    { 6, "dave",  10,       0,                               now - 5 * SECS_PER_REAL_DAY },
    { 7, "erin",  LVL_IMPL, 0,                               now - 100 * SECS_PER_REAL_DAY },
    { 8, "edna",  1,        0,                               now - 1 * SECS_PER_REAL_DAY }
  };
  const char *expected_names[] = { "adam", "beth", "dave", "edna" };
  const long expected_ids[] = { 1, 3, 6, 8 };
  const char *removed_names[] = { "alice", "bob", "cara", "erin" };
  char filename[MAX_STRING_LENGTH];
  size_t i;

  load_index_specs(specs, sizeof(specs) / sizeof(specs[0]));
  for (i = 0; i < sizeof(specs) / sizeof(specs[0]); i++)
    create_player_file(PLR_FILE, specs[i].name);

  clean_pfiles();

  TEST_ASSERT_EQUAL_INT(3, top_of_p_table);
  TEST_ASSERT_NOT_NULL(player_table);
  for (i = 0; i < sizeof(expected_names) / sizeof(expected_names[0]); i++) {
    TEST_ASSERT_EQUAL_STRING(expected_names[i], player_table[i].name);
    TEST_ASSERT_EQUAL_INT((int)expected_ids[i], (int)player_table[i].id);
    TEST_ASSERT_TRUE(get_filename(filename, sizeof(filename), PLR_FILE, expected_names[i]));
    TEST_ASSERT_EQUAL_INT(0, test_access(filename));
  }
  for (i = 0; i < sizeof(removed_names) / sizeof(removed_names[0]); i++) {
    TEST_ASSERT_TRUE(get_filename(filename, sizeof(filename), PLR_FILE, removed_names[i]));
    TEST_ASSERT_NOT_EQUAL(0, test_access(filename));
  }

  free_player_index();
  top_of_p_table = -1;
  build_player_index();
  TEST_ASSERT_EQUAL_INT(3, top_of_p_table);
  TEST_ASSERT_NOT_NULL(player_table);
  for (i = 0; i < sizeof(expected_names) / sizeof(expected_names[0]); i++) {
    TEST_ASSERT_EQUAL_STRING(expected_names[i], player_table[i].name);
    TEST_ASSERT_EQUAL_INT((int)expected_ids[i], (int)player_table[i].id);
  }
}

static void test_clean_pfiles_empties_table_when_all_removed(void)
{
  time_t now = time(0);
  const struct index_spec specs[] = {
    { 1, "alice", 1,  0,              now - 10 * SECS_PER_REAL_DAY },
    { 2, "bob",   10, PINDEX_DELETED, now },
    { 3, "cara",  4,  0,              now - 10 * SECS_PER_REAL_DAY }
  };
  char contents[64];
  size_t i;

  load_index_specs(specs, sizeof(specs) / sizeof(specs[0]));
  for (i = 0; i < sizeof(specs) / sizeof(specs[0]); i++)
    create_all_player_files(specs[i].name);

  clean_pfiles();

  TEST_ASSERT_EQUAL_INT(-1, top_of_p_table);
  TEST_ASSERT_NULL(player_table);
  for (i = 0; i < sizeof(specs) / sizeof(specs[0]); i++)
    assert_player_files_exist(specs[i].name, FALSE);

  read_file_contents(PLAYER_INDEX_PATH, contents, sizeof(contents));
  TEST_ASSERT_EQUAL_STRING("~\n", contents);
}

static void test_clean_pfiles_handles_empty_and_null_table(void)
{
  player_table = NULL;
  top_of_p_table = -1;
  clean_pfiles();
  TEST_ASSERT_NULL(player_table);
  TEST_ASSERT_EQUAL_INT(-1, top_of_p_table);

  /* Matches the default global state before build_player_index() or after
   * free_player_index(). */
  player_table = NULL;
  top_of_p_table = 0;
  clean_pfiles();
  TEST_ASSERT_NULL(player_table);
  TEST_ASSERT_EQUAL_INT(0, top_of_p_table);
}

int main(void)
{
  char cwd[4096], temp[4096];
  int status;

  if (!test_getcwd(cwd, sizeof(cwd)))
    return 1;

#if defined(CIRCLE_WINDOWS) || defined(_WIN32)
  {
    char root[MAX_PATH];
    if (!GetTempPathA(sizeof(root), root) ||
        !GetTempFileNameA(root, "plr", 0, temp))
      return 1;
    if (!DeleteFileA(temp) || test_mkdir(temp))
      return 1;
  }
#else
  strcpy(temp, "/tmp/tbamud-players-XXXXXX");
  if (!mkdtemp(temp))
    return 1;
#endif

  if (test_chdir(temp) ||
      test_mkdir("plrfiles") || test_mkdir("plrfiles/A-E") ||
      test_mkdir("plrobjs") || test_mkdir("plrobjs/A-E") ||
      test_mkdir("plrtext") || test_mkdir("plrtext/A-E") ||
      test_mkdir("plrvars") || test_mkdir("plrvars/A-E"))
    return 1;

  logfile = stderr;
  UNITY_BEGIN();
  RUN_TEST(test_clean_pfiles_removes_consecutive_expired_and_deleted_players);
  RUN_TEST(test_clean_pfiles_preserves_nodelete_active_and_order);
  RUN_TEST(test_clean_pfiles_empties_table_when_all_removed);
  RUN_TEST(test_clean_pfiles_handles_empty_and_null_table);
  status = UNITY_END();

  test_rmdir("plrfiles/A-E");
  test_rmdir("plrfiles");
  test_rmdir("plrobjs/A-E");
  test_rmdir("plrobjs");
  test_rmdir("plrtext/A-E");
  test_rmdir("plrtext");
  test_rmdir("plrvars/A-E");
  test_rmdir("plrvars");
  if (test_chdir(cwd) || test_rmdir(temp))
    return 1;
  return status;
}
