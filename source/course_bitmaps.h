/* course_bitmaps.h — 8 walker route background bitmaps.
 *
 * 32x24 pixels, 2 bits per pixel, packed = 192 bytes per bitmap.
 * Indexed by (course_def_t.graphic_id - 1).
 *
 * The pixel data is NOT bundled. data_loader.c loads
 * sdmc:/3ds/pokeStride/data/course_bitmaps.bin at boot. If absent, the
 * array stays zero and write sites skip touching the pweep's bitmap
 * region (walker keeps the template's existing background).
 */
#ifndef COURSE_BITMAPS_H
#define COURSE_BITMAPS_H

#include <stdint.h>

#define COURSE_BITMAP_COUNT  8
#define COURSE_BITMAP_SIZE   192
#define COURSE_BITMAP_W      32
#define COURSE_BITMAP_H      24

extern uint8_t course_bitmaps[COURSE_BITMAP_COUNT][COURSE_BITMAP_SIZE];

#endif /* COURSE_BITMAPS_H */
