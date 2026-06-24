#include "nds_picker.h"
#include "applog.h"
#include "game_profile.h"
#include "ui.h"

#include <3ds.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

/* Roots scanned for HGSS dumps. Each root is walked recursively with a
 * small depth budget so we don't traverse the entire SD. */
static const char *const ROOTS[] = {
    "sdmc:/3ds/pokeStride/data",
    "sdmc:/3ds/pokeStride",
    "sdmc:/3ds/pkwbridge",
    "sdmc:/roms/nds",
    "sdmc:/roms",
    "sdmc:/games/nds",
    "sdmc:/games",
    "sdmc:/Pokemon",
    "sdmc:",
};

/* Dirs we never recurse into — they are huge / not where ROMs live. */
static bool skip_dirname(const char *name) {
    if (name[0] == '.') return true;
    if (!strcmp(name, "Nintendo 3DS")) return true;
    if (!strcmp(name, "$Recycle.Bin")) return true;
    if (!strcmp(name, "System Volume Information")) return true;
    if (!strcmp(name, "cias")) return true;
    if (!strcmp(name, "luma")) return true;
    if (!strcmp(name, "boot9strap")) return true;
    return false;
}

static bool ends_nds(const char *n) {
    size_t l = strlen(n);
    if (l < 5) return false;
    const char *s = n + l - 4;
    if (s[0] != '.') return false;
    char c1 = s[1] | 0x20, c2 = s[2] | 0x20, c3 = s[3] | 0x20;
    return c1 == 'n' && c2 == 'd' && c3 == 's';
}

static bool peek_gamecode(const char *full_path, char out[4]) {
    FILE *fp = fopen(full_path, "rb");
    if (!fp) return false;
    if (fseek(fp, 0x0C, SEEK_SET) != 0) { fclose(fp); return false; }
    size_t got = fread(out, 1, 4, fp);
    fclose(fp);
    return got == 4;
}

/* Human-readable label for an entry, e.g. "SoulSilver (ES)". Falls back
 * to the raw game code when the profile doesn't provide a regional
 * dictionary (shouldn't normally happen). */
static const char *entry_label(const nds_entry_t *e) {
    static char buf[40];
    if (e->profile && e->profile->region_label) {
        return e->profile->region_label(e->gamecode);
    }
    snprintf(buf, sizeof buf, "%s", e->gamecode);
    return buf;
}

static void scan_dir(const char *dir, int depth, nds_list_t *l) {
    if (l->count >= NDS_PICKER_MAX) return;
    if (depth < 0) return;
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && l->count < NDS_PICKER_MAX) {
        if (skip_dirname(ent->d_name)) continue;
        char full[NDS_PICKER_DIRLEN + NDS_PICKER_NAMELEN + 4];
        snprintf(full, sizeof(full), "%s/%s", dir, ent->d_name);
        struct stat st;
        if (stat(full, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            scan_dir(full, depth - 1, l);
            continue;
        }
        if (!S_ISREG(st.st_mode)) continue;
        if (!ends_nds(ent->d_name)) continue;
        /* Skip implausibly small dumps so we don't peek random files. */
        if (st.st_size < 0x1000) continue;
        char gc[4];
        if (!peek_gamecode(full, gc)) continue;
        const game_profile_t *prof = game_profile_for_code(gc);
        if (!prof) continue;
        /* De-duplicate by full path. */
        bool dup = false;
        for (int i = 0; i < l->count && !dup; i++) {
            if (strcmp(l->entries[i].dir, dir) == 0 &&
                strcmp(l->entries[i].name, ent->d_name) == 0) {
                dup = true;
            }
        }
        if (dup) continue;
        nds_entry_t *e = &l->entries[l->count++];
        snprintf(e->dir,  sizeof(e->dir),  "%s", dir);
        snprintf(e->name, sizeof(e->name), "%s", ent->d_name);
        memcpy(e->gamecode, gc, 4);
        e->gamecode[4] = 0;
        e->size = st.st_size;
        e->profile = prof;
        pkw_log("nds_picker: found %s  (%s, %s)\n",
                full, e->gamecode, prof->display_name);
    }
    closedir(d);
}

void nds_picker_scan(nds_list_t *l) {
    memset(l, 0, sizeof(*l));
    pkw_log("nds_picker: scanning SD...\n");
    /* Recursive on /roms (HGSS lives in subdirs), shallow on the rest. */
    for (size_t i = 0; i < sizeof(ROOTS) / sizeof(ROOTS[0]); i++) {
        int depth;
        if      (strstr(ROOTS[i], "roms"))  depth = 3;
        else if (strstr(ROOTS[i], "games")) depth = 3;
        else if (strstr(ROOTS[i], "Pokemon")) depth = 3;
        else if (!strcmp(ROOTS[i], "sdmc:")) depth = 1;
        else                                 depth = 2;
        scan_dir(ROOTS[i], depth, l);
        if (l->count >= NDS_PICKER_MAX) break;
    }
    pkw_log("nds_picker: %d candidate(s)\n", l->count);
}

/* ===== Picker UI ===== */

#define PICKER_VIS_ROWS  6

static void draw_empty(void) {
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & (KEY_B | KEY_A | KEY_START)) return;
        frame_begin();
        draw_title("No supported NDS dump found");
        float y = 36;
        draw_text(MARGIN_X, y, 0.5f, COL_ERR,
                  "PkwBridge scanned the SD but did not find"); y += LINE_H;
        draw_text(MARGIN_X, y, 0.5f, COL_ERR,
                  "any recognised Pokemon NDS dump."); y += LINE_H + 4;
        draw_text(MARGIN_X, y, 0.45f, COL_TEXT,
                  "Place a .nds in one of these dirs:"); y += LINE_H + 2;
        const char *dirs[] = {
            "  sdmc:/roms/nds/",
            "  sdmc:/3ds/pokeStride/data/",
            "  sdmc:/games/nds/",
            "  sdmc:/Pokemon/",
        };
        for (size_t i = 0; i < sizeof(dirs)/sizeof(*dirs); i++) {
            draw_text(MARGIN_X, y, 0.45f, COL_DIM, dirs[i]);
            y += LINE_H;
        }
        y += 4;
        draw_text(MARGIN_X, y, 0.42f, COL_DIM,
                  "PkwBridge still runs without it — visual"); y += LINE_H;
        draw_text(MARGIN_X, y, 0.42f, COL_DIM,
                  "updates on the walker will be skipped.");
        draw_footer("(B) skip and continue");

        /* Bottom: list every registered game profile so adding a new
         * one (e.g. PROFILE_BW2) automatically updates the help text. */
        scene_bottom();
        draw_title_bot("Supported games");
        float by = 36;
        draw_text(MARGIN_X, by, 0.45f, COL_TEXT,
                  "Any dump from a copy you own of:"); by += LINE_H + 4;
        for (int i = 0; ; i++) {
            const game_profile_t *p = game_profile_at(i);
            if (!p) break;
            draw_textf(MARGIN_X, by, 0.42f, COL_HIFG,
                       " - %s", p->display_name);
            by += LINE_H;
        }
        by += 6;
        draw_text(MARGIN_X, by, 0.42f, COL_DIM,
                  "Read once to extract sprites + names"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_DIM,
                  "to local .bin cache. Subsequent boots"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_DIM,
                  "skip the extraction step.");
        draw_footer_bot("(B) continue without it");
        frame_end();
    }
}

static void draw_scanning_frame(void) {
    frame_begin();
    draw_title("Looking for NDS dump...");
    float y = 60;
    draw_text(MARGIN_X, y, 0.5f, COL_HIFG,
              "Scanning SD card for .nds files..."); y += LINE_H + 4;
    draw_text(MARGIN_X, y, 0.45f, COL_TEXT,
              "This usually takes 1-3 seconds.");
    scene_bottom();
    draw_title_bot("Game assets missing");
    float by = 36;
    draw_text(MARGIN_X, by, 0.45f, COL_TEXT,
              "Asset .bin files were not found in:"); by += LINE_H_TIGHT;
    draw_text(MARGIN_X + 4, by, 0.42f, COL_DIM,
              "  sdmc:/3ds/pokeStride/data/"); by += LINE_H + 6;
    draw_text(MARGIN_X, by, 0.45f, COL_TEXT,
              "PkwBridge will scan the SD for a"); by += LINE_H_TIGHT;
    draw_text(MARGIN_X, by, 0.45f, COL_TEXT,
              "supported Pokemon NDS and extract once.");
    frame_end();
}

bool nds_picker_pick(char *path_out, size_t cap) {
    /* Show one frame so the splash from show_welcome doesn't freeze
     * while we walk the filesystem. */
    draw_scanning_frame();
    static nds_list_t list;
    nds_picker_scan(&list);
    return nds_picker_run(&list, path_out, cap);
}

bool nds_picker_run(nds_list_t *l, char *path_out, size_t cap) {
    if (l->count == 0) { draw_empty(); return false; }

    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & (KEY_B | KEY_START)) return false;
        if (d & KEY_DUP)
            l->cursor = (l->cursor - 1 + l->count) % l->count;
        if (d & KEY_DDOWN)
            l->cursor = (l->cursor + 1) % l->count;
        if (d & KEY_A) {
            snprintf(path_out, cap, "%s/%s",
                     l->entries[l->cursor].dir,
                     l->entries[l->cursor].name);
            return true;
        }
        if (l->cursor < l->scroll) l->scroll = l->cursor;
        if (l->cursor >= l->scroll + PICKER_VIS_ROWS)
            l->scroll = l->cursor - PICKER_VIS_ROWS + 1;

        frame_begin();
        draw_title("Pick a Pokemon NDS dump");
        float y = 28;
        draw_textf(MARGIN_X, y, 0.42f, COL_DIM,
                   "%d candidate(s) found on SD", l->count);
        y += LINE_H + 6;
        int end = l->scroll + PICKER_VIS_ROWS;
        if (end > l->count) end = l->count;
        for (int i = l->scroll; i < end; i++) {
            const nds_entry_t *e = &l->entries[i];
            if (i == l->cursor) {
                draw_frame(0, y - 2, TOP_W, LINE_H * 2 + 2, 2, COL_HILITE);
                draw_text(MARGIN_X + 4, y, 0.45f, COL_HIFG, e->name);
                draw_textf(MARGIN_X + 16, y + LINE_H, 0.4f, COL_HIFG,
                           "%s", entry_label(e));
            } else {
                draw_text(MARGIN_X + 4, y, 0.45f, COL_TEXT, e->name);
                draw_textf(MARGIN_X + 16, y + LINE_H, 0.4f, COL_DIM,
                           "%s", entry_label(e));
            }
            y += LINE_H * 2 + 2;
        }
        draw_footer("(A) use this dump   (B) skip");

        /* Bottom: detail panel for the currently focused entry. */
        scene_bottom();
        const nds_entry_t *cur = &l->entries[l->cursor];
        draw_title_bot("Dump details");
        float by = 30;
        draw_text(MARGIN_X, by, 0.45f, COL_HIFG, cur->name); by += LINE_H + 2;
        draw_textf(MARGIN_X, by, 0.42f, COL_HILITE, "%s",
                   entry_label(cur)); by += LINE_H;
        draw_textf(MARGIN_X, by, 0.4f, COL_DIM,
                   "code: %s    size: %lld MB",
                   cur->gamecode, (long long)(cur->size / (1024 * 1024)));
        by += LINE_H + 6;
        draw_text(MARGIN_X, by, 0.4f, COL_DIM, "Path:"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X + 4, by, 0.38f, COL_TEXT, cur->dir); by += LINE_H_TIGHT;
        by += 4;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "After picking, PkwBridge will read"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "this NDS once and produce ~1.2 MB"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "of cached visual data in"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.4f, COL_DIM,
                  "  sdmc:/3ds/pokeStride/data/");
        draw_footer_bot("(D-Pad) move   (A) select   (B) skip");
        frame_end();
    }
    return false;
}
