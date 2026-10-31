/* edid.c - Lot 69 (US-080): EDID 1.x base block parser (see edid.h). */
#include "edid.h"

uint32_t edid_isqrt(uint32_t v) {
    uint32_t r = 0, bit = 1u << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= r + bit) { v -= r + bit; r = (r >> 1) + bit; }
        else r >>= 1;
        bit >>= 2;
    }
    return r;
}

static void zero(edid_info_t *e) {
    uint8_t *p = (uint8_t *)e;
    unsigned i;
    for (i = 0; i < sizeof(*e); i++) p[i] = 0;
}

int edid_parse(const uint8_t *b, int len, edid_info_t *e) {
    static const uint8_t hdr[8] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
    unsigned sum = 0;
    int i, k;
    if (!e) return -3;
    zero(e);
    if (!b || len < EDID_BLOCK) return -3;
    for (i = 0; i < 8; i++) if (b[i] != hdr[i]) return -1;
    for (i = 0; i < EDID_BLOCK; i++) sum += b[i];
    if ((sum & 0xFFu) != 0) return -2;
    {
        uint16_t id = (uint16_t)((b[8] << 8) | b[9]);
        e->mfg[0] = (char)('A' - 1 + ((id >> 10) & 31));
        e->mfg[1] = (char)('A' - 1 + ((id >> 5) & 31));
        e->mfg[2] = (char)('A' - 1 + (id & 31));
        e->mfg[3] = 0;
        for (k = 0; k < 3; k++) if (e->mfg[k] < 'A' || e->mfg[k] > 'Z') e->mfg[k] = '?';
    }
    e->product = (uint16_t)(b[10] | (b[11] << 8));
    e->year = b[17] ? 1990 + b[17] : 0;
    e->version = b[18];
    e->revision = b[19];
    /* bytes 21/22: size in cm; both 0 = unknown; one 0 = aspect only (1.4) */
    if (b[21] && b[22]) { e->cm_w = b[21]; e->cm_h = b[22]; }
    /* first detailed descriptor (54..71) with a pixel clock is the preferred timing */
    for (k = 0; k < 4; k++) {
        const uint8_t *d = b + 54 + 18 * k;
        if (d[0] == 0 && d[1] == 0) continue;
        e->native_w = d[2] | ((d[4] & 0xF0) << 4);
        e->native_h = d[5] | ((d[7] & 0xF0) << 4);
        e->mm_w = d[12] | ((d[14] & 0xF0) << 4);
        e->mm_h = d[13] | ((d[14] & 0x0F) << 8);
        break;
    }
    /* sizes under 3 cm are placeholders (projectors report 0, some 1x1) */
    if (e->mm_w >= 30 && e->mm_h >= 30) { e->size_mm_w = e->mm_w; e->size_mm_h = e->mm_h; }
    else if (e->cm_w >= 3 && e->cm_h >= 3) { e->size_mm_w = e->cm_w * 10; e->size_mm_h = e->cm_h * 10; }
    if (e->size_mm_w) {
        uint32_t d2 = (uint32_t)(e->size_mm_w * e->size_mm_w + e->size_mm_h * e->size_mm_h);
        e->diag_tenths = (int)((edid_isqrt(d2 * 100u) * 10u + 127u) / 254u);
    }
    e->ok = 1;
    return 0;
}

static int put(char *o, int cap, int n, const char *s) {
    while (*s && n < cap - 1) o[n++] = *s++;
    o[n] = 0;
    return n;
}

static int put_u(char *o, int cap, int n, unsigned v) {
    char t[12];
    int i = 0;
    do { t[i++] = (char)('0' + v % 10u); v /= 10u; } while (v && i < 11);
    while (i > 0 && n < cap - 1) o[n++] = t[--i];
    o[n] = 0;
    return n;
}

int edid_describe(const edid_info_t *e, char *o, int cap) {
    int n = 0;
    if (!o || cap < 2) return 0;
    o[0] = 0;
    if (!e || !e->ok) return put(o, cap, 0, "edid absent");
    n = put(o, cap, n, "edid ok ");
    n = put(o, cap, n, e->mfg);
    n = put(o, cap, n, " ");
    n = put_u(o, cap, n, (unsigned)e->native_w);
    n = put(o, cap, n, "x");
    n = put_u(o, cap, n, (unsigned)e->native_h);
    n = put(o, cap, n, " ");
    if (e->size_mm_w) {
        n = put_u(o, cap, n, (unsigned)e->size_mm_w);
        n = put(o, cap, n, "x");
        n = put_u(o, cap, n, (unsigned)e->size_mm_h);
        n = put(o, cap, n, "mm ");
        n = put_u(o, cap, n, (unsigned)(e->diag_tenths / 10));
        n = put(o, cap, n, ".");
        n = put_u(o, cap, n, (unsigned)(e->diag_tenths % 10));
        n = put(o, cap, n, "in");
    } else n = put(o, cap, n, "size unknown");
    return n;
}
