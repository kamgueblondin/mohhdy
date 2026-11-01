/* imgdec_jpeg.inc - JPEG decoder, included by imgdec.c only. Freestanding,
 * no libc, no allocation, bounded loops.
 * Supported: SOF0/SOF1 (8-bit Huffman sequential) and, since Lot 49, SOF2
 * progressive Huffman (spectral selection and successive approximation).
 * 1 or 3 components, sampling factors 1..2, DRI restart markers, 8 and
 * 16-bit DQT. Refused with IMGDEC_E_UNSUP: lossless, arithmetic, 12-bit,
 * CMYK. Output: 8-bit gray or RGB in the caller scratch. Progressive also
 * stores one int16 coefficient block per MCU block after the pixels
 * (imgdec_scratch_need). Integer IDCT (separable, Q12 cosine table),
 * nearest chroma upsampling. Quant tables stay in zigzag order. */

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
    int prog, eoi, ss, se, ah, al, scan_n, nmx, nmy, bpm;
    int scan_i[4], scan_td[4], scan_ta[4];
    short *coef;
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

/* parse markers from `start`. reset clears tables (a new file). decode = 0
 * stops at SOF for probe. Progressive resumes here after each scan. */
static int jpeg_headers(const unsigned char *b, int n, imgdec_t *img, int decode, int start, int reset) {
    int pos = start, guard = 0;
    unsigned i;
    if (reset) {
        for (i = 0; i < 4; i++) { g_j.qvalid[i] = 0; g_j.dc[i].valid = 0; g_j.ac[i].valid = 0; }
        g_j.dri = 0; g_j.sof_seen = 0; g_j.sos = 0; g_j.prog = 0; g_j.eoi = 0; g_j.scan_n = 0;
    } else {
        g_j.sos = 0; g_j.eoi = 0;
    }
    while (pos + 1 < n && guard++ < 4096) {
        int m, len, p;
        if (b[pos] != 0xFF) return IMGDEC_E_FORMAT;
        m = b[pos + 1];
        if (m == 0xFF) { pos++; continue; }
        if (m == 0xD9) {
            if (g_j.prog && g_j.sof_seen) { g_j.eoi = 1; g_j.sos = 0; return IMGDEC_OK; }
            break;
        }
        if (m >= 0xD0 && m <= 0xD7) { pos += 2; continue; }   /* restart between scans */
        len = jseg_len(b, n, pos + 2);
        if (len < 0) return IMGDEC_E_TRUNC;
        p = pos + 4;
        if (m == 0xC0 || m == 0xC1 || m == 0xC2) {
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
            g_j.prog = (m == 0xC2);
            img->jpeg_prog = g_j.prog;
            img->jpeg_hmax = g_j.hmax;
            img->jpeg_vmax = g_j.vmax;
            img->jpeg_mcu = 0;
            for (k = 0; k < g_j.nc; k++) img->jpeg_mcu += g_j.c[k].h * g_j.c[k].v;
            g_j.sof_seen = 1;
            if (!decode) return IMGDEC_OK;
        } else if ((m >= 0xC2 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC)) {
            int k;
            img->fmt = IMGDEC_JPEG;          /* lossless / arithmetic / other SOF */
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
            if (ns < 1 || len < 6 + 2 * ns) return IMGDEC_E_FORMAT;
            if (!g_j.prog && (ns != g_j.nc)) return IMGDEC_E_UNSUP;   /* one interleaved scan only */
            if (g_j.prog && ns > 4) return IMGDEC_E_UNSUP;
            g_j.scan_n = ns;
            for (k = 0; k < ns; k++) {
                int id = b[p + 1 + 2 * k], t = b[p + 2 + 2 * k], j;
                for (j = 0; j < g_j.nc; j++) if (g_j.c[j].id == id) break;
                if (j == g_j.nc || (!g_j.prog && j != k)) return IMGDEC_E_FORMAT;
                g_j.scan_i[k] = j;
                g_j.scan_td[k] = t >> 4; g_j.scan_ta[k] = t & 15;
                if (g_j.scan_td[k] > 3 || g_j.scan_ta[k] > 3) return IMGDEC_E_FORMAT;
                g_j.c[j].td = g_j.scan_td[k]; g_j.c[j].ta = g_j.scan_ta[k];
            }
            if (g_j.prog) {
                int tail = p + 1 + 2 * ns;
                g_j.ss = b[tail]; g_j.se = b[tail + 1];
                g_j.ah = b[tail + 2] >> 4; g_j.al = b[tail + 2] & 15;
                if (g_j.ss > g_j.se || g_j.se > 63 || g_j.ah > 13 || g_j.al > 13) return IMGDEC_E_FORMAT;
                if (g_j.ss == 0 && g_j.se != 0) return IMGDEC_E_UNSUP;
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
    return jpeg_headers(b, n, img, 0, 2, 1);
}

static int jscaled(int full, int shift) { return (full + (1 << shift) - 1) >> shift; }
static unsigned jpeg_scratch_need(const imgdec_t *img, int shift) {
    unsigned pix = (unsigned)jscaled(img->w, shift) * (unsigned)jscaled(img->h, shift) * (unsigned)img->channels;
    unsigned mcuw, mcuh, nmx, nmy, blocks, off, coef;
    if (!img->jpeg_prog || img->jpeg_hmax < 1 || img->jpeg_vmax < 1 || img->jpeg_mcu < 1) return pix;
    mcuw = 8U * (unsigned)img->jpeg_hmax;
    mcuh = 8U * (unsigned)img->jpeg_vmax;
    nmx = ((unsigned)img->w + mcuw - 1U) / mcuw;
    nmy = ((unsigned)img->h + mcuh - 1U) / mcuh;
    if (nmx > 4096U || nmy > 4096U) return 0xFFFFFFFFU;
    blocks = nmx * nmy * (unsigned)img->jpeg_mcu;
    coef = blocks * 128U;
    off = (pix + 1U) & ~1U;
    if (blocks / nmx / nmy != (unsigned)img->jpeg_mcu || off + coef < off) return 0xFFFFFFFFU;
    return off + coef;
}

static unsigned char jclamp(int v) { return (unsigned char)(v < 0 ? 0 : v > 255 ? 255 : v); }

/* Lot 72: scaled decode. The pixels of a JPEG decoded at 1/2^shift
 * (imgdec_open_scaled) are point samples of the full image; only the
 * output (img->w x img->h) is stored, so a 4096x4096 photo needs a
 * 512x512 buffer at shift 3. g_jfull_* are the full dimensions. */
static int g_jfull_w, g_jfull_h, g_jshift, g_jmask;
static void jpeg_put(unsigned char *scratch, const imgdec_t *img, int x, int y, const int *s) {
    unsigned at = (unsigned)(y >> g_jshift) * (unsigned)img->w + (unsigned)(x >> g_jshift);
    if (g_j.nc == 1) scratch[at] = (unsigned char)s[0];
    else {
        unsigned char *o = scratch + at * 3U;
        int cb = s[1] - 128, cr = s[2] - 128;
        o[0] = jclamp(s[0] + ((1436 * cr + 512) >> 10));
        o[1] = jclamp(s[0] - ((352 * cb + 731 * cr - 512) >> 10));
        o[2] = jclamp(s[0] + ((1815 * cb + 512) >> 10));
    }
}

static int sat16(int v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : v; }

/* q[] is zigzag order, qz[] is natural order (the progressive coefficient buffer). */
static void jpeg_idct_q(const short *qz, const unsigned short *q, short *out) {
    int coef[64], tmp[64], i, x, y, u, v;
    for (i = 0; i < 64; i++) {
        int nat = k_zz[i], c = (int)qz[nat] * (int)q[i];
        if (c > 2047) c = 2047;
        if (c < -2048) c = -2048;
        coef[nat] = c;
    }
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++) {
            int acc = 0;
            for (u = 0; u < 8; u++) acc += k_idct[x][u] * coef[y * 8 + u];
            tmp[y * 8 + x] = acc >> 10;
        }
    for (x = 0; x < 8; x++)
        for (y = 0; y < 8; y++) {
            int acc = 0;
            for (u = 0; u < 8; u++) acc += k_idct[y][u] * tmp[u * 8 + x];
            v = ((acc + (1 << 13)) >> 14) + 128;
            out[y * 8 + x] = (short)(v < 0 ? 0 : v > 255 ? 255 : v);
        }
}

static short *prog_coef(int comp, int bx, int by) {
    int h = g_j.c[comp].h, v = g_j.c[comp].v, mx = bx / h, my = by / v;
    int bi = 0, k;
    for (k = 0; k < comp; k++) bi += g_j.c[k].h * g_j.c[k].v;
    bi += (by % v) * h + (bx % h);
    return g_j.coef + ((my * g_j.nmx + mx) * g_j.bpm + bi) * 64;
}

static void coef_add_bit(short *c, int al, int bit) {
    int v = *c;
    if (bit) v += v < 0 ? -(1 << al) : (1 << al);
    *c = (short)sat16(v);
}

/* One progressive block. eobrun spans the blocks of this scan. */
static int prog_block(jbits_t *r, int comp, int td, int ta, int bx, int by, int *eobrun) {
    short *blk = prog_coef(comp, bx, by);
    int al = g_j.al, ss = g_j.ss, se = g_j.se, k;
    if (ss == 0) {
        if (g_j.ah == 0) {
            int s = jdecode(r, &g_j.dc[td]);
            if (s < 0 || s > 11) return IMGDEC_E_FORMAT;
            g_j.c[comp].pred += jextend(jbits(r, s), s);
            blk[0] = (short)sat16(g_j.c[comp].pred * (1 << al));
        } else {
            blk[0] = (short)((int)blk[0] | (jbit(r) << al));
        }
        return IMGDEC_OK;
    }
    if (g_j.ah == 0) {
        if (*eobrun) { (*eobrun)--; return IMGDEC_OK; }
        k = ss;
        while (k <= se) {
            int rs = jdecode(r, &g_j.ac[ta]), run, sz;
            if (rs < 0) return IMGDEC_E_FORMAT;
            run = rs >> 4; sz = rs & 15;
            if (sz) {
                k += run;
                if (k > se || k > 63) return IMGDEC_E_FORMAT;
                blk[k_zz[k]] = (short)sat16(jextend(jbits(r, sz), sz) * (1 << al));
                k++;
            } else if (run == 15) {
                k += 16;
            } else {
                *eobrun = (1 << run) - 1;
                if (run) *eobrun += jbits(r, run);
                break;
            }
        }
        return IMGDEC_OK;
    }
    k = ss;
    if (*eobrun == 0) {
        while (k <= se) {
            int rs = jdecode(r, &g_j.ac[ta]), run, sz, newv = 0;
            if (rs < 0) return IMGDEC_E_FORMAT;
            run = rs >> 4; sz = rs & 15;
            if (sz) {
                if (sz != 1) return IMGDEC_E_FORMAT;
                newv = jbit(r) ? (1 << al) : -(1 << al);
            } else if (run != 15) {
                *eobrun = 1 << run;
                if (run) *eobrun += jbits(r, run);
                break;
            }
            while (1) {
                if (k > se) { if (newv) return IMGDEC_E_FORMAT; break; }
                if (blk[k_zz[k]]) coef_add_bit(&blk[k_zz[k]], al, jbit(r));
                else { run--; if (run < 0) break; }
                k++;
            }
            if (newv) {
                if (k > se || k > 63) return IMGDEC_E_FORMAT;
                blk[k_zz[k]] = (short)newv;
            }
            k++;
        }
    }
    if (*eobrun > 0) {
        while (k <= se) {
            if (k <= 63 && blk[k_zz[k]]) coef_add_bit(&blk[k_zz[k]], al, jbit(r));
            k++;
        }
        (*eobrun)--;
    }
    return IMGDEC_OK;
}

static int prog_restart(jbits_t *r, const unsigned char *b, int n) {
    int k;
    r->nbits = 0;
    if (r->marker || (r->pos + 1 < n && b[r->pos] == 0xFF)) {
        if (r->pos + 1 < n && b[r->pos + 1] >= 0xD0 && b[r->pos + 1] <= 0xD7) {
            r->pos += 2; r->marker = 0;
            for (k = 0; k < g_j.nc; k++) g_j.c[k].pred = 0;
            return IMGDEC_OK;
        }
    }
    return IMGDEC_E_FORMAT;
}

static int prog_scan(jbits_t *r, const unsigned char *b, int n) {
    int eobrun = 0, left, units, u, s, k, interleaved = g_j.scan_n > 1;
    for (k = 0; k < g_j.nc; k++) g_j.c[k].pred = 0;
    for (s = 0; s < g_j.scan_n; s++) {
        if (g_j.ss == 0) { if (!g_j.dc[g_j.scan_td[s]].valid) return IMGDEC_E_FORMAT; }
        else if (!g_j.ac[g_j.scan_ta[s]].valid) return IMGDEC_E_FORMAT;
    }
    if (interleaved) units = g_j.nmx * g_j.nmy;
    else {
        int comp = g_j.scan_i[0];
        units = (g_j.nmx * g_j.c[comp].h) * (g_j.nmy * g_j.c[comp].v);
    }
    left = g_j.dri;
    for (u = 0; u < units; u++) {
        if (g_j.dri && !left) {
            int rc = prog_restart(r, b, n);
            if (rc) return rc;
            eobrun = 0;
            left = g_j.dri;
        }
        if (interleaved) {
            int mx = u % g_j.nmx, my = u / g_j.nmx;
            for (s = 0; s < g_j.scan_n; s++) {
                int comp = g_j.scan_i[s], by, bx, rc;
                for (by = 0; by < g_j.c[comp].v; by++)
                    for (bx = 0; bx < g_j.c[comp].h; bx++) {
                        rc = prog_block(r, comp, g_j.scan_td[s], g_j.scan_ta[s],
                                        mx * g_j.c[comp].h + bx, my * g_j.c[comp].v + by, &eobrun);
                        if (rc) return rc;
                    }
            }
        } else {
            int comp = g_j.scan_i[0], nbx = g_j.nmx * g_j.c[comp].h, rc;
            rc = prog_block(r, comp, g_j.scan_td[0], g_j.scan_ta[0], u % nbx, u / nbx, &eobrun);
            if (rc) return rc;
        }
        if (g_j.dri) left--;
    }
    return IMGDEC_OK;
}

static int jpeg_resync(jbits_t *r) {
    int guard = 0;
    r->nbits = 0; r->marker = 0;
    while (r->pos < r->n && r->b[r->pos] != 0xFF) {
        if (++guard > 65536) return -1;
        r->pos++;
    }
    return r->pos < r->n ? 0 : -1;
}

static void jpeg_paint(unsigned char *scratch, const imgdec_t *img) {
    int mx, my, mcuw = 8 * g_j.hmax, mcuh = 8 * g_j.vmax;
    for (my = 0; my < g_j.nmy; my++)
        for (mx = 0; mx < g_j.nmx; mx++) {
            int bi = 0, k, px, py;
            for (k = 0; k < g_j.nc; k++) {
                int by, bx;
                for (by = 0; by < g_j.c[k].v; by++)
                    for (bx = 0; bx < g_j.c[k].h; bx++)
                        jpeg_idct_q(prog_coef(k, mx * g_j.c[k].h + bx, my * g_j.c[k].v + by),
                                    g_j.q[g_j.c[k].tq], g_j.blk[bi++]);
            }
            for (py = 0; py < mcuh; py++) {
                int y = my * mcuh + py;
                if (y >= g_jfull_h) break;
                for (px = 0; px < mcuw; px++) {
                    int x = mx * mcuw + px, s[3], base = 0;
                    if (x >= g_jfull_w) break;
                    if ((x | y) & g_jmask) continue;
                    for (k = 0; k < g_j.nc; k++) {
                        int sx = px * g_j.c[k].h / g_j.hmax, sy = py * g_j.c[k].v / g_j.vmax;
                        s[k] = g_j.blk[base + (sy >> 3) * g_j.c[k].h + (sx >> 3)][(sy & 7) * 8 + (sx & 7)];
                        base += g_j.c[k].h * g_j.c[k].v;
                    }
                    jpeg_put(scratch, img, x, y, s);
                }
            }
        }
}

static void jpeg_set_scaled(imgdec_t *img) {
    img->w = jscaled(g_jfull_w, g_jshift);
    img->h = jscaled(g_jfull_h, g_jshift);
}

static int open_jpeg_prog(const unsigned char *b, int n, unsigned char *scratch, imgdec_t *img) {
    jbits_t r;
    unsigned pix = (unsigned)jscaled(g_jfull_w, g_jshift) * (unsigned)jscaled(g_jfull_h, g_jshift) * (unsigned)img->channels;
    unsigned off = (pix + 1U) & ~1U, blocks, i;
    int guard = 0, rc, k;
    int mcuw = 8 * g_j.hmax, mcuh = 8 * g_j.vmax;
    g_j.nmx = (g_jfull_w + mcuw - 1) / mcuw;
    g_j.nmy = (g_jfull_h + mcuh - 1) / mcuh;
    g_j.bpm = img->jpeg_mcu;
    blocks = (unsigned)g_j.nmx * (unsigned)g_j.nmy * (unsigned)g_j.bpm;
    g_j.coef = (short *)(void *)(scratch + off);
    for (i = 0; i < blocks * 64U; i++) g_j.coef[i] = 0;
    for (k = 0; k < g_j.nc; k++) if (!g_j.qvalid[g_j.c[k].tq]) return IMGDEC_E_FORMAT;
    if (!g_j.sos) return IMGDEC_E_TRUNC;
    r.b = b; r.n = n; r.pos = g_j.sos; r.bits = 0; r.nbits = 0; r.marker = 0;
    while (guard++ < 64) {
        rc = prog_scan(&r, b, n);
        if (rc) return rc;
        if (jpeg_resync(&r)) return IMGDEC_E_TRUNC;
        rc = jpeg_headers(b, n, img, 1, r.pos, 0);
        if (rc) return rc;
        if (g_j.eoi) break;
        if (!g_j.sos) return IMGDEC_E_TRUNC;
        r.pos = g_j.sos; r.bits = 0; r.nbits = 0; r.marker = 0;
    }
    if (!g_j.eoi) return IMGDEC_E_TRUNC;
    jpeg_set_scaled(img);
    jpeg_paint(scratch, img);
    img->data = scratch;
    img->stride = img->w * img->channels;
    return IMGDEC_OK;
}

static int open_jpeg(const unsigned char *b, int n, unsigned char *scratch, unsigned smax, imgdec_t *img, int shift) {
    jbits_t r;
    int rc, mx, my, mcuw, mcuh, nmx, nmy, k, left;
    unsigned need;
    if (shift < 0) shift = 0;
    if (shift > 3) shift = 3;
    need = jpeg_scratch_need(img, shift);
    if (!scratch || need > smax || need == 0xFFFFFFFFU) return IMGDEC_E_TOOBIG;
    rc = jpeg_headers(b, n, img, 1, 2, 1);
    if (rc) return rc;
    g_jfull_w = img->w; g_jfull_h = img->h; g_jshift = shift; g_jmask = (1 << shift) - 1;
    img->full_w = img->w; img->full_h = img->h; img->scale_shift = shift;
    if (g_j.prog) return open_jpeg_prog(b, n, scratch, img);
    jpeg_set_scaled(img);
    if (!g_j.sos) return IMGDEC_E_TRUNC;
    for (k = 0; k < g_j.nc; k++) {
        if (!g_j.qvalid[g_j.c[k].tq] || !g_j.dc[g_j.c[k].td].valid || !g_j.ac[g_j.c[k].ta].valid) return IMGDEC_E_FORMAT;
        g_j.c[k].pred = 0;
    }
    r.b = b; r.n = n; r.pos = g_j.sos; r.bits = 0; r.nbits = 0; r.marker = 0;
    mcuw = 8 * g_j.hmax; mcuh = 8 * g_j.vmax;
    nmx = (g_jfull_w + mcuw - 1) / mcuw; nmy = (g_jfull_h + mcuh - 1) / mcuh;
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
                if (y >= g_jfull_h) break;
                for (px = 0; px < mcuw; px++) {
                    int x = mx * mcuw + px, s[3], base = 0;
                    if (x >= g_jfull_w) break;
                    if ((x | y) & g_jmask) continue;
                    for (k = 0; k < g_j.nc; k++) {
                        int sx = px * g_j.c[k].h / g_j.hmax, sy = py * g_j.c[k].v / g_j.vmax;
                        s[k] = g_j.blk[base + (sy >> 3) * g_j.c[k].h + (sx >> 3)][(sy & 7) * 8 + (sx & 7)];
                        base += g_j.c[k].h * g_j.c[k].v;
                    }
                    jpeg_put(scratch, img, x, y, s);
                }
            }
        }
    img->data = scratch;
    img->stride = img->w * img->channels;
    return IMGDEC_OK;
}
