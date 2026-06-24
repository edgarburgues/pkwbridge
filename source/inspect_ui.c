/*
 * inspect_ui.c - read-only inspection of a pweep + a sav.
 *
 *   pick pweep (optional, B to skip)
 *   pick sav   (optional, B to skip)
 *   paginate L/R between loaded files
 */
#include "inspect_ui.h"

#include <3ds.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app_paths.h"
#include "app_state.h"
#include "applog.h"
#include "file_list.h"
#include "names.h"
#include "pokeicon.h"
#include "ui.h"

#include "hgss_sav.h"
#include "pk4.h"
#include "pweep.h"

static const char *LANG_TAG[] = {"?","JPN","ENG","FRE","ITA","GER","?","SPA","KOR"};

/* Draw a species icon at (x, y) using citro2d. scale 1.0 = 32x32,
 * 2.0 = 64x64. No-op if pokeicon isn't loaded or species is invalid. */
static void draw_icon_at(uint16_t species, float x, float y, float scale) {
    if (!pokeicon_loaded() || species == 0) return;
    C2D_Image img;
    if (pokeicon_get(species, &img)) {
        C2D_DrawImageAt(img, x, y, 0.5f, NULL, scale, scale);
    }
}

static void draw_pweep_info(pweep_t *pw) {
    static const char *st[] = { "OK", "UNINIT", "BAD" };
    float y = CONTENT_Y_TOP;
    if (!pw || !pw->loaded) {
        draw_text(MARGIN_X, y, 0.5f, COL_ERR, "pweep not loaded");
        return;
    }

    /* Icon strip on the right side: walking Pokemon (2x = 64x64) above
     * the caught list (each 1x = 32x32). Pure visual, the text on the
     * left already says what each is. */
    const uint8_t *wd = pweep_walk_data(pw);
    uint16_t walk_sp = wd[0] | (wd[1] << 8);
    if (walk_sp) {
        draw_icon_at(walk_sp, TOP_W - 76, CONTENT_Y_TOP + 4, 2.0f);
        draw_textf(TOP_W - 76, CONTENT_Y_TOP + 72, 0.36f, COL_DIM,
                   "walker");
    }
    int n = pweep_caught_count(pw);
    for (int i = 0; i < n; i++) {
        pweep_caught_t c;
        if (pweep_caught_get(pw, i, &c) && c.species) {
            float ix = TOP_W - 100 + i * 34;
            float iy = CONTENT_Y_TOP + 100;
            draw_icon_at(c.species, ix, iy, 1.0f);
        }
    }
    /* Group 1: reliable pair statuses on a single dense row. */
    draw_text(MARGIN_X, y, 0.45f, COL_HIFG, "Reliable pairs:");
    y += LINE_H_TIGHT;
    draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
               "pair %s  health %s  walk %s",
               st[pw->pairing], st[pw->health], st[pw->walk_sentinel]);
    y += LINE_H_TIGHT;
    draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
               "ident %s  lcd %s",
               st[pw->walk_identity], st[pw->lcd_init]);
    y += LINE_H_TIGHT + 4;

    /* Group 2: state. */
    draw_text(MARGIN_X, y, 0.45f, COL_HIFG, "State:");
    y += LINE_H_TIGHT;
    draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
               "has Pokemon : %s",
               pweep_has_pokemon(pw) ? "YES" : "no"); y += LINE_H_TIGHT;
    draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
               "watts       : %u / 9999", pweep_get_watts(pw)); y += LINE_H_TIGHT;
    draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
               "total steps : %lu",
               (unsigned long)pweep_get_total_steps(pw)); y += LINE_H_TIGHT;
    draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
               "caught used : %d / 3", pweep_caught_count(pw));
    y += LINE_H_TIGHT + 4;

    /* Group 3: sent Pokemon. */
    draw_text(MARGIN_X, y, 0.45f, COL_HIFG, "Sent Pokemon (walk_data):");
    y += LINE_H_TIGHT;
    const char *nm = species_name(walk_sp);
    if (nm) draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                       "species %s  level %u",
                       nm, wd[WALKDATA_OFF_LEVEL]);
    else    draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                       "species %u  level %u",
                       walk_sp, wd[WALKDATA_OFF_LEVEL]);
    y += LINE_H_TIGHT;

    /* Caught list, condensed. */
    if (n > 0) {
        y += 4;
        draw_text(MARGIN_X, y, 0.45f, COL_HIFG, "Caught slots:");
        y += LINE_H_TIGHT;
        for (int i = 0; i < n && y + LINE_H_TIGHT <= CONTENT_Y_BOT; i++) {
            pweep_caught_t c;
            if (pweep_caught_get(pw, i, &c) && c.species) {
                const char *cn = species_name(c.species);
                if (cn) draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                                    "[%d] %s lv %u", i, cn, c.level);
                else    draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                                    "[%d] sp %u lv %u", i, c.species, c.level);
                y += LINE_H_TIGHT;
            }
        }
    }
}

static void draw_sav_info(hgss_sav_t *sv) {
    float y = CONTENT_Y_TOP;
    if (!sv || !sv->loaded) {
        draw_text(MARGIN_X, y, 0.5f, COL_ERR, "sav not loaded");
        return;
    }
    /* CRCs */
    draw_textf(MARGIN_X, y, 0.40f,
               sv->general_status == HGSS_OK ? COL_OK : COL_ERR,
               "General CRC %s p%d   Storage CRC %s p%d",
               sv->general_status == HGSS_OK ? "OK" : "BAD",
               sv->active_general_part,
               sv->storage_status == HGSS_OK ? "OK" : "BAD",
               sv->active_storage_part);
    y += LINE_H_TIGHT + 4;

    /* Trainer */
    uint8_t l = hgss_ot_language(sv);
    draw_textf(MARGIN_X, y, 0.40f, COL_TEXT,
               "TID %u   SID %u",
               hgss_tid(sv), hgss_sid(sv)); y += LINE_H_TIGHT;
    draw_textf(MARGIN_X, y, 0.40f, COL_TEXT,
               "OT gender %s   language %u (%s)",
               hgss_ot_gender(sv) ? "F" : "M", l, LANG_TAG[l & 0xF]);
    y += LINE_H_TIGHT + 4;

    /* Walker stats in HGSS */
    draw_textf(MARGIN_X, y, 0.40f, COL_TEXT,
               "walker steps %lu",
               (unsigned long)hgss_walker_steps(sv)); y += LINE_H_TIGHT;
    draw_textf(MARGIN_X, y, 0.40f, COL_TEXT,
               "walker watts %lu",
               (unsigned long)hgss_walker_watts(sv)); y += LINE_H_TIGHT;
    draw_textf(MARGIN_X, y, 0.40f, COL_TEXT,
               "walker courses 0x%08lX",
               (unsigned long)hgss_walker_courses(sv)); y += LINE_H_TIGHT + 4;

    /* WalkerPair PK4 */
    uint8_t pk[PK4_SIZE_STORED];
    memcpy(pk, hgss_walker_pair_const(sv), PK4_SIZE_STORED);
    pk4_decrypt(pk);
    uint16_t cs = pk4_u16(pk, PK4_OFF_CHECKSUM);
    uint16_t cc = pk4_compute_checksum(pk);
    draw_text(MARGIN_X, y, 0.45f, COL_HIFG, "WalkerPair PK4:"); y += LINE_H_TIGHT;
    if (cs == cc && cs && pk4_u16(pk, PK4_OFF_SPECIES)) {
        uint16_t sp = pk4_u16(pk, PK4_OFF_SPECIES);
        const char *nm = species_name(sp);
        if (nm) draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                           "%s   EXP %lu",
                           nm, (unsigned long)pk4_u32(pk, PK4_OFF_EXP));
        else    draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                           "species %u   EXP %lu",
                           sp, (unsigned long)pk4_u32(pk, PK4_OFF_EXP));
        y += LINE_H_TIGHT;
        draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                   "fr %u  ab %u  lang %u",
                   pk[PK4_OFF_FRIENDSHIP], pk[PK4_OFF_ABILITY],
                   pk[PK4_OFF_LANGUAGE]);
        /* Icon top-right (64x64) of the WalkerPair Pokemon. */
        draw_icon_at(sp, TOP_W - 76, CONTENT_Y_TOP + 4, 2.0f);
        draw_textf(TOP_W - 88, CONTENT_Y_TOP + 72, 0.36f, COL_DIM,
                   "WalkerPair");
    } else {
        draw_text(MARGIN_X + 10, y, 0.40f, COL_DIM, "(empty)");
    }
}

void inspect_flow(void) {
    pkw_log("\n=== inspect flow: enter ===\n");
    pweep_t    *pw = app_state_pweep();
    hgss_sav_t *sv = app_state_sav();
    pkw_log("inspect: using active pweep=%s sav=%s\n",
            app_state_pweep_path(), app_state_sav_path());

    int page = 0;
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & (KEY_B | KEY_START)) {
            pkw_log("=== inspect flow: exit ===\n");
            return;
        }
        if (d & (KEY_R | KEY_DRIGHT)) page++;
        if (d & (KEY_L | KEY_DLEFT))  page--;
        int p = ((page % 2) + 2) % 2;

        frame_begin();
        if (p == 0) {
            draw_title("Inspect: pweep");
            draw_pweep_info(pw);
        } else {
            draw_title("Inspect: HGSS sav");
            draw_sav_info(sv);
        }
        draw_footer("(L/R) page  (B) back");
        frame_end();
    }
}
