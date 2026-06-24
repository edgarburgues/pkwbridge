/*
 * claim_ui.c - Claim + Return flow.
 *
 *   1. Use the active pweep + sav (selected via app_state, not picked
 *      again here — guarantees same pair as Send, Inspect, etc.).
 *   2. Preview state of both, ask for confirmation.
 *   3. Run pweep_claim_into_sav + pweep_return_sent in memory.
 *   4. Write timestamped outputs (history) AND overwrite canonical
 *      paths (so pokeStride / HGSS see the new state).
 *   5. Show result screen.
 */
#include "claim_ui.h"

#include <3ds.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "app_paths.h"
#include "app_state.h"
#include "applog.h"
#include "file_list.h"
#include "io_util.h"
#include "names.h"
#include "pokeicon.h"
#include "ui.h"

#include "claim.h"
#include "hgss_sav.h"
#include "pk4.h"
#include "pweep.h"

static const char *LANG_TAG[] = {"?","JPN","ENG","FRE","ITA","GER","?","SPA","KOR"};

/* Draw a species icon. No-op if pokeicon isn't loaded or species==0. */
static void draw_icon_at(uint16_t species, float x, float y, float scale) {
    if (!pokeicon_loaded() || species == 0) return;
    C2D_Image img;
    if (pokeicon_get(species, &img)) {
        C2D_DrawImageAt(img, x, y, 0.5f, NULL, scale, scale);
    }
}

/* ----- preview screen ----- */
static void draw_preview(const pweep_t *pw, const hgss_sav_t *sv) {
    float y = CONTENT_Y_TOP;
    draw_text(MARGIN_X, y, 0.45f, COL_HIFG, "Walker (pweep):");
    y += LINE_H_TIGHT;
    if (pw && pw->loaded) {
        int n = pweep_caught_count(pw);
        draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                   "caught %d   has_pokemon %s",
                   n, pweep_has_pokemon(pw) ? "yes (return)" : "no");
        y += LINE_H_TIGHT;
        for (int i = 0; i < n && y + LINE_H_TIGHT <= CONTENT_Y_BOT - 80; i++) {
            pweep_caught_t c;
            if (pweep_caught_get(pw, i, &c) && c.species) {
                const char *name = species_name(c.species);
                if (name) draw_textf(MARGIN_X + 38, y - 2, 0.38f, COL_TEXT,
                                     "[%d] %s lv %u", i, name, c.level);
                else      draw_textf(MARGIN_X + 38, y - 2, 0.38f, COL_TEXT,
                                     "[%d] sp %u lv %u",
                                     i, c.species, c.level);
                /* Mini icon (1x = 32x32) on the left of the row — but
                 * shrunk visually via row height. Drawn even though it
                 * overflows because pokeicon is transparent at edges. */
                draw_icon_at(c.species, MARGIN_X + 4, y - 10, 0.5f);
                y += 16;
            }
        }
        pweep_walker_stats_t ws;
        if (pweep_walker_stats_get(pw, &ws)) {
            draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                       "steps %lu   watts %u",
                       (unsigned long)ws.total_steps, ws.total_watts);
            y += LINE_H_TIGHT;
        }
    } else {
        draw_text(MARGIN_X + 10, y, 0.40f, COL_ERR, "not loaded");
        y += LINE_H_TIGHT;
    }
    y += 4;

    draw_text(MARGIN_X, y, 0.45f, COL_HIFG, "HGSS sav:");
    y += LINE_H_TIGHT;
    if (sv && sv->loaded) {
        uint8_t l = hgss_ot_language(sv);
        draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                   "TID %u  SID %u  lang %u (%s)",
                   hgss_tid(sv), hgss_sid(sv), l, LANG_TAG[l & 0xF]);
        y += LINE_H_TIGHT;
        draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                   "walker steps %lu  watts %lu",
                   (unsigned long)hgss_walker_steps(sv),
                   (unsigned long)hgss_walker_watts(sv));
        y += LINE_H_TIGHT;

        uint8_t pk[PK4_SIZE_STORED];
        memcpy(pk, hgss_walker_pair_const(sv), PK4_SIZE_STORED);
        pk4_decrypt(pk);
        uint16_t cs = pk4_u16(pk, PK4_OFF_CHECKSUM);
        uint16_t cc = pk4_compute_checksum(pk);
        if (cs == cc && cs && pk4_u16(pk, PK4_OFF_SPECIES)) {
            uint16_t wp_sp = pk4_u16(pk, PK4_OFF_SPECIES);
            const char *nm = species_name(wp_sp);
            if (nm) draw_textf(MARGIN_X + 10, y, 0.40f, COL_OK,
                                "WalkerPair %s (will return)", nm);
            else    draw_textf(MARGIN_X + 10, y, 0.40f, COL_OK,
                                "WalkerPair species %u (will return)",
                                wp_sp);
            /* Prominent 2x icon (64x64) of what's about to be returned. */
            draw_icon_at(wp_sp, TOP_W - 76, CONTENT_Y_TOP + 4, 2.0f);
            draw_textf(TOP_W - 88, CONTENT_Y_TOP + 72, 0.36f, COL_OK,
                       "returning");
        } else {
            draw_text(MARGIN_X + 10, y, 0.40f, COL_DIM, "WalkerPair empty");
        }
    } else {
        draw_text(MARGIN_X + 10, y, 0.40f, COL_ERR, "not loaded");
    }
}

/* ----- result screen state ----- */
typedef struct {
    claim_result_t  cr;
    return_result_t rr;
    bool   saved_sav, saved_pwp;
    char   ts[32];
} result_ctx_t;

static void draw_result(void *ctx) {
    result_ctx_t *r = ctx;
    float y = CONTENT_Y_TOP;

    draw_text(MARGIN_X, y, 0.45f, COL_HIFG, "Claim:"); y += LINE_H_TIGHT;
    draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
               "claimed %d   skipped %d",
               r->cr.pokemon_claimed, r->cr.pokemon_skipped); y += LINE_H_TIGHT;
    draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
               "steps %lu -> %lu   watts +%u",
               (unsigned long)r->cr.walker_steps_before,
               (unsigned long)r->cr.walker_steps_after,
               r->cr.walker_watts_added);
    y += LINE_H_TIGHT;
    if (r->cr.items_claimed > 0) {
        draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                   "dowsing items %d/%d added to bag",
                   r->cr.items_added, r->cr.items_claimed);
        y += LINE_H_TIGHT;
    }
    y += 4;

    draw_text(MARGIN_X, y, 0.45f, COL_HIFG, "Return:"); y += LINE_H_TIGHT;
    if (r->rr.returned) {
        const char *nm = species_name(r->rr.species);
        if (nm) draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                            "%s   level %u -> %u",
                            nm, r->rr.level_before, r->rr.level_after);
        else    draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                            "species %u   level %u -> %u",
                            r->rr.species, r->rr.level_before, r->rr.level_after);
        y += LINE_H_TIGHT;
        draw_textf(MARGIN_X + 10, y, 0.40f, COL_TEXT,
                   "fr %u -> %u   box %d slot %d",
                   r->rr.friendship_before, r->rr.friendship_after,
                   r->rr.box, r->rr.slot);
        y += LINE_H_TIGHT + 4;
    } else {
        draw_text(MARGIN_X + 10, y, 0.40f, COL_DIM, "no Pokemon to return");
        y += LINE_H_TIGHT + 4;
    }

    draw_text(MARGIN_X, y, 0.45f, COL_HIFG, "Output (canonical updated):");
    y += LINE_H_TIGHT;
    draw_textf(MARGIN_X + 10, y, 0.38f,
               r->saved_sav ? COL_OK : COL_ERR,
               "sav %s",  r->saved_sav ? "wrote ok" : "FAILED"); y += 12;
    draw_textf(MARGIN_X + 10, y, 0.38f,
               r->saved_pwp ? COL_OK : COL_ERR,
               "rom %s",  r->saved_pwp ? "wrote ok" : "FAILED"); y += 12;
    draw_textf(MARGIN_X + 10, y, 0.36f, COL_DIM,
               "history ts %s", r->ts);
}

/* Shared engine. `do_claim` / `do_return` toggle which operations run.
 * Title strings adapt to the selected mix. */
typedef struct {
    bool do_claim;
    bool do_return;
    const char *title_preview;
    const char *title_running;
    const char *title_result;
    const char *log_tag;
} flow_kind_t;

static void run_engine(const flow_kind_t *k) {
    pkw_log("\n=== %s flow: enter ===\n", k->log_tag);

    pweep_t    *pw = app_state_pweep();
    hgss_sav_t *sv = app_state_sav();
    const char *pweep_path = app_state_pweep_path();
    const char *sav_path   = app_state_sav_path();
    pkw_log("%s: pweep=%s\n", k->log_tag, pweep_path);
    pkw_log("%s: sav  =%s\n", k->log_tag, sav_path);

    /* Preview + confirm. */
    pkw_log("%s: showing preview\n", k->log_tag);
    bool go = false;
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & (KEY_B | KEY_START)) {
            pkw_log("%s: preview cancelled by user\n", k->log_tag);
            return;
        }
        if (d & KEY_A) { go = true; break; }
        frame_begin();
        draw_title(k->title_preview);
        draw_preview(pw, sv);
        draw_footer("(A) run   (B) cancel");
        frame_end();
    }
    if (!go) return;
    pkw_log("%s: preview confirmed -> running\n", k->log_tag);

    /* Run. */
    claim_result_t cr; return_result_t rr;
    memset(&cr, 0, sizeof cr);
    memset(&rr, 0, sizeof rr);
    uint32_t seed = (uint32_t)time(NULL);

    frame_begin();
    draw_title(k->title_running);
    draw_text(MARGIN_X, 60, 0.5f, COL_TEXT, "Processing...");
    draw_footer(NULL);
    frame_end();

    bool cok = false, rok = false;
    if (k->do_claim) {
        cok = pweep_claim_into_sav(pw, sv, seed, &cr);
        pkw_log("%s claim ok=%d claimed=%d skipped=%d steps %lu->%lu watts+%u "
                "items %d/%d\n",
                k->log_tag, cok, cr.pokemon_claimed, cr.pokemon_skipped,
                (unsigned long)cr.walker_steps_before,
                (unsigned long)cr.walker_steps_after,
                cr.walker_watts_added,
                cr.items_added, cr.items_claimed);
        for (int i = 0; i < CLAIM_MAX_ITEMS; i++) {
            if (cr.items[i].item_id == 0) continue;
            pkw_log("  item[%d] id=%u bag=%u %s\n", i,
                    cr.items[i].item_id, cr.items[i].bag,
                    cr.items[i].added ? "added" : "BAG-FULL");
        }
    }
    if (k->do_return) {
        rok = pweep_return_sent(pw, sv, 10, 0, &rr);
        pkw_log("%s return ok=%d returned=%d species=%u lvl %u->%u fr %u->%u "
                "(box %d slot %d)\n",
                k->log_tag, rok, rr.returned, rr.species,
                rr.level_before, rr.level_after,
                rr.friendship_before, rr.friendship_after,
                rr.box, rr.slot);
    }

    /* Dual write: timestamped backup + canonical (live). One-time .bak first. */
    result_ctx_t res; memset(&res, 0, sizeof res);
    res.cr = cr; res.rr = rr;
    timestamp_suffix(res.ts, sizeof res.ts);

    char out_sav[MAX_NAME_LEN * 2 + 64];
    char out_pwp[MAX_NAME_LEN * 2 + 64];
    output_path_for(sav_path,   "sav", res.ts, BACKUP_SAV_DIR,   out_sav, sizeof out_sav);
    output_path_for(pweep_path, "rom", res.ts, BACKUP_PWEEP_DIR, out_pwp, sizeof out_pwp);

    char bak_sav[MAX_NAME_LEN * 2 + 8];
    char bak_pwp[MAX_NAME_LEN * 2 + 8];
    backup_path_for(sav_path,   BACKUP_SAV_DIR,   bak_sav, sizeof bak_sav);
    backup_path_for(pweep_path, BACKUP_PWEEP_DIR, bak_pwp, sizeof bak_pwp);
    FILE *probe;
    if (!(probe = fopen(bak_sav, "rb"))) {
        bool b = copy_file(sav_path, bak_sav);
        pkw_log("%s: .bak sav %s [%s]\n", k->log_tag, bak_sav, b ? "ok" : "FAIL");
    } else { fclose(probe); pkw_log("%s: .bak sav already exists\n", k->log_tag); }
    if (!(probe = fopen(bak_pwp, "rb"))) {
        bool b = copy_file(pweep_path, bak_pwp);
        pkw_log("%s: .bak pweep %s [%s]\n", k->log_tag, bak_pwp, b ? "ok" : "FAIL");
    } else { fclose(probe); pkw_log("%s: .bak pweep already exists\n", k->log_tag); }

    /* Sav write happens if EITHER op did something to it. */
    if (cok || rok) {
        res.saved_sav = hgss_save(sv, out_sav, true);
        pkw_log("%s: wrote ts sav %s [%s]\n",
                k->log_tag, out_sav, res.saved_sav ? "ok" : "FAIL");
        bool b = hgss_save(sv, sav_path, true);
        pkw_log("%s: wrote canon sav %s [%s]\n",
                k->log_tag, sav_path, b ? "ok" : "FAIL");
        if (!b) res.saved_sav = false;
        prune_timestamped_backups(sav_path, "sav", BACKUP_SAV_DIR, 2);
    }
    /* Pweep is always written: even pure return mutates pairing flags. */
    res.saved_pwp = pweep_save(pw, out_pwp);
    pkw_log("%s: wrote ts pweep %s [%s]\n",
            k->log_tag, out_pwp, res.saved_pwp ? "ok" : "FAIL");
    {
        bool b = pweep_save(pw, pweep_path);
        pkw_log("%s: wrote canon pweep %s [%s]\n",
                k->log_tag, pweep_path, b ? "ok" : "FAIL");
        if (!b) res.saved_pwp = false;
    }
    prune_timestamped_backups(pweep_path, "rom", BACKUP_PWEEP_DIR, 2);

    pkw_log("%s: showing result screen\n", k->log_tag);
    run_info_screen(k->title_result, "(B) main menu", draw_result, &res);
    pkw_log("=== %s flow: exit ===\n", k->log_tag);
}

void claim_flow(void) {
    static const flow_kind_t K = {
        .do_claim   = true, .do_return = true,
        .title_preview = "Preview - claim + return",
        .title_running = "Running claim + return",
        .title_result  = "Result - claim + return",
        .log_tag = "claim+return",
    };
    run_engine(&K);
}

void claim_caught_flow(void) {
    static const flow_kind_t K = {
        .do_claim   = true, .do_return = false,
        .title_preview = "Preview - claim caught only",
        .title_running = "Running claim caught",
        .title_result  = "Result - claim caught",
        .log_tag = "claim-only",
    };
    run_engine(&K);
}

void return_pokemon_flow(void) {
    static const flow_kind_t K = {
        .do_claim   = false, .do_return = true,
        .title_preview = "Preview - return Pokemon only",
        .title_running = "Running return",
        .title_result  = "Result - return Pokemon",
        .log_tag = "return-only",
    };
    run_engine(&K);
}
