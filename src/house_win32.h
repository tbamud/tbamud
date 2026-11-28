/* Windows implementation of the private house save transaction in house.c. */
#ifndef TBA_HOUSE_WIN32_H
#define TBA_HOUSE_WIN32_H

#include <windows.h>
#include <aclapi.h>
#include <winefs.h>
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

struct house_win_metadata {
  PSECURITY_DESCRIPTOR security;
  DWORD volume_flags;
  DWORD attributes;
  int volume_flags_known;
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
                                   struct house_win_metadata *meta)
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
    file = CreateFileW(save->dest, READ_CONTROL | FILE_READ_ATTRIBUTES, HOUSE_WIN_SHARE, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE) {
      BY_HANDLE_FILE_INFORMATION info;
      error = House_win_final_path(file, save->dest);
      if (!error && !GetFileInformationByHandle(file, &info))
        error = GetLastError();
      if (!error)
        meta->attributes = info.dwFileAttributes;
      meta->volume_flags_known = GetVolumeInformationByHandleW(
          file, NULL, 0, NULL, NULL, &meta->volume_flags, NULL, 0) != 0;
      if (!meta->volume_flags_known)
        meta->volume_flags = FILE_PERSISTENT_ACLS | FILE_NAMED_STREAMS | FILE_FILE_COMPRESSION;
      if (!error && (meta->volume_flags & FILE_PERSISTENT_ACLS)) {
        error = GetSecurityInfo(file, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                NULL, NULL, NULL, NULL, &meta->security);
        if (!meta->volume_flags_known &&
            (error == ERROR_INVALID_FUNCTION || error == ERROR_NOT_SUPPORTED))
          error = ERROR_SUCCESS;
      }
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

static DWORD House_win_delete_on_close(HANDLE file, BOOLEAN enable)
{
  FILE_DISPOSITION_INFO disposition = { enable };
  return SetFileInformationByHandle(file, FileDispositionInfo, &disposition, sizeof(disposition))
         ? ERROR_SUCCESS : GetLastError();
}

static DWORD House_win_rename_file(HANDLE file, const wchar_t *dest, BOOLEAN replace)
{
  size_t bytes = wcslen(dest) * sizeof(WCHAR);
  DWORD size = (DWORD)(sizeof(FILE_RENAME_INFO) + bytes);
  FILE_RENAME_INFO *info = calloc(1, size);
  DWORD error;
  if (!info)
    return ERROR_NOT_ENOUGH_MEMORY;
  info->ReplaceIfExists = replace;
  info->FileNameLength = (DWORD)bytes;
  memcpy(info->FileName, dest, bytes);
  error = House_win_delete_on_close(file, FALSE);
  if (!error && !SetFileInformationByHandle(file, FileRenameInfo, info, size))
    error = GetLastError();
  free(info);
  return error;
}

static void House_discard_save(struct house_save *save)
{
  if (save->stage) {
    House_win_delete_on_close(save->stage, TRUE);
    CloseHandle(save->stage);
    save->stage = NULL;
  }
  free(save->dest);
  save->dest = NULL;
  save->existing = FALSE;
}

/* Temporarily restrict the empty stage file to the current user with full
 * access before copying named streams, so private stream bytes are never
 * exposed under a broader parent DACL and restrictive source DACLs cannot
 * block creating the destination streams. */
static DWORD House_win_owner_security(SECURITY_DESCRIPTOR *sd, PACL *result_acl)
{
  HANDLE token = NULL;
  union {
    TOKEN_USER user;
    BYTE bytes[256];
  } stack_buf;
  TOKEN_USER *token_user = &stack_buf.user;
  DWORD token_len = sizeof(stack_buf), error = ERROR_SUCCESS;
  PSID sid;
  DWORD acl_size;
  PACL acl = NULL;
  ACCESS_ALLOWED_ACE *ace;

  if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token)) {
    error = GetLastError();
    if (error != ERROR_NO_TOKEN || !OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
      return error == ERROR_NO_TOKEN ? GetLastError() : error;
    error = ERROR_SUCCESS;
  }
  if (!GetTokenInformation(token, TokenUser, token_user, token_len, &token_len)) {
    error = GetLastError();
    if (error == ERROR_INSUFFICIENT_BUFFER) {
      token_user = (TOKEN_USER *)malloc(token_len);
      if (!token_user) {
        error = ERROR_NOT_ENOUGH_MEMORY;
      } else if (GetTokenInformation(token, TokenUser, token_user, token_len, &token_len)) {
        error = ERROR_SUCCESS;
      } else {
        error = GetLastError();
      }
    }
  }
  CloseHandle(token);
  if (error) {
    if (token_user != &stack_buf.user)
      free(token_user);
    return error;
  }
  sid = token_user->User.Sid;
  acl_size = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(sid);
  acl = (PACL)malloc(acl_size);
  if (!acl) {
    error = ERROR_NOT_ENOUGH_MEMORY;
  } else if (!InitializeAcl(acl, acl_size, ACL_REVISION) ||
             !AddAccessAllowedAce(acl, ACL_REVISION, FILE_ALL_ACCESS, sid) ||
             !GetAce(acl, 0, (void **)&ace) ||
             !InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION) ||
             !SetSecurityDescriptorOwner(sd, &ace->SidStart, FALSE) ||
             !SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE) ||
             !SetSecurityDescriptorControl(sd, SE_DACL_PROTECTED, SE_DACL_PROTECTED)) {
    error = GetLastError();
  }
  if (error)
    free(acl);
  else
    *result_acl = acl;
  if (token_user != &stack_buf.user)
    free(token_user);
  return error;
}

static DWORD House_win_set_owner_dacl(HANDLE stage)
{
  SECURITY_DESCRIPTOR security;
  PACL acl = NULL;
  DWORD error = House_win_owner_security(&security, &acl);
  if (!error && !SetKernelObjectSecurity(stage, DACL_SECURITY_INFORMATION, &security))
    error = GetLastError();
  free(acl);
  return error;
}

static DWORD House_win_copy_stream(const struct house_save *save, const wchar_t *stage, const wchar_t *stream)
{
  const wchar_t *source = save->dest;
  size_t source_length = wcslen(source), stage_length = wcslen(stage);
  size_t capacity = (source_length > stage_length ? source_length : stage_length) + wcslen(stream) + 1;
  wchar_t *path = malloc(capacity * sizeof(*path));
  HANDLE from = INVALID_HANDLE_VALUE, to = INVALID_HANDLE_VALUE;
  BYTE buffer[16384];
  DWORD error = ERROR_SUCCESS, count, written;
  if (!path)
    return ERROR_NOT_ENOUGH_MEMORY;
  memcpy(path, source, source_length * sizeof(*path));
  wcscpy(path + source_length, stream);
  from = CreateFileW(path, FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (from == INVALID_HANDLE_VALUE) {
    error = GetLastError();
  } else {
    memcpy(path, stage, stage_length * sizeof(*path));
    wcscpy(path + stage_length, stream);
    /* Windows requires the file to have a live name to open a new stream.
     * Restore automatic cleanup before copying any of the stream's bytes. */
    error = House_win_delete_on_close(save->stage, FALSE);
    if (!error) {
      to = CreateFileW(path, FILE_WRITE_DATA, FILE_SHARE_DELETE, NULL,
                       CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
      if (to == INVALID_HANDLE_VALUE)
        error = GetLastError();
      DWORD cleanup_error = House_win_delete_on_close(save->stage, TRUE);
      if (!error)
        error = cleanup_error;
    }
  }
  while (!error) {
    if (!ReadFile(from, buffer, sizeof(buffer), &count, NULL))
      error = GetLastError();
    else if (!count)
      break;
    else if (!WriteFile(to, buffer, count, &written, NULL))
      error = GetLastError();
    else if (written != count)
      error = ERROR_WRITE_FAULT;
  }
  if (!error && !FlushFileBuffers(to))
    error = GetLastError();
  if (to != INVALID_HANDLE_VALUE)
    CloseHandle(to);
  if (from != INVALID_HANDLE_VALUE)
    CloseHandle(from);
  free(path);
  return error;
}

static int House_win_same_dacl(PSECURITY_DESCRIPTOR source, PSECURITY_DESCRIPTOR stage)
{
  SECURITY_DESCRIPTOR_CONTROL source_flags, stage_flags;
  PACL a = NULL, b = NULL;
  BOOL present_a, present_b, unused;
  DWORD revision;
  if (!GetSecurityDescriptorControl(source, &source_flags, &revision) ||
      !GetSecurityDescriptorControl(stage, &stage_flags, &revision) ||
      ((source_flags ^ stage_flags) & (SE_DACL_PROTECTED | SE_DACL_AUTO_INHERITED)) ||
      !GetSecurityDescriptorDacl(source, &present_a, &a, &unused) ||
      !GetSecurityDescriptorDacl(stage, &present_b, &b, &unused) || present_a != present_b)
    return FALSE;
  if (!present_a || !a || !b)
    return !present_a || a == b;
  if (a->AclRevision != b->AclRevision || a->AceCount != b->AceCount)
    return FALSE;
  for (WORD i = 0; i < a->AceCount; ++i) {
    ACE_HEADER *old_ace, *new_ace;
    if (!GetAce(a, i, (LPVOID *)&old_ace) || !GetAce(b, i, (LPVOID *)&new_ace) ||
        old_ace->AceSize != new_ace->AceSize || memcmp(old_ace, new_ace, old_ace->AceSize))
      return FALSE;
  }
  return TRUE;
}

static DWORD House_win_copy_streams(const struct house_save *save, const wchar_t *stage,
                                    const struct house_win_metadata *meta, int protect_before_write)
{
  WIN32_FIND_STREAM_DATA stream;
  HANDLE list = FindFirstStreamW(save->dest, FindStreamInfoStandard, &stream, 0);
  DWORD error;
  if (list == INVALID_HANDLE_VALUE) {
    error = GetLastError();
    if (error == ERROR_HANDLE_EOF)
      return ERROR_SUCCESS;
    /* FindFirstStreamW reports ERROR_INVALID_PARAMETER on filesystems without streams. */
    if (!meta->volume_flags_known &&
        (error == ERROR_INVALID_PARAMETER || error == ERROR_INVALID_FUNCTION || error == ERROR_NOT_SUPPORTED))
      return ERROR_SUCCESS;
    return error;
  }
  do {
    /* The house serializer supplies the unnamed data stream. */
    if (_wcsicmp(stream.cStreamName, L"::$DATA")) {
      if (protect_before_write) {
        error = House_win_set_owner_dacl(save->stage);
        if (error == ERROR_ACCESS_DENIED && meta->security) {
          PSECURITY_DESCRIPTOR actual = NULL;
          if (GetSecurityInfo(save->stage, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                              NULL, NULL, NULL, NULL, &actual) == ERROR_SUCCESS &&
              House_win_same_dacl(meta->security, actual))
            error = ERROR_SUCCESS;
          LocalFree(actual);
        }
        if (error) {
          FindClose(list);
          return error;
        }
        protect_before_write = FALSE;
      }
      error = House_win_copy_stream(save, stage, stream.cStreamName);
      if (error) {
        FindClose(list);
        return error;
      }
    }
  } while (FindNextStreamW(list, &stream));
  error = GetLastError();
  FindClose(list);
  return error == ERROR_HANDLE_EOF ? ERROR_SUCCESS : error;
}

static DWORD House_win_set_compression(HANDLE stage, DWORD source_attributes)
{
  BY_HANDLE_FILE_INFORMATION info;
  USHORT format = (source_attributes & FILE_ATTRIBUTE_COMPRESSED) ? COMPRESSION_FORMAT_DEFAULT : COMPRESSION_FORMAT_NONE;
  DWORD bytes;
  if (!GetFileInformationByHandle(stage, &info))
    return GetLastError();
  if (((source_attributes ^ info.dwFileAttributes) & FILE_ATTRIBUTE_COMPRESSED) &&
      !DeviceIoControl(stage, FSCTL_SET_COMPRESSION, &format, sizeof(format), NULL, 0, &bytes, NULL))
    return GetLastError();
  return ERROR_SUCCESS;
}

static DWORD House_win_set_attributes(HANDLE stage, DWORD source_attributes)
{
  const DWORD mask = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
  FILE_BASIC_INFO info;
  DWORD desired;
  if (!GetFileInformationByHandleEx(stage, FileBasicInfo, &info, sizeof(info)))
    return GetLastError();
  desired = (info.FileAttributes & ~mask) | (source_attributes & mask);
  if (desired == info.FileAttributes)
    return ERROR_SUCCESS;
  memset(&info, 0, sizeof(info));
  info.FileAttributes = desired ? desired : FILE_ATTRIBUTE_NORMAL;
  return SetFileInformationByHandle(stage, FileBasicInfo, &info, sizeof(info))
         ? ERROR_SUCCESS : GetLastError();
}

/* EFS grants decryption to certificate keys. User SIDs and display names
 * are optional, and the order returned by EFS is immaterial. */
static int House_win_contains_efs_keys(const ENCRYPTION_CERTIFICATE_HASH_LIST *haystack,
                                       const ENCRYPTION_CERTIFICATE_HASH_LIST *needles)
{
  if ((needles->nCert_Hash && !needles->pUsers) || (haystack->nCert_Hash && !haystack->pUsers))
    return FALSE;
  for (DWORD i = 0; i < needles->nCert_Hash; ++i) {
    const EFS_HASH_BLOB *key = needles->pUsers[i] ? needles->pUsers[i]->pHash : NULL;
    DWORD j;
    if (!key || !key->cbData || !key->pbData)
      return FALSE;
    for (j = 0; j < haystack->nCert_Hash; ++j) {
      const EFS_HASH_BLOB *candidate = haystack->pUsers[j] ? haystack->pUsers[j]->pHash : NULL;
      if (candidate && candidate->pbData && key->cbData == candidate->cbData &&
          !memcmp(key->pbData, candidate->pbData, key->cbData))
        break;
    }
    if (j == haystack->nCert_Hash)
      return FALSE;
  }
  return TRUE;
}

static DWORD House_win_verify_efs_keys(const wchar_t *source, const wchar_t *stage,
    DWORD (WINAPI *query)(LPCWSTR, PENCRYPTION_CERTIFICATE_HASH_LIST *))
{
  PENCRYPTION_CERTIFICATE_HASH_LIST old = NULL, fresh = NULL;
  DWORD error = query(source, &old);
  if (!error)
    error = query(stage, &fresh);
  if (!error && (!old || !fresh || !House_win_contains_efs_keys(old, fresh) ||
                 !House_win_contains_efs_keys(fresh, old)))
    error = ERROR_ENCRYPTION_FAILED;
  if (old)
    FreeEncryptionCertificateHashList(old);
  if (fresh)
    FreeEncryptionCertificateHashList(fresh);
  return error;
}

static DWORD House_win_check_private_dir(HANDLE dir, SECURITY_DESCRIPTOR *expected)
{
  BY_HANDLE_FILE_INFORMATION info;
  PSECURITY_DESCRIPTOR security = NULL;
  PSID owner;
  DWORD error;
  if (!GetFileInformationByHandle(dir, &info))
    return GetLastError();
  if ((info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != FILE_ATTRIBUTE_DIRECTORY)
    return ERROR_ACCESS_DENIED;
  error = GetSecurityInfo(dir, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                          &owner, NULL, NULL, NULL, &security);
  if (!error && (!EqualSid(owner, expected->Owner) || !House_win_same_dacl(expected, security)))
    error = ERROR_ACCESS_DENIED;
  LocalFree(security);
  return error;
}

/* DuplicateEncryptionInfoFile takes paths and needs exclusive file access.
 * Reserve a private directory for its CREATE_NEW output and deny rename of
 * the directory while verifying and opening that output. Only the verified,
 * still-empty file moves to the ordinary stage name. */
static HANDLE House_win_create_efs_stage(const wchar_t *source, const wchar_t *name, DWORD access, DWORD attributes)
{
  static const wchar_t suffix[] = L".efs", child[] = L"\\stage";
  size_t length = wcslen(name);
  wchar_t *path = malloc(length * sizeof(*path) + sizeof(suffix) + sizeof(child));
  HANDLE dir = INVALID_HANDLE_VALUE, file = INVALID_HANDLE_VALUE;
  SECURITY_DESCRIPTOR sd;
  SECURITY_ATTRIBUTES sa = {sizeof(sa), &sd, FALSE};
  PACL acl = NULL;
  int private_dir = FALSE, remove_child = FALSE;
  DWORD error = ERROR_NOT_ENOUGH_MEMORY;
  if (!path)
    goto done;
  wcscpy(path, name);
  wcscat(path, suffix);
  error = House_win_owner_security(&sd, &acl);
  if (error)
    goto done;
  if (!CreateDirectoryW(path, &sa)) {
    error = GetLastError();
    goto done;
  }
  dir = CreateFileW(path, FILE_LIST_DIRECTORY | FILE_TRAVERSE | FILE_READ_ATTRIBUTES | READ_CONTROL | DELETE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
  if (dir == INVALID_HANDLE_VALUE) {
    error = GetLastError();
    goto done;
  }
  error = House_win_check_private_dir(dir, &sd);
  if (error)
    goto done;
  private_dir = TRUE;
  wcscat(path, child);
  if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES) {
    error = ERROR_FILE_EXISTS;
    goto done;
  }
  if ((error = GetLastError()) != ERROR_FILE_NOT_FOUND)
    goto done;
  error = DuplicateEncryptionInfoFile(source, path, CREATE_NEW, attributes, &sa);
  /* A provider can fail after creating a partial output. CREATE_NEW reports
   * name collisions separately; leave any such entry alone. */
  remove_child = error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS;
  if (!error)
    error = House_win_verify_efs_keys(source, path, QueryUsersOnEncryptedFile);
  if (!error)
    error = House_win_verify_efs_keys(source, path, QueryRecoveryAgentsOnEncryptedFile);
  if (error) {
    log("SYSERR: Cannot copy and verify EFS user and recovery certificates for the house save "
        "(Windows error %lu). Check its EFS recipients and recovery policy before retrying.",
        (unsigned long)error);
    goto done;
  }
  file = CreateFileW(path, access | WRITE_DAC, 0, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, NULL);
  if (file == INVALID_HANDLE_VALUE) {
    error = GetLastError();
    goto done;
  }
  error = House_win_rename_file(file, name, FALSE);

done:
  if (error && file != INVALID_HANDLE_VALUE) {
    House_win_delete_on_close(file, TRUE);
    CloseHandle(file);
    file = INVALID_HANDLE_VALUE;
  } else if (error && remove_child) {
    DeleteFileW(path);
  }
  if (private_dir) {
    DWORD removal = House_win_delete_on_close(dir, TRUE);
    if (removal)
      log("SYSERR: Could not remove the EFS house staging directory (Windows error %lu).",
          (unsigned long)removal);
  }
  if (dir != INVALID_HANDLE_VALUE)
    CloseHandle(dir);
  free(acl);
  free(path);
  SetLastError(error);
  return file;
}

/* Establish the original access rules on the empty, exclusively opened file
 * before any content is copied. A directory that grants only inherited
 * Modify access may refuse an ACL write even when the DACL already matches. */
static DWORD House_win_set_dacl(HANDLE stage, PSECURITY_DESCRIPTOR security)
{
  SECURITY_DESCRIPTOR_CONTROL control;
  PSECURITY_DESCRIPTOR actual = NULL;
  DWORD revision, error;
  if (!GetSecurityDescriptorControl(security, &control, &revision) ||
      !SetSecurityDescriptorControl(security, SE_DACL_AUTO_INHERIT_REQ,
              (control & SE_DACL_AUTO_INHERITED) ? SE_DACL_AUTO_INHERIT_REQ : 0))
    return GetLastError();
  if (SetKernelObjectSecurity(stage, DACL_SECURITY_INFORMATION, security))
    return ERROR_SUCCESS;
  error = GetLastError();
  if (error == ERROR_ACCESS_DENIED &&
      GetSecurityInfo(stage, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &actual) == ERROR_SUCCESS &&
      House_win_same_dacl(security, actual))
    error = ERROR_SUCCESS;
  LocalFree(actual);
  return error;
}

static HANDLE House_win_create_stage_file(const wchar_t *name, DWORD base_access,
                                          DWORD opt_write_dac, DWORD opt_read_data,
                                          DWORD attributes, int *have_write_dac)
{
  DWORD attempts[4];
  int count = 0;
  attempts[count++] = base_access | opt_write_dac | opt_read_data;
  if (opt_write_dac)
    attempts[count++] = base_access | opt_read_data;
  if (opt_read_data) {
    if (opt_write_dac)
      attempts[count++] = base_access | opt_write_dac;
    attempts[count++] = base_access;
  }
  for (int i = 0; i < count; ++i) {
    HANDLE file = CreateFileW(name, attempts[i], 0, NULL, CREATE_NEW, attributes, NULL);
    if (file != INVALID_HANDLE_VALUE) {
      *have_write_dac = (attempts[i] & WRITE_DAC) != 0;
      return file;
    }
    if (GetLastError() != ERROR_ACCESS_DENIED)
      break;
  }
  *have_write_dac = FALSE;
  return INVALID_HANDLE_VALUE;
}

static DWORD House_win_create_stage(struct house_save *save, const struct house_win_metadata *meta)
{
  static LONG sequence;
  wchar_t *name, *leaf = wcsrchr(save->dest, L'\\');
  size_t prefix;
  DWORD error = ERROR_FILE_EXISTS;
  DWORD access = GENERIC_WRITE | DELETE;
  DWORD opt_write_dac = meta->security ? WRITE_DAC : 0;
  DWORD opt_read_data = 0;
  DWORD preserved = meta->attributes &
      (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ENCRYPTED);
  DWORD attributes = preserved ? preserved : FILE_ATTRIBUTE_NORMAL;
  int have_write_dac = FALSE;
  /* An EFS copy starts with the game's private DACL. Without the source
   * descriptor its other authorized readers could lose filesystem access. */
  if ((meta->attributes & FILE_ATTRIBUTE_ENCRYPTED) && !meta->security) {
    log("SYSERR: Cannot read the encrypted house file's DACL from this filesystem. "
        "The previous save was kept.");
    return ERROR_NOT_SUPPORTED;
  }
  if (!leaf)
    return ERROR_BAD_PATHNAME;
  prefix = (size_t)(leaf + 1 - save->dest);
  name = malloc((prefix + 40) * sizeof(*name));
  if (!name)
    return ERROR_NOT_ENOUGH_MEMORY;
  memcpy(name, save->dest, prefix * sizeof(*name));
  if (save->existing) {
    access |= FILE_READ_ATTRIBUTES;
    if (meta->volume_flags & FILE_FILE_COMPRESSION) {
      /* A new file inherits directory compression. Ask for read-data access
       * when FSCTL_SET_COMPRESSION will need to change that setting, or as an
       * optional right if the parent's attributes could not be inspected. */
      name[prefix] = L'\0';
      DWORD parent = GetFileAttributesW(name);
      if (parent == INVALID_FILE_ATTRIBUTES)
        opt_read_data = FILE_READ_DATA;
      else if ((parent ^ meta->attributes) & FILE_ATTRIBUTE_COMPRESSED)
        access |= FILE_READ_DATA;
    }
  }
  for (int i = 0; i < 256; ++i) {
    HANDLE file;
    swprintf(name + prefix, 40, L".tbahouse-%08lx-%08lx.tmp",
             (unsigned long)GetCurrentProcessId(), (unsigned long)InterlockedIncrement(&sequence));
    if (!_wcsicmp(name + prefix, leaf + 1))
      continue;
    if (meta->attributes & FILE_ATTRIBUTE_ENCRYPTED) {
      file = House_win_create_efs_stage(save->dest, name, access | opt_read_data, attributes);
      have_write_dac = TRUE;
    } else {
      file = House_win_create_stage_file(name, access, opt_write_dac, opt_read_data,
                                         attributes, &have_write_dac);
    }
    if (file != INVALID_HANDLE_VALUE) {
      save->stage = file;
      error = ERROR_SUCCESS;
      break;
    }
    error = GetLastError();
    if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS)
      break;
  }
  if (!error)
    error = House_win_delete_on_close(save->stage, TRUE);
  if (!error && meta->security && !have_write_dac)
    error = House_win_set_dacl(save->stage, meta->security);
  if (!error && save->existing)
    error = House_win_set_attributes(save->stage, meta->attributes);
  if (!error && save->existing)
    error = House_win_set_compression(save->stage, meta->attributes);
  if (!error && save->existing && (meta->volume_flags & FILE_NAMED_STREAMS))
    error = House_win_copy_streams(save, name, meta, have_write_dac && meta->security != NULL);
  if (!error && meta->security && have_write_dac)
    error = House_win_set_dacl(save->stage, meta->security);
  free(name);
  return error;
}

static FILE *House_open_save(const char *filename, struct house_save *save)
{
  struct house_win_metadata meta = {0};
  DWORD error;
  HANDLE stream;
  int fd;
  FILE *fp;
  save->dest = malloc(HOUSE_WIN_PATH * sizeof(*save->dest));
  if (!save->dest)
    return NULL;
  error = House_win_destination(filename, save, &meta);
  if (!error)
    error = House_win_create_stage(save, &meta);
  LocalFree(meta.security);
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
  /* Keep the unique stage delete-pending while it is being written, including
   * if the server exits. Clear this flag immediately before its rename. */
  DWORD error = House_win_rename_file(save->stage, save->dest, (BOOLEAN)save->existing);
  if (error) {
    House_win_errno(error);
    return FALSE;
  }
  CloseHandle(save->stage);
  save->stage = NULL;
  return TRUE;
}

#endif
