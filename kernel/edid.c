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
    e->serial = (uint32_t)b[12] | ((uint32_t)b[13] << 8) | ((uint32_t)b[14] << 16) | ((uint32_t)b[15] << 24);
    e->week = (b[16] >= 1 && b[16] <= 54) ? b[16] : 0;
    e->year = b[17] ? 1990 + b[17] : 0;
    e->version = b[18];
    e->revision = b[19];
    /* bytes 21/22: size in cm; both 0 = unknown; one 0 = aspect only (1.4) */
    if (b[21] && b[22]) { e->cm_w = b[21]; e->cm_h = b[22]; }
    e->digital = (b[20] & 0x80u) ? 1 : 0;
    /* gamma: stored 0 and 255 mean "not stated" (EDID 1.4) */
    if (b[23] && b[23] != 255) e->gamma_100 = 100 + b[23];
    /* Chromaticity, bytes 25..34. The low two bits are packed in 25 and 26. */
    {
        int rx = (b[27] << 2) | (b[25] >> 6);
        int ry = (b[28] << 2) | ((b[25] >> 4) & 3);
        int wx = (b[33] << 2) | ((b[26] >> 2) & 3);
        int wy = (b[34] << 2) | (b[26] & 3);
        int any = 0, bi;
        for (bi = 25; bi <= 34; bi++) if (b[bi]) any = 1;
        (void)rx; (void)ry;
        if (any) { e->chroma_ok = 1; e->chroma_wx = wx; e->chroma_wy = wy; }
    }
    /* established timings, bytes 35..37, the 17 VESA modes in bit order */
    {
        unsigned bits = (unsigned)b[35] << 9 | (unsigned)b[36] << 1 | ((b[37] >> 7) & 1u);
        int bit;
        e->established = 0;
        for (bit = 0; bit < 17; bit++) if (bits & (1u << (16 - bit))) {
            e->established |= 1 << bit;
            e->established_n++;
        }
    }
    /* standard timings, 8 entries of 2 bytes at offset 38. 00 00 and 01 01 are unused. */
    for (k = 0; k < 8; k++) {
        const uint8_t *s = b + 38 + 2 * k;
        int aspect, hz, w, h;
        if ((s[0] == 0 && s[1] == 0) || (s[0] == 1 && s[1] == 1)) continue;
        w = ((int)s[0] + 31) * 8;
        aspect = s[1] >> 6;
        hz = (s[1] & 0x3Fu) + 60;
        if (aspect == 0) h = w * 10 / 16;
        else if (aspect == 1) h = w * 3 / 4;
        else if (aspect == 2) h = w * 4 / 5;
        else h = w * 9 / 16;
        if (w < 256 || w > 4096 || h < 160 || hz < 60 || hz > 123) continue;
        e->standard_w[e->standard_n] = w;
        e->standard_h[e->standard_n] = h;
        e->standard_hz[e->standard_n] = hz;
        e->standard_n++;
    }
    /* detailed descriptors: a pixel clock is a timing; 0xFC name, 0xFD range */
    for (k = 0; k < 4; k++) {
        const uint8_t *d = b + 54 + 18 * k;
        unsigned clock;
        int ha, va, t;
        if (d[0] == 0 && d[1] == 0) {
            if (d[3] == 0xFC && !e->name[0]) {
                int p = 0, c;
                for (c = 5; c < 18 && p < 13; c++) {
                    if (d[c] == 0x0A || d[c] == 0) break;
                    if (d[c] >= 32 && d[c] < 127) e->name[p++] = (char)d[c];
                }
                e->name[p] = 0;
            } else if (d[3] == 0xFD) {
                e->range_vmin = d[5];
                e->range_vmax = d[6];
                e->range_hmin = d[7];
                e->range_hmax = d[8];
                e->range_clock = d[9] * 10;
            } else if (d[3] == 0xFF && !e->serial_text[0]) {
                int p = 0, c;
                for (c = 5; c < 18 && p < 13; c++) {
                    if (d[c] == 0x0A || d[c] == 0) break;
                    if (d[c] >= 32 && d[c] < 127) e->serial_text[p++] = (char)d[c];
                }
                e->serial_text[p] = 0;
            }
            continue;
        }
        clock = (unsigned)d[0] | ((unsigned)d[1] << 8);
        {
            int tw = d[2] | ((d[4] & 0xF0) << 4);
            int th = d[5] | ((d[7] & 0xF0) << 4);
            int hz = 0;
            ha = tw + (d[3] | ((d[4] & 0x0F) << 8));
            va = th + (d[6] | ((d[7] & 0x0F) << 8));
            t = ha > tw && va > th ? ha * va : 0;
            if (clock && t) {
                unsigned v = (clock * 10000u) / (unsigned)t;
                if (v >= 24u && v <= 360u) hz = (int)v;
            }
            if (e->dtd_n < 4 && tw > 0 && th > 0) {
                e->dtd_w[e->dtd_n] = tw;
                e->dtd_h[e->dtd_n] = th;
                e->dtd_hz[e->dtd_n] = hz;
                e->dtd_n++;
            }
            if (e->native_w) continue;   /* the first timing stays the preferred one */
            e->native_w = tw;
            e->native_h = th;
            e->mm_w = d[12] | ((d[14] & 0xF0) << 4);
            e->mm_h = d[13] | ((d[14] & 0x0F) << 8);
            if (hz) e->refresh_hz = hz;
        }
    }
    /* sizes under 3 cm are placeholders (projectors report 0, some 1x1) */
    if (e->mm_w >= 30 && e->mm_h >= 30) { e->size_mm_w = e->mm_w; e->size_mm_h = e->mm_h; }
    else if (e->cm_w >= 3 && e->cm_h >= 3) { e->size_mm_w = e->cm_w * 10; e->size_mm_h = e->cm_h * 10; }
    if (e->size_mm_w) {
        uint32_t d2 = (uint32_t)(e->size_mm_w * e->size_mm_w + e->size_mm_h * e->size_mm_h);
        e->diag_tenths = (int)((edid_isqrt(d2 * 100u) * 10u + 127u) / 254u);
    }
    e->ext_blocks = b[126];
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

int edid_established_mode(int bit, int *w, int *h) {
    static const int ew[17] = {
        720, 720, 640, 640, 640, 640, 800, 800,
        800, 800, 832, 1024, 1024, 1024, 1024, 1280, 1152
    };
    static const int eh[17] = {
        400, 400, 480, 480, 480, 480, 600, 600,
        600, 600, 624, 768, 768, 768, 768, 1024, 870
    };
    if (bit < 0 || bit > 16 || !w || !h) return -1;
    *w = ew[bit];
    *h = eh[bit];
    return 0;
}

int edid_established_hz(int bit, int *hz) {
    static const int ez[17] = {
        70, 88, 60, 67, 72, 75, 56, 60,
        72, 75, 75, 87, 60, 70, 75, 75, 75
    };
    if (bit < 0 || bit > 16 || !hz) return -1;
    *hz = ez[bit];
    return 0;
}

int edid_best_mode(const edid_info_t *e, int *w, int *h, int *hz) {
    int bit, best = -1, area = 0, bw = 0, bh = 0, bz = 0;
    if (!e || !e->ok || !w || !h || !hz) return -1;
    if (e->native_w > 0 && e->native_h > 0) {
        *w = e->native_w;
        *h = e->native_h;
        *hz = e->refresh_hz;
        return 0;
    }
    for (bit = 0; bit < 17; bit++) {
        int mw, mh, mz, a;
        if ((e->established & (1 << bit)) == 0) continue;
        if (edid_established_mode(bit, &mw, &mh) != 0) continue;
        if (edid_established_hz(bit, &mz) != 0) continue;
        a = mw * mh;
        if (a > area) { area = a; best = bit; bw = mw; bh = mh; bz = mz; }
    }
    if (best < 0) return -1;
    *w = bw;
    *h = bh;
    *hz = bz;
    (void)best;
    return 0;
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

/* ---- Lot 87: CEA-861 extension ---------------------------------------- */

typedef struct { uint8_t vic; uint16_t w, h; uint8_t hz, il; } vic_mode_t;
/* CTA-861-G table 3, the codes a TV or a monitor commonly lists */
static const vic_mode_t k_vic[] = {
    { 1, 640, 480, 60, 0 },    { 2, 720, 480, 60, 0 },    { 3, 720, 480, 60, 0 },
    { 4, 1280, 720, 60, 0 },   { 5, 1920, 1080, 60, 1 },  { 6, 1440, 480, 60, 1 },
    { 7, 1440, 480, 60, 1 },   { 16, 1920, 1080, 60, 0 }, { 17, 720, 576, 50, 0 },
    { 18, 720, 576, 50, 0 },   { 19, 1280, 720, 50, 0 },  { 20, 1920, 1080, 50, 1 },
    { 21, 1440, 576, 50, 1 },  { 22, 1440, 576, 50, 1 },  { 31, 1920, 1080, 50, 0 },
    { 32, 1920, 1080, 24, 0 }, { 33, 1920, 1080, 25, 0 }, { 34, 1920, 1080, 30, 0 },
    { 47, 1280, 720, 120, 0 }, { 63, 1920, 1080, 120, 0 }, { 64, 1920, 1080, 100, 0 },
    { 93, 3840, 2160, 24, 0 }, { 94, 3840, 2160, 25, 0 }, { 95, 3840, 2160, 30, 0 },
    { 96, 3840, 2160, 50, 0 }, { 97, 3840, 2160, 60, 0 }, { 98, 4096, 2160, 24, 0 },
    { 99, 4096, 2160, 25, 0 }, { 100, 4096, 2160, 30, 0 }, { 101, 4096, 2160, 50, 0 },
    { 102, 4096, 2160, 60, 0 },
};

int edid_vic_mode(int vic, int *w, int *h, int *hz, int *interlaced) {
    unsigned i;
    for (i = 0; i < sizeof(k_vic) / sizeof(k_vic[0]); i++) {
        if (k_vic[i].vic != vic) continue;
        if (w) *w = k_vic[i].w;
        if (h) *h = k_vic[i].h;
        if (hz) *hz = k_vic[i].hz;
        if (interlaced) *interlaced = k_vic[i].il;
        return 0;
    }
    return -1;
}

static void cea_zero(edid_cea_t *c) {
    uint8_t *p = (uint8_t *)c;
    unsigned i;
    for (i = 0; i < sizeof(*c); i++) p[i] = 0;
}

/* an 18 byte detailed timing: active size and refresh (0 if blanking is 0) */
static int dtd_decode(const uint8_t *d, int *w, int *h, int *hz) {
    unsigned clock = (unsigned)d[0] | ((unsigned)d[1] << 8);
    int tw, th, ha, va;
    if (!clock) return -1;
    tw = d[2] | ((d[4] & 0xF0) << 4);
    th = d[5] | ((d[7] & 0xF0) << 4);
    ha = tw + (d[3] | ((d[4] & 0x0F) << 8));
    va = th + (d[6] | ((d[7] & 0x0F) << 8));
    if (tw <= 0 || th <= 0) return -1;
    *w = tw;
    *h = th;
    *hz = 0;
    if (ha > tw && va > th) {
        unsigned v = (clock * 10000u) / (unsigned)(ha * va);
        if (v >= 24u && v <= 360u) *hz = (int)v;
    }
    return 0;
}

static void cea_svd(edid_cea_t *c, const uint8_t *p, int n) {
    int i;
    for (i = 0; i < n; i++) {
        int code = p[i], vic, native = 0;
        if (code == 0 || code == 128 || code >= 254) continue;   /* reserved */
        if (code >= 129 && code <= 192) { vic = code & 0x7F; native = 1; }
        else vic = code;
        c->svd_total++;
        if (c->svd_n >= EDID_CEA_SVD) continue;
        c->vic[c->svd_n] = (uint8_t)vic;
        c->svd_native[c->svd_n] = (uint8_t)native;
        c->svd_n++;
    }
}

static void cea_sad(edid_cea_t *c, const uint8_t *p, int n) {
    int i;
    for (i = 0; i + 3 <= n; i += 3) {
        int fmt = (p[i] >> 3) & 15;
        if (fmt == 0 || fmt == 15) continue;    /* reserved / extended: not kept */
        if (c->sad_n >= EDID_CEA_SAD) return;
        c->sad_format[c->sad_n] = fmt;
        c->sad_channels[c->sad_n] = (p[i] & 7) + 1;
        c->sad_rates[c->sad_n] = p[i + 1] & 0x7F;
        c->sad_extra[c->sad_n] = fmt == EDID_AUDIO_LPCM ? (p[i + 2] & 7) : (fmt <= 8 ? p[i + 2] * 8 : 0);
        c->sad_n++;
    }
}

static void cea_vsdb(edid_cea_t *c, const uint8_t *p, int n) {
    uint32_t oui;
    if (n < 3) { c->unknown_blocks++; return; }
    oui = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
    if (oui == 0x000C03u && n >= 5) {
        c->hdmi = 1;
        c->phys_addr = (uint16_t)((p[3] << 8) | p[4]);
        if (n >= 6) c->hdmi_dc = (p[5] >> 4) & 7;
        if (n >= 7 && p[6]) c->hdmi_tmds_mhz = p[6] * 5;
    } else if (oui == 0xC45DD8u && n >= 5) {
        c->hf = 1;
        if (p[4]) c->hf_tmds_mhz = p[4] * 5;
    } else c->unknown_blocks++;
}

static void cea_ext(edid_cea_t *c, const uint8_t *p, int n) {
    if (n < 1) { c->unknown_blocks++; return; }
    switch (p[0]) {
    case 0:
        if (n < 2) { c->unknown_blocks++; return; }
        c->vcdb = 1;
        c->quant_ycc = (p[1] >> 7) & 1;
        c->quant_rgb = (p[1] >> 6) & 1;
        break;
    case 5:
        if (n < 2) { c->unknown_blocks++; return; }
        c->colorimetry = p[1];
        break;
    case 6:
        if (n < 3) { c->unknown_blocks++; return; }
        c->hdr = 1;
        c->hdr_eotf = p[1] & 0x3F;
        if (n >= 4) c->hdr_max_lum = p[3];
        break;
    case 14:
        c->y420_n += n - 1;
        break;
    default:
        c->unknown_blocks++;
        break;
    }
}

int edid_cea_parse(const uint8_t *b, int len, edid_cea_t *c) {
    unsigned sum = 0;
    int i, d, pos;
    if (!c) return -3;
    cea_zero(c);
    if (!b || len < EDID_BLOCK) return -3;
    if (b[0] != 0x02) return -1;
    for (i = 0; i < EDID_BLOCK; i++) sum += b[i];
    if ((sum & 0xFFu) != 0) return -2;
    c->revision = b[1];
    d = b[2];
    c->dtd_offset = d;
    if (d != 0 && (d < 4 || d > 127)) return -4;
    if (c->revision >= 2) {
        c->underscan = (b[3] >> 7) & 1;
        c->basic_audio = (b[3] >> 6) & 1;
        c->ycbcr444 = (b[3] >> 5) & 1;
        c->ycbcr422 = (b[3] >> 4) & 1;
        c->native_dtds = b[3] & 15;
    }
    /* data block collection, revision 3 only, bytes 4 .. d-1 */
    if (c->revision >= 3 && d > 4) {
        pos = 4;
        while (pos < d) {
            int tag = b[pos] >> 5, n = b[pos] & 31;
            const uint8_t *p = b + pos + 1;
            if (pos + 1 + n > d) return -4;
            switch (tag) {
            case 1: cea_sad(c, p, n); break;
            case 2: cea_svd(c, p, n); break;
            case 3: cea_vsdb(c, p, n); break;
            case 4: if (n >= 1) c->speakers = p[0] | (n >= 2 ? p[1] << 8 : 0); else c->unknown_blocks++; break;
            case 7: cea_ext(c, p, n); break;
            default: c->unknown_blocks++; break;
            }
            pos += 1 + n;
        }
    }
    if (d) {
        for (pos = d; pos + 18 <= 127; pos += 18) {
            int w, h, hz;
            if (b[pos] == 0 && b[pos + 1] == 0) break;   /* padding starts */
            if (dtd_decode(b + pos, &w, &h, &hz) != 0) break;
            if (c->dtd_n >= EDID_CEA_DTD) break;
            c->dtd_w[c->dtd_n] = w;
            c->dtd_h[c->dtd_n] = h;
            c->dtd_hz[c->dtd_n] = hz;
            c->dtd_n++;
        }
    }
    c->ok = 1;
    return 0;
}

int edid_parse_ext(const uint8_t *data, int len, edid_info_t *e, edid_cea_t *c, int *tag) {
    int k;
    if (tag) *tag = 0;
    if (c) cea_zero(c);
    if (!e || !c || edid_parse(data, len, e) != 0) return -1;
    if (e->ext_blocks <= 0) return -1;
    for (k = 1; k <= e->ext_blocks && (k + 1) * EDID_BLOCK <= len; k++) {
        const uint8_t *blk = data + k * EDID_BLOCK;
        if (tag && !*tag) *tag = blk[0];
        if (blk[0] == 0x02 && edid_cea_parse(blk, EDID_BLOCK, c) == 0) {
            if (tag) *tag = 0x02;
            return 0;
        }
    }
    return (len >= 2 * EDID_BLOCK) ? 1 : -1;
}

int edid_cea_best(const edid_cea_t *c, int *w, int *h, int *hz) {
    int i, bw = 0, bh = 0, bz = 0;
    if (!c || !c->ok) return -1;
    for (i = 0; i < c->svd_n; i++) {
        int il = 0;
        if (c->svd_native[i] && edid_vic_mode(c->vic[i], &bw, &bh, &bz, &il) == 0 && !il) {
            *w = bw; *h = bh; *hz = bz;
            return 0;
        }
    }
    if (c->dtd_n) { *w = c->dtd_w[0]; *h = c->dtd_h[0]; *hz = c->dtd_hz[0]; return 0; }
    bw = bh = bz = 0;
    for (i = 0; i < c->svd_n; i++) {
        int vw, vh, vz, il = 0;
        if (edid_vic_mode(c->vic[i], &vw, &vh, &vz, &il) != 0 || il) continue;
        if (vw * vh > bw * bh || (vw * vh == bw * bh && vz > bz)) { bw = vw; bh = vh; bz = vz; }
    }
    if (!bw) return -1;
    *w = bw; *h = bh; *hz = bz;
    return 0;
}

int edid_cea_describe(const edid_cea_t *c, char *o, int cap) {
    int n = 0, w, h, hz;
    if (!o || cap < 2) return 0;
    o[0] = 0;
    if (!c || !c->ok) return put(o, cap, 0, "cea none");
    n = put(o, cap, n, "cea ok rev ");
    n = put_u(o, cap, n, (unsigned)c->revision);
    n = put(o, cap, n, " svd ");
    n = put_u(o, cap, n, (unsigned)c->svd_total);
    if (edid_cea_best(c, &w, &h, &hz) == 0) {
        n = put(o, cap, n, " best ");
        n = put_u(o, cap, n, (unsigned)w);
        n = put(o, cap, n, "x");
        n = put_u(o, cap, n, (unsigned)h);
        if (hz) { n = put(o, cap, n, "@"); n = put_u(o, cap, n, (unsigned)hz); }
    }
    n = put(o, cap, n, " audio ");
    n = put_u(o, cap, n, (unsigned)c->sad_n);
    if (c->hdmi) {
        n = put(o, cap, n, " hdmi ");
        n = put_u(o, cap, n, (unsigned)((c->phys_addr >> 12) & 15));
        n = put(o, cap, n, ".");
        n = put_u(o, cap, n, (unsigned)((c->phys_addr >> 8) & 15));
        n = put(o, cap, n, ".");
        n = put_u(o, cap, n, (unsigned)((c->phys_addr >> 4) & 15));
        n = put(o, cap, n, ".");
        n = put_u(o, cap, n, (unsigned)(c->phys_addr & 15));
    }
    if (c->hdr) n = put(o, cap, n, " hdr");
    n = put(o, cap, n, " dtd ");
    n = put_u(o, cap, n, (unsigned)c->dtd_n);
    return n;
}

/* ---- Lot 87: the summary SYS_DISPLAY_INFO carries -------------------- */

static uint8_t clamp8(int v) { return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v); }

void edid_cea_summarize(const edid_cea_t *c, int parse_rc, int tag, edid_cea_sum_t *s) {
    int i, w = 0, h = 0, hz = 0;
    uint8_t *b = (uint8_t *)s;
    if (!s) return;
    for (i = 0; i < (int)sizeof(*s); i++) b[i] = 0;
    s->ext_tag = clamp8(tag);
    if (parse_rc < 0) { s->status = 1; return; }
    if (parse_rc > 0 || !c || !c->ok) { s->status = (uint8_t)(tag == 0x02 ? 3 : 2); return; }
    s->status = 0;
    s->rev = clamp8(c->revision);
    if (c->underscan) s->flags |= EDID_SUM_UNDERSCAN;
    if (c->basic_audio) s->flags |= EDID_SUM_AUDIO;
    if (c->ycbcr444) s->flags |= EDID_SUM_YCC444;
    if (c->ycbcr422) s->flags |= EDID_SUM_YCC422;
    if (c->hdmi) s->flags |= EDID_SUM_HDMI;
    if (c->hf) s->flags |= EDID_SUM_HF;
    if (c->hdr) s->flags |= EDID_SUM_HDR;
    if (c->vcdb) s->flags |= EDID_SUM_VCDB;
    s->svd_n = clamp8(c->svd_n);
    for (i = 0; i < c->svd_n && i < EDID_CEA_SVD; i++) {
        if (i < EDID_SUM_VICS) s->vic[i] = c->vic[i];
        if (c->svd_native[i] && !s->native_vic) s->native_vic = c->vic[i];
    }
    s->sad_n = clamp8(c->sad_n);
    for (i = 0; i < c->sad_n && i < EDID_CEA_SAD; i++)
        if (c->sad_channels[i] > s->sad_max_ch) s->sad_max_ch = clamp8(c->sad_channels[i]);
    s->dtd_n = clamp8(c->dtd_n);
    s->y420_n = clamp8(c->y420_n);
    s->speakers = clamp8(c->speakers);
    s->hdr_eotf = clamp8(c->hdr_eotf);
    if (c->hdmi) s->phys_addr = c->phys_addr;
    {
        int t = c->hf_tmds_mhz > c->hdmi_tmds_mhz ? c->hf_tmds_mhz : c->hdmi_tmds_mhz;
        s->tmds_mhz = (uint16_t)(t < 0 ? 0 : t > 65535 ? 65535 : t);
    }
    if (edid_cea_best(c, &w, &h, &hz) == 0) {
        s->best_w = (uint16_t)w;
        s->best_h = (uint16_t)h;
        s->best_hz = clamp8(hz);
        /* the same order edid_cea_best uses */
        s->best_from = 3;
        if (s->native_vic) {
            int nw = 0, nh = 0, nz = 0, il = 0;
            if (edid_vic_mode(s->native_vic, &nw, &nh, &nz, &il) == 0 && !il && nw == w && nh == h && nz == hz)
                s->best_from = 1;
        }
        if (s->best_from == 3 && c->dtd_n > 0 && c->dtd_w[0] == w && c->dtd_h[0] == h && c->dtd_hz[0] == hz)
            s->best_from = 2;
    }
}

int edid_cea_is_tv(const edid_cea_sum_t *s, int size_mm_w) {
    if (!s || s->status != 0 || !(s->flags & EDID_SUM_HDMI)) return 0;
    if (s->flags & EDID_SUM_UNDERSCAN) return 1;
    /* CTA TV formats: 480p/576p, 720p, 1080i/p, 2160p families */
    switch (s->native_vic) {
    case 2: case 3: case 4: case 5: case 16: case 17: case 18: case 19: case 20: case 31:
    case 93: case 94: case 95: case 96: case 97:
        return 1;
    default:
        break;
    }
    return size_mm_w == 0;
}
