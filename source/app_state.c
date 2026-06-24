#include "app_state.h"

#include <stdio.h>
#include <string.h>

#include "app_paths.h"
#include "applog.h"
#include "file_list.h"

#define APP_PATH_LEN 192

static char       g_pweep_path[APP_PATH_LEN];
static char       g_sav_path  [APP_PATH_LEN];
static pweep_t    g_pweep;
static hgss_sav_t g_sav;
static bool       g_pweep_loaded = false;
static bool       g_sav_loaded   = false;

bool app_state_pweep_loaded(void) { return g_pweep_loaded; }
bool app_state_sav_loaded(void)   { return g_sav_loaded;   }
bool app_state_both_loaded(void)  { return g_pweep_loaded && g_sav_loaded; }
pweep_t    *app_state_pweep(void) { return &g_pweep; }
hgss_sav_t *app_state_sav(void)   { return &g_sav;   }
const char *app_state_pweep_path(void) { return g_pweep_path; }
const char *app_state_sav_path(void)   { return g_sav_path;   }

const char *app_state_pweep_basename(void) {
    if (!g_pweep_loaded) return "(none)";
    const char *s = strrchr(g_pweep_path, '/');
    return s ? s + 1 : g_pweep_path;
}
const char *app_state_sav_basename(void) {
    if (!g_sav_loaded) return "(none)";
    const char *s = strrchr(g_sav_path, '/');
    return s ? s + 1 : g_sav_path;
}

bool app_state_set_pweep(const char *path) {
    snprintf(g_pweep_path, sizeof g_pweep_path, "%s", path);
    g_pweep_loaded = pweep_load(&g_pweep, path);
    if (g_pweep_loaded) pweep_validate(&g_pweep);
    pkw_log("app_state: pweep <- %s [%s]\n",
            path, g_pweep_loaded ? "ok" : "FAIL");
    if (g_pweep_loaded) {
        pkw_log("  pairing=%d health=%d sentinel=%d ident=%d lcd=%d\n",
                g_pweep.pairing, g_pweep.health, g_pweep.walk_sentinel,
                g_pweep.walk_identity, g_pweep.lcd_init);
        pkw_log("  has_pokemon=%d watts=%u steps=%lu days=%u caught=%d\n",
                pweep_has_pokemon(&g_pweep), pweep_get_watts(&g_pweep),
                (unsigned long)pweep_get_total_steps(&g_pweep),
                pweep_get_total_days(&g_pweep),
                pweep_caught_count(&g_pweep));
    }
    if (g_pweep_loaded) app_state_save_session();
    return g_pweep_loaded;
}

bool app_state_set_sav(const char *path) {
    snprintf(g_sav_path, sizeof g_sav_path, "%s", path);
    g_sav_loaded = hgss_load(&g_sav, path);
    pkw_log("app_state: sav <- %s [%s]\n",
            path, g_sav_loaded ? "ok" : "FAIL");
    if (g_sav_loaded) {
        pkw_log("  general_crc=%d storage_crc=%d tid=%u sid=%u\n",
                g_sav.general_status, g_sav.storage_status,
                hgss_tid(&g_sav), hgss_sid(&g_sav));
        pkw_log("  ot_gender=%u ot_lang=%u walker_steps=%lu watts=%lu\n",
                hgss_ot_gender(&g_sav), hgss_ot_language(&g_sav),
                (unsigned long)hgss_walker_steps(&g_sav),
                (unsigned long)hgss_walker_watts(&g_sav));
    }
    if (g_sav_loaded) app_state_save_session();
    return g_sav_loaded;
}

void app_state_reload(void) {
    if (g_pweep_loaded) app_state_set_pweep(g_pweep_path);
    if (g_sav_loaded)   app_state_set_sav  (g_sav_path);
}

void app_state_save_session(void) {
    FILE *f = fopen(SESSION_PATH, "w");
    if (!f) { pkw_log("app_state: save_session FAILED to open %s\n",
                       SESSION_PATH); return; }
    fprintf(f, "pweep=%s\n", g_pweep_loaded ? g_pweep_path : "");
    fprintf(f, "sav=%s\n",   g_sav_loaded   ? g_sav_path   : "");
    fclose(f);
    pkw_log("app_state: session saved (pweep=%s sav=%s)\n",
            g_pweep_loaded ? "ok" : "none",
            g_sav_loaded   ? "ok" : "none");
}

/* Forward decls. */
static bool pick_canonical(const char *title,
                            const char *primary, const char *ext,
                            off_t expected_size,
                            const char *preferred,
                            char *out_path, size_t out_cap);

bool app_state_load_session(void) {
    FILE *f = fopen(SESSION_PATH, "r");
    if (!f) {
        pkw_log("app_state: no session config at %s\n", SESSION_PATH);
        return false;
    }
    char last_pweep[APP_PATH_LEN] = {0};
    char last_sav  [APP_PATH_LEN] = {0};
    char line[300];
    while (fgets(line, sizeof line, f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        char *val = eq + 1;
        size_t n = strlen(val);
        while (n && (val[n-1] == '\n' || val[n-1] == '\r')) val[--n] = 0;
        if (!strcmp(line, "pweep")) snprintf(last_pweep, sizeof last_pweep, "%s", val);
        if (!strcmp(line, "sav"))   snprintf(last_sav,   sizeof last_sav,   "%s", val);
    }
    fclose(f);
    pkw_log("app_state: session config: pweep=%s sav=%s\n",
            last_pweep[0] ? last_pweep : "(empty)",
            last_sav[0]   ? last_sav   : "(empty)");
    if (last_pweep[0]) app_state_set_pweep(last_pweep);
    if (last_sav[0])   app_state_set_sav  (last_sav);
    return app_state_both_loaded();
}

/* Picker that:
 *   - lists only canonical files (no .bak, no timestamped)
 *   - pre-positions cursor on `preferred` path if present
 *   - returns true if user picked AND set succeeded */
static bool pick_canonical(const char *title,
                            const char *primary, const char *ext,
                            off_t expected_size,
                            const char *preferred,
                            char *out_path, size_t out_cap)
{
    file_list_t fl;
    const char *fb[] = { APP_DIR, SDMC_ROOT };
    if (!build_canonical_list(&fl, primary, fb, 2, ext, expected_size)) {
        pkw_log("app_state: no canonical files in %s\n", primary);
        return false;
    }
    int pref = -1;
    if (preferred && *preferred)
        pref = file_list_find(&fl, preferred);
    if (pref >= 0) fl.cursor = pref;
    int idx = run_file_picker(title, &fl);
    if (idx < 0) return false;
    file_full_path(&fl, idx, out_path, out_cap);
    return true;
}

bool app_state_pick_pweep(void) {
    char p[APP_PATH_LEN];
    if (!pick_canonical("Pick active pweep.rom (64 KB)",
                         PWEEP_DIR_PRIMARY, ".rom", PWEEP_SIZE,
                         g_pweep_loaded ? g_pweep_path : NULL,
                         p, sizeof p)) {
        pkw_log("app_state: pweep picker cancel/empty\n");
        return false;
    }
    return app_state_set_pweep(p);
}

bool app_state_pick_sav(void) {
    char p[APP_PATH_LEN];
    if (!pick_canonical("Pick active HGSS .sav (512 KB)",
                         SAV_DIR_PRIMARY, ".sav", HGSS_SAV_SIZE,
                         g_sav_loaded ? g_sav_path : NULL,
                         p, sizeof p)) {
        pkw_log("app_state: sav picker cancel/empty\n");
        return false;
    }
    return app_state_set_sav(p);
}

void app_state_change_files_flow(void) {
    pkw_log("\n=== change active files: enter ===\n");
    app_state_pick_pweep();
    app_state_pick_sav();
    pkw_log("=== change active files: exit (pweep=%s sav=%s) ===\n",
            g_pweep_loaded ? "ok" : "none",
            g_sav_loaded   ? "ok" : "none");
}

bool app_state_ensure_loaded(void) {
    if (app_state_both_loaded()) return true;
    pkw_log("app_state: missing files -> prompting picker\n");
    app_state_change_files_flow();
    return app_state_both_loaded();
}
