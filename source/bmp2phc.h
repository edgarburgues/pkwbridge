/*
 * bmp2phc.h - Convert 2-bit pixel buffers into the Pokewalker LCD's
 * 2-plane interleaved column-major format.
 *
 * Walker LCD = 96×64 px, 4 grayscale shades (2 bits per pixel).
 * On-EEPROM encoding:
 *
 *   For each x in [0, w):
 *     For each y_strip in [0, h/8):
 *       Emit 2 bytes:
 *         byte0 = bit (pixel>>1) for y_strip*8 + 0..7  (high plane)
 *         byte1 = bit (pixel & 1) for y_strip*8 + 0..7 (low  plane)
 *
 * Output size: w * (h/8) * 2 bytes.
 * Common shapes used by PHCCourseData:
 *   80×16 -> 320 B  (courseName, pokeName, enemyName[i])
 *   96×16 -> 384 B  (item name glyphs)
 *   32×24 ->  ??    (imagePoke - 384 B total for 2 frames? layout TBD)
 *   64×48 -> 768 B  (imagePokeBig single frame - 1536 B for 2 frames)
 *
 * Note: the Pokémon-sprite NARCs already store data in this format
 * (LZ-compressed). For sprites we just decompress and copy; bmp2phc
 * is only needed when we RENDER text/glyphs from a source pixel buffer.
 */

#ifndef BMP2PHC_H
#define BMP2PHC_H

#include <stdint.h>
#include <stddef.h>

/* Convert a 2bpp planar pixel buffer (1 byte per pixel, low 2 bits used)
 * of dimensions w × h into walker format. Output size = w * (h/8) * 2.
 * `pixels` must have w*h entries; `h` must be a multiple of 8. */
void bmp2phc(const uint8_t *pixels, int w, int h, uint8_t *out);

/* Inverse: walker 2-plane format -> 2bpp linear pixels (for debug/diff). */
void phc2bmp(const uint8_t *src, int w, int h, uint8_t *pixels_out);

#endif
