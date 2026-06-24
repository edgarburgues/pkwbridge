/*
 * send_ui.c - Send Pokemon flow with proper back-navigation.
 *
 * Stages: box -> slot -> route -> preview -> run.
 *   B at slot picker  -> back to box picker
 *   B at route picker -> back to slot picker
 *   B at preview      -> back to route picker
 *   B at box picker   -> exits the flow (main menu)
 *
 * When the user picks a route different from the pweep template's
 * current, we copy the encounter data + item tables from course_defs
 * (course_data.c) directly into walk_data. That way the walker shows
 * the right Pokemon for the chosen route, not whatever the template
 * happened to have.
 */
#include "send_ui.h"

#include <3ds.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_paths.h"
#include "app_state.h"
#include "applog.h"
#include "course_bitmaps.h"
#include "course_data.h"
#include "courses.h"
#include "data_loader.h"
#include "file_list.h"
#include "gen4_chartable.h"
#include "io_util.h"
#include "names.h"
#include "pk_summary.h"
#include "pokeicon.h"
#include "ui.h"

#include "hgss_sav.h"
#include "pk4.h"
#include "pweep.h"

/* Encode an ASCII C string into a gen4 wide-char buffer, terminated by
 * 0xFFFF and padded with 0xFFFF. cap_bytes must be even. */
static void encode_gen4_str(const char *src, uint8_t *dst, size_t cap_bytes) {
    if (cap_bytes < 2) return;
    size_t max_chars = (cap_bytes - 2) / 2;  /* leave room for EOM */
    size_t i = 0;
    while (src && src[i] && i < max_chars) {
        uint16_t code = gen4_from_ascii(src[i]);
        if (code == 0) code = gen4_from_ascii(' ');
        dst[i*2]     = (uint8_t)( code       & 0xFF);
        dst[i*2 + 1] = (uint8_t)((code >> 8) & 0xFF);
        i++;
    }
    /* EOM + pad. */
    while ((i + 1) * 2 <= cap_bytes) {
        dst[i*2]     = 0xFF;
        dst[i*2 + 1] = 0xFF;
        i++;
    }
}

/* Offset of the 32x24 course background bitmap inside the pweep,
 * relative to the start of the file. Matches PHCCourseData.course[192]
 * which sits at +0xBE within PHCCourseData; PHCCourseData starts at
 * the walk_data offset 0x8F00. */
#define PWEEP_OFF_COURSE_BITMAP   (PWEEP_OFF_WALK_DATA + 0xBE)

#define BOXES   18
#define SLOTS   30

/* ---------------------------------------------------------------------- */
/* helpers                                                                */
/* ---------------------------------------------------------------------- */

static bool decrypt_slot(const hgss_sav_t *sv, int box, int slot,
                          uint8_t out[PK4_SIZE_STORED])
{
    const uint8_t *src = hgss_box_slot_const(sv, box, slot);
    if (!src) return false;
    memcpy(out, src, PK4_SIZE_STORED);
    pk4_decrypt(out);
    uint16_t cs = pk4_u16(out, PK4_OFF_CHECKSUM);
    uint16_t cc = pk4_compute_checksum(out);
    if (cs != cc) return false;
    if (pk4_u16(out, PK4_OFF_SPECIES) == 0) return false;
    return true;
}

static const course_def_t *find_course(uint8_t course_id) {
    for (int i = 0; i < COURSE_COUNT; i++)
        if (course_defs[i].course_id == course_id) return &course_defs[i];
    return NULL;
}

/* Apply a course definition into the pweep. Writes:
 *   +0x27 courseId  (= graphic_id - 1)
 *   +0x28..+0x51 courseNameString (LEFT UNTOUCHED — needs gen4 encoder)
 *   +0x52..+0x81 Enemy[3] (PHCPokemonBase per enemy)
 *   +0x82..+0x87 walkStep[3]
 *   +0x88..+0x8A encountRate[3]
 *   +0x8C..+0x9F itemNumber[10]
 *   +0xA0..+0xB3 itemStep[10]
 *   +0xB4..+0xBD itemRate[10]
 *   walk_data + 0xBE..+0x17D (192 B) = course background bitmap, from
 *       course_bitmaps[graphic_id - 1]. THIS is what makes the walker
 *       LCD show a different scenery per route — without writing it the
 *       walker draws whatever the template had.
 * Returns true if the course was found and applied. */
static bool apply_course_to_pweep(pweep_t *pw, uint8_t course_id) {
    const course_def_t *cd = find_course(course_id);
    if (!cd) {
        pkw_log("send: course_id %u has no built-in data, "
                "writing only the id byte\n", course_id);
        uint8_t *wd = pweep_walk_data_mut(pw);
        wd[WALKDATA_OFF_COURSE_ID] = course_id;
        return false;
    }
    uint8_t *wd = pweep_walk_data_mut(pw);
    /* The walker's courseId byte is graphic_id-1 (range 0..7), NOT the
     * canonical 1..27. The 27 courses share 8 unique background
     * graphics; this byte indexes that small set. */
    wd[WALKDATA_OFF_COURSE_ID] = (uint8_t)(cd->graphic_id - 1);
    /* courseNameString (+0x28..+0x51, 42 B = 21 gen4 wide chars): use
     * the English name from COURSES[] (which is what the user picked in
     * the UI). The walker font on EU/US carts supports Latin chars. */
    encode_gen4_str(course_name(course_id),
                    wd + WALKDATA_OFF_COURSE_NAME,
                    WALKDATA_COURSE_NAME_SIZE);
    /* Enemy[3]: PHCPokemonBase struct, 16 B each, starting at +0x52. */
    for (int i = 0; i < 3; i++) {
        uint8_t *e = wd + WALKDATA_OFF_ENEMY + i * WALKDATA_ENEMY_STRIDE;
        const course_enemy_t *src = &cd->enemies[i];
        /* +0x00 species (u16 LE) */
        e[0] = (uint8_t)(src->species & 0xFF);
        e[1] = (uint8_t)((src->species >> 8) & 0xFF);
        /* +0x02 held_item */
        e[2] = (uint8_t)(src->held_item & 0xFF);
        e[3] = (uint8_t)((src->held_item >> 8) & 0xFF);
        /* +0x04..+0x0B moves[4] */
        for (int m = 0; m < 4; m++) {
            e[4 + m*2]     = (uint8_t)(src->moves[m] & 0xFF);
            e[4 + m*2 + 1] = (uint8_t)((src->moves[m] >> 8) & 0xFF);
        }
        e[0x0C] = src->level;
        /* folm:5 | sex:2 | isEgg:1 */
        e[0x0D] = (uint8_t)((src->form & 0x1F) | ((src->sex & 0x03) << 5));
        /* reverseFlag:1 | rareColor:1 | padding:6 */
        e[0x0E] = 0;
        e[0x0F] = 0;
    }
    /* walkStep[3] (u16 LE) */
    for (int i = 0; i < 3; i++) {
        wd[WALKDATA_OFF_WALK_STEP + i*2]     = (uint8_t)(cd->enemies[i].need_step & 0xFF);
        wd[WALKDATA_OFF_WALK_STEP + i*2 + 1] = (uint8_t)((cd->enemies[i].need_step >> 8) & 0xFF);
    }
    /* encountRate[3] (u8) */
    for (int i = 0; i < 3; i++)
        wd[WALKDATA_OFF_ENCOUNT_RATE + i] = cd->enemies[i].odds;
    /* itemNumber[10] (u16 LE) */
    for (int i = 0; i < 10; i++) {
        wd[WALKDATA_OFF_ITEM_NUMBER + i*2]     = (uint8_t)(cd->items[i].item_no & 0xFF);
        wd[WALKDATA_OFF_ITEM_NUMBER + i*2 + 1] = (uint8_t)((cd->items[i].item_no >> 8) & 0xFF);
    }
    /* itemStep[10] (u16 LE) */
    for (int i = 0; i < 10; i++) {
        wd[WALKDATA_OFF_ITEM_STEP + i*2]     = (uint8_t)(cd->items[i].need_step & 0xFF);
        wd[WALKDATA_OFF_ITEM_STEP + i*2 + 1] = (uint8_t)((cd->items[i].need_step >> 8) & 0xFF);
    }
    /* itemRate[10] (u8) */
    for (int i = 0; i < 10; i++)
        wd[WALKDATA_OFF_ITEM_RATE + i] = cd->items[i].odds;

    /* Course background bitmap: 192 bytes (32x24 @ 2bpp packed) at
     * pweep + 0x8FBE. Only write if the data was loaded from
     * sdmc:/3ds/pokeStride/data/course_bitmaps.bin; otherwise keep
     * the template's bytes (walker shows its previous background). */
    uint8_t gid = cd->graphic_id;
    if (app_data_course_bitmaps_loaded() &&
        gid >= 1 && gid <= COURSE_BITMAP_COUNT) {
        uint8_t *raw = pw->data;
        memcpy(raw + PWEEP_OFF_COURSE_BITMAP,
               course_bitmaps[gid - 1],
               COURSE_BITMAP_SIZE);
        pkw_log("send: wrote course bitmap (graphic_id=%u, 192 B at 0x%X)\n",
                gid, PWEEP_OFF_COURSE_BITMAP);
    } else if (!app_data_course_bitmaps_loaded()) {
        pkw_log("send: course_bitmaps.bin not loaded, "
                "skipped background write\n");
    } else {
        pkw_log("send: graphic_id %u out of range, no bitmap written\n", gid);
    }

    pkw_log("send: applied course %u (graphic %u) data "
            "(enemies %u/%u/%u, %d items)\n",
            course_id, gid,
            cd->enemies[0].species, cd->enemies[1].species, cd->enemies[2].species,
            10);
    return true;
}

/* ---------------------------------------------------------------------- */
/* storage grid picker                                                    */
/*                                                                        */
/* Pokemon-Storage-style: shows all 30 slots of a box as a 6x5 grid of    */
/* icons on the top screen, L/R cycles through the 18 boxes, the bottom  */
/* screen shows full details (species, level, OT info, EXP, friendship)  */
/* for the focused slot. Replaces the previous separate box+slot pickers */
/* in a single step.                                                      */
/*                                                                        */
/* Returns 0 on selection (fills *out_box, *out_slot), -1 on B, -2 on    */
/* START.                                                                 */
/* ---------------------------------------------------------------------- */

typedef struct {
    bool     occupied;
    uint8_t  pk[PK4_SIZE_STORED];    /* decrypted */
    uint16_t species;
    uint8_t  level;
} slot_info_t;

#define GRID_ROWS    5
#define GRID_COLS    6
#define GRID_ICON   32                              /* sprite size */
#define GRID_STRIDE 34                              /* 32 icon + 2 gap */
#define GRID_X      ((TOP_W - GRID_COLS * GRID_STRIDE) / 2)
#define GRID_Y      48                              /* below box header */

static void load_box_slots(const hgss_sav_t *sv, int box,
                           slot_info_t out[SLOTS])
{
    for (int s = 0; s < SLOTS; s++) {
        memset(&out[s], 0, sizeof out[s]);
        if (!decrypt_slot(sv, box, s, out[s].pk)) continue;
        uint16_t sp = pk4_u16(out[s].pk, PK4_OFF_SPECIES);
        if (sp == 0) continue;
        out[s].occupied = true;
        out[s].species  = sp;
        out[s].level    = pweep_pk4_level(out[s].pk);
    }
}

static int box_count(const slot_info_t slots[SLOTS]) {
    int n = 0;
    for (int s = 0; s < SLOTS; s++) if (slots[s].occupied) n++;
    return n;
}

/* HGSS-summary-style bottom panel. Single page, dense, fits in the
 * 200 px content area between title (y=22) and footer (y=222).
 *
 *   y= 24..38   box / slot position
 *   y= 38..100  identity card (64x64 icon + name, lv, gender, nature,
 *               species-when-nicknamed, held item)
 *   y=106..134  Stats: IVs + hidden-power / ability slot / ball / fr
 *   y=140..156  Moves: 4 move IDs (names need NDS msg extraction)
 *   y=162..192  Origin: OT name+gender+IDs, met level/loc/date/lang
 *
 * Each block is introduced by a label in COL_HILITE on the left
 * (no full-width divider — clutters more than it organises). */
static void draw_grid_detail(const slot_info_t *si, int box, int slot) {
    draw_title_bot("Pokemon");
    draw_textf(MARGIN_X, 26, 0.40f, COL_DIM,
               "Box %d / 18    slot %d / %d",
               box + 1, slot + 1, SLOTS);

    if (!si->occupied) {
        draw_text(MARGIN_X, 90, 0.55f, COL_DIM, "(empty slot)");
        draw_footer_bot("(B) back");
        return;
    }

    pk_summary_t s;
    if (!pk_summary_from(si->pk, si->level, &s)) {
        draw_text(MARGIN_X, 90, 0.5f, COL_ERR, "(decode failed)");
        draw_footer_bot("(B) back");
        return;
    }

    /* === Identity card =========================================== */
    if (pokeicon_loaded()) {
        C2D_Image img;
        if (pokeicon_get(s.species, &img)) {
            C2D_DrawImageAt(img, MARGIN_X, 40, 0.5f, NULL, 2.0f, 2.0f);
        }
    }
    const char *display_name = s.nickname[0]
        ? s.nickname
        : (species_name(s.species) ? species_name(s.species) : "?");
    const char *gender_str = (s.gender == 0) ? "M"
                           : (s.gender == 1) ? "F" : "-";
    draw_textf(MARGIN_X + 74, 40, 0.55f, COL_HIFG,
               "%s", display_name);
    draw_textf(MARGIN_X + 74, 58, 0.42f, COL_TEXT,
               "Lv %u   %s   %s",
               s.level, gender_str, pk_nature_name(s.nature));
    if (s.is_nicknamed && species_name(s.species)) {
        draw_textf(MARGIN_X + 74, 74, 0.36f, COL_DIM,
                   "(%s)", species_name(s.species));
    }
    if (s.held_item) {
        const char *iname = item_name(s.held_item);
        if (iname) {
            draw_textf(MARGIN_X + 74, 88, 0.40f, COL_WARN,
                       "@ %s", iname);
        } else {
            draw_textf(MARGIN_X + 74, 88, 0.40f, COL_WARN,
                       "holds item #%u", s.held_item);
        }
    }

    /* === Stats ================================================== */
    float y = 106;
    draw_text(MARGIN_X, y, 0.40f, COL_HILITE, "Stats");
    draw_textf(MARGIN_X + 44, y, 0.40f, COL_TEXT,
               "HP %2u  At %2u  Df %2u  Sp %2u  SA %2u  SD %2u",
               s.iv[0], s.iv[1], s.iv[2], s.iv[3], s.iv[4], s.iv[5]);
    y += LINE_H_TIGHT;
    draw_textf(MARGIN_X + 44, y, 0.36f, COL_DIM,
               "HP type %u/%u  ability slot %u  ball %u  fr %u",
               s.ev_hidden_power_type, s.ev_hidden_power_power,
               s.ability_slot, s.pokeball, s.friendship);

    /* === Moves ================================================== */
    y += LINE_H_TIGHT + 4;
    draw_text(MARGIN_X, y, 0.40f, COL_HILITE, "Moves");
    /* Two rows of two moves each: easier to fit names than one row of
     * four. Empty slots (move id 0) get a dash. */
    for (int m = 0; m < 4; m++) {
        const char *mn = (s.move[m] != 0) ? move_name(s.move[m]) : NULL;
        char fallback[8];
        if (s.move[m] == 0) {
            fallback[0] = '-'; fallback[1] = 0;
            mn = fallback;
        } else if (!mn) {
            snprintf(fallback, sizeof fallback, "#%u", s.move[m]);
            mn = fallback;
        }
        float mx = MARGIN_X + 44 + (m % 2) * 138;
        float my = y + (m / 2) * LINE_H_TIGHT;
        draw_textf(mx, my, 0.38f, COL_TEXT, "%s", mn);
    }
    y += LINE_H_TIGHT;   /* second move row */

    /* === Origin (OT + met) ====================================== */
    y += LINE_H_TIGHT + 4;
    draw_text(MARGIN_X, y, 0.40f, COL_HILITE, "Origin");
    draw_textf(MARGIN_X + 44, y, 0.40f, COL_TEXT,
               "OT %s %s  ID %u / %u",
               s.ot_name, s.ot_gender ? "F" : "M",
               s.ot_tid, s.ot_sid);
    y += LINE_H_TIGHT;
    draw_textf(MARGIN_X + 44, y, 0.36f, COL_DIM,
               "met lv%u  loc %u  %02u-%02u-%02u  lang %s",
               s.met_level, s.met_location,
               s.met_year, s.met_month, s.met_day,
               pk_language_short(s.language));
    draw_footer_bot("(A) select   (B) back");
}

static int run_grid_picker(const hgss_sav_t *sv,
                           int initial_box,
                           int *out_box, int *out_slot)
{
    slot_info_t slots[SLOTS];
    int box = (initial_box >= 0 && initial_box < BOXES) ? initial_box : 0;
    load_box_slots(sv, box, slots);

    int cursor = 0;
    for (int s = 0; s < SLOTS; s++) {
        if (slots[s].occupied) { cursor = s; break; }
    }

    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & KEY_START) return -2;
        if (d & KEY_B)     return -1;
        if (d & KEY_DUP) {
            int n = cursor - GRID_COLS;
            if (n < 0) n += SLOTS;
            cursor = n;
        }
        if (d & KEY_DDOWN) cursor = (cursor + GRID_COLS) % SLOTS;
        if (d & KEY_DLEFT) {
            int n = cursor - 1;
            if (n < 0) n += SLOTS;
            cursor = n;
        }
        if (d & KEY_DRIGHT) cursor = (cursor + 1) % SLOTS;
        if (d & KEY_L) {
            box = (box + BOXES - 1) % BOXES;
            load_box_slots(sv, box, slots);
        }
        if (d & KEY_R) {
            box = (box + 1) % BOXES;
            load_box_slots(sv, box, slots);
        }
        if ((d & KEY_A) && slots[cursor].occupied) {
            *out_box  = box;
            *out_slot = cursor;
            return 0;
        }

        frame_begin();
        /* === TOP: header + grid =================================== */
        draw_title("Send 1/3: pick a Pokemon");
        draw_textf(MARGIN_X + 6, 28, 0.5f, COL_HIFG,
                   "Box %2d / %d", box + 1, BOXES);
        draw_textf(MARGIN_X + 100, 30, 0.42f, COL_DIM,
                   "%d / %d   slot %d / %d",
                   box_count(slots), SLOTS, cursor + 1, SLOTS);
        draw_text(TOP_W - 96, 30, 0.42f, COL_HILITE, "L < box >  R");

        /* grid cells. Each cell occupies a 34x34 tile but the icon is
         * only 32x32, centred with 1 px of breathing room on each side.
         * The cursor frame fills the full 34x34 tile so it lives in
         * that gap and never overlaps neighbour icons. */
        for (int s = 0; s < SLOTS; s++) {
            int r = s / GRID_COLS;
            int c = s % GRID_COLS;
            float tx = (float)(GRID_X + c * GRID_STRIDE);
            float ty = (float)(GRID_Y + r * GRID_STRIDE);
            float ix = tx + 1;     /* icon top-left, centred in tile */
            float iy = ty + 1;
            if (slots[s].occupied) {
                if (pokeicon_loaded()) {
                    C2D_Image img;
                    if (pokeicon_get(slots[s].species, &img)) {
                        C2D_DrawImageAt(img, ix, iy, 0.5f,
                                        NULL, 1.0f, 1.0f);
                    }
                } else {
                    draw_textf(ix + 4, iy + 10, 0.35f, COL_TEXT,
                               "#%u", slots[s].species);
                }
            }
            if (s == cursor) {
                draw_frame(tx, ty, GRID_STRIDE, GRID_STRIDE, 2, COL_HIFG);
            }
        }
        draw_footer("(D-Pad) move  (L/R) box  (A) pick  (B) back");

        /* === BOTTOM: detail panel ================================= */
        scene_bottom();
        draw_grid_detail(&slots[cursor], box, cursor);
        frame_end();
    }
    return -2;
}

/* ---------------------------------------------------------------------- */
/* route picker                                                           */
/* Returns: course id, -1 on B, -2 on START.                              */
/* ---------------------------------------------------------------------- */

/* Bottom-screen detail panel for the focused route: 3 enemy slots with
 * icon, species name, level, encounter %. Uses course_defs[] which has
 * the canonical enemy_species/level/odds per route. */
static void draw_route_detail(uint8_t course_id) {
    draw_title_bot("Route details");
    const course_def_t *cd = find_course(course_id);
    if (!cd) {
        draw_textf(MARGIN_X, 36, 0.45f, COL_DIM,
                   "%s", course_name(course_id));
        draw_text(MARGIN_X, 60, 0.45f, COL_WARN,
                  "No built-in encounter data.");
        return;
    }
    draw_textf(MARGIN_X, 30, 0.5f, COL_HIFG, "%s", course_name(course_id));
    draw_textf(BOT_W - 56, 32, 0.4f, COL_DIM, "ID %u", course_id);
    draw_text(MARGIN_X, 52, 0.42f, COL_HILITE, "Encounters");
    float y = 70;
    for (int i = 0; i < 3; i++) {
        const course_enemy_t *e = &cd->enemies[i];
        if (e->species == 0) continue;
        if (pokeicon_loaded()) {
            C2D_Image img;
            if (pokeicon_get(e->species, &img)) {
                C2D_DrawImageAt(img, MARGIN_X, y, 0.5f, NULL, 1.0f, 1.0f);
            }
        }
        const char *nm = species_name(e->species);
        if (nm) draw_textf(MARGIN_X + 40, y + 4, 0.45f, COL_HIFG, "%s", nm);
        else    draw_textf(MARGIN_X + 40, y + 4, 0.45f, COL_HIFG,
                           "species %u", e->species);
        draw_textf(MARGIN_X + 40, y + 18, 0.4f, COL_TEXT,
                   "lv %u  rate %u%%  step %u",
                   e->level, e->odds, e->need_step);
        y += 38;
    }
}

static int run_route_picker(uint8_t current) {
    int cursor = 0;
    for (int i = 0; i < COURSES_COUNT; i++)
        if (COURSES[i].id == current) { cursor = i; break; }
    int scroll = 0;
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & KEY_START) return -2;
        if (d & KEY_B)     return -1;
        if (d & KEY_DUP)   cursor = (cursor - 1 + COURSES_COUNT) % COURSES_COUNT;
        if (d & KEY_DDOWN) cursor = (cursor + 1) % COURSES_COUNT;
        if (d & KEY_DLEFT)  cursor = (cursor > 5 ? cursor - 5 : 0);
        if (d & KEY_DRIGHT) cursor = (cursor + 5 < COURSES_COUNT
                                       ? cursor + 5 : COURSES_COUNT - 1);
        if (d & KEY_A)     return COURSES[cursor].id;

        if (cursor < scroll) scroll = cursor;
        if (cursor >= scroll + VIS_ROWS) scroll = cursor - VIS_ROWS + 1;
        int end = scroll + VIS_ROWS;
        if (end > COURSES_COUNT) end = COURSES_COUNT;

        frame_begin();
        draw_title("Send 2/3: pick route");
        draw_textf(MARGIN_X, 28, 0.4f, COL_DIM,
                   "Current: %s", course_name(current));
        /* Legend at top-right corner of the content area. */
        draw_text(TOP_W - 100, 28, 0.36f, COL_WARN, "[WiFi] = event-only");
        float y = 50;
        for (int i = scroll; i < end; i++) {
            bool sel = (i == cursor);
            bool is_current = (COURSES[i].id == current);
            bool has_data = (find_course(COURSES[i].id) != NULL);
            bool is_wifi  = COURSES[i].wifi_only;
            u32 col = sel ? COL_HIFG
                          : (is_current ? COL_OK
                                        : (has_data ? COL_TEXT : COL_DIM));
            if (sel) draw_frame(0, y - 2, TOP_W, LINE_H + 2, 2, COL_HILITE);
            const char *mark = sel ? ">" : (is_current ? "*" : " ");
            const char *tag  = is_wifi ? "[WiFi] " : "       ";
            u32 tag_col = sel ? COL_HIFG : (is_wifi ? COL_WARN : COL_DIM);
            /* Tag drawn separately so it can have its own colour. */
            draw_textf(MARGIN_X + 6,  y, 0.42f, col, "%s %2u", mark, COURSES[i].id);
            draw_text (MARGIN_X + 50, y, 0.42f, tag_col, tag);
            draw_textf(MARGIN_X + 100, y, 0.42f, col,
                       "%s%s",
                       COURSES[i].name,
                       has_data ? "" : " (no data)");
            y += LINE_H + 2;
        }
        draw_footer("(A) pick  (B) back to slot");

        /* Bottom screen: detail panel for the focused route. */
        scene_bottom();
        draw_route_detail(COURSES[cursor].id);
        draw_footer_bot("(A) pick   (B) back");

        frame_end();
    }
    return -2;
}

/* ---------------------------------------------------------------------- */
/* preview                                                                */
/* ---------------------------------------------------------------------- */
typedef struct {
    uint8_t  pk_dec[PK4_SIZE_STORED];
    uint16_t species;
    uint8_t  level;
    uint16_t tid, sid;
    uint32_t exp;
    uint8_t  language;
    uint8_t  friendship;
    int      box, slot;
    uint8_t  route_id;
    uint8_t  route_current;
    bool     have_course_data;   /* true if route_id is in course_defs */
} send_ctx_t;

static void draw_send_preview(void *vctx) {
    send_ctx_t *c = vctx;
    static const char *LANG_TAG[] = {"?","JPN","ENG","FRE","ITA","GER","?","SPA","KOR"};
    float y = CONTENT_Y_TOP;
    const char *nm = species_name(c->species);
    if (nm) draw_textf(MARGIN_X, y, 0.55f, COL_HIFG, "%s", nm);
    else    draw_textf(MARGIN_X, y, 0.55f, COL_HIFG, "species %u", c->species);
    draw_textf(TOP_W - 80, y + 6, 0.45f, COL_DIM, "lv %u", c->level);
    y += LINE_H + 8;

    draw_textf(MARGIN_X, y, 0.40f, COL_TEXT,
               "From sav box %d slot %d", c->box + 1, c->slot + 1);
    y += LINE_H_TIGHT;
    draw_textf(MARGIN_X, y, 0.40f, COL_TEXT,
               "TID %u  SID %u  lang %u (%s)",
               c->tid, c->sid, c->language, LANG_TAG[c->language & 0xF]);
    y += LINE_H_TIGHT;
    draw_textf(MARGIN_X, y, 0.40f, COL_TEXT,
               "EXP %lu  fr %u",
               (unsigned long)c->exp, c->friendship);
    y += LINE_H_TIGHT + 6;

    draw_textf(MARGIN_X, y, 0.45f, COL_HIFG,
               "Route: %s", course_name(c->route_id));
    y += LINE_H_TIGHT;
    if (c->have_course_data) {
        draw_text(MARGIN_X + 10, y, 0.38f, COL_OK,
                  "Encounters + items will be set"); y += 12;
        draw_text(MARGIN_X + 10, y, 0.38f, COL_DIM,
                  "from built-in course tables.");
        y += 12;
    } else {
        draw_text(MARGIN_X + 10, y, 0.38f, COL_WARN,
                  "No built-in data for this course."); y += 12;
        draw_text(MARGIN_X + 10, y, 0.38f, COL_DIM,
                  "Walker name updates; encounters won't.");
        y += 12;
    }
    y += 4;
    draw_text(MARGIN_X, y, 0.38f, COL_DIM,
              "Also mirrors PK4 to sav.WalkerPair so a");
    y += 12;
    draw_text(MARGIN_X, y, 0.38f, COL_DIM,
              "future claim+return can retrieve it.");
}

/* ---------------------------------------------------------------------- */
/* result screen                                                          */
/* ---------------------------------------------------------------------- */
typedef struct {
    bool ok_send;
    bool ok_pwp_canon, ok_pwp_ts;
    bool ok_sav_canon, ok_sav_ts;
    char pwp_base[MAX_NAME_LEN];
    char sav_base[MAX_NAME_LEN];
    char ts[32];
} send_result_t;

static void draw_send_result(void *vctx) {
    send_result_t *r = vctx;
    float y = CONTENT_Y_TOP;
    if (!r->ok_send) {
        draw_text(MARGIN_X, y, 0.55f, COL_ERR, "Send FAILED");
        return;
    }
    draw_text(MARGIN_X, y, 0.55f, COL_OK, "Pokemon written"); y += LINE_H + 6;
    draw_textf(MARGIN_X, y, 0.42f, COL_DIM, "ts %s", r->ts);
    y += LINE_H_TIGHT + 4;

    draw_text(MARGIN_X, y, 0.45f, COL_HIFG, "Live (what's read):");
    y += LINE_H_TIGHT;
    draw_textf(MARGIN_X + 10, y, 0.38f,
               r->ok_pwp_canon ? COL_OK : COL_ERR,
               "pweep canonical  %s",
               r->ok_pwp_canon ? "ok" : "FAIL"); y += 13;
    draw_textf(MARGIN_X + 10, y, 0.38f,
               r->ok_sav_canon ? COL_OK : COL_ERR,
               "sav canonical    %s",
               r->ok_sav_canon ? "ok" : "FAIL"); y += 13 + 4;

    draw_text(MARGIN_X, y, 0.45f, COL_HIFG, "Backups:");
    y += LINE_H_TIGHT;
    draw_textf(MARGIN_X + 10, y, 0.38f,
               r->ok_pwp_ts ? COL_OK : COL_ERR,
               "pwp  %s", r->pwp_base); y += 13;
    draw_textf(MARGIN_X + 10, y, 0.38f,
               r->ok_sav_ts ? COL_OK : COL_ERR,
               "sav  %s", r->sav_base);
}

/* ---------------------------------------------------------------------- */
/* main flow — state machine with B back-nav                              */
/* ---------------------------------------------------------------------- */
typedef enum {
    STG_BOX,
    STG_SLOT,
    STG_ROUTE,
    STG_PREVIEW,
    STG_RUN,
    STG_DONE,
    STG_ABORT,
} stage_t;

/* Pairing safety. The walker may already hold a Pokemon that the user
 * doesn't want to lose. Three cases:
 *
 *   1. Walker empty (has_pokemon=0):      free to send, no warning.
 *   2. Walker has Pokemon AND it matches  paired to this save — BLOCK.
 *      sav.WalkerPair species:             User must Claim+Return first.
 *   3. Walker has Pokemon but species     foreign walker or desync; allow
 *      doesn't match sav.WalkerPair:       with an explicit "I know" confirm.
 *
 * Returns true to proceed with Send, false to abort the flow. */
typedef enum {
    PAIR_FREE,      /* nothing on walker */
    PAIR_LOCKED,    /* paired to this save, has Pokemon -> block */
    PAIR_FOREIGN,   /* has Pokemon but doesn't match this save */
} pair_state_t;

static pair_state_t check_pair_state(const pweep_t *pw, const hgss_sav_t *sv,
                                     uint16_t *out_walker_sp,
                                     uint8_t  *out_walker_lv)
{
    *out_walker_sp = 0;
    *out_walker_lv = 0;
    if (!pweep_has_pokemon(pw)) return PAIR_FREE;

    const uint8_t *wd = pweep_walk_data(pw);
    uint16_t walker_sp = (uint16_t)wd[WALKDATA_OFF_SPECIES]
                       | ((uint16_t)wd[WALKDATA_OFF_SPECIES + 1] << 8);
    *out_walker_sp = walker_sp;
    *out_walker_lv = wd[WALKDATA_OFF_LEVEL];
    if (walker_sp == 0) return PAIR_FREE;

    uint8_t pk[PK4_SIZE_STORED];
    memcpy(pk, hgss_walker_pair_const(sv), PK4_SIZE_STORED);
    pk4_decrypt(pk);
    uint16_t cs = pk4_u16(pk, PK4_OFF_CHECKSUM);
    uint16_t cc = pk4_compute_checksum(pk);
    uint16_t sav_sp = pk4_u16(pk, PK4_OFF_SPECIES);
    bool sav_valid = (cs == cc) && sav_sp != 0;

    if (sav_valid && sav_sp == walker_sp) return PAIR_LOCKED;
    return PAIR_FOREIGN;
}

/* Block screen: walker has a Pokemon paired to this save. User must
 * Claim+Return before they can send a new one. */
static void show_pair_locked(uint16_t species, uint8_t level) {
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & (KEY_B | KEY_A | KEY_START)) return;
        frame_begin();
        draw_title("Walker is in use");
        float y = CONTENT_Y_TOP + 10;
        draw_text(MARGIN_X, y, 0.5f, COL_WARN,
                  "Send blocked: walker already has a"); y += LINE_H;
        draw_text(MARGIN_X, y, 0.5f, COL_WARN,
                  "Pokemon paired to this HGSS save."); y += LINE_H + 8;
        const char *nm = species_name(species);
        if (nm) draw_textf(MARGIN_X + 10, y, 0.5f, COL_HIFG,
                           "%s   lv %u", nm, level);
        else    draw_textf(MARGIN_X + 10, y, 0.5f, COL_HIFG,
                           "species %u  lv %u", species, level);
        y += LINE_H + 10;
        draw_text(MARGIN_X, y, 0.42f, COL_TEXT,
                  "To send a different Pokemon, first run"); y += LINE_H_TIGHT;
        draw_text(MARGIN_X, y, 0.42f, COL_TEXT,
                  "Finish walk (or Only return walker)"); y += LINE_H_TIGHT;
        draw_text(MARGIN_X, y, 0.42f, COL_TEXT,
                  "to bring the current one back safely.");
        draw_footer("(B) main menu");

        scene_bottom();
        draw_title_bot("Why this guard?");
        float by = 30;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "Overwriting the walker would erase"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "the currently-paired Pokemon with no"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "way to recover it. Both the walker"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "and your save reference the same"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "PK4 in WalkerPair — they're in sync."); by += LINE_H + 4;
        draw_text(MARGIN_X, by, 0.42f, COL_DIM,
                  "Finish walk first, then Send.");
        draw_footer_bot("(B) main menu");
        frame_end();
    }
}

/* Confirmation screen: walker has a Pokemon but it doesn't match this
 * save. Likely a foreign walker or a previous desync. Allow if user
 * explicitly accepts. Returns true to proceed. */
static bool confirm_pair_foreign(uint16_t species, uint8_t level) {
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & (KEY_B | KEY_START)) return false;
        if (d & KEY_X) return true;
        frame_begin();
        draw_title("Foreign walker detected");
        float y = CONTENT_Y_TOP + 10;
        draw_text(MARGIN_X, y, 0.5f, COL_WARN,
                  "Walker has a Pokemon, but it doesn't"); y += LINE_H;
        draw_text(MARGIN_X, y, 0.5f, COL_WARN,
                  "match this HGSS save's WalkerPair."); y += LINE_H + 8;
        const char *nm = species_name(species);
        if (nm) draw_textf(MARGIN_X + 10, y, 0.5f, COL_HIFG,
                           "On walker: %s   lv %u", nm, level);
        else    draw_textf(MARGIN_X + 10, y, 0.5f, COL_HIFG,
                           "On walker: species %u  lv %u", species, level);
        y += LINE_H + 10;
        draw_text(MARGIN_X, y, 0.42f, COL_TEXT,
                  "Sending will overwrite this Pokemon"); y += LINE_H_TIGHT;
        draw_text(MARGIN_X, y, 0.42f, COL_TEXT,
                  "and re-pair the walker with this save.");
        draw_footer("(X) overwrite + re-pair   (B) cancel");

        scene_bottom();
        draw_title_bot("When does this happen?");
        float by = 30;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "- You loaded a pweep dumped from a"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "  different HGSS save."); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "- A previous flow desynced before we"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "  started dual-writing the canonical"); by += LINE_H_TIGHT;
        draw_text(MARGIN_X, by, 0.42f, COL_TEXT,
                  "  save."); by += LINE_H + 4;
        draw_text(MARGIN_X, by, 0.42f, COL_DIM,
                  "If unsure, cancel and inspect first.");
        draw_footer_bot("(X) confirm   (B) cancel");
        frame_end();
    }
    return false;
}

void send_flow(void) {
    pkw_log("\n=== send flow: enter ===\n");

    pweep_t    *pw = app_state_pweep();
    hgss_sav_t *sv = app_state_sav();
    const char *pweep_path = app_state_pweep_path();
    const char *sav_path   = app_state_sav_path();
    pkw_log("send: active pweep=%s sav=%s\n", pweep_path, sav_path);

    /* Pairing safety: refuse to overwrite a paired walker. */
    uint16_t walker_sp = 0; uint8_t walker_lv = 0;
    pair_state_t ps = check_pair_state(pw, sv, &walker_sp, &walker_lv);
    pkw_log("send: pair_state=%d (walker sp=%u lv=%u)\n",
            (int)ps, walker_sp, walker_lv);
    if (ps == PAIR_LOCKED) {
        show_pair_locked(walker_sp, walker_lv);
        pkw_log("=== send flow: blocked by pairing guard ===\n");
        return;
    }
    if (ps == PAIR_FOREIGN) {
        if (!confirm_pair_foreign(walker_sp, walker_lv)) {
            pkw_log("=== send flow: foreign-walker confirm cancelled ===\n");
            return;
        }
        pkw_log("send: user accepted foreign-walker overwrite\n");
    }

    send_ctx_t  ctx; memset(&ctx, 0, sizeof ctx);
    stage_t     stage = STG_BOX;     /* re-used as "grid stage" */
    int         box_cursor = -1;     /* remember last box for back-nav */
    int         box = -1, slot = -1;

    while (stage != STG_DONE && stage != STG_ABORT) {
        switch (stage) {
            case STG_BOX:
            case STG_SLOT: {
                int r = run_grid_picker(sv, box_cursor, &box, &slot);
                if (r == -2 || r == -1) {
                    pkw_log("send: grid picker -> %s\n",
                            r == -2 ? "START/exit" : "B/main menu");
                    stage = STG_ABORT;
                    break;
                }
                box_cursor = box;
                pkw_log("send: box %d slot %d selected\n", box + 1, slot + 1);
                if (!decrypt_slot(sv, box, slot, ctx.pk_dec)) {
                    pkw_log("send: decrypt failed -> abort\n");
                    run_info_screen("Send: bad slot", "(B) back", NULL, NULL);
                    stage = STG_ABORT;
                    break;
                }
                ctx.species    = pk4_u16(ctx.pk_dec, PK4_OFF_SPECIES);
                ctx.tid        = pk4_u16(ctx.pk_dec, PK4_OFF_OT_TID);
                ctx.sid        = pk4_u16(ctx.pk_dec, PK4_OFF_OT_SID);
                ctx.exp        = pk4_u32(ctx.pk_dec, PK4_OFF_EXP);
                ctx.language   = ctx.pk_dec[PK4_OFF_LANGUAGE];
                ctx.friendship = ctx.pk_dec[PK4_OFF_FRIENDSHIP];
                ctx.level      = pweep_pk4_level(ctx.pk_dec);
                ctx.box        = box;
                ctx.slot       = slot;
                ctx.route_current = pweep_walk_data(pw)[WALKDATA_OFF_COURSE_ID];
                pkw_log("send: pk4 sp=%u lv=%u exp=%lu fr=%u lang=%u tid=%u sid=%u\n",
                        ctx.species, ctx.level, (unsigned long)ctx.exp,
                        ctx.friendship, ctx.language, ctx.tid, ctx.sid);
                pkw_log("send: pweep current course=%u (%s)\n",
                        ctx.route_current, course_name(ctx.route_current));
                stage = STG_ROUTE;
                break;
            }
            case STG_ROUTE: {
                int r = run_route_picker(ctx.route_current);
                if (r == -2) { stage = STG_ABORT; break; }
                if (r == -1) {
                    pkw_log("send: route picker B -> grid\n");
                    stage = STG_BOX;   /* back to grid */
                    break;
                }
                ctx.route_id = (uint8_t)r;
                ctx.have_course_data = (find_course(ctx.route_id) != NULL);
                pkw_log("send: route %u (%s) selected  data=%s\n",
                        ctx.route_id, course_name(ctx.route_id),
                        ctx.have_course_data ? "yes" : "no");
                stage = STG_PREVIEW;
                break;
            }
            case STG_PREVIEW: {
                bool decided = false;
                while (aptMainLoop() && !decided) {
                    hidScanInput();
                    u32 d = hidKeysDown();
                    if (d & KEY_START) { stage = STG_ABORT; decided = true; break; }
                    if (d & KEY_B) {
                        pkw_log("send: preview B -> route\n");
                        stage = STG_ROUTE;
                        decided = true;
                        break;
                    }
                    if (d & KEY_A) {
                        pkw_log("send: preview confirmed\n");
                        stage = STG_RUN;
                        decided = true;
                        break;
                    }
                    frame_begin();
                    draw_title("Send 3/3: confirm");
                    draw_send_preview(&ctx);
                    draw_footer("(A) send   (B) back to route");
                    frame_end();
                }
                break;
            }
            case STG_RUN:
                /* fall through — handled below the loop */
                stage = STG_DONE;
                break;
            default: break;
        }
    }

    if (stage == STG_ABORT) {
        pkw_log("=== send flow: aborted ===\n");
        return;
    }

    /* ---- execute ---- */
    frame_begin();
    draw_title("Send: writing");
    draw_text(MARGIN_X, 60, 0.5f, COL_TEXT, "Processing...");
    draw_footer(NULL);
    frame_end();

    /* Apply course data BEFORE pweep_send_pokemon so the courseId byte
     * we set survives.  pweep_send_pokemon will overwrite walk_data
     * 0x00..0x27 with the Pokemon-specific bytes; 0x28..0xBD (where
     * encounters live) stays untouched. The +0x27 courseId byte gets
     * overwritten too, so apply_course_to_pweep runs LAST below. */
    bool ok_send = pweep_send_pokemon(pw, ctx.pk_dec, ctx.level, ctx.route_id);
    if (ok_send) {
        apply_course_to_pweep(pw, ctx.route_id);
    }
    pkw_log("send: pweep_send_pokemon sp=%u lv=%u route=%u -> %s\n",
            ctx.species, ctx.level, ctx.route_id, ok_send ? "ok" : "FAIL");

    /* Mirror PK4 (encrypted) into sav.WalkerPair AND remove from source
     * box slot. Without this last step the Pokemon would still exist in
     * the box, allowing duplication (clone in box + clone on walker). */
    bool ok_sav_mirror = false;
    if (ok_send) {
        uint8_t pk_enc[PK4_SIZE_STORED];
        memcpy(pk_enc, ctx.pk_dec, PK4_SIZE_STORED);
        pk4_encrypt(pk_enc);
        uint8_t *wp = hgss_walker_pair(sv);
        if (wp) {
            memcpy(wp, pk_enc, PK4_SIZE_STORED);
            ok_sav_mirror = true;
            /* Tell HGSS a Pokemon is now out on the walker: deposit flag
             * (General[0xE5DC]) = PHC_DEPOSIT, return-box = source box.
             * This is the bit that makes the game actually show the Pokemon
             * as "out walking"; without it the PK4 sits there ignored. */
            hgss_set_walker_deposit(sv, (uint16_t)HGSS_PHC_DEPOSIT,
                                    (uint16_t)ctx.box);
        }
        uint8_t *src_slot = hgss_box_slot(sv, ctx.box, ctx.slot);
        if (src_slot) {
            memset(src_slot, 0, PK4_SIZE_STORED);
        }
        pkw_log("send: sav.WalkerPair mirror [%s], cleared box %d slot %d\n",
                ok_sav_mirror ? "ok" : "FAIL", ctx.box + 1, ctx.slot + 1);
    }

    /* Write outputs: timestamped + canonical for BOTH pweep + sav. */
    send_result_t res; memset(&res, 0, sizeof res);
    res.ok_send = ok_send;
    timestamp_suffix(res.ts, sizeof res.ts);
    char out_pwp[MAX_NAME_LEN * 2 + 64];
    char out_sav[MAX_NAME_LEN * 2 + 64];
    output_path_for(pweep_path, "rom", res.ts, BACKUP_PWEEP_DIR, out_pwp, sizeof out_pwp);
    output_path_for(sav_path,   "sav", res.ts, BACKUP_SAV_DIR,   out_sav, sizeof out_sav);
    snprintf(res.pwp_base, sizeof res.pwp_base, "%s", basename_of(out_pwp));
    snprintf(res.sav_base, sizeof res.sav_base, "%s", basename_of(out_sav));

    char bak_pwp[MAX_NAME_LEN * 2 + 8];
    char bak_sav[MAX_NAME_LEN * 2 + 8];
    backup_path_for(pweep_path, BACKUP_PWEEP_DIR, bak_pwp, sizeof bak_pwp);
    backup_path_for(sav_path,   BACKUP_SAV_DIR,   bak_sav, sizeof bak_sav);
    FILE *probe;
    if (!(probe = fopen(bak_pwp, "rb"))) {
        bool b = copy_file(pweep_path, bak_pwp);
        pkw_log("send: .bak pweep [%s]\n", b ? "ok" : "FAIL");
    } else { fclose(probe); }
    if (!(probe = fopen(bak_sav, "rb"))) {
        bool b = copy_file(sav_path, bak_sav);
        pkw_log("send: .bak sav [%s]\n", b ? "ok" : "FAIL");
    } else { fclose(probe); }

    if (ok_send) {
        res.ok_pwp_ts    = pweep_save(pw, out_pwp);
        res.ok_pwp_canon = pweep_save(pw, pweep_path);
        pkw_log("send: pweep ts [%s] canon [%s]\n",
                res.ok_pwp_ts ? "ok" : "FAIL",
                res.ok_pwp_canon ? "ok" : "FAIL");
        prune_timestamped_backups(pweep_path, "rom", BACKUP_PWEEP_DIR, 2);
    }
    if (ok_send && ok_sav_mirror) {
        res.ok_sav_ts    = hgss_save(sv, out_sav, true);
        res.ok_sav_canon = hgss_save(sv, sav_path, true);
        pkw_log("send: sav ts [%s] canon [%s]\n",
                res.ok_sav_ts ? "ok" : "FAIL",
                res.ok_sav_canon ? "ok" : "FAIL");
        prune_timestamped_backups(sav_path, "sav", BACKUP_SAV_DIR, 2);
    }

    run_info_screen("Send result", "(B) main menu", draw_send_result, &res);
    pkw_log("=== send flow: exit ===\n");
}
