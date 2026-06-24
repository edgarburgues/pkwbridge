/* phcicon_data.h — 1-bit walker icon sprites (540 × 384 B).
 *
 * Pixel data lives in BSS, loaded at boot by data_loader.c from
 *   sdmc:/3ds/pokeStride/data/phcicon_data.bin
 * Generate with scripts/extract_nds.py against your HGSS dump.
 *
 * Indexing: phcicon archive index (NOT species). Use
 * phcicon_index_from_species() to translate from species+form.
 */
#ifndef PHCICON_DATA_H
#define PHCICON_DATA_H

#include <stdint.h>

#define PHCICON_COUNT       540
#define PHCICON_ENTRY_SIZE  384

extern uint8_t phcicon_data[PHCICON_COUNT][PHCICON_ENTRY_SIZE];

const uint8_t *phcicon_get(uint16_t index);
uint16_t       phcicon_index_from_species(uint16_t species, uint8_t form);

#endif
