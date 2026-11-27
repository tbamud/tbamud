/* Native filesystem tests for the Windows backend used by House_crashsave. */
#include "unity.h"
#include <windows.h>

/* Exercise errors returned by providers that cannot report volume flags. */
static DWORD stream_probe_error;
static HANDLE WINAPI test_find_first_stream(LPCWSTR path, STREAM_INFO_LEVELS level, LPVOID data, DWORD flags)
{
  if (stream_probe_error) {
    SetLastError(stream_probe_error);
    return INVALID_HANDLE_VALUE;
  }
  return FindFirstStreamW(path, level, data, flags);
}

#define FindFirstStreamW test_find_first_stream
#include "house_win32.h"
#undef FindFirstStreamW
#include <sddl.h>

static char house[MAX_PATH], link_name[MAX_PATH + sizeof(".link")];
static char directory[MAX_PATH + sizeof(".dir")], child[MAX_PATH + sizeof(".dir\\room.house")];
static int owns_house, owns_link, owns_directory, owns_child;

void setUp(void)
{
  char temp[MAX_PATH];
  DWORD length = GetTempPathA(sizeof(temp), temp);
  TEST_ASSERT_TRUE(length > 0 && length < sizeof(temp));
  owns_house = GetTempFileNameA(temp, "tba", 0, house) != 0;
  TEST_ASSERT_TRUE(owns_house);
  snprintf(link_name, sizeof(link_name), "%s.link", house);
}

void tearDown(void)
{
  stream_probe_error = ERROR_SUCCESS;
  if (owns_link)
    DeleteFileA(link_name);
  if (owns_house) {
    SetFileAttributesA(house, FILE_ATTRIBUTE_NORMAL);
    DeleteFileA(house);
  }
  if (owns_child) {
    SetFileAttributesA(child, FILE_ATTRIBUTE_NORMAL);
    DeleteFileA(child);
  }
  if (owns_directory)
    RemoveDirectoryA(directory);
  owns_link = owns_house = owns_directory = owns_child = 0;
}

static void save_house(const char *name)
{
  struct house_save save = {0};
  FILE *fp = House_open_save(name, &save);
  TEST_ASSERT_NOT_NULL(fp);
  int written = fputs("saved", fp);
  int closed = fclose(fp);
  int installed = written >= 0 && !closed && House_install_save(&save);
  House_discard_save(&save);
  TEST_ASSERT_TRUE(installed);
}

static void test_save_updates_the_target_of_a_file_symlink(void)
{
  /* 0x2 permits unprivileged creation when Windows Developer Mode is on. */
  BOOLEAN created = CreateSymbolicLinkA(link_name, house, 0x2);
  if (!created && GetLastError() == ERROR_INVALID_PARAMETER)
    created = CreateSymbolicLinkA(link_name, house, 0);
  owns_link = created;
  if (!created && GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
    TEST_IGNORE_MESSAGE("Windows file symlink creation is unavailable to this user");
  TEST_ASSERT_TRUE(created);
  save_house(link_name);
  DWORD attributes = GetFileAttributesA(link_name);
  TEST_ASSERT_TRUE(attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT));
  char data[8] = "";
  FILE *fp = fopen(house, "rb");
  TEST_ASSERT_NOT_NULL(fp);
  size_t bytes = fread(data, 1, sizeof(data), fp);
  fclose(fp);
  TEST_ASSERT_EQUAL_UINT(5, bytes);
  TEST_ASSERT_EQUAL_STRING("saved", data);
}

/* Read the DACL and its protection/inheritance flags as SDDL. */
static void read_dacl(char *name, char **sddl)
{
  PSECURITY_DESCRIPTOR security = NULL;
  TEST_ASSERT_EQUAL_UINT32(ERROR_SUCCESS, GetNamedSecurityInfoA(name, SE_FILE_OBJECT,
                           DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &security));
  BOOL converted = ConvertSecurityDescriptorToStringSecurityDescriptorA(
      security, SDDL_REVISION_1, DACL_SECURITY_INFORMATION, sddl, NULL);
  LocalFree(security);
  TEST_ASSERT_TRUE(converted);
}

static void check_dacl_after_save(char *name)
{
  char *before, *after;
  read_dacl(name, &before);
  save_house(name);
  read_dacl(name, &after);
  int same = strcmp(before, after) == 0;
  LocalFree(after);
  LocalFree(before);
  TEST_ASSERT_TRUE(same);
}

static void test_save_preserves_a_protected_custom_dacl(void)
{
  PSECURITY_DESCRIPTOR security = NULL;
  TEST_ASSERT_TRUE(ConvertStringSecurityDescriptorToSecurityDescriptorW(
      L"D:P(A;;FA;;;OW)(A;;FR;;;SY)", SDDL_REVISION_1, &security, NULL));
  /* A DACL protected after converting its inherited rules keeps both flags. */
  SECURITY_DESCRIPTOR_CONTROL auto_inherit = SE_DACL_AUTO_INHERIT_REQ | SE_DACL_AUTO_INHERITED;
  TEST_ASSERT_TRUE(SetSecurityDescriptorControl(security, auto_inherit, auto_inherit));
  BOOL set = SetFileSecurityA(house, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, security);
  LocalFree(security);
  TEST_ASSERT_TRUE(set);
  char *sddl;
  read_dacl(house, &sddl);
  int flags_set = strstr(sddl, "D:PAI") != NULL;
  LocalFree(sddl);
  TEST_ASSERT_TRUE(flags_set);
  check_dacl_after_save(house);
}

static void test_save_preserves_a_protected_dacl_without_write_dac(void)
{
  PSECURITY_DESCRIPTOR security = NULL;
  TEST_ASSERT_TRUE(ConvertStringSecurityDescriptorToSecurityDescriptorW(
      L"D:P(A;;FRFWSD;;;OW)", SDDL_REVISION_1, &security, NULL));
  BOOL set = SetFileSecurityA(house, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, security);
  LocalFree(security);
  TEST_ASSERT_TRUE(set);
  check_dacl_after_save(house);
}

static void test_save_preserves_an_inherited_dacl(void)
{
  check_dacl_after_save(house);
}

static void test_save_with_only_inherited_modify_access(void)
{
  PSECURITY_DESCRIPTOR sd = NULL;
  /* Owner has full control of the directory, Modify on its children. */
  TEST_ASSERT_TRUE(ConvertStringSecurityDescriptorToSecurityDescriptorW(
      L"D:P(A;;FA;;;OW)(A;OIIO;0x1301bf;;;OW)", SDDL_REVISION_1, &sd, NULL));
  SECURITY_ATTRIBUTES attrs = { sizeof(attrs), sd, FALSE };
  snprintf(directory, sizeof(directory), "%s.dir", house);
  owns_directory = CreateDirectoryA(directory, &attrs);
  LocalFree(sd);
  TEST_ASSERT_TRUE(owns_directory);
  snprintf(child, sizeof(child), "%s\\room.house", directory);
  HANDLE file = CreateFileA(child, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
  TEST_ASSERT_TRUE(file != INVALID_HANDLE_VALUE);
  owns_child = 1;
  CloseHandle(file);
  file = CreateFileA(child, WRITE_DAC, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file != INVALID_HANDLE_VALUE)
    CloseHandle(file);
  TEST_ASSERT_TRUE(file == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ACCESS_DENIED);
  check_dacl_after_save(child);
}

static void require_volume_feature(DWORD feature)
{
  DWORD flags;
  HANDLE file = CreateFileA(house, 0, HOUSE_WIN_SHARE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  TEST_ASSERT_TRUE(file != INVALID_HANDLE_VALUE);
  BOOL ok = GetVolumeInformationByHandleW(file, NULL, 0, NULL, NULL, &flags, NULL, 0);
  CloseHandle(file);
  TEST_ASSERT_TRUE(ok);
  if (!(flags & feature))
    TEST_IGNORE_MESSAGE("fixture volume does not support this NTFS feature");
}

static void test_save_copies_named_and_empty_data_streams(void)
{
  char named[MAX_PATH + 16], empty[MAX_PATH + 16];
  static unsigned char data[35000], readback[sizeof(data)];
  require_volume_feature(FILE_NAMED_STREAMS);
  snprintf(named, sizeof(named), "%s:backup-label", house);
  snprintf(empty, sizeof(empty), "%s:empty", house);
  for (size_t i = 0; i < sizeof(data); ++i)
    data[i] = (unsigned char)(i % 251);
  FILE *fp = fopen(named, "wb");
  TEST_ASSERT_NOT_NULL(fp);
  size_t wrote = fwrite(data, 1, sizeof(data), fp);
  TEST_ASSERT_EQUAL_INT(0, fclose(fp));
  TEST_ASSERT_EQUAL_UINT(sizeof(data), wrote);
  fp = fopen(empty, "wb");
  TEST_ASSERT_NOT_NULL(fp);
  TEST_ASSERT_EQUAL_INT(0, fclose(fp));
  save_house(house);
  fp = fopen(named, "rb");
  TEST_ASSERT_NOT_NULL(fp);
  size_t count = fread(readback, 1, sizeof(readback), fp);
  int next = fgetc(fp);
  fclose(fp);
  TEST_ASSERT_EQUAL_UINT(sizeof(data), count);
  TEST_ASSERT_EQUAL_MEMORY(data, readback, count);
  TEST_ASSERT_EQUAL_INT(EOF, next);
  fp = fopen(empty, "rb");
  TEST_ASSERT_NOT_NULL(fp);
  next = fgetc(fp);
  fclose(fp);
  TEST_ASSERT_EQUAL_INT(EOF, next);
}

static void test_save_keeps_ntfs_compression(void)
{
  DWORD bytes;
  USHORT format = COMPRESSION_FORMAT_DEFAULT;
  require_volume_feature(FILE_FILE_COMPRESSION);
  HANDLE file = CreateFileA(house, GENERIC_READ | GENERIC_WRITE, 0,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  TEST_ASSERT_TRUE(file != INVALID_HANDLE_VALUE);
  BOOL ok = DeviceIoControl(file, FSCTL_SET_COMPRESSION, &format, sizeof(format), NULL, 0, &bytes, NULL);
  CloseHandle(file);
  TEST_ASSERT_TRUE(ok);
  save_house(house);
  DWORD flags = GetFileAttributesA(house);
  TEST_ASSERT_TRUE(flags != INVALID_FILE_ATTRIBUTES && (flags & FILE_ATTRIBUTE_COMPRESSED));
}

static void test_locked_named_stream_leaves_the_previous_save_intact(void)
{
  char name[MAX_PATH + 16], contents[5] = "";
  struct house_save save = {0};
  require_volume_feature(FILE_NAMED_STREAMS);
  FILE *fp = fopen(house, "wb");
  TEST_ASSERT_NOT_NULL(fp);
  int wrote = fputs("good", fp);
  TEST_ASSERT_EQUAL_INT(0, fclose(fp));
  TEST_ASSERT_TRUE(wrote >= 0);
  snprintf(name, sizeof(name), "%s:in-use", house);
  HANDLE lock = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
  TEST_ASSERT_TRUE(lock != INVALID_HANDLE_VALUE);
  fp = House_open_save(house, &save);
  if (fp)
    fclose(fp);
  House_discard_save(&save);
  CloseHandle(lock);
  TEST_ASSERT_NULL(fp);
  fp = fopen(house, "rb");
  TEST_ASSERT_NOT_NULL(fp);
  size_t count = fread(contents, 1, sizeof(contents), fp);
  fclose(fp);
  TEST_ASSERT_EQUAL_UINT(4, count);
  TEST_ASSERT_EQUAL_STRING("good", contents);
}

static void read_current_user_sid(char *buf, size_t cap)
{
  HANDLE token = NULL;
  union {
    TOKEN_USER user;
    BYTE bytes[256];
  } info;
  DWORD len = sizeof(info);
  LPSTR sid = NULL;
  TEST_ASSERT_TRUE(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token));
  BOOL ok = GetTokenInformation(token, TokenUser, &info.user, len, &len);
  CloseHandle(token);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_TRUE(ConvertSidToStringSidA(info.user.User.Sid, &sid));
  int written = snprintf(buf, cap, "%s", sid);
  LocalFree(sid);
  TEST_ASSERT_TRUE(written > 0 && (size_t)written < cap);
}

static void write_stream_text(const char *path, const char *text)
{
  HANDLE file = CreateFileA(path, FILE_WRITE_DATA, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  DWORD count = 0;
  TEST_ASSERT_TRUE(file != INVALID_HANDLE_VALUE);
  BOOL wrote = WriteFile(file, text, (DWORD)strlen(text), &count, NULL);
  CloseHandle(file);
  TEST_ASSERT_TRUE(wrote);
  TEST_ASSERT_EQUAL_UINT(strlen(text), count);
}

static void assert_stream_text(const char *path, const char *expected)
{
  char buf[64];
  DWORD count = 0;
  HANDLE file = CreateFileA(path, FILE_READ_DATA, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  TEST_ASSERT_TRUE(file != INVALID_HANDLE_VALUE);
  BOOL read = ReadFile(file, buf, sizeof(buf), &count, NULL);
  CloseHandle(file);
  TEST_ASSERT_TRUE(read);
  TEST_ASSERT_EQUAL_UINT(strlen(expected), count);
  TEST_ASSERT_EQUAL_MEMORY(expected, buf, count);
}

static void test_save_copies_named_streams_with_restrictive_and_inherited_dacls(void)
{
  char sid[128], sddl[256], stream[MAX_PATH + 16];
  PSECURITY_DESCRIPTOR security = NULL;
  require_volume_feature(FILE_NAMED_STREAMS);
  read_current_user_sid(sid, sizeof(sid));
  snprintf(stream, sizeof(stream), "%s:note", house);

  /* 1. Inherited DACL + named stream keeps both the inherited flags and stream. */
  write_stream_text(stream, "inherited-stream");
  check_dacl_after_save(house);
  assert_stream_text(stream, "inherited-stream");

  /* 2. Data can be read and written without access to extended attributes. */
  snprintf(sddl, sizeof(sddl), "D:P(A;;0x130083;;;%s)", sid);
  TEST_ASSERT_TRUE(ConvertStringSecurityDescriptorToSecurityDescriptorA(sddl, SDDL_REVISION_1, &security, NULL));
  BOOL set = SetFileSecurityA(house, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, security);
  LocalFree(security);
  TEST_ASSERT_TRUE(set);
  check_dacl_after_save(house);
  assert_stream_text(stream, "inherited-stream");

  /* 3. Read-only DACL (owner has FR only; parent grants delete) + named stream. */
  snprintf(sddl, sizeof(sddl), "D:P(A;;FR;;;%s)", sid);
  TEST_ASSERT_TRUE(ConvertStringSecurityDescriptorToSecurityDescriptorA(sddl, SDDL_REVISION_1, &security, NULL));
  set = SetFileSecurityA(house, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, security);
  LocalFree(security);
  TEST_ASSERT_TRUE(set);
  check_dacl_after_save(house);
  assert_stream_text(stream, "inherited-stream");

  /* 4. The inherited DACL permits writes, but grants neither APPEND_DATA nor WRITE_DAC. */
  {
    PSECURITY_DESCRIPTOR dir_sd = NULL;
    char child_stream[sizeof(child) + 16];
    TEST_ASSERT_TRUE(ConvertStringSecurityDescriptorToSecurityDescriptorW(
        L"D:P(A;;FA;;;OW)(A;OIIO;0x13008b;;;OW)", SDDL_REVISION_1, &dir_sd, NULL));
    SECURITY_ATTRIBUTES attrs = { sizeof(attrs), dir_sd, FALSE };
    snprintf(directory, sizeof(directory), "%s.dir", house);
    owns_directory = CreateDirectoryA(directory, &attrs);
    LocalFree(dir_sd);
    TEST_ASSERT_TRUE(owns_directory);
    snprintf(child, sizeof(child), "%s\\room.house", directory);
    HANDLE file = CreateFileA(child, FILE_WRITE_DATA, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    TEST_ASSERT_TRUE(file != INVALID_HANDLE_VALUE);
    owns_child = 1;
    CloseHandle(file);
    snprintf(child_stream, sizeof(child_stream), "%s:note", child);
    write_stream_text(child_stream, "modify-stream");
    check_dacl_after_save(child);
    assert_stream_text(child_stream, "modify-stream");
  }
}

static void test_stream_probe_without_volume_flags(void)
{
  struct house_save save = {0};
  struct house_win_metadata meta = {0};
  save.dest = L"provider-file";
  stream_probe_error = ERROR_INVALID_PARAMETER;
  TEST_ASSERT_EQUAL_UINT32(ERROR_SUCCESS, House_win_copy_streams(&save, NULL, &meta, FALSE));
  meta.volume_flags_known = TRUE;
  TEST_ASSERT_EQUAL_UINT32(ERROR_INVALID_PARAMETER, House_win_copy_streams(&save, NULL, &meta, FALSE));
  meta.volume_flags_known = FALSE;
  stream_probe_error = ERROR_ACCESS_DENIED;
  TEST_ASSERT_EQUAL_UINT32(ERROR_ACCESS_DENIED, House_win_copy_streams(&save, NULL, &meta, FALSE));
}

static void test_save_preserves_hidden_and_not_indexed_attributes(void)
{
  DWORD want = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
  DWORD vflags = 0;
  HANDLE h = CreateFileA(house, 0, HOUSE_WIN_SHARE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  TEST_ASSERT_TRUE(h != INVALID_HANDLE_VALUE);
  BOOL have_streams = GetVolumeInformationByHandleW(h, NULL, 0, NULL, NULL, &vflags, NULL, 0) &&
                      (vflags & FILE_NAMED_STREAMS);
  CloseHandle(h);
  char stream[MAX_PATH + 16];
  if (have_streams) {
    snprintf(stream, sizeof(stream), "%s:note", house);
    write_stream_text(stream, "hidden-stream");
  }
  TEST_ASSERT_TRUE(SetFileAttributesA(house, want));
  save_house(house);
  DWORD got = GetFileAttributesA(house);
  TEST_ASSERT_TRUE(got != INVALID_FILE_ATTRIBUTES);
  TEST_ASSERT_EQUAL_HEX32(want, got & want);
  if (have_streams)
    assert_stream_text(stream, "hidden-stream");
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_save_updates_the_target_of_a_file_symlink);
  RUN_TEST(test_save_preserves_a_protected_custom_dacl);
  RUN_TEST(test_save_preserves_a_protected_dacl_without_write_dac);
  RUN_TEST(test_save_preserves_an_inherited_dacl);
  RUN_TEST(test_save_with_only_inherited_modify_access);
  RUN_TEST(test_save_copies_named_and_empty_data_streams);
  RUN_TEST(test_save_copies_named_streams_with_restrictive_and_inherited_dacls);
  RUN_TEST(test_stream_probe_without_volume_flags);
  RUN_TEST(test_save_keeps_ntfs_compression);
  RUN_TEST(test_save_preserves_hidden_and_not_indexed_attributes);
  RUN_TEST(test_locked_named_stream_leaves_the_previous_save_intact);
  return UNITY_END();
}
