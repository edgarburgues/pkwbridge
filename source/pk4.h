/*
 * pk4.h - Generation 4 Pokemon entity codec (136-byte stored format).
 *
 * Format: 8-byte header + 4 blocks (A, B, C, D) of 32 bytes each.
 *   At rest the four blocks are shuffled by sv = (PID >> 13) & 31, and
 *   their bytes are XOR'd with a per-byte LCG keyed by the checksum.
 *
 *   Decrypt:  CryptArray(blocks, chk) -> Shuffle45(blocks, sv)
 *   Encrypt:  Shuffle45(blocks, invert[sv]) -> CryptArray(blocks, chk)
 *
 * Algorithm is a transcription of PokeCrypto.cs (PKHeX, MIT). Verified
 * by round-trip: encrypt(decrypt(buf)) == buf for any valid input.
 */

#ifndef PK4_H
#define PK4_H

#include <stdint.h>
#include <stdbool.h>

#define PK4_SIZE_STORED 136

/* ---- Decrypted-PK4 field offsets (post-unshuffle) ---- */
#define PK4_OFF_PID         0x00  /* u32 LE */
#define PK4_OFF_CHECKSUM    0x06  /* u16 LE; computed sum of u16s 0x08..0x87 */

/* Block A (0x08..0x27, "growth") */
#define PK4_OFF_SPECIES     0x08  /* u16 LE */
#define PK4_OFF_HELD_ITEM   0x0A  /* u16 LE */
#define PK4_OFF_OT_TID      0x0C  /* u16 LE */
#define PK4_OFF_OT_SID      0x0E  /* u16 LE */
#define PK4_OFF_EXP         0x10  /* u32 LE */
#define PK4_OFF_FRIENDSHIP  0x14
#define PK4_OFF_ABILITY     0x15
#define PK4_OFF_MARKINGS    0x16
#define PK4_OFF_LANGUAGE    0x17
/* Block B (0x28..0x47, "moves") */
#define PK4_OFF_MOVE1       0x28  /* u16 LE x4 */
#define PK4_OFF_MOVE_PP     0x30
/* Block C (0x48..0x67, "EVs/contest/ribbons") */
#define PK4_OFF_NICKNAME    0x48  /* 22 bytes, gen4 char codes, term 0xFFFF */
#define PK4_OFF_NICKNAME_LEN 22
/* Block D (0x68..0x87, "misc"). Offsets cross-checked against PKHeX PK4.cs. */
#define PK4_OFF_OT_NAME       0x68  /* 16 bytes, gen4 char codes, term 0xFFFF */
#define PK4_OFF_OT_NAME_LEN   16
#define PK4_OFF_EGG_DATE      0x78  /* 3 bytes: year-since-2000, month, day */
#define PK4_OFF_MET_DATE      0x7B  /* 3 bytes: year-since-2000, month, day */
#define PK4_OFF_EGG_LOCATION  0x7E  /* u16 LE */
#define PK4_OFF_MET_LOCATION  0x80  /* u16 LE */
#define PK4_OFF_POKERUS       0x82
#define PK4_OFF_BALL          0x83
#define PK4_OFF_MET_LVL_GEND  0x84  /* bits 0-6 met level, bit 7 OT gender */
#define PK4_OFF_ENCOUNTER     0x85  /* HGSS encounter type byte */
#define PK4_OFF_LEVEL         0x8C  /* not in stored 136 — only in party 236 */

/* =====================================================================
 * Encrypt/decrypt the in-place 136-byte buffer.
 * `buf` must be exactly PK4_SIZE_STORED. After pk4_decrypt the four
 * blocks are in canonical (A,B,C,D) order; pk4_encrypt restores the
 * shuffled, XOR'd at-rest form.
 * ===================================================================== */

void pk4_decrypt(uint8_t buf[PK4_SIZE_STORED]);
void pk4_encrypt(uint8_t buf[PK4_SIZE_STORED]);

/* Sum of the 64 little-endian u16s at offsets 0x08..0x87. This is
 * stored at offset 0x06 of every valid PK4. After editing the body,
 * call pk4_recompute_checksum() before encrypting, or every read will
 * see garbage (the chk value also seeds the encryption LCG). */
uint16_t pk4_compute_checksum(const uint8_t buf[PK4_SIZE_STORED]);
void     pk4_recompute_checksum(uint8_t buf[PK4_SIZE_STORED]);

static inline uint32_t pk4_pid(const uint8_t *b) {
    return (uint32_t)b[0] | ((uint32_t)b[1]<<8) | ((uint32_t)b[2]<<16) | ((uint32_t)b[3]<<24);
}
static inline uint16_t pk4_u16(const uint8_t *b, int off) {
    return (uint16_t)b[off] | ((uint16_t)b[off+1]<<8);
}
static inline uint32_t pk4_u32(const uint8_t *b, int off) {
    return (uint32_t)b[off] | ((uint32_t)b[off+1]<<8)
         | ((uint32_t)b[off+2]<<16) | ((uint32_t)b[off+3]<<24);
}

/* Alternate form (0..31). Per PKHeX PK4.cs the form lives in the high 5
 * bits of byte 0x40 (the low 3 bits are gender). Returns 0 for any
 * Pokemon without alt forms. */
static inline uint8_t pk4_form(const uint8_t *b) {
    return (uint8_t)((b[0x40] >> 3) & 0x1F);
}

#endif /* PK4_H */
