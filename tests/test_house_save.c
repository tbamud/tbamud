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
#if defined(__linux__)
#include <sys/xattr.h>
#elif defined(__FreeBSD__)
#include <sys/acl.h>
#endif

#define HOUSE_FILE LIB_HOUSE "3000.house"
#define ELSEWHERE_FILE "elsewhere/3000.house"
static struct room_data rooms[2];
static struct obj_data item;
static int fail_record;

#ifdef HOUSE_TEST_WRAP_IO
#include <fcntl.h>
/* Fail only the sync of the installed file's parent. File syncs and other
 * directories still use the filesystem normally. */
static struct stat watched_parent;
static int watch_sync, fail_parent_sync, parent_syncs;
int __real_fsync(int fd);
static int watch_create, saw_private_create;
int __real_openat(int dirfd, const char *name, int flags, ...);
static int deny_chown;
int __real_fchown(int fd, uid_t owner, gid_t group);

int __wrap_fchown(int fd, uid_t owner, gid_t group)
{
  if (deny_chown) {
    errno = EPERM;
    return -1;
  }
  return __real_fchown(fd, owner, group);
}

int __wrap_openat(int dirfd, const char *name, int flags, ...)
{
  struct stat file, parent;
  va_list ap;
  int fd, mode;
  if (!(flags & O_CREAT))
    return __real_openat(dirfd, name, flags);
  va_start(ap, flags);
  mode = va_arg(ap, int);
  va_end(ap);
  fd = __real_openat(dirfd, name, flags, mode);
  /* Inspect our own descriptor before the saver starts copying permissions. */
  if (watch_create && fd >= 0 && (flags & O_EXCL) && fstat(fd, &file) == 0 &&
      fstat(dirfd, &parent) == 0 && S_ISDIR(parent.st_mode))
    saw_private_create = mode == 0600 && (file.st_mode & 0177) == 0 &&
                         (parent.st_mode & 0777) == 0700;
  return fd;
}

int __wrap_fsync(int fd)
{
  struct stat st;
  if (watch_sync && fstat(fd, &st) == 0 && st.st_dev == watched_parent.st_dev &&
      st.st_ino == watched_parent.st_ino) {
    ++parent_syncs;
    if (fail_parent_sync) {
      errno = EIO;
      return -1;
    }
  }
  return __real_fsync(fd);
}
#endif

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
#ifdef HOUSE_TEST_WRAP_IO
  watch_sync = fail_parent_sync = parent_syncs = 0;
  watch_create = saw_private_create = deny_chown = 0;
#endif
  if (ROOM_FLAGGED(0, ROOM_HOUSE)) {
    char command[] = "destroy 3000";
    do_hcontrol(NULL, command, 0, 0);
  }
  umask(0022);
  chmod("house", 0700);
  remove(HCONTROL_FILE);
  remove(HOUSE_FILE);
  remove(HOUSE_FILE ".tmp");
  remove(ELSEWHERE_FILE);
  remove(ELSEWHERE_FILE ".tmp");
  remove("bystander");
#if defined(__linux__)
  removexattr("house", "system.posix_acl_default");
  remove(LIB_HOUSE "reference");
#endif
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

/* First-time saves follow the same creation rules as fopen(). */
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

static void set_alternate_house_group(gid_t *chosen)
{
  gid_t *groups = NULL;
  int count = getgroups(0, NULL), i;
  struct stat dir_st;

  TEST_ASSERT_TRUE(count >= 0);
  TEST_ASSERT_EQUAL_INT(0, stat("house", &dir_st));
  if (count > 0) {
    groups = malloc((size_t)count * sizeof(*groups));
    TEST_ASSERT_NOT_NULL(groups);
    count = getgroups(count, groups);
    TEST_ASSERT_TRUE(count >= 0);
  }
  /* Pick a group that differs from both our effective GID and house/'s GID,
   * so BSD/macOS or setgid directory inheritance cannot satisfy it by chance. */
  for (i = 0; i < count && (groups[i] == getegid() || groups[i] == dir_st.st_gid); i++)
    ;
  if (i >= count) {
    free(groups);
    TEST_IGNORE_MESSAGE("this user has no alternate group distinct from house/");
  }
  *chosen = groups[i];
  free(groups);
  TEST_ASSERT_EQUAL_INT(0, chown(HOUSE_FILE, (uid_t)-1, *chosen));
}

static void test_existing_house_keeps_a_group_the_game_is_in(void)
{
  struct stat st;
  gid_t chosen;
  write_file(HOUSE_FILE, "old contents");
  set_alternate_house_group(&chosen);
  TEST_ASSERT_EQUAL_INT(0, chmod(HOUSE_FILE, 0640));
  House_crashsave(3000);
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(0, stat(HOUSE_FILE, &st));
  TEST_ASSERT_EQUAL_UINT(chosen, st.st_gid);
  TEST_ASSERT_EQUAL_UINT(0640, st.st_mode & 0777);
}

static void test_partial_record_preserves_previous_save(void)
{
  write_file(HOUSE_FILE, "old contents");
  write_file(HOUSE_FILE ".tmp", "operator's copy");
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  fail_record = TRUE;
  House_crashsave(3000);
  assert_file(HOUSE_FILE, "old contents");
  TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  assert_file(HOUSE_FILE ".tmp", "operator's copy");
}

static void test_saving_leaves_sibling_files_untouched(void)
{
  write_file(HOUSE_FILE ".tmp", "operator's copy");
  House_crashsave(3000);
  assert_file(HOUSE_FILE, item.name);
  assert_file(HOUSE_FILE ".tmp", "operator's copy");
}

static void test_saving_leaves_sibling_links_untouched(void)
{
  struct stat before, after;
  for (int symbolic = 0; symbolic < 2; ++symbolic) {
    write_file("bystander", "bystander");
    TEST_ASSERT_EQUAL_INT(0, symbolic ? symlink("../bystander", HOUSE_FILE ".tmp")
                                      : link("bystander", HOUSE_FILE ".tmp"));
    TEST_ASSERT_EQUAL_INT(0, lstat(HOUSE_FILE ".tmp", &before));
    House_crashsave(3000);
    assert_file(HOUSE_FILE, item.name);
    assert_file("bystander", "bystander");
    TEST_ASSERT_EQUAL_INT(0, lstat(HOUSE_FILE ".tmp", &after));
    TEST_ASSERT_TRUE(before.st_dev == after.st_dev && before.st_ino == after.st_ino);
    TEST_ASSERT_EQUAL_INT(0, unlink(HOUSE_FILE ".tmp"));
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

static void test_dangling_symlink_house_file_creates_its_target(void)
{
  struct stat st;
  TEST_ASSERT_EQUAL_INT(0, symlink("../" ELSEWHERE_FILE, HOUSE_FILE));
  House_crashsave(3000);
  TEST_ASSERT_EQUAL_INT(0, lstat(HOUSE_FILE, &st));
  TEST_ASSERT_TRUE(S_ISLNK(st.st_mode));
  assert_file(ELSEWHERE_FILE, item.name);
  TEST_ASSERT_FALSE(exists(HOUSE_FILE ".tmp"));
  TEST_ASSERT_FALSE(exists(ELSEWHERE_FILE ".tmp"));
}

static void test_broken_symlink_house_file_is_not_replaced(void)
{
  const char *targets[] = {"../missing/3000.house", "3000.house"};
  struct stat st;
  for (size_t i = 0; i < sizeof(targets) / sizeof(targets[0]); ++i) {
    remove(HOUSE_FILE);
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
    TEST_ASSERT_EQUAL_INT(0, symlink(targets[i], HOUSE_FILE));
    House_crashsave(3000);
    TEST_ASSERT_EQUAL_INT(0, lstat(HOUSE_FILE, &st));
    TEST_ASSERT_TRUE(S_ISLNK(st.st_mode));
    TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    TEST_ASSERT_FALSE(exists(HOUSE_FILE ".tmp"));
  }
}

static void test_destroying_a_house_leaves_sibling_files_untouched(void)
{
  char build[] = "build 3000 north owner", destroy[] = "destroy 3000";
  do_hcontrol(NULL, build, 0, 0);
  write_file(HOUSE_FILE ".tmp", "operator's copy");
  do_hcontrol(NULL, destroy, 0, 0);
  TEST_ASSERT_FALSE(exists(HOUSE_FILE));
  assert_file(HOUSE_FILE ".tmp", "operator's copy");
}

static void test_destroying_a_symlinked_house_leaves_both_siblings_untouched(void)
{
  char build[] = "build 3000 north owner", destroy[] = "destroy 3000";
  do_hcontrol(NULL, build, 0, 0);
  remove(HOUSE_FILE);
  write_file(ELSEWHERE_FILE, "saved contents");
  TEST_ASSERT_EQUAL_INT(0, symlink("../" ELSEWHERE_FILE, HOUSE_FILE));
  write_file(HOUSE_FILE ".tmp", "house sibling");
  write_file(ELSEWHERE_FILE ".tmp", "target sibling");
  do_hcontrol(NULL, destroy, 0, 0);
  TEST_ASSERT_FALSE(exists(HOUSE_FILE));
  assert_file(HOUSE_FILE ".tmp", "house sibling");
  assert_file(ELSEWHERE_FILE ".tmp", "target sibling");
}

#ifdef HOUSE_TEST_WRAP_IO
static void test_replacement_starts_private_before_permissions_are_copied(void)
{
  const mode_t masks[] = {0000, 0777};
  struct stat st;
  TEST_ASSERT_EQUAL_INT(0, chmod("house", 0755));
  for (size_t i = 0; i < sizeof(masks) / sizeof(masks[0]); ++i) {
    write_file(HOUSE_FILE, "old contents");
    TEST_ASSERT_EQUAL_INT(0, chmod(HOUSE_FILE, 0640));
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
    umask(masks[i]);
    watch_create = 1;
    saw_private_create = 0;
    House_crashsave(3000);
    umask(0022);
    assert_file(HOUSE_FILE, item.name);
    TEST_ASSERT_TRUE(saw_private_create);
    TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    TEST_ASSERT_EQUAL_INT(0, stat(HOUSE_FILE, &st));
    TEST_ASSERT_EQUAL_UINT(0640, st.st_mode & 0777);
  }
}

static void test_parent_sync_failure_keeps_house_queued_until_retry(void)
{
  char build[] = "build 3000 north owner";
  do_hcontrol(NULL, build, 0, 0);
  TEST_ASSERT_EQUAL_INT(0, rename(HOUSE_FILE, ELSEWHERE_FILE));
  TEST_ASSERT_EQUAL_INT(0, symlink("../" ELSEWHERE_FILE, HOUSE_FILE));
  TEST_ASSERT_EQUAL_INT(0, stat("elsewhere", &watched_parent));
  watch_sync = fail_parent_sync = 1;

  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  item.name = "first update";
  House_save_all();
  assert_file(ELSEWHERE_FILE, "first update");
  TEST_ASSERT_TRUE(parent_syncs > 0);
  TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));

  fail_parent_sync = parent_syncs = 0;
  item.name = "second update";
  House_save_all();
  assert_file(ELSEWHERE_FILE, "second update");
  TEST_ASSERT_TRUE(parent_syncs > 0);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
}
#endif

#if defined(__linux__)
static const unsigned char acl[4 + 5 * 8] = {
    2, 0, 0, 0,                             /* version = 2 */
    0x01, 0, 6, 0, 0xff, 0xff, 0xff, 0xff,  /* ACL_USER_OBJ:  rw- */
    0x02, 0, 6, 0, 0xfe, 0xff, 0x00, 0x00,  /* ACL_USER 65534: rw- */
    0x04, 0, 4, 0, 0xff, 0xff, 0xff, 0xff,  /* ACL_GROUP_OBJ: r-- */
    0x10, 0, 6, 0, 0xff, 0xff, 0xff, 0xff,  /* ACL_MASK:      rw- */
    0x20, 0, 0, 0, 0xff, 0xff, 0xff, 0xff,  /* ACL_OTHER:     --- */
};

static void set_linux_acl(void)
{
  if (setxattr(HOUSE_FILE, "system.posix_acl_access", acl, sizeof(acl), 0) < 0) {
    if (errno == ENOTSUP)
      TEST_IGNORE_MESSAGE("filesystem does not support POSIX ACLs");
    TEST_FAIL_MESSAGE("setxattr failed unexpectedly");
  }
}

#ifdef HOUSE_TEST_WRAP_IO
static void test_group_restore_denied_preserves_named_acl_access(void)
{
  unsigned char expected[sizeof(acl)], saved[sizeof(acl)];
  struct stat st;
  gid_t old_group;
  write_file(HOUSE_FILE, "old contents");
  set_alternate_house_group(&old_group);
  set_linux_acl();
  deny_chown = 1;
  House_crashsave(3000);
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(0, stat(HOUSE_FILE, &st));
  TEST_ASSERT_TRUE(st.st_gid != old_group);
  memcpy(expected, acl, sizeof(acl));
  /* The new owning group is limited to ACL_OTHER. The named user and mask survive. */
  expected[4 + 2 * 8 + 2] = 0;
  TEST_ASSERT_EQUAL_INT(sizeof(acl), getxattr(HOUSE_FILE, "system.posix_acl_access", saved, sizeof(saved)));
  TEST_ASSERT_EQUAL_MEMORY(expected, saved, sizeof(saved));
}
#endif

static void test_linux_acl_preserved_and_inherited_default_acl_cleared(void)
{
  unsigned char saved[sizeof(acl)], inherited[sizeof(acl)];
  ssize_t n;

  /* 1. An explicit POSIX ACL on an existing house file survives a save. */
  write_file(HOUSE_FILE, "old contents");
  set_linux_acl();
  House_crashsave(3000);
  assert_file(HOUSE_FILE, item.name);
  n = getxattr(HOUSE_FILE, "system.posix_acl_access", saved, sizeof(saved));
  TEST_ASSERT_EQUAL_INT((int)sizeof(acl), (int)n);
  TEST_ASSERT_EQUAL_MEMORY(acl, saved, sizeof(acl));

  /* 2. A house file with no ACL does not pick up its parent's default ACL. */
  remove(HOUSE_FILE);
  write_file(HOUSE_FILE, "old contents");
  TEST_ASSERT_EQUAL_INT(0, chmod(HOUSE_FILE, 0600));
  TEST_ASSERT_EQUAL_INT(0, setxattr("house", "system.posix_acl_default", acl, sizeof(acl), 0));
  House_crashsave(3000);
  assert_file(HOUSE_FILE, item.name);
  n = getxattr(HOUSE_FILE, "system.posix_acl_access", saved, sizeof(saved));
  TEST_ASSERT_EQUAL_INT(-1, (int)n);
  TEST_ASSERT_EQUAL_INT(ENODATA, errno);

  /* 3. A first save inherits the same effective ACL as an ordinary new file. */
  TEST_ASSERT_EQUAL_INT(0, remove(HOUSE_FILE));
  umask(0077);
  write_file(LIB_HOUSE "reference", "reference");
  House_crashsave(3000);
  assert_file(HOUSE_FILE, item.name);
  n = getxattr(LIB_HOUSE "reference", "system.posix_acl_access", inherited, sizeof(inherited));
  TEST_ASSERT_EQUAL_INT((int)sizeof(inherited), (int)n);
  n = getxattr(HOUSE_FILE, "system.posix_acl_access", saved, sizeof(saved));
  TEST_ASSERT_EQUAL_INT((int)sizeof(inherited), (int)n);
  TEST_ASSERT_EQUAL_MEMORY(inherited, saved, sizeof(inherited));
}
#elif defined(__FreeBSD__)
static void test_freebsd_keeps_an_existing_named_acl(void)
{
  acl_type_t type;
  acl_t requested, before, after;
  const char *text;
  int different, set_result;
  write_file(HOUSE_FILE, "old contents");
  if (pathconf(HOUSE_FILE, _PC_ACL_NFS4) > 0) {
    type = ACL_TYPE_NFS4;
    text = "owner@:full_set::allow,user:65534:read_set::allow,group@:read_set::allow";
  } else if (pathconf(HOUSE_FILE, _PC_ACL_EXTENDED) > 0) {
    type = ACL_TYPE_ACCESS;
    text = "user::rw-,user:65534:rw-,group::r--,mask::rw-,other::---";
  } else {
    TEST_IGNORE_MESSAGE("filesystem has no extended ACL support");
    return;
  }
  requested = acl_from_text(text);
  TEST_ASSERT_NOT_NULL(requested);
  set_result = acl_set_file(HOUSE_FILE, type, requested);
  acl_free(requested);
  TEST_ASSERT_EQUAL_INT(0, set_result);
  before = acl_get_file(HOUSE_FILE, type);
  TEST_ASSERT_NOT_NULL(before);
  House_crashsave(3000);
  after = acl_get_file(HOUSE_FILE, type);
  different = after ? acl_cmp_np(before, after) : -1;
  acl_free(before);
  if (after)
    acl_free(after);
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(0, different);
}
#endif

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
  RUN_TEST(test_saving_leaves_sibling_files_untouched);
  RUN_TEST(test_saving_leaves_sibling_links_untouched);
  RUN_TEST(test_symlinked_house_file_is_saved_where_it_points);
  RUN_TEST(test_dangling_symlink_house_file_creates_its_target);
  RUN_TEST(test_broken_symlink_house_file_is_not_replaced);
  RUN_TEST(test_destroying_a_house_leaves_sibling_files_untouched);
  RUN_TEST(test_destroying_a_symlinked_house_leaves_both_siblings_untouched);
#ifdef HOUSE_TEST_WRAP_IO
  RUN_TEST(test_replacement_starts_private_before_permissions_are_copied);
  RUN_TEST(test_parent_sync_failure_keeps_house_queued_until_retry);
  RUN_TEST(test_group_restore_denied_preserves_named_acl_access);
#endif
#if defined(__linux__)
  RUN_TEST(test_linux_acl_preserved_and_inherited_default_acl_cleared);
#elif defined(__FreeBSD__)
  RUN_TEST(test_freebsd_keeps_an_existing_named_acl);
#endif
  result = UNITY_END();
  if (rmdir("house") || rmdir("etc") || rmdir("elsewhere") || chdir("/") || rmdir(scratch))
    return 1;
  return result;
}
