/* os_charset.h - Lot 71: the one display character set of Mohhdy.
 *
 * Every string drawn by the desktop, the console terminal, the chat and
 * the web pane is one byte per character in Windows-1252 (CP1252):
 * ASCII, Latin-1 (U+00A0..U+00FF, all French accents) and the CP1252
 * extras in 0x80..0x9F (oe ligatures, euro sign, typographic quotes and
 * dashes, ellipsis, bullet). Text that arrives as UTF-8 (web pages, model
 * output, files) is converted here; Latin Extended-A letters that CP1252
 * does not carry (U+0100..U+017F, for example Polish or Czech letters)
 * are shown as their base letter, any other character as '?'.
 *
 * The bitmap fonts (kernel/gfx_font8.h, kernel/gfx_font_hd.h, generated
 * by scripts/gen_bitmap_font.py) hold OS_GLYPH_COUNT glyphs in the order
 * of os_glyph_index(). Header only, freestanding, shared by the kernel
 * and Ring 3; no allocation, every output is bounded by its capacity. */
#ifndef OS_CHARSET_H
#define OS_CHARSET_H
#include <stdint.h>

#define OS_GLYPH_COUNT 223      /* 95 ASCII + 32 CP1252 0x80..0x9F + 96 Latin-1 */
#define OS_GLYPH_QMARK ('?' - 32)
#define OS_UNICODE_BAD 0xFFFDu

/* Unicode code point of CP1252 bytes 0x80..0x9F (0 = undefined). */
static const uint16_t os_cp1252_high[32] = {
    0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017D, 0,
    0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178
};

/* Base letters of Latin Extended-A, U+0100..U+017F. */
static const char os_latin_ext_a[129] =
    "AaAaAaCcCcCcCcDd" "DdEeEeEeEeEeGgGg" "GgGgHhHhIiIiIiIi" "IiIiJjKkkLlLlLlL"
    "lLlNnNnNnnNnOoOo" "OoOoRrRrRrSsSsSs" "SsTtTtTtUuUuUuUu" "UuUuWwYyYZzZzZzs";

/* Glyph slot of a display byte; undefined bytes and controls give '?'. */
static inline unsigned os_glyph_index(unsigned char c) {
    if (c >= 32u && c <= 126u) return (unsigned)c - 32u;
    if (c >= 0x80u && c <= 0x9Fu) return os_cp1252_high[c - 0x80u] ? 95u + (unsigned)(c - 0x80u) : (unsigned)OS_GLYPH_QMARK;
    if (c >= 0xA0u) return 127u + (unsigned)(c - 0xA0u);
    return (unsigned)OS_GLYPH_QMARK;
}

/* A display byte that the fonts can draw (printable ASCII or a defined
 * CP1252 / Latin-1 character). */
static inline int os_display_printable(unsigned char c) {
    if (c >= 32u && c <= 126u) return 1;
    if (c >= 0xA0u) return 1;
    if (c >= 0x80u && c <= 0x9Fu) return os_cp1252_high[c - 0x80u] != 0;
    return 0;
}

/* CP1252 byte for code point cp, or 0 when it has none. */
static inline unsigned char os_cp1252_exact(uint32_t cp) {
    unsigned i;
    if (cp < 0x80u) return (unsigned char)cp;
    if (cp >= 0xA0u && cp <= 0xFFu) return (unsigned char)cp;
    for (i = 0; i < 32u; i++)
        if (os_cp1252_high[i] && os_cp1252_high[i] == cp) return (unsigned char)(0x80u + i);
    return 0;
}

/* Display byte for any code point: exact CP1252, else a close letter or
 * punctuation, else '?'. Controls other than tab and newline give 0
 * (dropped). */
static inline unsigned char os_cp1252_from_cp(uint32_t cp) {
    unsigned char b;
    if (cp < 0x20u) return (cp == '\t' || cp == '\n') ? (unsigned char)cp : 0;
    if (cp == 0x7Fu || (cp >= 0x80u && cp < 0xA0u)) return 0;
    b = os_cp1252_exact(cp);
    if (b) return b;
    if (cp >= 0x100u && cp <= 0x17Fu) return (unsigned char)os_latin_ext_a[cp - 0x100u];
    switch (cp) {
    case 0x2010: case 0x2011: case 0x2012: case 0x2015: case 0x2212: return '-';
    case 0x2032: case 0x02BC: return '\'';
    case 0x2033: return '"';
    case 0x2000: case 0x2001: case 0x2002: case 0x2003: case 0x2004: case 0x2005: case 0x2006:
    case 0x2007: case 0x2008: case 0x2009: case 0x200A: case 0x202F: case 0x205F: case 0x3000: return ' ';
    case 0x200B: case 0x200C: case 0x200D: case 0x200E: case 0x200F: case 0xFEFF: case 0x00AD: return 0;
    case 0x2192: return '>';
    case 0x2190: return '<';
    case 0x2264: return '<';
    case 0x2265: return '>';
    case 0x00A0: return 0xA0;
    default: return '?';
    }
}

/* Decodes one UTF-8 sequence at s[*i] (n bytes total) and advances *i.
 * Overlong forms, surrogates, values above U+10FFFF and cut sequences
 * give OS_UNICODE_BAD and consume one byte. */
static inline uint32_t os_utf8_next(const char *s, int n, int *i) {
    const unsigned char *u = (const unsigned char *)s;
    int k = *i, len, j;
    uint32_t cp, min;
    unsigned char c;
    if (k >= n) return 0;
    c = u[k];
    if (c < 0x80u) { *i = k + 1; return c; }
    if ((c & 0xE0u) == 0xC0u) { len = 2; cp = c & 0x1Fu; min = 0x80u; }
    else if ((c & 0xF0u) == 0xE0u) { len = 3; cp = c & 0x0Fu; min = 0x800u; }
    else if ((c & 0xF8u) == 0xF0u) { len = 4; cp = c & 0x07u; min = 0x10000u; }
    else { *i = k + 1; return OS_UNICODE_BAD; }
    if (k + len > n) { *i = k + 1; return OS_UNICODE_BAD; }
    for (j = 1; j < len; j++) {
        if ((u[k + j] & 0xC0u) != 0x80u) { *i = k + 1; return OS_UNICODE_BAD; }
        cp = (cp << 6) | (u[k + j] & 0x3Fu);
    }
    if (cp < min || cp > 0x10FFFFu || (cp >= 0xD800u && cp <= 0xDFFFu)) { *i = k + 1; return OS_UNICODE_BAD; }
    *i = k + len;
    return cp;
}

/* 1 when s[i..] starts a well formed multi-byte UTF-8 sequence. */
static inline int os_utf8_seq_ok(const char *s, int n, int i) {
    int k = i;
    uint32_t cp;
    if (i >= n || (unsigned char)s[i] < 0x80u) return 0;
    cp = os_utf8_next(s, n, &k);
    return cp != OS_UNICODE_BAD || k - i > 1;
}

/* Converts text to display bytes. Well formed UTF-8 sequences are
 * decoded; any other byte >= 0x80 is taken as already CP1252 (so the
 * function is idempotent on display text). Returns the output length;
 * out is always NUL-terminated when cap > 0. */
static inline int os_text_to_display(const char *in, int n, char *out, int cap) {
    int i = 0, o = 0;
    if (cap <= 0) return 0;
    while (i < n && in[i] && o < cap - 1) {
        unsigned char c = (unsigned char)in[i];
        if (c >= 0x80u && os_utf8_seq_ok(in, n, i)) {
            uint32_t cp = os_utf8_next(in, n, &i);
            unsigned char b = os_cp1252_from_cp(cp);
            if (b) out[o++] = (char)b;
            continue;
        }
        i++;
        if (c < 0x20u && c != '\n' && c != '\t') continue;
        out[o++] = (char)c;
    }
    out[o] = 0;
    return o;
}

/* In place version (the output is never longer than the input). */
static inline int os_text_to_display_inplace(char *s, int cap) {
    int r = 0, w = 0, n = 0;
    while (n < cap && s[n]) n++;
    while (r < n) {
        unsigned char c = (unsigned char)s[r];
        if (c >= 0x80u && os_utf8_seq_ok(s, n, r)) {
            uint32_t cp = os_utf8_next(s, n, &r);
            unsigned char b = os_cp1252_from_cp(cp);
            if (b) s[w++] = (char)b;
            continue;
        }
        r++;
        if (c < 0x20u && c != '\n' && c != '\t') continue;
        s[w++] = (char)c;
    }
    if (w < cap) s[w] = 0;
    return w;
}

/* Streaming decoder for a byte sink (console terminal): feed bytes one at
 * a time, get 0 (need more), or a display byte. A sequence that turns out
 * malformed is replaced by '?'. */
typedef struct { uint32_t cp, min; int need; } os_utf8_stream_t;
static inline void os_utf8_stream_reset(os_utf8_stream_t *s) { s->cp = 0; s->min = 0; s->need = 0; }
static inline int os_utf8_stream_feed(os_utf8_stream_t *s, unsigned char c, unsigned char *outb) {
    if (s->need) {
        if ((c & 0xC0u) == 0x80u) {
            s->cp = (s->cp << 6) | (c & 0x3Fu);
            if (--s->need) return 0;
            if (s->cp < s->min || s->cp > 0x10FFFFu || (s->cp >= 0xD800u && s->cp <= 0xDFFFu)) { *outb = '?'; return 1; }
            *outb = os_cp1252_from_cp(s->cp);
            return *outb ? 1 : 0;
        }
        s->need = 0;   /* cut sequence: '?' then c starts over (reported by the caller twice) */
        *outb = '?';
        return 2;
    }
    if (c < 0x80u) { *outb = c; return 1; }
    if ((c & 0xE0u) == 0xC0u) { s->need = 1; s->cp = c & 0x1Fu; s->min = 0x80u; return 0; }
    if ((c & 0xF0u) == 0xE0u) { s->need = 2; s->cp = c & 0x0Fu; s->min = 0x800u; return 0; }
    if ((c & 0xF8u) == 0xF0u) { s->need = 3; s->cp = c & 0x07u; s->min = 0x10000u; return 0; }
    *outb = c;          /* a lone high byte: already CP1252 */
    return 1;
}

/* Appends the UTF-8 form of display byte b to out (serial logs, files).
 * Returns bytes written (0 when it does not fit). */
static inline int os_display_to_utf8(unsigned char b, char *out, int cap) {
    uint32_t cp = b;
    if (b >= 0x80u && b <= 0x9Fu) cp = os_cp1252_high[b - 0x80u] ? os_cp1252_high[b - 0x80u] : '?';
    if (cp < 0x80u) { if (cap < 1) return 0; out[0] = (char)cp; return 1; }
    if (cp < 0x800u) {
        if (cap < 2) return 0;
        out[0] = (char)(0xC0u | (cp >> 6)); out[1] = (char)(0x80u | (cp & 0x3Fu));
        return 2;
    }
    if (cap < 3) return 0;
    out[0] = (char)(0xE0u | (cp >> 12)); out[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu)); out[2] = (char)(0x80u | (cp & 0x3Fu));
    return 3;
}
/* VGA text mode and the console terminal window draw the VGA ROM font
 * (code page 437). Code points of CP437 0x80..0xAF (French letters are
 * all there except A/E/I/O/U with grave or circumflex in capitals). */
static const uint16_t os_cp437_hi[48] = {
    0xC7, 0xFC, 0xE9, 0xE2, 0xE4, 0xE0, 0xE5, 0xE7, 0xEA, 0xEB, 0xE8, 0xEF, 0xEE, 0xEC, 0xC4, 0xC5,
    0xC9, 0xE6, 0xC6, 0xF4, 0xF6, 0xF2, 0xFB, 0xF9, 0xFF, 0xD6, 0xDC, 0xA2, 0xA3, 0xA5, 0x20A7, 0x192,
    0xE1, 0xED, 0xF3, 0xFA, 0xF1, 0xD1, 0xAA, 0xBA, 0xBF, 0x2310, 0xAC, 0xBD, 0xBC, 0xA1, 0xAB, 0xBB
};
static const char os_latin1_base[65] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";

/* CP437 byte for a code point: the exact glyph when the ROM font has it,
 * else the base letter / ASCII look-alike, else '?'. */
static inline unsigned char os_cp437_from_cp(uint32_t cp) {
    unsigned i;
    unsigned char b;
    if (cp < 0x80u) return (unsigned char)cp;
    for (i = 0; i < 48u; i++) if (os_cp437_hi[i] == cp) return (unsigned char)(0x80u + i);
    switch (cp) {
    case 0xDF: return 0xE1;   /* sharp s */
    case 0xB5: return 0xE6;
    case 0xB1: return 0xF1;
    case 0xF7: return 0xF6;
    case 0xB0: return 0xF8;
    case 0xB7: return 0xFA;
    case 0xB2: return 0xFD;
    case 0xA0: return 0xFF;
    case 0x2219: return 0xF9;
    default: break;
    }
    if (cp >= 0xC0u && cp <= 0xFFu) return (unsigned char)os_latin1_base[cp - 0xC0u];
    if (cp == 0x152) return 'O';
    if (cp == 0x153) return 'o';
    b = os_cp1252_from_cp(cp);
    if (b >= 0x80u) return '?';
    return b ? b : '?';
}

/* Display (CP1252) byte of a CP437 cell, for copying console text. */
static inline unsigned char os_cp437_to_display(unsigned char c) {
    uint32_t cp;
    if (c >= 32u && c <= 126u) return c;
    if (c >= 0x80u && c <= 0xAFu) {
        cp = os_cp437_hi[c - 0x80u];
        return os_cp1252_from_cp(cp);
    }
    switch (c) {
    case 0xE1: return 0xDF;
    case 0xE6: return 0xB5;
    case 0xF1: return 0xB1;
    case 0xF6: return 0xF7;
    case 0xF8: return 0xB0;
    case 0xFA: return 0xB7;
    case 0xFD: return 0xB2;
    default: return ' ';
    }
}
#endif
