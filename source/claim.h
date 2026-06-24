/*
 * claim.h - Claim caught Pokemon from walker back into HGSS sav.
 *
 * Workflow:
 *   1. Read 3 caught_pokemon slots from pweep @ 0xCE8C.
 *   2. For each non-empty slot:
 *      - Synthesize a PK4 using HGSS player's OT (TID/SID/name) + a fresh
 *        random PID (or seeded by caller). Met location = "Pokewalker"
 *        (0x37 = MAPNAME_GS_PHC). Met level = walker's recorded level.
 *      - Find first empty box slot in HGSS storage and write the
 *        encrypted PK4 there.
 *   3. Sync walker stats to HGSS sav:
 *      - walker_steps = walker's HealthData total_steps (REPLACE)
 *      - walker_watts += walker's HealthData total_watts (ADD, cap 9_999_999)
 *   4. Recompute storage CRC (handled by hgss_save).
 *
 * IMPORTANT: HGSS `_set_pokemon_base` IGNORES the walker's rareColor
 * (shiny) flag — claimed Pokemon are forced non-shiny regardless. We
 * replicate that behavior.
 */

#ifndef CLAIM_H
#define CLAIM_H

#include <stdint.h>
#include <stdbool.h>
#include "pweep.h"
#include "hgss_sav.h"

/* Dowsing-find summary for one slot of the walker's 3-entry buffer at
 * EEP 0xCEBC. */
#define CLAIM_MAX_ITEMS  3
typedef struct {
    uint16_t item_id;     /* gen-4 item id; 0 = slot was empty */
    uint8_t  bag;         /* hgss_bag_kind_t value */
    bool     added;       /* true if hgss_bag_add succeeded */
} claim_item_t;

typedef struct {
    int pokemon_claimed;    /* number of slots successfully claimed */
    int pokemon_skipped;    /* slots that couldn't fit (storage full) */
    uint32_t walker_steps_before;
    uint32_t walker_steps_after;
    uint16_t walker_watts_added;
    uint32_t prng_seed_used;

    int items_claimed;      /* number of non-empty dowsing slots */
    int items_added;        /* successfully routed into HGSS bag */
    claim_item_t items[CLAIM_MAX_ITEMS];
} claim_result_t;

/* Run claim. Returns false on fatal error (loaded state invalid).
 * `prng_seed` selects the PID/nature RNG state — pass 0 for a clock-based
 * seed, or any non-zero value for deterministic output (good for testing).
 * On success, fills `result` with stats. */
bool pweep_claim_into_sav(pweep_t *pweep,
                          hgss_sav_t *sav,
                          uint32_t prng_seed,
                          claim_result_t *result);

/* Return the sent Pokemon back into HGSS PC box.
 *
 * Reads HGSS WalkerPair (encrypted PK4 @ 0xE5E0), if PID != 0:
 *   - decrypts, applies walker session bonuses:
 *       friendship += friendship_bonus (cap 255)
 *       EXP += exp_bonus (cap level 100 per species growth curve)
 *   - re-encrypts with updated checksum
 *   - writes to first empty PC box slot
 *   - zeros the WalkerPair area (clears the "out" state)
 *   - clears the walker's walk_data Pokemon flags + pairing has-pokemon bit
 *
 * `friendship_bonus` and `exp_bonus` defaults: simple model derived from
 * walker watts. Typical: friendship = +10 per 100 watts walked,
 * EXP = watts * 10 (rough). Set both to 0 to skip bonuses (just move
 * the unchanged Pokemon back).
 */
typedef struct {
    bool   returned;          /* true if a Pokemon was returned */
    uint16_t species;
    uint8_t  level_before;
    uint8_t  level_after;
    uint8_t  friendship_before;
    uint8_t  friendship_after;
    int      box, slot;       /* where it was deposited */
} return_result_t;

bool pweep_return_sent(pweep_t *pweep,
                       hgss_sav_t *sav,
                       uint8_t friendship_bonus,
                       uint32_t exp_bonus,
                       return_result_t *result);

#endif
