/*
 * backup_ui.c - Manage backups menu.
 *
 * Backups live under /3ds/pkwbridge/backups/{pweep,sav}/. The user
 * picks pweep or sav, browses files (.bak + timestamped), and either
 * RESTORES (copies the chosen file back to its canonical location in
 * /3ds/pokeStride or /roms/nds/saves) or DELETES.
 */
#include "backup_ui.h"

#include <3ds.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "app_paths.h"
#include "applog.h"
#include "file_list.h"
#include "io_util.h"
#include "ui.h"

#include "hgss_sav.h"
#include "pweep.h"

/* Where does a restored file go?  pweep backups go back to the
 * pokeStride dir; sav backups to roms/nds/saves. The original file
 * name (canonical) is recovered by stripping the .bak suffix or the
 * timestamp infix. */
typedef struct {
    const char *backup_dir;
    const char *target_dir;
    off_t       size_filter;
    const char *title;
} backup_kind_t;

static const backup_kind_t KIND_PWEEP = {
    BACKUP_PWEEP_DIR, PWEEP_DIR_PRIMARY, PWEEP_SIZE,    "Backups - pweep"
};
static const backup_kind_t KIND_SAV = {
    BACKUP_SAV_DIR,   SAV_DIR_PRIMARY,   HGSS_SAV_SIZE, "Backups - sav"
};

typedef struct { bool ok; const char *src; const char *canon; } restore_ctx_t;
static void draw_restore_result(void *c) {
    restore_ctx_t *x = c;
    if (x->ok) {
        draw_text(MARGIN_X, 40, 0.5f, COL_OK, "Restored");
        draw_textf(MARGIN_X, 64, 0.42f, COL_TEXT, "%s", x->src);
        draw_textf(MARGIN_X, 80, 0.42f, COL_DIM, "  -> %s", x->canon);
    } else {
        draw_text(MARGIN_X, 40, 0.5f, COL_ERR, "Restore FAILED");
    }
}

typedef struct { int rc; const char *name; } delete_ctx_t;
static void draw_delete_result(void *c) {
    delete_ctx_t *x = c;
    if (x->rc == 0) {
        draw_text(MARGIN_X, 40, 0.5f, COL_OK, "Deleted.");
        draw_textf(MARGIN_X, 64, 0.42f, COL_TEXT, "%s", x->name);
    } else {
        draw_text(MARGIN_X, 40, 0.5f, COL_ERR, "Delete FAILED.");
    }
}

typedef struct { const char *d; } missing_ctx_t;
static void draw_missing(void *c) {
    missing_ctx_t *x = c;
    draw_text(MARGIN_X, 60, 0.5f, COL_DIM, "Empty backup folder:");
    draw_textf(MARGIN_X + 14, 80, 0.42f, COL_DIM, "%s", x->d);
    draw_text(MARGIN_X, 110, 0.42f, COL_DIM,
              "Backups appear here automatically");
    draw_text(MARGIN_X, 126, 0.42f, COL_DIM,
              "after a Send or Claim+Return.");
}

static bool file_actions(const backup_kind_t *kind,
                          const file_list_t *fl, int idx)
{
    char src_path[MAX_NAME_LEN * 2 + 8];
    snprintf(src_path, sizeof src_path, "%s/%s", fl->dir, fl->names[idx]);
    char canon[MAX_NAME_LEN];
    int cls = classify_backup(fl->names[idx], canon, sizeof canon);
    char dest_path[MAX_NAME_LEN * 2 + 8];
    snprintf(dest_path, sizeof dest_path, "%s/%s", kind->target_dir, canon);
    bool can_restore = (cls != 0) && (strcmp(fl->names[idx], canon) != 0);

    menu_item_t items[] = {
        { "Restore as live file", "Copy to the active location", can_restore },
        { "Delete this backup",   "Remove permanently",            true },
        { "Back",                  NULL,                             true },
    };
    int sel = run_menu("Backup: actions", items, 3, can_restore ? 0 : 1);
    if (sel < 0 || sel == 2) return false;

    if (sel == 0) {
        char msg[200];
        snprintf(msg, sizeof msg, "to %s/%s", kind->target_dir, canon);
        if (!confirm_dialog("Restore?", fl->names[idx], msg)) return false;
        bool ok = copy_file(src_path, dest_path);
        pkw_log("backup: restore %s -> %s [%s]\n",
                src_path, dest_path, ok ? "ok" : "FAIL");
        restore_ctx_t r = { ok, fl->names[idx], dest_path };
        run_info_screen(ok ? "Restored" : "Restore failed", "(B) continue",
                        draw_restore_result, &r);
        return true;
    }
    if (sel == 1) {
        if (!confirm_dialog("Delete this backup?", fl->names[idx],
                             "This cannot be undone.")) return false;
        int rc = unlink(src_path);
        pkw_log("backup: delete %s [%s]\n", src_path, rc == 0 ? "ok" : "FAIL");
        delete_ctx_t r = { rc, fl->names[idx] };
        run_info_screen(rc == 0 ? "Deleted" : "Delete failed", "(B) continue",
                        draw_delete_result, &r);
        return true;
    }
    return false;
}

static void backup_browser(const backup_kind_t *kind) {
    ensure_dir(kind->backup_dir);
    pkw_log("backups: browser open dir=%s\n", kind->backup_dir);
    while (aptMainLoop()) {
        file_list_t fl;
        if (!scan_all_files(&fl, kind->backup_dir, kind->size_filter)) {
            pkw_log("backups: scan empty -> %s\n", kind->backup_dir);
            missing_ctx_t m = { kind->backup_dir };
            run_info_screen(kind->title, "(B) back", draw_missing, &m);
            return;
        }
        pkw_log("backups: scan found %d file(s) in %s\n", fl.count, fl.dir);
        int idx = run_backup_picker(kind->title, &fl);
        if (idx < 0) { pkw_log("backups: picker cancel (%d)\n", idx); return; }
        pkw_log("backups: selected %s\n", fl.names[idx]);
        file_actions(kind, &fl, idx);
    }
}

void backups_flow(void) {
    pkw_log("\n=== backups flow: enter ===\n");
    while (aptMainLoop()) {
        menu_item_t items[] = {
            { "pweep backups", "Walker EEPROMs: .bak + timestamped",  true },
            { "sav backups",   "HGSS .sav: .bak + timestamped",       true },
            { "Back",          NULL,                                   true },
        };
        int sel = run_menu("Manage backups", items, 3, 0);
        pkw_log("backups menu -> %d\n", sel);
        if (sel < 0 || sel == 2) { pkw_log("=== backups flow: exit ===\n"); return; }
        switch (sel) {
            case 0: backup_browser(&KIND_PWEEP); break;
            case 1: backup_browser(&KIND_SAV);   break;
        }
    }
}
