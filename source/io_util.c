#include "io_util.h"

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define MAX_PATH_LEN  192

bool copy_file(const char *src, const char *dst) {
    FILE *fi = fopen(src, "rb");
    if (!fi) return false;
    FILE *fo = fopen(dst, "wb");
    if (!fo) { fclose(fi); return false; }
    uint8_t buf[4096];
    bool ok = true;
    for (;;) {
        size_t n = fread(buf, 1, sizeof(buf), fi);
        if (n == 0) break;
        if (fwrite(buf, 1, n, fo) != n) { ok = false; break; }
    }
    if (ferror(fi)) ok = false;
    fclose(fi);
    if (fclose(fo) != 0) ok = false;
    return ok;
}

bool move_file(const char *src, const char *dst) {
    /* Try rename first (atomic on the same FS — SD card is a single FAT
     * volume so this always works in our case). Fall back to copy+unlink
     * for cross-fs cases that shouldn't really happen here. */
    if (rename(src, dst) == 0) return true;
    if (!copy_file(src, dst))  return false;
    if (unlink(src) != 0)      return false;
    return true;
}

bool ensure_dir(const char *path) {
    /* mkdir -p: walk the path, mkdir at each '/' boundary. Treats
     * existing dirs as success (we don't check errno; if the final dir
     * exists after the calls, we're good). */
    char buf[MAX_PATH_LEN];
    snprintf(buf, sizeof buf, "%s", path);
    char *p = buf;
    if (strncmp(p, "sdmc:", 5) == 0) p += 5;
    if (*p == '/') p++;
    while (*p) {
        if (*p == '/') {
            *p = 0;
            mkdir(buf, 0777);
            *p = '/';
        }
        p++;
    }
    mkdir(buf, 0777);
    /* Probe with stat to confirm. */
    struct stat st;
    return stat(buf, &st) == 0 && S_ISDIR(st.st_mode);
}

void timestamp_suffix(char *out, size_t cap) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    if (tm) snprintf(out, cap, "%04d%02d%02d-%02d%02d%02d",
                      tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                      tm->tm_hour, tm->tm_min, tm->tm_sec);
    else    snprintf(out, cap, "%lu", (unsigned long)t);
}

void output_path_for(const char *in_path, const char *ext, const char *ts,
                     const char *backup_dir, char *out, size_t cap)
{
    /* basename(in_path) stripped of extension, then prefix with backup_dir */
    const char *slash = strrchr(in_path, '/');
    const char *base  = slash ? slash + 1 : in_path;
    char stem[MAX_PATH_LEN];
    snprintf(stem, sizeof stem, "%s", base);
    char *dot = strrchr(stem, '.');
    if (dot) *dot = 0;
    snprintf(out, cap, "%s/%s.%s.%s", backup_dir, stem, ts, ext);
}

void backup_path_for(const char *in_path, const char *backup_dir,
                     char *out, size_t cap)
{
    const char *slash = strrchr(in_path, '/');
    const char *base  = slash ? slash + 1 : in_path;
    snprintf(out, cap, "%s/%s.bak", backup_dir, base);
}

const char *basename_of(const char *p) {
    const char *s = strrchr(p, '/');
    return s ? s + 1 : p;
}

/* Parse `<stem>.YYYYMMDD-HHMMSS.<ext>` and write the 15-char timestamp
 * into `out_ts` (NUL-terminated, cap >= 16). Returns true if the name
 * matches; otherwise out_ts is unchanged. */
static bool parse_ts_name(const char *name, const char *stem, const char *ext,
                          char out_ts[16])
{
    size_t lst = strlen(stem);
    size_t lex = strlen(ext);
    size_t ln  = strlen(name);
    /* Required shape: stem(lst) + '.' + 15-char-ts + '.' + ext(lex) */
    if (ln != lst + 1 + 15 + 1 + lex) return false;
    if (memcmp(name, stem, lst) != 0) return false;
    if (name[lst] != '.') return false;
    if (name[lst + 1 + 15] != '.') return false;
    if (memcmp(name + lst + 1 + 15 + 1, ext, lex) != 0) return false;
    const char *ts = name + lst + 1;
    /* ts must be 8 digits, '-', 6 digits */
    for (int i = 0; i < 15; i++) {
        char c = ts[i];
        if (i == 8) { if (c != '-') return false; }
        else        { if (c < '0' || c > '9') return false; }
    }
    memcpy(out_ts, ts, 15);
    out_ts[15] = 0;
    return true;
}

#define PRUNE_MAX  64   /* per-dir cap; if exceeded, oldest still get removed */

void prune_timestamped_backups(const char *in_path, const char *ext,
                               const char *backup_dir, int keep)
{
    if (keep < 0) keep = 0;

    /* Derive stem = basename(in_path) without extension. */
    const char *slash = strrchr(in_path, '/');
    const char *base  = slash ? slash + 1 : in_path;
    char stem[128];
    snprintf(stem, sizeof stem, "%s", base);
    char *dot = strrchr(stem, '.');
    if (dot) *dot = 0;

    DIR *d = opendir(backup_dir);
    if (!d) return;

    /* Collect matching entries. */
    typedef struct { char name[160]; char ts[16]; } entry_t;
    static entry_t entries[PRUNE_MAX];
    int n = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && n < PRUNE_MAX) {
        char ts[16];
        if (!parse_ts_name(ent->d_name, stem, ext, ts)) continue;
        snprintf(entries[n].name, sizeof entries[n].name, "%s", ent->d_name);
        memcpy(entries[n].ts, ts, 16);
        n++;
    }
    closedir(d);
    if (n <= keep) return;

    /* Sort by ts ascending (insertion sort — n is small). */
    for (int i = 1; i < n; i++) {
        entry_t cur = entries[i];
        int j = i - 1;
        while (j >= 0 && strcmp(entries[j].ts, cur.ts) > 0) {
            entries[j + 1] = entries[j];
            j--;
        }
        entries[j + 1] = cur;
    }

    /* Delete the oldest n - keep entries. */
    int to_delete = n - keep;
    for (int i = 0; i < to_delete; i++) {
        char path[256];
        snprintf(path, sizeof path, "%s/%s", backup_dir, entries[i].name);
        unlink(path);
    }
}
