/*
 * exp_curve.h - Gen4 EXP-to-level + level-to-EXP conversion.
 *
 * PK4 stores EXP (u32 LE at +0x10) but does NOT store level directly in
 * its 136-byte form (party form has level cached at +0x8C, but party is
 * derived from stored). HGSS computes the level from EXP at runtime
 * using the species's growth_rate from personal data.
 *
 * 6 growth curves (gen 4 enum order, matches personal data byte +0x13):
 *   0  Medium Fast    n^3                          (max EXP 1,000,000)
 *   1  Erratic        piecewise                    (max     600,000)
 *   2  Fluctuating    piecewise                    (max   1,640,000)
 *   3  Medium Slow    6n^3/5 - 15n^2 + 100n - 140  (max   1,059,860)
 *   4  Fast           4n^3/5                       (max     800,000)
 *   5  Slow           5n^3/4                       (max   1,250,000)
 *
 * Formulas from Bulbapedia (Experience#Experience_groups), cross-checked
 * against PKHeX's Experience.cs.
 */

#ifndef EXP_CURVE_H
#define EXP_CURVE_H

#include <stdint.h>

/* Minimum EXP required to be AT exactly `level` for a given growth rate. */
uint32_t exp_for_level(uint8_t growth_rate, uint32_t level);

/* Current level given total EXP and growth rate. Returns 1..100. */
uint8_t level_from_exp(uint8_t growth_rate, uint32_t exp);

#endif
