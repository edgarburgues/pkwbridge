/*
 * nds_extract.h - On-device extractor for PkwBridge runtime data.
 *
 * Given a path to an NDS dump and the game_profile_t that recognises
 * its game code, decodes the relevant NARCs / arm9 tables and writes
 * the .bin files that data_loader.c later loads into RAM. Pokewalker-
 * specific assets (course bitmaps, walker font, phcgra, phcicon) are
 * skipped if profile->has_pokewalker is false.
 *
 * Output files (sdmc:/3ds/pokeStride/data/):
 *   course_bitmaps.bin       (Pokewalker, HGSS only)
 *   phcgra_data.bin          (Pokewalker, HGSS only)
 *   phcicon_data.bin         (Pokewalker, HGSS only)
 *   font_sys_glyphs.bin      (Pokewalker, HGSS only)
 *   font_sys_widths.bin      (Pokewalker, HGSS only)
 *   pokeicon_glyphs.bin      (every game)
 *   pokeicon_palettes.bin    (every game)
 *   pokeicon_pal_index.bin   (every game)
 *   names_items.bin          (every game)
 *   names_moves.bin          (every game)
 *   names_locations.bin      (every game)
 *
 * Pure SD I/O — no on-cart access, no graphics. Logs progress via
 * pkw_log so the user can debug from the log file on SD.
 */
#ifndef NDS_EXTRACT_H
#define NDS_EXTRACT_H

#include <stdbool.h>

#include "game_profile.h"

/* Try each well-known candidate filename in DATA_DIR. For every NDS
 * found, looks up the matching game_profile_t and runs extraction.
 * Returns true on the first successful extraction. */
bool nds_extract_try(void);

/* Extract from an arbitrary NDS path. `prof` must be the profile that
 * recognises the NDS's 4-byte game code; pass NULL and the extractor
 * will look it up itself (reads the game code from the NDS header). */
bool nds_extract_from_path(const char *nds_path,
                           const game_profile_t *prof);

#endif
