/* imgdec.c - see imgdec.h. Freestanding: no libc, no allocation. */
#include "imgdec.h"

#define MAX_SEG 256

/* ---- input over one or more segments (PNG IDAT chunks) ---------------- */
typedef struct {
    const unsigned char *seg[MAX_SEG];
    int len[MAX_SEG];
    int nseg, cur, pos;
    unsigned bitbuf;
    int bitcnt;
    int eof;
} rd_t;

static int rd_byte(rd_t *r) {
    while (r->cur < r->nseg && r->pos >= r->len[r->cur]) { r->cur++; r->pos = 0; }
    if (r->cur >= r->nseg) { r->eof = 1; return 0; }
    return r->seg[r->cur][r->pos++];
}

static unsigned rd_bits(rd_t *r, int n) {
    unsigned v;
    while (r->bitcnt < n) { r->bitbuf |= (unsigned)rd_byte(r) << r->bitcnt; r->bitcnt += 8; }
    v = r->bitbuf & ((1U << n) - 1U);
    r->bitbuf >>= n;
    r->bitcnt -= n;
    return v;
}

/* ---- inflate (RFC 1951), canonical Huffman like zlib's puff ----------- */
typedef struct { short count[16]; short sym[320]; } huff_t;

static int huff_build(huff_t *h, const short *len, int n) {
    short offs[16];
    int i, left = 1;
    for (i = 0; i < 16; i++) h->count[i] = 0;
    for (i = 0; i < n; i++) h->count[len[i]]++;
    if (h->count[0] == n) return 0;
    for (i = 1; i < 16; i++) { left <<= 1; left -= h->count[i]; if (left < 0) return -1; }
    offs[1] = 0;
    for (i = 1; i < 15; i++) offs[i + 1] = (short)(offs[i] + h->count[i]);
    for (i = 0; i < n; i++) if (len[i]) h->sym[offs[len[i]]++] = (short)i;
    return left;
}

static int huff_decode(rd_t *r, const huff_t *h) {
    int code = 0, first = 0, index = 0, l;
    for (l = 1; l < 16; l++) {
        int count;
        code |= (int)rd_bits(r, 1);
        count = h->count[l];
        if (code - count < first) return h->sym[index + (code - first)];
        index += count; first += count; first <<= 1; code <<= 1;
        if (r->eof) return -1;
    }
    return -1;
}

static const short k_lbase[29] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258 };
static const short k_lext[29] = { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0 };
static const short k_dbase[30] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577 };
static const short k_dext[30] = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13 };

static int codes(rd_t *r, unsigned char *out, unsigned max, unsigned *op, const huff_t *lc, const huff_t *dc) {
    for (;;) {
        int sym = huff_decode(r, lc);
        if (sym < 0 || r->eof) return IMGDEC_E_INFLATE;
        if (sym < 256) {
            if (*op >= max) return IMGDEC_E_TOOBIG;
            out[(*op)++] = (unsigned char)sym;
        } else if (sym == 256) {
            return 0;
        } else {
            unsigned len, dist, k;
            sym -= 257;
            if (sym >= 29) return IMGDEC_E_INFLATE;
            len = (unsigned)k_lbase[sym] + rd_bits(r, k_lext[sym]);
            sym = huff_decode(r, dc);
            if (sym < 0 || sym >= 30) return IMGDEC_E_INFLATE;
            dist = (unsigned)k_dbase[sym] + rd_bits(r, k_dext[sym]);
            if (dist > *op) return IMGDEC_E_INFLATE;
            if (*op + len > max) return IMGDEC_E_TOOBIG;
            for (k = 0; k < len; k++) { out[*op] = out[*op - dist]; (*op)++; }
        }
    }
}

static int inflate_rd(rd_t *r, unsigned char *out, unsigned max) {
    static huff_t lc, dc;          /* off the (small) user stack */
    unsigned op = 0;
    int last;
    do {
        int type, rc;
        last = (int)rd_bits(r, 1);
        type = (int)rd_bits(r, 2);
        if (r->eof) return IMGDEC_E_TRUNC;
        if (type == 0) {
            unsigned len, nlen, k;
            r->bitbuf = 0; r->bitcnt = 0;
            len = (unsigned)rd_byte(r); len |= (unsigned)rd_byte(r) << 8;
            nlen = (unsigned)rd_byte(r); nlen |= (unsigned)rd_byte(r) << 8;
            if (r->eof || (len ^ 0xFFFFU) != nlen) return IMGDEC_E_INFLATE;
            if (op + len > max) return IMGDEC_E_TOOBIG;
            for (k = 0; k < len; k++) out[op++] = (unsigned char)rd_byte(r);
            if (r->eof) return IMGDEC_E_TRUNC;
        } else if (type == 1) {
            short l[320];
            int i;
            for (i = 0; i < 144; i++) l[i] = 8;
            for (; i < 256; i++) l[i] = 9;
            for (; i < 280; i++) l[i] = 7;
            for (; i < 288; i++) l[i] = 8;
            huff_build(&lc, l, 288);
            for (i = 0; i < 30; i++) l[i] = 5;
            huff_build(&dc, l, 30);
            rc = codes(r, out, max, &op, &lc, &dc);
            if (rc) return rc;
        } else if (type == 2) {
            static const short ord[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
            short l[320];
            int nlen, ndist, ncode, i, idx;
            nlen = (int)rd_bits(r, 5) + 257;
            ndist = (int)rd_bits(r, 5) + 1;
            ncode = (int)rd_bits(r, 4) + 4;
            if (nlen > 286 || ndist > 30) return IMGDEC_E_INFLATE;
            for (i = 0; i < 19; i++) l[i] = 0;
            for (i = 0; i < ncode; i++) l[ord[i]] = (short)rd_bits(r, 3);
            if (huff_build(&lc, l, 19) != 0) return IMGDEC_E_INFLATE;
            idx = 0;
            while (idx < nlen + ndist) {
                int sym = huff_decode(r, &lc), rep, v = 0;
                if (sym < 0 || r->eof) return IMGDEC_E_INFLATE;
                if (sym < 16) { l[idx++] = (short)sym; continue; }
                if (sym == 16) { if (!idx) return IMGDEC_E_INFLATE; v = l[idx - 1]; rep = 3 + (int)rd_bits(r, 2); }
                else if (sym == 17) rep = 3 + (int)rd_bits(r, 3);
                else rep = 11 + (int)rd_bits(r, 7);
                if (idx + rep > nlen + ndist) return IMGDEC_E_INFLATE;
                while (rep--) l[idx++] = (short)v;
            }
            if (l[256] == 0) return IMGDEC_E_INFLATE;
            if (huff_build(&lc, l, nlen) < 0) return IMGDEC_E_INFLATE;
            if (huff_build(&dc, l + nlen, ndist) < 0) return IMGDEC_E_INFLATE;
            rc = codes(r, out, max, &op, &lc, &dc);
            if (rc) return rc;
        } else return IMGDEC_E_INFLATE;
    } while (!last);
    return (int)op;
}

static int zlib_header(rd_t *r) {
    int cmf = rd_byte(r), flg = rd_byte(r);
    if (r->eof || (cmf & 0x0F) != 8 || ((cmf << 8) | flg) % 31 != 0 || (flg & 0x20)) return -1;
    return 0;
}

int imgdec_inflate_zlib(const unsigned char *in, int n, unsigned char *out, unsigned out_max) {
    static rd_t r;
    if (!in || n < 2 || !out) return IMGDEC_E_FORMAT;
    r.seg[0] = in; r.len[0] = n; r.nseg = 1; r.cur = 0; r.pos = 0; r.bitbuf = 0; r.bitcnt = 0; r.eof = 0;
    if (zlib_header(&r)) return IMGDEC_E_FORMAT;
    return inflate_rd(&r, out, out_max);
}

/* ---- helpers ---------------------------------------------------------- */
static unsigned be32(const unsigned char *p) { return ((unsigned)p[0] << 24) | ((unsigned)p[1] << 16) | ((unsigned)p[2] << 8) | p[3]; }
static unsigned le32(const unsigned char *p) { return ((unsigned)p[3] << 24) | ((unsigned)p[2] << 16) | ((unsigned)p[1] << 8) | p[0]; }
static unsigned le16(const unsigned char *p) { return ((unsigned)p[1] << 8) | p[0]; }

static unsigned g_crc_tab[256];
static int g_crc_ready;
static unsigned crc32(const unsigned char *p, unsigned n) {
    unsigned c = 0xFFFFFFFFU, i;
    if (!g_crc_ready) {
        unsigned k, v;
        int j;
        for (k = 0; k < 256; k++) { v = k; for (j = 0; j < 8; j++) v = (v & 1) ? 0xEDB88320U ^ (v >> 1) : v >> 1; g_crc_tab[k] = v; }
        g_crc_ready = 1;
    }
    for (i = 0; i < n; i++) c = g_crc_tab[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFU;
}

static int pnm_num(const unsigned char *b, int n, int *i, int *v) {
    int x = 0, got = 0;
    for (;;) {
        while (*i < n && (b[*i] == ' ' || b[*i] == '\n' || b[*i] == '\r' || b[*i] == '\t')) (*i)++;
        if (*i < n && b[*i] == '#') { while (*i < n && b[*i] != '\n') (*i)++; continue; }
        break;
    }
    while (*i < n && b[*i] >= '0' && b[*i] <= '9' && x < 100000) { x = x * 10 + (b[*i] - '0'); (*i)++; got = 1; }
    *v = x;
    return got ? 0 : -1;
}

static void zero(imgdec_t *img) {
    unsigned char *p = (unsigned char *)img;
    unsigned i;
    for (i = 0; i < sizeof(*img); i++) p[i] = 0;
}

static int sane(int w, int h) { return w > 0 && h > 0 && w <= IMGDEC_MAX_DIM && h <= IMGDEC_MAX_DIM; }

/* ---- probe ------------------------------------------------------------ */
static int probe_png(const unsigned char *b, int n, imgdec_t *img) {
    static const unsigned char sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    int i, ch;
    for (i = 0; i < 8; i++) if (i >= n || b[i] != sig[i]) return IMGDEC_E_FORMAT;
    if (n < 33) return IMGDEC_E_TRUNC;
    if (be32(b + 8) != 13 || b[12] != 'I' || b[13] != 'H' || b[14] != 'D' || b[15] != 'R') return IMGDEC_E_FORMAT;
    img->fmt = IMGDEC_PNG;
    img->w = (int)be32(b + 16); img->h = (int)be32(b + 20);
    img->color_type = b[25];
    if (!sane(img->w, img->h)) return IMGDEC_E_TOOBIG;
    if (b[24] != 8 || b[26] != 0 || b[27] != 0 || b[28] != 0) return IMGDEC_E_UNSUP;   /* depth 8, no interlace */
    switch (img->color_type) {
    case 0: ch = 1; break;
    case 2: ch = 3; break;
    case 3: ch = 1; break;
    case 4: ch = 2; break;
    case 6: ch = 4; break;
    default: return IMGDEC_E_UNSUP;
    }
    img->channels = ch;
    img->bpp = 8 * ch;
    return IMGDEC_OK;
}

static int probe_bmp(const unsigned char *b, int n, imgdec_t *img) {
    int w, h, bpp;
    unsigned off, hsz, comp;
    if (n < 2 || b[0] != 'B' || b[1] != 'M') return IMGDEC_E_FORMAT;
    if (n < 54) return IMGDEC_E_TRUNC;
    off = le32(b + 10); hsz = le32(b + 14);
    if (hsz < 40) return IMGDEC_E_UNSUP;
    w = (int)le32(b + 18); h = (int)le32(b + 22);
    bpp = (int)le16(b + 28); comp = le32(b + 30);
    img->fmt = IMGDEC_BMP;
    img->top_down = h < 0;
    if (h < 0) h = -h;
    img->w = w; img->h = h; img->bpp = bpp;
    if (!sane(w, h)) return IMGDEC_E_TOOBIG;
    if ((bpp != 24 && bpp != 32) || !(comp == 0 || (comp == 3 && bpp == 32))) return IMGDEC_E_UNSUP;
    img->stride = ((w * (bpp / 8)) + 3) & ~3;
    if (off >= (unsigned)n || (unsigned)img->stride * (unsigned)h > (unsigned)n - off) return IMGDEC_E_TRUNC;
    img->data = b + off;
    return IMGDEC_OK;
}

static int probe_pnm(const unsigned char *b, int n, imgdec_t *img) {
    int i = 2, w, h, maxv;
    if (n < 3 || b[0] != 'P' || (b[1] != '5' && b[1] != '6')) return IMGDEC_E_FORMAT;
    if (pnm_num(b, n, &i, &w) || pnm_num(b, n, &i, &h) || pnm_num(b, n, &i, &maxv)) return IMGDEC_E_FORMAT;
    img->fmt = IMGDEC_PNM;
    img->w = w; img->h = h;
    img->channels = b[1] == '6' ? 3 : 1;
    img->bpp = 8 * img->channels;
    if (!sane(w, h)) return IMGDEC_E_TOOBIG;
    if (maxv != 255) return IMGDEC_E_UNSUP;
    i++;
    img->stride = w * img->channels;
    if (i > n || (unsigned)img->stride * (unsigned)h > (unsigned)(n - i)) return IMGDEC_E_TRUNC;
    img->data = b + i;
    return IMGDEC_OK;
}

int imgdec_probe(const unsigned char *buf, int n, imgdec_t *img) {
    int rc;
    if (!buf || !img || n <= 0) return IMGDEC_E_FORMAT;
    zero(img);
    rc = probe_png(buf, n, img); if (rc != IMGDEC_E_FORMAT) return rc;
    zero(img);
    rc = probe_bmp(buf, n, img); if (rc != IMGDEC_E_FORMAT) return rc;
    zero(img);
    return probe_pnm(buf, n, img);
}

/* ---- PNG decode ------------------------------------------------------- */
static int paeth(int a, int b, int c) {
    int p = a + b - c, pa = p > a ? p - a : a - p, pb = p > b ? p - b : b - p, pc = p > c ? p - c : c - p;
    if (pa <= pb && pa <= pc) return a;
    return pb <= pc ? b : c;
}

static int open_png(const unsigned char *b, int n, unsigned char *scratch, unsigned smax, imgdec_t *img) {
    static rd_t r;
    unsigned need = IMGDEC_PNG_RAW(img->w, img->h, img->channels);
    int pos = 8, got_end = 0, y;
    int bpp = img->channels, stride = img->w * img->channels;
    if (!scratch || need > smax) return IMGDEC_E_TOOBIG;
    r.nseg = 0; r.cur = 0; r.pos = 0; r.bitbuf = 0; r.bitcnt = 0; r.eof = 0;
    while (pos + 12 <= n) {
        unsigned len = be32(b + pos);
        const unsigned char *type = b + pos + 4;
        if (len > (unsigned)(n - pos - 12)) return IMGDEC_E_TRUNC;
        if (crc32(type, len + 4) != be32(b + pos + 8 + len)) return IMGDEC_E_CRC;
        if (type[0] == 'P' && type[1] == 'L' && type[2] == 'T' && type[3] == 'E') {
            unsigned k;
            img->npal = (int)(len / 3) > 256 ? 256 : (int)(len / 3);
            for (k = 0; k < (unsigned)img->npal; k++) { img->pal[k][0] = type[4 + k * 3]; img->pal[k][1] = type[5 + k * 3]; img->pal[k][2] = type[6 + k * 3]; }
        } else if (type[0] == 'I' && type[1] == 'D' && type[2] == 'A' && type[3] == 'T') {
            if (r.nseg >= MAX_SEG) return IMGDEC_E_UNSUP;
            r.seg[r.nseg] = type + 4; r.len[r.nseg] = (int)len; r.nseg++;
        } else if (type[0] == 'I' && type[1] == 'E' && type[2] == 'N' && type[3] == 'D') {
            got_end = 1;
            break;
        }
        pos += 12 + (int)len;
    }
    if (!got_end || !r.nseg) return IMGDEC_E_TRUNC;
    if (img->color_type == 3 && !img->npal) return IMGDEC_E_FORMAT;
    if (zlib_header(&r)) return IMGDEC_E_FORMAT;
    {
        int got = inflate_rd(&r, scratch, need);
        if (got < 0) return got;
        if ((unsigned)got != need) return IMGDEC_E_TRUNC;
    }
    /* unfilter in place: row y at scratch + y * (1 + stride), filter byte first */
    for (y = 0; y < img->h; y++) {
        unsigned char *row = scratch + (unsigned)y * (unsigned)(1 + stride);
        const unsigned char *prev = y ? row - (1 + stride) + 1 : 0;
        int f = row[0], x;
        unsigned char *p = row + 1;
        if (f > 4) return IMGDEC_E_FORMAT;
        for (x = 0; x < stride; x++) {
            int a = x >= bpp ? p[x - bpp] : 0, up = prev ? prev[x] : 0, c = (prev && x >= bpp) ? prev[x - bpp] : 0;
            switch (f) {
            case 1: p[x] = (unsigned char)(p[x] + a); break;
            case 2: p[x] = (unsigned char)(p[x] + up); break;
            case 3: p[x] = (unsigned char)(p[x] + ((a + up) >> 1)); break;
            case 4: p[x] = (unsigned char)(p[x] + paeth(a, up, c)); break;
            default: break;
            }
        }
    }
    img->data = scratch + 1;
    img->stride = 1 + stride;
    return IMGDEC_OK;
}

int imgdec_open(const unsigned char *buf, int n, unsigned char *scratch, unsigned scratch_max, imgdec_t *img) {
    int rc = imgdec_probe(buf, n, img);
    if (rc) return rc;
    if (img->fmt == IMGDEC_PNG) return open_png(buf, n, scratch, scratch_max, img);
    return IMGDEC_OK;
}

void imgdec_pixel(const imgdec_t *img, int x, int y, unsigned char rgb[3]) {
    const unsigned char *p;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= img->w) x = img->w - 1;
    if (y >= img->h) y = img->h - 1;
    if (img->fmt == IMGDEC_BMP) {
        int row = img->top_down ? y : img->h - 1 - y;
        p = img->data + row * img->stride + x * (img->bpp / 8);
        rgb[0] = p[2]; rgb[1] = p[1]; rgb[2] = p[0];
        return;
    }
    p = img->data + y * img->stride + x * img->channels;
    if (img->fmt == IMGDEC_PNG && img->color_type == 3) {
        int k = p[0] < img->npal ? p[0] : 0;
        rgb[0] = img->pal[k][0]; rgb[1] = img->pal[k][1]; rgb[2] = img->pal[k][2];
    } else if (img->channels >= 3) {
        rgb[0] = p[0]; rgb[1] = p[1]; rgb[2] = p[2];
    } else {
        rgb[0] = rgb[1] = rgb[2] = p[0];
    }
}

int imgdec_render(const imgdec_t *img, unsigned char *rgb, int max_w, int max_h,
                  int zoom_pct, int pan_x, int pan_y, int *w_out, int *h_out) {
    int tw, th, x, y;
    if (!img || !rgb || !img->data || max_w <= 0 || max_h <= 0) return IMGDEC_E_FORMAT;
    if (zoom_pct <= 0) {
        tw = img->w; th = img->h;
        if (tw > max_w) { th = (th * max_w) / tw; tw = max_w; }
        if (th > max_h) { tw = (tw * max_h) / th; th = max_h; }
        if (tw < 1) tw = 1;
        if (th < 1) th = 1;
        for (y = 0; y < th; y++)
            for (x = 0; x < tw; x++) imgdec_pixel(img, (x * img->w) / tw, (y * img->h) / th, rgb + (y * tw + x) * 3);
    } else {
        int zw, zh;
        if (zoom_pct > 800) zoom_pct = 800;
        if (zoom_pct < 10) zoom_pct = 10;
        zw = (img->w * zoom_pct) / 100; zh = (img->h * zoom_pct) / 100;
        if (zw < 1) zw = 1;
        if (zh < 1) zh = 1;
        tw = zw < max_w ? zw : max_w;
        th = zh < max_h ? zh : max_h;
        /* pan in source pixels, clamped so the window stays on the image */
        {
            int px = (pan_x * zoom_pct) / 100, py = (pan_y * zoom_pct) / 100;
            if (px > zw - tw) px = zw - tw;
            if (py > zh - th) py = zh - th;
            if (px < 0) px = 0;
            if (py < 0) py = 0;
            for (y = 0; y < th; y++)
                for (x = 0; x < tw; x++)
                    imgdec_pixel(img, ((x + px) * 100) / zoom_pct, ((y + py) * 100) / zoom_pct, rgb + (y * tw + x) * 3);
        }
    }
    if (w_out) *w_out = tw;
    if (h_out) *h_out = th;
    return IMGDEC_OK;
}

const char *imgdec_fmt_name(int fmt) {
    switch (fmt) { case IMGDEC_PNM: return "pnm"; case IMGDEC_BMP: return "bmp"; case IMGDEC_PNG: return "png"; default: return "inconnu"; }
}

const char *imgdec_strerror(int rc) {
    switch (rc) {
    case IMGDEC_OK: return "ok";
    case IMGDEC_E_FORMAT: return "format inconnu";
    case IMGDEC_E_TRUNC: return "fichier tronque";
    case IMGDEC_E_UNSUP: return "variante non supportee";
    case IMGDEC_E_TOOBIG: return "image trop grande";
    case IMGDEC_E_INFLATE: return "flux deflate invalide";
    case IMGDEC_E_CRC: return "crc PNG invalide";
    default: return "erreur";
    }
}
