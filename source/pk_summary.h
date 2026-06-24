/*
 * pk_summary.h - derived fields for a Pokemon storage summary view.
 *
 * Bundles together everything that's useful to show on the bottom
 * screen when the user has a PK4 in focus: name, level, gender,
 * nature, IVs, moves, met info, OT. Caller hands in a decrypted
 * 136-byte PK4 plus the derived level (which lives outside the
 * stored format) and gets back a `pk_summary_t` filled in.
 */
#ifndef PK_SUMMARY_H
#define PK_SUMMARY_H

#include <stdbool.h>
#include <stdint.h>

#define PK_NICKNAME_ASCII_CAP 12   /* 11 chars + NUL */
#define PK_OT_NAME_ASCII_CAP   9   /* 8 chars + NUL */

typedef struct {
    uint16_t species;
    uint8_t  level;
    uint8_t  gender;      /* 0 = male, 1 = female, 2 = genderless */
    uint8_t  nature;      /* 0..24, name via pk_nature_name(nature) */
    uint16_t held_item;
    uint16_t move[4];

    uint8_t  iv[6];       /* HP, Atk, Def, Spe, SpA, SpD (Gen 4 order) */
    uint8_t  ev_hidden_power_type;   /* derived from IVs, 0..15 */
    uint8_t  ev_hidden_power_power;  /* derived from IVs, 30..70 */

    uint8_t  ability_slot;           /* 0 or 1 (from PID bit 0) */
    uint8_t  language;
    uint8_t  friendship;
    uint8_t  pokeball;
    uint8_t  met_level;
    uint8_t  met_year;     /* year since 2000 */
    uint8_t  met_month;
    uint8_t  met_day;
    uint16_t met_location;

    uint16_t ot_tid;
    uint16_t ot_sid;
    uint8_t  ot_gender;    /* 0 male, 1 female */
    bool     is_nicknamed;

    uint32_t exp;
    uint32_t pid;

    /* ASCII previews of the gen4-encoded strings. Untranslatable chars
     * are rendered as '?'. */
    char nickname[PK_NICKNAME_ASCII_CAP];
    char ot_name[PK_OT_NAME_ASCII_CAP];
} pk_summary_t;

/* Populate `out` from a decrypted PK4 plus the level computed by the
 * caller (level isn't in the stored 136-byte form). Returns false if
 * the input is obviously invalid (species == 0). */
bool pk_summary_from(const uint8_t pk_dec[136], uint8_t level,
                     pk_summary_t *out);

/* Static name lookups for the small Pokemon-game enums. */
const char *pk_nature_name(uint8_t nature);     /* 0..24, else "?" */
const char *pk_language_short(uint8_t lang);    /* 1..8, else "?"  */

#endif
