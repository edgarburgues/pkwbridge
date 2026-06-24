#include "data_loader.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "applog.h"
#include "course_bitmaps.h"
#include "font_sys.h"
#include "names.h"
#include "nds_extract.h"
#include "nds_picker.h"
#include "personal_gen4.h"
#include "phcgra_data.h"
#include "phcicon_data.h"
#include "pokeicon.h"
#include "ui.h"

#define DATA_DIR  "sdmc:/3ds/pokeStride/data"

static bool g_font_loaded           = false;
static bool g_course_bitmaps_loaded = false;
static bool g_phcgra_loaded         = false;
static bool g_phcicon_loaded        = false;

bool app_data_font_loaded(void)           { return g_font_loaded; }
bool app_data_course_bitmaps_loaded(void) { return g_course_bitmaps_loaded; }
bool app_data_phcgra_loaded(void)         { return g_phcgra_loaded; }
bool app_data_phcicon_loaded(void)        { return g_phcicon_loaded; }

static bool load_blob(const char *path, void *dst, size_t expected) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        pkw_log("data_loader: NOT FOUND %s\n", path);
        return false;
    }
    size_t got = fread(dst, 1, expected, f);
    fclose(f);
    if (got != expected) {
        pkw_log("data_loader: SHORT READ %s (got %zu / want %zu)\n",
                path, got, expected);
        return false;
    }
    pkw_log("data_loader: loaded %s (%zu B)\n", path, got);
    return true;
}

/* Cheap presence check (does not load) — used to decide whether NDS
 * extraction needs to run for pokeicon, since pokeicon_init() is heavy
 * (decode + GPU upload) and we defer it until after extraction. */
static bool file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

static bool pokeicon_files_present(void) {
    if (!file_exists(DATA_DIR "/pokeicon_glyphs.bin") ||
        !file_exists(DATA_DIR "/pokeicon_palettes.bin") ||
        !file_exists(DATA_DIR "/pokeicon_pal_index.bin")) {
        return false;
    }
    /* Sanity-check pokeicon_pal_index.bin against three species whose
     * palette is unambiguous: Bulbasaur (1) = pal 1, Rattata (19) = 2,
     * Pikachu (25) = 2. A stale cache fails this and triggers
     * re-extraction. */
    FILE *f = fopen(DATA_DIR "/pokeicon_pal_index.bin", "rb");
    if (!f) return false;
    uint8_t probe[26];
    size_t got = fread(probe, 1, sizeof probe, f);
    fclose(f);
    if (got != sizeof probe) return false;
    if (probe[1] != 1 || probe[19] != 2 || probe[25] != 2) {
        pkw_log("data_loader: stale pokeicon table "
                "(b[1]=%u b[19]=%u b[25]=%u) — will re-extract\n",
                probe[1], probe[19], probe[25]);
        return false;
    }
    return true;
}

/* Names tables are simple presence + magic-prefix check. */
static bool names_files_present(void) {
    static const char *const PATHS[] = {
        DATA_DIR "/names_items.bin",
        DATA_DIR "/names_moves.bin",
        DATA_DIR "/names_locations.bin",
    };
    for (size_t i = 0; i < sizeof PATHS / sizeof PATHS[0]; i++) {
        FILE *f = fopen(PATHS[i], "rb");
        if (!f) return false;
        uint8_t magic[4];
        size_t got = fread(magic, 1, 4, f);
        fclose(f);
        if (got != 4) return false;
        /* 'NMSG' little-endian. */
        if (magic[0] != 'N' || magic[1] != 'M' ||
            magic[2] != 'S' || magic[3] != 'G') {
            return false;
        }
    }
    return true;
}

/* personal.bin presence + magic check. A missing/old cache forces NDS
 * re-extraction. */
static bool personal_files_present(void) {
    FILE *f = fopen(DATA_DIR "/personal.bin", "rb");
    if (!f) return false;
    uint8_t magic[4];
    size_t got = fread(magic, 1, 4, f);
    fclose(f);
    return got == 4 && magic[0]=='P' && magic[1]=='R' &&
           magic[2]=='S' && magic[3]=='1';
}

static bool try_load_all(void) {
    g_course_bitmaps_loaded = load_blob(
        DATA_DIR "/course_bitmaps.bin",
        (void *)course_bitmaps,
        (size_t)COURSE_BITMAP_COUNT * COURSE_BITMAP_SIZE);

    bool fg = load_blob(
        DATA_DIR "/font_sys_glyphs.bin",
        (void *)font_sys_glyphs,
        (size_t)FONT_SYS_NUM_GLYPHS * FONT_SYS_BYTES_PER_GLYPH);
    bool fw = load_blob(
        DATA_DIR "/font_sys_widths.bin",
        (void *)font_sys_widths,
        (size_t)FONT_SYS_NUM_GLYPHS);
    g_font_loaded = fg && fw;

    g_phcgra_loaded = load_blob(
        DATA_DIR "/phcgra_data.bin",
        (void *)phcgra_data,
        (size_t)PHCGRA_COUNT * PHCGRA_ENTRY_SIZE);
    g_phcicon_loaded = load_blob(
        DATA_DIR "/phcicon_data.bin",
        (void *)phcicon_data,
        (size_t)PHCICON_COUNT * PHCICON_ENTRY_SIZE);

    return g_font_loaded && g_course_bitmaps_loaded &&
           g_phcgra_loaded && g_phcicon_loaded &&
           pokeicon_files_present() &&
           names_files_present() &&
           personal_files_present();
}

void app_data_init(void) {
    pkw_log("=== data_loader: start ===\n");

    if (!try_load_all()) {
        pkw_log("data_loader: missing .bin files, trying NDS extraction\n");
        bool extracted = nds_extract_try();
        if (!extracted) {
            /* No NDS at the well-known location — scan the SD and let
             * the user pick one from anywhere. */
            pkw_log("data_loader: scanning SD for HGSS dumps\n");
            char picked[NDS_PICKER_DIRLEN + NDS_PICKER_NAMELEN + 4];
            if (nds_picker_pick(picked, sizeof(picked))) {
                pkw_log("data_loader: user picked %s\n", picked);
                /* Splash while we crunch ~1.2 MB through BLZ + LZ77. */
                frame_begin();
                draw_title("Extracting walker assets...");
                draw_text(MARGIN_X, 60, 0.5f, COL_HIFG,
                          "Reading the NDS, decompressing");
                draw_text(MARGIN_X, 80, 0.5f, COL_HIFG,
                          "sprites and writing the cache.");
                draw_text(MARGIN_X, 110, 0.45f, COL_DIM,
                          "Should take just a few seconds.");
                scene_bottom();
                draw_title_bot("First-boot setup");
                draw_text(MARGIN_X, 40, 0.45f, COL_TEXT,
                          "Extracting from:");
                draw_text(MARGIN_X + 4, 60, 0.38f, COL_DIM, picked);
                draw_text(MARGIN_X, 90, 0.45f, COL_TEXT,
                          "Writing to:");
                draw_text(MARGIN_X + 4, 110, 0.38f, COL_DIM,
                          "sdmc:/3ds/pokeStride/data/*.bin");
                draw_text(MARGIN_X, 140, 0.42f, COL_DIM,
                          "Only happens once.");
                frame_end();
                extracted = nds_extract_from_path(picked, NULL);
            } else {
                pkw_log("data_loader: user skipped NDS picker\n");
            }
        }
        if (extracted) {
            pkw_log("data_loader: extraction succeeded, reloading\n");
            try_load_all();
        } else {
            pkw_log("data_loader: running in degraded mode\n");
        }
    }

    /* Build the GPU atlas of HGSS box icons. Independent of the other
     * blobs above; if its .bin files are missing we just won't show
     * icons in pickers (text-only fallback). */
    bool pi = pokeicon_init();

    /* Load the item / move / met-location string tables into RAM.
     * If files are missing the lookup helpers return NULL and the UI
     * falls back to numeric IDs. */
    bool nm = names_init();

    /* Load per-species personal data (gender ratio + growth rate) from
     * the user's own cart-extracted cache. Falls back to neutral
     * defaults if absent (claim/send still run, gender/EXP may be off
     * until the cache exists). */
    bool ps = personal_init();

    pkw_log("=== data_loader: done "
            "(font=%d course_bg=%d phcgra=%d phcicon=%d "
            "pokeicon=%d names=%d personal=%d) ===\n",
            g_font_loaded, g_course_bitmaps_loaded,
            g_phcgra_loaded, g_phcicon_loaded, pi, nm, ps);
}
