/* personal_gen4.h - per-species personal data (gender ratio + growth rate).
 *
 * These two byte tables are runtime-loaded from
 * `sdmc:/3ds/pokeStride/data/personal.bin`, which the NDS extractor
 * (nds_extract.c::extract_personal) generates from the user's own HGSS
 * cartridge dump — the same first-boot extraction that produces the
 * sprite/name caches. No game-derived data is kept in the source tree;
 * the values come off the player's own ROM.
 *
 * Personal data is region-independent (a species' gender ratio / growth
 * rate is identical in every localisation), so the extracted values match
 * across JP / ES / EN / ... carts.
 *
 * Until personal_init() succeeds the tables hold safe neutral defaults
 * (50% gender ratio, Medium-Fast growth) so callers never crash; behaviour
 * is only fully correct once the cache is present.
 *
 * Indexing: 0 is the "no species" / placeholder slot. 1..493 are the
 * canonical gen 4 species. 494..507 are alternate forms per the NARC order.
 *
 * gender_ratio interpretation:
 *   0x00 = always male
 *   0xFE = always female
 *   0xFF = genderless
 *   else = threshold; female if (PID & 0xFF) < gender_ratio.
 *
 * growth_rate:
 *   0 Medium Fast   1 Erratic       2 Fluctuating
 *   3 Medium Slow   4 Fast          5 Slow
 */
#ifndef PERSONAL_GEN4_H
#define PERSONAL_GEN4_H
#include <stdint.h>
#include <stdbool.h>

#define PERSONAL_GEN4_COUNT 508

/* Runtime buffers (filled by personal_init from personal.bin; default
 * neutral until then). NOT const — populated at boot. */
extern uint8_t personal_gen4_gender_ratio[PERSONAL_GEN4_COUNT];
extern uint8_t personal_gen4_growth_rate [PERSONAL_GEN4_COUNT];

/* Load personal.bin into the tables. Validates the "PRS1" magic and a
 * known gender-ratio anchor (Bulbasaur). Returns true on success; on
 * failure the tables keep their neutral defaults and false is returned.
 * Idempotent. */
bool personal_init(void);

/* True once personal_init() has loaded a valid cache. */
bool personal_loaded(void);

/* Convenience accessors that clamp out-of-range species to safe defaults
 * (50% gender ratio, MedFast growth). */
static inline uint8_t personal_gender_ratio(uint16_t species) {
    return (species < PERSONAL_GEN4_COUNT) ? personal_gen4_gender_ratio[species] : 0x7F;
}
static inline uint8_t personal_growth_rate(uint16_t species) {
    return (species < PERSONAL_GEN4_COUNT) ? personal_gen4_growth_rate[species] : 0;
}

#endif
