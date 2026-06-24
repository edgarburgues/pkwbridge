/*
 * hgss_sav.h - HeartGold/SoulSilver .sav (NDS) parser/editor.
 *
 * Layout (524288 B = 2 partitions of 0x40000):
 *   Each partition has TWO embedded blocks:
 *     General: bytes 0..0xF628                 (0xF628 size)
 *     Gap:     0xF628..0xF700                  (0xD8 padding)
 *     Storage: bytes 0xF700..(0xF700+0x12310)  (0x12310 size)
 *   Both blocks end with a 0x10-byte footer; CRC16-CCITT (PKHeX byte-by-byte
 *   variant) of data[..^0x10] is stored at data[^2..^0]. Magic at [^8..^4].
 *
 * Active partition = the one whose footer counter is highest.
 *
 * Pokewalker block within General (cross-checked against pret/pokeheartgold
 * `struct POKEWALKER` and against a real Spanish HGSS .sav).
 * Total block size 0x130 = 304 B. The BoxPokemon starts at offset 0 in
 * real saves:
 *
 *   0xE5E0  +0x000  BoxPokemon (PK4 stored, 136 B) - sent Pokemon
 *   0xE668  +0x088  filler                          108 B
 *   0xE6D4  +0x0F4  unk_0F8  u16  course/state
 *   0xE6D6  +0x0F6  unk_0FA  u16  BOOL flag
 *   0xE6D8  +0x0F8  identity[10]  10 x u32 = 40 B  pairing identity
 *                                  (random at new game)
 *   0xE700  +0x120  counter  u16
 *   0xE702  +0x122  filler   2 B
 *   0xE704  +0x124  WATTS    u32
 *   0xE708  +0x128  lifetime walker steps u32  cap 9,999,999
 *   0xE70C  +0x12C  unlockedCourses  u32  bitmask of 32 courses
 *
 * (PKHeX SAV4HGSS.cs labels these last three differently - it calls
 * 0xE704 "Steps", 0xE708 "Watts", 0xE70C "Courses". The in-game order is
 * watts/steps/courses as above.)
 */

#ifndef HGSS_SAV_H
#define HGSS_SAV_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define HGSS_SAV_SIZE       0x80000   /* 524288 = 2 partitions */
#define HGSS_PART_SIZE      0x40000   /* 262144 per partition */
#define HGSS_GENERAL_SIZE   0xF628
#define HGSS_GENERAL_GAP    0x00D8
#define HGSS_STORAGE_OFF    0xF700    /* relative to partition start */
#define HGSS_STORAGE_SIZE   0x12310
#define HGSS_FOOTER_SIZE    0x10
#define HGSS_FOOTER_CTR_OFF 0x14      /* major+minor counter at -0x14 from end */

#define HGSS_MAGIC_INTL     0x20060623u
#define HGSS_MAGIC_KOR      0x20070903u

/* Pokewalker offsets within General */
#define HGSS_WALKER_BASE         0xE5E0  /* struct start (= PK4 start) */
#define HGSS_WALKER_PAIR_SIZE    0x0124  /* up to (not including) WATTS */
#define HGSS_WALKER_TOTAL_SIZE   0x0130  /* end of struct: BASE + 0x130 */
/* offsets within the walker block, struct-relative */
#define HGSS_WALKER_OFF_PK4      0x000   /* BoxPokemon, 136 B */
#define HGSS_WALKER_OFF_FILLER1  0x088   /* 108 B */
#define HGSS_WALKER_OFF_UNK_0F8  0x0F4   /* u16 - course/state */
#define HGSS_WALKER_OFF_UNK_0FA  0x0F6   /* u16 - BOOL */
#define HGSS_WALKER_OFF_IDENTITY 0x0F8   /* 10 x u32 - pairing identity */
#define HGSS_WALKER_IDENTITY_SZ  0x028   /* 40 B */
#define HGSS_WALKER_OFF_COUNTER  0x120   /* u16 + 2 B padding */
#define HGSS_WALKER_OFF_WATTS    0x124   /* u32 LE */
#define HGSS_WALKER_OFF_STEPS    0x128   /* u32 LE, lifetime walker steps */
#define HGSS_WALKER_OFF_COURSES  0x12C   /* u32 LE, bitmask */
/* absolute General-relative offsets (= base + struct offset) */
#define HGSS_WALKER_PAIR         (HGSS_WALKER_BASE + HGSS_WALKER_OFF_PK4)
#define HGSS_WALKER_WATTS        (HGSS_WALKER_BASE + HGSS_WALKER_OFF_WATTS)
#define HGSS_WALKER_STEPS        (HGSS_WALKER_BASE + HGSS_WALKER_OFF_STEPS)
#define HGSS_WALKER_COURSES      (HGSS_WALKER_BASE + HGSS_WALKER_OFF_COURSES)

/* PHC deposit state. The deposit struct (flag, toBox, hidePoke, ...)
 * starts 4 bytes before the BoxPokemon, so the deposit flag + return-box
 * sit immediately before HGSS_WALKER_PAIR. The PK4 (hidePoke) stays at
 * HGSS_WALKER_PAIR = BASE; the tail counters (steps/watts/courses) follow.
 * NOTE: offsets confirmed for EU 1.1 (Spanish). May shift by version/region. */
#define HGSS_WALKER_DEPOSIT_FLAG (HGSS_WALKER_BASE - 4)  /* General-rel, u16 LE */
#define HGSS_WALKER_DEPOSIT_TOBOX (HGSS_WALKER_BASE - 2) /* General-rel, u16 LE */
enum {
    HGSS_PHC_NO_PLUG   = 0,  /* never linked to a PokeWalker */
    HGSS_PHC_NO_DEPSIT = 1,  /* linked, no Pokemon currently out */
    HGSS_PHC_DEPOSIT   = 2,  /* a Pokemon is out walking */
};

/* Trainer info */
#define HGSS_TRAINER1_OFF       0x0064  /* per PKHeX SAV4HGSS.GetSAVOffsets() */

/* =====================================================================
 * HGSS item bags (General-block-relative offsets, per PKHeX SAV4HGSS.cs).
 *
 * Each bag is an array of slots: { u16 item_id LE, u16 count LE }.
 * count==0 OR item_id==0 means empty slot.  When adding an item, prefer
 * incrementing an existing slot with the same item_id (capped at 999),
 * otherwise write to the first empty slot.
 *
 * Slot caps below are PKHeX's `LegalItems` lengths for HGSS — the bag
 * region in the sav is sized for exactly this many slots.  Writing past
 * the cap would overlap the next bag.
 *
 * The "Items" pouch (HeldItem) is the catch-all for general-purpose
 * items (potions live in Medicine, balls in Balls, etc. — see the
 * gen4_item_bag() classifier in claim.c).
 * ===================================================================== */
typedef enum {
    HGSS_BAG_ITEMS    = 0,  /* general items: held items, sellables, ... */
    HGSS_BAG_KEY      = 1,
    HGSS_BAG_TMHM     = 2,
    HGSS_BAG_MEDICINE = 3,  /* potions, ethers, status heals, revives */
    HGSS_BAG_BERRIES  = 4,
    HGSS_BAG_BALLS    = 5,
    HGSS_BAG_BATTLE   = 6,
    HGSS_BAG_MAIL     = 7,
    HGSS_BAG_COUNT
} hgss_bag_kind_t;

/* General-relative pouch offsets (verified against PKHeX SAV4HGSS.cs). */
#define HGSS_BAG_ITEMS_OFF      0x0644
#define HGSS_BAG_KEY_OFF        0x08E4
#define HGSS_BAG_TMHM_OFF       0x09F4
#define HGSS_BAG_MEDICINE_OFF   0x0BEC
#define HGSS_BAG_BERRIES_OFF    0x0C4C
#define HGSS_BAG_BALLS_OFF      0x0CEC
#define HGSS_BAG_BATTLE_OFF     0x0D2C
#define HGSS_BAG_MAIL_OFF       0x0D9C

/* Slot caps (PKHeX LegalItems lengths). Each slot is 4 bytes. */
#define HGSS_BAG_ITEMS_CAP      165
#define HGSS_BAG_KEY_CAP         43
#define HGSS_BAG_TMHM_CAP       108
#define HGSS_BAG_MEDICINE_CAP    38
#define HGSS_BAG_BERRIES_CAP     64
#define HGSS_BAG_BALLS_CAP       16
#define HGSS_BAG_BATTLE_CAP      28
#define HGSS_BAG_MAIL_CAP        12

#define HGSS_BAG_ITEM_MAX_COUNT  999  /* per-slot cap, gen4 standard */

/* =====================================================================
 * PC box storage layout (offsets RELATIVE to the start of the 0x12310-B
 * Storage block returned by hgss_storage()).
 *
 * Verified against PKHeX SAV4HGSS.cs (lines 34, 102-121, 145) and
 * SAV4.cs (lines 88, 93). HGSS-specific: do NOT reuse for DP/Pt.
 * ===================================================================== */
#define HGSS_BOX_DATA_OFF       0x0000  /* box 0, slot 0 (PKHeX BoxOffset=0) */
#define HGSS_BOX_SIZE_PK4       136     /* PokeCrypto.SIZE_4STORED */
#define HGSS_BOX_SLOTS          30      /* per box */
#define HGSS_BOX_COUNT          18      /* HGSS */
#define HGSS_BOX_PAYLOAD_LEN    (HGSS_BOX_SLOTS * HGSS_BOX_SIZE_PK4) /* 0xFE0 */
/* Per-box stride is padded to 0x1000 (PKHeX GetBoxOffset = box * 0x1000).
 * That leaves 0x20 of padding/footer per box after the 0xFE0 of payload. */
#define HGSS_BOX_STRIDE         0x1000

/* Header right after the 18 boxes: current-box pointer + flags. */
#define HGSS_CURRENT_BOX_OFF    0x12000 /* u8: index of last-used box */
#define HGSS_BOX_FLAGS_OFF      0x12001 /* u8: BoxFlags (see PKHeX) */
#define HGSS_BOX_CHANGED_FLAGS  0x12004 /* u32: BoxContentChanged */

/* Box names: 18 entries, 40 bytes each (20 UCS-2 wide chars; max 8 used). */
#define HGSS_BOX_NAMES_OFF      0x12008
#define HGSS_BOX_NAME_LEN       40
#define HGSS_BOX_NAMES_TOTAL    (HGSS_BOX_COUNT * HGSS_BOX_NAME_LEN) /* 0x2D0 */

/* Wallpapers: 18 bytes, one per box. Values >= 0x10 are HGSS "Primo
 * phrase" specials; PKHeX subtracts 0x10 on read. */
#define HGSS_BOX_WALLPAPERS_OFF 0x122D8
#define HGSS_BOX_WP_FLAGS_OFF   0x122EA  /* u8: trailing flags (BOX_FLAGS) */
/* 0x122EB..0x1230F: ~37 B trailing (padding + 2-byte CRC footer at end). */

/* =====================================================================
 * Container.
 * ===================================================================== */

typedef enum { HGSS_OK = 0, HGSS_UNINIT, HGSS_BAD } hgss_status_t;

typedef struct {
    uint8_t  data[HGSS_SAV_SIZE];
    bool     loaded;

    /* Which partition (0 or 1) holds the active general / storage block. */
    int      active_general_part;   /* 0 or 1 */
    int      active_storage_part;   /* 0 or 1 */

    /* Status of computed CRCs against the stored footer values. */
    hgss_status_t general_status;
    hgss_status_t storage_status;

    /* Magic value (should be HGSS_MAGIC_INTL or HGSS_MAGIC_KOR). */
    uint32_t general_magic;
    uint32_t storage_magic;
} hgss_sav_t;

/* =====================================================================
 * Lifecycle.
 * ===================================================================== */

bool hgss_load(hgss_sav_t *s, const char *path);

/* Recompute footer CRCs in the active blocks (and optionally bump the
 * minor counter so the saved partition becomes "newer" than the other).
 * Atomically replaces `path`. */
bool hgss_save(hgss_sav_t *s, const char *path, bool bump_counter);

/* Validate footer CRCs, populate the *_status fields. */
void hgss_validate(hgss_sav_t *s);

/* =====================================================================
 * Block accessors. All return pointers INTO s->data, so writes via
 * these spans modify the live save. After writing, call hgss_save().
 * ===================================================================== */

uint8_t       *hgss_general(hgss_sav_t *s);          /* HGSS_GENERAL_SIZE bytes */
const uint8_t *hgss_general_const(const hgss_sav_t *s);
uint8_t       *hgss_storage(hgss_sav_t *s);          /* HGSS_STORAGE_SIZE bytes */
const uint8_t *hgss_storage_const(const hgss_sav_t *s);

/* WalkerPair: 292 bytes at General[0xE5E0]. Exposes the raw region. */
uint8_t       *hgss_walker_pair(hgss_sav_t *s);
const uint8_t *hgss_walker_pair_const(const hgss_sav_t *s);

uint32_t hgss_walker_steps(const hgss_sav_t *s);
void     hgss_set_walker_steps(hgss_sav_t *s, uint32_t v);
uint32_t hgss_walker_watts(const hgss_sav_t *s);
void     hgss_set_walker_watts(hgss_sav_t *s, uint32_t v);
uint32_t hgss_walker_courses(const hgss_sav_t *s);
void     hgss_set_walker_courses(hgss_sav_t *s, uint32_t mask);

/* PHC deposit flag (HGSS_PHC_NO_PLUG/NO_DEPSIT/DEPOSIT) + return box.
 * Setting DEPOSIT is what makes the game show "a Pokemon is out on your
 * PokeWalker". Call after writing the PK4 to hgss_walker_pair(). */
uint16_t hgss_walker_deposit_flag(const hgss_sav_t *s);
void     hgss_set_walker_deposit(hgss_sav_t *s, uint16_t flag, uint16_t tobox);

/* Trainer ID / Secret ID (LE u16 each, at trainer1+0x10 / +0x12). */
uint16_t hgss_tid(const hgss_sav_t *s);
uint16_t hgss_sid(const hgss_sav_t *s);

/* Player name (16 B, gen4 wide chars, FFFF-terminated) at trainer1+0x00.
 * Same byte format as PK4 OT name (+0x68), so copying is verbatim. */
#define HGSS_OT_NAME_LEN  16
void hgss_ot_name(const hgss_sav_t *s, uint8_t out[HGSS_OT_NAME_LEN]);

/* OT gender at trainer1+0x18: 0 = male, 1 = female. */
uint8_t hgss_ot_gender(const hgss_sav_t *s);

/* OT language at trainer1+0x19 (HGSS): 1=JPN, 2=ENG, 3=FRE, 4=ITA,
 *   5=GER, 7=SPA, 8=KOR. Used in PK4 language byte +0x17.
 *   Defaults to English (2) if the byte is uninitialized (out of range). */
uint8_t hgss_ot_language(const hgss_sav_t *s);

/* PC box slot access. `box` is [0..HGSS_BOX_COUNT), `slot` is
 * [0..HGSS_BOX_SLOTS). Returns NULL if out of range; otherwise points
 * into the live save buffer at the stored PK4 (136 B, encrypted). Use
 * pk4_decrypt on a copy to read fields. */
const uint8_t *hgss_box_slot_const(const hgss_sav_t *s, int box, int slot);
uint8_t       *hgss_box_slot(hgss_sav_t *s, int box, int slot);

/* Box name (40 B, UCS-2). NULL if out of range. */
const uint8_t *hgss_box_name_const(const hgss_sav_t *s, int box);
uint8_t       *hgss_box_name(hgss_sav_t *s, int box);

/* Wallpaper byte (0..15 standard, >=0x10 = HGSS Primo specials). 0 if oor. */
uint8_t  hgss_box_wallpaper(const hgss_sav_t *s, int box);
void     hgss_set_box_wallpaper(hgss_sav_t *s, int box, uint8_t wp);

uint8_t  hgss_current_box(const hgss_sav_t *s);
void     hgss_set_current_box(hgss_sav_t *s, uint8_t box);

/* Check if a slot is empty. PK4 stored format: empty slot = first
 * 16 bytes (PID..species) all zero. Convenience for `claim` op,
 * which needs to find the first empty slot. */
bool hgss_slot_is_empty(const hgss_sav_t *s, int box, int slot);

/* Find the first empty slot scanning boxes/slots in order. Returns
 * true and writes box/slot indices on success; false if storage is
 * full. Used by `claim` to deposit caught Pokemon. */
bool hgss_find_first_empty_slot(const hgss_sav_t *s, int *box, int *slot);

/* =====================================================================
 * Item bag write API.
 *
 * Add `count` of `item_id` to the requested bag.  Behaviour matches the
 * gen-4 game engine:
 *   - If a slot already holds `item_id`, increment its count (capped at
 *     HGSS_BAG_ITEM_MAX_COUNT = 999).
 *   - Otherwise, write {item_id, count} into the first empty slot
 *     (item_id == 0 OR count == 0 = empty).
 *   - If the bag is full (no existing slot AND no empty slot), returns
 *     false and the bag is unchanged.
 *
 * Returns true on success.  Caller is responsible for calling hgss_save
 * afterwards to commit + recompute the footer CRC.
 * ===================================================================== */
bool hgss_bag_add(hgss_sav_t *s, hgss_bag_kind_t bag,
                  uint16_t item_id, uint16_t count);

/* =====================================================================
 * Helpers.
 * ===================================================================== */

uint16_t hgss_crc16_ccitt(const uint8_t *data, size_t n);

#endif /* HGSS_SAV_H */
