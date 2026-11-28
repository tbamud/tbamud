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
#include <inttypes.h>
#include <limits.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#if defined(__linux__)
#include <linux/magic.h>
#include <sys/sysmacros.h>
#include <sys/vfs.h>
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
static int fail_record, crash_record;
static void (*during_record)(void);
static int observe_stage, unlocked_during_record;
static int count_unlocked_stages(void);

#ifdef HOUSE_TEST_WRAP_IO
/* Fail only the sync of the installed file's parent. File syncs and other
 * directories still use the filesystem normally. */
static struct stat watched_parent;
static int watch_sync, fail_parent_sync, parent_sync_errno, parent_syncs;
int __real_fsync(int fd);
static int fail_volume_sync;
int __real_syncfs(int fd);
static int watch_create, saw_private_create;
int __real_openat(int dirfd, const char *name, int flags, ...);
static int deny_chown;
int __real_fchown(int fd, uid_t owner, gid_t group);
static int reject_named_chmod, following_chmods;
int __real_fchmodat(int dirfd, const char *name, mode_t mode, int flags);
static int fail_lock_chmod, attempted_lock_chmod, visible_before_chmod;
static int observe_publication, published_dir_mode, published_guard_mode;
static int report_publication_enoent;
static int boot_before_guard_create, candidate_was_removed;
static int replace_emptied_lock_on_guard_open;
static int require_writable_lock, exclusive_access_mode;
static unsigned int mock_fs_type;
static int mock_statfs_errno;
static const char *mock_mount_record;
static int mock_nfs4_version, fail_nfs4_copy, reject_stage_read;
static int nfs4_acl_read_errno, nfs4_readable_source = -1;
static const unsigned char *nfs4_mode_acl;
static size_t nfs4_mode_acl_size;
static int nfs4_sacl_get_errno, nfs4_sacl_set_errno, nfs4_sacl_sets;
static int inherit_audit_sacl;
/* Protected SACL recording successful reads by sam@lab. */
static const unsigned char audit_sacl[] = {
  0,0,0,2, 0,0,0,1,
  0,0,0,2, 0,0,0,16, 0,0,0,1, 0,0,0,7, 's','a','m','@','l','a','b',0
};
ssize_t __real_getxattr(const char *, const char *, void *, size_t);
ssize_t __real_fgetxattr(int, const char *, void *, size_t);
int __real_fsetxattr(int, const char *, const void *, size_t, int);
int __real_fchmod(int fd, mode_t mode);
int __real_renameat(int from_fd, const char *from, int to_fd, const char *to);
int __real_flock(int fd, int operation);
int __real_fstatfs(int fd, struct statfs *out);
FILE *__real_fopen(const char *path, const char *mode);

FILE *__wrap_fopen(const char *path, const char *mode)
{
  if (mock_mount_record && !strcmp(path, "/proc/self/mountinfo")) {
    FILE *file = tmpfile();
    if (file) {
      fprintf(file, "7 1 %u:%u / /arbitrary/alias rw - %s\n",
              major(watched_parent.st_dev), minor(watched_parent.st_dev), mock_mount_record);
      rewind(file);
    }
    return file;
  }
  return __real_fopen(path, mode);
}

/* Exercise both Linux NFS wire formats using ordinary per-file xattrs. */
static const char *test_nfs4_attr(const char *name)
{
  if (mock_nfs4_version == 4 && !strcmp(name, "system.nfs4_acl"))
    return "user.tba_acl";
  if (mock_nfs4_version == 41 && !strcmp(name, "system.nfs4_dacl"))
    return "user.tba_dacl";
  if (mock_nfs4_version == 41 && !strcmp(name, "system.nfs4_sacl"))
    return "user.tba_sacl";
  return NULL;
}

ssize_t __wrap_getxattr(const char *path, const char *name, void *value, size_t size)
{
  const char *mapped = test_nfs4_attr(name);
  if (mapped && value && nfs4_acl_read_errno && strcmp(name, "system.nfs4_sacl")) {
    errno = nfs4_acl_read_errno;
    return -1;
  }
  if (mapped && nfs4_readable_source >= 0 && !strcmp(path, HOUSE_FILE))
    return __real_fgetxattr(nfs4_readable_source, mapped, value, size);
  if (mapped && nfs4_sacl_get_errno && !strcmp(name, "system.nfs4_sacl")) {
    errno = nfs4_sacl_get_errno;
    return -1;
  }
  return __real_getxattr(path, mapped ? mapped : name, value, size);
}

ssize_t __wrap_fgetxattr(int fd, const char *name, void *value, size_t size)
{
  const char *mapped = test_nfs4_attr(name);
  if (nfs4_mode_acl && mapped && strcmp(name, "system.nfs4_sacl")) {
    if (value && size < nfs4_mode_acl_size) {
      errno = ERANGE;
      return -1;
    }
    if (value)
      memcpy(value, nfs4_mode_acl, nfs4_mode_acl_size);
    return (ssize_t)nfs4_mode_acl_size;
  }
  if (mapped && !value) /* Even newly created objects have an access ACL. */
    return 4;
  return __real_fgetxattr(fd, mapped ? mapped : name, value, size);
}

int __wrap_fsetxattr(int fd, const char *name, const void *value, size_t size, int flags)
{
  const char *mapped = test_nfs4_attr(name);
  if (mapped && !strcmp(name, "system.nfs4_sacl")) {
    ++nfs4_sacl_sets;
    if (nfs4_sacl_set_errno) {
      errno = nfs4_sacl_set_errno;
      return -1;
    }
  }
  if (mapped && fail_nfs4_copy && size > 32) {
    errno = EACCES;
    return -1;
  }
  return __real_fsetxattr(fd, mapped ? mapped : name, value, size, flags);
}

int __wrap_fstatfs(int fd, struct statfs *out)
{
  struct stat dir;
  int result = __real_fstatfs(fd, out);
  if (result == 0 && (mock_fs_type || mock_statfs_errno) && fstat(fd, &dir) == 0 &&
      dir.st_dev == watched_parent.st_dev && dir.st_ino == watched_parent.st_ino) {
    if (mock_statfs_errno) {
      errno = mock_statfs_errno;
      return -1;
    }
    out->f_type = mock_fs_type;
  }
  return result;
}

int __wrap_flock(int fd, int operation)
{
  if (require_writable_lock && (operation & LOCK_EX)) {
    exclusive_access_mode = fcntl(fd, F_GETFL) & O_ACCMODE;
    if (exclusive_access_mode != O_RDWR) {
      errno = EBADF;
      return -1;
    }
  }
  return __real_flock(fd, operation);
}

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

/* Simulate a filesystem that refuses to rename a directory with an open child. */
static int windows_rename_rules;
static int has_open_child(int from_fd, const char *from)
{
  struct stat child, open_file;
  struct dirent *entry, *slot;
  DIR *dir, *fds;
  int found = FALSE;
  int dir_fd = openat(from_fd, from, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
  if (dir_fd < 0)
    return FALSE;
  if (!(dir = fdopendir(dir_fd))) {
    close(dir_fd);
    return FALSE;
  }
  while (!found && (entry = readdir(dir)) != NULL) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") ||
        fstatat(dir_fd, entry->d_name, &child, AT_SYMLINK_NOFOLLOW) < 0 ||
        !(fds = opendir("/proc/self/fd")))
      continue;
    while (!found && (slot = readdir(fds)) != NULL) {
      int n = atoi(slot->d_name);
      found = slot->d_name[0] != '.' && n != dirfd(fds) && n != dir_fd &&
              fstat(n, &open_file) == 0 && open_file.st_dev == child.st_dev &&
              open_file.st_ino == child.st_ino;
    }
    closedir(fds);
  }
  closedir(dir);
  return found;
}

int __wrap_renameat(int from_fd, const char *from, int to_fd, const char *to)
{
  int result;
  if (windows_rename_rules && has_open_child(from_fd, from)) {
    errno = EACCES;
    return -1;
  }
  result = __real_renameat(from_fd, from, to_fd, to);
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
  if (!(flags & O_CREAT)) {
    if (nfs4_readable_source >= 0 && !strcmp(name, HOUSE_FILE) && (flags & O_ACCMODE) == O_RDONLY)
      return dup(nfs4_readable_source);  /* The modeled named ACE grants the service read access. */
    if (reject_stage_read && !strcmp(name, "contents") && (flags & O_ACCMODE) == O_RDONLY) {
      errno = EACCES;
      return -1;
    }
    fd = __real_openat(dirfd, name, flags);
    if (replace_emptied_lock_on_guard_open > 0 && fd >= 0 && !strcmp(name, LOCK_NAME)) {
      int mode_action = replace_emptied_lock_on_guard_open;
      replace_emptied_lock_on_guard_open = 0;
      if (rmdir(LOCK_PATH) < 0) {
        replace_emptied_lock_on_guard_open = -1;
      } else if (mode_action == 1) {
        House_boot();
        if (lstat(LOCK_GUARD, &file) < 0 || !S_ISREG(file.st_mode))
          replace_emptied_lock_on_guard_open = -1;
      }
    }
    return fd;
  }
  va_start(ap, flags);
  mode = va_arg(ap, int);
  va_end(ap);
  if (boot_before_guard_create && !strcmp(name, "guard")) {
    boot_before_guard_create = FALSE;
    House_boot();
    candidate_was_removed = fstat(dirfd, &parent) == 0 && parent.st_nlink == 0;
  }
  fd = __real_openat(dirfd, name, flags, mode);
  if (fd >= 0 && inherit_audit_sacl && !strcmp(name, "contents"))
    TEST_ASSERT_EQUAL_INT(0, __real_fsetxattr(fd, "user.tba_sacl", audit_sacl, sizeof(audit_sacl), 0));
  /* Inspect our own descriptor before the saver starts copying permissions. */
  if (watch_create && fd >= 0 && (flags & O_EXCL) && fstat(fd, &file) == 0 &&
      fstat(dirfd, &parent) == 0 && S_ISDIR(parent.st_mode))
    saw_private_create = mode == 0600 && (file.st_mode & 0177) == 0 &&
                         (parent.st_mode & 0777) == 0700;
  return fd;
}

int __wrap_syncfs(int fd)
{
  if (fail_volume_sync) {
    errno = EIO;
    return -1;
  }
  return __real_syncfs(fd);
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
  if (during_record) {
    void (*hook)(void) = during_record;
    during_record = NULL;
    hook();
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
  during_record = NULL;
  observe_stage = FALSE;
  unlocked_during_record = -1;
}

void tearDown(void)
{
  char running[64];
#ifdef HOUSE_TEST_WRAP_IO
  watch_sync = fail_parent_sync = parent_sync_errno = parent_syncs = 0;
  fail_volume_sync = FALSE;
  watch_create = saw_private_create = deny_chown = 0;
  reject_named_chmod = following_chmods = 0;
  fail_lock_chmod = attempted_lock_chmod = visible_before_chmod = 0;
  observe_publication = report_publication_enoent = 0;
  windows_rename_rules = 0;
  published_dir_mode = published_guard_mode = -1;
  require_writable_lock = boot_before_guard_create = candidate_was_removed = FALSE;
  replace_emptied_lock_on_guard_open = 0;
  exclusive_access_mode = -1;
  mock_fs_type = 0;
  mock_statfs_errno = 0;
  mock_mount_record = NULL;
  mock_nfs4_version = fail_nfs4_copy = reject_stage_read = 0;
  nfs4_acl_read_errno = 0;
  nfs4_mode_acl = NULL;
  if (nfs4_readable_source >= 0) {
    close(nfs4_readable_source);
    nfs4_readable_source = -1;
  }
  nfs4_sacl_get_errno = nfs4_sacl_set_errno = nfs4_sacl_sets = 0;
  inherit_audit_sacl = FALSE;
#endif
  if (ROOM_FLAGGED(0, ROOM_HOUSE)) {
    char command[] = "destroy 3000";
    do_hcontrol(NULL, command, 0, 0);
  }
  umask(0022);
  chmod("house", 0700);
  chmod(LOCK_PATH, 0700);
  remove(LOCK_PATH "/marker");  /* Only the unlocked-save fixture creates this. */
  rmdir(LOCK_GUARD);
  remove(LOCK_GUARD);
  remove(LOCK_PATH);
  remove(HCONTROL_FILE);
  remove(HOUSE_FILE);
  remove(HOUSE_FILE ".tmp");
  remove(ELSEWHERE_FILE);
  remove(ELSEWHERE_FILE ".tmp");
  remove("bystander");
  remove("snapshot-guard");
  snprintf(running, sizeof(running), LIB_HOUSE ".tba-house-%ld-0", (long)getppid());
  remove(running);
  remove(LIB_HOUSE STAGE_PREFIX "notes");
#if defined(__linux__)
  removexattr("house", "system.posix_acl_default");
  remove(LIB_HOUSE "reference");
#endif
#if defined(__FreeBSD__) || defined(__linux__)
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

#if defined(__linux__) || defined(O_SEARCH)
static void test_saves_in_a_directory_with_only_write_and_search_access(void)
{
  struct stat st;
  TEST_ASSERT_EQUAL_INT(0, chmod("house", 0300));
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));

#ifdef HOUSE_TEST_WRAP_IO
  if (geteuid() != 0) {
    /* A failed post-rename sync on a search-only parent must still retry. */
    fail_volume_sync = TRUE;
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
    House_crashsave(3000);
    fail_volume_sync = FALSE;
    TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  }
#endif
  item.name = "updated contents";
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  TEST_ASSERT_EQUAL_INT(0, stat("house", &st));
  TEST_ASSERT_EQUAL_UINT(0300, st.st_mode & 0777);
}
#endif

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

static void current_stage_prefix(char *prefix, size_t size)
{
  struct stat st;
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &st));
  int length = snprintf(prefix, size, LIB_HOUSE STAGE_PREFIX "g%jx-", (uintmax_t)st.st_ino);
  TEST_ASSERT_TRUE(length > 0 && (size_t)length < size);
}

/* Run House_boot() and keep what it logged. */
static FILE *log_before_capture;

static void start_log_capture(void)
{
  FILE *capture = tmpfile();
  TEST_ASSERT_NOT_NULL(capture);
  log_before_capture = logfile;
  logfile = capture;
}

/* Stop capturing and keep what was logged since start_log_capture(). */
static void end_log_capture(char *out, size_t size)
{
  FILE *capture = logfile;
  size_t length;
  logfile = log_before_capture;
  rewind(capture);
  length = fread(out, 1, size - 1, capture);
  out[length] = '\0';
  fclose(capture);
}

static void boot_and_capture_log(char *out, size_t size)
{
  start_log_capture();
  House_boot();
  end_log_capture(out, size);
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
    during_record = House_boot;
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
  char prefix[64], paths[6][128];
  int preserved = 0;
  long pid = (long)crash_in_the_middle_of_a_save();
  current_stage_prefix(prefix, sizeof(prefix));
  snprintf(paths[0], sizeof(paths[0]), "%s0%ld-1", prefix, pid);
  snprintf(paths[1], sizeof(paths[1]), "%s%ld-01", prefix, pid);
  snprintf(paths[2], sizeof(paths[2]), "%s%ld-%lu0", prefix, pid, ULONG_MAX);
  snprintf(paths[3], sizeof(paths[3]), LIB_HOUSE ".tba-house-%ld-0", pid);
  snprintf(paths[4], sizeof(paths[4]), LIB_HOUSE ".tba-house-unlocked-%ld-0", pid);
  snprintf(paths[5], sizeof(paths[5]), LIB_HOUSE STAGE_PREFIX "%ld-0", pid);
  for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i)
    write_file(paths[i], "operator's copy");

  House_boot();
  for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
    preserved += exists(paths[i]);
    remove(paths[i]);
  }
  TEST_ASSERT_EQUAL_INT(6, preserved);
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

/* The first writer is paused inside its record writer while another process
 * handles the lost guard and installs contents with the replacement guard. */
static void boot_and_save_after_guard_loss(void)
{
  int status;
  pid_t child = fork();
  TEST_ASSERT_TRUE(child >= 0);
  if (child == 0) {
    if (unlink(LOCK_GUARD) != 0)
      _exit(1);
    House_boot();
    item.name = "new writer";
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
    observe_stage = TRUE;
    House_crashsave(3000);
    _exit(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH) || unlocked_during_record != 0);
  }
  TEST_ASSERT_EQUAL_INT(child, waitpid(child, &status, 0));
  TEST_ASSERT_TRUE(WIFEXITED(status) && WEXITSTATUS(status) == 0);
  assert_file(HOUSE_FILE, "new writer");
}

static void test_replacing_the_guard_preserves_the_previous_writers_stage(void)
{
  char prefix[64], pattern[128];
  glob_t abandoned;
  struct stat old_guard, new_guard;
  long first = (long)crash_in_the_middle_of_a_save();
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &old_guard));
  current_stage_prefix(prefix, sizeof(prefix));
  snprintf(pattern, sizeof(pattern), "%s%ld-*", prefix, first);
  TEST_ASSERT_EQUAL_INT(0, glob(pattern, 0, NULL, &abandoned));
  TEST_ASSERT_EQUAL_UINT(1, abandoned.gl_pathc);

  write_file(HOUSE_FILE, "previous contents");
  during_record = boot_and_save_after_guard_loss;
  item.name = "original writer";
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &new_guard));
  TEST_ASSERT_TRUE(old_guard.st_ino != new_guard.st_ino);

  crash_in_the_middle_of_a_save();
  TEST_ASSERT_EQUAL_INT(2, count_stages());
  House_boot();
  TEST_ASSERT_EQUAL_INT(1, count_stages());
  assert_file(abandoned.gl_pathv[0], "");
  TEST_ASSERT_EQUAL_INT(0, unlink(abandoned.gl_pathv[0]));
  globfree(&abandoned);
}

/* A stage named for a guard that has since been replaced is never cleared
 * automatically, so boot names it for the operator. */
static void test_boot_reports_a_stage_left_under_an_earlier_guard(void)
{
  char prefix[64], pattern[128], log[4096], contents[256];
  char link_name[96], fifo_name[96];
  glob_t abandoned;
  long child;
  write_file(HOUSE_FILE, "old contents");
  child = (long)crash_in_the_middle_of_a_save();
  current_stage_prefix(prefix, sizeof(prefix));
  snprintf(pattern, sizeof(pattern), "%s%ld-*", prefix, child);
  TEST_ASSERT_EQUAL_INT(0, glob(pattern, 0, NULL, &abandoned));
  TEST_ASSERT_EQUAL_UINT(1, abandoned.gl_pathc);
  snprintf(link_name, sizeof(link_name), "%s1-10", prefix);
  snprintf(fifo_name, sizeof(fifo_name), "%s1-11", prefix);
  TEST_ASSERT_EQUAL_INT(0, symlink("unrelated", link_name));
  TEST_ASSERT_EQUAL_INT(0, mkfifo(fifo_name, 0600));
  /* Keep the old guard's inode in use so the new guard gets another number. */
  TEST_ASSERT_EQUAL_INT(0, link(LOCK_GUARD, "snapshot-guard"));
  TEST_ASSERT_EQUAL_INT(0, unlink(LOCK_GUARD));

  boot_and_capture_log(log, sizeof(log));
  TEST_ASSERT_TRUE(exists(abandoned.gl_pathv[0]));
  TEST_ASSERT_NOT_NULL(strstr(log, abandoned.gl_pathv[0]));
  TEST_ASSERT_NOT_NULL(strstr(log, "under an earlier lock"));
  TEST_ASSERT_NULL(strstr(log, link_name));
  TEST_ASSERT_NULL(strstr(log, fifo_name));
  TEST_ASSERT_EQUAL_INT(0, unlink(link_name));
  TEST_ASSERT_EQUAL_INT(0, unlink(fifo_name));

  snprintf(contents, sizeof(contents), "%s/contents", abandoned.gl_pathv[0]);
  TEST_ASSERT_EQUAL_INT(0, unlink(contents));
  TEST_ASSERT_EQUAL_INT(0, rmdir(abandoned.gl_pathv[0]));
  globfree(&abandoned);
  boot_and_capture_log(log, sizeof(log));
  TEST_ASSERT_NULL(strstr(log, "under an earlier lock"));
  assert_file(HOUSE_FILE, "old contents");
}

/* A second name for the guard -- a cp -al snapshot of lib/, say -- leaves the
 * lock working.  Its permissions are left alone until it is the only name. */
static void test_a_second_link_to_the_guard_keeps_the_lock(void)
{
  const mode_t modes[] = {0640, 0400};
  struct stat st;
  write_file(HOUSE_FILE, "old contents");
  House_boot();
  TEST_ASSERT_EQUAL_INT(0, link(LOCK_GUARD, "snapshot-guard"));
  for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
    TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_GUARD, modes[i]));
    crash_in_the_middle_of_a_save();
    TEST_ASSERT_EQUAL_INT(1, count_stages());
    TEST_ASSERT_EQUAL_INT(0, count_unlocked_stages());
    House_boot();
    TEST_ASSERT_EQUAL_INT(0, count_stages());
    TEST_ASSERT_EQUAL_INT(0, stat("snapshot-guard", &st));
    TEST_ASSERT_EQUAL_UINT(2, st.st_nlink);
    TEST_ASSERT_EQUAL_UINT(modes[i], st.st_mode & 0777);
  }
  TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_GUARD, 0000));
  House_boot();
  TEST_ASSERT_EQUAL_INT(0, stat("snapshot-guard", &st));
  TEST_ASSERT_EQUAL_UINT(0000, st.st_mode & 0777);

  TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_GUARD, 0400));
  TEST_ASSERT_EQUAL_INT(0, unlink("snapshot-guard"));
  House_boot();
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &st));
  TEST_ASSERT_EQUAL_UINT(0600, st.st_mode & 0777);
  assert_file(HOUSE_FILE, "old contents");
}

/* Publication always includes the guard, so a lock directory without one was
 * emptied afterwards.  It is replaced instead of switching the lock off. */
static void test_an_emptied_lock_directory_is_replaced(void)
{
  struct stat st;
  write_file(HOUSE_FILE, "old contents");
  House_boot();
  TEST_ASSERT_EQUAL_INT(0, unlink(LOCK_GUARD));
  crash_in_the_middle_of_a_save();
  TEST_ASSERT_EQUAL_INT(1, count_stages());
  TEST_ASSERT_EQUAL_INT(0, count_unlocked_stages());

  House_boot();
  TEST_ASSERT_EQUAL_INT(0, count_stages());
  TEST_ASSERT_EQUAL_INT(0, lstat(LOCK_GUARD, &st));
  TEST_ASSERT_TRUE(S_ISREG(st.st_mode));
  TEST_ASSERT_EQUAL_UINT(0600, st.st_mode & 0777);
  assert_file(HOUSE_FILE, "old contents");
}

/* Anything else at the lock's name is not the game's: saves go ahead
 * unlocked, and it stays as it was. */
static void test_a_file_at_the_lock_name_is_left_alone(void)
{
  struct stat before, after;
  write_file(LOCK_PATH, "operator's file");
  TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_PATH, 0644));
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_PATH, &before));
  write_file(HOUSE_FILE, "old contents");
  observe_stage = TRUE;
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(1, unlocked_during_record);

  House_boot();
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_PATH, &after));
  TEST_ASSERT_TRUE(before.st_dev == after.st_dev && before.st_ino == after.st_ino);
  TEST_ASSERT_EQUAL_UINT(0644, after.st_mode & 0777);
  assert_file(LOCK_PATH, "operator's file");
}

/* With no lock yet and none possible, boot says it could not make one,
 * not that it could not open one. */
static void test_boot_says_when_it_cannot_create_the_lock(void)
{
  char log[4096];
  if (geteuid() == 0)
    TEST_IGNORE_MESSAGE("root can create the lock in a read-only directory");
  TEST_ASSERT_FALSE(exists(LOCK_PATH));
  TEST_ASSERT_EQUAL_INT(0, chmod("house", 0500));
  boot_and_capture_log(log, sizeof(log));
  TEST_ASSERT_EQUAL_INT(0, chmod("house", 0700));
  TEST_ASSERT_NOT_NULL(strstr(log, "Cannot create " LOCK_PATH));
  TEST_ASSERT_NULL(strstr(log, "Cannot open " LOCK_PATH));
}

/* A subdirectory or symlink at the guard path is not a lock this game made:
 * saves proceed unlocked and boot leaves it untouched. */
static void test_a_foreign_guard_is_left_alone(void)
{
  struct stat before, after;
  write_file(HOUSE_FILE, "old contents");

  /* 1. Subdirectory at .tba-house.lock/guard (EISDIR on O_RDWR boot). */
  TEST_ASSERT_EQUAL_INT(0, mkdir(LOCK_PATH, 0700));
  TEST_ASSERT_EQUAL_INT(0, mkdir(LOCK_GUARD, 0700));
  TEST_ASSERT_EQUAL_INT(0, lstat(LOCK_GUARD, &before));
  observe_stage = TRUE;
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(1, unlocked_during_record);
  House_boot();
  TEST_ASSERT_EQUAL_INT(0, lstat(LOCK_GUARD, &after));
  TEST_ASSERT_TRUE(S_ISDIR(after.st_mode));
  TEST_ASSERT_TRUE(before.st_dev == after.st_dev && before.st_ino == after.st_ino);
  TEST_ASSERT_EQUAL_INT(0, rmdir(LOCK_GUARD));

  /* 2. Symlink at .tba-house.lock/guard (ELOOP/EMLINK/EFTYPE on O_NOFOLLOW). */
  write_file("bystander", "bystander");
  TEST_ASSERT_EQUAL_INT(0, symlink("../../bystander", LOCK_GUARD));
  TEST_ASSERT_EQUAL_INT(0, lstat(LOCK_GUARD, &before));
  item.name = "after symlink guard";
  observe_stage = TRUE;
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(1, unlocked_during_record);
  House_boot();
  TEST_ASSERT_EQUAL_INT(0, lstat(LOCK_GUARD, &after));
  TEST_ASSERT_TRUE(S_ISLNK(after.st_mode));
  TEST_ASSERT_TRUE(before.st_dev == after.st_dev && before.st_ino == after.st_ino);
  assert_file("bystander", "bystander");
}

/* A crash while the lock was first being set up leaves the directory it was
 * filling, with or without its guard.  Boot removes it once the lock exists. */
static void test_boot_removes_an_unfinished_lock_setup(void)
{
  TEST_ASSERT_EQUAL_INT(0, mkdir(LIB_HOUSE LOCK_NAME ".init-4242-0", 0700));
  write_file(LIB_HOUSE LOCK_NAME ".init-4242-0/guard", "");
  TEST_ASSERT_EQUAL_INT(0, mkdir(LIB_HOUSE LOCK_NAME ".init-4242-1", 0000));
  TEST_ASSERT_EQUAL_INT(0, mkdir(LIB_HOUSE LOCK_NAME ".init-notes", 0700));
  write_file(LIB_HOUSE LOCK_NAME ".init-4242-2", "operator's copy");

  House_boot();
  TEST_ASSERT_TRUE(exists(LOCK_GUARD));
  TEST_ASSERT_FALSE(exists(LIB_HOUSE LOCK_NAME ".init-4242-0"));
  TEST_ASSERT_FALSE(exists(LIB_HOUSE LOCK_NAME ".init-4242-1"));
  TEST_ASSERT_TRUE(exists(LIB_HOUSE LOCK_NAME ".init-notes"));
  assert_file(LIB_HOUSE LOCK_NAME ".init-4242-2", "operator's copy");
  TEST_ASSERT_EQUAL_INT(0, rmdir(LIB_HOUSE LOCK_NAME ".init-notes"));
  TEST_ASSERT_EQUAL_INT(0, unlink(LIB_HOUSE LOCK_NAME ".init-4242-2"));
}

#ifdef HOUSE_TEST_WRAP_IO
static void test_concurrent_replacement_of_emptied_lock_does_not_fail_with_einval(void)
{
  struct stat st;

  /* 1. Another process replaces the emptied lock directory with a live lock
   * before our unlinkat() runs: follow the winning lock instead of dropping
   * to unlocked saves. */
  TEST_ASSERT_EQUAL_INT(0, mkdir(LOCK_PATH, 0700));
  replace_emptied_lock_on_guard_open = 1;
  observe_stage = TRUE;
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_EQUAL_INT(0, replace_emptied_lock_on_guard_open);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  TEST_ASSERT_EQUAL_INT(0, unlocked_during_record);
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(0, lstat(LOCK_GUARD, &st));
  TEST_ASSERT_TRUE(S_ISREG(st.st_mode));

  /* 2. Another process removes the emptied lock directory and has not yet
   * published a replacement: publish a fresh lock instead of misreporting EINVAL. */
  TEST_ASSERT_EQUAL_INT(0, unlink(LOCK_GUARD));
  replace_emptied_lock_on_guard_open = 2;
  observe_stage = TRUE;
  item.name = "republished lock";
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_EQUAL_INT(0, replace_emptied_lock_on_guard_open);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  TEST_ASSERT_EQUAL_INT(0, unlocked_during_record);
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(0, lstat(LOCK_GUARD, &st));
  TEST_ASSERT_TRUE(S_ISREG(st.st_mode));
}

static void test_save_uses_winning_guard_after_its_initializer_was_removed(void)
{
  boot_before_guard_create = observe_stage = TRUE;
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_TRUE(candidate_was_removed);
  TEST_ASSERT_TRUE(exists(LOCK_GUARD));
  TEST_ASSERT_EQUAL_INT(0, unlocked_during_record);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  assert_file(HOUSE_FILE, item.name);
}

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

  /* An owner-owned lock left read-only (0400) is also restored to 0600,
   * and an overly open lock directory (0777) is tightened back to 0700. */
  TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_GUARD, 0400));
  TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_PATH, 0777));
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &st));
  TEST_ASSERT_EQUAL_UINT(0600, st.st_mode & 0777);
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_PATH, &st));
  TEST_ASSERT_EQUAL_UINT(0700, st.st_mode & 0777);
}

#if defined(DT_REG) || defined(O_SEARCH) || (defined(O_PATH) && (defined(__linux__) || defined(__FreeBSD__)))
static void test_restrictive_lock_directory_preserves_guard_and_rejects_foreign_entries(void)
{
  const mode_t modes[] = {
#ifdef DT_REG
    0600,
#endif
#if defined(O_SEARCH) || (defined(O_PATH) && (defined(__linux__) || defined(__FreeBSD__)))
    0300,
#endif
  };
  struct stat before, after;
  write_file(HOUSE_FILE, "previous contents");
  write_file("bystander", "operator's copy");
  House_boot();
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &before));
  for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
    crash_in_the_middle_of_a_save();
    TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_PATH, modes[i]));
    House_boot();
    TEST_ASSERT_EQUAL_INT(0, count_stages());
    TEST_ASSERT_EQUAL_INT(0, stat(LOCK_PATH, &after));
    TEST_ASSERT_EQUAL_UINT(0700, after.st_mode & 0777);
    TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &after));
    TEST_ASSERT_TRUE(before.st_dev == after.st_dev && before.st_ino == after.st_ino);

    /* A symlink in the same layout must never authorize a permission repair. */
    TEST_ASSERT_EQUAL_INT(0, rename(LOCK_GUARD, "snapshot-guard"));
    TEST_ASSERT_EQUAL_INT(0, symlink("../../bystander", LOCK_GUARD));
    TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_PATH, modes[i]));
    House_boot();
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
    observe_stage = TRUE;
    House_crashsave(3000);
    TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    TEST_ASSERT_EQUAL_INT(1, unlocked_during_record);
    TEST_ASSERT_EQUAL_INT(0, stat(LOCK_PATH, &after));
    TEST_ASSERT_EQUAL_UINT(modes[i], after.st_mode & 0777);
    TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_PATH, 0700));
    TEST_ASSERT_EQUAL_INT(0, lstat(LOCK_GUARD, &after));
    TEST_ASSERT_TRUE(S_ISLNK(after.st_mode));
    TEST_ASSERT_EQUAL_INT(0, unlink(LOCK_GUARD));
    TEST_ASSERT_EQUAL_INT(0, rename("snapshot-guard", LOCK_GUARD));
    assert_file("bystander", "operator's copy");
  }
  /* A 0000 lock directory cannot be inspected or repaired safely; boot must
   * report EACCES rather than clobbering it with EBADF from fdopendir(O_PATH). */
  if (geteuid() != 0) {
    char log[4096];
    TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_PATH, 0000));
    boot_and_capture_log(log, sizeof(log));
    TEST_ASSERT_EQUAL_INT(0, stat(LOCK_PATH, &after));
    TEST_ASSERT_EQUAL_UINT(0000, after.st_mode & 0777);
    TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_PATH, 0700));
    TEST_ASSERT_NOT_NULL(strstr(log, "Cannot open"));
    TEST_ASSERT_NOT_NULL(strstr(log, strerror(EACCES)));
    TEST_ASSERT_NULL(strstr(log, strerror(EBADF)));
  }
}
#endif

static void test_boot_repairs_the_guard_and_sweeps_with_write_access(void)
{
  const mode_t modes[] = {
    0400,
#if defined(__linux__) || (defined(__FreeBSD__) && defined(O_PATH) && defined(AT_EMPTY_PATH) && defined(O_EMPTY_PATH))
    0000,
#endif
  };
  struct stat before, after;
  write_file(HOUSE_FILE, "old contents");
  House_boot();
  TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &before));
  for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
    crash_in_the_middle_of_a_save();
    TEST_ASSERT_EQUAL_INT(1, count_stages());
    TEST_ASSERT_EQUAL_INT(0, chmod(LOCK_GUARD, modes[i]));
#ifdef HOUSE_TEST_WRAP_IO
    require_writable_lock = TRUE;  /* NFS requires O_RDWR for exclusive flock. */
    exclusive_access_mode = -1;
#endif
    House_boot();
#ifdef HOUSE_TEST_WRAP_IO
    require_writable_lock = FALSE;
    TEST_ASSERT_EQUAL_INT(O_RDWR, exclusive_access_mode);
#endif
    TEST_ASSERT_EQUAL_INT(0, count_stages());
    TEST_ASSERT_EQUAL_INT(0, stat(LOCK_GUARD, &after));
    TEST_ASSERT_TRUE(before.st_dev == after.st_dev && before.st_ino == after.st_ino);
    TEST_ASSERT_EQUAL_UINT(0600, after.st_mode & 0777);
  }
  assert_file(HOUSE_FILE, "old contents");
}

static void assert_boot_preserves_unlocked_stage(pid_t child)
{
  char pattern[96], contents[256];
  glob_t paths;
  int length;
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

static void test_boot_leaves_an_interrupted_unlocked_save_untouched(void)
{
  pid_t child;
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
  assert_boot_preserves_unlocked_stage(child);
}

#ifdef HOUSE_TEST_WRAP_IO
static void test_drvfs_preserves_new_and_existing_houses(void)
{
  static const struct {
    unsigned int magic;
    const char *record;
  } mounts[] = {
    {0x01021997, "9p C: rw,aname=drvfs;path=C:;uid=1000,trans=fd"},
    {0x01021997, "9p drvfsa rw,trans=virtio,aname=drvfs;metadata"},
    {0x65735546, "virtiofs drvfs rw"},
    {0x65735546, "virtiofs drvfsa rw"},
    {0x53464846, "drvfs C: rw"}
  };
  char messages[1024];
  TEST_ASSERT_EQUAL_INT(0, stat("house", &watched_parent));
  for (size_t i = 0; i < sizeof(mounts) / sizeof(mounts[0]); ++i) {
    mock_fs_type = mounts[i].magic;
    mock_mount_record = mounts[i].record;
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
    House_crashsave(3000);
    TEST_ASSERT_FALSE(exists(HOUSE_FILE));
    TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    write_file(HOUSE_FILE, "old house");
    House_crashsave(3000);
    assert_file(HOUSE_FILE, "old house");
    TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    boot_and_capture_log(messages, sizeof(messages));
    TEST_ASSERT_NOT_NULL(strstr(messages, "Windows ACLs"));
    TEST_ASSERT_FALSE(exists(LOCK_PATH));
    TEST_ASSERT_EQUAL_INT(0, count_unlocked_stages());
    TEST_ASSERT_EQUAL_INT(0, unlink(HOUSE_FILE));
  }

  mock_statfs_errno = EIO;
  boot_and_capture_log(messages, sizeof(messages));
  TEST_ASSERT_NOT_NULL(strstr(messages, "Cannot identify the house filesystem"));
  TEST_ASSERT_FALSE(exists(LOCK_PATH));
  mock_statfs_errno = 0;

  /* A regular Plan 9 export has the same magic as the Windows transport. */
  mock_fs_type = 0x01021997;
  mock_mount_record = "9p ordinary rw,aname=drvfs-similar,trans=virtio";
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_TRUE(exists(LOCK_GUARD));
  mock_fs_type = 0x65735546;
  mock_mount_record = "virtiofs aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa rw";
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
}

static void test_smb_saves_without_inode_based_cleanup(void)
{
  const unsigned int types[] = {
    SMB_SUPER_MAGIC,
#ifdef CIFS_SUPER_MAGIC
    CIFS_SUPER_MAGIC,
#endif
#ifdef SMB2_SUPER_MAGIC
    SMB2_SUPER_MAGIC,
#endif
  };
  TEST_ASSERT_EQUAL_INT(0, stat("house", &watched_parent));
  write_file(HOUSE_FILE, "previous contents");
  for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
    crash_in_the_middle_of_a_save();
    TEST_ASSERT_EQUAL_INT(1, count_stages());
    mock_fs_type = types[i];
    observe_stage = TRUE;
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
    House_crashsave(3000);
    assert_file(HOUSE_FILE, item.name);
    TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    TEST_ASSERT_EQUAL_INT(1, unlocked_during_record);

    pid_t child = crash_in_the_middle_of_a_save();
    House_boot();
    TEST_ASSERT_EQUAL_INT(1, count_stages());
    TEST_ASSERT_EQUAL_INT(1, count_unlocked_stages());
    mock_fs_type = 0;
    assert_boot_preserves_unlocked_stage(child);
    TEST_ASSERT_EQUAL_INT(0, count_stages());
  }
}

/* On SMB, boot says cleanup is off and why, rather than reporting a failure. */
static void test_boot_explains_cleanup_is_off_on_smb(void)
{
  const char *first = LIB_HOUSE ".tba-house-unlocked-1-0";
  const char *notes = LIB_HOUSE ".tba-house-unlocked-notes";
  const char *directory = LIB_HOUSE ".tba-house-unlocked-1-1";
  const char *other_file = LIB_HOUSE ".tba-house-unlocked-1-1/notes";
  char log[4096];
  write_file(first, "staged contents");
  write_file(notes, "operator notes");
  TEST_ASSERT_EQUAL_INT(0, mkdir(directory, 0700));
  write_file(other_file, "operator notes");
  TEST_ASSERT_EQUAL_INT(0, stat("house", &watched_parent));
  mock_fs_type = SMB_SUPER_MAGIC;
  boot_and_capture_log(log, sizeof(log));
  mock_fs_type = 0;
  TEST_ASSERT_NOT_NULL(strstr(log, "it is on an SMB mount"));
  TEST_ASSERT_NULL(strstr(log, "Cannot lock"));
  TEST_ASSERT_NOT_NULL(strstr(log, first));
  TEST_ASSERT_NULL(strstr(log, notes));
  TEST_ASSERT_NULL(strstr(log, directory));
  TEST_ASSERT_NULL(strstr(log, ".tba-house-unlocked-*"));
  assert_file(first, "staged contents");
  TEST_ASSERT_EQUAL_INT(0, unlink(first));
  TEST_ASSERT_EQUAL_INT(0, unlink(notes));
  TEST_ASSERT_EQUAL_INT(0, unlink(other_file));
  TEST_ASSERT_EQUAL_INT(0, rmdir(directory));
}
#endif

/* Boot clears a crashed first save and replacement, preserving legacy stages
 * and unrelated names. */
static void test_boot_removes_saves_a_crash_left_unfinished(void)
{
  char prefix[64], running[64], large_pid[128], overflowing_pid[128];
  crash_in_the_middle_of_a_save();
  write_file(HOUSE_FILE, "old contents");
  crash_in_the_middle_of_a_save();
  TEST_ASSERT_EQUAL_INT(2, count_stages());
  assert_file(HOUSE_FILE, "old contents");

  current_stage_prefix(prefix, sizeof(prefix));
  snprintf(large_pid, sizeof(large_pid), "%s4294967296-0", prefix);
  snprintf(overflowing_pid, sizeof(overflowing_pid), "%s99999999999999999999-0", prefix);
  snprintf(running, sizeof(running), LIB_HOUSE ".tba-house-%ld-0", (long)getppid());
  write_file(running, "a running game's save");
  write_file(LIB_HOUSE STAGE_PREFIX "notes", "operator's copy");
  write_file(large_pid, "out-of-range pid 1");
  write_file(overflowing_pid, "out-of-range pid 2");
  House_boot();
  TEST_ASSERT_EQUAL_INT(3, count_stages());
  assert_file(running, "a running game's save");
  assert_file(LIB_HOUSE STAGE_PREFIX "notes", "operator's copy");
  assert_file(large_pid, "out-of-range pid 1");
  assert_file(overflowing_pid, "out-of-range pid 2");
  TEST_ASSERT_EQUAL_INT(0, unlink(large_pid));
  TEST_ASSERT_EQUAL_INT(0, unlink(overflowing_pid));
  assert_file(HOUSE_FILE, "old contents");
}

#ifdef HOUSE_TEST_WRAP_IO
static void test_boot_clears_an_empty_unreadable_stage_without_chmod(void)
{
  char prefix[64], empty[128];
  long pid = (long)crash_in_the_middle_of_a_save();
  /* A crash between mkdirat() and permission setup leaves this empty. */
  current_stage_prefix(prefix, sizeof(prefix));
  snprintf(empty, sizeof(empty), "%s%ld-%lu", prefix, pid, ULONG_MAX);
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
/* A protected DACL with a named denial and an owner grant. The v4.0
 * combined ACL starts at the ACE count. */
static const unsigned char dacl[] = {
  0,0,0,2, 0,0,0,2,
  0,0,0,1, 0,0,0,0, 0,0,0,2, 0,0,0,7, 's','a','m','@','l','a','b',0,
  0,0,0,0, 0,0,0,0, 0,0,0,3, 0,0,0,6, 'O','W','N','E','R','@',0,0
};

static void test_linux_nfs4_access_rules_survive_replacement(void)
{
  const unsigned char sacl[] = {0,0,0,2, 0,0,0,0};
  unsigned char saved[sizeof(dacl)];
  const int versions[] = {4, 41};
  for (size_t i = 0; i < sizeof(versions) / sizeof(versions[0]); ++i) {
    size_t offset = versions[i] == 4 ? 4 : 0;
    const char *name = versions[i] == 4 ? "user.tba_acl" : "user.tba_dacl";
    write_file(HOUSE_FILE, "old contents");
    TEST_ASSERT_EQUAL_INT(0, setxattr(HOUSE_FILE, name, dacl + offset, sizeof(dacl) - offset, 0));
    if (versions[i] == 41)
      TEST_ASSERT_EQUAL_INT(0, setxattr(HOUSE_FILE, "user.tba_sacl", sacl, sizeof(sacl), 0));
    mock_nfs4_version = versions[i];
    fail_nfs4_copy = TRUE;
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
    House_crashsave(3000);
    TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    assert_file(HOUSE_FILE, "old contents");

    fail_nfs4_copy = FALSE;
    House_crashsave(3000);
    TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    assert_file(HOUSE_FILE, item.name);
    TEST_ASSERT_EQUAL_INT(sizeof(dacl) - offset, getxattr(HOUSE_FILE, name, saved, sizeof(saved)));
    TEST_ASSERT_EQUAL_MEMORY(dacl + offset, saved, sizeof(dacl) - offset);
    if (versions[i] == 41) {
      TEST_ASSERT_EQUAL_INT(sizeof(sacl), getxattr(HOUSE_FILE, "user.tba_sacl", saved, sizeof(saved)));
      TEST_ASSERT_EQUAL_MEMORY(sacl, saved, sizeof(sacl));
    }
    /* Boot secures the guard with one noninheritable OWNER@ ACE. */
    House_boot();
    TEST_ASSERT_EQUAL_INT(32 - offset, getxattr(LOCK_GUARD, name, saved, sizeof(saved)));
    TEST_ASSERT_EQUAL_INT(1, saved[7 - offset]);
    TEST_ASSERT_EQUAL_MEMORY("OWNER@", saved + 24 - offset, 6);
    TEST_ASSERT_EQUAL_INT(0, unlink(HOUSE_FILE));
  }
}

static void test_linux_nfs4_audit_policy_survives_replacement(void)
{
  unsigned char saved[sizeof(dacl)];
  const unsigned char empty_sacl[8] = {0};

  mock_nfs4_version = 41;

  write_file(HOUSE_FILE, "old contents");
  TEST_ASSERT_EQUAL_INT(0, setxattr(HOUSE_FILE, "user.tba_dacl", dacl, sizeof(dacl), 0));
  nfs4_sacl_get_errno = EACCES;
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  nfs4_sacl_get_errno = 0;
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(sizeof(dacl), getxattr(HOUSE_FILE, "user.tba_dacl", saved, sizeof(saved)));
  TEST_ASSERT_EQUAL_MEMORY(dacl, saved, sizeof(dacl));

  /* Some servers advertise a SACL without returning a value. */
  TEST_ASSERT_EQUAL_INT(0, setxattr(HOUSE_FILE, "user.tba_sacl", "", 0, 0));
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));

  TEST_ASSERT_EQUAL_INT(0, setxattr(HOUSE_FILE, "user.tba_sacl", empty_sacl, sizeof(empty_sacl), 0));
  nfs4_sacl_sets = 0;
  nfs4_sacl_set_errno = EPERM;
  item.name = "empty sacl update";
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  TEST_ASSERT_EQUAL_INT(0, nfs4_sacl_sets);
  assert_file(HOUSE_FILE, item.name);

  TEST_ASSERT_EQUAL_INT(0, setxattr(HOUSE_FILE, "user.tba_sacl", audit_sacl, sizeof(audit_sacl), 0));
  item.name = "updated inventory with audit rule";
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  TEST_ASSERT_EQUAL_INT(1, nfs4_sacl_sets);
  assert_file(HOUSE_FILE, "empty sacl update");
  TEST_ASSERT_EQUAL_INT(sizeof(audit_sacl), getxattr(HOUSE_FILE, "user.tba_sacl", saved, sizeof(saved)));
  TEST_ASSERT_EQUAL_MEMORY(audit_sacl, saved, sizeof(audit_sacl));

  /* With the same inherited rules a retry needs no privileged write. */
  inherit_audit_sacl = TRUE;
  House_crashsave(3000);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  TEST_ASSERT_EQUAL_INT(1, nfs4_sacl_sets);
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(sizeof(audit_sacl), getxattr(HOUSE_FILE, "user.tba_sacl", saved, sizeof(saved)));
  TEST_ASSERT_EQUAL_MEMORY(audit_sacl, saved, sizeof(audit_sacl));

  /* Even an empty source must override a different inherited policy. */
  TEST_ASSERT_EQUAL_INT(0, setxattr(HOUSE_FILE, "user.tba_sacl", empty_sacl, sizeof(empty_sacl), 0));
  item.name = "updated inventory without audit rule";
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  assert_file(HOUSE_FILE, "updated inventory with audit rule");
  nfs4_sacl_set_errno = 0;
  House_crashsave(3000);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(sizeof(empty_sacl), getxattr(HOUSE_FILE, "user.tba_sacl", saved, sizeof(saved)));
  TEST_ASSERT_EQUAL_MEMORY(empty_sacl, saved, sizeof(empty_sacl));
  TEST_ASSERT_EQUAL_INT(sizeof(dacl), getxattr(HOUSE_FILE, "user.tba_dacl", saved, sizeof(saved)));
  TEST_ASSERT_EQUAL_MEMORY(dacl, saved, sizeof(dacl));
}

/* A change of group only rebinds GROUP@. Explicit named rules and OWNER@
 * still mean the same thing and must survive byte for byte. */
static void test_group_change_preserves_audit_principals(void)
{
  const unsigned char group_audit[] = {
    0,0,0,2, 0,0,0,1,
    0,0,0,2, 0,0,0,16, 0,0,0,1, 0,0,0,6, 'G','R','O','U','P','@',0,0
  };
  unsigned char saved[sizeof(audit_sacl)];
  gid_t old_group;

  write_file(HOUSE_FILE, "old contents");
  set_alternate_house_group(&old_group);
  mock_nfs4_version = 41;
  deny_chown = TRUE;
  TEST_ASSERT_EQUAL_INT(0, setxattr(HOUSE_FILE, "user.tba_dacl", dacl, sizeof(dacl), 0));
  TEST_ASSERT_EQUAL_INT(0, setxattr(HOUSE_FILE, "user.tba_sacl", group_audit, sizeof(group_audit), 0));
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  House_crashsave(3000);
  TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  assert_file(HOUSE_FILE, "old contents");
  TEST_ASSERT_EQUAL_INT(sizeof(group_audit), getxattr(HOUSE_FILE, "user.tba_sacl", saved, sizeof(saved)));
  TEST_ASSERT_EQUAL_MEMORY(group_audit, saved, sizeof(group_audit));

  TEST_ASSERT_EQUAL_INT(0, setxattr(HOUSE_FILE, "user.tba_sacl", audit_sacl, sizeof(audit_sacl), 0));
  House_crashsave(3000);
  TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  assert_file(HOUSE_FILE, item.name);
  TEST_ASSERT_EQUAL_INT(sizeof(audit_sacl), getxattr(HOUSE_FILE, "user.tba_sacl", saved, sizeof(saved)));
  TEST_ASSERT_EQUAL_MEMORY(audit_sacl, saved, sizeof(audit_sacl));
}

static void test_nfs4_house_of_another_account_still_saves(void)
{
  /* v4.1 DACLs: a flags word, then the ACE count where a v4.0 ACL starts. */
  static const unsigned char plain[] = {
    0,0,0,0, 0,0,0,3,
    0,0,0,0, 0,0,0,0, 0,0x1f,0x01,0x9f, 0,0,0,6, 'O','W','N','E','R','@',0,0,
    0,0,0,0, 0,0,0,0, 0,0x12,0x00,0x81, 0,0,0,6, 'G','R','O','U','P','@',0,0,
    0,0,0,0, 0,0,0,0, 0,0x12,0x00,0x81, 0,0,0,9, 'E','V','E','R','Y','O','N','E','@',0,0,0
  };
  static const unsigned char named[] = {
    0,0,0,0, 0,0,0,2,
    0,0,0,1, 0,0,0,0, 0,0,0,4, 0,0,0,7, 's','a','m','@','l','a','b',0,
    0,0,0,0, 0,0,0,0, 0,0x1f,0x01,0xff, 0,0,0,6, 'O','W','N','E','R','@',0,0
  };
  static const unsigned char owner_only[] = {
    0,0,0,2, 0,0,0,1,
    0,0,0,0, 0,0,0,0, 0,0x1f,0x01,0x9f, 0,0,0,6, 'O','W','N','E','R','@',0,0
  };
  const int versions[] = {4, 41};
  gid_t old_group;
  struct stat before, after;
  unsigned char saved[sizeof(plain)], acl[sizeof(plain)];

  for (size_t v = 0; v < sizeof(versions) / sizeof(versions[0]); ++v) {
    size_t offset = versions[v] == 4 ? 4 : 0;
    const char *name = versions[v] == 4 ? "user.tba_acl" : "user.tba_dacl";

    write_file(HOUSE_FILE, "old contents");
    set_alternate_house_group(&old_group);
    TEST_ASSERT_EQUAL_INT(0, setxattr(HOUSE_FILE, name, named + offset, sizeof(named) - offset, 0));
    TEST_ASSERT_EQUAL_INT(0, stat(HOUSE_FILE, &before));
    mock_nfs4_version = versions[v];
    deny_chown = TRUE;
    const int read_errors[] = {EIO, ERANGE};
    for (size_t i = 0; i < sizeof(read_errors) / sizeof(read_errors[0]); ++i) {
      nfs4_acl_read_errno = read_errors[i];
      SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
      House_crashsave(3000);
      TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
      assert_file(HOUSE_FILE, "old contents");
    }
    nfs4_acl_read_errno = 0;
    House_crashsave(3000);
    TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    TEST_ASSERT_EQUAL_INT(sizeof(named) - offset, getxattr(HOUSE_FILE, name, saved, sizeof(saved)));
    TEST_ASSERT_EQUAL_MEMORY(named + offset, saved, sizeof(named) - offset);

    /* The modeled server produces this ACL on chmod. An extra append-only
     * grant to EVERYONE@ is invisible in mode and prevents that conversion. */
    memcpy(acl, plain, sizeof(plain));
    nfs4_mode_acl = acl + offset;
    nfs4_mode_acl_size = sizeof(plain) - offset;
    memcpy(saved, acl, sizeof(plain));
    saved[67] |= 4; /* EVERYONE@ also has APPEND_DATA, without WRITE_DATA. */
    TEST_ASSERT_EQUAL_INT(0, setxattr(HOUSE_FILE, name, saved + offset, sizeof(plain) - offset, 0));
    TEST_ASSERT_EQUAL_INT(0, chown(HOUSE_FILE, (uid_t)-1, old_group));
    TEST_ASSERT_EQUAL_INT(0, chmod(HOUSE_FILE, 0644));
    TEST_ASSERT_EQUAL_INT(0, stat(HOUSE_FILE, &before));
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
    House_crashsave(3000);
    TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    TEST_ASSERT_EQUAL_INT(0, stat(HOUSE_FILE, &after));
    TEST_ASSERT_TRUE(before.st_ino == after.st_ino);
    TEST_ASSERT_EQUAL_INT(0, setxattr(HOUSE_FILE, name, nfs4_mode_acl, nfs4_mode_acl_size, 0));
    House_crashsave(3000);
    TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    TEST_ASSERT_EQUAL_INT(0, stat(HOUSE_FILE, &after));
    TEST_ASSERT_EQUAL_UINT(0644, after.st_mode & 0777);

    /* A named rule and a rebound GROUP@ use the private fallback. NFSv4
     * can grant read to the named game user without setting the owner bit. */
    memcpy(saved, named, sizeof(named));
    memcpy(saved + 48, "GROUP@", 6);
    TEST_ASSERT_EQUAL_INT(0, setxattr(HOUSE_FILE, name, saved + offset, sizeof(named) - offset, 0));
    TEST_ASSERT_EQUAL_INT(0, chown(HOUSE_FILE, (uid_t)-1, old_group));
    /* A local user.* attribute requires read permission. Give the wrapper
     * a readable anonymous copy to model the server's named read grant. */
    FILE *readable = tmpfile();
    TEST_ASSERT_NOT_NULL(readable);
    nfs4_readable_source = dup(fileno(readable));
    TEST_ASSERT_EQUAL_INT(0, fclose(readable));
    TEST_ASSERT_TRUE(nfs4_readable_source >= 0);
    TEST_ASSERT_EQUAL_INT(0, fsetxattr(nfs4_readable_source, name, saved + offset, sizeof(named) - offset, 0));
    TEST_ASSERT_EQUAL_INT(0, chmod(HOUSE_FILE, 0200));
    TEST_ASSERT_EQUAL_INT(0, stat(HOUSE_FILE, &before));
    reject_stage_read = TRUE;
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
    House_crashsave(3000);
    TEST_ASSERT_TRUE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    TEST_ASSERT_EQUAL_INT(0, stat(HOUSE_FILE, &after));
    TEST_ASSERT_TRUE(before.st_ino == after.st_ino);
    reject_stage_read = FALSE;
    House_crashsave(3000);
    TEST_ASSERT_FALSE(ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
    assert_file(HOUSE_FILE, item.name);
    TEST_ASSERT_EQUAL_INT(sizeof(owner_only) - offset, getxattr(HOUSE_FILE, name, saved, sizeof(saved)));
    TEST_ASSERT_EQUAL_MEMORY(owner_only + offset, saved, sizeof(owner_only) - offset);
    TEST_ASSERT_EQUAL_INT(0, stat(HOUSE_FILE, &after));
    TEST_ASSERT_EQUAL_UINT(0600, after.st_mode & 0777);
    close(nfs4_readable_source);
    nfs4_readable_source = -1;
    nfs4_mode_acl = NULL;
    TEST_ASSERT_EQUAL_INT(0, unlink(HOUSE_FILE));
    deny_chown = mock_nfs4_version = 0;
  }
}

/* Publication also works on filesystems that require closing the child first. */
static void test_lock_is_published_where_open_files_block_directory_renames(void)
{
  TEST_ASSERT_FALSE(exists(LOCK_PATH));
  windows_rename_rules = TRUE;
  House_boot();
  TEST_ASSERT_TRUE(exists(LOCK_GUARD));
  write_file(HOUSE_FILE, "old contents");
  crash_in_the_middle_of_a_save();
  TEST_ASSERT_EQUAL_INT(1, count_stages());
  TEST_ASSERT_EQUAL_INT(0, count_unlocked_stages());
  House_boot();
  windows_rename_rules = FALSE;
  TEST_ASSERT_EQUAL_INT(0, count_stages());
  assert_file(HOUSE_FILE, "old contents");
}

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
#if defined(__FreeBSD__) || defined(__linux__)
static void test_foreign_owned_house_remains_readable_to_game(void)
{
  const uid_t game = 65534;
  const char *file = "elsewhere/" HOUSE_FILE;
  pid_t child;
  int status;

  if (geteuid() != 0)
    TEST_IGNORE_MESSAGE("requires root to create a foreign-owned house and run as its named reader");
#if defined(__FreeBSD__)
  if (pathconf("elsewhere", _PC_ACL_NFS4) <= 0)
    TEST_IGNORE_MESSAGE("requires a FreeBSD NFSv4 ACL filesystem");
#endif

  /* The outer test directory stays private. The child can use this subtree
   * as its working directory after dropping privileges. */
  TEST_ASSERT_EQUAL_INT(0, mkdir("elsewhere/house", 0700));
  TEST_ASSERT_EQUAL_INT(0, chown("elsewhere/house", game, (gid_t)-1));
  TEST_ASSERT_EQUAL_INT(0, chmod("elsewhere", 0711));
  write_file(file, "old contents");
#if defined(__linux__)
  unsigned char source[sizeof(acl)];
  memcpy(source, acl, sizeof(source));
  source[6] = 2; /* Old owner: write-only. Named game user: read/write. */
  TEST_ASSERT_EQUAL_INT(0, setxattr(file, "system.posix_acl_access", source, sizeof(source), 0));
#else
  acl_t acl = acl_from_text("owner@:r::deny,user:65534:full_set::allow,owner@:full_set::allow");
  TEST_ASSERT_NOT_NULL(acl);
  int result = acl_set_file(file, ACL_TYPE_NFS4, acl);
  acl_free(acl);
  TEST_ASSERT_EQUAL_INT(0, result);
#endif

  child = fork();
  TEST_ASSERT_TRUE(child >= 0);
  if (child == 0) {
    FILE *fp;
    if (chdir("elsewhere") < 0 || setuid(game) < 0 || access(HOUSE_FILE, R_OK) < 0)
      _exit(1);
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
#ifdef HOUSE_TEST_WRAP_IO
    reject_stage_read = TRUE;
    House_crashsave(3000);
    if (!ROOM_FLAGGED(0, ROOM_HOUSE_CRASH) || !(fp = fopen(HOUSE_FILE, "rb")))
      _exit(4);
    if (fgetc(fp) != 'o' || fclose(fp) != 0)
      _exit(5);
    reject_stage_read = FALSE;
#endif
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
#if defined(__linux__) || defined(O_SEARCH)
  RUN_TEST(test_saves_in_a_directory_with_only_write_and_search_access);
#endif
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
  RUN_TEST(test_the_lock_file_is_private);
#if defined(DT_REG) || defined(O_SEARCH) || (defined(O_PATH) && (defined(__linux__) || defined(__FreeBSD__)))
  RUN_TEST(test_restrictive_lock_directory_preserves_guard_and_rejects_foreign_entries);
#endif
  RUN_TEST(test_a_second_link_to_the_guard_keeps_the_lock);
  RUN_TEST(test_an_emptied_lock_directory_is_replaced);
  RUN_TEST(test_replacing_the_guard_preserves_the_previous_writers_stage);
  RUN_TEST(test_boot_reports_a_stage_left_under_an_earlier_guard);
  RUN_TEST(test_a_file_at_the_lock_name_is_left_alone);
  RUN_TEST(test_boot_says_when_it_cannot_create_the_lock);
  RUN_TEST(test_a_foreign_guard_is_left_alone);
  RUN_TEST(test_boot_removes_an_unfinished_lock_setup);
  RUN_TEST(test_boot_repairs_the_guard_and_sweeps_with_write_access);
  RUN_TEST(test_boot_leaves_an_interrupted_unlocked_save_untouched);
#ifdef HOUSE_TEST_WRAP_IO
  RUN_TEST(test_smb_saves_without_inode_based_cleanup);
  RUN_TEST(test_drvfs_preserves_new_and_existing_houses);
  RUN_TEST(test_boot_explains_cleanup_is_off_on_smb);
  RUN_TEST(test_concurrent_replacement_of_emptied_lock_does_not_fail_with_einval);
  RUN_TEST(test_save_uses_winning_guard_after_its_initializer_was_removed);
  RUN_TEST(test_lock_is_secured_before_publication_and_retries_after_failure);
  RUN_TEST(test_boot_clears_an_empty_unreadable_stage_without_chmod);
  RUN_TEST(test_replacement_starts_private_before_permissions_are_copied);
  RUN_TEST(test_parent_sync_failure_keeps_house_queued_until_retry);
#if defined(__linux__)
  RUN_TEST(test_group_restore_denied_preserves_named_acl_access);
  RUN_TEST(test_linux_nfs4_access_rules_survive_replacement);
  RUN_TEST(test_linux_nfs4_audit_policy_survives_replacement);
  RUN_TEST(test_group_change_preserves_audit_principals);
  RUN_TEST(test_nfs4_house_of_another_account_still_saves);
  RUN_TEST(test_lock_is_published_where_open_files_block_directory_renames);
#endif
#endif
#if defined(__linux__)
  RUN_TEST(test_linux_acl_preserved_and_inherited_default_acl_cleared);
#elif defined(__FreeBSD__)
  RUN_TEST(test_freebsd_keeps_an_existing_named_acl);
#endif
#if defined(__FreeBSD__) || defined(__linux__)
  RUN_TEST(test_foreign_owned_house_remains_readable_to_game);
#endif
  result = UNITY_END();
  remove("elsewhere/.tba-house.lock/guard");
  remove("elsewhere/.tba-house.lock");
  if (rmdir("house") || rmdir("etc") || rmdir("elsewhere") || chdir("/") || rmdir(scratch))
    return 1;
  return result;
}
