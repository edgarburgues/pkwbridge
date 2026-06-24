#include "nds_extract.h"
#include "applog.h"
#include "gen4_chartable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define DATA_DIR  "sdmc:/3ds/pokeStride/data"

/* ============================================================== *
 * FORMAT CONSTANTS (same for every game, not in game_profile_t). *
 * ============================================================== */

/* HGSS Pokewalker: 8 background bitmaps inside the courses overlay,
 * each 192 bytes (32 x 24 @ 2bpp packed). Offsets within the
 * BLZ-decompressed overlay (HGSS-specific — the only game with the
 * Pokewalker). */
#define COURSE_SIZE       192
#define COURSE_COUNT        8
static const uint32_t COURSE_BITMAP_OFFSETS[COURSE_COUNT] = {
    0x11644, 0x11044, 0x11404, 0x11584,
    0x11344, 0x11104, 0x11284, 0x111C4
};

/* Pokewalker phcgra (Pokemon big portraits) + phcicon (walking icons)
 * per-entry sizes. */
#define PHCGRA_ENTRY     1536
#define PHCICON_ENTRY     384

/* PHC system font inside its NARC inner file: header + 509 glyphs of
 * 64 bytes each + 509 advance-width bytes. */
#define FONT_GLYPH_OFFSET  0x10
#define FONT_GLYPH_SIZE    (509 * 64)
#define FONT_WIDTHS_OFFSET 0x7F50
#define FONT_WIDTHS_SIZE   509

/* Pokeicon NARC structure: NCLR at inner [0], NCGRs at inner
 * [first_ncgr .. first_ncgr+count-1]. The header offsets are stable
 * across Gen 4 and Gen 5. */
#define POKEICON_NCLR_INNER          0
#define POKEICON_FIRST_NCGR          7
#define POKEICON_NCGR_PIXEL_OFFSET 0x30
#define POKEICON_GLYPH_SIZE        512
#define POKEICON_NCLR_PAL_OFFSET   0x28
#define POKEICON_PAL_BYTES         (3 * 16 * 2)

/* Format of the names_*.bin files we emit: magic + count + offsets
 * table + concatenated UTF-8 NUL-terminated strings. */
#define MSG_BIN_MAGIC          0x47534D4Eu     /* 'NMSG' LE */

/* ============================================================== *
 * Helpers — pure format, no game knowledge.                       *
 * ============================================================== */

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* BLZ ("Bottom LZ") decompressor used for arm9 + overlays. Stream is
 * decoded right-to-left; match descriptor byte order is opposite to
 * standard LZ77 0x10 (higher-address byte carries (length-3, disp_high),
 * lower-address byte carries disp_low). */
static uint8_t *blz_decompress(const uint8_t *in, size_t in_len,
                               size_t *out_len) {
    if (in_len < 8) return NULL;
    const uint8_t *f = in + in_len - 8;
    uint32_t enc_size = f[0] | (f[1] << 8) | (f[2] << 16);
    uint8_t  hdr_size = f[3];
    uint32_t add_size = rd32(f + 4);
    if (hdr_size > in_len || enc_size > in_len) return NULL;

    size_t dec_size = in_len + add_size;
    uint8_t *out = (uint8_t *)malloc(dec_size);
    if (!out) return NULL;
    memcpy(out, in, in_len);
    memset(out + in_len, 0, add_size);

    size_t in_pos  = in_len - hdr_size;
    size_t out_pos = dec_size;
    size_t in_end  = in_len - enc_size;
    uint8_t flags = 0;
    int bits = 0;

    while (in_pos > in_end) {
        if (bits == 0) {
            if (in_pos == 0) goto fail;
            in_pos--;
            flags = out[in_pos];
            bits = 8;
        }
        int bit = (flags & 0x80) != 0;
        flags = (uint8_t)(flags << 1);
        bits--;

        if (!bit) {
            if (in_pos == 0 || out_pos == 0) goto fail;
            in_pos--;
            out_pos--;
            out[out_pos] = out[in_pos];
        } else {
            if (in_pos < 2) goto fail;
            in_pos -= 2;
            uint8_t b_lo = out[in_pos];
            uint8_t b_hi = out[in_pos + 1];
            int length = (b_hi >> 4) + 3;
            int disp   = (((b_hi & 0xF) << 8) | b_lo) + 3;
            for (int k = 0; k < length; k++) {
                if (out_pos == 0 || out_pos + (size_t)disp > dec_size) goto fail;
                out_pos--;
                out[out_pos] = out[out_pos + disp];
            }
        }
    }
    *out_len = dec_size;
    return out;
fail:
    free(out);
    return NULL;
}

static uint8_t *lz77_decompress(const uint8_t *in, size_t in_len,
                                size_t *out_len) {
    if (in_len < 4 || in[0] != 0x10) return NULL;
    uint32_t size = in[1] | (in[2] << 8) | (in[3] << 16);
    uint8_t *out = (uint8_t *)malloc(size ? size : 1);
    if (!out) return NULL;
    size_t pos = 0;
    size_t i = 4;
    while (pos < size && i < in_len) {
        uint8_t flags = in[i++];
        for (int b = 0; b < 8 && pos < size; b++) {
            if (flags & (0x80 >> b)) {
                if (i + 1 >= in_len) goto fail;
                uint8_t hi = in[i];
                uint8_t lo = in[i + 1];
                i += 2;
                int length = (hi >> 4) + 3;
                int disp   = ((hi & 0xF) << 8 | lo) + 1;
                if ((size_t)disp > pos) goto fail;
                for (int k = 0; k < length && pos < size; k++) {
                    out[pos] = out[pos - disp];
                    pos++;
                }
            } else {
                if (i >= in_len) goto fail;
                out[pos++] = in[i++];
            }
        }
    }
    if (pos != size) goto fail;
    *out_len = size;
    return out;
fail:
    free(out);
    return NULL;
}

static bool nds_read_u32(FILE *fp, uint32_t off, uint32_t *out) {
    uint8_t buf[4];
    if (fseek(fp, (long)off, SEEK_SET) != 0) return false;
    if (fread(buf, 1, 4, fp) != 4) return false;
    *out = rd32(buf);
    return true;
}

static uint8_t *nds_read_file(FILE *fp, uint32_t fat_off, uint32_t idx,
                              size_t *out_len) {
    uint32_t start, end;
    if (!nds_read_u32(fp, fat_off + idx * 8, &start) ||
        !nds_read_u32(fp, fat_off + idx * 8 + 4, &end)) return NULL;
    if (end <= start) return NULL;
    size_t len = end - start;
    uint8_t *buf = (uint8_t *)malloc(len);
    if (!buf) return NULL;
    if (fseek(fp, (long)start, SEEK_SET) != 0 ||
        fread(buf, 1, len, fp) != len) {
        free(buf);
        return NULL;
    }
    *out_len = len;
    return buf;
}

typedef struct {
    const uint8_t *blob;
    size_t blob_len;
    uint32_t count;
    uint32_t entries_off;
    uint32_t gmif_data;
} narc_t;

static bool narc_open(const uint8_t *blob, size_t blob_len, narc_t *n) {
    if (blob_len < 32) return false;
    if (memcmp(blob, "NARC", 4) != 0) return false;
    if (memcmp(blob + 16, "BTAF", 4) != 0) return false;
    uint32_t count = rd32(blob + 24);
    /* Bounded by available space; the 100000 cap is a sanity belt
     * on top so we don't allocate huge tables for malformed dumps. */
    if (count == 0 || count > 100000) return false;
    if (count > (uint32_t)((blob_len - 28) / 8)) return false;
    uint32_t entries_off = 16 + 12;
    uint32_t btnf_off = entries_off + count * 8;
    if ((size_t)btnf_off + 8 > blob_len) return false;
    uint32_t btnf_size = rd32(blob + btnf_off + 4);
    /* Guard against u32 overflow in btnf_off + btnf_size + 8 — a
     * crafted NARC could set btnf_size = 0xFFFFFFFF and wrap to a
     * small value that passes the subsequent bounds check. */
    if (btnf_size > blob_len) return false;
    if (btnf_off > blob_len - btnf_size - 8) return false;
    uint32_t gmif_data = btnf_off + btnf_size + 8;
    if ((size_t)gmif_data > blob_len) return false;
    n->blob = blob;
    n->blob_len = blob_len;
    n->count = count;
    n->entries_off = entries_off;
    n->gmif_data = gmif_data;
    return true;
}

static const uint8_t *narc_inner(const narc_t *n, uint32_t idx,
                                 size_t *out_len) {
    if (idx >= n->count) return NULL;
    uint32_t en = n->entries_off + idx * 8;
    uint32_t fs = rd32(n->blob + en);
    uint32_t fe = rd32(n->blob + en + 4);
    if (fe <= fs) return NULL;
    size_t off = (size_t)n->gmif_data + fs;
    size_t len = fe - fs;
    if (off + len > n->blob_len) return NULL;
    *out_len = len;
    return n->blob + off;
}

static bool write_bin(const char *path, const uint8_t *data, size_t len) {
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        pkw_log("nds_extract: cannot open for write: %s\n", path);
        return false;
    }
    size_t w = fwrite(data, 1, len, fp);
    fclose(fp);
    if (w != len) {
        pkw_log("nds_extract: short write %s (%zu/%zu)\n", path, w, len);
        return false;
    }
    pkw_log("nds_extract: wrote %s (%zu B)\n", path, len);
    return true;
}

/* ============================================================== *
 * Per-asset extractors.                                           *
 * ============================================================== */

static bool extract_courses(FILE *fp, const game_profile_t *prof,
                            uint32_t fat_off, uint32_t ov_tbl_off) {
    uint8_t ent[32];
    if (fseek(fp, (long)(ov_tbl_off + prof->overlay_courses * 32),
              SEEK_SET) != 0 ||
        fread(ent, 1, 32, fp) != 32) {
        return false;
    }
    uint32_t file_id = rd32(ent + 24);
    bool compressed = (rd32(ent + 28) >> 24) & 1;

    size_t ov_len;
    uint8_t *ov_raw = nds_read_file(fp, fat_off, file_id, &ov_len);
    if (!ov_raw) return false;

    const uint8_t *ov_dec = ov_raw;
    size_t dec_len = ov_len;
    uint8_t *blz_out = NULL;
    if (compressed) {
        blz_out = blz_decompress(ov_raw, ov_len, &dec_len);
        if (!blz_out) {
            pkw_log("nds_extract: BLZ failed on courses overlay\n");
            free(ov_raw);
            return false;
        }
        ov_dec = blz_out;
    }

    uint8_t cb[COURSE_COUNT * COURSE_SIZE];
    bool ok = true;
    for (int i = 0; i < COURSE_COUNT && ok; i++) {
        uint32_t off = COURSE_BITMAP_OFFSETS[i];
        if ((size_t)off + COURSE_SIZE > dec_len) { ok = false; break; }
        memcpy(cb + i * COURSE_SIZE, ov_dec + off, COURSE_SIZE);
    }
    if (blz_out) free(blz_out);
    free(ov_raw);
    if (!ok) return false;
    return write_bin(DATA_DIR "/course_bitmaps.bin", cb, sizeof(cb));
}

static bool extract_narc(FILE *fp, uint32_t fat_off, uint32_t file_id,
                         uint32_t expected_count, size_t entry_size,
                         const char *out_path) {
    size_t narc_len;
    uint8_t *narc_blob = nds_read_file(fp, fat_off, file_id, &narc_len);
    if (!narc_blob) return false;

    narc_t n;
    if (!narc_open(narc_blob, narc_len, &n) || n.count != expected_count) {
        pkw_log("nds_extract: bad NARC at file %u\n", (unsigned)file_id);
        free(narc_blob);
        return false;
    }

    size_t total = (size_t)expected_count * entry_size;
    uint8_t *out = (uint8_t *)malloc(total);
    if (!out) { free(narc_blob); return false; }
    memset(out, 0, total);

    for (uint32_t i = 0; i < expected_count; i++) {
        size_t inner_len;
        const uint8_t *inner = narc_inner(&n, i, &inner_len);
        if (!inner) {
            pkw_log("nds_extract: NARC %u missing inner %u\n",
                    (unsigned)file_id, (unsigned)i);
            free(out); free(narc_blob);
            return false;
        }
        const uint8_t *src;
        size_t src_len;
        uint8_t *dec = NULL;
        if (inner_len > 0 && inner[0] == 0x10) {
            dec = lz77_decompress(inner, inner_len, &src_len);
            if (!dec) {
                pkw_log("nds_extract: LZ77 fail NARC %u inner %u\n",
                        (unsigned)file_id, (unsigned)i);
                free(out); free(narc_blob);
                return false;
            }
            src = dec;
        } else {
            src = inner;
            src_len = inner_len;
        }
        size_t copy = src_len < entry_size ? src_len : entry_size;
        memcpy(out + (size_t)i * entry_size, src, copy);
        if (dec) free(dec);
    }
    free(narc_blob);
    bool ok = write_bin(out_path, out, total);
    free(out);
    return ok;
}

static bool extract_font(FILE *fp, const game_profile_t *prof,
                         uint32_t fat_off) {
    size_t narc_len;
    uint8_t *narc_blob = nds_read_file(fp, fat_off, prof->font_file_id,
                                       &narc_len);
    if (!narc_blob) return false;

    narc_t n;
    if (!narc_open(narc_blob, narc_len, &n) ||
        prof->font_inner >= n.count) {
        pkw_log("nds_extract: bad font NARC (count=%u, want inner %u)\n",
                (unsigned)n.count, (unsigned)prof->font_inner);
        free(narc_blob);
        return false;
    }

    size_t inner_len;
    const uint8_t *inner = narc_inner(&n, prof->font_inner, &inner_len);
    if (!inner ||
        inner_len < FONT_WIDTHS_OFFSET + FONT_WIDTHS_SIZE) {
        pkw_log("nds_extract: font inner short (%zu B)\n",
                inner ? inner_len : 0);
        free(narc_blob);
        return false;
    }

    bool ok = write_bin(DATA_DIR "/font_sys_glyphs.bin",
                        inner + FONT_GLYPH_OFFSET, FONT_GLYPH_SIZE);
    ok = ok && write_bin(DATA_DIR "/font_sys_widths.bin",
                         inner + FONT_WIDTHS_OFFSET, FONT_WIDTHS_SIZE);
    free(narc_blob);
    return ok;
}

/* UTF-8 encode one codepoint. Returns bytes written (1..4) or 0. */
static int utf8_encode(uint32_t cp, uint8_t *out) {
    if (cp < 0x80) { out[0] = (uint8_t)cp; return 1; }
    if (cp < 0x800) {
        out[0] = (uint8_t)(0xC0 | (cp >> 6));
        out[1] = (uint8_t)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        out[0] = (uint8_t)(0xE0 | (cp >> 12));
        out[1] = (uint8_t)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (uint8_t)(0x80 | (cp & 0x3F));
        return 3;
    }
    if (cp < 0x110000) {
        out[0] = (uint8_t)(0xF0 | (cp >> 18));
        out[1] = (uint8_t)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (uint8_t)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (uint8_t)(0x80 | (cp & 0x3F));
        return 4;
    }
    return 0;
}

static bool extract_msg_strings(FILE *fp, uint32_t fat_off,
                                uint32_t narc_id, uint32_t inner_id,
                                const char *out_path)
{
    size_t narc_len;
    uint8_t *narc_blob = nds_read_file(fp, fat_off, narc_id, &narc_len);
    if (!narc_blob) return false;
    narc_t n;
    if (!narc_open(narc_blob, narc_len, &n) || inner_id >= n.count) {
        pkw_log("msg: bad NARC %u (count=%u, want inner %u)\n",
                (unsigned)narc_id, (unsigned)n.count, (unsigned)inner_id);
        free(narc_blob);
        return false;
    }
    size_t msg_len;
    const uint8_t *msg = narc_inner(&n, inner_id, &msg_len);
    if (!msg || msg_len < 4) {
        free(narc_blob);
        return false;
    }
    uint32_t count    = (uint32_t)msg[0] | ((uint32_t)msg[1] << 8);
    uint32_t file_key = (uint32_t)msg[2] | ((uint32_t)msg[3] << 8);

    uint32_t *offsets = (uint32_t *)malloc(count * sizeof(uint32_t));
    uint32_t *sizes   = (uint32_t *)malloc(count * sizeof(uint32_t));
    if (!offsets || !sizes) {
        free(offsets); free(sizes); free(narc_blob);
        return false;
    }
    for (uint32_t i = 0; i < count; i++) {
        uint32_t k = (file_key * 0x2FDu * (i + 1)) & 0xFFFFu;
        uint32_t mask = (k << 16) | k;
        offsets[i] = rd32(msg + 4 + i * 8) ^ mask;
        sizes[i]   = rd32(msg + 8 + i * 8) ^ mask;
    }

    size_t bin_table_size = 4 + 4 + (size_t)count * 4;
    size_t string_cap  = 4096;
    uint8_t *strings_buf = (uint8_t *)malloc(string_cap);
    uint32_t *out_offsets = (uint32_t *)calloc(count, sizeof(uint32_t));
    if (!strings_buf || !out_offsets) {
        free(offsets); free(sizes); free(strings_buf); free(out_offsets);
        free(narc_blob);
        return false;
    }
    size_t string_len = 0;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t off = offsets[i];
        uint32_t sz  = sizes[i];
        if (sz == 0 || sz > 300 || (size_t)off + (size_t)sz * 2 > msg_len) {
            out_offsets[i] = 0;
            continue;
        }
        out_offsets[i] = (uint32_t)(bin_table_size + string_len);
        uint32_t ck = (0x91BD3u * (i + 1)) & 0xFFFFu;
        for (uint32_t j = 0; j < sz; j++) {
            uint16_t c_enc = (uint16_t)msg[off + j * 2]
                           | ((uint16_t)msg[off + j * 2 + 1] << 8);
            uint16_t c = (uint16_t)(c_enc ^ ck);
            ck = (uint16_t)((ck + 0x493Du) & 0xFFFFu);
            if (c == 0xFFFF) break;
            uint32_t cp = (c < GEN4_CHAR_TABLE_SIZE)
                ? gen4_to_unicode_table[c] : 0;
            if (cp == 0) cp = '?';
            if (string_len + 8 > string_cap) {
                string_cap *= 2;
                uint8_t *grown = (uint8_t *)realloc(strings_buf, string_cap);
                if (!grown) {
                    free(strings_buf); free(out_offsets);
                    free(offsets); free(sizes); free(narc_blob);
                    return false;
                }
                strings_buf = grown;
            }
            int nb = utf8_encode(cp, strings_buf + string_len);
            string_len += (size_t)nb;
        }
        if (string_len + 1 > string_cap) {
            string_cap *= 2;
            uint8_t *grown = (uint8_t *)realloc(strings_buf, string_cap);
            if (!grown) {
                free(strings_buf); free(out_offsets);
                free(offsets); free(sizes); free(narc_blob);
                return false;
            }
            strings_buf = grown;
        }
        strings_buf[string_len++] = 0;
    }
    free(offsets);
    free(sizes);
    free(narc_blob);

    FILE *out = fopen(out_path, "wb");
    if (!out) {
        pkw_log("msg: cannot open %s for write\n", out_path);
        free(strings_buf); free(out_offsets);
        return false;
    }
    uint8_t hdr[8];
    uint32_t magic = MSG_BIN_MAGIC;
    hdr[0] = (uint8_t)(magic);       hdr[1] = (uint8_t)(magic >> 8);
    hdr[2] = (uint8_t)(magic >> 16); hdr[3] = (uint8_t)(magic >> 24);
    hdr[4] = (uint8_t)(count);       hdr[5] = (uint8_t)(count >> 8);
    hdr[6] = (uint8_t)(count >> 16); hdr[7] = (uint8_t)(count >> 24);
    bool ok = fwrite(hdr, 1, 8, out) == 8;
    for (uint32_t i = 0; i < count && ok; i++) {
        uint8_t buf[4];
        uint32_t v = out_offsets[i];
        buf[0] = (uint8_t)v;
        buf[1] = (uint8_t)(v >> 8);
        buf[2] = (uint8_t)(v >> 16);
        buf[3] = (uint8_t)(v >> 24);
        if (fwrite(buf, 1, 4, out) != 4) ok = false;
    }
    if (ok && fwrite(strings_buf, 1, string_len, out) != string_len) {
        ok = false;
    }
    fclose(out);
    if (ok) {
        pkw_log("msg: wrote %s (%u entries, %zu B strings)\n",
                out_path, (unsigned)count, string_len);
    }
    free(strings_buf);
    free(out_offsets);
    return ok;
}

static bool extract_pokeicon(FILE *fp, const game_profile_t *prof,
                             uint32_t fat_off) {
    size_t narc_len;
    uint8_t *narc_blob = nds_read_file(fp, fat_off, prof->pokeicon_file_id,
                                       &narc_len);
    if (!narc_blob) return false;

    narc_t n;
    if (!narc_open(narc_blob, narc_len, &n) ||
        n.count < (uint32_t)(POKEICON_FIRST_NCGR + prof->pokeicon_count)) {
        pkw_log("nds_extract: bad pokeicon NARC (count=%u, want %u)\n",
                (unsigned)n.count,
                (unsigned)(POKEICON_FIRST_NCGR + prof->pokeicon_count));
        free(narc_blob);
        return false;
    }

    size_t nclr_len;
    const uint8_t *nclr = narc_inner(&n, POKEICON_NCLR_INNER, &nclr_len);
    if (!nclr || nclr_len < POKEICON_NCLR_PAL_OFFSET + POKEICON_PAL_BYTES) {
        free(narc_blob);
        return false;
    }
    bool ok = write_bin(DATA_DIR "/pokeicon_palettes.bin",
                        nclr + POKEICON_NCLR_PAL_OFFSET,
                        POKEICON_PAL_BYTES);

    if (ok) {
        size_t total = (size_t)prof->pokeicon_count * POKEICON_GLYPH_SIZE;
        uint8_t *glyphs = (uint8_t *)malloc(total);
        if (!glyphs) {
            free(narc_blob);
            return false;
        }
        memset(glyphs, 0, total);
        for (uint32_t i = 0; i < prof->pokeicon_count && ok; i++) {
            size_t inner_len;
            const uint8_t *inner = narc_inner(&n, POKEICON_FIRST_NCGR + i,
                                              &inner_len);
            if (!inner ||
                inner_len < POKEICON_NCGR_PIXEL_OFFSET + POKEICON_GLYPH_SIZE) {
                pkw_log("nds_extract: pokeicon inner %u too short\n",
                        (unsigned)(POKEICON_FIRST_NCGR + i));
                ok = false;
                break;
            }
            memcpy(glyphs + (size_t)i * POKEICON_GLYPH_SIZE,
                   inner + POKEICON_NCGR_PIXEL_OFFSET,
                   POKEICON_GLYPH_SIZE);
        }
        if (ok) {
            ok = write_bin(DATA_DIR "/pokeicon_glyphs.bin", glyphs, total);
        }
        free(glyphs);
    }
    free(narc_blob);
    if (!ok) return false;

    /* Palette index table inside BLZ-compressed arm9. */
    uint32_t arm9_off, arm9_size;
    if (!nds_read_u32(fp, 0x20, &arm9_off) ||
        !nds_read_u32(fp, 0x2C, &arm9_size) ||
        arm9_size == 0) {
        pkw_log("nds_extract: cannot read arm9 header for pokeicon table\n");
        return false;
    }
    uint8_t *arm9_raw = (uint8_t *)malloc(arm9_size);
    if (!arm9_raw) return false;
    if (fseek(fp, (long)arm9_off, SEEK_SET) != 0 ||
        fread(arm9_raw, 1, arm9_size, fp) != arm9_size) {
        free(arm9_raw);
        return false;
    }
    size_t arm9_dec_len = 0;
    uint8_t *arm9_dec = blz_decompress(arm9_raw, arm9_size, &arm9_dec_len);
    free(arm9_raw);
    if (!arm9_dec) {
        pkw_log("nds_extract: BLZ failed on arm9\n");
        return false;
    }
    if (arm9_dec_len <
        (size_t)prof->pokeicon_arm9_pal_index_offset + prof->pokeicon_count) {
        pkw_log("nds_extract: arm9 too short for pokeicon table\n");
        free(arm9_dec);
        return false;
    }
    ok = write_bin(DATA_DIR "/pokeicon_pal_index.bin",
                   arm9_dec + prof->pokeicon_arm9_pal_index_offset,
                   prof->pokeicon_count);
    free(arm9_dec);
    return ok;
}

/* ============================================================== *
 * Public API.                                                     *
 * ============================================================== */

/* --- personal data (gender ratio + growth rate) --------------------- *
 * gen4 personal entry: 0x2C bytes; gender ratio at +0x10, growth rate
 * (EXP curve) at +0x13. Both are region-independent. We pull just those
 * two bytes per species into a compact personal.bin:
 *   [4B magic "PRS1"][N gender bytes][N growth bytes]   (N = count)
 *
 * The NARC is located by `prof->personal_file_id` (a validated hint),
 * with a FAT scan fallback that recognises the table by its entry count
 * + a known gender ratio (Bulbasaur species 1 == 0x1F), so a wrong/zero
 * hint or a different region still resolves. */
#define PERSONAL_OFF_GENDER  0x10
#define PERSONAL_OFF_GROWTH  0x13
#define PERSONAL_MAGIC0 'P'
#define PERSONAL_MAGIC1 'R'
#define PERSONAL_MAGIC2 'S'
#define PERSONAL_MAGIC3 '1'

/* Returns true if `blob` is a NARC whose inner files look like gen4
 * personal entries with `count` species and Bulbasaur(1) gender 0x1F. */
static bool personal_narc_ok(const uint8_t *blob, size_t len, uint32_t count) {
    narc_t n;
    if (!narc_open(blob, len, &n)) return false;
    if (n.count != count) return false;
    size_t il;
    const uint8_t *bulba = narc_inner(&n, 1, &il);
    if (!bulba || il <= PERSONAL_OFF_GROWTH) return false;
    return bulba[PERSONAL_OFF_GENDER] == 0x1F;
}

static bool extract_personal(FILE *fp, const game_profile_t *prof,
                             uint32_t fat_off) {
    uint32_t count = prof->personal_count;
    if (count == 0) return false;

    /* 1) Try the profile's hint file_id. */
    uint8_t *blob = NULL;
    size_t blob_len = 0;
    uint32_t file_id = prof->personal_file_id;
    if (file_id != 0) {
        size_t l; uint8_t *b = nds_read_file(fp, fat_off, file_id, &l);
        if (b && personal_narc_ok(b, l, count)) { blob = b; blob_len = l; }
        else { if (b) free(b); file_id = 0; }
    }

    /* 2) Fallback: scan the FAT for a matching personal NARC. */
    if (!blob) {
        uint32_t fat_size = 0;
        if (!nds_read_u32(fp, 0x4C, &fat_size)) return false;
        uint32_t nfiles = fat_size / 8;
        for (uint32_t idx = 0; idx < nfiles; idx++) {
            size_t l; uint8_t *b = nds_read_file(fp, fat_off, idx, &l);
            if (!b) continue;
            if (l >= 32 && memcmp(b, "NARC", 4) == 0 &&
                personal_narc_ok(b, l, count)) {
                blob = b; blob_len = l; file_id = idx;
                break;
            }
            free(b);
        }
    }
    if (!blob) {
        pkw_log("nds_extract: personal NARC not found\n");
        return false;
    }
    pkw_log("nds_extract: personal NARC at file %u (count=%u)\n",
            (unsigned)file_id, (unsigned)count);

    narc_t n;
    narc_open(blob, blob_len, &n);  /* known-good from personal_narc_ok */
    size_t total = 4 + (size_t)count * 2;
    uint8_t *out = (uint8_t *)malloc(total);
    if (!out) { free(blob); return false; }
    out[0] = PERSONAL_MAGIC0; out[1] = PERSONAL_MAGIC1;
    out[2] = PERSONAL_MAGIC2; out[3] = PERSONAL_MAGIC3;
    for (uint32_t i = 0; i < count; i++) {
        size_t il;
        const uint8_t *e = narc_inner(&n, i, &il);
        uint8_t gender = 0x7F, growth = 0x00;  /* neutral defaults */
        if (e && il > PERSONAL_OFF_GROWTH) {
            gender = e[PERSONAL_OFF_GENDER];
            growth = e[PERSONAL_OFF_GROWTH];
        }
        out[4 + i]         = gender;
        out[4 + count + i] = growth;
    }
    free(blob);
    bool ok = write_bin(DATA_DIR "/personal.bin", out, total);
    free(out);
    return ok;
}

bool nds_extract_from_path(const char *nds_path,
                           const game_profile_t *prof) {
    pkw_log("nds_extract: opening %s\n", nds_path);
    FILE *fp = fopen(nds_path, "rb");
    if (!fp) {
        pkw_log("nds_extract: fopen failed\n");
        return false;
    }

    /* If caller didn't supply a profile, sniff one from the NDS header. */
    if (!prof) {
        char gc[4];
        if (fseek(fp, 0x0C, SEEK_SET) != 0 || fread(gc, 1, 4, fp) != 4) {
            pkw_log("nds_extract: cannot read game code\n");
            fclose(fp);
            return false;
        }
        prof = game_profile_for_code(gc);
        if (!prof) {
            pkw_log("nds_extract: no profile recognises game code "
                    "'%c%c%c%c'\n", gc[0], gc[1], gc[2], gc[3]);
            fclose(fp);
            return false;
        }
    }
    pkw_log("nds_extract: profile = %s\n", prof->display_name);

    uint32_t fat_off, ov_tbl_off;
    if (!nds_read_u32(fp, 0x48, &fat_off) ||
        !nds_read_u32(fp, 0x50, &ov_tbl_off)) {
        pkw_log("nds_extract: NDS header read failed\n");
        fclose(fp);
        return false;
    }
    pkw_log("nds_extract: fat_off=0x%X ov_tbl=0x%X\n",
            (unsigned)fat_off, (unsigned)ov_tbl_off);

    bool ok = true;

    /* Pokewalker-only assets — gated on profile flag. */
    if (prof->has_pokewalker) {
        ok = ok && extract_courses(fp, prof, fat_off, ov_tbl_off);
        ok = ok && extract_narc(fp, fat_off, prof->phcgra_file_id,
                                prof->phcgra_count, PHCGRA_ENTRY,
                                DATA_DIR "/phcgra_data.bin");
        ok = ok && extract_narc(fp, fat_off, prof->phcicon_file_id,
                                prof->phcicon_count, PHCICON_ENTRY,
                                DATA_DIR "/phcicon_data.bin");
        ok = ok && extract_font(fp, prof, fat_off);
    } else {
        pkw_log("nds_extract: profile has no Pokewalker — "
                "skipping walker assets\n");
    }

    /* Cross-generation assets. */
    ok = ok && extract_pokeicon(fp, prof, fat_off);
    ok = ok && extract_personal(fp, prof, fat_off);
    ok = ok && extract_msg_strings(fp, fat_off,
                                   prof->msg_narc_file_id,
                                   prof->msg_inner_items,
                                   DATA_DIR "/names_items.bin");
    ok = ok && extract_msg_strings(fp, fat_off,
                                   prof->msg_narc_file_id,
                                   prof->msg_inner_moves,
                                   DATA_DIR "/names_moves.bin");
    ok = ok && extract_msg_strings(fp, fat_off,
                                   prof->msg_narc_file_id,
                                   prof->msg_inner_locations,
                                   DATA_DIR "/names_locations.bin");

    fclose(fp);
    return ok;
}

bool nds_extract_try(void) {
    static const char *const candidates[] = {
        DATA_DIR "/SoulSilver.nds",
        DATA_DIR "/HeartGold.nds",
    };
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        FILE *fp = fopen(candidates[i], "rb");
        if (fp) {
            fclose(fp);
            return nds_extract_from_path(candidates[i], NULL);
        }
    }
    pkw_log("nds_extract: no NDS dump found in %s\n", DATA_DIR);
    return false;
}
