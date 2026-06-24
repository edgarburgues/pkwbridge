#include "game_profile.h"

#include <stdbool.h>
#include <string.h>

/* ------------------------- HGSS profile -------------------------- */

static const char *const HGSS_CODES[] = {
    "IPG*",   /* SoulSilver, all regions (IPGE / IPGS / IPGI / ...) */
    "IPK*",   /* HeartGold,  all regions                            */
    NULL,
};

static const char *hgss_region_label(const char gc[4]) {
    bool ss = gc[2] == 'G';
    switch (gc[3]) {
        case 'E': return ss ? "SoulSilver (US)" : "HeartGold (US)";
        case 'P': return ss ? "SoulSilver (EU)" : "HeartGold (EU)";
        case 'S': return ss ? "SoulSilver (ES)" : "HeartGold (ES)";
        case 'F': return ss ? "SoulSilver (FR)" : "HeartGold (FR)";
        case 'I': return ss ? "SoulSilver (IT)" : "HeartGold (IT)";
        case 'D': return ss ? "SoulSilver (DE)" : "HeartGold (DE)";
        case 'J': return ss ? "SoulSilver (JP)" : "HeartGold (JP)";
        case 'K': return ss ? "SoulSilver (KO)" : "HeartGold (KO)";
        default : return ss ? "SoulSilver"      : "HeartGold";
    }
}

static const game_profile_t PROFILE_HGSS = {
    .display_name         = "Pokemon HeartGold / SoulSilver",
    .accepted_codes       = HGSS_CODES,
    .has_pokewalker       = true,

    .pokeicon_file_id     = 149,
    .pokeicon_count       = 544,
    .pokeicon_arm9_pal_index_offset = 0x0FFC00,

    .phcgra_file_id       = 385,
    .phcgra_count         = 664,
    .phcicon_file_id      = 377,
    .phcicon_count        = 540,
    .font_file_id         = 145,
    .font_inner           = 10,
    .overlay_courses      = 112,
    .msg_narc_file_id     = 156,
    .msg_inner_items      = 222,
    .msg_inner_moves      = 750,
    .msg_inner_locations  = 279,

    /* Personal NARC: validated against ES SoulSilver (IPGS) = file_id
     * 131, 508 entries. HG / other regions may differ; the extractor
     * sanity-checks this id and falls back to a FAT scan if it fails. */
    .personal_file_id     = 131,
    .personal_count       = 508,

    .region_label         = hgss_region_label,
};

/* ----------------------- Profile registry ------------------------ *
 * Append new profiles (e.g. PROFILE_BW2) to this array. Order is
 * insignificant — game_profile_for_code returns the first match. */
static const game_profile_t *const PROFILES[] = {
    &PROFILE_HGSS,
    /* TODO: PROFILE_BW, PROFILE_B2W2, PROFILE_PT, PROFILE_DP */
};
static const int N_PROFILES =
    (int)(sizeof PROFILES / sizeof PROFILES[0]);

static bool code_matches(const char *pattern, const char gamecode[4]) {
    /* `pattern` is exactly 4 chars; '*' wildcards a single position. */
    if (strlen(pattern) != 4) return false;
    for (int i = 0; i < 4; i++) {
        if (pattern[i] == '*') continue;
        if (pattern[i] != gamecode[i]) return false;
    }
    return true;
}

const game_profile_t *game_profile_for_code(const char gamecode[4]) {
    for (int p = 0; p < N_PROFILES; p++) {
        const game_profile_t *pr = PROFILES[p];
        for (int i = 0; pr->accepted_codes[i]; i++) {
            if (code_matches(pr->accepted_codes[i], gamecode)) {
                return pr;
            }
        }
    }
    return NULL;
}

const game_profile_t *game_profile_at(int idx) {
    if (idx < 0 || idx >= N_PROFILES) return NULL;
    return PROFILES[idx];
}
