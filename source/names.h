/*
 * names.h - English species + item names for display.
 *
 * HGSS stores Pokemon names in the game's own language, which is not
 * helpful for a Spanish/English user. This module ships a built-in
 * English table for Gen 4 species (#1..#493). Items fall back to
 * numeric IDs.
 */
#ifndef NAMES_H
#define NAMES_H

#include <stdbool.h>
#include <stdint.h>

/* Returns "Bulbasaur" etc. or "species N" if N is out of range. */
const char *species_name(uint16_t species);

/* Combined "#0001 Bulbasaur" form for compact display. Writes into out. */
void species_label(uint16_t species, char *out, int cap);

/* Lookup item / move / met-location names. These are populated at boot
 * from the .bin files at sdmc:/3ds/pokeStride/data/names_*.bin (those
 * files are extracted on first boot from the user's HGSS .nds — same
 * pattern as pokeicon). Returns NULL if the tables haven't been
 * loaded yet OR the id is out of range OR the slot is empty. Callers
 * should fall back to a numeric display in that case. */
const char *item_name(uint16_t id);
const char *move_name(uint16_t id);
const char *met_location_name(uint16_t loc);

/* Load the three name tables from SD. Idempotent; safe to call before
 * extraction if files don't exist (returns false in that case). */
bool names_init(void);

#endif
