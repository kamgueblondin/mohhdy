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
 * and Ring 3; no allocation, every output is bounded by its capacity.
 *
 * Lot 72: glyph pages. The five bytes CP1252 leaves undefined (0x81,
 * 0x8D, 0x8F, 0x90, 0x9D, os_page_sel[] in os_charset_pages.h) are page
 * selectors: they draw nothing, take no cell, and make the bytes
 * 0x80..0xFF after them mean the characters of a page: 0 CP1252 (the
 * default at the start of every string), 1 Latin Extended-A, 2 Greek
 * (ISO-8859-7 layout), 3 Cyrillic (ISO-8859-5 layout plus Ukrainian and
 * Kazakh letters), 4 symbols (arrows, mathematics, box drawing, blocks).
 * ASCII means the same in every page. A web TEXT item carries its page in
 * bits 8..10 of os_fb_web_item_t.pad instead (the web engine never puts a
 * selector in item text). The fonts hold OS_GLYPH_TOTAL glyphs. */
#ifndef OS_CHARSET_H
#define OS_CHARSET_H
#include <stdint.h>
#include "os_charset_pages.h"

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

/* Lot 72: page of a selector byte, or -1 for any other byte. */
static inline int os_page_of_sel(unsigned char c) {
    int k;
    if (c < 0x81u || c > 0x9Du) return -1;
    for (k = 0; k < OS_PAGE_COUNT; k++)
        if (os_page_sel[k] == c) return k;
    return -1;
}

/* Glyph slot of display byte c in page p (page 0 = os_glyph_index). */
static inline unsigned os_glyph_index_page(unsigned page, unsigned char c) {
    if (page == 0u || page >= (unsigned)OS_PAGE_COUNT || c < 0x80u) return os_glyph_index(c);
    return os_page_glyph[page - 1u][c - 0x80u];
}

/* Code point of display byte c in page p; 0 for a selector or a byte the
 * page leaves undefined. */
static inline uint32_t os_display_cp(unsigned page, unsigned char c) {
    if (os_page_of_sel(c) >= 0) return 0;
    if (c < 0x80u) return c;
    if (page == 0u || page >= (unsigned)OS_PAGE_COUNT) return c >= 0xA0u ? c : os_cp1252_high[c - 0x80u];
    return os_page_cp[page - 1u][c - 0x80u];
}

/* Page and byte of a code point: 0 when CP1252 has it, 1..4 from the
 * page tables, -1 when no page has it. */
static inline int os_cp_page(uint32_t cp, unsigned char *byte) {
    unsigned char b = os_cp1252_exact(cp);
    int lo = 0, hi = OS_PAGE_REV_COUNT - 1;
    if (cp >= 0x20u && cp != 0x7Fu && (b || cp < 0x80u)) { *byte = b; return 0; }
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        uint32_t c = os_page_rev[mid] >> 16;
        if (c == cp) { *byte = (unsigned char)(os_page_rev[mid] & 0xFFu); return (int)((os_page_rev[mid] >> 8) & 0xFFu); }
        if (c < cp) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}

/* Writes code point cp as display bytes at out (cap bytes), the selector
 * first when it is not in the current page *page (updated). Returns the
 * bytes written, 0 when cp is dropped (controls, zero width), -1 when it
 * does not fit. A code point no page has takes os_cp1252_from_cp (a
 * close ASCII form or '?'). */
static inline int os_display_put(uint32_t cp, int *page, char *out, int cap) {
    unsigned char b = 0;
    int p = cp < 0x20u ? -1 : os_cp_page(cp, &b), n = 0;
    if (p < 0) {
        b = os_cp1252_from_cp(cp);
        if (!b) return 0;
        p = b >= 0x80u ? 0 : *page;
    }
    if (b >= 0x80u && p != *page) n = 1;
    if (n + 1 > cap) return -1;
    if (n) { out[0] = (char)os_page_sel[p]; *page = p; }
    out[n] = (char)b;
    return n + 1;
}

/* Cells a display string takes (selectors take none). */
static inline int os_display_cells(const char *s, int n) {
    int i, c = 0;
    for (i = 0; i < n && s[i]; i++)
        if (os_page_of_sel((unsigned char)s[i]) < 0) c++;
    return c;
}

/* Page in force at s[pos] (the last selector before it, else 0). */
static inline int os_display_page_at(const char *s, int pos) {
    int i, p = 0;
    for (i = 0; i < pos && s[i]; i++) {
        int k = os_page_of_sel((unsigned char)s[i]);
        if (k >= 0) p = k;
    }
    return p;
}

/* Bytes of s (from byte 0, page 0) that hold at most cells cells: the
 * prefix to draw in a field cells wide. */
static inline int os_display_take(const char *s, int n, int cells) {
    int i, c = 0;
    for (i = 0; i < n && s[i]; i++) {
        if (c == cells) break;
        if (os_page_of_sel((unsigned char)s[i]) < 0) c++;
    }
    return i;
}

/* Converts text to display bytes. Well formed UTF-8 sequences are
 * decoded; any other byte >= 0x80 is taken as already display text (so
 * the function is idempotent on display text). Lot 72: a decoded code
 * point outside CP1252 is written with its page selector; a selector in
 * the input is display text, and after one (other than the page 0 one)
 * high bytes are copied, not decoded as UTF-8 (Cyrillic display bytes
 * often look like UTF-8 pairs). Returns the output length; out is always
 * NUL-terminated when cap > 0. */
static inline int os_text_to_display(const char *in, int n, char *out, int cap) {
    int i = 0, o = 0, ip = 0, op = 0;
    if (cap <= 0) return 0;
    while (i < n && in[i] && o < cap - 1) {
        unsigned char c = (unsigned char)in[i];
        int sp;
        if (c >= 0x80u && ip == 0 && os_utf8_seq_ok(in, n, i)) {
            int k = i, r;
            uint32_t cp = os_utf8_next(in, n, &k);
            r = os_display_put(cp, &op, out + o, cap - 1 - o);
            if (r < 0) break;
            i = k;
            o += r;
            continue;
        }
        i++;
        sp = os_page_of_sel(c);
        if (sp >= 0) {
            ip = sp;
            if (op != sp) { out[o++] = (char)c; op = sp; }
            continue;
        }
        if (c < 0x20u && c != '\n' && c != '\t') continue;
        if (c >= 0x80u && op != ip) {
            if (o + 2 > cap - 1) break;
            out[o++] = (char)os_page_sel[ip];
            op = ip;
        }
        out[o++] = (char)c;
    }
    out[o] = 0;
    return o;
}

/* In place version (the output is never longer than the input: a raw
 * high byte that would need a selector it has no room for becomes '?'). */
static inline int os_text_to_display_inplace(char *s, int cap) {
    int r = 0, w = 0, n = 0, ip = 0, op = 0;
    while (n < cap && s[n]) n++;
    while (r < n) {
        unsigned char c = (unsigned char)s[r];
        int sp;
        if (c >= 0x80u && ip == 0 && os_utf8_seq_ok(s, n, r)) {
            char tmp[2];
            int k = r, m, j;
            uint32_t cp = os_utf8_next(s, n, &k);
            m = os_display_put(cp, &op, tmp, 2);
            r = k;
            for (j = 0; j < m; j++) s[w++] = tmp[j];
            continue;
        }
        r++;
        sp = os_page_of_sel(c);
        if (sp >= 0) {
            ip = sp;
            if (op != sp) { s[w++] = (char)c; op = sp; }
            continue;
        }
        if (c < 0x20u && c != '\n' && c != '\t') continue;
        if (c >= 0x80u && op != ip) {
            if (w + 2 > r) { s[w++] = '?'; continue; }
            s[w++] = (char)os_page_sel[ip];
            op = ip;
        }
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
/* Lot 72: UTF-8 of display byte b in page p (a selector or an undefined
 * byte of the page gives '?'; callers track selectors themselves). */
static inline int os_display_to_utf8_page(unsigned page, unsigned char b, char *out, int cap) {
    uint32_t cp;
    if (page == 0u || b < 0x80u) return os_display_to_utf8(b, out, cap);
    cp = os_display_cp(page, b);
    if (!cp) cp = '?';
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

/* A display string (selectors included) as UTF-8; returns the length,
 * out NUL-terminated, cut on a whole character. */
static inline int os_display_str_to_utf8(const char *s, int n, char *out, int cap) {
    int i, o = 0, page = 0;
    if (cap <= 0) return 0;
    for (i = 0; i < n && s[i]; i++) {
        int k = os_page_of_sel((unsigned char)s[i]), m;
        if (k >= 0) { page = k; continue; }
        m = os_display_to_utf8_page((unsigned)page, (unsigned char)s[i], out + o, cap - 1 - o);
        if (m == 0) break;
        o += m;
    }
    out[o] = 0;
    return o;
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
    /* Lot 72: the Greek letters and signs of CP437 0xE0..0xFE */
    case 0x3B1: return 0xE0;
    case 0x3B2: return 0xE1;
    case 0x393: return 0xE2;
    case 0x3C0: return 0xE3;
    case 0x3A3: return 0xE4;
    case 0x3C3: return 0xE5;
    case 0x3BC: return 0xE6;
    case 0x3C4: return 0xE7;
    case 0x3A6: return 0xE8;
    case 0x398: return 0xE9;
    case 0x3A9: return 0xEA;
    case 0x3B4: return 0xEB;
    case 0x221E: return 0xEC;
    case 0x3C6: return 0xED;
    case 0x3B5: return 0xEE;
    case 0x2229: return 0xEF;
    case 0x2261: return 0xF0;
    case 0x2265: return 0xF2;
    case 0x2264: return 0xF3;
    case 0x2248: return 0xF7;
    case 0x221A: return 0xFB;
    case 0x25A0: return 0xFE;
    default: break;
    }
    /* Lot 72: Greek and Cyrillic letters drawn like a Latin one */
    {
        static const uint16_t look[] = {
            0x391, 'A', 0x392, 'B', 0x395, 'E', 0x396, 'Z', 0x397, 'H', 0x399, 'I', 0x39A, 'K', 0x39C, 'M',
            0x39D, 'N', 0x39F, 'O', 0x3A1, 'P', 0x3A4, 'T', 0x3A5, 'Y', 0x3A7, 'X', 0x3BF, 'o', 0x3BD, 'v',
            0x405, 'S', 0x406, 'I', 0x408, 'J', 0x410, 'A', 0x412, 'B', 0x415, 'E', 0x41A, 'K', 0x41C, 'M',
            0x41D, 'H', 0x41E, 'O', 0x420, 'P', 0x421, 'C', 0x422, 'T', 0x425, 'X', 0x430, 'a', 0x435, 'e',
            0x43E, 'o', 0x440, 'p', 0x441, 'c', 0x443, 'y', 0x445, 'x', 0x455, 's', 0x456, 'i', 0x458, 'j'
        };
        for (i = 0; i < sizeof(look) / sizeof(look[0]); i += 2)
            if (look[i] == cp) return (unsigned char)look[i + 1];
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
