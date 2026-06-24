/* font_sys.h — PHC font (16x16, 509 glyphs).
 *
 * Glyph data lives in BSS, loaded at boot by data_loader.c from
 *   sdmc:/3ds/pokeStride/data/font_sys_glyphs.bin
 *   sdmc:/3ds/pokeStride/data/font_sys_widths.bin
 * Generate those with scripts/extract_nds.py against your own
 * SoulSilver dump.
 */
#ifndef FONT_SYS_H
#define FONT_SYS_H

#include <stdint.h>

#define FONT_SYS_GLYPH_W          16
#define FONT_SYS_GLYPH_H          16
#define FONT_SYS_NUM_GLYPHS       509
#define FONT_SYS_BYTES_PER_GLYPH  64

extern uint8_t font_sys_glyphs[FONT_SYS_NUM_GLYPHS][FONT_SYS_BYTES_PER_GLYPH];
extern uint8_t font_sys_widths[FONT_SYS_NUM_GLYPHS];

int     font_sys_render_glyph  (uint16_t glyph_id, uint8_t out[FONT_SYS_GLYPH_W * FONT_SYS_GLYPH_H]);
int     font_sys_render_strcode(uint16_t strcode,  uint8_t out[FONT_SYS_GLYPH_W * FONT_SYS_GLYPH_H]);
uint8_t font_sys_width_strcode (uint16_t strcode);

#endif
