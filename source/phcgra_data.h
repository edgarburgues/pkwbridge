/* phcgra_data.h — Pokemon "big" sprite archive (664 × 1536 B).
 *
 * Data lives in BSS, loaded at boot by data_loader.c from
 *   sdmc:/3ds/pokeStride/data/phcgra_data.bin
 * Generate it with scripts/extract_nds.py.  phcgra_index.c provides
 * species/sex/form -> archive index mapping (kept compiled-in).
 */
#ifndef PKWBRIDGE_PHCGRA_DATA_H
#define PKWBRIDGE_PHCGRA_DATA_H

#include <stdint.h>

#define PHCGRA_COUNT       664
#define PHCGRA_ENTRY_SIZE  1536

extern uint8_t phcgra_data[PHCGRA_COUNT][PHCGRA_ENTRY_SIZE];

const uint8_t *phcgra_get(uint16_t archive_index);
uint16_t       phcgra_index_from_species(uint16_t species, uint8_t sex, uint8_t form);

#endif
