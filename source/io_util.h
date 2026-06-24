/*
 * io_util.h - file copy + path helpers shared by all flows.
 */
#ifndef IO_UTIL_H
#define IO_UTIL_H

#include <stdbool.h>
#include <stddef.h>

bool copy_file(const char *src, const char *dst);
bool move_file(const char *src, const char *dst);  /* rename, falls back to copy+unlink */
bool ensure_dir(const char *path);                 /* mkdir -p style */

/* "YYYYMMDD-HHMMSS" from local time (or epoch on RTC failure). */
void timestamp_suffix(char *out, size_t cap);

/* Build "<backup_dir>/<base>.<ts>.<ext>" from in_path (basename of input,
 * strip extension, plant inside the user-managed backup dir instead of
 * littering the source dir). */
void output_path_for(const char *in_path, const char *ext, const char *ts,
                     const char *backup_dir, char *out, size_t cap);

/* "<backup_dir>/<basename>.bak" — one-time pre-overwrite backup. */
void backup_path_for(const char *in_path, const char *backup_dir,
                     char *out, size_t cap);

/* Keep at most `keep` of the most-recent timestamped backups in
 * `backup_dir` that match `<stem>.YYYYMMDD-HHMMSS.<ext>` where `stem`
 * comes from the basename of `in_path` with its extension stripped.
 * The `.bak` file is NEVER touched (it represents the original state).
 * Older timestamped files are removed. */
void prune_timestamped_backups(const char *in_path, const char *ext,
                               const char *backup_dir, int keep);

const char *basename_of(const char *p);

#endif
