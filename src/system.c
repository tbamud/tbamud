/**
 * @file system.c
 * @brief Implements portable system operations across Unix, macOS, and Windows.
 *
 * @details Centralizes file, directory, external command, and tar archive operations.
 *
 * @author Victor Augusto Borges Dias de Almeida (Stoneheart) (BrMUD Engine, 2002-2026).
 *
 * @copyright Victor Augusto Borges Dias de Almeida (C) 2002 - 2026.
 * @date 2026-07-30
 */

#include "conf.h"
#include "sysdep.h"

#include "system.h"

#if defined(CIRCLE_WINDOWS)
#include <windows.h>
#endif

/**
 * @brief Creates a directory portably.
 *
 * @details On Unix, requests permissions 0777, still subject to the process umask. An existing
 * path is treated as success only when it is a directory.
 *
 * @param path Non-empty path of the directory to create.
 * @return 0 when the directory is created or already exists; -1 for an invalid argument or a
 * system failure.
 */
int system_make_directory(const char *path)
{
  int result;

  if (!path || !*path) {
    errno = EINVAL;
    return -1;
  }

#if defined(CIRCLE_WINDOWS)
  result = _mkdir(path);
#else
  result = mkdir(path, 0777);
#endif

  if (result == 0)
    return 0;

  if (errno == EEXIST) {
#if defined(CIRCLE_WINDOWS)
    DWORD attributes = GetFileAttributesA(path);

    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY))
      return 0;
#else
    struct stat info;

    if (stat(path, &info) == 0 && S_ISDIR(info.st_mode))
      return 0;
#endif
  }

  return -1;
}

/**
 * @brief Removes a file idempotently and portably.
 *
 * @details A missing file is treated as success. On Windows, attempts to clear the read-only
 * attribute when the first removal fails because access was denied.
 *
 * @param path Non-empty path of the file to remove.
 * @return 0 when the file was removed or did not exist; -1 on failure.
 */
int system_remove_file(const char *path)
{
  if (!path || !*path) {
    errno = EINVAL;
    return -1;
  }

  if (remove(path) == 0 || errno == ENOENT)
    return 0;

#if defined(CIRCLE_WINDOWS)
  if (errno == EACCES) {
    DWORD attributes = GetFileAttributesA(path);

    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_READONLY)) {
      if (SetFileAttributesA(path, attributes & ~FILE_ATTRIBUTE_READONLY)) {
        if (remove(path) == 0 || errno == ENOENT)
          return 0;
      }
    }
  }
#endif

  return -1;
}

/**
 * @brief Renames or moves a file, replacing the destination when supported.
 *
 * @details On POSIX systems, rename() replaces the destination atomically when both paths are on
 * the same filesystem. On Windows, MoveFileExA() requests replacement of an existing destination.
 *
 * @param old_path Non-empty path of the source file.
 * @param new_path Non-empty path of the destination file.
 * @return 0 on success; -1 for an invalid argument or a system failure.
 */
int system_rename_file(const char *old_path, const char *new_path)
{
  if (!old_path || !*old_path || !new_path || !*new_path) {
    errno = EINVAL;
    return -1;
  }

#if defined(CIRCLE_WINDOWS)
  if (MoveFileExA(old_path, new_path, MOVEFILE_REPLACE_EXISTING))
    return 0;
  return -1;
#else
  return rename(old_path, new_path);
#endif
}

/**
 * @brief Executes a command through the operating system shell.
 *
 * @details The text is passed directly to system(); callers must provide a trusted command that
 * is correctly escaped for the target platform.
 *
 * @param command Non-empty command to execute.
 * @return The status returned by system(), or -1 when the argument is null or empty.
 */
int system_run_command(const char *command)
{
  if (!command || !*command) {
    errno = EINVAL;
    return -1;
  }

  return system(command);
}

/**
 * @brief Checks whether a command is available in the execution environment.
 *
 * @details Uses where on Windows and command -v on other platforms. The argument is inserted
 * directly into the probe command and must be a trusted executable name.
 *
 * @param command Non-empty executable name to locate.
 * @return 1 when the command is found; 0 when it is missing or the probe is invalid.
 */
int system_command_exists(const char *command)
{
  char probe[512];
  int length;

  if (!command || !*command)
    return 0;

#if defined(CIRCLE_WINDOWS)
  length = snprintf(probe, sizeof(probe), "where %s >NUL 2>NUL", command);
#else
  length = snprintf(probe, sizeof(probe), "command -v %s >/dev/null 2>&1", command);
#endif

  if (length < 0 || (size_t)length >= sizeof(probe))
    return 0;

  return system_run_command(probe) == 0;
}

/**
 * @brief Removes the .tar.gz and .tar variants of an archive base name.
 *
 * @details The folder and base name are concatenated directly; callers must include the required
 * trailing path separator in folder. Files that are already missing count as removed.
 *
 * @param folder Directory prefix containing the archive files.
 * @param base_name Archive base name without an extension.
 * @return 0 when both variants are absent afterward; -1 if either removal fails.
 */
int system_remove_archive_pair(const char *folder, const char *base_name)
{
  char path[1024];
  int result = 0;
  int length;

  if (!folder || !*folder || !base_name || !*base_name) {
    errno = EINVAL;
    return -1;
  }

  length = snprintf(path, sizeof(path), "%s%s.tar.gz", folder, base_name);
  if (length < 0 || (size_t)length >= sizeof(path) || system_remove_file(path) != 0)
    result = -1;

  length = snprintf(path, sizeof(path), "%s%s.tar", folder, base_name);
  if (length < 0 || (size_t)length >= sizeof(path) || system_remove_file(path) != 0)
    result = -1;

  return result;
}

/**
 * @brief Creates a gzip-compressed tar archive with the external tar command.
 *
 * @details Builds the destination by concatenating folder and base_name. The files list is
 * inserted directly into the command and must contain only trusted names accepted by the shell.
 * The operation is not attempted when tar is unavailable.
 *
 * @param folder Source directory, including the trailing separator used in the destination path.
 * @param base_name Non-empty base name of the output archive.
 * @param files Non-empty list of files to include in the archive.
 * @return The status returned by tar; -1 for invalid input or command preparation failure.
 */
int system_archive_tar_gz(const char *folder, const char *base_name, const char *files)
{
  char command[2048];
  int length;

  if (!folder || !*folder || !base_name || !*base_name || !files || !*files) {
    errno = EINVAL;
    return -1;
  }

  if (!system_command_exists("tar"))
    return -1;

  length = snprintf(command, sizeof(command), "tar -czf \"%s%s.tar.gz\" -C \"%s\" %s",
                    folder, base_name, folder, files);
  if (length < 0 || (size_t)length >= sizeof(command))
    return -1;

  return system_run_command(command);
}
