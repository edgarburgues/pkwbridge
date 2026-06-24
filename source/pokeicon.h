/*
 * pokeicon.h — HGSS box icon sprites (32x32) loaded into a citro2d
 * texture atlas at boot.
 *
 * Data files (under sdmc:/3ds/pokeStride/data/):
 *   pokeicon_glyphs.bin    544 x 512 B raw NCGR tile data (frame 0)
 *   pokeicon_palettes.bin  3 x 32 B    BGR555 palettes
 *   pokeicon_pal_index.bin 544 B       per-species palette id (0..2)
 *
 * If any of those is missing pokeicon_init returns false and
 * pokeicon_loaded() stays false; callers should skip drawing icons.
 */
#ifndef POKEICON_H
#define POKEICON_H

#include <stdbool.h>
#include <stdint.h>
#include <citro2d.h>

#define POKEICON_DIM    32
#define POKEICON_COUNT 544

/* Load .bin files from SD, decode every icon, upload to GPU. Returns
 * false if any file was missing or short, or if texture init failed. */
bool pokeicon_init(void);

/* Free the GPU texture. Safe to call even if init failed. */
void pokeicon_fini(void);

bool pokeicon_loaded(void);

/* Returns true and fills `out` with a C2D_Image referencing the atlas
 * texture and the subtexture coords for the requested species. Returns
 * false if species is out of range or pokeicon hasn't been loaded. */
bool pokeicon_get(uint16_t species, C2D_Image *out);

#endif
