/*
 * pweep.c - Pokewalker EEPROM (M95512, 64 KB) parser/editor.
 * See pweep.h for offset documentation.
 */

#include "pweep.h"
#include "course_data.h"
#include "personal_gen4.h"
#include "exp_curve.h"
#include "pk4.h"
#include "route_tail.h"

#include <stdio.h>
#include <string.h>

/* ===================================================================== */

static uint8_t reliable_checksum(const uint8_t *data, uint16_t size) {
    uint8_t s = 0x01;
    for (uint16_t i = 0; i < size; i++) s += data[i];
    return s;
}

static inline uint16_t addrpair_lo(uint32_t pair) { return (uint16_t)pair; }
static inline uint16_t addrpair_hi(uint32_t pair) { return (uint16_t)(pair >> 16); }

/* ===================================================================== */

bool pweep_load(pweep_t *p, const char *path) {
    memset(p, 0, sizeof(*p));
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    size_t got = fread(p->data, 1, PWEEP_SIZE, f);
    int eof_or_err = fgetc(f);  /* must be EOF for size to be exact */
    fclose(f);
    if (got != PWEEP_SIZE || eof_or_err != EOF) return false;
    p->loaded = true;
    return true;
}

bool pweep_save(pweep_t *p, const char *path) {
    /* Recompute every known reliable checksum so callers who edited raw
     * `data` directly still produce a consistent file. */
    static const struct { uint32_t pair; uint16_t size; } pairs[] = {
        { PWEEP_REL_PAIRING,       PWEEP_REL_PAIRING_SIZE },
        { PWEEP_REL_HEALTH,        PWEEP_REL_HEALTH_SIZE },
        { PWEEP_REL_WALK_SENTINEL, PWEEP_REL_WALK_SENTINEL_SZ },
        { PWEEP_REL_WALK_IDENTITY, PWEEP_REL_WALK_IDENT_SIZE },
        { PWEEP_REL_LCD_INIT,      PWEEP_REL_LCD_INIT_SIZE },
    };
    for (size_t i = 0; i < sizeof(pairs)/sizeof(pairs[0]); i++) {
        uint16_t a0 = addrpair_lo(pairs[i].pair);
        uint16_t a1 = addrpair_hi(pairs[i].pair);
        uint16_t sz = pairs[i].size;
        /* Take copy 0 as authoritative, mirror it to copy 1 + checksums. */
        uint8_t cs = reliable_checksum(p->data + a0, sz);
        memcpy(p->data + a1, p->data + a0, sz);
        p->data[a0 + sz] = cs;
        p->data[a1 + sz] = cs;
    }

    /* Atomic-ish write: tmp + rename. */
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return false;
    size_t wrote = fwrite(p->data, 1, PWEEP_SIZE, f);
    int rc_close = fclose(f);
    if (wrote != PWEEP_SIZE || rc_close != 0) { remove(tmp); return false; }
    /* POSIX rename(2) overwrites the destination on Linux/3DS-sdmc but
     * NOT on stock Windows (mingw). Remove existing destination first so
     * the rename succeeds across platforms. */
    remove(path);
    if (rename(tmp, path) != 0) { remove(tmp); return false; }
    return true;
}

/* ===================================================================== */

static bool all_ff(const uint8_t *p, uint16_t n) {
    for (uint16_t i = 0; i < n; i++) if (p[i] != 0xFF) return false;
    return true;
}

static pweep_status_t reliable_status_one(const pweep_t *p, uint32_t pair,
                                          uint16_t size) {
    uint16_t a0 = addrpair_lo(pair), a1 = addrpair_hi(pair);
    /* Whole "data + checksum byte" region 0xFF on both copies = factory
     * uninitialized. Firmware (readReliableDataFromEeprom case 0) treats
     * this as default and continues; it's not corruption. */
    bool uninit0 = all_ff(p->data + a0, size + 1);
    bool uninit1 = all_ff(p->data + a1, size + 1);
    if (uninit0 && uninit1) return PWEEP_UNINIT;

    uint8_t cs0 = reliable_checksum(p->data + a0, size);
    uint8_t cs1 = reliable_checksum(p->data + a1, size);
    bool ok0 = (cs0 == p->data[a0 + size]);
    bool ok1 = (cs1 == p->data[a1 + size]);
    return (ok0 && ok1) ? PWEEP_OK : PWEEP_BAD;
}

bool pweep_validate(pweep_t *p) {
    p->pairing       = reliable_status_one(p, PWEEP_REL_PAIRING,       PWEEP_REL_PAIRING_SIZE);
    p->health        = reliable_status_one(p, PWEEP_REL_HEALTH,        PWEEP_REL_HEALTH_SIZE);
    p->walk_sentinel = reliable_status_one(p, PWEEP_REL_WALK_SENTINEL, PWEEP_REL_WALK_SENTINEL_SZ);
    p->walk_identity = reliable_status_one(p, PWEEP_REL_WALK_IDENTITY, PWEEP_REL_WALK_IDENT_SIZE);
    p->lcd_init      = reliable_status_one(p, PWEEP_REL_LCD_INIT,      PWEEP_REL_LCD_INIT_SIZE);
    /* "Validates" = OK or UNINIT. Firmware tolerates UNINIT, so we do too. */
    return p->pairing       != PWEEP_BAD &&
           p->health        != PWEEP_BAD &&
           p->walk_sentinel != PWEEP_BAD &&
           p->walk_identity != PWEEP_BAD &&
           p->lcd_init      != PWEEP_BAD;
}

/* ===================================================================== */

bool pweep_read_reliable(const pweep_t *p, uint32_t addrPair,
                         uint16_t size, uint8_t *out) {
    uint16_t a0 = addrpair_lo(addrPair), a1 = addrpair_hi(addrPair);
    uint8_t cs0 = reliable_checksum(p->data + a0, size);
    uint8_t cs1 = reliable_checksum(p->data + a1, size);
    bool ok0 = (cs0 == p->data[a0 + size]);
    bool ok1 = (cs1 == p->data[a1 + size]);

    if (ok0)      memcpy(out, p->data + a0, size);
    else if (ok1) memcpy(out, p->data + a1, size);
    else          return false;
    return true;
}

void pweep_write_reliable(pweep_t *p, uint32_t addrPair,
                          uint16_t size, const uint8_t *in) {
    uint16_t a0 = addrpair_lo(addrPair), a1 = addrpair_hi(addrPair);
    memcpy(p->data + a0, in, size);
    memcpy(p->data + a1, in, size);
    uint8_t cs = reliable_checksum(in, size);
    p->data[a0 + size] = cs;
    p->data[a1 + size] = cs;
}

/* ===================================================================== */
/* High-level helpers                                                    */
/* ===================================================================== */

bool pweep_has_pokemon(const pweep_t *p) {
    uint8_t pairing[PWEEP_REL_PAIRING_SIZE];
    if (!pweep_read_reliable(p, PWEEP_REL_PAIRING, PWEEP_REL_PAIRING_SIZE, pairing))
        return false;
    return (pairing[PAIRING_OFF_FLAGS] & PAIRING_FLAG_HAS_POKEMON) != 0;
}

void pweep_set_has_pokemon(pweep_t *p, bool yes) {
    uint8_t pairing[PWEEP_REL_PAIRING_SIZE];
    if (!pweep_read_reliable(p, PWEEP_REL_PAIRING, PWEEP_REL_PAIRING_SIZE, pairing)) {
        /* Both copies have bad checksums — silently raw-reading copy 0
         * and re-signing it would commit identity garbage with a fresh
         * cs (TID/SID/OT name live in this block). No-op instead so
         * callers see the flag unchanged and the UI can flag it. */
        return;
    }
    if (yes) pairing[PAIRING_OFF_FLAGS] |= PAIRING_FLAG_HAS_POKEMON;
    else     pairing[PAIRING_OFF_FLAGS] &= (uint8_t)~PAIRING_FLAG_HAS_POKEMON;
    pweep_write_reliable(p, PWEEP_REL_PAIRING, PWEEP_REL_PAIRING_SIZE, pairing);
}

uint16_t pweep_get_watts(const pweep_t *p) {
    uint8_t h[PWEEP_REL_HEALTH_SIZE];
    if (!pweep_read_reliable(p, PWEEP_REL_HEALTH, PWEEP_REL_HEALTH_SIZE, h))
        return 0;
    return pweep_rd16be(h + HEALTH_OFF_CUR_WATTS);
}

void pweep_set_watts(pweep_t *p, uint16_t w) {
    if (w > 9999) w = 9999;
    uint8_t h[PWEEP_REL_HEALTH_SIZE];
    if (!pweep_read_reliable(p, PWEEP_REL_HEALTH, PWEEP_REL_HEALTH_SIZE, h)) {
        /* Both HealthData copies corrupt — refuse to write so we don't
         * commit garbage step/watts/day counters with a fresh checksum. */
        return;
    }
    pweep_wr16be(h + HEALTH_OFF_CUR_WATTS, w);
    pweep_write_reliable(p, PWEEP_REL_HEALTH, PWEEP_REL_HEALTH_SIZE, h);
}

uint32_t pweep_get_total_steps(const pweep_t *p) {
    uint8_t h[PWEEP_REL_HEALTH_SIZE];
    if (!pweep_read_reliable(p, PWEEP_REL_HEALTH, PWEEP_REL_HEALTH_SIZE, h))
        return 0;
    return pweep_rd32be(h + HEALTH_OFF_TOTAL_STEPS);
}

uint16_t pweep_get_total_days(const pweep_t *p) {
    uint8_t h[PWEEP_REL_HEALTH_SIZE];
    if (!pweep_read_reliable(p, PWEEP_REL_HEALTH, PWEEP_REL_HEALTH_SIZE, h))
        return 0;
    return pweep_rd16be(h + HEALTH_OFF_TOTAL_DAYS);
}

const uint8_t *pweep_walk_data(const pweep_t *p) {
    return p->data + PWEEP_OFF_WALK_DATA;
}

uint8_t *pweep_walk_data_mut(pweep_t *p) {
    return p->data + PWEEP_OFF_WALK_DATA;
}

void pweep_walk_data_commit(pweep_t *p) {
    /* The firmware promotes the 0xD700 staging buffer to the live
     * walk_data at 0x8F00 (0x2900 bytes) at walk start. Mirror the live
     * walk_data back into the 0xD700 backup so that promotion is a no-op. */
    memcpy(p->data + PWEEP_OFF_TEAM_BACKUP,
           p->data + PWEEP_OFF_WALK_DATA,
           PWEEP_WALK_DATA_SIZE);
    /* 0xD700+0xBE..end of the 0x2900 staging region is left untouched. */
}

const uint8_t *pweep_caught_pokemon(const pweep_t *p) {
    return p->data + PWEEP_OFF_CAUGHT_POKEMON;
}

void pweep_clear_caught_pokemon(pweep_t *p) {
    memset(p->data + PWEEP_OFF_CAUGHT_POKEMON, 0, PWEEP_CAUGHT_POKEMON_SIZE);
}

bool pweep_read_pairing(const pweep_t *p, uint8_t out[PWEEP_REL_PAIRING_SIZE]) {
    return pweep_read_reliable(p, PWEEP_REL_PAIRING, PWEEP_REL_PAIRING_SIZE, out);
}

void pweep_write_pairing(pweep_t *p, const uint8_t in[PWEEP_REL_PAIRING_SIZE]) {
    pweep_write_reliable(p, PWEEP_REL_PAIRING, PWEEP_REL_PAIRING_SIZE, in);
}

/* PHCPokemonBase.sex value: 0=male, 1=female, 2=genderless.
 * Derived from the species gender ratio (gen-4 algorithm); see
 * personal_gen4.h for the ratio table. */
static uint8_t compute_pkmn_sex(uint16_t species, uint32_t pid) {
    uint8_t ratio = personal_gender_ratio(species);
    if (ratio == 0xFF) return 2;
    if (ratio == 0xFE) return 1;
    if (ratio == 0x00) return 0;
    return ((pid & 0xFF) < ratio) ? 1 : 0;
}

/* ===================================================================== */
/* Caught Pokemon                                                        */
/* ===================================================================== */

static uint16_t rd16le(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }

bool pweep_caught_get(const pweep_t *p, int slot, pweep_caught_t *out) {
    if (!p->loaded || slot < 0 || slot >= PWEEP_CAUGHT_SLOT_COUNT || !out) return false;
    const uint8_t *s = p->data + PWEEP_OFF_CAUGHT_POKEMON + slot * PWEEP_CAUGHT_SLOT_SIZE;
    out->species   = rd16le(s + 0x00);
    out->held_item = rd16le(s + 0x02);
    for (int i = 0; i < 4; i++) out->move[i] = rd16le(s + 0x04 + i*2);
    out->level     = s[0x0C];
    uint8_t bf1    = s[0x0D];
    out->folm      = bf1 & 0x1F;
    out->sex       = (bf1 >> 5) & 0x03;
    out->is_egg    = (bf1 >> 7) & 0x01;
    uint8_t bf2    = s[0x0E];
    out->reverse_flag = bf2 & 0x01;
    out->rare_color   = (bf2 >> 1) & 0x01;
    return true;
}

bool pweep_caught_is_empty(const pweep_t *p, int slot) {
    pweep_caught_t c;
    if (!pweep_caught_get(p, slot, &c)) return true;
    return c.species == 0;
}

int pweep_caught_count(const pweep_t *p) {
    int n = 0;
    for (int i = 0; i < PWEEP_CAUGHT_SLOT_COUNT; i++)
        if (!pweep_caught_is_empty(p, i)) n++;
    return n;
}

void pweep_caught_clear_all(pweep_t *p) {
    if (!p->loaded) return;
    memset(p->data + PWEEP_OFF_CAUGHT_POKEMON, 0, PWEEP_CAUGHT_POKEMON_SIZE);
}

/* ===================================================================== */
/* Dowsing finds buffer                                                  */
/* ===================================================================== */

uint16_t pweep_dowsing_get_at(const pweep_t *p, int slot) {
    if (!p || !p->loaded) return 0;
    if (slot < 0 || slot >= PWEEP_DOWSING_SLOT_COUNT) return 0;
    const uint8_t *s = p->data + PWEEP_OFF_DOWSING_ITEMS
                         + slot * PWEEP_DOWSING_SLOT_STRIDE;
    /* item_id is a 16-bit little-endian value at byte 0..1 of each slot. */
    return (uint16_t)s[0] | ((uint16_t)s[1] << 8);
}

int pweep_dowsing_count(const pweep_t *p) {
    int n = 0;
    for (int i = 0; i < PWEEP_DOWSING_SLOT_COUNT; i++) {
        if (pweep_dowsing_get_at(p, i) != 0) n++;
    }
    return n;
}

void pweep_dowsing_clear(pweep_t *p) {
    if (!p || !p->loaded) return;
    memset(p->data + PWEEP_OFF_DOWSING_ITEMS, 0, PWEEP_DOWSING_ITEMS_SIZE);
}

void pweep_detach_pokemon(pweep_t *p) {
    if (!p->loaded) return;
    uint8_t pairing[PWEEP_REL_PAIRING_SIZE];
    if (pweep_read_pairing(p, pairing)) {
        pairing[PAIRING_OFF_FLAGS] &= (uint8_t)~PAIRING_FLAG_HAS_POKEMON;
        pweep_write_pairing(p, pairing);
    }
    uint8_t *wd = pweep_walk_data_mut(p);
    memset(wd, 0, WALKDATA_OFF_COURSE_ID);
}

bool pweep_walker_stats_get(const pweep_t *p, pweep_walker_stats_t *out) {
    if (!p->loaded || !out) return false;
    uint8_t h[PWEEP_REL_HEALTH_SIZE];
    if (!pweep_read_reliable(p, PWEEP_REL_HEALTH, PWEEP_REL_HEALTH_SIZE, h))
        return false;
    out->total_steps = pweep_rd32be(h + HEALTH_OFF_TOTAL_STEPS);
    out->total_days  = pweep_rd16be(h + HEALTH_OFF_TOTAL_DAYS);
    out->total_watts = pweep_rd16be(h + HEALTH_OFF_CUR_WATTS);
    return true;
}

/* Compute level from PK4 EXP using the species growth curve.
 * PK4 EXP is at +0x10, u32 LE. */
uint8_t pweep_pk4_level(const uint8_t *pk4_dec) {
    uint16_t species = (uint16_t)pk4_dec[0x08] | ((uint16_t)pk4_dec[0x09] << 8);
    uint32_t exp = (uint32_t)pk4_dec[0x10]
                 | ((uint32_t)pk4_dec[0x11] << 8)
                 | ((uint32_t)pk4_dec[0x12] << 16)
                 | ((uint32_t)pk4_dec[0x13] << 24);
    return level_from_exp(personal_growth_rate(species), exp);
}

/* ===================================================================== */
/* send: write a PK4 Pokemon into walker walk_data + flip pairing flags. */
/* ===================================================================== */

bool pweep_send_pokemon(pweep_t *p, const uint8_t *pk4_dec,
                        uint8_t level, uint8_t route_id) {
    if (!p->loaded) return false;
    uint8_t *wd = pweep_walk_data_mut(p);

    /* --- Pokemon-specific bytes (walk_data 0x00..0x27) --- */
    /* Zero this region first; preserve 0x28..0xBD route/encounter data. */
    memset(wd, 0, 0x28);

    /* species: PK4 +0x08 (u16 LE).
     * Held item is intentionally zeroed: the walker discards the held
     * item on send, so walk_data keeps held=0 to match. */
    memcpy(wd + WALKDATA_OFF_SPECIES, pk4_dec + 0x08, 2);
    /* wd[OFF_HELD_ITEM] = 0 (already zero from memset above) */

    /* 4 moves: PK4 block B starts at +0x28, moves are 4 x u16 LE. */
    memcpy(wd + WALKDATA_OFF_MOVE0, pk4_dec + 0x28, 2);
    memcpy(wd + WALKDATA_OFF_MOVE1, pk4_dec + 0x2A, 2);
    memcpy(wd + WALKDATA_OFF_MOVE2, pk4_dec + 0x2C, 2);
    memcpy(wd + WALKDATA_OFF_MOVE3, pk4_dec + 0x2E, 2);

    wd[WALKDATA_OFF_LEVEL] = level;

    /* Extract PK4 fields needed for flag computation. */
    uint16_t species = (uint16_t)pk4_dec[0x08] | ((uint16_t)pk4_dec[0x09] << 8);
    uint32_t pid = (uint32_t)pk4_dec[0x00]
                 | ((uint32_t)pk4_dec[0x01] << 8)
                 | ((uint32_t)pk4_dec[0x02] << 16)
                 | ((uint32_t)pk4_dec[0x03] << 24);
    uint16_t tid = (uint16_t)pk4_dec[0x0C] | ((uint16_t)pk4_dec[0x0D] << 8);
    uint16_t sid = (uint16_t)pk4_dec[0x0E] | ((uint16_t)pk4_dec[0x0F] << 8);

    /* Byte 0x0D = folm(5) | sex(2) | isEgg(1). Form lives in PK4 byte
     * 0x40 bits 3-7. Sending the right form is what makes Rotom-Wash
     * render as Rotom-Wash on the walker LCD instead of plain Rotom —
     * same for Wormadam/Shellos/Castform/etc. */
    uint8_t sex = compute_pkmn_sex(species, pid);
    uint8_t folm = pk4_form(pk4_dec);
    uint8_t is_egg = 0; /* HGSS never sends eggs to the walker. */
    wd[WALKDATA_OFF_FOLM_SEX_EGG] =
        (uint8_t)((folm & 0x1F) | ((sex & 0x03) << 5) | ((is_egg & 0x01) << 7));

    /* Byte 0x0E = reverseFlag(bit0) | rareColor(bit1) | padding.
     * Shiny: ((pid_lo ^ pid_hi ^ tid ^ sid) < 8). */
    uint16_t pid_lo = (uint16_t)(pid & 0xFFFF);
    uint16_t pid_hi = (uint16_t)(pid >> 16);
    bool shiny = ((pid_lo ^ pid_hi ^ tid ^ sid) < 8);
    wd[WALKDATA_OFF_REVERSE_SHINY] = shiny ? WALKDATA_FLAG_SHINY : 0x00;
    /* wd[0x0F] left zero by memset */

    /* Nickname: 22 B verbatim from PK4 block C (offset 0x48), with a
     * tail fix-up: the walker forces the LAST 2 bytes of the nickname
     * buffer to 0xFFFF even when the PK4 source has 0x0000 padding there.
     * Normalize to match the walker's on-disk form. */
    memcpy(wd + WALKDATA_OFF_NICKNAME,
           pk4_dec + 0x48, WALKDATA_NICKNAME_SIZE);
    wd[WALKDATA_OFF_NICKNAME + WALKDATA_NICKNAME_SIZE - 2] = 0xFF;
    wd[WALKDATA_OFF_NICKNAME + WALKDATA_NICKNAME_SIZE - 1] = 0xFF;

    wd[WALKDATA_OFF_FRIENDSHIP] = pk4_dec[0x14];
    /* wd[0x27] left zero */

    /* --- Pairing flags (offsets within the 0x68-byte block) --- */
    uint8_t pairing[PWEEP_REL_PAIRING_SIZE];
    if (!pweep_read_pairing(p, pairing)) {
        /* Both pairing copies have bad checksums. Falling back to raw
         * copy 0 + re-sign would commit a 104-byte garbage block with a
         * fresh-looking checksum, corrupting the identity bytes (TID/SID/
         * OT name) embedded in the pairing block. Abort instead and let
         * the UI surface a validation failure. The on-device pairing
         * repair needs at least one valid copy, so if both are bad we
         * can't safely write here either. */
        return false;
    }
    /* Bytes +0x04 (u32 BE session counter) and +0x0A (u16 BE sync nibble)
     * are bumped on each send. The exact firmware rule is unknown, so:
     *   - on a cold pairing block (all-zero session field), set the
     *     empirical first-send values;
     *   - otherwise, increment the session counter by 1 and leave the
     *     unk_0A nibble alone (the walker only checks for monotonicity,
     *     not exact equality).
     * This avoids regressing an established walker that already has a
     * high counter while still working on a fresh pweep. */
    uint32_t session = pweep_rd32be(pairing + PAIRING_OFF_SESSION);
    if (session == 0) {
        pweep_wr32be(pairing + PAIRING_OFF_SESSION, 1);
        pairing[PAIRING_OFF_UNK_0A]     = 0x00;
        pairing[PAIRING_OFF_UNK_0A + 1] = 0x08;
    } else {
        if (session < 0xFFFFFFFFu) session++;
        pweep_wr32be(pairing + PAIRING_OFF_SESSION, session);
        /* leave unk_0A as-is */
    }
    /* Walk-start flags: set paired (bit0) + hasPokemon (bit1), clear bit2 —
     * matching the firmware's startWalkEepromActions (id[0x5B] |= 3, &= ~4).
     * A real "out walking" pweep reads 0x03 here. */
    pairing[PAIRING_OFF_FLAGS] =
        (uint8_t)((pairing[PAIRING_OFF_FLAGS]
                   | PAIRING_FLAG_PAIRED | PAIRING_FLAG_HAS_POKEMON)
                  & ~PAIRING_FLAG_BIT2);
    pweep_write_pairing(p, pairing);

    /* --- Route ID byte at 0xCF90 --- */
    if (route_id != 0) {
        p->data[PWEEP_ROUTE_ID_OFF] = route_id;
    }

    /* --- Route tail (10240 B at 0x8FBE..0xB7FF): re-render the walker's
     * route tail blob with the chosen Pokemon (icon, big portrait,
     * nickname) AND the chosen course (background, enemy sprites + big
     * portrait + name glyphs). Anything that can't be regenerated is
     * preserved from the existing tail.
     *
     * Enemy slots come from course_defs: the walker firmware draws them
     * out of atlas slots indexed by encounter slot, NOT by species id, so
     * leaving them at the template values renders the wrong enemy sprites
     * even when the species table at +0x52 (sent over IR) is correct. */
    {
        route_tail_pokemon_t pk;
        pk.species   = species;
        pk.form      = pk4_form(pk4_dec);
        pk.sex       = sex;
        pk.shiny     = shiny;
        pk.nickname22 = pk4_dec + 0x48;
        pk.pid       = pid;

        route_tail_course_t cr;
        memset(&cr, 0, sizeof(cr));
        cr.course_id  = route_id;
        cr.graphic_id = 0;  /* 0 = preserve template's course bitmap */

        /* Look up the canonical course definition (1..27 -> course_def_t)
         * so we can hand the renderer the enemy species / form / sex per
         * slot. Without this every enemy field stays zero and route_tail
         * cannot index its atlases. */
        for (int i = 0; i < COURSE_COUNT; i++) {
            if (course_defs[i].course_id == route_id) {
                cr.graphic_id = course_defs[i].graphic_id;
                for (int e = 0; e < 3; e++) {
                    cr.enemy_species[e] = course_defs[i].enemies[e].species;
                    cr.enemy_sex[e]     = course_defs[i].enemies[e].sex;
                    cr.enemy_form[e]    = course_defs[i].enemies[e].form;
                }
                break;
            }
        }

        uint8_t *tail = p->data + 0x8FBE;
        uint8_t buf[ROUTE_TAIL_SIZE];
        if (route_tail_render(&pk, &cr, tail, buf)) {
            memcpy(tail, buf, ROUTE_TAIL_SIZE);
        }
    }

    return true;
}
