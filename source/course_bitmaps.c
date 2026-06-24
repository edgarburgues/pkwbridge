/* course_bitmaps.c — runtime storage for walker route backgrounds.
 *
 * The pixel data lives at sdmc:/3ds/pokeStride/data/course_bitmaps.bin
 * and is loaded by data_loader.c at boot. Until then this array is BSS
 * (zero-filled).
 */
#include "course_bitmaps.h"

uint8_t course_bitmaps[COURSE_BITMAP_COUNT][COURSE_BITMAP_SIZE];
