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

#include <dirent.h>
#include <fcntl.h>
#include <glob.h>
#include <limits.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#if defined(__linux__)
#include <sys/xattr.h>
#elif defined(__FreeBSD__)
#include <sys/acl.h>
#endif

#define HOUSE_FILE LIB_HOUSE "3000.house"
#define ELSEWHERE_FILE "elsewhere/3000.house"
#define LOCK_NAME ".tba-house.lock"
#define LOCK_PATH LIB_HOUSE LOCK_NAME
#define LOCK_GUARD LOCK_PATH "/guard"
#define STAGE_PREFIX ".tba-house-save-"
static struct room_data rooms[2];
static struct obj_data item;
static int fail_record, crash_record, boot_during_record;
static int observe_stage, unlocked_during_record;
static int count_unlocked_stages(void);

#ifdef HOUSE_TEST_WRAP_IO
/* Fail only the sync of the installed file's parent. File syncs and other
 * directories still use the filesystem normally. */
static struct stat watched_parent;
static int watch_sync, fail_parent_sync, parent_sync_errno, parent_syncs;
int __real_fsync(int fd);
static int watch_create, saw_private_create;
int __real_openat(int dirfd, const char *name, int flags, ...);
static int deny_chown;
int __real_fchown(int fd, uid_t owner, gid_t group);
static int reject_named_chmod, following_chmods;
int __real_fchmodat(int dirfd, const char *name, mode_t mode, int flags);
static int fail_lock_chmod, attempted_lock_chmod, visible_before_chmod;
static int observe_publication, published_dir_mode, published_guard_mode;
static int report_publication_enoent;
int __real_fchmod(int fd, mode_t mode);
int __real_renameat(int from_fd, const char *from, int to_fd, const char *to);

int __wrap_fchmod(int fd, mode_t mode)
{
  struct stat st;
  if (fail_lock_chmod && mode == 0600 && fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
    attempted_lock_chmod = TRUE;
    visible_before_chmod = lstat(LOCK_PATH, &st) == 0;
    errno = EIO;
    return -1;
  }
  return __real_fchmod(fd, mode);
}

int __wrap_renameat(int from_fd, const char *from, int to_fd, const char *to)
{
  int result = __real_renameat(from_fd, from, to_fd, to);
  if (observe_publication && result == 0 && !strcmp(to, LOCK_NAME)) {
    struct stat st;
    if (fstatat(to_fd, to, &st, AT_SYMLINK_NOFOLLOW) == 0 && S_ISDIR(st.st_mode))
      published_dir_mode = st.st_mode & 0777;
    if (fstatat(to_fd, LOCK_NAME "/guard", &st, AT_SYMLINK_NOFOLLOW) == 0 && S_ISREG(st.st_mode))
      published_guard_mode = st.st_mode & 0777;
    if (report_publication_enoent) {
      /* NFS may install the directory even though the caller sees ENOENT. */
      report_publication_enoent = FALSE;
      errno = ENOENT;
      return -1;
    }
  }
  return result;
}

int __wrap_fchmodat(int dirfd, const char *name, mode_t mode, int flags)
{
  if (reject_named_chmod && *name) {
    if (!(flags & AT_SYMLINK_NOFOLLOW))
      ++following_chmods;
    errno = EOPNOTSUPP;
    return -1;
  }
  return __real_fchmodat(dirfd, name, mode, flags);
}

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
      errno = parent_sync_errno ? parent_sync_errno : EIO;
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
/* House_boot() loads the houses it finds; these tests give it none. */
void objsave_reassemble(obj_save_data *loaded,
                        void (*place)(struct obj_data *obj, void *dest), void *dest) {}
void obj_to_room(struct obj_data *object, room_rnum room) {}
void extract_obj(struct obj_data *obj) {}
obj_rnum real_object(obj_vnum vnum) { return NOTHING; }
struct obj_data *read_object(obj_vnum vnum, int type) { return NULL; }

int objsave_save_obj_record(struct obj_data *obj, FILE *fp, int location)
{
  if (observe_stage)
    unlocked_during_record = count_unlocked_stages();
  if (crash_record)
    _exit(1);	/* die part-way through the save, as a crash would */
  if (boot_during_record) {
    boot_during_record = FALSE;
    House_boot();  /* Opens the directory independently of the active save. */
  }
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
  boot_during_record = FALSE;
  observe_stage = FALSE;
  unlocked_during_record = -1;
}

void tearDown(void)
{
  char running[64];
#ifdef HOUSE_TEST_WRAP_IO
  watch_sync = fail_parent_sync = parent_sync_errno = parent_syncs = 0;
  watch_create = saw_private_create = deny_chown = 0;
  reject_named_chmod = following_chmods = 0;
  fail_lock_chmod = attempted_lock_chmod = visible_before_chmod = 0;
  observe_publication = report_publication_enoent = 0;
  published_dir_mode = published_guard_mode = -1;
#endif
  if (ROOM_FLAGGED(0, ROOM_HOUSE)) {
    char command[] = "destroy 3000";
    do_hcontrol(NULL, command, 0, 0);
  }
  umask(0022);
  chmod("house", 0700);
  remove(LOCK_PATH "/marker");  /* Only the unlocked-save fixture creates this. */
  remove(LOCK_GUARD);
  remove(LOCK_PATH);
  remove(HCONTROL_FILE);
  remove(HOUSE_FILE);
  remove(HOUSE_FILE ".tmp");
  remove(ELSEWHERE_FILE);
  remove(ELSEWHERE_FILE ".tmp");
  remove("bystander");
  snprintf(running, sizeof(running), LIB_HOUSE ".tba-house-%ld-0", (long)getppid());
  remove(running);
  remove(LIB_HOUSE STAGE_PREFIX "notes");
  remove(LIB_HOUSE STAGE_PREFIX "4294967296-0");
  remove(LIB_HOUSE STAGE_PREFIX "99999999999999999999-0");
#if defined(__linux__)
  removexattr("house", "system.posix_acl_default");
  remove(LIB_HOUSE "reference");
#elif defined(__FreeBSD__)
  remove("elsewhere/" HOUSE_FILE);
  remove("elsewhere/house/.tba-house.lock/guard");
  remove("elsewhere/house/.tba-house.lock");
  rmdir("elsewhere/house");
  chmod("elsewhere", 0700);
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
  if (length > 0)
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

#if defined(__APPLE__) && defined(__MACH__)
static void test_darwin_stage_recovery_under_restrictive_umask(void)
{
  if (geteuid() == 0)
    TEST_IGNORE_MESSAGE("requires an unprivileged owner to exercise directory permissions");
  write_file(HOUSE_FILE, "previous contents");
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  umask(0777);
  House_crashsave(3000);
  umask(0022);
  if (ROOM_FLAGGED(0, ROOM_HOUSE_CRASH)) {
    assert_file(HOUSE_FILE, "previous contents");
    House_crashsave(3000);
  }
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));

#ifdef O_SEARCH
  /* Owner search permission is enough to acquire a directory descriptor. */
  item.name = "searchable stage";
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  umask(0400);
  House_crashsave(3000);
  umask(0022);
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
#endif
}
#endif

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

static void test_symlink_to_lock_guard_preserves_guard_and_pending_save(void)
{
  char build[] = "build 3000 north owner";
  struct stat before, after, house_link;

  do_hcontrol(NULL, build, 0, 0);
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &before));
  TEST_ASSERT_EQUAL_INT(0, unlink(HOUSE_FILE));
  TEST_ASSERT_EQUAL_INT(0, symlink(LOCK_NAME "/guard", HOUSE_FILE));
  item.name = "pending contents";
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_save_all();

  TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  TEST_ASSERT_EQUAL_INT(0, lstat(HOUSE_FILE, &house_link));
  TEST_ASSERT_TRUE(S_ISLNK(house_link.st_mode));
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &after));
  TEST_ASSERT_TRUE(before.st_dev == after.st_dev && before.st_ino == after.st_ino);
  assert_file(LOCK_GUARD, "");

  TEST_ASSERT_EQUAL_INT(0, unlink(HOUSE_FILE));
  House_save_all();
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
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

static int count_stages(void)
{
  DIR *dir = opendir("house");
  struct dirent *entry;
  int count = 0;
  TEST_ASSERT_NOT_NULL(dir);
  while ((entry = readdir(dir)) != NULL)
    count += !strncmp(entry->d_name, STAGE_PREFIX, sizeof(STAGE_PREFIX) - 1);
  closedir(dir);
  return count;
}

/* The child dies part-way through writing the house, and its stage stays. */
static pid_t crash_in_the_middle_of_a_save(void)
{
  int status;
  pid_t child = fork();
  TEST_ASSERT_TRUE(child >= 0);
  if (child == 0) {
    crash_record = TRUE;
    House_crashsave(3000);
    _exit(0);
  }
  TEST_ASSERT_EQUAL_INT(child, waitpid(child, &status, 0));
  TEST_ASSERT_TRUE(WIFEXITED(status) && WEXITSTATUS(status) == 1);
  return child;
}

static void test_boot_defers_cleanup_while_a_save_is_active(void)
{
  char updates[][16] = {"first save", "replacement"};
  crash_in_the_middle_of_a_save();
  for (size_t i = 0; i < sizeof(updates) / sizeof(updates[0]); ++i) {
    item.name = updates[i];
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
    boot_during_record = TRUE;
    House_crashsave(3000);
    assert_file(HOUSE_FILE, updates[i]);
    TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    /* Even the abandoned stage must wait until the writer releases its lock. */
    TEST_ASSERT_EQUAL_INT(1, count_stages());
  }
  House_boot();
  TEST_ASSERT_EQUAL_INT(0, count_stages());
}

static void test_boot_keeps_noncanonical_stage_names(void)
{
  char paths[5][128];
  int preserved = 0;
  long pid = (long)crash_in_the_middle_of_a_save();
  snprintf(paths[0], sizeof(paths[0]), LIB_HOUSE STAGE_PREFIX "0%ld-1", pid);
  snprintf(paths[1], sizeof(paths[1]), LIB_HOUSE STAGE_PREFIX "%ld-01", pid);
  snprintf(paths[2], sizeof(paths[2]), LIB_HOUSE STAGE_PREFIX "%ld-%lu0", pid, ULONG_MAX);
  snprintf(paths[3], sizeof(paths[3]), LIB_HOUSE ".tba-house-%ld-0", pid);
  snprintf(paths[4], sizeof(paths[4]), LIB_HOUSE ".tba-house-unlocked-%ld-0", pid);
  for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i)
    write_file(paths[i], "operator's copy");

  House_boot();
  for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
    preserved += exists(paths[i]);
    remove(paths[i]);
  }
  TEST_ASSERT_EQUAL_INT(5, preserved);
  TEST_ASSERT_EQUAL_INT(0, count_stages());
}

static int count_unlocked_stages(void)
{
  glob_t paths;
  int count, result = glob(LIB_HOUSE ".tba-house-unlocked-*", 0, NULL, &paths);
  if (result == GLOB_NOMATCH)
    return 0;
  TEST_ASSERT_EQUAL_INT(0, result);
  count = (int)paths.gl_pathc;
  globfree(&paths);
  return count;
}

static void test_multiply_linked_lock_leaves_sibling_permissions_alone(void)
{
  const mode_t modes[] = {0640, 0000};
  struct stat before, after;
  TEST_ASSERT_FALSE(exists(LOCK_PATH));
  write_file("bystander", "operator's copy");
  TEST_ASSERT_EQUAL_INT(0, link("bystander", LOCK_PATH));
  TEST_ASSERT_EQUAL_INT(0, stat("bystander", &before));
  observe_stage = TRUE;

  for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
    TEST_ASSERT_EQUAL_INT(0, chmod("bystander", modes[i]));
    unlocked_during_record = -1;
    House_boot();
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
    House_crashsave(3000);
    assert_file(HOUSE_FILE, item.name);
    TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    TEST_ASSERT_EQUAL_INT(1, unlocked_during_record);
    TEST_ASSERT_EQUAL_INT(0, count_unlocked_stages());
    TEST_ASSERT_EQUAL_INT(0, lstat(LOCK_PATH, &after));
    TEST_ASSERT_TRUE(after.st_dev == before.st_dev && after.st_ino == before.st_ino);
    TEST_ASSERT_EQUAL_UINT(modes[i], after.st_mode & 0777);
  }
  TEST_ASSERT_EQUAL_INT(0, chmod("bystander", 0600));
  assert_file("bystander", "operator's copy");
  TEST_ASSERT_EQUAL_INT(0, unlink(LOCK_PATH));
}

#if defined(__linux__) || (defined(__FreeBSD__) && defined(O_PATH) && defined(AT_EMPTY_PATH) && defined(O_EMPTY_PATH))
static void test_boot_repairs_an_inaccessible_legacy_lock_and_sweeps_its_stage(void)
{
  struct stat before, after;
  write_file(LOCK_PATH, "legacy guard");
  write_file(HOUSE_FILE, "old contents");
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_PATH, &before));
  crash_in_the_middle_of_a_save();
  TEST_ASSERT_EQUAL_INT(1, count_stages());
  TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_PATH, 0000));

  House_boot();

  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_PATH, &after));
  TEST_ASSERT_TRUE(before.st_dev == after.st_dev && before.st_ino == after.st_ino);
  TEST_ASSERT_EQUAL_UINT(0600, after.st_mode & 0777);
  TEST_ASSERT_EQUAL_INT(0, count_stages());
  assert_file(LOCK_PATH, "legacy guard");
  assert_file(HOUSE_FILE, "old contents");
}
#endif

#ifdef HOUSE_TEST_WRAP_IO
static void test_lock_is_secured_before_publication_and_retries_after_failure(void)
{
  struct stat st;
  TEST_ASSERT_FALSE(exists(LOCK_PATH));
  fail_lock_chmod = TRUE;
  observe_stage = TRUE;
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  fail_lock_chmod = FALSE;
  TEST_ASSERT_TRUE(attempted_lock_chmod);
  TEST_ASSERT_FALSE(visible_before_chmod);
  TEST_ASSERT_FALSE(exists(LOCK_PATH));
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  TEST_ASSERT_EQUAL_INT(1, unlocked_during_record);

  observe_publication = TRUE;
  report_publication_enoent = TRUE;
  published_dir_mode = published_guard_mode = -1;
  unlocked_during_record = -1;
  item.name = "contents after retry";
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  umask(0777);
  House_crashsave(3000);
  umask(0022);
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  TEST_ASSERT_EQUAL_INT(0700, published_dir_mode);
  TEST_ASSERT_EQUAL_INT(0600, published_guard_mode);
  TEST_ASSERT_FALSE(report_publication_enoent);
  TEST_ASSERT_EQUAL_INT(0, unlocked_during_record);
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &st));
  TEST_ASSERT_EQUAL_UINT(1, st.st_nlink);
}
#endif

/* Read access is enough to hold the lock, so holding it must not stop a
 * save: the save goes ahead without it and leaves nothing behind. */
static void check_save_with_exclusively_held_lock(const char *lock)
{
  char build[] = "build 3000 north owner";
  struct stat before, after;
  int fd, locked, saved;
  do_hcontrol(NULL, build, 0, 0);
  TEST_ASSERT_EQUAL_INT(0, stat(lock, &before));
  fd = open(lock, O_RDONLY);
  TEST_ASSERT_TRUE(fd >= 0);
  locked = flock(fd, LOCK_EX | LOCK_NB);
  if (locked < 0) {
    close(fd);
    TEST_FAIL_MESSAGE("unable to acquire the cleanup lock");
  }
  item.name = "pending update";
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  observe_stage = TRUE;
  House_save_all();
  saved = !ROOM_FLAGGED(0, ROOM_HOUSE_CRASH);
  TEST_ASSERT_EQUAL_INT(0, close(fd));
  TEST_ASSERT_TRUE(saved);
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(1, unlocked_during_record);
  TEST_ASSERT_EQUAL_INT(0, stat(lock, &after));
  TEST_ASSERT_TRUE(before.st_dev == after.st_dev && before.st_ino == after.st_ino);
  TEST_ASSERT_EQUAL_INT(0, count_stages());
  TEST_ASSERT_EQUAL_INT(0, count_unlocked_stages());
}

static void test_a_held_lock_does_not_stop_a_save(void)
{
  check_save_with_exclusively_held_lock(LOCK_GUARD);
}

static void test_held_legacy_lock_preserves_its_inode_and_uses_unlocked_stage(void)
{
  write_file(LOCK_PATH, "legacy guard");
  check_save_with_exclusively_held_lock(LOCK_PATH);
  assert_file(LOCK_PATH, "legacy guard");
}

/* Other accounts cannot open the lock to hold it.  A lock created under a
 * restrictive umask or previously left non-0600 (0644 or 0400) goes back to
 * 0600 at the next save, even while it is held. */
static void test_the_lock_file_is_private(void)
{
  struct stat st;
  int fd, held;
  TEST_ASSERT_FALSE(exists(LOCK_PATH));
  umask(0077);
  House_crashsave(3000);
  umask(0022);
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &st));
  TEST_ASSERT_EQUAL_UINT(0600, st.st_mode & 0777);

  TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_GUARD, 0644));
  fd = open(LOCK_GUARD, O_RDONLY);
  TEST_ASSERT_TRUE(fd >= 0);
  held = flock(fd, LOCK_EX | LOCK_NB) == 0;
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_EQUAL_INT(0, close(fd));
  TEST_ASSERT_TRUE(held);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &st));
  TEST_ASSERT_EQUAL_UINT(0600, st.st_mode & 0777);

  /* An owner-owned lock left read-only (0400) is also restored to 0600. */
  TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_GUARD, 0400));
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &st));
  TEST_ASSERT_EQUAL_UINT(0600, st.st_mode & 0777);
}

/* A lock file the game can only read -- one root left behind -- still lets
 * boot clear a crashed save. */
static void test_boot_sweeps_with_a_lock_file_it_can_only_read(void)
{
  int stages;
  /* Older versions kept the guard directly at the public name. */
  write_file(LOCK_PATH, "");
  write_file(HOUSE_FILE, "old contents");
  crash_in_the_middle_of_a_save();
  TEST_ASSERT_EQUAL_INT(1, count_stages());
  TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_PATH, 0400));
  House_boot();
  stages = count_stages();
  TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_PATH, 0600));
  TEST_ASSERT_EQUAL_INT(0, stages);
  assert_file(HOUSE_FILE, "old contents");
}

static void test_boot_leaves_an_interrupted_unlocked_save_untouched(void)
{
  char pattern[96], contents[256];
  glob_t paths;
  pid_t child;
  int length;
  TEST_ASSERT_EQUAL_INT(0, mkdir(LOCK_PATH, 0700));
  write_file(LOCK_PATH "/marker", "operator's copy");
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));

  child = crash_in_the_middle_of_a_save();
  assert_file(LOCK_PATH "/marker", "operator's copy");
  TEST_ASSERT_EQUAL_INT(0, unlink(LOCK_PATH "/marker"));
  TEST_ASSERT_EQUAL_INT(0, rmdir(LOCK_PATH));
  /* Locate this child's one abandoned replacement in the isolated fixture. */
  snprintf(pattern, sizeof(pattern), LIB_HOUSE ".tba-house-unlocked-%ld-*", (long)child);
  TEST_ASSERT_EQUAL_INT(0, glob(pattern, 0, NULL, &paths));
  TEST_ASSERT_EQUAL_UINT(1, paths.gl_pathc);
  length = snprintf(contents, sizeof(contents), "%s/contents", paths.gl_pathv[0]);
  TEST_ASSERT_TRUE(length > 0 && (size_t)length < sizeof(contents));

  House_boot();
  assert_file(HOUSE_FILE, item.name);
  assert_file(contents, "");
  TEST_ASSERT_EQUAL_INT(0, unlink(contents));
  TEST_ASSERT_EQUAL_INT(0, rmdir(paths.gl_pathv[0]));
  globfree(&paths);
}

/* Boot clears a crashed first save and replacement, preserving legacy stages
 * and unrelated names. */
static void test_boot_removes_saves_a_crash_left_unfinished(void)
{
  char running[64];
  crash_in_the_middle_of_a_save();
  write_file(HOUSE_FILE, "old contents");
  crash_in_the_middle_of_a_save();
  TEST_ASSERT_EQUAL_INT(2, count_stages());
  assert_file(HOUSE_FILE, "old contents");

  snprintf(running, sizeof(running), LIB_HOUSE ".tba-house-%ld-0", (long)getppid());
  write_file(running, "a running game's save");
  write_file(LIB_HOUSE STAGE_PREFIX "notes", "operator's copy");
  write_file(LIB_HOUSE STAGE_PREFIX "4294967296-0", "out-of-range pid 1");
  write_file(LIB_HOUSE STAGE_PREFIX "99999999999999999999-0", "out-of-range pid 2");
  House_boot();
  TEST_ASSERT_EQUAL_INT(3, count_stages());
  assert_file(running, "a running game's save");
  assert_file(LIB_HOUSE STAGE_PREFIX "notes", "operator's copy");
  assert_file(LIB_HOUSE STAGE_PREFIX "4294967296-0", "out-of-range pid 1");
  assert_file(LIB_HOUSE STAGE_PREFIX "99999999999999999999-0", "out-of-range pid 2");
  assert_file(HOUSE_FILE, "old contents");
}

#ifdef HOUSE_TEST_WRAP_IO
static void test_boot_clears_an_empty_unreadable_stage_without_chmod(void)
{
  char empty[128];
  long pid = (long)crash_in_the_middle_of_a_save();
  /* A crash between mkdirat() and permission setup leaves this empty. */
  snprintf(empty, sizeof(empty), LIB_HOUSE STAGE_PREFIX "%ld-%lu", pid, ULONG_MAX);
  TEST_ASSERT_EQUAL_INT(0, mkdir(empty, 0000));
  reject_named_chmod = 1;
  House_boot();
  TEST_ASSERT_EQUAL_INT(0, following_chmods);
  TEST_ASSERT_FALSE(exists(empty));
  TEST_ASSERT_EQUAL_INT(0, count_stages());
}

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
    reject_named_chmod = TRUE;
    watch_create = 1;
    saw_private_create = 0;
    House_crashsave(3000);
    umask(0022);
    assert_file(HOUSE_FILE, item.name);
    TEST_ASSERT_TRUE(saw_private_create);
    TEST_ASSERT_EQUAL_INT(0, following_chmods);
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

  /* EINVAL means the filesystem does not support directory fsync; do not
   * leave the house queued in an endless retry loop. */
  fail_parent_sync = 1;
  parent_sync_errno = EINVAL;
  parent_syncs = 0;
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  item.name = "third update";
  House_save_all();
  assert_file(ELSEWHERE_FILE, "third update");
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

static void test_freebsd_old_owner_deny_preserves_game_read_access(void)
{
  const uid_t game = 65534;
  const char *file = "elsewhere/" HOUSE_FILE;
  acl_t acl;
  pid_t child;
  int status, result;

  if (geteuid() != 0 || pathconf("elsewhere", _PC_ACL_NFS4) <= 0)
    TEST_IGNORE_MESSAGE("requires root on a FreeBSD NFSv4 ACL filesystem");

  /* The outer test directory stays private. The child can use this subtree
   * as its working directory after dropping privileges. */
  TEST_ASSERT_EQUAL_INT(0, mkdir("elsewhere/house", 0700));
  TEST_ASSERT_EQUAL_INT(0, chown("elsewhere/house", game, (gid_t)-1));
  TEST_ASSERT_EQUAL_INT(0, chmod("elsewhere", 0711));
  write_file(file, "old contents");
  acl = acl_from_text("owner@:r::deny,user:65534:full_set::allow,owner@:full_set::allow");
  TEST_ASSERT_NOT_NULL(acl);
  result = acl_set_file(file, ACL_TYPE_NFS4, acl);
  acl_free(acl);
  TEST_ASSERT_EQUAL_INT(0, result);

  child = fork();
  TEST_ASSERT_TRUE(child >= 0);
  if (child == 0) {
    FILE *fp;
    if (chdir("elsewhere") < 0 || setuid(game) < 0 || access(HOUSE_FILE, R_OK) < 0)
      _exit(1);
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
    House_crashsave(3000);
    if (ROOM_FLAGGED(0, ROOM_HOUSE_CRASH) || !(fp = fopen(HOUSE_FILE, "rb")))
      _exit(2);
    _exit(fclose(fp) == 0 ? 0 : 3);
  }
  TEST_ASSERT_EQUAL_INT(child, waitpid(child, &status, 0));
  TEST_ASSERT_TRUE(WIFEXITED(status));
  TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
  assert_file(file, item.name);
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
#if defined(__APPLE__) && defined(__MACH__)
  RUN_TEST(test_darwin_stage_recovery_under_restrictive_umask);
#endif
  RUN_TEST(test_existing_house_keeps_a_group_the_game_is_in);
  RUN_TEST(test_partial_record_preserves_previous_save);
  RUN_TEST(test_saving_leaves_sibling_files_untouched);
  RUN_TEST(test_saving_leaves_sibling_links_untouched);
  RUN_TEST(test_symlinked_house_file_is_saved_where_it_points);
  RUN_TEST(test_symlink_to_lock_guard_preserves_guard_and_pending_save);
  RUN_TEST(test_dangling_symlink_house_file_creates_its_target);
  RUN_TEST(test_broken_symlink_house_file_is_not_replaced);
  RUN_TEST(test_destroying_a_house_leaves_sibling_files_untouched);
  RUN_TEST(test_destroying_a_symlinked_house_leaves_both_siblings_untouched);
  RUN_TEST(test_boot_removes_saves_a_crash_left_unfinished);
  RUN_TEST(test_boot_defers_cleanup_while_a_save_is_active);
  RUN_TEST(test_boot_keeps_noncanonical_stage_names);
  RUN_TEST(test_a_held_lock_does_not_stop_a_save);
  RUN_TEST(test_held_legacy_lock_preserves_its_inode_and_uses_unlocked_stage);
  RUN_TEST(test_the_lock_file_is_private);
  RUN_TEST(test_multiply_linked_lock_leaves_sibling_permissions_alone);
#if defined(__linux__) || (defined(__FreeBSD__) && defined(O_PATH) && defined(AT_EMPTY_PATH) && defined(O_EMPTY_PATH))
  RUN_TEST(test_boot_repairs_an_inaccessible_legacy_lock_and_sweeps_its_stage);
#endif
  RUN_TEST(test_boot_sweeps_with_a_lock_file_it_can_only_read);
  RUN_TEST(test_boot_leaves_an_interrupted_unlocked_save_untouched);
#ifdef HOUSE_TEST_WRAP_IO
  RUN_TEST(test_lock_is_secured_before_publication_and_retries_after_failure);
  RUN_TEST(test_boot_clears_an_empty_unreadable_stage_without_chmod);
  RUN_TEST(test_replacement_starts_private_before_permissions_are_copied);
  RUN_TEST(test_parent_sync_failure_keeps_house_queued_until_retry);
#if defined(__linux__)
  RUN_TEST(test_group_restore_denied_preserves_named_acl_access);
#endif
#endif
#if defined(__linux__)
  RUN_TEST(test_linux_acl_preserved_and_inherited_default_acl_cleared);
#elif defined(__FreeBSD__)
  RUN_TEST(test_freebsd_keeps_an_existing_named_acl);
  RUN_TEST(test_freebsd_old_owner_deny_preserves_game_read_access);
#endif
  result = UNITY_END();
  remove("elsewhere/.tba-house.lock/guard");
  remove("elsewhere/.tba-house.lock");
  if (rmdir("house") || rmdir("etc") || rmdir("elsewhere") || chdir("/") || rmdir(scratch))
    return 1;
  return result;
}
