/*
 * hgss_sav.c - HGSS .sav parser/editor. See hgss_sav.h for offsets.
 */

#include "hgss_sav.h"

#include <stdio.h>
#include <string.h>

/* CRC16-CCITT (PKHeX byte-by-byte variant). Init = 0xFFFF.
 * Transcribed from PKHeX.Core/Saves/Util/Checksums.cs Checksums.CRC16_CCITT.
 * NOT the textbook polynomial form; this matches the in-game checksum
 * implementation exactly. */
uint16_t hgss_crc16_ccitt(const uint8_t *data, size_t n) {
    uint8_t top = 0xFF, bot = 0xFF;
    for (size_t i = 0; i < n; i++) {
        uint8_t x = data[i] ^ top;
        x ^= (uint8_t)(x >> 4);
        top = (uint8_t)(bot ^ (x >> 3) ^ (x << 4));
        bot = (uint8_t)(x ^ (x << 5));
    }
    return (uint16_t)((top << 8) | bot);
}

/* ===================================================================== */

static inline uint16_t rd16le(const uint8_t *p) { return p[0] | ((uint16_t)p[1] << 8); }
static inline uint32_t rd32le(const uint8_t *p) {
    return p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline void wr32le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static inline void wr16le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}

/* ===================================================================== */

static int compare_counters_part(const uint8_t *p1, const uint8_t *p2) {
    /* Mirrors SAV4BlockDetection.CompareFooters: major then minor (u32 LE).
     * Returns 0 (first) or 1 (second).
     * If counter==UINT_MAX and the other is < MAX-1, it's the OLD one
     * (rolled over). PKHeX has special-case logic; we replicate. */
    uint32_t maj1 = rd32le(p1), maj2 = rd32le(p2);
    if (maj1 == 0xFFFFFFFFu && maj2 != 0xFFFFFFFEu) return 1;
    if (maj2 == 0xFFFFFFFFu && maj1 != 0xFFFFFFFEu) return 0;
    if (maj1 > maj2) return 0;
    if (maj1 < maj2) return 1;
    /* Tied on major; check minor (u32 LE just after major). */
    uint32_t min1 = rd32le(p1 + 4), min2 = rd32le(p2 + 4);
    return (min2 > min1) ? 1 : 0;
}

/* For a block of length `block_len`, the activation footer counter sits
 * at block_end - 0x14 (i.e. block_start + block_len - 0x14). */
static int active_partition(const uint8_t *data, size_t block_off,
                            size_t block_len) {
    const uint8_t *p1 = data + block_off + block_len - HGSS_FOOTER_CTR_OFF;
    const uint8_t *p2 = data + HGSS_PART_SIZE + block_off + block_len
                             - HGSS_FOOTER_CTR_OFF;
    return compare_counters_part(p1, p2);
}

/* ===================================================================== */

bool hgss_load(hgss_sav_t *s, const char *path) {
    memset(s, 0, sizeof(*s));
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    size_t got = fread(s->data, 1, HGSS_SAV_SIZE, f);
    int eof_or_err = fgetc(f);
    fclose(f);
    if (got != HGSS_SAV_SIZE || eof_or_err != EOF) return false;

    /* Pick active partition for each block. */
    s->active_general_part = active_partition(s->data, 0, HGSS_GENERAL_SIZE);
    s->active_storage_part = active_partition(s->data, HGSS_STORAGE_OFF, HGSS_STORAGE_SIZE);

    s->loaded = true;
    hgss_validate(s);
    return true;
}

void hgss_validate(hgss_sav_t *s) {
    const uint8_t *gen = hgss_general_const(s);
    const uint8_t *sto = hgss_storage_const(s);

    uint16_t gen_calc = hgss_crc16_ccitt(gen, HGSS_GENERAL_SIZE - HGSS_FOOTER_SIZE);
    uint16_t gen_save = rd16le(gen + HGSS_GENERAL_SIZE - 2);
    s->general_status = (gen_calc == gen_save) ? HGSS_OK : HGSS_BAD;
    s->general_magic  = rd32le(gen + HGSS_GENERAL_SIZE - 8);

    uint16_t sto_calc = hgss_crc16_ccitt(sto, HGSS_STORAGE_SIZE - HGSS_FOOTER_SIZE);
    uint16_t sto_save = rd16le(sto + HGSS_STORAGE_SIZE - 2);
    s->storage_status = (sto_calc == sto_save) ? HGSS_OK : HGSS_BAD;
    s->storage_magic  = rd32le(sto + HGSS_STORAGE_SIZE - 8);
}

bool hgss_save(hgss_sav_t *s, const char *path, bool bump_counter) {
    uint8_t *gen = hgss_general(s);
    uint8_t *sto = hgss_storage(s);

    if (bump_counter) {
        /* Increment major counter on the active block so it stays the
         * "newest" partition next load. PKHeX semantics: in-game saves
         * always advance this counter. */
        uint32_t maj = rd32le(gen + HGSS_GENERAL_SIZE - HGSS_FOOTER_CTR_OFF);
        wr32le(gen + HGSS_GENERAL_SIZE - HGSS_FOOTER_CTR_OFF, maj + 1);
        uint32_t maj2 = rd32le(sto + HGSS_STORAGE_SIZE - HGSS_FOOTER_CTR_OFF);
        wr32le(sto + HGSS_STORAGE_SIZE - HGSS_FOOTER_CTR_OFF, maj2 + 1);
    }

    /* Recompute footer CRCs (last 2 bytes of each block). */
    uint16_t gen_crc = hgss_crc16_ccitt(gen, HGSS_GENERAL_SIZE - HGSS_FOOTER_SIZE);
    wr16le(gen + HGSS_GENERAL_SIZE - 2, gen_crc);
    uint16_t sto_crc = hgss_crc16_ccitt(sto, HGSS_STORAGE_SIZE - HGSS_FOOTER_SIZE);
    wr16le(sto + HGSS_STORAGE_SIZE - 2, sto_crc);

    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return false;
    size_t wrote = fwrite(s->data, 1, HGSS_SAV_SIZE, f);
    int rc_close = fclose(f);
    if (wrote != HGSS_SAV_SIZE || rc_close != 0) { remove(tmp); return false; }
    /* Cross-platform overwrite (Windows rename won't replace dst). */
    remove(path);
    if (rename(tmp, path) != 0) { remove(tmp); return false; }
    return true;
}

/* ===================================================================== */

const uint8_t *hgss_general_const(const hgss_sav_t *s) {
    size_t off = (s->active_general_part == 0) ? 0 : HGSS_PART_SIZE;
    return s->data + off;
}
uint8_t *hgss_general(hgss_sav_t *s) {
    return (uint8_t *)hgss_general_const(s);
}

const uint8_t *hgss_storage_const(const hgss_sav_t *s) {
    size_t off = (s->active_storage_part == 0)
                  ? HGSS_STORAGE_OFF
                  : HGSS_PART_SIZE + HGSS_STORAGE_OFF;
    return s->data + off;
}
uint8_t *hgss_storage(hgss_sav_t *s) {
    return (uint8_t *)hgss_storage_const(s);
}

const uint8_t *hgss_walker_pair_const(const hgss_sav_t *s) {
    return hgss_general_const(s) + HGSS_WALKER_PAIR;
}
uint8_t *hgss_walker_pair(hgss_sav_t *s) {
    return hgss_general(s) + HGSS_WALKER_PAIR;
}

uint32_t hgss_walker_steps(const hgss_sav_t *s) {
    return rd32le(hgss_general_const(s) + HGSS_WALKER_STEPS);
}
void hgss_set_walker_steps(hgss_sav_t *s, uint32_t v) {
    wr32le(hgss_general(s) + HGSS_WALKER_STEPS, v);
}
uint32_t hgss_walker_watts(const hgss_sav_t *s) {
    return rd32le(hgss_general_const(s) + HGSS_WALKER_WATTS);
}
void hgss_set_walker_watts(hgss_sav_t *s, uint32_t v) {
    wr32le(hgss_general(s) + HGSS_WALKER_WATTS, v);
}
uint32_t hgss_walker_courses(const hgss_sav_t *s) {
    return rd32le(hgss_general_const(s) + HGSS_WALKER_COURSES);
}
void hgss_set_walker_courses(hgss_sav_t *s, uint32_t mask) {
    wr32le(hgss_general(s) + HGSS_WALKER_COURSES, mask);
}

uint16_t hgss_walker_deposit_flag(const hgss_sav_t *s) {
    return rd16le(hgss_general_const(s) + HGSS_WALKER_DEPOSIT_FLAG);
}
void hgss_set_walker_deposit(hgss_sav_t *s, uint16_t flag, uint16_t tobox) {
    wr16le(hgss_general(s) + HGSS_WALKER_DEPOSIT_FLAG, flag);
    wr16le(hgss_general(s) + HGSS_WALKER_DEPOSIT_TOBOX, tobox);
}

uint16_t hgss_tid(const hgss_sav_t *s) {
    return rd16le(hgss_general_const(s) + HGSS_TRAINER1_OFF + 0x10);
}
uint16_t hgss_sid(const hgss_sav_t *s) {
    return rd16le(hgss_general_const(s) + HGSS_TRAINER1_OFF + 0x12);
}
void hgss_ot_name(const hgss_sav_t *s, uint8_t out[HGSS_OT_NAME_LEN]) {
    memcpy(out, hgss_general_const(s) + HGSS_TRAINER1_OFF, HGSS_OT_NAME_LEN);
}
uint8_t hgss_ot_gender(const hgss_sav_t *s) {
    return hgss_general_const(s)[HGSS_TRAINER1_OFF + 0x18];
}
uint8_t hgss_ot_language(const hgss_sav_t *s) {
    /* HGSS stores the OT language at trainer1+0x19 (+0x1A is the DP/Pt
     * offset). Valid codes: 1=JPN, 2=ENG, 3=FRE, 4=ITA, 5=GER, 7=SPA,
     * 8=KOR. Default to English if the byte is out of range. */
    uint8_t lang = hgss_general_const(s)[HGSS_TRAINER1_OFF + 0x19];
    if (lang < 1 || lang > 8 || lang == 6) lang = 2;
    return lang;
}

/* ===================================================================== */
/* PC boxes                                                              */
/* ===================================================================== */

static bool valid_slot(int box, int slot) {
    return box  >= 0 && box  < HGSS_BOX_COUNT
        && slot >= 0 && slot < HGSS_BOX_SLOTS;
}

const uint8_t *hgss_box_slot_const(const hgss_sav_t *s, int box, int slot) {
    if (!valid_slot(box, slot)) return NULL;
    return hgss_storage_const(s) + HGSS_BOX_DATA_OFF
         + box * HGSS_BOX_STRIDE
         + slot * HGSS_BOX_SIZE_PK4;
}
uint8_t *hgss_box_slot(hgss_sav_t *s, int box, int slot) {
    return (uint8_t *)hgss_box_slot_const(s, box, slot);
}

const uint8_t *hgss_box_name_const(const hgss_sav_t *s, int box) {
    if (box < 0 || box >= HGSS_BOX_COUNT) return NULL;
    return hgss_storage_const(s) + HGSS_BOX_NAMES_OFF
         + box * HGSS_BOX_NAME_LEN;
}
uint8_t *hgss_box_name(hgss_sav_t *s, int box) {
    return (uint8_t *)hgss_box_name_const(s, box);
}

uint8_t hgss_box_wallpaper(const hgss_sav_t *s, int box) {
    if (box < 0 || box >= HGSS_BOX_COUNT) return 0;
    return hgss_storage_const(s)[HGSS_BOX_WALLPAPERS_OFF + box];
}
void hgss_set_box_wallpaper(hgss_sav_t *s, int box, uint8_t wp) {
    if (box < 0 || box >= HGSS_BOX_COUNT) return;
    hgss_storage(s)[HGSS_BOX_WALLPAPERS_OFF + box] = wp;
}

uint8_t hgss_current_box(const hgss_sav_t *s) {
    return hgss_storage_const(s)[HGSS_CURRENT_BOX_OFF];
}
void hgss_set_current_box(hgss_sav_t *s, uint8_t box) {
    if (box >= HGSS_BOX_COUNT) box = 0;
    hgss_storage(s)[HGSS_CURRENT_BOX_OFF] = box;
}

bool hgss_slot_is_empty(const hgss_sav_t *s, int box, int slot) {
    const uint8_t *p = hgss_box_slot_const(s, box, slot);
    if (!p) return false;
    /* PK4 stored format: empty slot has first 8 bytes zero (PID u32 +
     * unused u16 + checksum u16). The remaining 128 B are NOT zero on
     * empty slots: they're the deterministic output of the encryption
     * LCG seeded at chk=0, so plaintext-zero XOR LCG(0) yields noise.
     * Checking only the first 8 bytes matches what PKHeX does for the
     * stored form (`PKM.PID == 0` is the cheap presence test). */
    for (int i = 0; i < 8; i++) if (p[i] != 0) return false;
    return true;
}

bool hgss_find_first_empty_slot(const hgss_sav_t *s, int *box, int *slot) {
    for (int b = 0; b < HGSS_BOX_COUNT; b++) {
        for (int sl = 0; sl < HGSS_BOX_SLOTS; sl++) {
            if (hgss_slot_is_empty(s, b, sl)) {
                if (box)  *box  = b;
                if (slot) *slot = sl;
                return true;
            }
        }
    }
    return false;
}

/* ===================================================================== */
/* Item bags                                                              */
/* ===================================================================== */

static bool bag_layout(hgss_bag_kind_t bag, uint16_t *off, uint16_t *cap) {
    switch (bag) {
    case HGSS_BAG_ITEMS:    *off = HGSS_BAG_ITEMS_OFF;    *cap = HGSS_BAG_ITEMS_CAP;    return true;
    case HGSS_BAG_KEY:      *off = HGSS_BAG_KEY_OFF;      *cap = HGSS_BAG_KEY_CAP;      return true;
    case HGSS_BAG_TMHM:     *off = HGSS_BAG_TMHM_OFF;     *cap = HGSS_BAG_TMHM_CAP;     return true;
    case HGSS_BAG_MEDICINE: *off = HGSS_BAG_MEDICINE_OFF; *cap = HGSS_BAG_MEDICINE_CAP; return true;
    case HGSS_BAG_BERRIES:  *off = HGSS_BAG_BERRIES_OFF;  *cap = HGSS_BAG_BERRIES_CAP;  return true;
    case HGSS_BAG_BALLS:    *off = HGSS_BAG_BALLS_OFF;    *cap = HGSS_BAG_BALLS_CAP;    return true;
    case HGSS_BAG_BATTLE:   *off = HGSS_BAG_BATTLE_OFF;   *cap = HGSS_BAG_BATTLE_CAP;   return true;
    case HGSS_BAG_MAIL:     *off = HGSS_BAG_MAIL_OFF;     *cap = HGSS_BAG_MAIL_CAP;     return true;
    default: return false;
    }
}

bool hgss_bag_add(hgss_sav_t *s, hgss_bag_kind_t bag,
                  uint16_t item_id, uint16_t count) {
    if (!s || !s->loaded) return false;
    if (item_id == 0 || count == 0) return false;

    uint16_t off, cap;
    if (!bag_layout(bag, &off, &cap)) return false;

    uint8_t *bag_base = hgss_general(s) + off;

    /* Pass 1: look for an existing slot with the same item_id; if found,
     * increment its count (capped at HGSS_BAG_ITEM_MAX_COUNT). */
    for (uint16_t i = 0; i < cap; i++) {
        uint8_t *slot = bag_base + (size_t)i * 4u;
        uint16_t cur_id = rd16le(slot);
        uint16_t cur_n  = rd16le(slot + 2);
        if (cur_id == 0 || cur_n == 0) continue;  /* skip empty */
        if (cur_id == item_id) {
            uint32_t total = (uint32_t)cur_n + count;
            if (total > HGSS_BAG_ITEM_MAX_COUNT) total = HGSS_BAG_ITEM_MAX_COUNT;
            wr16le(slot + 2, (uint16_t)total);
            return true;
        }
    }

    /* Pass 2: write into the first empty slot. */
    for (uint16_t i = 0; i < cap; i++) {
        uint8_t *slot = bag_base + (size_t)i * 4u;
        uint16_t cur_id = rd16le(slot);
        uint16_t cur_n  = rd16le(slot + 2);
        if (cur_id == 0 || cur_n == 0) {
            wr16le(slot,     item_id);
            wr16le(slot + 2, count > HGSS_BAG_ITEM_MAX_COUNT
                              ? (uint16_t)HGSS_BAG_ITEM_MAX_COUNT
                              : count);
            return true;
        }
    }

    /* Bag full. */
    return false;
}
