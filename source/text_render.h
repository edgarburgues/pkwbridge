/*
 * text_render.h - Render a gen4 wide-char string into walker LCD format.
 *
 * Pipeline:
 *   gen4 string (u16 LE, FFFF-terminated)
 *      -> font_sys glyph lookup (variable-width, extracted at boot from
 *         gs_font.narc[10] — the real HGSS FONT_SYSTEM)
 *      -> raster into a w*h pixel buffer with values 0..3
 *      -> bmp2phc to walker 2-plane format
 *
 * Output size by field:
 *   80×16 -> 320 B  (pokeName, courseName, enemyName[i])
 *   96×16 -> 384 B  (item name glyphs)
 */

#ifndef TEXT_RENDER_H
#define TEXT_RENDER_H

#include <stdint.h>
#include <stddef.h>

/* Render a gen4 string into a walker glyph buffer.
 *
 * @param gen4_chars  source u16 array (LE), terminated by 0xFFFF.
 * @param max_chars   maximum chars to read (PK4 nickname = 11, OT = 8).
 * @param x0, y0      pixel offset of first glyph within the buffer.
 *                    HGSS: pokeName/itemName use (2, 1), courseName/heroName
 *                    use (2, 0).
 * @param w           output pixel width (80 for names, 96 for items).
 * @param h           output pixel height (always 16 in HGSS).
 * @param out         output buffer of size w*h/8*2 bytes (320 or 384).
 */
void text_render_gen4(const uint8_t *gen4_chars,
                      size_t max_chars,
                      int x0, int y0,
                      int w, int h,
                      uint8_t *out);

#endif
