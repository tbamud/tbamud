/* Exercise mail retrieval failure recovery and postmaster delivery against an
 * isolated etc/ spool directory. */
#include "conf.h"
#include "sysdep.h"
#include "unity.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "mail.h"
#include "modify.h"
#include "genolc.h"

#if defined(CIRCLE_WINDOWS) || defined(_WIN32)
#include <direct.h>
#include <windows.h>
#define test_mkdir(path) _mkdir(path)
#define test_chdir(path) _chdir(path)
#define test_rmdir(path) _rmdir(path)
#define test_getcwd(buf, size) _getcwd(buf, size)
#else
#include <sys/stat.h>
#include <unistd.h>
#define test_mkdir(path) mkdir(path, 0700)
#define test_chdir(path) chdir(path)
#define test_rmdir(path) rmdir(path)
#define test_getcwd(buf, size) getcwd(buf, size)
#endif

static int fail_install = FALSE;

static int test_install_file(const char *scratch, const char *dest)
{
  if (fail_install) {
    errno = EACCES;
    return FALSE;
  }
  return genolc_install_file(scratch, dest);
}

/* Reach postmaster_receive_mail() and allow deterministic install-failure
 * injection while still delegating normal installs to real genolc_install_file(). */
#define genolc_install_file test_install_file
#include "mail.c"
#undef genolc_install_file

#define ALICE_ID 10L
#define BOB_ID   20L

int no_mail = 0;

static struct index_data test_obj_index[2];
static int create_obj_calls;
static int extract_obj_calls;
static char last_act_msg[MAX_STRING_LENGTH];
static char alice_name[] = "Alice";
static char bob_name[] = "Bob";

char *get_name_by_id(long id)
{
  if (id == ALICE_ID)
    return alice_name;
  if (id == BOB_ID)
    return bob_name;
  return NULL;
}

long get_id_by_name(const char *name)
{
  if (name && !strcmp(name, "Alice"))
    return ALICE_ID;
  if (name && !strcmp(name, "Bob"))
    return BOB_ID;
  return -1;
}

int decrease_gold(struct char_data *ch, int amt)
{
  (void)ch;
  (void)amt;
  return 0;
}

void string_write(struct descriptor_data *d, char **txt, size_t len, long mailto, void *data)
{
  (void)d;
  (void)txt;
  (void)len;
  (void)mailto;
  (void)data;
}

char *act(const char *str, int hide_invisible, struct char_data *ch,
          struct obj_data *obj, void *vict_obj, int type)
{
  (void)hide_invisible;
  (void)ch;
  (void)obj;
  (void)vict_obj;
  if (str && type == TO_VICT)
    strlcpy(last_act_msg, str, sizeof(last_act_msg));
  return NULL;
}

struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  create_obj_calls++;
  CREATE(obj, struct obj_data, 1);
  obj->item_number = NOTHING;
  return obj;
}

void obj_to_char(struct obj_data *object, struct char_data *ch)
{
  object->next_content = ch->carrying;
  ch->carrying = object;
  object->carried_by = ch;
}

static void free_test_obj(struct obj_data *obj)
{
  if (obj->name)
    free(obj->name);
  if (obj->short_description)
    free(obj->short_description);
  if (obj->description)
    free(obj->description);
  if (obj->action_description)
    free(obj->action_description);
  free(obj);
}

void extract_obj(struct obj_data *obj)
{
  extract_obj_calls++;
  if (GET_OBJ_RNUM(obj) != NOTHING && obj_index != NULL)
    obj_index[GET_OBJ_RNUM(obj)].number--;
  free_test_obj(obj);
}

static void free_inventory(struct char_data *ch)
{
  struct obj_data *obj, *next;

  for (obj = ch->carrying; obj; obj = next) {
    next = obj->next_content;
    free_test_obj(obj);
  }
  ch->carrying = NULL;
}

static int file_exists(const char *path)
{
  FILE *fp = fopen(path, "rb");

  if (!fp)
    return FALSE;
  fclose(fp);
  return TRUE;
}

static void read_entire_file(const char *path, char *buffer, size_t capacity)
{
  FILE *fp = fopen(path, "rb");
  size_t length;
  int extra, err;

  TEST_ASSERT_NOT_NULL(fp);
  length = fread(buffer, 1, capacity - 1, fp);
  extra = fgetc(fp);
  err = ferror(fp);
  fclose(fp);
  TEST_ASSERT_EQUAL_INT(EOF, extra);
  TEST_ASSERT_EQUAL_INT(0, err);
  buffer[length] = '\0';
}

static void append_raw_file(const char *path, const char *text)
{
  FILE *fp = fopen(path, "ab");

  TEST_ASSERT_NOT_NULL(fp);
  TEST_ASSERT_TRUE(fputs(text, fp) >= 0);
  TEST_ASSERT_EQUAL_INT(0, fclose(fp));
}

void setUp(void)
{
  logfile = stderr;
  no_mail = 0;
  fail_install = FALSE;
  create_obj_calls = 0;
  extract_obj_calls = 0;
  last_act_msg[0] = '\0';

  memset(test_obj_index, 0, sizeof(test_obj_index));
  obj_index = test_obj_index;
  top_of_objt = 1;

  remove(MAIL_FILE);
  remove(MAIL_FILE_TMP);
  test_rmdir(MAIL_FILE_TMP);
}

void tearDown(void)
{
#if !defined(CIRCLE_WINDOWS) && !defined(_WIN32)
  chmod("etc", 0700);
#endif
  remove(MAIL_FILE);
  remove(MAIL_FILE_TMP);
  test_rmdir(MAIL_FILE_TMP);
}

static void test_failed_install_preserves_all_mail_and_retries(void)
{
  char before[4096], after[4096];
  char *letter;

  TEST_ASSERT_TRUE(store_mail(ALICE_ID, BOB_ID, "Hello Alice"));
  TEST_ASSERT_TRUE(store_mail(BOB_ID, ALICE_ID, "Hello Bob"));
  read_entire_file(MAIL_FILE, before, sizeof(before));

  fail_install = TRUE;
  letter = read_delete(ALICE_ID);
  TEST_ASSERT_NULL(letter);
  TEST_ASSERT_EQUAL_INT(0, no_mail);
  TEST_ASSERT_FALSE(file_exists(MAIL_FILE_TMP));
  read_entire_file(MAIL_FILE, after, sizeof(after));
  TEST_ASSERT_EQUAL_STRING(before, after);
  TEST_ASSERT_TRUE(has_mail(ALICE_ID));
  TEST_ASSERT_TRUE(has_mail(BOB_ID));

  fail_install = FALSE;
  letter = read_delete(ALICE_ID);
  TEST_ASSERT_NOT_NULL(letter);
  TEST_ASSERT_NOT_NULL(strstr(letter, "To  : Alice"));
  TEST_ASSERT_NOT_NULL(strstr(letter, "From: Bob"));
  TEST_ASSERT_NOT_NULL(strstr(letter, "Hello Alice"));
  free(letter);

  TEST_ASSERT_EQUAL_INT(0, no_mail);
  TEST_ASSERT_FALSE(file_exists(MAIL_FILE_TMP));
  TEST_ASSERT_FALSE(has_mail(ALICE_ID));
  TEST_ASSERT_TRUE(has_mail(BOB_ID));

  letter = read_delete(BOB_ID);
  TEST_ASSERT_NOT_NULL(letter);
  TEST_ASSERT_NOT_NULL(strstr(letter, "Hello Bob"));
  free(letter);
  TEST_ASSERT_FALSE(has_mail(BOB_ID));
}

static void test_real_install_failure_preserves_mail_and_retries(void)
{
  char before[4096], after[4096];
  char *letter;

  TEST_ASSERT_TRUE(store_mail(ALICE_ID, BOB_ID, "Hello Alice"));
  TEST_ASSERT_TRUE(store_mail(BOB_ID, ALICE_ID, "Hello Bob"));
  read_entire_file(MAIL_FILE, before, sizeof(before));

#if defined(CIRCLE_WINDOWS) || defined(_WIN32)
  {
    /* Hold MAIL_FILE open without FILE_SHARE_DELETE so fopen("r") succeeds
     * while MoveFileExA(..., MOVEFILE_REPLACE_EXISTING) fails. */
    HANDLE h = CreateFileA(MAIL_FILE, GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    TEST_ASSERT_TRUE(h != INVALID_HANDLE_VALUE);
    letter = read_delete(ALICE_ID);
    CloseHandle(h);

    TEST_ASSERT_NULL(letter);
    TEST_ASSERT_EQUAL_INT(0, no_mail);
    TEST_ASSERT_FALSE(file_exists(MAIL_FILE_TMP));
    read_entire_file(MAIL_FILE, after, sizeof(after));
    TEST_ASSERT_EQUAL_STRING(before, after);
  }
#else
  if (geteuid() != 0) {
    /* Pre-create a writable temp file inside a read-only etc/ directory so
     * opening and writing the temp file succeed while rename() fails with EACCES. */
    FILE *fp = fopen(MAIL_FILE_TMP, "wb");
    TEST_ASSERT_NOT_NULL(fp);
    TEST_ASSERT_EQUAL_INT(0, fclose(fp));
    TEST_ASSERT_EQUAL_INT(0, chmod("etc", 0500));

    letter = read_delete(ALICE_ID);

    TEST_ASSERT_EQUAL_INT(0, chmod("etc", 0700));
    TEST_ASSERT_NULL(letter);
    TEST_ASSERT_EQUAL_INT(0, no_mail);
    read_entire_file(MAIL_FILE, after, sizeof(after));
    TEST_ASSERT_EQUAL_STRING(before, after);
    remove(MAIL_FILE_TMP);
  }
#endif

  letter = read_delete(ALICE_ID);
  TEST_ASSERT_NOT_NULL(letter);
  TEST_ASSERT_NOT_NULL(strstr(letter, "Hello Alice"));
  free(letter);
  TEST_ASSERT_FALSE(file_exists(MAIL_FILE_TMP));
  TEST_ASSERT_FALSE(has_mail(ALICE_ID));
  TEST_ASSERT_TRUE(has_mail(BOB_ID));
}

static void test_temp_open_failure_preserves_mail_and_retries(void)
{
  char before[4096], after[4096];
  char *letter;

  TEST_ASSERT_TRUE(store_mail(ALICE_ID, BOB_ID, "Hello Alice"));
  TEST_ASSERT_TRUE(store_mail(BOB_ID, ALICE_ID, "Hello Bob"));
  read_entire_file(MAIL_FILE, before, sizeof(before));

  TEST_ASSERT_EQUAL_INT(0, test_mkdir(MAIL_FILE_TMP));
  letter = read_delete(ALICE_ID);
  TEST_ASSERT_NULL(letter);
  TEST_ASSERT_EQUAL_INT(0, no_mail);
  read_entire_file(MAIL_FILE, after, sizeof(after));
  TEST_ASSERT_EQUAL_STRING(before, after);

  TEST_ASSERT_EQUAL_INT(0, test_rmdir(MAIL_FILE_TMP));
  letter = read_delete(ALICE_ID);
  TEST_ASSERT_NOT_NULL(letter);
  TEST_ASSERT_NOT_NULL(strstr(letter, "Hello Alice"));
  free(letter);
  TEST_ASSERT_FALSE(has_mail(ALICE_ID));
  TEST_ASSERT_TRUE(has_mail(BOB_ID));
}

static void test_temp_write_failure_preserves_mail_and_retries(void)
{
#if !defined(CIRCLE_WINDOWS) && !defined(_WIN32)
  if (access("/dev/full", W_OK) == 0) {
    char large_body[4096];
    char before[32768], after[32768];
    char *letter;
    int i;

    memset(large_body, 'B', sizeof(large_body) - 1);
    large_body[sizeof(large_body) - 1] = '\0';
    for (i = 63; i + 1 < (int)sizeof(large_body) - 1; i += 64) {
      large_body[i] = '\r';
      large_body[i + 1] = '\n';
    }

    TEST_ASSERT_TRUE(store_mail(ALICE_ID, BOB_ID, "Hello Alice"));
    /* Write enough remaining mail to overflow stdio's buffer so write_mail_record()
     * itself hits ENOSPC while copying to /dev/full. */
    for (i = 0; i < 4; i++)
      TEST_ASSERT_TRUE(store_mail(BOB_ID, ALICE_ID, large_body));

    read_entire_file(MAIL_FILE, before, sizeof(before));
    TEST_ASSERT_EQUAL_INT(0, symlink("/dev/full", MAIL_FILE_TMP));

    letter = read_delete(ALICE_ID);
    TEST_ASSERT_NULL(letter);
    TEST_ASSERT_EQUAL_INT(0, no_mail);
    TEST_ASSERT_FALSE(file_exists(MAIL_FILE_TMP));
    read_entire_file(MAIL_FILE, after, sizeof(after));
    TEST_ASSERT_EQUAL_STRING(before, after);

    letter = read_delete(ALICE_ID);
    TEST_ASSERT_NOT_NULL(letter);
    TEST_ASSERT_NOT_NULL(strstr(letter, "Hello Alice"));
    free(letter);
    TEST_ASSERT_EQUAL_INT(0, no_mail);
    TEST_ASSERT_FALSE(file_exists(MAIL_FILE_TMP));
    TEST_ASSERT_FALSE(has_mail(ALICE_ID));
    TEST_ASSERT_TRUE(has_mail(BOB_ID));
  }
#endif
}

static void test_corrupt_spool_after_target_disables_mail_and_preserves_file(void)
{
  char before[4096], after[4096];
  char *letter;

  TEST_ASSERT_TRUE(store_mail(ALICE_ID, BOB_ID, "Hello Alice"));
  append_raw_file(MAIL_FILE, "corrupt header line\n");
  read_entire_file(MAIL_FILE, before, sizeof(before));

  /* has_mail() stops at Alice's valid record before hitting the corruption. */
  TEST_ASSERT_TRUE(has_mail(ALICE_ID));
  TEST_ASSERT_EQUAL_INT(0, no_mail);

  letter = read_delete(ALICE_ID);
  TEST_ASSERT_NULL(letter);
  TEST_ASSERT_EQUAL_INT(1, no_mail);
  TEST_ASSERT_FALSE(file_exists(MAIL_FILE_TMP));
  read_entire_file(MAIL_FILE, after, sizeof(after));
  TEST_ASSERT_EQUAL_STRING(before, after);
}

static void test_postmaster_receive_skips_obj_creation_on_failure_and_recovers(void)
{
  struct char_data alice, mailman;
  char before[4096], after[4096];

  memset(&alice, 0, sizeof(alice));
  memset(&mailman, 0, sizeof(mailman));
  GET_IDNUM(&alice) = ALICE_ID;
  test_obj_index[1].number = 5;

  TEST_ASSERT_TRUE(store_mail(ALICE_ID, BOB_ID, "Hello Alice"));
  TEST_ASSERT_TRUE(store_mail(BOB_ID, ALICE_ID, "Hello Bob"));
  read_entire_file(MAIL_FILE, before, sizeof(before));

  /* Repeated transient failures must not disable mail, lose waiting mail,
   * or create/extract throwaway objects (which would decrement obj_index[1]). */
  fail_install = TRUE;
  postmaster_receive_mail(&alice, &mailman, 0, "");
  postmaster_receive_mail(&alice, &mailman, 0, "");

  TEST_ASSERT_EQUAL_INT(0, create_obj_calls);
  TEST_ASSERT_EQUAL_INT(0, extract_obj_calls);
  TEST_ASSERT_EQUAL_INT(5, test_obj_index[1].number);
  TEST_ASSERT_NULL(alice.carrying);
  TEST_ASSERT_EQUAL_INT(0, no_mail);
  TEST_ASSERT_NOT_NULL(strstr(last_act_msg, "encountered a problem"));
  TEST_ASSERT_FALSE(file_exists(MAIL_FILE_TMP));
  read_entire_file(MAIL_FILE, after, sizeof(after));
  TEST_ASSERT_EQUAL_STRING(before, after);

  /* Retrying once the install failure clears delivers Alice's letter and leaves Bob's. */
  fail_install = FALSE;
  postmaster_receive_mail(&alice, &mailman, 0, "");

  TEST_ASSERT_EQUAL_INT(1, create_obj_calls);
  TEST_ASSERT_EQUAL_INT(0, extract_obj_calls);
  TEST_ASSERT_EQUAL_INT(5, test_obj_index[1].number);
  TEST_ASSERT_NOT_NULL(alice.carrying);
  TEST_ASSERT_NULL(alice.carrying->next_content);
  TEST_ASSERT_NOT_NULL(alice.carrying->action_description);
  TEST_ASSERT_NOT_NULL(strstr(alice.carrying->action_description, "Hello Alice"));
  TEST_ASSERT_FALSE(has_mail(ALICE_ID));
  TEST_ASSERT_TRUE(has_mail(BOB_ID));
  TEST_ASSERT_FALSE(file_exists(MAIL_FILE_TMP));
  free_inventory(&alice);

  /* A spool read error through postmaster_receive_mail() still sets no_mail = 1
   * without touching obj_index[1] or overwriting the spool. */
  remove(MAIL_FILE);
  TEST_ASSERT_TRUE(store_mail(ALICE_ID, BOB_ID, "Second letter"));
  append_raw_file(MAIL_FILE, "corrupt header line\n");
  read_entire_file(MAIL_FILE, before, sizeof(before));

  postmaster_receive_mail(&alice, &mailman, 0, "");
  TEST_ASSERT_EQUAL_INT(1, create_obj_calls);
  TEST_ASSERT_EQUAL_INT(0, extract_obj_calls);
  TEST_ASSERT_EQUAL_INT(5, test_obj_index[1].number);
  TEST_ASSERT_NULL(alice.carrying);
  TEST_ASSERT_EQUAL_INT(1, no_mail);
  TEST_ASSERT_FALSE(file_exists(MAIL_FILE_TMP));
  read_entire_file(MAIL_FILE, after, sizeof(after));
  TEST_ASSERT_EQUAL_STRING(before, after);
}

int main(void)
{
  char cwd[4096], scratch[4096];
  int status;

  if (!test_getcwd(cwd, sizeof(cwd)))
    return 1;
#if defined(CIRCLE_WINDOWS) || defined(_WIN32)
  {
    char root[MAX_PATH];
    if (!GetTempPathA(sizeof(root), root) ||
        !GetTempFileNameA(root, "mal", 0, scratch))
      return 1;
    if (!DeleteFileA(scratch) || test_mkdir(scratch))
      return 1;
  }
#else
  strcpy(scratch, "/tmp/tbamud-mail-XXXXXX");
  if (!mkdtemp(scratch))
    return 1;
#endif
  if (test_chdir(scratch) || test_mkdir("etc"))
    return 1;

  UNITY_BEGIN();
  RUN_TEST(test_failed_install_preserves_all_mail_and_retries);
  RUN_TEST(test_real_install_failure_preserves_mail_and_retries);
  RUN_TEST(test_temp_open_failure_preserves_mail_and_retries);
  RUN_TEST(test_temp_write_failure_preserves_mail_and_retries);
  RUN_TEST(test_corrupt_spool_after_target_disables_mail_and_preserves_file);
  RUN_TEST(test_postmaster_receive_skips_obj_creation_on_failure_and_recovers);
  status = UNITY_END();

  if (test_rmdir("etc") || test_chdir(cwd) || test_rmdir(scratch))
    return 1;
  return status;
}
