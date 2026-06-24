/* bmp2phc.c - See header for the format.
 *
 * Byte order is strip-major. For an 80x16 image with 2 strips of 8 rows
 * each:
 *   bytes  0..159 = strip 0 (top 8 rows), 80 columns × 2 bytes/col
 *   bytes 160..319 = strip 1 (bottom 8 rows), 80 columns × 2 bytes/col
 */

#include "bmp2phc.h"
#include <string.h>

void bmp2phc(const uint8_t *pixels, int w, int h, uint8_t *out) {
    int strips = h / 8;
    int total = w * strips * 2;
    memset(out, 0, total);
    int count = 0;
    for (int s = 0; s < strips; s++) {
        for (int x = 0; x < w; x++) {
            uint8_t b0 = 0, b1 = 0;
            for (int yl = 0; yl < 8; yl++) {
                int y = s * 8 + yl;
                uint8_t p = pixels[y * w + x] & 0x03;
                b0 |= (uint8_t)(((p >> 1) & 1) << yl);
                b1 |= (uint8_t)(((p & 1)      ) << yl);
            }
            out[count++] = b0;
            out[count++] = b1;
        }
    }
}

void phc2bmp(const uint8_t *src, int w, int h, uint8_t *pixels_out) {
    int strips = h / 8;
    int count = 0;
    for (int s = 0; s < strips; s++) {
        for (int x = 0; x < w; x++) {
            uint8_t b0 = src[count++];
            uint8_t b1 = src[count++];
            for (int yl = 0; yl < 8; yl++) {
                uint8_t bit0 = (b0 >> yl) & 1;
                uint8_t bit1 = (b1 >> yl) & 1;
                int y = s * 8 + yl;
                pixels_out[y * w + x] = (uint8_t)((bit0 << 1) | bit1);
            }
        }
    }
}
