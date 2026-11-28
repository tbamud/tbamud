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

#include <sys/stat.h>

#define PLAYER_INDEX_PATH LIB_PLRFILES INDEX_FILE
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

int top_of_p_table = -1;
int top_of_p_file = -1;
long top_idnum = 0;

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

/* Local flag decoder for test index fixtures (db.c collides with test_stubs.c). */
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
  return is_num ? (bitvector_t)atol(flag) : flags;
}

/* Callbacks referenced by genolc.c's save_types[] table (retained under ASan). */
int save_mobiles(zone_rnum rznum) { (void)rznum; return FALSE; }
int save_objects(zone_rnum vznum) { (void)vznum; return FALSE; }
int save_shops(zone_rnum zone_num) { (void)zone_num; return FALSE; }
int save_rooms(zone_rnum rnum) { (void)rnum; return FALSE; }
int save_zone(zone_rnum zone_num) { (void)zone_num; return FALSE; }
int save_config(IDXTYPE nowhere) { (void)nowhere; return FALSE; }
int save_quests(zone_rnum zone_num) { (void)zone_num; return FALSE; }
int save_triggers(zone_rnum zrnum) { (void)zrnum; return FALSE; }

struct index_spec {
  long id;
  const char *name;
  int level;
  bitvector_t flags;
  time_t last;
};

static const char *const test_names[] = {
  "adam", "alice", "beth", "bob", "cara", "dave", "edna", "erin"
};

static void create_files(const char *name, int all_modes)
{
  char filename[MAX_STRING_LENGTH];
  int mode, start = all_modes ? 0 : PLR_FILE;

  for (mode = start; mode < MAX_FILES; mode++) {
    FILE *fp;
    TEST_ASSERT_TRUE(get_filename(filename, sizeof(filename), mode, name));
    fp = fopen(filename, "w");
    TEST_ASSERT_NOT_NULL(fp);
    TEST_ASSERT_TRUE(fputs("stub\n", fp) >= 0);
    TEST_ASSERT_EQUAL_INT(0, fclose(fp));
  }
}

static void assert_files(const char *name, int all_modes, int expected)
{
  char filename[MAX_STRING_LENGTH];
  int mode, start = all_modes ? 0 : PLR_FILE;

  for (mode = start; mode < MAX_FILES; mode++) {
    TEST_ASSERT_TRUE(get_filename(filename, sizeof(filename), mode, name));
    TEST_ASSERT_EQUAL_INT(expected, access(filename, F_OK) == 0);
  }
}

static void load_specs(const struct index_spec *specs, size_t count, int all_modes)
{
  FILE *fp = fopen(PLAYER_INDEX_PATH, "w");
  char bits[64];
  size_t i;

  TEST_ASSERT_NOT_NULL(fp);
  for (i = 0; i < count; i++) {
    sprintascii(bits, specs[i].flags);
    TEST_ASSERT_TRUE(fprintf(fp, "%ld %s %d %s %ld\n",
                             specs[i].id, specs[i].name, specs[i].level,
                             *bits ? bits : "0", (long)specs[i].last) > 0);
    create_files(specs[i].name, all_modes);
  }
  TEST_ASSERT_TRUE(fputs("~\n", fp) >= 0);
  TEST_ASSERT_EQUAL_INT(0, fclose(fp));

  build_player_index();
  TEST_ASSERT_EQUAL_INT((int)count - 1, top_of_p_table);
}

static void assert_table_matches(const char *const *names, const long *ids, size_t count)
{
  size_t i;

  TEST_ASSERT_EQUAL_INT((int)count - 1, top_of_p_table);
  TEST_ASSERT_NOT_NULL(player_table);
  for (i = 0; i < count; i++) {
    TEST_ASSERT_EQUAL_STRING(names[i], player_table[i].name);
    TEST_ASSERT_EQUAL_INT((int)ids[i], (int)player_table[i].id);
  }
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
  char filename[MAX_STRING_LENGTH];
  size_t i;
  int mode;

  free_player_index();
  top_of_p_table = -1;
  top_of_p_file = -1;
  top_idnum = 0;
  remove(PLAYER_INDEX_PATH);
  for (i = 0; i < ARRAY_LEN(test_names); i++)
    for (mode = 0; mode < MAX_FILES; mode++)
      if (get_filename(filename, sizeof(filename), mode, test_names[i]))
        remove(filename);
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
  const char *const kept_names[] = { "erin" };
  const long kept_ids[] = { 5 };
  size_t i;

  load_specs(specs, ARRAY_LEN(specs), TRUE);
  clean_pfiles();

  assert_table_matches(kept_names, kept_ids, ARRAY_LEN(kept_names));
  for (i = 0; i < 4; i++)
    assert_files(specs[i].name, TRUE, FALSE);
  assert_files("erin", TRUE, TRUE);

  free_player_index();
  top_of_p_table = -1;
  build_player_index();
  assert_table_matches(kept_names, kept_ids, ARRAY_LEN(kept_names));
}

static void test_clean_pfiles_preserves_nodelete_active_and_order(void)
{
  time_t now = time(0);
  const struct index_spec specs[] = {
    { 1, "adam",  5,        0,                                now - 2 * SECS_PER_REAL_DAY },
    { 2, "alice", 1,        0,                                now - 10 * SECS_PER_REAL_DAY },
    { 3, "beth",  1,        PINDEX_NODELETE | PINDEX_DELETED, now - 100 * SECS_PER_REAL_DAY },
    { 4, "bob",   20,       PINDEX_DELETED,                   now },
    { 5, "cara",  0,        0,                                now - 10 },
    { 6, "dave",  10,       0,                                now - 5 * SECS_PER_REAL_DAY },
    { 7, "erin",  LVL_IMPL, 0,                                now - 100 * SECS_PER_REAL_DAY },
    { 8, "edna",  1,        0,                                now - 1 * SECS_PER_REAL_DAY }
  };
  const char *const kept_names[] = { "adam", "beth", "dave", "edna" };
  const long kept_ids[] = { 1, 3, 6, 8 };
  const char *const removed[] = { "alice", "bob", "cara", "erin" };
  size_t i;

  load_specs(specs, ARRAY_LEN(specs), FALSE);
  clean_pfiles();

  assert_table_matches(kept_names, kept_ids, ARRAY_LEN(kept_names));
  for (i = 0; i < ARRAY_LEN(kept_names); i++)
    assert_files(kept_names[i], FALSE, TRUE);
  for (i = 0; i < ARRAY_LEN(removed); i++)
    assert_files(removed[i], FALSE, FALSE);

  free_player_index();
  top_of_p_table = -1;
  build_player_index();
  assert_table_matches(kept_names, kept_ids, ARRAY_LEN(kept_names));
}

static void test_clean_pfiles_empties_table_when_all_removed(void)
{
  time_t now = time(0);
  const struct index_spec specs[] = {
    { 1, "alice", 1,  0,              now - 10 * SECS_PER_REAL_DAY },
    { 2, "bob",   10, PINDEX_DELETED, now },
    { 3, "cara",  4,  0,              now - 10 * SECS_PER_REAL_DAY }
  };
  char contents[16];
  FILE *fp;
  size_t i, len;

  load_specs(specs, ARRAY_LEN(specs), TRUE);
  clean_pfiles();

  TEST_ASSERT_EQUAL_INT(-1, top_of_p_table);
  TEST_ASSERT_NULL(player_table);
  for (i = 0; i < ARRAY_LEN(specs); i++)
    assert_files(specs[i].name, TRUE, FALSE);

  fp = fopen(PLAYER_INDEX_PATH, "r");
  TEST_ASSERT_NOT_NULL(fp);
  len = fread(contents, 1, sizeof(contents) - 1, fp);
  TEST_ASSERT_EQUAL_INT(EOF, fgetc(fp));
  TEST_ASSERT_EQUAL_INT(0, ferror(fp));
  TEST_ASSERT_EQUAL_INT(0, fclose(fp));
  contents[len] = '\0';
  TEST_ASSERT_EQUAL_STRING("~\n", contents);
}

static void test_clean_pfiles_handles_missing_index(void)
{
  build_player_index();
  TEST_ASSERT_EQUAL_INT(-1, top_of_p_table);
  TEST_ASSERT_NULL(player_table);

  clean_pfiles();
  TEST_ASSERT_EQUAL_INT(-1, top_of_p_table);
  TEST_ASSERT_NULL(player_table);
}

int main(void)
{
  char scratch[] = "/tmp/tbamud-players-XXXXXX";
  int status;

  if (!mkdtemp(scratch) || chdir(scratch) ||
      mkdir("plrfiles", 0700) || mkdir("plrfiles/A-E", 0700) ||
      mkdir("plrobjs", 0700) || mkdir("plrobjs/A-E", 0700) ||
      mkdir("plrtext", 0700) || mkdir("plrtext/A-E", 0700) ||
      mkdir("plrvars", 0700) || mkdir("plrvars/A-E", 0700))
    return 1;

  logfile = stderr;
  UNITY_BEGIN();
  RUN_TEST(test_clean_pfiles_removes_consecutive_expired_and_deleted_players);
  RUN_TEST(test_clean_pfiles_preserves_nodelete_active_and_order);
  RUN_TEST(test_clean_pfiles_empties_table_when_all_removed);
  RUN_TEST(test_clean_pfiles_handles_missing_index);
  status = UNITY_END();

  if (rmdir("plrfiles/A-E") || rmdir("plrfiles") ||
      rmdir("plrobjs/A-E") || rmdir("plrobjs") ||
      rmdir("plrtext/A-E") || rmdir("plrtext") ||
      rmdir("plrvars/A-E") || rmdir("plrvars") ||
      chdir("/") || rmdir(scratch))
    return 1;
  return status;
}
