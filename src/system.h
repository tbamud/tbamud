/**
 * @file system.h
 * @brief Declares the portable interface for operating system operations.
 *
 * @details Exposes file, directory, external command, and archive wrappers used by the engine on
 * Unix, macOS, and Windows.
 *
 * @author Victor Augusto Borges Dias de Almeida (Stoneheart) (BrMUD Engine, 2002-2026).
 *
 * @copyright Victor Augusto Borges Dias de Almeida (C) 2002 - 2026.
 * @date 2026-07-30
 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

int system_make_directory(const char *path);
int system_remove_file(const char *path);
int system_rename_file(const char *old_path, const char *new_path);
int system_run_command(const char *command);
int system_command_exists(const char *command);
int system_remove_archive_pair(const char *folder, const char *base_name);
int system_archive_tar_gz(const char *folder, const char *base_name, const char *files);

#endif /* _SYSTEM_H_ */
