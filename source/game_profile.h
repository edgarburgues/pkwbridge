/*
 * game_profile.h — per-game NDS dump descriptor.
 *
 * PkwBridge was written against Pokemon HeartGold / SoulSilver, but
 * everything game-specific (FAT file IDs, arm9 offsets, accepted game
 * codes, sprite counts) is centralised in a `game_profile_t` so a new
 * generation can be added by registering a new profile without touching
 * the extractor or picker.
 *
 * Truly HGSS-only flows (Pokewalker — Send, Claim, Inspect, walker
 * font, walker phcgra/phcicon) gate themselves on
 * `profile->has_pokewalker`. The Pokewalker doesn't exist in Gen 5+, so
 * for non-HGSS profiles those constants are zero and the corresponding
 * extraction steps are skipped.
 *
 * To add a new game:
 *   1. Define a `game_profile_t` in game_profile.c.
 *   2. Add it to the PROFILES[] table at the bottom of that file.
 *   3. If the game has sprite formats we already know how to decode,
 *      that's enough. If formats differ, parametrise pokeicon.c
 *      similarly.
 */
#ifndef GAME_PROFILE_H
#define GAME_PROFILE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    /* Human label, e.g. "Pokemon HeartGold / SoulSilver". */
    const char *display_name;

    /* NDS game codes this profile accepts. Each entry is exactly 4
     * chars; the last char is matched against `*` as a wildcard so a
     * single entry covers all regions of one title. Array is
     * NULL-terminated. Examples:
     *   { "IPGE", "IPGS", "IPG*", NULL } — wildcard region for SS
     *   { "IPK*", "IPG*", NULL }         — both HG and SS, all regions
     */
    const char *const *accepted_codes;

    /* True if this game has the Pokewalker accessory (HGSS only). All
     * Pokewalker-related extraction steps and UI flows check this. */
    bool has_pokewalker;

    /* === Per-game NDS extraction ================================ */

    /* Pokeicon NARC location + count (sprites + form variants). */
    uint16_t pokeicon_file_id;
    uint16_t pokeicon_count;

    /* Per-species palette index table inside the BLZ-decompressed arm9.
     * For HGSS it's at 0x0FFC00. Gen 5 has its own offset. */
    uint32_t pokeicon_arm9_pal_index_offset;

    /* === Pokewalker-only (zero if !has_pokewalker) ============== */

    uint16_t phcgra_file_id;       /* Pokemon big-portrait NARC      */
    uint16_t phcgra_count;
    uint16_t phcicon_file_id;      /* Pokemon walking-icon NARC      */
    uint16_t phcicon_count;
    uint16_t font_file_id;         /* PHC system font NARC           */
    uint16_t font_inner;           /* Inner file index for the font  */
    uint16_t overlay_courses;      /* ARM9 overlay with course bmps  */
    uint16_t msg_narc_file_id;     /* In-game text NARC              */
    uint16_t msg_inner_items;      /* Inner file: item names         */
    uint16_t msg_inner_moves;      /* Inner file: move names         */
    uint16_t msg_inner_locations;  /* Inner file: location names     */

    /* === Cross-gen: personal data (gender ratio + growth rate) ===
     * NARC of per-species personal entries (gen4 entry = 0x2C bytes,
     * gender ratio at +0x10, growth rate at +0x13). `personal_file_id`
     * is a hint validated at extraction; if it fails the sanity check
     * the extractor scans the FAT for a NARC with `personal_count`
     * entries that passes known gender ratios. 0 = scan only.
     * Validated for ES SoulSilver (IPGS): file_id 131, count 508. */
    uint16_t personal_file_id;
    uint16_t personal_count;

    /* === Region labels =========================================== *
     * Map from the 4th game-code char ('E', 'S', 'F', 'I', 'D', 'J',
     * 'P', 'K') to a human label. Each profile provides its own table
     * because the title differs per profile. NULL entries fall back to
     * the bare game code. */
    const char *(*region_label)(const char gamecode[4]);
} game_profile_t;


/* Look up the profile that recognises this NDS game code, or NULL if
 * none does. */
const game_profile_t *game_profile_for_code(const char gamecode[4]);

/* Iterate all registered profiles (for diagnostics or picker labels).
 * Returns NULL once idx is past the last profile. */
const game_profile_t *game_profile_at(int idx);

#endif
