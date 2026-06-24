/*
 * pweep.h - Pokewalker EEPROM (M95512, 64 KB) parser/editor.
 *
 * H8/38606 is big-endian. Multi-byte fields are stored BE in EEPROM.
 * Helpers below convert to/from native (3DS ARM11 = little-endian).
 *
 * "Reliable" data uses two copies + 1-byte checksum at the end.
 *   addrPair (u32): low16 = copy0 addr, high16 = copy1 addr.
 *   checksum byte = 0x01 + sum(data) mod 256, written at addr+size.
 */

#ifndef PWEEP_H
#define PWEEP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PWEEP_SIZE 0x10000   /* 64 KB */

/* ---- raw region offsets ---- */
#define PWEEP_OFF_MAGIC            0x0000   /* 1 B, presence check (!= 0xFF) */
#define PWEEP_OFF_IDENTITY8        0x0008   /* 8 B, boot identity (loaded to RAM 0xF7D8) */
#define PWEEP_OFF_WALK_DATA        0x8F00   /* 0xBE B, sent Pokemon + route + encounter tables */
#define PWEEP_WALK_DATA_SIZE       0x00BE
#define PWEEP_OFF_WALK_TABLES_EXT  0x91BE   /* 0x180 B, extended encounter/item tables */
#define PWEEP_WALK_TABLES_EXT_SZ   0x0180
#define PWEEP_OFF_TABLES_SOURCE    0x9D7E   /* 0x180 B, static source for tables */
#define PWEEP_OFF_SPECIAL_EVT      0xBF00   /* 0x7C B, special event data */
#define PWEEP_SPECIAL_EVT_SIZE     0x007C
#define PWEEP_OFF_CONFIG_PRIMARY   0xCC00   /* 0x280 B (5x128), config */
#define PWEEP_OFF_CONFIG_BACKUP    0xD480
#define PWEEP_CONFIG_SIZE          0x0280
#define PWEEP_OFF_CAUGHT_POKEMON   0xCE8C   /* 0x30 B, caught during walk */
#define PWEEP_CAUGHT_POKEMON_SIZE  0x0030
/* Dowsing find buffer: 0xCEBC = 0xCE8C + 0x30, immediately after the
 * caught Pokemon block.  13 entries of 4 bytes each (0x34 B total, ending
 * right before the daily step history at 0xCEF0):
 *   { u16 item_id LE, u16 _padding (always 0) }
 * Populated when the player accepts a dowsing item-find pop-up. The slot
 * allocator picks the first slot whose first u16 is 0, so 0 = empty. */
#define PWEEP_OFF_DOWSING_ITEMS    0xCEBC
#define PWEEP_DOWSING_ITEMS_SIZE   0x0034   /* 52 B total (13 x 4) */
#define PWEEP_DOWSING_SLOT_COUNT   13
#define PWEEP_DOWSING_SLOT_STRIDE  4
#define PWEEP_OFF_DAILY_STEPS      0xCEF0   /* 7 x u32 BE daily step counts,
                                             * rotated at midnight by the
                                             * firmware and cleared at
                                             * endWalkAction. */
#define PWEEP_DAILY_STEPS_SIZE     0x001C
#define PWEEP_OFF_ROUTE_FLAGS_BASE 0xCF0C   /* 24 entries, stride 0x88 */
#define PWEEP_ROUTE_FLAGS_STRIDE   0x0088
#define PWEEP_ROUTE_FLAGS_COUNT    0x18
#define PWEEP_OFF_TEAM_BACKUP      0xD700   /* 0x2900 B (82x128), backup of walk staging */
#define PWEEP_TEAM_BACKUP_SIZE     0x2900
#define PWEEP_OFF_DAILY_LOG_BASE   0xDC00   /* 10 blocks of 0x224 each, day-step log */
#define PWEEP_DAILY_LOG_STRIDE     0x0224
#define PWEEP_DAILY_LOG_COUNT      10

/* ---- reliable pairs (u32 addrPair: low16=copy0, high16=copy1) ----
 *
 * Note: the event-log-pointer byte at EEPROM 0x0153 is NOT an
 * independent reliable pair. It lies inside the pairing block (offset
 * 0x66 of the 0x68-byte block at 0x00ED) and is set by that block's
 * write path, so its standalone checksum never satisfies 1+data. Read
 * the value directly at data[0x0153] (== pairing_block[0x66]). */
#define PWEEP_REL_PAIRING          0x01ED00EDu  /* size 0x68: identity/pairing block */
#define PWEEP_REL_PAIRING_SIZE     0x0068
#define PWEEP_REL_HEALTH           0x02560156u  /* size 0x18: HealthData */
#define PWEEP_REL_HEALTH_SIZE      0x0018
#define PWEEP_REL_WALK_SENTINEL    0x026F016Fu  /* size 0x01: walk-in-progress (0xA5/0x00) */
#define PWEEP_REL_WALK_SENTINEL_SZ 0x0001
#define PWEEP_REL_WALK_IDENTITY    0x01830083u  /* size 0x28: peer-play walk identity */
#define PWEEP_REL_WALK_IDENT_SIZE  0x0028
#define PWEEP_REL_LCD_INIT         0x01AC00ACu  /* size 0x40: LCD init blob */
#define PWEEP_REL_LCD_INIT_SIZE    0x0040

#define PWEEP_OFF_EVTLOG_PTR       0x0153  /* raw byte; same as pairing[0x66] */

/* ---- offsets within the 190-byte walk_data buffer @ 0x8F00 ----
 * (Distinct from the `RouteInfo` struct, which describes the 192-byte
 *  buffer at RAM 0x7FB0; 0x8F00 has a different layout.) */
/* The walk_data buffer at 0x8F00 (190 B / 0xBE) IS the HGSS-internal
 * `PHCCourseValue` struct sent to the walker via IR. Layout:
 *
 *   PHCPokemonBase pokemon;       // 0x00..0x0F (16 B)
 *   u8 nickname[20+2];            // 0x10..0x25 (22 B, gen4 wide chars)
 *   u8 friendly;                  // 0x26
 *   u8 courseId;                  // 0x27
 *   u8 courseNameString[40+2];    // 0x28..0x51 (42 B gen4 chars)
 *   PHCPokemonBase Enemy[3];      // 0x52..0x81 (48 B = 3 x 16)
 *   u16 walkStep[3];              // 0x82..0x87 (steps before encounter)
 *   u8  encountRate[3];           // 0x88..0x8A (% per slot)
 *   u8  padding1[1];              // 0x8B
 *   u16 itemNumber[10];           // 0x8C..0x9F (dowsing item IDs)
 *   u16 itemStep[10];             // 0xA0..0xB3 (steps before item)
 *   u8  itemRate[10];             // 0xB4..0xBD (% per item)
 *
 * PHCPokemonBase layout (16 B, same for Enemy[i]):
 *   u16 id;       u16 item;       // 0x00..0x03
 *   u16 skill[4];                 // 0x04..0x0B (moves 0..3)
 *   u8  level;                    // 0x0C
 *   u8  folm:5 + sex:2 + isEgg:1; // 0x0D
 *   u8  reverseFlag:1 + rareColor:1 + padding:6; // 0x0E (rareColor=shiny)
 *   u8  padding[1];               // 0x0F
 */
#define WALKDATA_OFF_SPECIES        0x00  /* PHCPokemonBase.id (u16 LE) */
#define WALKDATA_OFF_HELD_ITEM      0x02  /* PHCPokemonBase.item (u16 LE) - usually 0
                                             in walker (zeroed by send path) */
#define WALKDATA_OFF_MOVE0          0x04  /* PHCPokemonBase.skill[0] (u16 LE) */
#define WALKDATA_OFF_MOVE1          0x06  /* PHCPokemonBase.skill[1] */
#define WALKDATA_OFF_MOVE2          0x08  /* PHCPokemonBase.skill[2] */
#define WALKDATA_OFF_MOVE3          0x0A  /* PHCPokemonBase.skill[3] */
#define WALKDATA_OFF_LEVEL          0x0C  /* u8 */
#define WALKDATA_OFF_FOLM_SEX_EGG   0x0D  /* bits 0-4 folm, 5-6 sex, 7 isEgg */
#define WALKDATA_OFF_REVERSE_SHINY  0x0E  /* bit 0 = reverseFlag (route-bg flip),
                                             bit 1 = rareColor (shiny). */
#define  WALKDATA_FLAG_REVERSE      (1u << 0)
#define  WALKDATA_FLAG_SHINY        (1u << 1)
/* 0x0F: padding[1] in PHCPokemonBase. */

#define WALKDATA_OFF_NICKNAME       0x10  /* 22 B (PHC_NICKNAME_SIZE+EOF). Gen4
                                             wide chars, FFFF-terminated. Verbatim
                                             from PK4 +0x48. */
#define WALKDATA_NICKNAME_SIZE      22
#define WALKDATA_OFF_FRIENDSHIP     0x26  /* u8 (PHCCourseValue.friendly) */
#define WALKDATA_OFF_COURSE_ID      0x27  /* u8 (PHCCourseValue.courseId) */
#define WALKDATA_OFF_COURSE_NAME    0x28  /* 42 B (PHC_COURSE_NAME_SIZE+EOF). Gen4
                                             wide chars. Per-course, not
                                             per-walker. */
#define WALKDATA_COURSE_NAME_SIZE   42
#define WALKDATA_OFF_ENEMY          0x52  /* PHCPokemonBase[3] - 3 wild encounter
                                             Pokemon, 16 B each (48 B total) */
#define WALKDATA_ENEMY_COUNT        3
#define WALKDATA_ENEMY_STRIDE       16
#define WALKDATA_OFF_WALK_STEP      0x82  /* u16[3] - steps required before each
                                             encounter becomes available */
#define WALKDATA_OFF_ENCOUNT_RATE   0x88  /* u8[3] - encounter probability % per slot */
/* 0x8B: padding[1] */
#define WALKDATA_OFF_ITEM_NUMBER    0x8C  /* u16[10] - dowsing item IDs */
#define WALKDATA_OFF_ITEM_STEP      0xA0  /* u16[10] - steps before each item */
#define WALKDATA_OFF_ITEM_RATE      0xB4  /* u8[10] - item find probability */

/* ---- offsets within the 0x68-byte pairing block ---- */
#define PAIRING_OFF_SESSION        0x04   /* u32 BE: session/sync timestamp */
#define PAIRING_OFF_UNK_0A         0x0A   /* u16 BE: cleared in endWalkAction */
#define PAIRING_OFF_FLAGS          0x5B   /* u8: bit0=paired, bit1=hasPokemon, bit2=cleared at walk-start */
#define  PAIRING_FLAG_PAIRED       (1u << 0)  /* identity[0x5B] bit0 */
#define  PAIRING_FLAG_HAS_POKEMON  (1u << 1)
#define  PAIRING_FLAG_BIT2         (1u << 2)

/* ---- offsets within the 0x18-byte HealthData (BE) ---- */
#define HEALTH_OFF_TOTAL_STEPS     0x00   /* u32 BE, max 9,999,999 */
#define HEALTH_OFF_IDENTITY_STEPS  0x04   /* u32 BE */
#define HEALTH_OFF_LAST_SYNC       0x08   /* u32 BE */
#define HEALTH_OFF_TOTAL_DAYS      0x0C   /* u16 BE */
#define HEALTH_OFF_CUR_WATTS       0x0E   /* u16 BE, max 9999 */

/* =====================================================================
 * Container.
 * ===================================================================== */

/* Per-pair status reported by pweep_validate():
 *   PWEEP_OK     - both copies sum-check correctly
 *   PWEEP_UNINIT - all bytes are 0xFF (fresh from factory; firmware
 *                  treats this as the default-zero case and works fine).
 *                  Notably evtlog_ptr is only written by logPeerPlay()
 *                  and walk_sentinel only by startWalkEepromActions(),
 *                  so they may legitimately be UNINIT on a real walker.
 *   PWEEP_BAD    - bytes are not all 0xFF and the checksum doesn't match.
 *                  Possibly corrupted; firmware would auto-repair on
 *                  next read. */
typedef enum {
    PWEEP_OK = 0,
    PWEEP_UNINIT,
    PWEEP_BAD,
} pweep_status_t;

typedef struct {
    uint8_t  data[PWEEP_SIZE];
    bool     loaded;
    pweep_status_t pairing;
    pweep_status_t health;
    pweep_status_t walk_sentinel;
    pweep_status_t walk_identity;
    pweep_status_t lcd_init;
} pweep_t;

/* =====================================================================
 * Lifecycle.
 * ===================================================================== */

/* Read PWEEP_SIZE bytes from `path` into `p->data`. Returns false on I/O
 * failure or wrong size. Does NOT validate checksums; call pweep_validate
 * after. */
bool pweep_load(pweep_t *p, const char *path);

/* Recompute every reliable-pair checksum (in case data was edited
 * directly) and write the buffer atomically: write `path.tmp`, fsync,
 * rename to `path`. Returns false on I/O failure. */
bool pweep_save(pweep_t *p, const char *path);

/* Run validation across all known reliable pairs; populates the *_ok
 * flags. Returns true if EVERY pair validates. */
bool pweep_validate(pweep_t *p);

/* =====================================================================
 * Reliable-pair access (mirrors the firmware's reliable read/write).
 * ===================================================================== */

/* Read a reliable pair into `out`. If both copies are valid, copy 0
 * wins. If only one is valid, returns that one. If both are corrupt,
 * returns false and leaves `out` untouched. */
bool pweep_read_reliable(const pweep_t *p, uint32_t addrPair,
                         uint16_t size, uint8_t *out);

/* Write `in` to both copies + recompute checksum byte at addr+size in
 * each. */
void pweep_write_reliable(pweep_t *p, uint32_t addrPair,
                          uint16_t size, const uint8_t *in);

/* =====================================================================
 * High-level helpers.
 * ===================================================================== */

bool     pweep_has_pokemon(const pweep_t *p);
void     pweep_set_has_pokemon(pweep_t *p, bool yes);

uint16_t pweep_get_watts(const pweep_t *p);
void     pweep_set_watts(pweep_t *p, uint16_t w);  /* clamped to 9999 */

uint32_t pweep_get_total_steps(const pweep_t *p);
uint16_t pweep_get_total_days(const pweep_t *p);

/* Returns pointer into p->data at 0x8F00 (190 bytes). Caller may read
 * directly. After write, call pweep_walk_data_commit() to mirror to the
 * 0xD700 backup region (matches copyTeamAndConfigData semantics). */
const uint8_t *pweep_walk_data(const pweep_t *p);
uint8_t       *pweep_walk_data_mut(pweep_t *p);
void           pweep_walk_data_commit(pweep_t *p);

/* Caught-Pokemon slots at 0xCE8C, 48 bytes (slot layout documented
 * below). Populated by the walker game engine. */
const uint8_t *pweep_caught_pokemon(const pweep_t *p);
void           pweep_clear_caught_pokemon(pweep_t *p);

/* Pairing/identity block (0x68 bytes). Returns a freshly-decoded copy
 * via pweep_read_reliable (in the local `out` arg). Use the FLAGS/SESSION
 * offsets above to read fields. */
bool pweep_read_pairing(const pweep_t *p, uint8_t out[PWEEP_REL_PAIRING_SIZE]);
void pweep_write_pairing(pweep_t *p, const uint8_t in[PWEEP_REL_PAIRING_SIZE]);

/* =====================================================================
 * `send` operation: write a Pokemon into the walker.
 *
 * Takes a DECRYPTED PK4 (136-byte stored form) + level + route_id, and
 * writes the Pokemon-specific bytes into walk_data (0x00..0x27), updates
 * the pairing-block flags + checksum, and sets the active-route byte at
 * 0xCF90. ROUTE-specific bytes in walk_data (0x28..0xBD) and the larger
 * "active route data tail" (0x8FBE..0xB7FF) are PRESERVED from the
 * current pweep state — they hold per-route encounter tables that we
 * don't yet know how to regenerate from scratch. For `send` to a fresh
 * route, copy that region from a template dump first.
 *
 * `level` is mandatory because the stored PK4 format doesn't include it
 * (it's derived at runtime from EXP+species growth curve). Pass the
 * party-form level (PK4 party offset 0x8C) or compute it elsewhere.
 *
 * `route_id` defaults to 0x19. Pass 0 to leave the existing route byte
 * untouched.
 *
 * Returns true on success. */
#define PWEEP_DEFAULT_ROUTE_ID 0x19
#define PWEEP_ROUTE_ID_OFF     0xCF90  /* byte that flips when a Pokemon is active */
bool pweep_send_pokemon(pweep_t *p,
                        const uint8_t *pk4_dec,   /* 136 B, PK4_SIZE_STORED */
                        uint8_t level,
                        uint8_t route_id);

/* Compute level from a decrypted PK4 (using species growth curve + EXP).
 * This matches what HGSS computes at send-time; the party-form level
 * field at +0x8C is just a cache. */
uint8_t pweep_pk4_level(const uint8_t *pk4_dec);

/* =====================================================================
 * Caught Pokemon (post-walk) at 0xCE8C..0xCEBB
 *
 * 3 slots of 16 B each (PHCPokemonBase struct, same as walk_data 0x00..0x0F):
 *   0x00 u16 species   (0 = empty slot)
 *   0x02 u16 held_item
 *   0x04 u16 move[0..3]
 *   0x0C u8  level
 *   0x0D u8  folm(5)|sex(2)|isEgg(1)
 *   0x0E u8  reverseFlag(1)|rareColor(1)|padding
 *   0x0F u8  padding
 *
 * Walker stores in little-endian (HGSS doesn't byte-swap on RX). HGSS
 * IGNORES rareColor — claimed Pokemon are never shiny regardless of
 * walker's flag. Item is preserved.
 * ===================================================================== */

#define PWEEP_CAUGHT_SLOT_COUNT  3
#define PWEEP_CAUGHT_SLOT_SIZE   16

typedef struct {
    uint16_t species;     /* 0 = empty */
    uint16_t held_item;
    uint16_t move[4];
    uint8_t  level;
    uint8_t  folm;        /* form id 0..31 */
    uint8_t  sex;         /* 0=male, 1=female, 2=genderless */
    uint8_t  is_egg;      /* always 0 from walker (catches aren't eggs) */
    uint8_t  reverse_flag;
    uint8_t  rare_color;  /* informational; HGSS ignores it */
} pweep_caught_t;

bool pweep_caught_get(const pweep_t *p, int slot /*0..2*/, pweep_caught_t *out);
bool pweep_caught_is_empty(const pweep_t *p, int slot);
int  pweep_caught_count(const pweep_t *p);

/* Clear all 3 slots (mirrors eepromClearCaughtPokemonSlots; firmware
 * does this at next startWalkEepromActions). */
void pweep_caught_clear_all(pweep_t *p);

/* =====================================================================
 * Dowsing finds buffer at 0xCEBC (0x34 B, 13 entries x 4 bytes).
 *
 * The walker stores up to 13 item-find events here between IR syncs.
 * Each entry is a u16 item_id (gen-4 IDs, LE) + u16 padding (=0).
 * `pweep_dowsing_get_at` returns item_id or 0 for empty/oor slots.
 * `pweep_dowsing_count` counts non-zero slots.
 * `pweep_dowsing_clear` zeros all 12 bytes — mirrors what the IR-sync
 * ACK would do on the walker side; call after routing the items into
 * the HGSS bag.
 * ===================================================================== */
uint16_t pweep_dowsing_get_at(const pweep_t *p, int slot /*0..2*/);
int      pweep_dowsing_count(const pweep_t *p);
void     pweep_dowsing_clear(pweep_t *p);

/* Detach the walker's currently-attached Pokemon. The minimal reset
 * that makes the walker ready to accept a new Send:
 *   - pairing.flags bit 1 (has_pokemon) cleared.
 *   - walk_data Pokemon bytes (0x00..0x26) zeroed; route_id at +0x27
 *     is preserved so the walker UI keeps the last-walked route.
 *
 * NOT a full endWalkAction emulation — that firmware function also
 * wipes daily_steps, route encounter sentinels, pairing session_ts,
 * unk_0A, and pairing flag bit 2. Those belong to "user picks 'end
 * walk' on the device", which is a depair-style hard reset, not a
 * sync. claim+return is a sync, so we only touch the fields that need
 * to change for the next Send to work. */
void pweep_detach_pokemon(pweep_t *p);

/* Walker session stats (from HealthData reliable pair):
 *   total_steps : u32 BE, max 9,999,999 — absolute lifetime walker total
 *   total_watts : u16 BE, max 9999 — current accumulated watts to claim
 *   total_days  : u16 BE — walker uptime days
 */
typedef struct {
    uint32_t total_steps;
    uint16_t total_watts;
    uint16_t total_days;
} pweep_walker_stats_t;

bool pweep_walker_stats_get(const pweep_t *p, pweep_walker_stats_t *out);

/* =====================================================================
 * Endianness helpers (EEPROM is big-endian, ARM11 is little-endian).
 * ===================================================================== */

static inline uint16_t pweep_rd16be(const uint8_t *p) {
    return ((uint16_t)p[0] << 8) | p[1];
}
static inline uint32_t pweep_rd32be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}
static inline void pweep_wr16be(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v;
}
static inline void pweep_wr32be(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8); p[3] = (uint8_t)v;
}

#endif /* PWEEP_H */
