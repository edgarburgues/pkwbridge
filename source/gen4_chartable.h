/* Gen4 character encoding for Western (Latin) scripts.
 *
 * Gen4 strings are arrays of u16 LE codepoints terminated by 0xFFFF (EOM).
 * The same code table is shared by all Western languages (EN/FR/DE/IT/ES);
 * Japanese uses the low range (0x0001..0x00A1 hira/kata + 0x00A2..0x00B0
 * fullwidth digits + 0x00B1..0x00CA fullwidth Latin). Korean is not handled
 * by this table.
 *
 * Layout of relevant ranges:
 *   0x0000        : NUL slot (unused; 0x0001 = fullwidth space)
 *   0x0001..0x00DF: Japanese kana, fullwidth digits, fullwidth A-Z/a-z
 *   0x00E1..0x011E: punctuation, suit pips, pocket icons, arrows
 *   0x011F..0x0120: cursor, ampersand
 *   0x0121..0x012A: ASCII digits '0'..'9'    (halfwidth, "h_n0_".."h_n9_")
 *   0x012B..0x0144: ASCII uppercase 'A'..'Z' (halfwidth, "h_A__".."h_Z__")
 *   0x0145..0x015E: ASCII lowercase 'a'..'z' (halfwidth, "h_a__".."h_z__")
 *   0x015F..0x01A2: Latin-1/Western accented letters (A-grave..s-cedil)
 *   0x01A3..0x01DD: superscripts, currency, more punctuation, weather icons
 *   0x01DE        : halfwidth space ' '
 *   0xFFFF        : EOM_ (string terminator)
 *
 * Maps u16 char_code -> ASCII (or 0 for unsupported / non-ASCII).
 * For rendering, use gen4_to_ascii() to get the printable char.
 * Reverse mapping (gen4_from_ascii) is provided for synthesis.
 *
 * NOTE: accented Western glyphs (e.g. 0x0170 Ntilde, 0x0190 ntilde for
 * Spanish, 0x0188 eacute for French) cannot be represented in 7-bit ASCII;
 * the table returns 0 for them. Use the wide variants if you need
 * round-tripping through UTF-8 -- see gen4_to_unicode() helper below.
 */
#ifndef GEN4_CHARTABLE_H
#define GEN4_CHARTABLE_H

#include <stdint.h>

#define GEN4_CHAR_TABLE_SIZE   0x0200
#define GEN4_CHAR_EOM          0xFFFFu

/* ASCII fast-path table. Entries are printable 7-bit chars,
 * or 0 for non-ASCII / unused / control codes. */
extern const char gen4_to_ascii_table[GEN4_CHAR_TABLE_SIZE];

/* UTF-32 (Unicode codepoint) table. Entries are full Unicode codepoints
 * for accented chars and special symbols, or 0 for unused.
 * Use this for ES/FR/DE/IT names that contain non-ASCII glyphs. */
extern const uint32_t gen4_to_unicode_table[GEN4_CHAR_TABLE_SIZE];

static inline char gen4_to_ascii(uint16_t code) {
    if (code >= GEN4_CHAR_TABLE_SIZE) return '?';
    return gen4_to_ascii_table[code];
}

static inline uint32_t gen4_to_unicode(uint16_t code) {
    if (code == GEN4_CHAR_EOM) return 0;
    if (code >= GEN4_CHAR_TABLE_SIZE) return 0xFFFDu; /* U+FFFD replacement */
    return gen4_to_unicode_table[code];
}

/* Reverse mapping: ASCII char -> Gen4 char_code (halfwidth Western glyph).
 * Returns 0 if c is not representable. Space maps to 0x01DE. */
uint16_t gen4_from_ascii(char c);

/* Reverse mapping for Unicode codepoint (covers accented letters).
 * Returns 0 if cp is not representable in the Gen4 charset. */
uint16_t gen4_from_unicode(uint32_t cp);

#endif /* GEN4_CHARTABLE_H */
