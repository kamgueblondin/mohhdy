/* imgdec_webp.inc - Lot 73: WebP lossless (VP8L, RFC 9649), included by
 * imgdec.c only. Freestanding, no allocation, bounded by the scratch.
 * RIFF "WEBP" with a VP8L chunk, alone or inside VP8X (still images;
 * an animated VP8X and lossy VP8 data are refused with IMGDEC_E_UNSUP).
 * The four transforms, the color cache, meta prefix codes and LZ77
 * backward references are decoded; the result is RGBA in the scratch.
 * Prefix codes are canonical and decoded bit by bit, like inflate. */

#define WEBP_POOL (256U * 1024U)
#define WEBP_GREEN_MAX (256 + 24 + 2048)
#define WEBP_GROUPS_MAX 256

typedef struct {
    unsigned short count[16];
    unsigned short *sym;       /* symbols by code, in the pool */
    int single;                /* one symbol: no bits read; -1 none */
} webp_code_t;

typedef struct {
    rd_t *r;
    unsigned char *pool;
    unsigned used, max;
    int err;
} webp_t;

static void *webp_alloc(webp_t *s, unsigned n) {
    void *p;
    n = (n + 7U) & ~7U;          /* pointer-aligned pieces (the pool is) */
    if (n > s->max - s->used) { s->err = IMGDEC_E_TOOBIG; return 0; }
    p = s->pool + s->used;
    s->used += n;
    return p;
}

static unsigned webp_bits(webp_t *s, int n) {
    unsigned v = rd_bits(s->r, n);
    if (s->r->eof && !s->err) s->err = IMGDEC_E_TRUNC;
    return v;
}

/* canonical code from lengths; a complete tree, or one symbol */
static int webp_build(webp_t *s, webp_code_t *c, const unsigned char *len, int n) {
    unsigned short offs[16];
    int i, left = 1, used = 0, last = -1;
    for (i = 0; i < 16; i++) c->count[i] = 0;
    for (i = 0; i < n; i++) if (len[i]) { c->count[len[i]]++; used++; last = i; }
    c->single = -1;
    c->sym = 0;
    if (used == 0) { c->single = 0; return 0; }     /* empty code: reads symbol 0 */
    if (used == 1) { c->single = last; return 0; }
    for (i = 1; i < 16; i++) { left <<= 1; left -= c->count[i]; if (left < 0) return IMGDEC_E_FORMAT; }
    if (left) return IMGDEC_E_FORMAT;                 /* incomplete */
    c->sym = (unsigned short *)webp_alloc(s, (unsigned)used * 2U);
    if (!c->sym) return s->err;
    offs[1] = 0;
    for (i = 1; i < 15; i++) offs[i + 1] = (unsigned short)(offs[i] + c->count[i]);
    for (i = 0; i < n; i++) if (len[i]) c->sym[offs[len[i]]++] = (unsigned short)i;
    return 0;
}

static int webp_sym(webp_t *s, const webp_code_t *c) {
    int code = 0, first = 0, index = 0, l;
    if (c->single >= 0) return c->single;
    for (l = 1; l < 16; l++) {
        int count;
        code |= (int)webp_bits(s, 1);
        count = c->count[l];
        if (code - count < first) return c->sym[index + (code - first)];
        index += count; first += count; first <<= 1; code <<= 1;
        if (s->err) return -1;
    }
    s->err = IMGDEC_E_FORMAT;
    return -1;
}

static const unsigned char k_webp_cl_order[19] = { 17, 18, 0, 1, 2, 3, 4, 5, 16, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

static int webp_read_code(webp_t *s, webp_code_t *c, int alphabet) {
    static unsigned char len[WEBP_GREEN_MAX];
    int i;
    for (i = 0; i < alphabet; i++) len[i] = 0;
    if (webp_bits(s, 1)) {
        /* simple: one or two symbols of length 1 */
        int two = (int)webp_bits(s, 1), first8 = (int)webp_bits(s, 1);
        int s0 = (int)webp_bits(s, first8 ? 8 : 1);
        if (s0 >= alphabet) return IMGDEC_E_FORMAT;
        len[s0] = 1;
        if (two) {
            int s1 = (int)webp_bits(s, 8);
            if (s1 >= alphabet) return IMGDEC_E_FORMAT;
            len[s1] = 1;
        }
    } else {
        unsigned char cl[19];
        webp_code_t clc;
        unsigned save = s->used;
        int ncl = 4 + (int)webp_bits(s, 4), max_sym, sym = 0, prev = 8, rc;
        for (i = 0; i < 19; i++) cl[i] = 0;
        for (i = 0; i < ncl; i++) cl[k_webp_cl_order[i]] = (unsigned char)webp_bits(s, 3);
        rc = webp_build(s, &clc, cl, 19);
        if (rc) return rc;
        if (webp_bits(s, 1)) {
            int nb = 2 + 2 * (int)webp_bits(s, 3);
            max_sym = 2 + (int)webp_bits(s, nb);
            if (max_sym > alphabet) return IMGDEC_E_FORMAT;
        } else max_sym = alphabet;
        while (sym < alphabet) {
            int v, rep, val;
            if (max_sym-- == 0) break;
            v = webp_sym(s, &clc);
            if (s->err) return s->err;
            if (v < 16) { len[sym++] = (unsigned char)v; if (v) prev = v; continue; }
            if (v == 16) { rep = 3 + (int)webp_bits(s, 2); val = prev; }
            else if (v == 17) { rep = 3 + (int)webp_bits(s, 3); val = 0; }
            else { rep = 11 + (int)webp_bits(s, 7); val = 0; }
            if (sym + rep > alphabet) return IMGDEC_E_FORMAT;
            while (rep--) len[sym++] = (unsigned char)val;
        }
        s->used = save;               /* the code-length code is temporary */
    }
    if (s->err) return s->err;
    return webp_build(s, c, len, alphabet);
}

static int webp_prefix_value(webp_t *s, int prefix) {
    int extra;
    if (prefix < 4) return prefix + 1;
    extra = (prefix - 2) >> 1;
    return ((2 + (prefix & 1)) << extra) + (int)webp_bits(s, extra) + 1;
}

static const signed char k_webp_dist[120][2] = {
    {0,1},{1,0},{1,1},{-1,1},{0,2},{2,0},{1,2},{-1,2},{2,1},{-2,1},{2,2},{-2,2},{0,3},{3,0},{1,3},{-1,3},{3,1},{-3,1},{2,3},{-2,3},
    {3,2},{-3,2},{0,4},{4,0},{1,4},{-1,4},{4,1},{-4,1},{3,3},{-3,3},{2,4},{-2,4},{4,2},{-4,2},{0,5},{3,4},{-3,4},{4,3},{-4,3},{5,0},
    {1,5},{-1,5},{5,1},{-5,1},{2,5},{-2,5},{5,2},{-5,2},{4,4},{-4,4},{3,5},{-3,5},{5,3},{-5,3},{0,6},{6,0},{1,6},{-1,6},{6,1},{-6,1},
    {2,6},{-2,6},{6,2},{-6,2},{4,5},{-4,5},{5,4},{-5,4},{3,6},{-3,6},{6,3},{-6,3},{0,7},{7,0},{1,7},{-1,7},{5,5},{-5,5},{7,1},{-7,1},
    {4,6},{-4,6},{6,4},{-6,4},{2,7},{-2,7},{7,2},{-7,2},{3,7},{-3,7},{7,3},{-7,3},{5,6},{-5,6},{6,5},{-6,5},{8,0},{4,7},{-4,7},{7,4},
    {-7,4},{8,1},{8,2},{6,6},{-6,6},{8,3},{5,7},{-5,7},{7,5},{-7,5},{8,4},{6,7},{-6,7},{7,6},{-7,6},{8,5},{7,7},{-7,7},{8,6},{8,7}
};

static int webp_image(webp_t *s, unsigned *out, int w, int h, int top);

static unsigned webp_div(int v, int bits) { return (unsigned)((v + (1 << bits) - 1) >> bits); }

/* a sub-image (transform data, entropy image) of w x h in the pool */
static unsigned *webp_sub(webp_t *s, int w, int h) {
    unsigned *p = (unsigned *)webp_alloc(s, (unsigned)w * (unsigned)h * 4U);
    if (!p) return 0;
    if (webp_image(s, p, w, h, 0)) return 0;
    return p;
}

/* an entropy-coded image; top: the ARGB image (meta prefix codes) */
static int webp_image(webp_t *s, unsigned *out, int w, int h, int top) {
    int cache_bits = 0, prefix_bits = 0, ngroups = 1, g, k, pw = 0;
    unsigned *cache = 0, *meta = 0;
    webp_code_t *codes;
    unsigned total = (unsigned)w * (unsigned)h, pos;
    if (webp_bits(s, 1)) {
        cache_bits = (int)webp_bits(s, 4);
        if (cache_bits < 1 || cache_bits > 11) return s->err = IMGDEC_E_FORMAT;
        cache = (unsigned *)webp_alloc(s, 4U << cache_bits);
        if (!cache) return s->err;
        for (k = 0; k < (1 << cache_bits); k++) cache[k] = 0;
    }
    if (top && webp_bits(s, 1)) {
        unsigned i, n;
        prefix_bits = (int)webp_bits(s, 3) + 2;
        pw = (int)webp_div(w, prefix_bits);
        meta = webp_sub(s, pw, (int)webp_div(h, prefix_bits));
        if (!meta) return s->err ? s->err : IMGDEC_E_FORMAT;
        n = (unsigned)pw * webp_div(h, prefix_bits);
        for (i = 0; i < n; i++) {
            int m = (int)((meta[i] >> 8) & 0xffffU);
            if (m + 1 > ngroups) ngroups = m + 1;
        }
        if (ngroups > WEBP_GROUPS_MAX) return s->err = IMGDEC_E_TOOBIG;
    }
    if (s->err) return s->err;
    codes = (webp_code_t *)webp_alloc(s, (unsigned)ngroups * 5U * (unsigned)sizeof(webp_code_t));
    if (!codes) return s->err;
    for (g = 0; g < ngroups; g++) {
        static const int alpha[5] = { 0, 256, 256, 256, 40 };
        for (k = 0; k < 5; k++) {
            int rc = webp_read_code(s, &codes[g * 5 + k], k ? alpha[k] : 256 + 24 + (cache_bits ? 1 << cache_bits : 0));
            if (rc) return s->err = rc;
        }
    }
    pos = 0;
    while (pos < total) {
        int x = (int)(pos % (unsigned)w), y = (int)(pos / (unsigned)w), sym;
        const webp_code_t *c = codes;
        unsigned start = pos;
        if (meta) c = codes + 5 * (int)((meta[(unsigned)(y >> prefix_bits) * (unsigned)pw + (unsigned)(x >> prefix_bits)] >> 8) & 0xffffU);
        sym = webp_sym(s, &c[0]);
        if (s->err) return s->err;
        if (sym < 256) {
            unsigned r = (unsigned)webp_sym(s, &c[1]), b = (unsigned)webp_sym(s, &c[2]), a = (unsigned)webp_sym(s, &c[3]);
            if (s->err) return s->err;
            out[pos++] = (a << 24) | (r << 16) | ((unsigned)sym << 8) | b;
        } else if (sym < 256 + 24) {
            int len = webp_prefix_value(s, sym - 256), dsym = webp_sym(s, &c[4]), dist;
            if (s->err) return s->err;
            dist = webp_prefix_value(s, dsym);
            if (dist > 120) dist -= 120;
            else {
                dist = k_webp_dist[dist - 1][0] + k_webp_dist[dist - 1][1] * w;
                if (dist < 1) dist = 1;
            }
            if ((unsigned)dist > pos || (unsigned)len > total - pos) return s->err = IMGDEC_E_FORMAT;
            while (len--) { out[pos] = out[pos - (unsigned)dist]; pos++; }
        } else {
            int idx = sym - 280;
            if (!cache || idx >= (1 << cache_bits)) return s->err = IMGDEC_E_FORMAT;
            out[pos++] = cache[idx];
        }
        if (cache)
            for (; start < pos; start++) cache[(0x1e35a7bdU * out[start]) >> (32 - cache_bits)] = out[start];
    }
    return 0;
}

static unsigned webp_avg(unsigned a, unsigned b) {
    return (((a ^ b) & 0xfefefefeU) >> 1) + (a & b);
}
static int webp_abs(int v) { return v < 0 ? -v : v; }
static unsigned webp_select(unsigned l, unsigned t, unsigned tl) {
    int pl = 0, pt = 0, sh;
    for (sh = 0; sh < 32; sh += 8) {
        int a = (int)((l >> sh) & 255), b = (int)((t >> sh) & 255), c = (int)((tl >> sh) & 255);
        pl += webp_abs(b - c);      /* |p - L| with p = L + T - TL */
        pt += webp_abs(a - c);
    }
    return pl < pt ? l : t;
}
static int webp_clamp(int v) { return v < 0 ? 0 : v > 255 ? 255 : v; }
static unsigned webp_full(unsigned a, unsigned b, unsigned c) {
    unsigned r = 0;
    int sh;
    for (sh = 0; sh < 32; sh += 8)
        r |= (unsigned)webp_clamp((int)((a >> sh) & 255) + (int)((b >> sh) & 255) - (int)((c >> sh) & 255)) << sh;
    return r;
}
static unsigned webp_half(unsigned a, unsigned b) {
    unsigned r = 0;
    int sh;
    for (sh = 0; sh < 32; sh += 8) {
        int x = (int)((a >> sh) & 255), y = (int)((b >> sh) & 255);
        r |= (unsigned)webp_clamp(x + (x - y) / 2) << sh;
    }
    return r;
}
static unsigned webp_add(unsigned a, unsigned b) {
    return (((a & 0xff00ff00U) + (b & 0xff00ff00U)) & 0xff00ff00U) | (((a & 0x00ff00ffU) + (b & 0x00ff00ffU)) & 0x00ff00ffU);
}

static unsigned webp_predict(int mode, unsigned l, unsigned t, unsigned tr, unsigned tl) {
    switch (mode) {
    case 0: return 0xff000000U;
    case 1: return l;
    case 2: return t;
    case 3: return tr;
    case 4: return tl;
    case 5: return webp_avg(webp_avg(l, tr), t);
    case 6: return webp_avg(l, tl);
    case 7: return webp_avg(l, t);
    case 8: return webp_avg(tl, t);
    case 9: return webp_avg(t, tr);
    case 10: return webp_avg(webp_avg(l, tl), webp_avg(t, tr));
    case 11: return webp_select(l, t, tl);
    case 12: return webp_full(l, t, tl);
    case 13: return webp_half(webp_avg(l, t), tl);
    default: return 0xff000000U;    /* 14, 15: as mode 0 (libwebp) */
    }
}

static void webp_unpredict(unsigned *p, int w, int h, const unsigned *data, int bits) {
    int x, y, bw = (int)webp_div(w, bits);
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            unsigned *q = p + (unsigned)y * (unsigned)w + (unsigned)x, pred;
            if (!y) pred = x ? q[-1] : 0xff000000U;
            else if (!x) pred = q[-w];
            else pred = webp_predict((int)((data[(y >> bits) * bw + (x >> bits)] >> 8) & 15), q[-1], q[-w], q[-w + 1], q[-w - 1]);
            *q = webp_add(*q, pred);
        }
}

static int webp_delta(unsigned t, unsigned c) { return ((int)(signed char)t * (int)(signed char)c) >> 5; }

static void webp_uncolor(unsigned *p, int w, int h, const unsigned *data, int bits) {
    int x, y, bw = (int)webp_div(w, bits);
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            unsigned *q = p + (unsigned)y * (unsigned)w + (unsigned)x, e = data[(y >> bits) * bw + (x >> bits)];
            unsigned g = (*q >> 8) & 255;
            int r = (int)((*q >> 16) & 255), b = (int)(*q & 255);
            r = (r + webp_delta(e & 255, g)) & 255;
            b = (b + webp_delta((e >> 8) & 255, g)) & 255;
            b = (b + webp_delta((e >> 16) & 255, (unsigned)r)) & 255;
            *q = (*q & 0xff00ff00U) | ((unsigned)r << 16) | (unsigned)b;
        }
}

/* packed rows of pw pixels to rows of w, from the end (pw <= w) */
static void webp_unindex(unsigned *p, int w, int h, int pw, int wbits, const unsigned *table, int nt) {
    int x, y, bpp = 8 >> wbits, mask = (1 << bpp) - 1;
    for (y = h - 1; y >= 0; y--)
        for (x = w - 1; x >= 0; x--) {
            unsigned g = (p[(unsigned)y * (unsigned)pw + (unsigned)(x >> wbits)] >> 8) & 255;
            int idx = (int)(g >> ((x & ((1 << wbits) - 1)) * bpp)) & mask;
            p[(unsigned)y * (unsigned)w + (unsigned)x] = idx < nt ? table[idx] : 0U;
        }
}

/* ---- container ---------------------------------------------------------- */
static int webp_vp8l_header(const unsigned char *p, int n, imgdec_t *img) {
    unsigned v;
    if (n < 5) return IMGDEC_E_TRUNC;
    if (p[0] != 0x2f) return IMGDEC_E_FORMAT;
    v = le32(p + 1);
    img->w = (int)(v & 0x3fffU) + 1;
    img->h = (int)((v >> 14) & 0x3fffU) + 1;
    if ((v >> 29) != 0U) return IMGDEC_E_UNSUP;           /* version */
    if (img->w > IMGDEC_MAX_DIM || img->h > IMGDEC_MAX_DIM) return IMGDEC_E_TOOBIG;
    img->webp_alpha = (int)((v >> 28) & 1U);
    return IMGDEC_OK;
}

static int probe_webp(const unsigned char *b, int n, imgdec_t *img) {
    int i;
    if (n < 12 || b[0] != 'R' || b[1] != 'I' || b[2] != 'F' || b[3] != 'F' || b[8] != 'W' || b[9] != 'E' || b[10] != 'B' || b[11] != 'P')
        return IMGDEC_E_FORMAT;
    img->fmt = IMGDEC_WEBP;
    img->channels = 4;
    img->bpp = 32;
    for (i = 12; i + 8 <= n; ) {
        const unsigned char *c = b + i;
        unsigned len = le32(c + 4);
        if (len > (unsigned)(n - i - 8)) return IMGDEC_E_TRUNC;
        if (c[0] == 'V' && c[1] == 'P' && c[2] == '8' && c[3] == 'X') {
            if (len < 10) return IMGDEC_E_FORMAT;
            if (c[8] & 0x02) return IMGDEC_E_UNSUP;     /* animation */
        } else if (c[0] == 'V' && c[1] == 'P' && c[2] == '8' && c[3] == 'L') {
            img->webp_off = i + 8;
            img->webp_len = (int)len;
            return webp_vp8l_header(c + 8, (int)len, img);
        } else if (c[0] == 'V' && c[1] == 'P' && c[2] == '8' && c[3] == ' ') {
            /* lossy: the size is known, the pixels are not decoded */
            if (len >= 10) { img->w = (int)(le16(c + 14) & 0x3fffU); img->h = (int)(le16(c + 16) & 0x3fffU); }
            img->webp_lossy = 1;
            return IMGDEC_E_UNSUP;
        }
        i += 8 + (int)((len + 1U) & ~1U);
    }
    return IMGDEC_E_TRUNC;
}

static unsigned webp_scratch_need(const imgdec_t *img) {
    return 8U + (unsigned)img->w * (unsigned)img->h * 4U + WEBP_POOL +
           3U * webp_div(img->w, 2) * webp_div(img->h, 2) * 4U;
}

static int open_webp(const unsigned char *b, int n, unsigned char *scratch, unsigned smax, imgdec_t *img) {
    rd_t r;
    webp_t s;
    unsigned *px, *tdata[4];
    int tbits[4], ttype[4], tw[4], nt = 0, seen = 0, xs, k, ntable = 0;
    unsigned need = webp_scratch_need(img), npx;
    unsigned char *base;
    (void)n;
    if (!scratch || need > smax) return IMGDEC_E_TOOBIG;
    base = scratch + ((8U - ((unsigned)(unsigned long)scratch & 7U)) & 7U);
    npx = (unsigned)img->w * (unsigned)img->h;
    px = (unsigned *)base;
    r.seg[0] = b + img->webp_off + 5; r.len[0] = img->webp_len - 5;
    r.nseg = 1; r.cur = 0; r.pos = 0; r.bitbuf = 0; r.bitcnt = 0; r.eof = 0;
    s.r = &r;
    s.pool = base + ((npx * 4U + 7U) & ~7U);
    s.used = 0;
    s.max = need - 8U - ((npx * 4U + 7U) & ~7U);
    s.err = 0;
    xs = img->w;
    while (webp_bits(&s, 1)) {
        int t = (int)webp_bits(&s, 2);
        if (s.err) return s.err;
        if (seen & (1 << t) || nt >= 4) return IMGDEC_E_FORMAT;
        seen |= 1 << t;
        ttype[nt] = t; tw[nt] = xs; tdata[nt] = 0; tbits[nt] = 0;
        if (t == 0 || t == 1) {
            tbits[nt] = (int)webp_bits(&s, 3) + 2;
            tdata[nt] = webp_sub(&s, (int)webp_div(xs, tbits[nt]), (int)webp_div(img->h, tbits[nt]));
            if (!tdata[nt]) return s.err ? s.err : IMGDEC_E_FORMAT;
        } else if (t == 3) {
            ntable = (int)webp_bits(&s, 8) + 1;
            tdata[nt] = webp_sub(&s, ntable, 1);
            if (!tdata[nt]) return s.err ? s.err : IMGDEC_E_FORMAT;
            for (k = 1; k < ntable; k++) tdata[nt][k] = webp_add(tdata[nt][k], tdata[nt][k - 1]);
            tbits[nt] = ntable <= 2 ? 3 : ntable <= 4 ? 2 : ntable <= 16 ? 1 : 0;
            xs = (int)webp_div(xs, tbits[nt]);
        }
        nt++;
    }
    if (s.err) return s.err;
    if (webp_image(&s, px, xs, img->h, 1)) return s.err;
    for (k = nt - 1; k >= 0; k--) {
        if (ttype[k] == 0) webp_unpredict(px, tw[k], img->h, tdata[k], tbits[k]);
        else if (ttype[k] == 1) webp_uncolor(px, tw[k], img->h, tdata[k], tbits[k]);
        else if (ttype[k] == 2) {
            unsigned i, m = (unsigned)tw[k] * (unsigned)img->h;
            for (i = 0; i < m; i++) {
                unsigned g = (px[i] >> 8) & 255;
                px[i] = (px[i] & 0xff00ff00U) | ((((px[i] >> 16) + g) & 255) << 16) | (((px[i] & 255) + g) & 255);
            }
        } else webp_unindex(px, tw[k], img->h, (int)webp_div(tw[k], tbits[k]), tbits[k], tdata[k], ntable);
    }
    /* ARGB words to RGBA bytes, in place */
    {
        unsigned i;
        for (i = 0; i < npx; i++) {
            unsigned v = px[i];
            base[i * 4] = (unsigned char)(v >> 16); base[i * 4 + 1] = (unsigned char)(v >> 8);
            base[i * 4 + 2] = (unsigned char)v; base[i * 4 + 3] = (unsigned char)(v >> 24);
        }
    }
    img->data = base;
    img->stride = img->w * 4;
    img->channels = 4;
    return IMGDEC_OK;
}
