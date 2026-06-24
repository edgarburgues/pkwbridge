/* Gen4 character encoding tables.
 * See gen4_chartable.h for layout details.
 */
#include "gen4_chartable.h"

/* -------------------------------------------------------------------------
 * ASCII fast-path table.
 * Indexed by Gen4 u16 code. Entry is the printable ASCII char, or 0.
 * ------------------------------------------------------------------------- */
const char gen4_to_ascii_table[GEN4_CHAR_TABLE_SIZE] = {
    /* 0x0000..0x011E: kana, fullwidth, punctuation, icons - not ASCII */
    /* explicit ASCII slots only below */

    /* 0x0121..0x012A : '0'..'9' (halfwidth digits) */
    [0x0121] = '0', [0x0122] = '1', [0x0123] = '2', [0x0124] = '3',
    [0x0125] = '4', [0x0126] = '5', [0x0127] = '6', [0x0128] = '7',
    [0x0129] = '8', [0x012A] = '9',

    /* 0x012B..0x0144 : 'A'..'Z' (halfwidth uppercase) */
    [0x012B] = 'A', [0x012C] = 'B', [0x012D] = 'C', [0x012E] = 'D',
    [0x012F] = 'E', [0x0130] = 'F', [0x0131] = 'G', [0x0132] = 'H',
    [0x0133] = 'I', [0x0134] = 'J', [0x0135] = 'K', [0x0136] = 'L',
    [0x0137] = 'M', [0x0138] = 'N', [0x0139] = 'O', [0x013A] = 'P',
    [0x013B] = 'Q', [0x013C] = 'R', [0x013D] = 'S', [0x013E] = 'T',
    [0x013F] = 'U', [0x0140] = 'V', [0x0141] = 'W', [0x0142] = 'X',
    [0x0143] = 'Y', [0x0144] = 'Z',

    /* 0x0145..0x015E : 'a'..'z' (halfwidth lowercase) */
    [0x0145] = 'a', [0x0146] = 'b', [0x0147] = 'c', [0x0148] = 'd',
    [0x0149] = 'e', [0x014A] = 'f', [0x014B] = 'g', [0x014C] = 'h',
    [0x014D] = 'i', [0x014E] = 'j', [0x014F] = 'k', [0x0150] = 'l',
    [0x0151] = 'm', [0x0152] = 'n', [0x0153] = 'o', [0x0154] = 'p',
    [0x0155] = 'q', [0x0156] = 'r', [0x0157] = 's', [0x0158] = 't',
    [0x0159] = 'u', [0x015A] = 'v', [0x015B] = 'w', [0x015C] = 'x',
    [0x015D] = 'y', [0x015E] = 'z',

    /* Halfwidth ASCII punctuation (h_*_ family) */
    [0x01AB] = '!',   /* h_gyoe_     */
    [0x01AC] = '?',   /* h_hate_     */
    [0x01AD] = ',',   /* h_comma_    */
    [0x01AE] = '.',   /* h_period_   */
    [0x01AF] = '|',   /* h_tenten_   */
    [0x01B1] = '/',   /* h_sura_     */
    [0x01B2] = '\'',  /* us_quote1_ - single quote */
    [0x01B3] = '\'',  /* us_h_quote1_ - apostrophe */
    [0x01B4] = '"',   /* us_quote2_ */
    [0x01B5] = '"',   /* us_quote2d_ */
    [0x01B6] = '"',   /* ger_quote2_ */
    [0x01B7] = '<',   /* fre_quote_ - guillemet */
    [0x01B8] = '>',   /* fre_quoted_ */
    [0x01B9] = '(',   /* h_MaruKako__ */
    [0x01BA] = ')',   /* h_MaruKakot__ */
    [0x01BD] = '+',
    [0x01BE] = '-',
    [0x01BF] = '*',
    [0x01C0] = '#',
    [0x01C1] = '=',
    [0x01C2] = '&',
    [0x01C3] = '~',
    [0x01C4] = ':',
    [0x01C5] = ';',
    [0x01D0] = '@',
    [0x01D2] = '%',
    [0x01DE] = ' ',   /* halfwidth space */
    [0x01E9] = '_',   /* h_under_ */

    /* Fullwidth ASCII (kept as ASCII approximations for completeness) */
    [0x00A2] = '0', [0x00A3] = '1', [0x00A4] = '2', [0x00A5] = '3',
    [0x00A6] = '4', [0x00A7] = '5', [0x00A8] = '6', [0x00A9] = '7',
    [0x00AA] = '8', [0x00AB] = '9',
    [0x00AC] = 'A', [0x00AD] = 'B', [0x00AE] = 'C', [0x00AF] = 'D',
    [0x00B0] = 'E', [0x00B1] = 'F', [0x00B2] = 'G', [0x00B3] = 'H',
    [0x00B4] = 'I', [0x00B5] = 'J', [0x00B6] = 'K', [0x00B7] = 'L',
    [0x00B8] = 'M', [0x00B9] = 'N', [0x00BA] = 'O', [0x00BB] = 'P',
    [0x00BC] = 'Q', [0x00BD] = 'R', [0x00BE] = 'S', [0x00BF] = 'T',
    [0x00C0] = 'U', [0x00C1] = 'V', [0x00C2] = 'W', [0x00C3] = 'X',
    [0x00C4] = 'Y', [0x00C5] = 'Z',
    [0x00C6] = 'a', [0x00C7] = 'b', [0x00C8] = 'c', [0x00C9] = 'd',
    [0x00CA] = 'e', [0x00CB] = 'f', [0x00CC] = 'g', [0x00CD] = 'h',
    [0x00CE] = 'i', [0x00CF] = 'j', [0x00D0] = 'k', [0x00D1] = 'l',
    [0x00D2] = 'm', [0x00D3] = 'n', [0x00D4] = 'o', [0x00D5] = 'p',
    [0x00D6] = 'q', [0x00D7] = 'r', [0x00D8] = 's', [0x00D9] = 't',
    [0x00DA] = 'u', [0x00DB] = 'v', [0x00DC] = 'w', [0x00DD] = 'x',
    [0x00DE] = 'y', [0x00DF] = 'z',

    /* Fullwidth space */
    [0x0001] = ' ',
};

/* -------------------------------------------------------------------------
 * Unicode codepoint table.
 * Covers the Western accented range used by ES/FR/DE/IT.
 * ------------------------------------------------------------------------- */
const uint32_t gen4_to_unicode_table[GEN4_CHAR_TABLE_SIZE] = {
    /* Fullwidth space and Japanese kana */
    [0x0001] = 0x3000, /* IDEOGRAPHIC SPACE */

    /* Halfwidth digits / Latin (ASCII subset) */
    [0x0121] = '0', [0x0122] = '1', [0x0123] = '2', [0x0124] = '3',
    [0x0125] = '4', [0x0126] = '5', [0x0127] = '6', [0x0128] = '7',
    [0x0129] = '8', [0x012A] = '9',
    [0x012B] = 'A', [0x012C] = 'B', [0x012D] = 'C', [0x012E] = 'D',
    [0x012F] = 'E', [0x0130] = 'F', [0x0131] = 'G', [0x0132] = 'H',
    [0x0133] = 'I', [0x0134] = 'J', [0x0135] = 'K', [0x0136] = 'L',
    [0x0137] = 'M', [0x0138] = 'N', [0x0139] = 'O', [0x013A] = 'P',
    [0x013B] = 'Q', [0x013C] = 'R', [0x013D] = 'S', [0x013E] = 'T',
    [0x013F] = 'U', [0x0140] = 'V', [0x0141] = 'W', [0x0142] = 'X',
    [0x0143] = 'Y', [0x0144] = 'Z',
    [0x0145] = 'a', [0x0146] = 'b', [0x0147] = 'c', [0x0148] = 'd',
    [0x0149] = 'e', [0x014A] = 'f', [0x014B] = 'g', [0x014C] = 'h',
    [0x014D] = 'i', [0x014E] = 'j', [0x014F] = 'k', [0x0150] = 'l',
    [0x0151] = 'm', [0x0152] = 'n', [0x0153] = 'o', [0x0154] = 'p',
    [0x0155] = 'q', [0x0156] = 'r', [0x0157] = 's', [0x0158] = 't',
    [0x0159] = 'u', [0x015A] = 'v', [0x015B] = 'w', [0x015C] = 'x',
    [0x015D] = 'y', [0x015E] = 'z',

    /* Western accented letters (0x015F..0x01A2) */
    [0x015F] = 0x00C0, /* A grave   */
    [0x0160] = 0x00C1, /* A acute   */
    [0x0161] = 0x00C2, /* A circ    */
    [0x0162] = 0x00C3, /* A tilde   */
    [0x0163] = 0x00C4, /* A uml     */
    [0x0164] = 0x00C5, /* A ring    */
    [0x0165] = 0x00C6, /* AE        */
    [0x0166] = 0x00C7, /* C cedil   */
    [0x0167] = 0x00C8, /* E grave   */
    [0x0168] = 0x00C9, /* E acute   */
    [0x0169] = 0x00CA, /* E circ    */
    [0x016A] = 0x00CB, /* E uml     */
    [0x016B] = 0x00CC, /* I grave   */
    [0x016C] = 0x00CD, /* I acute   */
    [0x016D] = 0x00CE, /* I circ    */
    [0x016E] = 0x00CF, /* I uml     */
    [0x016F] = 0x00D0, /* ETH       */
    [0x0170] = 0x00D1, /* N tilde   */
    [0x0171] = 0x00D2, /* O grave   */
    [0x0172] = 0x00D3, /* O acute   */
    [0x0173] = 0x00D4, /* O circ    */
    [0x0174] = 0x00D5, /* O tilde   */
    [0x0175] = 0x00D6, /* O uml     */
    [0x0176] = 0x00D7, /* multiplication sign (h_times_) */
    [0x0177] = 0x00D8, /* O slash   */
    [0x0178] = 0x00D9, /* U grave   */
    [0x0179] = 0x00DA, /* U acute   */
    [0x017A] = 0x00DB, /* U circ    */
    [0x017B] = 0x00DC, /* U uml     */
    [0x017C] = 0x00DD, /* Y acute   */
    [0x017D] = 0x00DE, /* THORN     */
    [0x017E] = 0x00DF, /* sz (sharp s) */
    [0x017F] = 0x00E0, /* a grave   */
    [0x0180] = 0x00E1, /* a acute   */
    [0x0181] = 0x00E2, /* a circ    */
    [0x0182] = 0x00E3, /* a tilde   */
    [0x0183] = 0x00E4, /* a uml     */
    [0x0184] = 0x00E5, /* a ring    */
    [0x0185] = 0x00E6, /* ae        */
    [0x0186] = 0x00E7, /* c cedil   */
    [0x0187] = 0x00E8, /* e grave   */
    [0x0188] = 0x00E9, /* e acute   */
    [0x0189] = 0x00EA, /* e circ    */
    [0x018A] = 0x00EB, /* e uml     */
    [0x018B] = 0x00EC, /* i grave   */
    [0x018C] = 0x00ED, /* i acute   */
    [0x018D] = 0x00EE, /* i circ    */
    [0x018E] = 0x00EF, /* i uml     */
    [0x018F] = 0x00F0, /* eth       */
    [0x0190] = 0x00F1, /* n tilde   */
    [0x0191] = 0x00F2, /* o grave   */
    [0x0192] = 0x00F3, /* o acute   */
    [0x0193] = 0x00F4, /* o circ    */
    [0x0194] = 0x00F5, /* o tilde   */
    [0x0195] = 0x00F6, /* o uml     */
    [0x0196] = 0x00F7, /* division sign (h_divide_) */
    [0x0197] = 0x00F8, /* o slash   */
    [0x0198] = 0x00F9, /* u grave   */
    [0x0199] = 0x00FA, /* u acute   */
    [0x019A] = 0x00FB, /* u circ    */
    [0x019B] = 0x00FC, /* u uml     */
    [0x019C] = 0x00FD, /* y acute   */
    [0x019D] = 0x00FE, /* thorn     */
    [0x019E] = 0x00FF, /* y uml     */
    [0x019F] = 0x0152, /* OE        */
    [0x01A0] = 0x0153, /* oe        */
    [0x01A1] = 0x015E, /* S cedil   */
    [0x01A2] = 0x015F, /* s cedil   */

    /* Superscripts and Spanish/French punctuation */
    [0x01A3] = 0x00AA, /* sup_a_ (ordinal a) */
    [0x01A4] = 0x00BA, /* sup_o_ (ordinal o) */
    [0x01A5] = 0x1D49, /* sup_er_ -- best-effort: modifier 'e' */
    [0x01A6] = 0x02B3, /* sup_re_ -- best-effort: modifier 'r' */
    [0x01A7] = 0x02B3, /* sup_r_ */
    [0x01A8] = 0x20BD, /* pokedoru_ (Poke-dollar; no official glyph, placeholder) */
    [0x01A9] = 0x00A1, /* inverted ! */
    [0x01AA] = 0x00BF, /* inverted ? */

    /* Halfwidth ASCII punctuation */
    [0x01AB] = '!',
    [0x01AC] = '?',
    [0x01AD] = ',',
    [0x01AE] = '.',
    [0x01AF] = '|',
    [0x01B0] = 0x00B7, /* h_nakag_ middle dot */
    [0x01B1] = '/',
    [0x01B2] = 0x2019, /* right single quotation mark */
    [0x01B3] = '\'',
    [0x01B4] = 0x201D, /* right double quotation */
    [0x01B5] = 0x201C, /* left double quotation */
    [0x01B6] = 0x201E, /* german low-9 quotation */
    [0x01B7] = 0x00AB, /* french quote << */
    [0x01B8] = 0x00BB, /* french quote >> */
    [0x01B9] = '(',
    [0x01BA] = ')',
    [0x01BD] = '+',
    [0x01BE] = '-',
    [0x01BF] = '*',
    [0x01C0] = '#',
    [0x01C1] = '=',
    [0x01C2] = '&',
    [0x01C3] = '~',
    [0x01C4] = ':',
    [0x01C5] = ';',
    [0x01D0] = '@',
    [0x01D2] = '%',
    [0x01DE] = ' ',
    [0x01DF] = 0x1D49, /* sup_e_ */
    [0x01E9] = '_',
    [0x01EA] = '_',    /* under_ */

    /* Fullwidth alphanum (rarely used in Western saves but possible) */
    [0x00A2] = 0xFF10, [0x00A3] = 0xFF11, [0x00A4] = 0xFF12, [0x00A5] = 0xFF13,
    [0x00A6] = 0xFF14, [0x00A7] = 0xFF15, [0x00A8] = 0xFF16, [0x00A9] = 0xFF17,
    [0x00AA] = 0xFF18, [0x00AB] = 0xFF19,
    [0x00AC] = 0xFF21, [0x00AD] = 0xFF22, [0x00AE] = 0xFF23, [0x00AF] = 0xFF24,
    [0x00B0] = 0xFF25, [0x00B1] = 0xFF26, [0x00B2] = 0xFF27, [0x00B3] = 0xFF28,
    [0x00B4] = 0xFF29, [0x00B5] = 0xFF2A, [0x00B6] = 0xFF2B, [0x00B7] = 0xFF2C,
    [0x00B8] = 0xFF2D, [0x00B9] = 0xFF2E, [0x00BA] = 0xFF2F, [0x00BB] = 0xFF30,
    [0x00BC] = 0xFF31, [0x00BD] = 0xFF32, [0x00BE] = 0xFF33, [0x00BF] = 0xFF34,
    [0x00C0] = 0xFF35, [0x00C1] = 0xFF36, [0x00C2] = 0xFF37, [0x00C3] = 0xFF38,
    [0x00C4] = 0xFF39, [0x00C5] = 0xFF3A,
    [0x00C6] = 0xFF41, [0x00C7] = 0xFF42, [0x00C8] = 0xFF43, [0x00C9] = 0xFF44,
    [0x00CA] = 0xFF45, [0x00CB] = 0xFF46, [0x00CC] = 0xFF47, [0x00CD] = 0xFF48,
    [0x00CE] = 0xFF49, [0x00CF] = 0xFF4A, [0x00D0] = 0xFF4B, [0x00D1] = 0xFF4C,
    [0x00D2] = 0xFF4D, [0x00D3] = 0xFF4E, [0x00D4] = 0xFF4F, [0x00D5] = 0xFF50,
    [0x00D6] = 0xFF51, [0x00D7] = 0xFF52, [0x00D8] = 0xFF53, [0x00D9] = 0xFF54,
    [0x00DA] = 0xFF55, [0x00DB] = 0xFF56, [0x00DC] = 0xFF57, [0x00DD] = 0xFF58,
    [0x00DE] = 0xFF59, [0x00DF] = 0xFF5A,
};

/* -------------------------------------------------------------------------
 * Reverse mappings.
 * ------------------------------------------------------------------------- */
uint16_t gen4_from_ascii(char c) {
    if (c >= '0' && c <= '9') return (uint16_t)(0x0121 + (c - '0'));
    if (c >= 'A' && c <= 'Z') return (uint16_t)(0x012B + (c - 'A'));
    if (c >= 'a' && c <= 'z') return (uint16_t)(0x0145 + (c - 'a'));
    switch ((unsigned char)c) {
        case ' ':  return 0x01DE;
        case '!':  return 0x01AB;
        case '?':  return 0x01AC;
        case ',':  return 0x01AD;
        case '.':  return 0x01AE;
        case '/':  return 0x01B1;
        case '\'': return 0x01B3;
        case '"':  return 0x01B4;
        case '(':  return 0x01B9;
        case ')':  return 0x01BA;
        case '+':  return 0x01BD;
        case '-':  return 0x01BE;
        case '*':  return 0x01BF;
        case '#':  return 0x01C0;
        case '=':  return 0x01C1;
        case '&':  return 0x01C2;
        case '~':  return 0x01C3;
        case ':':  return 0x01C4;
        case ';':  return 0x01C5;
        case '@':  return 0x01D0;
        case '%':  return 0x01D2;
        case '_':  return 0x01E9;
        case '|':  return 0x01AF;
        default:   return 0;
    }
}

uint16_t gen4_from_unicode(uint32_t cp) {
    /* ASCII fast path */
    if (cp < 0x80) return gen4_from_ascii((char)cp);

    /* Latin-1 accented range -> contiguous Gen4 0x015F..0x019E
     * (gen4_code = 0x015F + (cp - 0x00C0)) */
    if (cp >= 0x00C0 && cp <= 0x00FF) {
        return (uint16_t)(0x015F + (cp - 0x00C0));
    }

    switch (cp) {
        case 0x0152: return 0x019F; /* OE */
        case 0x0153: return 0x01A0; /* oe */
        case 0x015E: return 0x01A1; /* S cedil */
        case 0x015F: return 0x01A2; /* s cedil */
        case 0x00A1: return 0x01A9; /* inverted ! */
        case 0x00BF: return 0x01AA; /* inverted ? */
        case 0x00AA: return 0x01A3; /* feminine ordinal */
        case 0x00BA: return 0x01A4; /* masculine ordinal */
        case 0x00AB: return 0x01B7; /* << */
        case 0x00BB: return 0x01B8; /* >> */
        case 0x00B7: return 0x01B0; /* middle dot */
        case 0x2019: return 0x01B2;
        case 0x201C: return 0x01B5;
        case 0x201D: return 0x01B4;
        case 0x201E: return 0x01B6;
        case 0x3000: return 0x0001; /* fullwidth space */
        default:     return 0;
    }
}
