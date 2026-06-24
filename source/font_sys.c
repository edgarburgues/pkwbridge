/* font_sys.c — runtime-loaded font glyphs + decode helpers.
 *
 * font_sys_glyphs and font_sys_widths live in BSS; data_loader.c fills
 * them at boot from:
 *   sdmc:/3ds/pokeStride/data/font_sys_glyphs.bin
 *   sdmc:/3ds/pokeStride/data/font_sys_widths.bin
 * Generate these with scripts/extract_nds.py against your own HGSS
 * dump. If the files are absent, the arrays stay zero and callers
 * should check app_data_font_loaded() before relying on the output.
 */
#include "font_sys.h"

uint8_t font_sys_glyphs[FONT_SYS_NUM_GLYPHS][FONT_SYS_BYTES_PER_GLYPH];
uint8_t font_sys_widths[FONT_SYS_NUM_GLYPHS];

static const uint8_t PHC_PALETTE[4] = { 0, 3, 1, 0 };

static void decode_tile(const uint8_t *tile, uint8_t *out, int stride, int x0, int y0) {
    for (int r = 0; r < 8; r++) {
        uint8_t b_lo = tile[r*2];
        uint8_t b_hi = tile[r*2 + 1];
        for (int half = 0; half < 2; half++) {
            uint8_t byte = (half == 0) ? b_hi : b_lo;
            int col_base = x0 + half * 4;
            for (int p = 0; p < 4; p++) {
                int d = (byte >> (6 - p*2)) & 3;
                out[(y0+r)*stride + (col_base+p)] = PHC_PALETTE[d];
            }
        }
    }
}

int font_sys_render_glyph(uint16_t glyph_id, uint8_t out[FONT_SYS_GLYPH_W * FONT_SYS_GLYPH_H]) {
    if (glyph_id >= FONT_SYS_NUM_GLYPHS) return 0;
    const uint8_t *g = font_sys_glyphs[glyph_id];
    decode_tile(g +  0, out, FONT_SYS_GLYPH_W, 0, 0);
    decode_tile(g + 16, out, FONT_SYS_GLYPH_W, 8, 0);
    decode_tile(g + 32, out, FONT_SYS_GLYPH_W, 0, 8);
    decode_tile(g + 48, out, FONT_SYS_GLYPH_W, 8, 8);
    return 1;
}

int font_sys_render_strcode(uint16_t strcode, uint8_t out[FONT_SYS_GLYPH_W * FONT_SYS_GLYPH_H]) {
    if (strcode == 0 || strcode > FONT_SYS_NUM_GLYPHS) return 0;
    return font_sys_render_glyph(strcode - 1, out);
}

uint8_t font_sys_width_strcode(uint16_t strcode) {
    if (strcode == 0 || strcode > FONT_SYS_NUM_GLYPHS) return 0;
    return font_sys_widths[strcode - 1];
}
