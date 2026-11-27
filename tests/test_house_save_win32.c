/* Native filesystem tests for the Windows backend used by House_crashsave. */
#include "unity.h"
#include "house_win32.h"
#include <sddl.h>

static char house[MAX_PATH], link_name[MAX_PATH + sizeof(".link")];
static int owns_house, owns_link;

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
  owns_link = owns_house = 0;
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
static void read_dacl(char **sddl)
{
  PSECURITY_DESCRIPTOR security = NULL;
  TEST_ASSERT_EQUAL_UINT32(ERROR_SUCCESS, GetNamedSecurityInfoA(house, SE_FILE_OBJECT,
                           DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &security));
  BOOL converted = ConvertSecurityDescriptorToStringSecurityDescriptorA(
      security, SDDL_REVISION_1, DACL_SECURITY_INFORMATION, sddl, NULL);
  LocalFree(security);
  TEST_ASSERT_TRUE(converted);
}

static void test_save_preserves_a_protected_custom_dacl(void)
{
  PSECURITY_DESCRIPTOR security = NULL;
  TEST_ASSERT_TRUE(ConvertStringSecurityDescriptorToSecurityDescriptorW(
      L"D:P(A;;FA;;;OW)(A;;FR;;;SY)", SDDL_REVISION_1, &security, NULL));
  BOOL set = SetFileSecurityA(house, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, security);
  LocalFree(security);
  TEST_ASSERT_TRUE(set);
  char *before, *after;
  read_dacl(&before);
  save_house(house);
  read_dacl(&after);
  int same = strcmp(before, after) == 0;
  LocalFree(after);
  LocalFree(before);
  TEST_ASSERT_TRUE(same);
}

static void test_save_preserves_a_protected_dacl_without_write_dac(void)
{
  PSECURITY_DESCRIPTOR security = NULL;
  TEST_ASSERT_TRUE(ConvertStringSecurityDescriptorToSecurityDescriptorW(
      L"D:P(A;;FRFWSD;;;OW)", SDDL_REVISION_1, &security, NULL));
  BOOL set = SetFileSecurityA(house, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, security);
  LocalFree(security);
  TEST_ASSERT_TRUE(set);
  char *before, *after;
  read_dacl(&before);
  save_house(house);
  read_dacl(&after);
  int same = strcmp(before, after) == 0;
  LocalFree(after);
  LocalFree(before);
  TEST_ASSERT_TRUE(same);
}

static void test_save_preserves_an_inherited_dacl(void)
{
  char *before, *after;
  read_dacl(&before);
  save_house(house);
  read_dacl(&after);
  int same = strcmp(before, after) == 0;
  LocalFree(after);
  LocalFree(before);
  TEST_ASSERT_TRUE(same);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_save_updates_the_target_of_a_file_symlink);
  RUN_TEST(test_save_preserves_a_protected_custom_dacl);
  RUN_TEST(test_save_preserves_a_protected_dacl_without_write_dac);
  RUN_TEST(test_save_preserves_an_inherited_dacl);
  return UNITY_END();
}
