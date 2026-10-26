/* imgdec_jpeg.inc - Lot 47 (US-G03): JPEG baseline decoder, included by
 * imgdec.c only. Freestanding, no libc, no allocation, bounded loops.
 * Supported: SOF0/SOF1 (8-bit Huffman sequential), 1 or 3 components,
 * sampling factors 1..2, one interleaved scan, DRI restart markers, 8 and
 * 16-bit DQT. Refused with IMGDEC_E_UNSUP: progressive, lossless,
 * arithmetic, 12-bit, CMYK, multi-scan. Output: 8-bit gray or RGB in the
 * caller scratch (w * h * components bytes). Integer IDCT (separable,
 * Q12 cosine table), nearest chroma upsampling. */

static const unsigned char k_zz[64] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21,
    28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61,
    54, 47, 55, 62, 63
};
/* T[x][u] = round(4096 * c(u) / 2 * cos((2x + 1) u pi / 16)), c(0) = 1/sqrt(2) */
static const short k_idct[8][8] = {
    {1448, 2009, 1892, 1703, 1448, 1138, 784, 400},
    {1448, 1703, 784, -400, -1448, -2009, -1892, -1138},
    {1448, 1138, -784, -2009, -1448, 400, 1892, 1703},
    {1448, 400, -1892, -1138, 1448, 1703, -784, -2009},
    {1448, -400, -1892, 1138, 1448, -1703, -784, 2009},
    {1448, -1138, -784, 2009, -1448, -400, 1892, -1703},
    {1448, -1703, 784, 400, -1448, 2009, -1892, 1138},
    {1448, -2009, 1892, -1703, 1448, -1138, 784, -400}
};

typedef struct {
    unsigned char valid, vals[256];
    int maxcode[18], valptr[17], mincode[17];
} jhuff_t;

typedef struct {
    const unsigned char *b;
    int n, pos;
    unsigned bits;
    int nbits, marker;      /* marker seen: feed zeros */
} jbits_t;

typedef struct {
    int id, h, v, tq, td, ta, pred;
} jcomp_t;

static struct {
    unsigned short q[4][64];
    unsigned char qvalid[4];
    jhuff_t dc[4], ac[4];
    jcomp_t c[3];
    int nc, hmax, vmax, dri, sof_seen, sos;
    short blk[6][64];
} g_j;

static int jpeg_build_huff(jhuff_t *t, const unsigned char *cnt, const unsigned char *vals, int total) {
    int l, k = 0, code = 0, i;
    for (i = 0; i < total; i++) t->vals[i] = vals[i];
    for (l = 1; l <= 16; l++) {
        t->valptr[l] = k;
        t->mincode[l] = code;
        code += cnt[l - 1];
        k += cnt[l - 1];
        t->maxcode[l] = cnt[l - 1] ? code - 1 : -1;
        if (code > (1 << l)) return IMGDEC_E_FORMAT;
        code <<= 1;
    }
    t->maxcode[17] = 0x7FFFFFFF;
    t->valid = 1;
    return IMGDEC_OK;
}

static int jbit(jbits_t *r) {
    if (!r->nbits) {
        unsigned c = 0;
        if (!r->marker && r->pos < r->n) {
            c = r->b[r->pos];
            if (c == 0xFF) {
                unsigned d = r->pos + 1 < r->n ? r->b[r->pos + 1] : 0xD9;
                if (d == 0) r->pos += 2;
                else { r->marker = 1; c = 0; }
            } else r->pos++;
        }
        r->bits = c;
        r->nbits = 8;
    }
    r->nbits--;
    return (int)((r->bits >> r->nbits) & 1U);
}

static int jbits(jbits_t *r, int n) {
    int v = 0;
    while (n-- > 0) v = (v << 1) | jbit(r);
    return v;
}

static int jdecode(jbits_t *r, const jhuff_t *t) {
    int l, code = 0;
    if (!t->valid) return -1;
    for (l = 1; l <= 16; l++) {
        code = (code << 1) | jbit(r);
        if (t->maxcode[l] >= 0 && code <= t->maxcode[l] && code >= t->mincode[l]) {
            int i = t->valptr[l] + code - t->mincode[l];
            return i < 256 ? t->vals[i] : -1;
        }
    }
    return -1;
}

static int jextend(int v, int s) { return s && v < (1 << (s - 1)) ? v - (1 << s) + 1 : v; }

static int jpeg_block(jbits_t *r, jcomp_t *c, short *out) {
    int coef[64], k, s, i;
    const unsigned short *q = g_j.q[c->tq];
    for (k = 0; k < 64; k++) coef[k] = 0;
    s = jdecode(r, &g_j.dc[c->td]);
    if (s < 0 || s > 11) return IMGDEC_E_FORMAT;
    c->pred += jextend(jbits(r, s), s);
    coef[0] = c->pred * (int)q[0];
    for (k = 1; k < 64;) {
        int rs = jdecode(r, &g_j.ac[c->ta]), run, sz;
        if (rs < 0) return IMGDEC_E_FORMAT;
        run = rs >> 4; sz = rs & 15;
        if (!sz) {
            if (run != 15) break;    /* EOB */
            k += 16;
            continue;
        }
        k += run;
        if (k > 63) return IMGDEC_E_FORMAT;
        coef[k_zz[k]] = jextend(jbits(r, sz), sz) * (int)q[k];
        k++;
    }
    /* integer IDCT; valid coefficients stay within +-2047 */
    {
        int tmp[64], x, y, u;
        for (i = 0; i < 64; i++) { if (coef[i] > 2047) coef[i] = 2047; if (coef[i] < -2048) coef[i] = -2048; }
        for (y = 0; y < 8; y++)
            for (x = 0; x < 8; x++) {
                int acc = 0;
                for (u = 0; u < 8; u++) acc += k_idct[x][u] * coef[y * 8 + u];
                tmp[y * 8 + x] = acc >> 10;          /* Q2 */
            }
        for (x = 0; x < 8; x++)
            for (y = 0; y < 8; y++) {
                int acc = 0, v;
                for (u = 0; u < 8; u++) acc += k_idct[y][u] * tmp[u * 8 + x];
                v = ((acc + (1 << 13)) >> 14) + 128;
                out[y * 8 + x] = (short)(v < 0 ? 0 : v > 255 ? 255 : v);
            }
    }
    return IMGDEC_OK;
}

static int jseg_len(const unsigned char *b, int n, int pos) {
    int len;
    if (pos + 2 > n) return -1;
    len = (b[pos] << 8) | b[pos + 1];
    if (len < 2 || pos + len > n) return -1;
    return len;
}

/* parse header up to SOS (decode = 0 stops at SOF for probe) */
static int jpeg_headers(const unsigned char *b, int n, imgdec_t *img, int decode) {
    int pos = 2, guard = 0;
    unsigned i;
    for (i = 0; i < 4; i++) { g_j.qvalid[i] = 0; g_j.dc[i].valid = 0; g_j.ac[i].valid = 0; }
    g_j.dri = 0; g_j.sof_seen = 0; g_j.sos = 0;
    while (pos + 4 <= n && guard++ < 4096) {
        int m, len, p;
        if (b[pos] != 0xFF) return IMGDEC_E_FORMAT;
        m = b[pos + 1];
        if (m == 0xFF) { pos++; continue; }
        if (m == 0xD9) break;
        len = jseg_len(b, n, pos + 2);
        if (len < 0) return IMGDEC_E_TRUNC;
        p = pos + 4;
        if (m == 0xC0 || m == 0xC1) {
            int k;
            if (len < 8) return IMGDEC_E_FORMAT;
            img->fmt = IMGDEC_JPEG;
            img->h = (b[p + 1] << 8) | b[p + 2];
            img->w = (b[p + 3] << 8) | b[p + 4];
            g_j.nc = b[p + 5];
            img->channels = g_j.nc;
            img->bpp = 8 * g_j.nc;
            if (b[p] != 8) return IMGDEC_E_UNSUP;
            if (!sane(img->w, img->h)) return IMGDEC_E_TOOBIG;
            if (g_j.nc != 1 && g_j.nc != 3) return IMGDEC_E_UNSUP;
            if (len < 8 + 3 * g_j.nc) return IMGDEC_E_FORMAT;
            g_j.hmax = g_j.vmax = 1;
            for (k = 0; k < g_j.nc; k++) {
                jcomp_t *c = &g_j.c[k];
                c->id = b[p + 6 + k * 3];
                c->h = b[p + 7 + k * 3] >> 4; c->v = b[p + 7 + k * 3] & 15;
                c->tq = b[p + 8 + k * 3];
                if (c->h < 1 || c->h > 2 || c->v < 1 || c->v > 2 || c->tq > 3) return IMGDEC_E_UNSUP;
                if (c->h > g_j.hmax) g_j.hmax = c->h;
                if (c->v > g_j.vmax) g_j.vmax = c->v;
            }
            if (g_j.nc == 1) g_j.c[0].h = g_j.c[0].v = g_j.hmax = g_j.vmax = 1;
            else if (g_j.c[1].h != 1 || g_j.c[1].v != 1 || g_j.c[2].h != 1 || g_j.c[2].v != 1) return IMGDEC_E_UNSUP;
            g_j.sof_seen = 1;
            if (!decode) return IMGDEC_OK;
        } else if ((m >= 0xC2 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC)) {
            int k;
            img->fmt = IMGDEC_JPEG;          /* progressive / lossless / arithmetic */
            if (len >= 8) { img->h = (b[p + 1] << 8) | b[p + 2]; img->w = (b[p + 3] << 8) | b[p + 4]; k = b[p + 5]; img->channels = k; img->bpp = 8 * k; }
            return IMGDEC_E_UNSUP;
        } else if (m == 0xDB) {
            int e = pos + 2 + len;
            while (p < e) {
                int pq = b[p] >> 4, tq = b[p] & 15, k;
                if (tq > 3 || pq > 1 || p + 1 + 64 * (pq + 1) > e) return IMGDEC_E_FORMAT;
                for (k = 0; k < 64; k++)
                    g_j.q[tq][k] = pq ? (unsigned short)((b[p + 1 + 2 * k] << 8) | b[p + 2 + 2 * k]) : b[p + 1 + k];
                g_j.qvalid[tq] = 1;
                p += 1 + 64 * (pq + 1);
            }
        } else if (m == 0xC4) {
            int e = pos + 2 + len;
            while (p < e) {
                int tc = b[p] >> 4, th = b[p] & 15, total = 0, k, rc;
                if (tc > 1 || th > 3 || p + 17 > e) return IMGDEC_E_FORMAT;
                for (k = 0; k < 16; k++) total += b[p + 1 + k];
                if (total > 256 || p + 17 + total > e) return IMGDEC_E_FORMAT;
                rc = jpeg_build_huff(tc ? &g_j.ac[th] : &g_j.dc[th], b + p + 1, b + p + 17, total);
                if (rc) return rc;
                p += 17 + total;
            }
        } else if (m == 0xDD) {
            if (len < 4) return IMGDEC_E_FORMAT;
            g_j.dri = (b[p] << 8) | b[p + 1];
        } else if (m == 0xDA) {
            int ns = b[p], k;
            if (!g_j.sof_seen) return IMGDEC_E_FORMAT;
            if (ns != g_j.nc || len < 6 + 2 * ns) return IMGDEC_E_UNSUP;   /* one interleaved scan only */
            for (k = 0; k < ns; k++) {
                int id = b[p + 1 + 2 * k], t = b[p + 2 + 2 * k], j;
                for (j = 0; j < g_j.nc; j++) if (g_j.c[j].id == id) break;
                if (j == g_j.nc || j != k) return IMGDEC_E_FORMAT;
                g_j.c[k].td = t >> 4; g_j.c[k].ta = t & 15;
                if (g_j.c[k].td > 3 || g_j.c[k].ta > 3) return IMGDEC_E_FORMAT;
            }
            g_j.sos = pos + 2 + len;
            return IMGDEC_OK;
        }
        pos += 2 + len;
    }
    return g_j.sof_seen ? IMGDEC_E_TRUNC : IMGDEC_E_FORMAT;
}

static int probe_jpeg(const unsigned char *b, int n, imgdec_t *img) {
    if (n < 2 || b[0] != 0xFF || b[1] != 0xD8) return IMGDEC_E_FORMAT;
    if (n < 4) return IMGDEC_E_TRUNC;
    img->fmt = IMGDEC_JPEG;
    return jpeg_headers(b, n, img, 0);
}

static unsigned jpeg_scratch_need(const imgdec_t *img) {
    return (unsigned)img->w * (unsigned)img->h * (unsigned)img->channels;
}

static unsigned char jclamp(int v) { return (unsigned char)(v < 0 ? 0 : v > 255 ? 255 : v); }

static int open_jpeg(const unsigned char *b, int n, unsigned char *scratch, unsigned smax, imgdec_t *img) {
    jbits_t r;
    int rc, mx, my, mcuw, mcuh, nmx, nmy, k, left;
    unsigned need = jpeg_scratch_need(img);
    if (!scratch || need > smax) return IMGDEC_E_TOOBIG;
    rc = jpeg_headers(b, n, img, 1);
    if (rc) return rc;
    if (!g_j.sos) return IMGDEC_E_TRUNC;
    for (k = 0; k < g_j.nc; k++) {
        if (!g_j.qvalid[g_j.c[k].tq] || !g_j.dc[g_j.c[k].td].valid || !g_j.ac[g_j.c[k].ta].valid) return IMGDEC_E_FORMAT;
        g_j.c[k].pred = 0;
    }
    r.b = b; r.n = n; r.pos = g_j.sos; r.bits = 0; r.nbits = 0; r.marker = 0;
    mcuw = 8 * g_j.hmax; mcuh = 8 * g_j.vmax;
    nmx = (img->w + mcuw - 1) / mcuw; nmy = (img->h + mcuh - 1) / mcuh;
    left = g_j.dri;
    for (my = 0; my < nmy; my++)
        for (mx = 0; mx < nmx; mx++) {
            int bi = 0, px, py;
            if (g_j.dri && !left) {
                /* expect RSTn: realign and reset predictors */
                r.nbits = 0;
                if (r.marker || (r.pos + 1 < n && b[r.pos] == 0xFF)) {
                    if (r.pos + 1 < n && b[r.pos + 1] >= 0xD0 && b[r.pos + 1] <= 0xD7) r.pos += 2;
                    else return IMGDEC_E_FORMAT;
                    r.marker = 0;
                } else return IMGDEC_E_FORMAT;
                for (k = 0; k < g_j.nc; k++) g_j.c[k].pred = 0;
                left = g_j.dri;
            }
            for (k = 0; k < g_j.nc; k++) {
                int by, bx;
                for (by = 0; by < g_j.c[k].v; by++)
                    for (bx = 0; bx < g_j.c[k].h; bx++) {
                        rc = jpeg_block(&r, &g_j.c[k], g_j.blk[bi++]);
                        if (rc) return rc;
                    }
            }
            if (g_j.dri) left--;
            for (py = 0; py < mcuh; py++) {
                int y = my * mcuh + py;
                if (y >= img->h) break;
                for (px = 0; px < mcuw; px++) {
                    int x = mx * mcuw + px, s[3], base = 0;
                    if (x >= img->w) break;
                    for (k = 0; k < g_j.nc; k++) {
                        int sx = px * g_j.c[k].h / g_j.hmax, sy = py * g_j.c[k].v / g_j.vmax;
                        s[k] = g_j.blk[base + (sy >> 3) * g_j.c[k].h + (sx >> 3)][(sy & 7) * 8 + (sx & 7)];
                        base += g_j.c[k].h * g_j.c[k].v;
                    }
                    if (g_j.nc == 1) scratch[(unsigned)y * (unsigned)img->w + (unsigned)x] = (unsigned char)s[0];
                    else {
                        unsigned char *o = scratch + ((unsigned)y * (unsigned)img->w + (unsigned)x) * 3U;
                        int cb = s[1] - 128, cr = s[2] - 128;
                        o[0] = jclamp(s[0] + ((1436 * cr + 512) >> 10));
                        o[1] = jclamp(s[0] - ((352 * cb + 731 * cr - 512) >> 10));
                        o[2] = jclamp(s[0] + ((1815 * cb + 512) >> 10));
                    }
                }
            }
        }
    img->data = scratch;
    img->stride = img->w * img->channels;
    return IMGDEC_OK;
}
