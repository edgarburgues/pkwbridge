/*
 * main.c - pkwbridge entry point.
 *
 * Boot citro2d + log, prompt for active pweep + sav, run the main menu
 * loop. The active files are global state (app_state.c) so every flow
 * uses the same pair.
 */
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "app_paths.h"
#include "app_state.h"
#include "app_version.h"
#include "applog.h"
#include "data_loader.h"
#include "io_util.h"
#include "migrate.h"
#include "ui.h"

#include "backup_ui.h"
#include "claim_ui.h"
#include "help_ui.h"
#include "inspect_ui.h"
#include "send_ui.h"

/* ----- splash + beta disclaimer ----- *
 *
 * Two-screen splash. Top: app identity + workflow. Bottom: the beta
 * warning. The user must press A to acknowledge before continuing.
 * START aborts to Home Menu. */
static bool show_welcome(void) {
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & KEY_START) return false;   /* user aborts */
        if (d & KEY_A)     return true;

        frame_begin();

        /* --- Top screen: branding + workflow --- */
        draw_title(APP_NAME);
        float y = 32;
        draw_text(MARGIN_X, y, 0.55f, COL_HIFG, APP_TAGLINE); y += LINE_H + 6;
        draw_text(MARGIN_X, y, 0.45f, COL_TEXT,
                  "A full Pokewalker cycle, no IR needed."); y += LINE_H + 8;

        draw_text(MARGIN_X, y, 0.5f, COL_HILITE, "Workflow"); y += LINE_H + 2;
        draw_text(MARGIN_X + 10, y, 0.45f, COL_TEXT,
                  "1. Pick your pweep + HGSS save."); y += LINE_H;
        draw_text(MARGIN_X + 10, y, 0.45f, COL_TEXT,
                  "2. Send a Pokemon to the walker."); y += LINE_H;
        draw_text(MARGIN_X + 10, y, 0.45f, COL_TEXT,
                  "3. Walk it on pokeStride or hardware."); y += LINE_H;
        draw_text(MARGIN_X + 10, y, 0.45f, COL_TEXT,
                  "4. Finish walk: claim + return to box."); y += LINE_H + 8;

        draw_textf(MARGIN_X, TOP_H - 38, 0.4f, COL_DIM,
                   "v%s    log: %s", APP_VERSION, LOG_PATH);
        draw_footer("(A) I understand, continue   (START) quit");

        /* --- Bottom screen: beta disclaimer --- */
        scene_bottom();
        draw_rect(0, 0, BOT_W, 22, COL_WARN);
        draw_text(78, 3, 0.55f, COL_TITLEFG, "EARLY BETA - READ FIRST");
        float by = 32;
        draw_text(MARGIN_X, by, 0.45f, COL_TEXT,
                  "PkwBridge writes directly to:"); by += LINE_H + 2;
        draw_text(MARGIN_X + 10, by, 0.45f, COL_HIFG,
                  "- your HGSS .sav file"); by += LINE_H;
        draw_text(MARGIN_X + 10, by, 0.45f, COL_HIFG,
                  "- your Pokewalker EEPROM dump"); by += LINE_H + 8;
        draw_text(MARGIN_X, by, 0.5f, COL_WARN,
                  "Use at your own risk."); by += LINE_H + 4;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "Always keep a separate backup of your"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  ".sav outside PkwBridge before each"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "session. A bug here could corrupt your"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "HGSS playthrough or walker data."); by += LINE_H_TIGHT + 4;
        draw_text(MARGIN_X, by, 0.42f, COL_DIM,
                  "Author accepts no responsibility for"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_DIM,
                  "damage to your game, save, or console.");
        draw_footer_bot("(A) continue   (START) quit");
        frame_end();
    }
    return false;
}

/* Custom main menu: shows the action list on the top screen, with a
 * detailed status + action-description panel on the bottom. Returns
 * the action index or -2 on START (exit). */
typedef enum {
    M_SEND = 0,
    M_CLAIM,           /* claim caught + return sent (combined) */
    M_CLAIM_ONLY,      /* only deposit caught + sync stats */
    M_RETURN_ONLY,     /* only return WalkerPair Pokemon */
    M_INSPECT,
    M_BACKUPS,
    M_CHANGE_FILES,
    M_HELP,
    M_EXIT,
    M_COUNT
} menu_id_t;

static const char *menu_labels[M_COUNT] = {
    "Send Pokemon to walker",
    "Finish walk (claim + return)",
    "  Only deposit caught",
    "  Only return walker Pokemon",
    "Inspect state",
    "Manage backups",
    "Change active files",
    "Help",
    "Exit",
};

/* One-line hint shown in the bottom-screen header strip. */
static const char *menu_hints[M_COUNT] = {
    "Write a Pokemon into pweep + sav.WalkerPair",
    "End-of-walk: claim caught + return walker mon",
    "Deposit caught only (leave walker mon active)",
    "Return walker mon only (leave caught list)",
    "Read-only view of pweep + sav",
    "Restore or delete .bak / timestamped files",
    "Choose pweep.rom + HGSS .sav for this session",
    "Workflow + file locations",
    "Quit to the 3DS Home Menu",
};

/* Multi-line description shown on the bottom screen. NULL-terminated. */
static const char *const desc_send[] = {
    "Write a Pokemon from your HGSS boxes",
    "into pweep.rom and the HGSS WalkerPair",
    "slot. You can then walk it on pokeStride",
    "or transfer to a real Pokewalker.",
    "",
    "Both files are updated atomically — the",
    "walker and the save stay in sync.",
    NULL,
};
static const char *const desc_claim[] = {
    "Standard end-of-walk action:",
    "",
    "  - Caught Pokemon go to HGSS boxes",
    "  - Watts + steps sync to HGSS",
    "  - Walker Pokemon returns to its box",
    "",
    "Run this after every walk to keep both",
    "files clean and consistent.",
    NULL,
};
static const char *const desc_claim_only[] = {
    "Like Finish walk, but skips the return",
    "step. Your walking Pokemon stays active",
    "on the walker.",
    "",
    "Use this if you only want to collect what",
    "was caught and keep walking with the same",
    "partner.",
    NULL,
};
static const char *const desc_return_only[] = {
    "Take your walking Pokemon back to its",
    "HGSS box without touching the caught",
    "list or watts/steps.",
    "",
    "Useful when you want to swap the partner",
    "without ending the walk session.",
    NULL,
};
static const char *const desc_inspect[] = {
    "Read-only inspector for both files.",
    "",
    "Shows the walking Pokemon, caught list,",
    "watts, steps, current route, and trainer",
    "info. Touches nothing on disk.",
    NULL,
};
static const char *const desc_backups[] = {
    "Browse, restore, or delete backups.",
    "",
    "  .bak       previous version of a file",
    "  .TS.ext    timestamped output snapshot",
    "",
    "Backups live under:",
    "  /3ds/pkwbridge/backups/",
    NULL,
};
static const char *const desc_change[] = {
    "Pick a different pweep.rom and / or HGSS",
    ".sav for this session. The picker walks",
    "common SD locations and shows only files",
    "that match the expected sizes (64 KB /",
    "512 KB).",
    NULL,
};
static const char *const desc_help[] = {
    "Quick reference: the full walker cycle,",
    "where each file lives on SD, and what",
    "PkwBridge writes vs. preserves.",
    NULL,
};
static const char *const desc_exit[] = {
    "Quit cleanly to the 3DS Home Menu.",
    "",
    "Your last picked pweep + sav are saved",
    "to last_session.cfg so the next boot",
    "skips the picker.",
    NULL,
};

static const char *const *menu_desc[M_COUNT] = {
    desc_send, desc_claim, desc_claim_only, desc_return_only,
    desc_inspect, desc_backups, desc_change, desc_help, desc_exit,
};

static void draw_lines(float x, float y, float size, u32 col,
                       const char *const *lines) {
    for (int i = 0; lines[i] != NULL; i++) {
        if (lines[i][0]) draw_text(x, y, size, col, lines[i]);
        y += LINE_H_TIGHT;
    }
}

static bool action_needs_files(int id) {
    return id == M_CLAIM || id == M_CLAIM_ONLY || id == M_RETURN_ONLY
        || id == M_SEND  || id == M_INSPECT;
}

static int run_main_menu(int initial) {
    int cursor = initial;
    if (cursor < 0 || cursor >= M_COUNT) cursor = M_SEND;
    bool pw_ok = false, sv_ok = false;
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & KEY_START) return -2;
        if (d & KEY_DUP)   cursor = (cursor - 1 + M_COUNT) % M_COUNT;
        if (d & KEY_DDOWN) cursor = (cursor + 1) % M_COUNT;
        if (d & KEY_A)     return cursor;

        pw_ok = app_state_pweep_loaded();
        sv_ok = app_state_sav_loaded();
        bool enabled = !action_needs_files(cursor) || (pw_ok && sv_ok);

        frame_begin();

        /* === TOP SCREEN: action list === */
        draw_title("PkwBridge - main menu");
        float y = CONTENT_Y_TOP + 6;
        for (int i = 0; i < M_COUNT; i++) {
            bool i_enabled = !action_needs_files(i) || (pw_ok && sv_ok);
            if (i == cursor) {
                draw_frame(0, y - 2, TOP_W, LINE_H + 2, 2, COL_HILITE);
                draw_text(MARGIN_X + 8,  y, 0.5f, COL_HIFG, ">");
                draw_text(MARGIN_X + 26, y, 0.5f, COL_HIFG, menu_labels[i]);
                if (!i_enabled) {
                    draw_text(TOP_W - 92, y, 0.4f, COL_WARN,
                              "[need files]");
                }
            } else {
                u32 col = i_enabled ? COL_TEXT : COL_DIM;
                draw_text(MARGIN_X + 26, y, 0.5f, col, menu_labels[i]);
            }
            y += LINE_H + 2;
        }
        draw_footer("(D-Pad) move   (A) select   (START) exit");

        /* === BOTTOM SCREEN: status + action panel === */
        scene_bottom();
        draw_title_bot("Status & details");

        /* Active files block */
        float by = 30;
        draw_text(MARGIN_X, by, 0.45f, COL_HILITE, "ACTIVE FILES"); by += LINE_H + 2;
        draw_textf(MARGIN_X + 4, by, 0.42f,
                   pw_ok ? COL_OK : COL_ERR,
                   "pweep  %s", app_state_pweep_basename());
        by += LINE_H_TIGHT;
        draw_textf(MARGIN_X + 4, by, 0.42f,
                   sv_ok ? COL_OK : COL_ERR,
                   "save   %s", app_state_sav_basename());
        by += LINE_H_TIGHT + 6;
        draw_rect(MARGIN_X, by, BOT_W - 2 * MARGIN_X, 1, COL_DIM); by += 6;

        /* Focused action header */
        draw_text(MARGIN_X, by, 0.45f, COL_HILITE, "ACTION"); by += LINE_H + 2;
        draw_text(MARGIN_X + 4, by, 0.45f,
                  enabled ? COL_HIFG : COL_DIM, menu_labels[cursor]);
        by += LINE_H + 2;
        if (!enabled) {
            draw_text(MARGIN_X + 4, by, 0.4f, COL_WARN,
                      "Pick files first."); by += LINE_H_TIGHT;
        }
        by += 2;
        draw_lines(MARGIN_X + 4, by, 0.4f, COL_TEXT, menu_desc[cursor]);

        draw_footer_bot(menu_hints[cursor]);
        frame_end();
    }
    return -2;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    if (!ui_init()) return 1;

    mkdir("sdmc:/3ds", 0777);
    ensure_dir(APP_DIR);
    ensure_dir(BACKUP_DIR);
    ensure_dir(BACKUP_PWEEP_DIR);
    ensure_dir(BACKUP_SAV_DIR);
    applog_open(LOG_PATH);
    pkw_log("=== %s v%s boot ===\n", APP_NAME, APP_VERSION);
    pkw_log("paths: pweep=%s sav=%s app=%s\n",
            PWEEP_DIR_PRIMARY, SAV_DIR_PRIMARY, APP_DIR);
    pkw_log("backup dirs: %s, %s\n", BACKUP_PWEEP_DIR, BACKUP_SAV_DIR);

    /* One-shot: move any .bak / timestamped files left over by older
     * builds into the new backup layout. */
    migrate_legacy_backups();

    /* Show splash + beta disclaimer first. If the user aborts here we
     * skip everything else and exit cleanly. */
    if (!show_welcome()) {
        pkw_log("welcome: user aborted (START)\n");
        applog_close();
        ui_fini();
        return 0;
    }
    pkw_log("welcome accepted\n");

    /* Load asset blobs. If the .bin cache is empty this will prompt
     * the user to pick an HGSS .nds from anywhere on the SD, then
     * extract once. Missing data falls back to degraded mode. */
    app_data_init();

    /* Try to auto-load files from previous session. If that fails (first
     * boot or files moved), drop into the picker. */
    if (!app_state_load_session()) {
        pkw_log("no usable session config -> prompting picker\n");
        app_state_change_files_flow();
    } else {
        pkw_log("session loaded from last_session.cfg\n");
    }

    int last_sel = M_CLAIM;
    for (;;) {
        int sel = run_main_menu(last_sel);
        pkw_log("main menu -> %d (%s)\n", sel,
                sel == -2 ? "START/exit"
                          : (sel >= 0 && sel < M_COUNT) ? menu_labels[sel] : "?");
        if (sel == -2 || sel == M_EXIT) break;
        last_sel = sel;
        switch (sel) {
            case M_CHANGE_FILES: app_state_change_files_flow(); break;
            case M_CLAIM:
                if (!app_state_ensure_loaded()) break;
                claim_flow();
                break;
            case M_CLAIM_ONLY:
                if (!app_state_ensure_loaded()) break;
                claim_caught_flow();
                break;
            case M_RETURN_ONLY:
                if (!app_state_ensure_loaded()) break;
                return_pokemon_flow();
                break;
            case M_SEND:
                if (!app_state_ensure_loaded()) break;
                send_flow();
                break;
            case M_INSPECT:
                if (!app_state_ensure_loaded()) break;
                inspect_flow();
                break;
            case M_BACKUPS:
                backups_flow();
                app_state_reload();  /* canonical may have changed */
                break;
            case M_HELP: show_help(); break;
            default: break;
        }
        pkw_log("main menu: returned from %s\n",
                (sel >= 0 && sel < M_COUNT) ? menu_labels[sel] : "?");
    }

    pkw_log("=== exit ===\n");
    applog_close();
    ui_fini();
    return 0;
}
