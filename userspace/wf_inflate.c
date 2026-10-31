/* wf_inflate.c - Lot 69: bounded DEFLATE decoder (see wf_inflate.h). */
#include "wf_inflate.h"

typedef struct {
    const uint8_t* in; int in_len, pos;
    uint32_t bitbuf; int bitcnt;
    uint8_t* out; int out_len, out_cap;
    /* write limit: absolute pointer that output must not reach while
     * input from there on is unread (in-place mode) */
    int inplace;
    int err;   /* 1 corrupt, 2 output full */
} st_t;

typedef struct { uint16_t count[16]; uint16_t sym[288]; } huff_t;

static int need(st_t* s, int n) {
    while (s->bitcnt < n) {
        if (s->pos >= s->in_len) { s->err = 3; return -1; }   /* input ended */
        s->bitbuf |= (uint32_t)s->in[s->pos++] << s->bitcnt;
        s->bitcnt += 8;
    }
    return 0;
}

static int bits(st_t* s, int n) {
    int v;
    if (n == 0) return 0;
    if (need(s, n) != 0) return -1;
    v = (int)(s->bitbuf & ((1U << n) - 1U));
    s->bitbuf >>= n;
    s->bitcnt -= n;
    return v;
}

static int build(huff_t* h, const uint8_t* len, int n) {
    uint16_t offs[16];
    int i, left = 1;
    for (i = 0; i < 16; i++) h->count[i] = 0;
    for (i = 0; i < n; i++) h->count[len[i]]++;
    h->count[0] = 0;
    for (i = 1; i < 16; i++) { left <<= 1; left -= h->count[i]; if (left < 0) return -1; }
    offs[1] = 0;
    for (i = 1; i < 15; i++) offs[i + 1] = (uint16_t)(offs[i] + h->count[i]);
    for (i = 0; i < n; i++) if (len[i]) h->sym[offs[len[i]]++] = (uint16_t)i;
    return 0;
}

static int decode(st_t* s, const huff_t* h) {
    int code = 0, first = 0, index = 0, l;
    for (l = 1; l < 16; l++) {
        int b = bits(s, 1), count;
        if (b < 0) return -1;
        code |= b;
        count = h->count[l];
        if (code - count < first) return h->sym[index + (code - first)];
        index += count;
        first += count;
        first <<= 1;
        code <<= 1;
    }
    s->err = 1;
    return -1;
}

static int room(st_t* s, int n) {
    /* in place: the output must stay behind the unread input */
    int lim = s->out_cap;
    if (s->inplace) {
        int rd = (int)((s->in + s->pos) - s->out);
        if (rd < lim) lim = rd;
    }
    if (s->out_len + n > lim) { s->err = 2; return -1; }
    return 0;
}

static const uint16_t LBASE[29] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
static const uint8_t LEXT[29] = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
static const uint16_t DBASE[30] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
static const uint8_t DEXT[30] = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int codes(st_t* s, const huff_t* lh, const huff_t* dh) {
    for (;;) {
        int sym = decode(s, lh);
        if (sym < 0) return -1;
        if (sym < 256) {
            if (room(s, 1) != 0) return -1;
            s->out[s->out_len++] = (uint8_t)sym;
        } else if (sym == 256) {
            return 0;
        } else {
            int len, dist, e, i;
            sym -= 257;
            if (sym >= 29) { s->err = 1; return -1; }
            e = bits(s, LEXT[sym]); if (e < 0) return -1;
            len = LBASE[sym] + e;
            sym = decode(s, dh);
            if (sym < 0) return -1;
            if (sym >= 30) { s->err = 1; return -1; }
            e = bits(s, DEXT[sym]); if (e < 0) return -1;
            dist = DBASE[sym] + e;
            if (dist > s->out_len) { s->err = 1; return -1; }
            if (room(s, len) != 0) return -1;
            for (i = 0; i < len; i++) { s->out[s->out_len] = s->out[s->out_len - dist]; s->out_len++; }
        }
    }
}

static int stored(st_t* s) {
    int len, nlen;
    s->bitbuf = 0; s->bitcnt = 0;
    if (s->pos + 4 > s->in_len) { s->err = 3; return -1; }
    len = s->in[s->pos] | (s->in[s->pos + 1] << 8);
    nlen = s->in[s->pos + 2] | (s->in[s->pos + 3] << 8);
    s->pos += 4;
    if (len != (~nlen & 0xffff)) { s->err = 1; return -1; }
    while (len-- > 0) {
        if (s->pos >= s->in_len) { s->err = 3; return -1; }
        if (room(s, 1) != 0) return -1;
        s->out[s->out_len++] = s->in[s->pos++];
    }
    return 0;
}

static int fixed(st_t* s) {
    static huff_t lh, dh;
    static int ready;
    if (!ready) {
        uint8_t l[288];
        int i;
        for (i = 0; i < 144; i++) l[i] = 8;
        for (; i < 256; i++) l[i] = 9;
        for (; i < 280; i++) l[i] = 7;
        for (; i < 288; i++) l[i] = 8;
        build(&lh, l, 288);
        for (i = 0; i < 30; i++) l[i] = 5;
        build(&dh, l, 30);
        ready = 1;
    }
    return codes(s, &lh, &dh);
}

static int dynamic(st_t* s) {
    static const uint8_t ord[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
    static huff_t lh, dh;
    uint8_t len[320];
    int nlen, ndist, ncode, i, idx;
    nlen = bits(s, 5); ndist = bits(s, 5); ncode = bits(s, 4);
    if (nlen < 0 || ndist < 0 || ncode < 0) return -1;
    nlen += 257; ndist += 1; ncode += 4;
    if (nlen > 286 || ndist > 30) { s->err = 1; return -1; }
    for (i = 0; i < 19; i++) len[i] = 0;
    for (i = 0; i < ncode; i++) { int v = bits(s, 3); if (v < 0) return -1; len[ord[i]] = (uint8_t)v; }
    if (build(&lh, len, 19) != 0) { s->err = 1; return -1; }
    idx = 0;
    while (idx < nlen + ndist) {
        int sym = decode(s, &lh), rep, v = 0;
        if (sym < 0) return -1;
        if (sym < 16) { len[idx++] = (uint8_t)sym; continue; }
        if (sym == 16) { if (idx == 0) { s->err = 1; return -1; } v = len[idx - 1]; rep = bits(s, 2); if (rep < 0) return -1; rep += 3; }
        else if (sym == 17) { rep = bits(s, 3); if (rep < 0) return -1; rep += 3; }
        else { rep = bits(s, 7); if (rep < 0) return -1; rep += 11; }
        if (idx + rep > nlen + ndist) { s->err = 1; return -1; }
        while (rep--) len[idx++] = (uint8_t)v;
    }
    if (len[256] == 0) { s->err = 1; return -1; }
    if (build(&lh, len, nlen) != 0 && nlen - lh.count[0] != 1) { s->err = 1; return -1; }
    if (build(&dh, len + nlen, ndist) != 0 && ndist - dh.count[0] != 1) { s->err = 1; return -1; }
    return codes(s, &lh, &dh);
}

static int run(st_t* s) {
    int last, type;
    do {
        last = bits(s, 1);
        type = bits(s, 2);
        if (last < 0 || type < 0) return -1;
        if (type == 0) { if (stored(s) != 0) return -1; }
        else if (type == 1) { if (fixed(s) != 0) return -1; }
        else if (type == 2) { if (dynamic(s) != 0) return -1; }
        else { s->err = 1; return -1; }
    } while (!last);
    return 0;
}

static int inflate_core(const uint8_t* in, int in_len, int* in_used, uint8_t* out, int out_cap, int inplace, int* truncated) {
    st_t s;
    s.in = in; s.in_len = in_len; s.pos = 0; s.bitbuf = 0; s.bitcnt = 0;
    s.out = out; s.out_len = 0; s.out_cap = out_cap; s.inplace = inplace; s.err = 0;
    *truncated = 0;
    if (run(&s) != 0) {
        if (s.err == 1) return -1;
        *truncated = 1;          /* output full (2) or input cut short (3): keep what we have */
    }
    if (in_used) *in_used = s.pos;
    return s.out_len;
}

int wf_inflate(const uint8_t* in, int in_len, int* in_used, uint8_t* out, int out_cap, int* truncated) {
    return inflate_core(in, in_len, in_used, out, out_cap, 0, truncated);
}

int wf_decode_body_inplace(int enc, uint8_t* buf, int len, int cap, int* truncated) {
    int i, p = 0, src, n;
    if (len <= 0 || len > cap) return len <= 0 ? 0 : -1;
    /* move the compressed bytes to the end of the buffer */
    src = cap - len;
    for (i = len - 1; i >= 0; i--) buf[src + i] = buf[i];
    if (enc == WF_ENC_GZIP) {
        const uint8_t* g = buf + src;
        int flg;
        if (len < 18 || g[0] != 0x1f || g[1] != 0x8b || g[2] != 8) return -1;
        flg = g[3];
        p = 10;
        if (flg & 4) { if (p + 2 > len) return -1; p += 2 + (g[p] | (g[p + 1] << 8)); }
        if (flg & 8) { while (p < len && g[p]) p++; p++; }
        if (flg & 16) { while (p < len && g[p]) p++; p++; }
        if (flg & 2) p += 2;
        if (p >= len) return -1;
    } else if (enc == WF_ENC_DEFLATE) {
        const uint8_t* z = buf + src;
        /* zlib header (CMF/FLG) if it checks, else raw deflate (old IIS) */
        if (len >= 2 && (z[0] & 0x0f) == 8 && ((z[0] << 8) | z[1]) % 31 == 0 && !(z[1] & 0x20)) p = 2;
    } else {
        return -1;
    }
    n = inflate_core(buf + src + p, len - p, 0, buf, cap, 1, truncated);
    return n;
}
