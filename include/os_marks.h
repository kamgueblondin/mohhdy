/* os_marks.h - Lot 85: Vietnamese and horn letters on the bytes the
 * Lot 72 glyph pages leave undefined. The bitmap fonts stay as
 * scripts/gen_bitmap_font.py wrote them (font-check). A letter is the
 * base glyph shifted down one row when an accent sits above it, and
 * the accent is painted into the free row. Sixteen rarer combinations
 * (hook above a circumflex, breve or horn) have no free byte: they
 * become the base letter, which is a letter and not '?'.
 * Header only, freestanding, no allocation. */
#ifndef OS_MARKS_H
#define OS_MARKS_H
#include <stdint.h>

#define OS_MARK_NONE 0
#define OS_MARK_GRAVE 1
#define OS_MARK_ACUTE 2
#define OS_MARK_CIRC 3
#define OS_MARK_TILDE 4
#define OS_MARK_BREVE 5
#define OS_MARK_HOOK 6
#define OS_MARK_HORN 7
#define OS_MARK_DOTB 8
#define OS_MARK_COUNT 78

typedef struct {
    uint16_t cp;
    uint8_t base;     /* ASCII letter the fonts already draw */
    uint8_t m1, m2;   /* OS_MARK_*, m2 is 0 when there is one mark */
    uint8_t page;     /* glyph page 1..4 */
    uint8_t byte;     /* display byte on that page */
} os_mark_rec_t;

static const os_mark_rec_t os_mark_rec[OS_MARK_COUNT] = {
    { 0x1EA0, 0x41, 8, 0, 2, 0x80 },
    { 0x1EA1, 0x61, 8, 0, 2, 0x82 },
    { 0x1EA2, 0x41, 6, 0, 2, 0x83 },
    { 0x1EA3, 0x61, 6, 0, 2, 0x84 },
    { 0x1EA4, 0x41, 3, 2, 2, 0x85 },
    { 0x1EA5, 0x61, 3, 2, 2, 0x86 },
    { 0x1EA6, 0x41, 3, 1, 2, 0x87 },
    { 0x1EA7, 0x61, 3, 1, 2, 0x88 },
    { 0x1EAA, 0x41, 3, 4, 2, 0x89 },
    { 0x1EAB, 0x61, 3, 4, 2, 0x8A },
    { 0x1EAC, 0x41, 8, 3, 2, 0x8B },
    { 0x1EAD, 0x61, 8, 3, 2, 0x8C },
    { 0x1EAE, 0x41, 5, 2, 2, 0x8E },
    { 0x1EAF, 0x61, 5, 2, 2, 0x91 },
    { 0x1EB0, 0x41, 5, 1, 2, 0x92 },
    { 0x1EB1, 0x61, 5, 1, 2, 0x93 },
    { 0x1EB8, 0x45, 8, 0, 2, 0x94 },
    { 0x1EB9, 0x65, 8, 0, 2, 0x95 },
    { 0x1EBA, 0x45, 6, 0, 2, 0x96 },
    { 0x1EBB, 0x65, 6, 0, 2, 0x97 },
    { 0x1EBC, 0x45, 4, 0, 2, 0x98 },
    { 0x1EBD, 0x65, 4, 0, 2, 0x99 },
    { 0x1EBE, 0x45, 3, 2, 2, 0x9A },
    { 0x1EBF, 0x65, 3, 2, 2, 0x9B },
    { 0x1EC0, 0x45, 3, 1, 2, 0x9C },
    { 0x1EC1, 0x65, 3, 1, 2, 0x9E },
    { 0x1EC4, 0x45, 3, 4, 2, 0x9F },
    { 0x1EC5, 0x65, 3, 4, 2, 0xAD },
    { 0x1EC6, 0x45, 8, 3, 2, 0xAE },
    { 0x1EC7, 0x65, 8, 3, 2, 0xD2 },
    { 0x1EC8, 0x49, 6, 0, 2, 0xFF },
    { 0x1EC9, 0x69, 6, 0, 3, 0x98 },
    { 0x1ECA, 0x49, 8, 0, 3, 0x99 },
    { 0x1ECB, 0x69, 8, 0, 3, 0x9A },
    { 0x1ECC, 0x4F, 8, 0, 3, 0x9B },
    { 0x1ECD, 0x6F, 8, 0, 3, 0x9C },
    { 0x1ECE, 0x4F, 6, 0, 3, 0x9E },
    { 0x1ECF, 0x6F, 6, 0, 3, 0x9F },
    { 0x1ED0, 0x4F, 3, 2, 3, 0xAD },
    { 0x1ED1, 0x6F, 3, 2, 4, 0xDA },
    { 0x1ED2, 0x4F, 3, 1, 4, 0xDB },
    { 0x1ED3, 0x6F, 3, 1, 4, 0xDC },
    { 0x1ED6, 0x4F, 3, 4, 4, 0xDD },
    { 0x1ED7, 0x6F, 3, 4, 4, 0xDE },
    { 0x1ED8, 0x4F, 8, 3, 4, 0xDF },
    { 0x1ED9, 0x6F, 8, 3, 4, 0xE0 },
    { 0x1EDA, 0x4F, 7, 2, 4, 0xE1 },
    { 0x1EDB, 0x6F, 7, 2, 4, 0xE2 },
    { 0x1EDC, 0x4F, 7, 1, 4, 0xE3 },
    { 0x1EDD, 0x6F, 7, 1, 4, 0xE4 },
    { 0x1EE0, 0x4F, 7, 4, 4, 0xE5 },
    { 0x1EE1, 0x6F, 7, 4, 4, 0xE6 },
    { 0x1EE2, 0x4F, 7, 8, 4, 0xE7 },
    { 0x1EE3, 0x6F, 7, 8, 4, 0xE8 },
    { 0x1EE4, 0x55, 8, 0, 4, 0xE9 },
    { 0x1EE5, 0x75, 8, 0, 4, 0xEA },
    { 0x1EE6, 0x55, 6, 0, 4, 0xEB },
    { 0x1EE7, 0x75, 6, 0, 4, 0xEC },
    { 0x1EE8, 0x55, 7, 2, 4, 0xED },
    { 0x1EE9, 0x75, 7, 2, 4, 0xEE },
    { 0x1EEA, 0x55, 7, 1, 4, 0xEF },
    { 0x1EEB, 0x75, 7, 1, 4, 0xF0 },
    { 0x1EEE, 0x55, 7, 4, 4, 0xF1 },
    { 0x1EEF, 0x75, 7, 4, 4, 0xF2 },
    { 0x1EF0, 0x55, 7, 8, 4, 0xF3 },
    { 0x1EF1, 0x75, 7, 8, 4, 0xF4 },
    { 0x1EF2, 0x59, 1, 0, 4, 0xF5 },
    { 0x1EF3, 0x79, 1, 0, 4, 0xF6 },
    { 0x1EF4, 0x59, 8, 0, 4, 0xF7 },
    { 0x1EF5, 0x79, 8, 0, 4, 0xF8 },
    { 0x1EF6, 0x59, 6, 0, 4, 0xF9 },
    { 0x1EF7, 0x79, 6, 0, 4, 0xFA },
    { 0x1EF8, 0x59, 4, 0, 4, 0xFB },
    { 0x1EF9, 0x79, 4, 0, 4, 0xFC },
    { 0x01A0, 0x4F, 7, 0, 4, 0xFD },
    { 0x01A1, 0x6F, 7, 0, 4, 0xFE },
    { 0x01AF, 0x55, 7, 0, 4, 0xFF },
    { 0x01B0, 0x75, 7, 0, 1, 0xFE },
};

/* No free byte: the reader still shows the base letter. */
static const struct { uint16_t cp; uint8_t base; } os_mark_fall[] = {
    { 0x1EA8, 0x41 },
    { 0x1EA9, 0x61 },
    { 0x1EB2, 0x41 },
    { 0x1EB3, 0x61 },
    { 0x1EB4, 0x41 },
    { 0x1EB5, 0x61 },
    { 0x1EB6, 0x41 },
    { 0x1EB7, 0x61 },
    { 0x1EC2, 0x45 },
    { 0x1EC3, 0x65 },
    { 0x1ED4, 0x4F },
    { 0x1ED5, 0x6F },
    { 0x1EDE, 0x4F },
    { 0x1EDF, 0x6F },
    { 0x1EEC, 0x55 },
    { 0x1EED, 0x75 },
};

static inline int os_mark_above(int m) {
    return m == OS_MARK_GRAVE || m == OS_MARK_ACUTE || m == OS_MARK_CIRC ||
           m == OS_MARK_TILDE || m == OS_MARK_BREVE || m == OS_MARK_HOOK;
}

static inline int os_mark_index(uint32_t cp) {
    int i;
    if (cp < 0x1A0u || cp > 0x1EF9u) return -1;
    for (i = 0; i < OS_MARK_COUNT; i++) if (os_mark_rec[i].cp == cp) return i;
    return -1;
}

static inline int os_mark_index_byte(unsigned page, unsigned byte) {
    int i;
    if (page < 1u || page > 4u || byte < 0x80u) return -1;
    for (i = 0; i < OS_MARK_COUNT; i++)
        if (os_mark_rec[i].page == page && os_mark_rec[i].byte == byte) return i;
    return -1;
}

/* ASCII base of a composed letter (drawn or fallen back), else 0. */
static inline unsigned char os_mark_ascii_base(uint32_t cp) {
    int i = os_mark_index(cp), k, n;
    if (i >= 0) return os_mark_rec[i].base;
    n = (int)(sizeof(os_mark_fall) / sizeof(os_mark_fall[0]));
    for (k = 0; k < n; k++) if (os_mark_fall[k].cp == cp) return os_mark_fall[k].base;
    return 0;
}

/* Bit 0 is the left pixel, the same order as gfx_font8. */
static inline unsigned os_mark_above_bits(int m) {
    switch (m) {
    case OS_MARK_GRAVE: return 0x10u;
    case OS_MARK_ACUTE: return 0x04u;
    case OS_MARK_CIRC:  return 0x14u;
    case OS_MARK_TILDE: return 0x2Au;
    case OS_MARK_BREVE: return 0x22u;
    case OS_MARK_HOOK:  return 0x18u;
    default: return 0;
    }
}

/* Row 0..7. An above mark takes row 0 and the base moves down one row.
 * A dot below takes the centre of row 7. A horn is two pixels on the
 * right of rows 2 and 3. Two above marks share row 0. */
static inline unsigned os_mark_compose_row(const uint8_t base_rows[8], int row, int m1, int m2) {
    int above = os_mark_above(m1) || os_mark_above(m2);
    int below = (m1 == OS_MARK_DOTB) || (m2 == OS_MARK_DOTB);
    int horn = (m1 == OS_MARK_HORN) || (m2 == OS_MARK_HORN);
    int src;
    unsigned bits;
    if (row < 0 || row > 7 || !base_rows) return 0;
    if (above && row == 0) {
        bits = os_mark_above_bits(m1) | os_mark_above_bits(m2);
        if (os_mark_above(m1) && os_mark_above(m2)) bits |= (os_mark_above_bits(m2) << 1);
        return bits & 0xFFu;
    }
    src = above ? row - 1 : row;
    if (below && src >= 7) src = 6;
    bits = (src >= 0 && src < 8) ? base_rows[src] : 0;
    if (below && row == 7) bits = 0x08u;
    if (horn && (row == 2 || row == 3)) bits |= 0x03u;
    return bits & 0xFFu;
}
#endif
