/* Native filesystem tests for the Windows backend used by House_crashsave. */
#include "unity.h"
#include "house_win32.h"
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
  if (owns_link)
    DeleteFileA(link_name);
  if (owns_house)
    DeleteFileA(house);
  if (owns_child)
    DeleteFileA(child);
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

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_save_updates_the_target_of_a_file_symlink);
  RUN_TEST(test_save_preserves_a_protected_custom_dacl);
  RUN_TEST(test_save_preserves_a_protected_dacl_without_write_dac);
  RUN_TEST(test_save_preserves_an_inherited_dacl);
  RUN_TEST(test_save_with_only_inherited_modify_access);
  RUN_TEST(test_save_copies_named_and_empty_data_streams);
  RUN_TEST(test_save_keeps_ntfs_compression);
  RUN_TEST(test_locked_named_stream_leaves_the_previous_save_intact);
  return UNITY_END();
}
