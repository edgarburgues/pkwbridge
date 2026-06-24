/*
 * file_list.h - SD directory scanning + file picker for pkwbridge.
 *
 * Lists files matching extension(s) and (optionally) a specific size.
 * For pweep dumps the size filter is PWEEP_SIZE (65536 B); for HGSS
 * saves it is HGSS_SAV_SIZE (524288 B). 0 disables the size filter.
 *
 * The picker shares the same struct so it can scroll, paint a cursor,
 * etc., without ad-hoc state in each flow.
 */
#ifndef FILE_LIST_H
#define FILE_LIST_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>

#define MAX_FILES     64
#define MAX_NAME_LEN  96
/* Visible rows in pickers. citro2d top screen has ~190 px below the
 * title bar; each row is LINE_H+2 = 18 px; pickers with a 2-line header
 * (dir + count) start drawing at y=60, leaving room for 8 rows before
 * hitting the footer bar at y=221. Box/slot pickers with smaller headers
 * use their own count. */
#define VIS_ROWS       8

typedef struct {
    char  dir[MAX_NAME_LEN];
    char  names[MAX_FILES][MAX_NAME_LEN];
    off_t sizes[MAX_FILES];
    int   count;
    int   cursor;
    int   scroll;
} file_list_t;

bool ends_with_ci(const char *s, const char *suf);

/* Try primary dir; if empty, try each fallback. Returns true if any
 * matching file found. */
bool build_file_list(file_list_t *fl, const char *primary,
                     const char *const *fallbacks, int n_fallbacks,
                     const char *ext1, const char *ext2,
                     off_t expected_size);

/* Same but drops .bak and timestamped entries so the user sees ONLY
 * the canonical file (typically a single entry). */
bool build_canonical_list(file_list_t *fl, const char *primary,
                          const char *const *fallbacks, int n_fallbacks,
                          const char *ext, off_t expected_size);

void file_full_path(const file_list_t *fl, int idx, char *out, size_t cap);

/* Find an entry by full path. Returns index or -1. */
int file_list_find(const file_list_t *fl, const char *full_path);

/* Picker returns selected index, -1 on B (back), -2 on START (exit). */
int run_file_picker(const char *title, file_list_t *fl);

/* Backup-aware variant: shows tags [ok]/[bak]/[ts] next to each name. */
int run_backup_picker(const char *title, file_list_t *fl);

/* Scan ANY .rom/.sav/.bak in dir (used by backup browser). */
bool scan_all_files(file_list_t *fl, const char *dir, off_t expected_size);

/* Classify a backup filename:
 *   0 canonical (foo.rom)
 *   1 .bak      (foo.rom.bak)
 *   2 timestamped output (foo.YYYYMMDD-HHMMSS.rom)
 * Writes the canonical name to out_canon. */
int classify_backup(const char *name, char *out_canon, size_t cap);

#endif
