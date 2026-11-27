/* Windows implementation of the private house save transaction in house.c. */
#ifndef TBA_HOUSE_WIN32_H
#define TBA_HOUSE_WIN32_H

#include <windows.h>
#include <aclapi.h>
#include <winioctl.h>
#include <fcntl.h>
#include <io.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <errno.h>

#ifdef _MSC_VER
#pragma comment(lib, "advapi32.lib")
#endif

struct house_save {
  HANDLE stage;
  wchar_t *dest;
  int existing;
};

/* NT paths are limited to 32,767 UTF-16 code units. Keep these buffers off
 * the game thread's stack. */
#define HOUSE_WIN_PATH 32768
#define HOUSE_WIN_SHARE (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)

static void House_win_errno(DWORD error)
{
  switch (error) {
  case ERROR_FILE_NOT_FOUND: case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
  case ERROR_ACCESS_DENIED: case ERROR_SHARING_VIOLATION: case ERROR_LOCK_VIOLATION: errno = EACCES; break;
  case ERROR_NOT_ENOUGH_MEMORY: case ERROR_OUTOFMEMORY: errno = ENOMEM; break;
  case ERROR_FILENAME_EXCED_RANGE: errno = ENAMETOOLONG; break;
  case ERROR_FILE_EXISTS: case ERROR_ALREADY_EXISTS: errno = EEXIST; break;
  case ERROR_DISK_FULL: errno = ENOSPC; break;
  default: errno = EIO; break;
  }
}

static DWORD House_win_final_path(HANDLE file, wchar_t *path)
{
  DWORD size = GetFinalPathNameByHandleW(file, path, HOUSE_WIN_PATH, FILE_NAME_NORMALIZED);
  if (!size)
    return GetLastError();
  return size < HOUSE_WIN_PATH ? ERROR_SUCCESS : ERROR_FILENAME_EXCED_RANGE;
}

/* Only the symlink variant of the public FSCTL_GET_REPARSE_POINT wire format
 * is needed. Offsets and lengths are in bytes relative to names. */
struct house_link_data {
  DWORD tag;
  WORD data_length, reserved;
  WORD substitute_offset, substitute_length, print_offset, print_length;
  DWORD flags;
  WCHAR names[1];
};

/* Resolve one dangling file symlink. The ordinary existing-file case is
 * resolved by the filesystem when the destination handle is opened. */
static DWORD House_win_follow_missing(HANDLE link, wchar_t *path)
{
  union {
    struct house_link_data link;
    BYTE raw[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
  } reparse;
  DWORD bytes, error;
  size_t offset, count, prefix;
  const wchar_t *target;
  wchar_t *base;

  if (!DeviceIoControl(link, FSCTL_GET_REPARSE_POINT, NULL, 0,
                       &reparse, sizeof(reparse), &bytes, NULL))
    return GetLastError();
  if (bytes < offsetof(struct house_link_data, names) || reparse.link.tag != IO_REPARSE_TAG_SYMLINK)
    return ERROR_INVALID_REPARSE_DATA;
  offset = offsetof(struct house_link_data, names) + reparse.link.substitute_offset;
  if (offset + reparse.link.substitute_length > bytes ||
      (reparse.link.substitute_offset | reparse.link.substitute_length) % sizeof(WCHAR))
    return ERROR_INVALID_REPARSE_DATA;
  count = reparse.link.substitute_length / sizeof(WCHAR);
  target = (const WCHAR *)(reparse.raw + offset);
  if (!count)
    return ERROR_INVALID_REPARSE_DATA;
  if (!(reparse.link.flags & SYMLINK_FLAG_RELATIVE)) {
    if (count <= 4 || wcsncmp(target, L"\\??\\", 4))
      return ERROR_INVALID_REPARSE_DATA;
    memcpy(path, target, count * sizeof(WCHAR));
    path[1] = L'\\'; /* NT \??\ -> Win32 \\?\ */
    path[count] = 0;
    return ERROR_SUCCESS;
  }
  if ((error = House_win_final_path(link, path)) != ERROR_SUCCESS)
    return error;
  /* DOS path normalization handles relative ".." components. An extended
   * path bypasses normalization; strip its drive or UNC marker first. */
  if (!wcsncmp(path, L"\\\\?\\UNC\\", 8)) {
    memmove(path + 2, path + 8, (wcslen(path + 8) + 1) * sizeof(WCHAR));
  } else if (!wcsncmp(path, L"\\\\?\\", 4) && path[5] == L':') {
    memmove(path, path + 4, (wcslen(path + 4) + 1) * sizeof(WCHAR));
  } else {
    return ERROR_BAD_PATHNAME;
  }
  base = wcsrchr(path, L'\\');
  if (!base)
    return ERROR_BAD_PATHNAME;
  prefix = (size_t)(base + 1 - path);
  if (prefix + count >= HOUSE_WIN_PATH)
    return ERROR_FILENAME_EXCED_RANGE;
  memcpy(path + prefix, target, count * sizeof(WCHAR));
  path[prefix + count] = 0;
  return ERROR_SUCCESS;
}

/* Return either an existing file's resolved name and DACL or a new target's
 * full name. Opening the final link itself distinguishes an absent target
 * from an absent link without creating or truncating either one. */
static DWORD House_win_destination(const char *filename, struct house_save *save,
                                   PSECURITY_DESCRIPTOR *security)
{
  wchar_t *path = malloc(HOUSE_WIN_PATH * sizeof(*path));
  DWORD error = ERROR_NOT_ENOUGH_MEMORY;
  if (!path)
    return error;
  if (!MultiByteToWideChar(CP_ACP, 0, filename, -1, path, HOUSE_WIN_PATH)) {
    error = GetLastError();
    free(path);
    return error;
  }
  for (int hop = 0; hop < 64; ++hop) {
    DWORD length = GetFullPathNameW(path, HOUSE_WIN_PATH, save->dest, NULL);
    HANDLE file;
    if (!length || length >= HOUSE_WIN_PATH) {
      error = length ? ERROR_FILENAME_EXCED_RANGE : GetLastError();
      break;
    }
    file = CreateFileW(save->dest, READ_CONTROL, HOUSE_WIN_SHARE, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE) {
      DWORD features = FILE_PERSISTENT_ACLS;
      error = House_win_final_path(file, save->dest);
      /* FAT has no per-file DACL to copy. If the volume query is unavailable,
       * still require the normal ACL operation to succeed. */
      if (!error && (!GetVolumeInformationByHandleW(file, NULL, 0, NULL, NULL, &features, NULL, 0) ||
                     (features & FILE_PERSISTENT_ACLS)))
        error = GetSecurityInfo(file, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                NULL, NULL, NULL, NULL, security);
      CloseHandle(file);
      save->existing = TRUE;
      break;
    }
    error = GetLastError();
    if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
      break;
    file = CreateFileW(save->dest, 0, HOUSE_WIN_SHARE, NULL, OPEN_EXISTING,
                       FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (file == INVALID_HANDLE_VALUE) {
      error = GetLastError();
      if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
        error = ERROR_SUCCESS;
      break;
    }
    error = House_win_follow_missing(file, path);
    CloseHandle(file);
    if (error)
      break;
    error = ERROR_CANT_RESOLVE_FILENAME;
  }
  free(path);
  return error;
}

static void House_discard_save(struct house_save *save)
{
  if (save->stage) {
    FILE_DISPOSITION_INFO disposition = { TRUE };
    SetFileInformationByHandle(save->stage, FileDispositionInfo, &disposition, sizeof(disposition));
    CloseHandle(save->stage);
    save->stage = NULL;
  }
  free(save->dest);
  save->dest = NULL;
  save->existing = FALSE;
}

static DWORD House_win_create_stage(struct house_save *save, PSECURITY_DESCRIPTOR security)
{
  static LONG sequence;
  wchar_t *name, *leaf = wcsrchr(save->dest, L'\\');
  size_t prefix;
  DWORD error = ERROR_FILE_EXISTS;
  SECURITY_ATTRIBUTES attrs = { sizeof(attrs), security, FALSE };
  if (!leaf)
    return ERROR_BAD_PATHNAME;
  prefix = (size_t)(leaf + 1 - save->dest);
  name = malloc((prefix + 40) * sizeof(*name));
  if (!name)
    return ERROR_NOT_ENOUGH_MEMORY;
  memcpy(name, save->dest, prefix * sizeof(*name));
  for (int i = 0; i < 256; ++i) {
    HANDLE file;
    swprintf(name + prefix, 40, L".tbahouse-%08lx-%08lx.tmp",
             (unsigned long)GetCurrentProcessId(), (unsigned long)InterlockedIncrement(&sequence));
    if (!_wcsicmp(name + prefix, leaf + 1))
      continue;
    file = CreateFileW(name, GENERIC_WRITE | DELETE | (security ? WRITE_DAC : 0),
                       0, &attrs, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE) {
      save->stage = file;
      error = ERROR_SUCCESS;
      break;
    }
    error = GetLastError();
    if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS)
      break;
  }
  free(name);
  return error;
}

static FILE *House_open_save(const char *filename, struct house_save *save)
{
  PSECURITY_DESCRIPTOR security = NULL;
  SECURITY_DESCRIPTOR_CONTROL control = 0;
  DWORD revision, error;
  HANDLE stream;
  int fd;
  FILE *fp;
  FILE_DISPOSITION_INFO discard_on_close = { TRUE };
  save->dest = malloc(HOUSE_WIN_PATH * sizeof(*save->dest));
  if (!save->dest)
    return NULL;
  error = House_win_destination(filename, save, &security);
  /* Protect the DACL during creation so no rule from the parent directory
   * can grant access to the staged contents. Restore inheritance if needed
   * before passing the stream to the record writer. */
  if (!error && security &&
      (!GetSecurityDescriptorControl(security, &control, &revision) ||
       !SetSecurityDescriptorControl(security, SE_DACL_PROTECTED, SE_DACL_PROTECTED)))
    error = GetLastError();
  if (!error)
    error = House_win_create_stage(save, security);
  if (!error && !SetFileInformationByHandle(save->stage, FileDispositionInfo, &discard_on_close,
                                             sizeof(discard_on_close)))
    error = GetLastError();
  if (!error && security) {
    SECURITY_DESCRIPTOR_CONTROL mask = SE_DACL_PROTECTED | SE_DACL_AUTO_INHERIT_REQ;
    SECURITY_DESCRIPTOR_CONTROL flags = control & SE_DACL_PROTECTED;
    if (control & SE_DACL_AUTO_INHERITED)
      flags |= SE_DACL_AUTO_INHERIT_REQ;
    if (!SetSecurityDescriptorControl(security, mask, flags) ||
        !SetKernelObjectSecurity(save->stage, DACL_SECURITY_INFORMATION, security))
      error = GetLastError();
  }
  LocalFree(security);
  if (!error && !DuplicateHandle(GetCurrentProcess(), save->stage, GetCurrentProcess(), &stream,
                                 0, FALSE, DUPLICATE_SAME_ACCESS))
    error = GetLastError();
  if (error) {
    House_discard_save(save);
    House_win_errno(error);
    return NULL;
  }
  fd = _open_osfhandle((intptr_t)stream, _O_WRONLY | _O_BINARY | _O_NOINHERIT);
  if (fd >= 0 && (fp = _fdopen(fd, "wb")) != NULL)
    return fp;
  int saved_errno = errno;
  if (fd < 0)
    CloseHandle(stream);
  else
    _close(fd);
  House_discard_save(save);
  errno = saved_errno;
  return NULL;
}

static int House_install_save(struct house_save *save)
{
  size_t bytes = wcslen(save->dest) * sizeof(WCHAR);
  DWORD size = (DWORD)(sizeof(FILE_RENAME_INFO) + bytes);
  FILE_RENAME_INFO *rename_info = calloc(1, size);
  DWORD error = ERROR_SUCCESS;
  FILE_DISPOSITION_INFO retain = { FALSE };
  if (!rename_info)
    return FALSE;
  rename_info->ReplaceIfExists = (BOOLEAN)save->existing;
  rename_info->FileNameLength = (DWORD)bytes;
  memcpy(rename_info->FileName, save->dest, bytes);
  /* Keep the unique stage delete-pending while it is being written, including
   * if the server exits. Clear this flag immediately before its rename. */
  if (!SetFileInformationByHandle(save->stage, FileDispositionInfo, &retain, sizeof(retain)) ||
      !SetFileInformationByHandle(save->stage, FileRenameInfo, rename_info, size))
    error = GetLastError();
  free(rename_info);
  if (error) {
    House_win_errno(error);
    return FALSE;
  }
  CloseHandle(save->stage);
  save->stage = NULL;
  return TRUE;
}

#endif
