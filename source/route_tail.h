/*
 * route_tail.h - Regenerate the 10240-byte "route tail" of walker EEPROM
 * (offset 0x8FBE..0xB7FF), which is HGSS's PHCCourseData minus the
 * 190-byte PHCCourseValue we already write at 0x8F00.
 *
 * Layout (10240 B total):
 *
 *   offset  size  field
 *   0x0000   192  course[192]            route background bitmap
 *   0x00C0   320  courseName[320]        route name glyph (80×16, 2-plane)
 *   0x0200   384  imagePoke[384]         walked Pokemon icon (32×24×2 frames?)
 *   0x0380  1536  imagePokeBig[1536]     hi-res Pokemon portrait (64×48 ×2)
 *   0x0980   320  pokeName[320]          nickname glyph
 *   0x0AC0  1152  imageEnemy[3*384]      3 enemy icons
 *   0x0F40  1536  imageEnemyBig[1536]    enemy[2] hi-res portrait
 *   0x1540   960  enemyName[3*320]       3 enemy species name glyphs
 *   0x1900  3840  item[10*384]           10 dowsing item name glyphs
 *
 * Sprites and the course bitmap are injected from the extracted NARCs;
 * text glyphs are regenerated when the font is loaded, otherwise they
 * are preserved from the input template.
 */

#ifndef ROUTE_TAIL_H
#define ROUTE_TAIL_H

#include <stdint.h>
#include <stdbool.h>

#define ROUTE_TAIL_SIZE             10240
#define ROUTE_TAIL_OFF_COURSE       0x0000
#define ROUTE_TAIL_OFF_COURSE_NAME  0x00C0
#define ROUTE_TAIL_OFF_POKE_ICON    0x0200
#define ROUTE_TAIL_OFF_POKE_BIG     0x0380
#define ROUTE_TAIL_OFF_POKE_NAME    0x0980
#define ROUTE_TAIL_OFF_ENEMY_ICON   0x0AC0  /* +i*384 for i in [0..2] */
#define ROUTE_TAIL_OFF_ENEMY_BIG    0x0F40
#define ROUTE_TAIL_OFF_ENEMY_NAME   0x1540  /* +i*320 for i in [0..2] */
#define ROUTE_TAIL_OFF_ITEM_NAME    0x1900  /* +i*384 for i in [0..9] */

#define ROUTE_TAIL_SIZE_POKE_ICON   384
#define ROUTE_TAIL_SIZE_POKE_BIG    1536
#define ROUTE_TAIL_SIZE_NAME        320
#define ROUTE_TAIL_SIZE_ITEM_NAME   384

/* Per-Pokemon render parameters extracted from the PK4 being sent. */
typedef struct {
    uint16_t species;
    uint8_t  form;
    uint8_t  sex;          /* 0=male, 1=female, 2=genderless */
    bool     shiny;
    const uint8_t *nickname22;  /* PK4 nickname raw (22 B, gen4 wide chars) */
    uint32_t pid;          /* needed for Spinda spot mixer */
} route_tail_pokemon_t;

/* Per-course render parameters. */
typedef struct {
    uint8_t        course_id;   /* 1..27 */
    uint8_t        graphic_id;  /* 1..8 — which background bitmap */
    /* For now the enemy/item info gets baked into both PHCCourseValue.v
     * AND the route_tail sprites/names. The caller fills these from the
     * course_def_t table. */
    uint16_t enemy_species[3];
    uint8_t  enemy_sex[3];
    uint8_t  enemy_form[3];
    uint16_t item_id[10];
} route_tail_course_t;

/* Render the 10240-byte route tail into `out`. `template_tail` is used as
 * the fallback for anything not regenerated; pass NULL to zero-fill those.
 * Returns true on success. */
bool route_tail_render(const route_tail_pokemon_t *pkmn,
                       const route_tail_course_t *course,
                       const uint8_t *template_tail /* 10240 B or NULL */,
                       uint8_t *out /* 10240 B */);

#endif
