/*
 * pk4.c - Gen4 Pokemon codec. See pk4.h.
 *
 * Algorithm transcribed from PokeCrypto.cs (kwsch/PKHeX, MIT).
 */

#include "pk4.h"
#include <string.h>

/* 24 permutations of {0,1,2,3} indexed by sv = (PID>>13)&31 mod 24.
 * pos[sv*4 + i] = index of the block that ENDS UP at slot i in shuffled
 * form. Equivalent to "shuffled[i] = original[pos[i]]". */
static const uint8_t BLOCK_POS[] = {
    0,1,2,3,  0,1,3,2,  0,2,1,3,  0,3,1,2,
    0,2,3,1,  0,3,2,1,  1,0,2,3,  1,0,3,2,
    2,0,1,3,  3,0,1,2,  2,0,3,1,  3,0,2,1,
    1,2,0,3,  1,3,0,2,  2,1,0,3,  3,1,0,2,
    2,3,0,1,  3,2,0,1,  1,2,3,0,  1,3,2,0,
    2,1,3,0,  3,1,2,0,  2,3,1,0,  3,2,1,0,
};

/* Inverse permutation: applying Shuffle45 with this index UNDOES the
 * shuffle that BLOCK_POS at the same sv would apply. Used by encrypt. */
static const uint8_t BLOCK_POS_INVERT[24] = {
     0, 1, 2, 4,  3, 5, 6, 7,
    12,18,13,19,  8,10,14,20,
    16,22, 9,11, 15,21,17,23,
};

/* In-place LCG XOR: pairs of bytes XOR'd with the high 16 bits of an
 * advancing 32-bit LCG (multiplier 0x41C64E6D, increment 0x6073).
 * Same primitive used for encrypted Pokemon and party-stats area. */
static void crypt_array(uint8_t *buf, size_t n, uint32_t key) {
    uint32_t seed = key;
    for (size_t i = 0; i + 1 < n; i += 2) {
        seed = seed * 0x41C64E6Du + 0x6073u;
        uint16_t x = (uint16_t)(buf[i] | (buf[i+1] << 8));
        x ^= (uint16_t)(seed >> 16);
        buf[i]     = (uint8_t)x;
        buf[i + 1] = (uint8_t)(x >> 8);
    }
}

/* Reorder the four 32-byte blocks. After: out_block[i] = in_block[pos[i]] */
static void shuffle45(uint8_t *blocks /* 128 bytes */, uint8_t sv) {
    const uint8_t *pos = &BLOCK_POS[(sv % 24) * 4];
    uint8_t tmp[128];
    memcpy(tmp, blocks, 128);
    for (int i = 0; i < 4; i++)
        memcpy(blocks + 32 * i, tmp + 32 * pos[i], 32);
}

/* ===================================================================== */

void pk4_decrypt(uint8_t buf[PK4_SIZE_STORED]) {
    uint32_t pv  = pk4_pid(buf);
    uint16_t chk = pk4_u16(buf, PK4_OFF_CHECKSUM);
    uint8_t  sv  = (uint8_t)((pv >> 13) & 31);

    crypt_array(buf + 8, 128, chk);
    shuffle45(buf + 8, sv);
}

void pk4_encrypt(uint8_t buf[PK4_SIZE_STORED]) {
    uint32_t pv  = pk4_pid(buf);
    uint16_t chk = pk4_u16(buf, PK4_OFF_CHECKSUM);
    uint8_t  sv  = (uint8_t)((pv >> 13) & 31);
    uint8_t  iv  = BLOCK_POS_INVERT[sv % 24];

    shuffle45(buf + 8, iv);
    crypt_array(buf + 8, 128, chk);
}

uint16_t pk4_compute_checksum(const uint8_t buf[PK4_SIZE_STORED]) {
    uint16_t s = 0;
    for (int i = 8; i < PK4_SIZE_STORED; i += 2)
        s += (uint16_t)(buf[i] | (buf[i+1] << 8));
    return s;
}

void pk4_recompute_checksum(uint8_t buf[PK4_SIZE_STORED]) {
    uint16_t s = pk4_compute_checksum(buf);
    buf[PK4_OFF_CHECKSUM]     = (uint8_t)s;
    buf[PK4_OFF_CHECKSUM + 1] = (uint8_t)(s >> 8);
}
