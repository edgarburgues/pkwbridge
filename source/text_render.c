/* text_render.c - Variable-width gen4 text renderer using HGSS FONT_SYSTEM.
 *
 * For each gen4 STRCODE we look up the glyph (16x16 2bpp) and its width
 * from font_sys (extracted from gs_font.narc[0]). We advance the cursor
 * by the glyph's variable width and copy only the leftmost `width`
 * columns of each 16-wide glyph into the destination buffer.
 *
 * Walker stores pixel values 0..3 directly (no palette transform). Real
 * HGSS uses: 0 = transparent, 1 = outline, 2 = shadow, 3 = body.
 */

#include "text_render.h"
#include "gen4_chartable.h"
#include "font_sys.h"
#include "bmp2phc.h"

#include <string.h>

#define MAX_W   128
#define MAX_H   16

/* Plot one font_sys glyph clipped to `width` columns at (xpos, 0). */
static void plot_glyph_sys(uint8_t *pixels, int w, int h,
                            int xpos, uint16_t strcode) {
    uint8_t glyph[FONT_SYS_GLYPH_W * FONT_SYS_GLYPH_H];
    if (!font_sys_render_strcode(strcode, glyph)) return;
    int width = font_sys_width_strcode(strcode);
    if (width > FONT_SYS_GLYPH_W) width = FONT_SYS_GLYPH_W;
    for (int y = 0; y < FONT_SYS_GLYPH_H && y < h; y++) {
        for (int x = 0; x < width && xpos + x < w; x++) {
            uint8_t p = glyph[y * FONT_SYS_GLYPH_W + x];
            if (p) pixels[y * w + (xpos + x)] = p;
        }
    }
}

/* Plot glyph clipped to its variable width, with a y-offset relative to
 * the buffer top. */
static void plot_glyph_sys_xy(uint8_t *pixels, int w, int h, int xpos, int ypos,
                               uint16_t strcode) {
    uint8_t glyph[FONT_SYS_GLYPH_W * FONT_SYS_GLYPH_H];
    if (!font_sys_render_strcode(strcode, glyph)) return;
    int width = font_sys_width_strcode(strcode);
    if (width > FONT_SYS_GLYPH_W) width = FONT_SYS_GLYPH_W;
    for (int y = 0; y < FONT_SYS_GLYPH_H; y++) {
        int dy = ypos + y;
        if (dy < 0 || dy >= h) continue;
        for (int x = 0; x < width; x++) {
            int dx = xpos + x;
            if (dx < 0 || dx >= w) continue;
            uint8_t p = glyph[y * FONT_SYS_GLYPH_W + x];
            if (p) pixels[dy * w + dx] = p;
        }
    }
}

void text_render_gen4(const uint8_t *gen4_chars, size_t max_chars,
                      int x0, int y0,
                      int w, int h, uint8_t *out) {
    if (h > MAX_H) h = MAX_H;
    if (w > MAX_W) w = MAX_W;
    uint8_t pixels[MAX_W * MAX_H];
    memset(pixels, 0, (size_t)(w * h));

    int cursor = x0;
    for (size_t i = 0; i < max_chars; i++) {
        uint16_t code = (uint16_t)gen4_chars[i*2]
                      | ((uint16_t)gen4_chars[i*2 + 1] << 8);
        if (code == 0xFFFF || code == 0x0000) break;
        int glyph_w = font_sys_width_strcode(code);
        if (cursor + glyph_w > w) break;
        plot_glyph_sys_xy(pixels, w, h, cursor, y0, code);
        cursor += glyph_w;
    }

    bmp2phc(pixels, w, h, out);
}

