/*
 * exp_curve.c - Gen4 EXP/level conversion. See header for curve list.
 *
 * Implementation uses runtime formulas with 64-bit intermediate math
 * instead of a 6x101 lookup table. Spot-checks:
 *   - Medium Fast lvl 50  -> 125,000
 *   - Slow lvl 50         -> 156,250
 *   - Medium Slow lvl 5   -> 135
 *   - Fluctuating lvl 100 -> 1,640,000
 *   - Erratic lvl 100     -> 600,000
 */

#include "exp_curve.h"

uint32_t exp_for_level(uint8_t growth_rate, uint32_t n) {
    if (n <= 1) return 0;
    if (n > 100) n = 100;
    uint64_t n3 = (uint64_t)n * n * n;

    switch (growth_rate) {
    case 0: /* Medium Fast: n^3 */
        return (uint32_t)n3;

    case 1: { /* Erratic — piecewise */
        uint64_t v;
        if (n <= 50)      v = n3 * (100 - n) / 50;
        else if (n <= 68) v = n3 * (150 - n) / 100;
        else if (n <= 98) v = n3 * ((1911 - 10 * n) / 3) / 500;
        else              v = n3 * (160 - n) / 100;
        return (uint32_t)v;
    }

    case 2: { /* Fluctuating — piecewise */
        uint64_t v;
        if (n <= 15)      v = n3 * (((n + 1) / 3) + 24) / 50;
        else if (n <= 36) v = n3 * (n + 14) / 50;
        else              v = n3 * ((n / 2) + 32) / 50;
        return (uint32_t)v;
    }

    case 3: /* Medium Slow: 6n^3/5 - 15n^2 + 100n - 140 */
        /* For n>=2 the formula is well-defined (lvl 2 = 9, lvl 3 = 57). */
        return (uint32_t)((6 * n3 / 5) - 15 * n * n + 100 * n - 140);

    case 4: /* Fast: 4n^3/5 */
        return (uint32_t)(4 * n3 / 5);

    case 5: /* Slow: 5n^3/4 */
        return (uint32_t)(5 * n3 / 4);
    }
    return (uint32_t)n3; /* fallback */
}

uint8_t level_from_exp(uint8_t growth_rate, uint32_t exp) {
    /* Scan from level 100 downward; first level whose threshold ≤ exp wins. */
    for (uint32_t l = 100; l >= 1; l--) {
        if (exp >= exp_for_level(growth_rate, l)) return (uint8_t)l;
    }
    return 1;
}
