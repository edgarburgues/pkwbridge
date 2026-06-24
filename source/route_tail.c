/*
 * route_tail.c - Synthesize the 10240-byte route tail of walker EEPROM.
 *
 * Injects the Pokemon icon, big portrait, course background, and (when
 * the font is loaded) regenerated text glyphs. Fields that are not
 * regenerated are preserved from the template tail.
 */

#include "route_tail.h"
#include "course_bitmaps.h"
#include "courses.h"
#include "data_loader.h"
#include "gen4_chartable.h"
#include "names.h"
#include "phcgra_data.h"
#include "phcicon_data.h"
#include "text_render.h"

#include <ctype.h>
#include <string.h>

/* Encode an ASCII C string into a gen4 wide-char buffer, terminated by
 * 0xFFFF. dst must hold (max_chars + 1) * 2 bytes. Unknown chars (incl.
 * non-ASCII bytes from a UTF-8 multibyte) collapse to ' '. */
static void encode_gen4(const char *src, uint8_t *dst, size_t max_chars) {
    size_t i = 0;
    for (; src && src[i] && i < max_chars; i++) {
        uint16_t code = gen4_from_ascii(src[i]);
        if (code == 0) code = gen4_from_ascii(' ');
        dst[i*2]     = (uint8_t)( code       & 0xFF);
        dst[i*2 + 1] = (uint8_t)((code >> 8) & 0xFF);
    }
    dst[i*2]     = 0xFF;
    dst[i*2 + 1] = 0xFF;
}

bool route_tail_render(const route_tail_pokemon_t *pkmn,
                       const route_tail_course_t *course,
                       const uint8_t *template_tail,
                       uint8_t *out) {
    if (!pkmn || !course || !out) return false;

    /* Start from template (preserves text glyphs and any route data we
     * don't regenerate). If no template, zero-fill — walker may render
     * garbled UI but still operates. */
    if (template_tail) {
        memcpy(out, template_tail, ROUTE_TAIL_SIZE);
    } else {
        memset(out, 0, ROUTE_TAIL_SIZE);
    }

    /* --- 1. Course background bitmap (192 B at offset 0x0000) ---
     * Only overwrite if data_loader has populated course_bitmaps[].
     * If absent the template's bytes survive — walker keeps its
     * previous background. */
    if (app_data_course_bitmaps_loaded() &&
        course->graphic_id >= 1 && course->graphic_id <= COURSE_BITMAP_COUNT) {
        memcpy(out + ROUTE_TAIL_OFF_COURSE,
               course_bitmaps[course->graphic_id - 1],
               COURSE_BITMAP_SIZE);
    }

    /* --- 2. Pokemon walking icon (384 B at offset 0x0200) --- */
    if (app_data_phcicon_loaded()) {
        uint16_t icon_idx = phcicon_index_from_species(pkmn->species, pkmn->form);
        const uint8_t *icon = phcicon_get(icon_idx);
        if (icon) {
            memcpy(out + ROUTE_TAIL_OFF_POKE_ICON,
                   icon, ROUTE_TAIL_SIZE_POKE_ICON);
        }
    }

    /* --- 3. Pokemon big portrait (1536 B at offset 0x0380) --- */
    if (app_data_phcgra_loaded()) {
        uint16_t big_idx = phcgra_index_from_species(pkmn->species,
                                                     pkmn->sex,
                                                     pkmn->form);
        const uint8_t *big = phcgra_get(big_idx);
        if (big) {
            memcpy(out + ROUTE_TAIL_OFF_POKE_BIG,
                   big, ROUTE_TAIL_SIZE_POKE_BIG);
        }
    }

    /* --- 4. Pokemon nickname (320 B at offset 0x0980) ---
     * Renders the PK4 nickname using the PHC-specific walker font
     * (inner file 11 of the Western gs_font.narc). Rendered at (2, 1)
     * within the 80×16 BMPWIN. */
    if (pkmn->nickname22 && app_data_font_loaded()) {
        text_render_gen4(pkmn->nickname22, 11 /* 22B / 2 */,
                         /*x0=*/2, /*y0=*/1, 80, 16,
                         out + ROUTE_TAIL_OFF_POKE_NAME);
    }

    /* --- 5. Enemy icons (3 × 384 B at offset 0x0AC0) ---
     * The walker firmware indexes the enemy sprite atlas by encounter
     * slot 0..2, NOT by species. One icon per slot is pre-rasterised and
     * stored as an opaque blob in the route tail. If they are not written
     * here, the walker LCD shows whatever the previous template had. */
    if (app_data_phcicon_loaded()) {
        for (int i = 0; i < 3; i++) {
            uint16_t sp = course->enemy_species[i];
            if (sp == 0) continue;
            uint16_t idx = phcicon_index_from_species(sp,
                                                      course->enemy_form[i]);
            const uint8_t *icon = phcicon_get(idx);
            if (icon) {
                memcpy(out + ROUTE_TAIL_OFF_ENEMY_ICON
                           + (size_t)i * ROUTE_TAIL_SIZE_POKE_ICON,
                       icon, ROUTE_TAIL_SIZE_POKE_ICON);
            }
        }
    }

    /* --- 6. Enemy "big" portrait (1536 B at offset 0x0F40) ---
     * Per the route_tail layout there's only ONE big portrait slot, used
     * by the dowsing reveal screen for enemy[2] (the rarest of the three
     * encounters). Same atlas as the player big sprite. */
    if (app_data_phcgra_loaded() && course->enemy_species[2] != 0) {
        uint16_t big = phcgra_index_from_species(course->enemy_species[2],
                                                  course->enemy_sex[2],
                                                  course->enemy_form[2]);
        const uint8_t *p = phcgra_get(big);
        if (p) {
            memcpy(out + ROUTE_TAIL_OFF_ENEMY_BIG,
                   p, ROUTE_TAIL_SIZE_POKE_BIG);
        }
    }

    /* --- 7. Enemy name glyphs (3 × 320 B at offset 0x1540) ---
     * Same render path as the player nickname at (2, 1) inside the 80×16
     * BMPWIN. Walker HGSS uses uppercase species names. We use the bundled
     * English k_species table — most Western species names are identical
     * across EN/ES/FR/DE/IT, and the gen4 charset can represent the few
     * that diverge (e.g. accented chars in some moves). */
    if (app_data_font_loaded()) {
        for (int i = 0; i < 3; i++) {
            uint16_t sp = course->enemy_species[i];
            if (sp == 0) continue;
            const char *nm = species_name(sp);
            if (!nm) continue;

            /* Encode "Pikachu" -> "PIKACHU" -> gen4 wide chars (11 max
             * fits the BMPWIN with the variable-width FONT_SYSTEM). */
            uint8_t gen4[24] = {0};
            size_t j = 0;
            for (; j < 11 && nm[j]; j++) {
                char c = (char)toupper((unsigned char)nm[j]);
                uint16_t code = gen4_from_ascii(c);
                if (code == 0) code = gen4_from_ascii(' ');
                gen4[j*2]     = (uint8_t)(code & 0xFF);
                gen4[j*2 + 1] = (uint8_t)((code >> 8) & 0xFF);
            }
            /* 0xFFFF terminator (text_render_gen4 stops there). */
            gen4[j*2]     = 0xFF;
            gen4[j*2 + 1] = 0xFF;

            text_render_gen4(gen4, 11, /*x0=*/2, /*y0=*/1, 80, 16,
                             out + ROUTE_TAIL_OFF_ENEMY_NAME
                                 + (size_t)i * ROUTE_TAIL_SIZE_NAME);
        }
    }

    /* --- 8. Course name glyph (320 B at offset 0x00C0) ---
     * Walker BMPWIN is 80×16, same as pokeName/enemyName. We render the
     * English course name (mixed case — HGSS renders course names with
     * the same casing as displayed in the DS UI, unlike species which it
     * uppercases). Variable-width FONT_SYSTEM fits ~11 chars at 80 px;
     * longer names get clipped, which is the same behaviour real HGSS
     * exhibits if the courseName field is overflowed. */
    if (app_data_font_loaded()) {
        const char *cn = course_name(course->course_id);
        if (cn) {
            uint8_t gen4[24] = {0};  /* 11 chars + EOM = 12 wide chars */
            encode_gen4(cn, gen4, 11);
            text_render_gen4(gen4, 11, /*x0=*/2, /*y0=*/0, 80, 16,
                             out + ROUTE_TAIL_OFF_COURSE_NAME);
        }
    }

    /* --- 9. Item name glyphs (10 × 384 B at offset 0x1900) ---
     * BMPWIN here is 96×16 (wider than the 80-px name fields), which fits
     * ~13 chars of FONT_SYSTEM. We render at (2, 1) — same y-baseline as
     * pokeName. course->item_id == 0 means "no item in this slot", which
     * on the walker is a blank glyph (we leave the template byte pattern
     * alone). */
    if (app_data_font_loaded()) {
        for (int i = 0; i < 10; i++) {
            uint16_t it = course->item_id[i];
            if (it == 0) continue;
            const char *nm = item_name(it);
            if (!nm) continue;
            uint8_t gen4[28] = {0};  /* 13 chars + EOM = 14 wide chars */
            encode_gen4(nm, gen4, 13);
            text_render_gen4(gen4, 13, /*x0=*/2, /*y0=*/1, 96, 16,
                             out + ROUTE_TAIL_OFF_ITEM_NAME
                                 + (size_t)i * ROUTE_TAIL_SIZE_ITEM_NAME);
        }
    }

    return true;
}
