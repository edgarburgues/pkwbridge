#include "pk_summary.h"
#include "gen4_chartable.h"
#include "personal_gen4.h"
#include "pk4.h"

#include <string.h>

/* The 25 Gen 3+ nature labels in canonical order (PID % 25 -> name). */
static const char *const NATURES[25] = {
    "Hardy",  "Lonely", "Brave",   "Adamant", "Naughty",
    "Bold",   "Docile", "Relaxed", "Impish",  "Lax",
    "Timid",  "Hasty",  "Serious", "Jolly",   "Naive",
    "Modest", "Mild",   "Quiet",   "Bashful", "Rash",
    "Calm",   "Gentle", "Sassy",   "Careful", "Quirky",
};

/* HGSS language codes:
 *   1 JPN, 2 ENG, 3 FRE, 4 ITA, 5 GER, 7 SPA, 8 KOR
 * (6 is unused.) */
static const char *const LANG_SHORT[9] = {
    "?", "JPN", "ENG", "FRE", "ITA", "GER", "?", "SPA", "KOR",
};

const char *pk_nature_name(uint8_t nature) {
    return (nature < 25) ? NATURES[nature] : "?";
}

const char *pk_language_short(uint8_t lang) {
    return (lang < 9) ? LANG_SHORT[lang] : "?";
}

/* Compute gender from PID + species gender ratio (Gen 4 rule):
 *   ratio 0xFF -> genderless (2)
 *   ratio 0xFE -> female only (1)
 *   ratio 0x00 -> male only (0)
 *   else: (PID & 0xFF) < ratio -> female (1), otherwise male (0). */
static uint8_t derive_gender(uint16_t species, uint32_t pid) {
    uint8_t r = personal_gender_ratio(species);
    if (r == 0xFF) return 2;
    if (r == 0xFE) return 1;
    if (r == 0x00) return 0;
    return ((pid & 0xFF) < r) ? 1 : 0;
}

/* Unpack the 30 IV bits packed at PK4 +0x38 (u32 LE). Order in Gen 4:
 *   bits  0.. 4 HP
 *   bits  5.. 9 Atk
 *   bits 10..14 Def
 *   bits 15..19 Spe        (Speed is between Def and SpA in Gen 3+)
 *   bits 20..24 SpA
 *   bits 25..29 SpD
 *   bit  30     isEgg
 *   bit  31     isNicknamed
 * out array layout matches the struct: HP, Atk, Def, Spe, SpA, SpD. */
static void unpack_ivs(uint32_t iv_word, uint8_t out[6], bool *out_nick) {
    for (int i = 0; i < 6; i++) {
        out[i] = (uint8_t)((iv_word >> (i * 5)) & 0x1F);
    }
    if (out_nick) *out_nick = (iv_word >> 31) & 1;
}

/* Hidden Power type 0..15 derived from the LSB of each IV. Maps to
 * one of the 16 non-Normal/non-??? types via a fixed formula. */
static uint8_t hidden_power_type(const uint8_t iv[6]) {
    /* Reorder to PID-paper order: HP, Atk, Def, Spe, SpA, SpD == the
     * same array layout above. */
    uint32_t t = ((iv[0] & 1) << 0)
               | ((iv[1] & 1) << 1)
               | ((iv[2] & 1) << 2)
               | ((iv[3] & 1) << 3)
               | ((iv[4] & 1) << 4)
               | ((iv[5] & 1) << 5);
    return (uint8_t)((t * 15) / 63);
}

/* HP power 30..70 derived from bit 1 of each IV. */
static uint8_t hidden_power_power(const uint8_t iv[6]) {
    uint32_t t = (((iv[0] >> 1) & 1) << 0)
               | (((iv[1] >> 1) & 1) << 1)
               | (((iv[2] >> 1) & 1) << 2)
               | (((iv[3] >> 1) & 1) << 3)
               | (((iv[4] >> 1) & 1) << 4)
               | (((iv[5] >> 1) & 1) << 5);
    return (uint8_t)(((t * 40) / 63) + 30);
}

/* Decode a gen4 wide-char string into ASCII. Stops at the 0xFFFF
 * terminator or when `cap` is reached. Untranslatable chars become
 * '?'. Writes a NUL terminator. */
static void decode_string(const uint8_t *src, size_t max_chars,
                          char *out, size_t cap) {
    size_t w = 0;
    for (size_t i = 0; i < max_chars && w + 1 < cap; i++) {
        uint16_t code = (uint16_t)src[i * 2] | ((uint16_t)src[i * 2 + 1] << 8);
        if (code == 0xFFFF) break;
        char c = gen4_to_ascii(code);
        out[w++] = c ? c : '?';
    }
    out[w] = 0;
}

bool pk_summary_from(const uint8_t pk[136], uint8_t level, pk_summary_t *out) {
    if (!out) return false;
    memset(out, 0, sizeof *out);
    out->species = pk4_u16(pk, PK4_OFF_SPECIES);
    if (out->species == 0) return false;

    out->level     = level;
    out->pid       = pk4_pid(pk);
    out->held_item = pk4_u16(pk, PK4_OFF_HELD_ITEM);
    out->ot_tid    = pk4_u16(pk, PK4_OFF_OT_TID);
    out->ot_sid    = pk4_u16(pk, PK4_OFF_OT_SID);
    out->exp       = pk4_u32(pk, PK4_OFF_EXP);
    out->friendship= pk[PK4_OFF_FRIENDSHIP];
    out->language  = pk[PK4_OFF_LANGUAGE];
    out->ability_slot = (uint8_t)(out->pid & 1);
    for (int i = 0; i < 4; i++) {
        out->move[i] = pk4_u16(pk, PK4_OFF_MOVE1 + i * 2);
    }
    out->nature = (uint8_t)(out->pid % 25);
    out->gender = derive_gender(out->species, out->pid);

    bool nicknamed;
    uint32_t iv_word = pk4_u32(pk, 0x38);
    unpack_ivs(iv_word, out->iv, &nicknamed);
    out->is_nicknamed = nicknamed;
    out->ev_hidden_power_type  = hidden_power_type(out->iv);
    out->ev_hidden_power_power = hidden_power_power(out->iv);

    out->met_year     = pk[PK4_OFF_MET_DATE + 0];
    out->met_month    = pk[PK4_OFF_MET_DATE + 1];
    out->met_day      = pk[PK4_OFF_MET_DATE + 2];
    out->met_location = pk4_u16(pk, PK4_OFF_MET_LOCATION);
    uint8_t mg        = pk[PK4_OFF_MET_LVL_GEND];
    out->met_level    = (uint8_t)(mg & 0x7F);
    out->ot_gender    = (uint8_t)((mg >> 7) & 1);
    out->pokeball     = pk[PK4_OFF_BALL];

    decode_string(pk + PK4_OFF_NICKNAME,
                  PK4_OFF_NICKNAME_LEN / 2,
                  out->nickname, sizeof out->nickname);
    decode_string(pk + PK4_OFF_OT_NAME,
                  PK4_OFF_OT_NAME_LEN / 2,
                  out->ot_name, sizeof out->ot_name);
    return true;
}
