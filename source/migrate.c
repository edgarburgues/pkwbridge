#include "migrate.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "app_paths.h"
#include "applog.h"
#include "file_list.h"
#include "io_util.h"

static int migrate_dir(const char *src_dir, const char *dest_dir,
                       const char *ext)
{
    if (!ensure_dir(dest_dir)) {
        pkw_log("migrate: ensure_dir failed for %s\n", dest_dir);
        return 0;
    }
    DIR *d = opendir(src_dir);
    if (!d) {
        pkw_log("migrate: opendir %s failed\n", src_dir);
        return 0;
    }
    int moved = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        size_t n = strlen(ent->d_name);
        /* match either "<base>.bak" or "<base>.<ts>.<ext>" */
        bool is_bak = (n > 4 && !strcmp(ent->d_name + n - 4, ".bak"));
        char canon[MAX_NAME_LEN];
        int cls = classify_backup(ent->d_name, canon, sizeof canon);
        bool match_ext = (n > 4 && !strcasecmp(ent->d_name + n - 4, ext));
        bool is_timestamped = (cls == 2 && match_ext);
        if (!is_bak && !is_timestamped) continue;

        char src[MAX_NAME_LEN * 2 + 8];
        char dst[MAX_NAME_LEN * 2 + 8];
        snprintf(src, sizeof src, "%s/%s", src_dir,  ent->d_name);
        snprintf(dst, sizeof dst, "%s/%s", dest_dir, ent->d_name);
        struct stat st;
        if (stat(src, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        if (move_file(src, dst)) {
            pkw_log("migrate: %s -> %s\n", src, dst);
            moved++;
        } else {
            pkw_log("migrate: FAILED %s -> %s\n", src, dst);
        }
    }
    closedir(d);
    return moved;
}

void migrate_legacy_backups(void) {
    pkw_log("\n=== migrate_legacy_backups: start ===\n");
    int n_pw = migrate_dir(PWEEP_DIR_PRIMARY, BACKUP_PWEEP_DIR, ".rom");
    int n_sv = migrate_dir(SAV_DIR_PRIMARY,   BACKUP_SAV_DIR,   ".sav");
    pkw_log("migrate: moved %d pweep + %d sav file(s)\n", n_pw, n_sv);
    pkw_log("=== migrate_legacy_backups: end ===\n");
}
