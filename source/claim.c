/*
 * claim.c - Walker post-walk caught Pokemon -> HGSS sav storage.
 *
 * Synthesizes a PK4 from the 16-byte walker PHCPokemonBase + player's
 * OT info, then deposits in the first empty PC box slot.
 */

#include "claim.h"
#include "gen4_chartable.h"
#include "pk4.h"
#include "exp_curve.h"
#include "names.h"
#include "personal_gen4.h"

#include <3ds.h>     /* osGetTime for the RNG seed */
#include <ctype.h>
#include <string.h>
#include <time.h>

/* Classify a gen-4 item ID into the HGSS bag it belongs to.
 *
 * Buckets by ID range:
 *   Balls    (1..16)
 *   Medicine (17..44 — potions, heals, drinks, powders, ethers,
 *             Lava Cookie, Berry Juice, Sacred Ash)
 *   Berries  (149..212)
 *   TMHM     (328..427)
 *   Items    (everything else: held items, evolution stones, vitamins
 *             at 45-50, shards, flutes, valuables, ...) */
static hgss_bag_kind_t gen4_item_bag(uint16_t id) {
    if (id == 0) return HGSS_BAG_ITEMS;
    if (id <= 16) return HGSS_BAG_BALLS;
    if (id <= 44) return HGSS_BAG_MEDICINE;
    if (id >= 149 && id <= 212) return HGSS_BAG_BERRIES;
    if (id >= 328 && id <= 427) return HGSS_BAG_TMHM;
    return HGSS_BAG_ITEMS;
}

/* Met location ID for caught-by-walker Pokemon in HGSS (MAPNAME_GS_PHC).
 * Written to the trainer-memo place field at PK4 +0x80. */
#define MET_LOC_POKEWALKER  233  /* 0xE9 */

/* HGSS uses Poke Ball (4) for Pokewalker-caught Pokemon. */
#define DEFAULT_POKEBALL    4

/* xorshift32 PRNG for deterministic PID/nature generation. */
static uint32_t xs32_next(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x ? x : 0x1u;
    return *s;
}

/* Generate a PID that:
 *  - has the requested gender (matching species gender ratio threshold)
 *  - has the requested nature (PID % 25 == nature)
 *  - is NOT shiny (since HGSS forces shiny=0 on walker claim)
 * Re-rolls until conditions met. */
static uint32_t generate_pid(uint32_t *rng, uint16_t species, uint8_t sex,
                             uint8_t nature, uint16_t tid, uint16_t sid)
{
    uint8_t ratio = personal_gender_ratio(species);

    /* If the requested sex is unreachable from this species's gender
     * ratio, force sex to whatever the ratio dictates so the loop below
     * doesn't spin uselessly before falling back to a constraint-free PID. */
    if (ratio == 0xFF && sex != 2) sex = 2;          /* genderless */
    else if (ratio == 0xFE && sex != 1) sex = 1;     /* always female */
    else if (ratio == 0x00 && sex != 0) sex = 0;     /* always male */

    for (int tries = 0; tries < 100000; tries++) {
        uint32_t pid = xs32_next(rng);
        /* shiny check */
        uint16_t lo = (uint16_t)(pid & 0xFFFF);
        uint16_t hi = (uint16_t)(pid >> 16);
        if ((lo ^ hi ^ tid ^ sid) < 8) continue;  /* skip shiny */
        /* nature check */
        if ((pid % 25) != nature) continue;
        /* gender check */
        uint8_t pid_byte = pid & 0xFF;
        uint8_t got_sex;
        if (ratio == 0xFF) got_sex = 2;
        else if (ratio == 0xFE) got_sex = 1;
        else if (ratio == 0x00) got_sex = 0;
        else got_sex = (pid_byte < ratio) ? 1 : 0;
        if (got_sex != sex) continue;
        return pid;
    }
    /* fallback: any non-zero PID, accept whatever */
    return xs32_next(rng) | 1;
}

/* Compute EXP for a given (species, level). */
static uint32_t species_exp_for_level(uint16_t species, uint8_t level) {
    return exp_for_level(personal_growth_rate(species), level);
}

/* Encode an ASCII string into gen4 wide chars (uppercase), terminated
 * with 0xFFFF and 0xFFFF-padded. Gen 4 species names are stored uppercase
 * ("PIKACHU") and the nickname field bytes are shown as-is (no auto-
 * substitution of the species name at display time), so the default
 * nickname for non-nicknamed Pokemon is written uppercase to match. */
static void encode_gen4_nickname_upper(const char *src, uint8_t *dst,
                                       size_t cap_bytes)
{
    if (cap_bytes < 2) return;
    size_t max_chars = (cap_bytes - 2) / 2;     /* leave room for 0xFFFF */
    size_t i = 0;
    while (src && src[i] && i < max_chars) {
        unsigned char ch = (unsigned char)src[i];
        char up = (char)toupper(ch);
        uint16_t code = gen4_from_ascii(up);
        if (code == 0) code = gen4_from_ascii(' ');
        dst[i*2]     = (uint8_t)(code        & 0xFF);
        dst[i*2 + 1] = (uint8_t)((code >> 8) & 0xFF);
        i++;
    }
    /* 0xFFFF terminator + pad. */
    while ((i + 1) * 2 <= cap_bytes) {
        dst[i*2]     = 0xFF;
        dst[i*2 + 1] = 0xFF;
        i++;
    }
}

/* Synthesize a stored-format (encrypted) 136-byte PK4. */
static void synth_pk4_encrypted(const pweep_caught_t *c,
                                 uint16_t tid, uint16_t sid,
                                 const uint8_t ot_name16[16],
                                 uint8_t  ot_gender,
                                 uint8_t  ot_language,
                                 uint32_t pid,
                                 uint32_t exp_amount,
                                 uint8_t  ability_slot,
                                 uint8_t  pokeball,
                                 uint8_t  met_year, uint8_t met_month, uint8_t met_day,
                                 uint8_t  out_pk4[136])
{
    uint8_t pk[136];
    memset(pk, 0, 136);

    /* Header */
    pk[0x00] = (uint8_t)(pid       & 0xFF);
    pk[0x01] = (uint8_t)((pid >> 8) & 0xFF);
    pk[0x02] = (uint8_t)((pid >> 16) & 0xFF);
    pk[0x03] = (uint8_t)((pid >> 24) & 0xFF);
    /* unused [4..5] = 0; checksum [6..7] computed below */

    /* Block A: species, item, OT, EXP, friendship, ability, markings, lang */
    pk[0x08] = (uint8_t)(c->species       & 0xFF);
    pk[0x09] = (uint8_t)((c->species >> 8) & 0xFF);
    pk[0x0A] = (uint8_t)(c->held_item     & 0xFF);
    pk[0x0B] = (uint8_t)((c->held_item >> 8) & 0xFF);
    pk[0x0C] = (uint8_t)(tid & 0xFF);
    pk[0x0D] = (uint8_t)((tid >> 8) & 0xFF);
    pk[0x0E] = (uint8_t)(sid & 0xFF);
    pk[0x0F] = (uint8_t)((sid >> 8) & 0xFF);
    pk[0x10] = (uint8_t)(exp_amount & 0xFF);
    pk[0x11] = (uint8_t)((exp_amount >> 8) & 0xFF);
    pk[0x12] = (uint8_t)((exp_amount >> 16) & 0xFF);
    pk[0x13] = (uint8_t)((exp_amount >> 24) & 0xFF);
    pk[PK4_OFF_FRIENDSHIP] = 70;    /* base friendship; walk-up applied on return */
    pk[PK4_OFF_ABILITY]    = ability_slot;
    pk[PK4_OFF_MARKINGS]   = 0;
    pk[PK4_OFF_LANGUAGE]   = ot_language;

    /* Block B: moves */
    for (int i = 0; i < 4; i++) {
        pk[0x28 + i*2]     = (uint8_t)(c->move[i] & 0xFF);
        pk[0x28 + i*2 + 1] = (uint8_t)((c->move[i] >> 8) & 0xFF);
        /* PP: leave 0 — game will fill from waza data on first load */
    }
    /* IV bits at +0x38..+0x3B (random — for now leave zero meaning all 0 IVs;
     * not ideal but valid). Bit 30 = isEgg, bit 31 = isNicknamed. */
    pk[0x38] = 0;
    pk[0x39] = 0;
    pk[0x3A] = 0;
    pk[0x3B] = 0;
    /* isNicknamed = 0 → game uses species name as nickname */

    /* Block C: nickname at +0x48. The nickname field bytes are what the
     * box / status screen renders (the isNicknamed flag does not trigger a
     * fall back to the species name), so write the uppercase species name
     * as the default. */
    {
        const char *sn = species_name(c->species);
        if (!sn) sn = "?";
        encode_gen4_nickname_upper(sn, pk + PK4_OFF_NICKNAME,
                                   PK4_OFF_NICKNAME_LEN);
    }
    /* origin_game at +0x5F: 10 (HG) or 11 (SS). Caller patches via sav. */
    pk[0x5F] = 10;

    /* Block D: OT name at +0x68, 16 bytes (gen4 chars, FFFF-terminated) */
    if (ot_name16) memcpy(pk + PK4_OFF_OT_NAME, ot_name16, PK4_OFF_OT_NAME_LEN);

    /* met date (egg date left at 0 since walker catches aren't eggs) */
    pk[PK4_OFF_MET_DATE + 0] = met_year;
    pk[PK4_OFF_MET_DATE + 1] = met_month;
    pk[PK4_OFF_MET_DATE + 2] = met_day;

    /* met location u16 LE */
    pk[PK4_OFF_MET_LOCATION    ] = (uint8_t)(MET_LOC_POKEWALKER & 0xFF);
    pk[PK4_OFF_MET_LOCATION + 1] = (uint8_t)((MET_LOC_POKEWALKER >> 8) & 0xFF);

    /* pokerus (0), ball, met level + OT gender, encounter type */
    pk[PK4_OFF_POKERUS]      = 0;
    pk[PK4_OFF_BALL]         = pokeball;
    pk[PK4_OFF_MET_LVL_GEND] = (uint8_t)((c->level & 0x7F) | ((ot_gender & 1) << 7));
    pk[PK4_OFF_ENCOUNTER]    = 0;  /* "default" / event-style */

    /* Level cache (+0x8C) is party-only; stored form has none. */

    /* Compute checksum (sum of u16s at 0x08..0x87) */
    uint16_t chk = pk4_compute_checksum(pk);
    pk[0x06] = (uint8_t)(chk & 0xFF);
    pk[0x07] = (uint8_t)((chk >> 8) & 0xFF);

    /* Encrypt (shuffle + XOR) */
    pk4_encrypt(pk);
    memcpy(out_pk4, pk, 136);
}

bool pweep_claim_into_sav(pweep_t *pweep, hgss_sav_t *sav,
                          uint32_t prng_seed, claim_result_t *result)
{
    if (!pweep || !pweep->loaded || !sav || !sav->loaded) return false;
    if (!result) return false;
    memset(result, 0, sizeof(*result));

    /* PRNG seed: caller passes prng_seed != 0 for deterministic runs
     * (test fixtures), 0 for a fresh per-claim seed. We use the 3DS RTC
     * via osGetTime() — milliseconds since 1900, always non-zero on a
     * booted console, so no further fallback is needed. */
    uint32_t rng;
    if (prng_seed) {
        rng = prng_seed;
    } else {
        rng = (uint32_t)(osGetTime() & 0xFFFFFFFFu);
    }
    result->prng_seed_used = rng;

    uint16_t tid = hgss_tid(sav);
    uint16_t sid = hgss_sid(sav);
    uint8_t  ot_name[HGSS_OT_NAME_LEN];
    hgss_ot_name(sav, ot_name);
    uint8_t  ot_gender   = hgss_ot_gender(sav);
    uint8_t  ot_language = hgss_ot_language(sav);

    /* Met date: today, from the system clock. */
    time_t t = time(NULL);
    struct tm *now = localtime(&t);
    uint8_t my = (uint8_t)(now ? (now->tm_year - 100) : 26);  /* 2026 - 2000 */
    uint8_t mm = (uint8_t)(now ? (now->tm_mon + 1) : 1);
    uint8_t md = (uint8_t)(now ? now->tm_mday : 1);

    for (int slot = 0; slot < PWEEP_CAUGHT_SLOT_COUNT; slot++) {
        pweep_caught_t c;
        if (!pweep_caught_get(pweep, slot, &c) || c.species == 0) continue;

        /* Generate nature/ability/pid */
        uint8_t nature = (uint8_t)(xs32_next(&rng) % 25);
        uint8_t ability_slot = (uint8_t)(nature & 1);
        uint32_t pid = generate_pid(&rng, c.species, c.sex, nature, tid, sid);

        /* EXP for the recorded level */
        uint32_t exp_amount = species_exp_for_level(c.species, c.level);

        /* Synthesize encrypted PK4 */
        uint8_t pk4[136];
        synth_pk4_encrypted(&c, tid, sid, ot_name, ot_gender, ot_language,
                            pid, exp_amount, ability_slot, DEFAULT_POKEBALL,
                            my, mm, md, pk4);

        /* Find first empty slot in HGSS storage. */
        int box, slot_in_box;
        if (!hgss_find_first_empty_slot(sav, &box, &slot_in_box)) {
            result->pokemon_skipped++;
            continue;
        }
        uint8_t *dst = hgss_box_slot(sav, box, slot_in_box);
        if (!dst) { result->pokemon_skipped++; continue; }
        memcpy(dst, pk4, 136);
        result->pokemon_claimed++;
    }

    /* Sync walker stats. */
    pweep_walker_stats_t ws;
    if (pweep_walker_stats_get(pweep, &ws)) {
        result->walker_steps_before = hgss_walker_steps(sav);
        /* Per HGSS _step_calc semantics: replace (don't add). If walker total
         * is lower than saved (counter rollover), keep old. */
        if (ws.total_steps >= result->walker_steps_before) {
            hgss_set_walker_steps(sav, ws.total_steps);
        }
        result->walker_steps_after = hgss_walker_steps(sav);

        /* Watts: ADD walker session amount, cap at 9_999_999. */
        uint32_t old_watts = hgss_walker_watts(sav);
        uint64_t new_watts = (uint64_t)old_watts + ws.total_watts;
        if (new_watts > 9999999) new_watts = 9999999;
        hgss_set_walker_watts(sav, (uint32_t)new_watts);
        result->walker_watts_added = ws.total_watts;

        /* Zero the walker's CUR_WATTS field after transferring to the sav,
         * matching the real IR flow where HGSS acking the claim triggers
         * endWalkAction on the walker (which resets watts to 0). Without
         * this, running Claim twice would credit the same watts twice.
         * Steps are NOT zeroed: hgss_set_walker_steps uses replace
         * semantics (line above), so re-running is idempotent for steps. */
        pweep_set_watts(pweep, 0);
    }

    /* Dowsing item finds.
     *
     * The walker stores up to 3 dowsing finds in the 12-byte buffer
     * at EEP 0xCEBC (3 x {u16 item_id LE, u16 padding=0}), written when
     * the player accepts an item-find pop-up. On a real IR sync HGSS
     * adds each item to the bag; here each entry is routed into the
     * appropriate HGSS bag (Items / Medicine / Berries / Balls / TMHM,
     * classified by ID range) and the buffer is cleared afterwards. */
    for (int i = 0; i < CLAIM_MAX_ITEMS; i++) {
        uint16_t id = pweep_dowsing_get_at(pweep, i);
        if (id == 0) continue;
        hgss_bag_kind_t bag = gen4_item_bag(id);
        bool added = hgss_bag_add(sav, bag, id, 1);
        result->items[i].item_id = id;
        result->items[i].bag     = (uint8_t)bag;
        result->items[i].added   = added;
        result->items_claimed++;
        if (added) result->items_added++;
    }
    /* Clear the walker's dowsing buffer unconditionally — even if the
     * HGSS bag was full for some entries, we don't want them to be
     * re-claimed next sync (the walker firmware would treat them as
     * already-acked once the IR session completes). The user can fix
     * a full bag in-game and replay if needed. */
    pweep_dowsing_clear(pweep);

    /* After claim, wipe the caught Pokemon slots — they've been
     * deposited in HGSS boxes, so the walker can start fresh. The daily
     * step history at 0xCEF0 is left untouched. The pairing's has_pokemon
     * flag stays set because the walking Pokemon is still attached —
     * Return handles that side. */
    pweep_caught_clear_all(pweep);

    return true;
}

/* ===================================================================== */
/* return: take WalkerPair PK4, apply bonuses, deposit, clear            */
/* ===================================================================== */

bool pweep_return_sent(pweep_t *pweep, hgss_sav_t *sav,
                       uint8_t friendship_bonus, uint32_t exp_bonus,
                       return_result_t *result)
{
    if (!sav || !sav->loaded || !result) return false;
    memset(result, 0, sizeof(*result));

    uint8_t *wp = hgss_walker_pair(sav);
    /* Empty WalkerPair: first 8 bytes (PID + unused + checksum) all 0. */
    bool empty = true;
    for (int i = 0; i < 8; i++) if (wp[i] != 0) { empty = false; break; }
    if (empty) {
        /* Nothing to return — but if the walker still believes it has
         * a Pokemon attached, that's a desync. Detach it so the next
         * Send isn't blocked by the pairing guard. */
        pweep_detach_pokemon(pweep);
        /* Same desync on the HGSS side: if the game still thinks a Pokemon
         * is out (DEPOSIT) but the walker has none, drop it to NO_DEPSIT. */
        if (hgss_walker_deposit_flag(sav) == (uint16_t)HGSS_PHC_DEPOSIT)
            hgss_set_walker_deposit(sav, (uint16_t)HGSS_PHC_NO_DEPSIT, 0);
        return true;
    }

    /* Decrypt WalkerPair PK4. */
    uint8_t pk[136];
    memcpy(pk, wp, 136);
    pk4_decrypt(pk);

    uint16_t species = (uint16_t)pk[0x08] | ((uint16_t)pk[0x09] << 8);
    result->species = species;

    /* Current friendship + EXP. */
    uint8_t fr_old = pk[0x14];
    uint32_t exp_old = (uint32_t)pk[0x10] | ((uint32_t)pk[0x11] << 8)
                     | ((uint32_t)pk[0x12] << 16) | ((uint32_t)pk[0x13] << 24);
    uint8_t growth = personal_growth_rate(species);
    uint8_t lvl_old = level_from_exp(growth, exp_old);
    result->friendship_before = fr_old;
    result->level_before = lvl_old;

    /* Apply bonuses (clamped). */
    uint32_t fr_new = (uint32_t)fr_old + friendship_bonus;
    if (fr_new > 255) fr_new = 255;
    pk[0x14] = (uint8_t)fr_new;

    /* EXP: add, clamp at level 100 EXP. */
    uint32_t exp_max = exp_for_level(growth, 100);
    uint32_t exp_new = exp_old + exp_bonus;
    if (exp_new > exp_max) exp_new = exp_max;
    pk[0x10] = (uint8_t)(exp_new & 0xFF);
    pk[0x11] = (uint8_t)((exp_new >> 8) & 0xFF);
    pk[0x12] = (uint8_t)((exp_new >> 16) & 0xFF);
    pk[0x13] = (uint8_t)((exp_new >> 24) & 0xFF);

    /* Recompute checksum (changed friendship + EXP). */
    pk[0x06] = 0; pk[0x07] = 0;  /* reset before recompute */
    uint16_t chk = pk4_compute_checksum(pk);
    pk[0x06] = (uint8_t)(chk & 0xFF);
    pk[0x07] = (uint8_t)((chk >> 8) & 0xFF);

    result->friendship_after = pk[0x14];
    result->level_after = level_from_exp(growth, exp_new);

    /* Re-encrypt. */
    pk4_encrypt(pk);

    /* Find first empty box slot and write. */
    int box, slot;
    if (!hgss_find_first_empty_slot(sav, &box, &slot)) {
        /* No space — leave WalkerPair intact, abort. */
        return false;
    }
    uint8_t *dst = hgss_box_slot(sav, box, slot);
    if (!dst) return false;
    memcpy(dst, pk, 136);
    result->box = box;
    result->slot = slot;
    result->returned = true;

    /* Clear WalkerPair (zero the PK4 region — 0..0x87) and keep the
     * identity / counter / watts tail. Only the BoxPokemon (first 136 B)
     * gets zeroed; identity[10] at +0xF8 and downstream fields are
     * persistent walker state. */
    memset(wp, 0, 0x88);

    /* The Pokemon came back: drop the HGSS deposit flag from DEPOSIT to
     * NO_DEPSIT (still linked, nothing out). Mirror of the send-side set;
     * without this the game would still believe a Pokemon is out walking. */
    hgss_set_walker_deposit(sav, (uint16_t)HGSS_PHC_NO_DEPSIT, 0);

    /* Detach the walking Pokemon on the pweep side. Caught Pokemon
     * were already wiped by pweep_claim_into_sav; we do not touch
     * the daily step history, the per-route blocks, or any pairing
     * field beyond has_pokemon — preserving the user's walk data and
     * the walker<->save pairing identity. */
    pweep_detach_pokemon(pweep);

    return true;
}
