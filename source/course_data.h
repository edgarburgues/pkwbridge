/* course_data.h - 27 Pokewalker courses x (3 enemies + 10 dowsing items)
 * IDs are canonical (National Dex for species, game item IDs, game move IDs).
 */
#ifndef COURSE_DATA_H
#define COURSE_DATA_H

#include <stdint.h>

#define COURSE_COUNT     27
#define COURSE_ENEMIES   3
#define COURSE_ITEMS     10

typedef struct {
    uint16_t species;      /* National Pokedex no.; 0 = none */
    uint8_t  level;
    uint16_t held_item;    /* game item id; 0 = none */
    uint16_t moves[4];     /* canonical move ids; 0 = none */
    uint8_t  form;
    uint8_t  sex;          /* 0=male, 1=female, 2=genderless (per coursedata) */
    uint16_t need_step;    /* steps required before this enemy can appear */
    uint8_t  odds;         /* spawn probability weight */
} course_enemy_t;

typedef struct {
    uint16_t item_no;      /* game item id; 0 = none */
    uint16_t need_step;
    uint8_t  odds;
} course_item_t;

typedef struct {
    uint8_t        graphic_id;   /* 1..8 -> course_bitmaps[graphic_id-1] */
    uint8_t        course_id;    /* 1..27 */
    const char    *name_string;  /* UTF-8 Japanese course name (replace with localised text if available) */
    course_enemy_t enemies[COURSE_ENEMIES];
    course_item_t  items[COURSE_ITEMS];
} course_def_t;

extern const course_def_t course_defs[COURSE_COUNT];

#endif /* COURSE_DATA_H */
