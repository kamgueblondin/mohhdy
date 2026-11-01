/* imgdec_ico.inc - Lot 72: Windows icon (.ico, favicons), included by
 * imgdec.c only. Freestanding, no allocation, bounded.
 * The directory entry closest to 32 pixels is chosen (a larger one wins
 * over a smaller one at the same distance). PNG entries are decoded by
 * the PNG decoder; DIB entries (BITMAPINFOHEADER, 1 4 8 24 32 bits,
 * bottom-up, AND mask) are decoded to RGB in the scratch. Transparent
 * pixels (alpha < 128, or AND mask bit set) are white. */

typedef struct { int off, len, w, h; } ico_pick_t;

static int ico_choose(const unsigned char *b, int n, ico_pick_t *pk) {
    int count, k, best = -1, best_d = 1 << 30, best_w = 0;
    if (n < 6 || b[0] || b[1] || b[2] != 1 || b[3]) return IMGDEC_E_FORMAT;
    count = (int)le16(b + 4);
    if (count < 1 || count > 256) return IMGDEC_E_FORMAT;
    if (6 + 16 * count > n) return IMGDEC_E_TRUNC;
    for (k = 0; k < count; k++) {
        const unsigned char *e = b + 6 + 16 * k;
        int w = e[0] ? e[0] : 256, d = w > 32 ? w - 32 : 32 - w;
        unsigned len = le32(e + 8), off = le32(e + 12);
        if (off >= (unsigned)n || len < 16U || len > (unsigned)n - off) continue;
        if (d < best_d || (d == best_d && w > best_w)) { best = k; best_d = d; best_w = w; }
    }
    if (best < 0) return IMGDEC_E_TRUNC;
    {
        const unsigned char *e = b + 6 + 16 * best;
        pk->off = (int)le32(e + 12);
        pk->len = (int)le32(e + 8);
        pk->w = e[0] ? e[0] : 256;
        pk->h = e[1] ? e[1] : 256;
    }
    return IMGDEC_OK;
}

static int ico_is_png(const unsigned char *p, int n) {
    return n >= 8 && p[0] == 0x89 && p[1] == 'P' && p[2] == 'N' && p[3] == 'G';
}

/* DIB header of an entry: size from the header (height counts XOR + AND) */
static int ico_dib(const unsigned char *p, int n, int *w, int *h, int *bpp, int *ncol) {
    unsigned hs;
    int hh;
    if (n < 40) return IMGDEC_E_TRUNC;
    hs = le32(p);
    if (hs < 40U || hs > (unsigned)n) return IMGDEC_E_FORMAT;
    *w = (int)le32(p + 4);
    hh = (int)le32(p + 8);
    *bpp = (int)le16(p + 14);
    if (le32(p + 16) != 0U) return IMGDEC_E_UNSUP;   /* compressed DIB */
    *h = hh / 2;
    if (*w <= 0 || *h <= 0 || *w > 256 || *h > 256) return IMGDEC_E_FORMAT;
    if (*bpp != 1 && *bpp != 4 && *bpp != 8 && *bpp != 24 && *bpp != 32) return IMGDEC_E_UNSUP;
    *ncol = *bpp <= 8 ? (le32(p + 32) ? (int)le32(p + 32) : 1 << *bpp) : 0;
    if (*ncol > 256) return IMGDEC_E_FORMAT;
    return (int)hs;
}

static int probe_ico(const unsigned char *b, int n, imgdec_t *img) {
    ico_pick_t pk;
    int rc = ico_choose(b, n, &pk), w, h, bpp, ncol, hs;
    if (rc == IMGDEC_E_FORMAT) return rc;
    img->fmt = IMGDEC_ICO;
    if (rc) return rc;
    img->ico_off = pk.off;
    img->ico_len = pk.len;
    if (ico_is_png(b + pk.off, pk.len)) {
        imgdec_t sub;
        zero(&sub);
        rc = probe_png(b + pk.off, pk.len, &sub);
        if (rc) return rc;
        img->w = sub.w; img->h = sub.h; img->channels = sub.channels; img->bpp = sub.bpp;
        img->ico_png = 1;
        img->ico_need = png_raw_size(&sub) + (unsigned)sub.w * (unsigned)sub.h * (unsigned)sub.channels;
        return IMGDEC_OK;
    }
    hs = ico_dib(b + pk.off, pk.len, &w, &h, &bpp, &ncol);
    if (hs < 0) return hs;
    img->w = w; img->h = h; img->channels = 3; img->bpp = bpp;
    return IMGDEC_OK;
}

static unsigned ico_scratch_need(const imgdec_t *img) {
    return img->ico_png ? img->ico_need : (unsigned)img->w * (unsigned)img->h * 3U;
}

static int open_ico(const unsigned char *b, int n, unsigned char *scratch, unsigned smax, imgdec_t *img) {
    const unsigned char *p = b + img->ico_off;
    int len = img->ico_len, w, h, bpp, ncol, hs, y, x;
    unsigned xor_stride, and_stride, need;
    const unsigned char *pal, *xr, *am;
    (void)n;
    if (img->ico_png) {
        imgdec_t sub;
        int rc;
        zero(&sub);
        rc = probe_png(p, len, &sub);
        if (rc) return rc;
        if (!scratch || img->ico_need > smax) return IMGDEC_E_TOOBIG;
        rc = open_png(p, len, scratch, smax, &sub);
        if (rc) return rc;
        sub.ico_png = 1;
        sub.ico_off = img->ico_off;
        sub.ico_len = img->ico_len;
        sub.full_w = sub.w; sub.full_h = sub.h;
        *img = sub;                  /* pixels read as a PNG */
        return IMGDEC_OK;
    }
    hs = ico_dib(p, len, &w, &h, &bpp, &ncol);
    if (hs < 0) return hs;
    need = (unsigned)w * (unsigned)h * 3U;
    if (!scratch || need > smax) return IMGDEC_E_TOOBIG;
    pal = p + hs;
    xr = pal + ncol * 4;
    xor_stride = (((unsigned)w * (unsigned)bpp + 31U) / 32U) * 4U;
    and_stride = (((unsigned)w + 31U) / 32U) * 4U;
    if ((unsigned)(xr - p) + xor_stride * (unsigned)h > (unsigned)len) return IMGDEC_E_TRUNC;
    am = xr + xor_stride * (unsigned)h;
    if (bpp == 32 || (unsigned)(am - p) + and_stride * (unsigned)h > (unsigned)len) am = 0;   /* alpha, or no mask */
    for (y = 0; y < h; y++) {
        const unsigned char *row = xr + xor_stride * (unsigned)(h - 1 - y);
        const unsigned char *mrow = am ? am + and_stride * (unsigned)(h - 1 - y) : 0;
        for (x = 0; x < w; x++) {
            unsigned char *o = scratch + ((unsigned)y * (unsigned)w + (unsigned)x) * 3U;
            int r, g, bl, clear = 0;
            if (bpp >= 24) {
                const unsigned char *q = row + x * (bpp / 8);
                bl = q[0]; g = q[1]; r = q[2];
                if (bpp == 32 && q[3] < 128) clear = 1;
            } else {
                int idx;
                if (bpp == 8) idx = row[x];
                else if (bpp == 4) idx = (row[x >> 1] >> ((x & 1) ? 0 : 4)) & 15;
                else idx = (row[x >> 3] >> (7 - (x & 7))) & 1;
                if (idx >= ncol) idx = 0;
                bl = pal[idx * 4]; g = pal[idx * 4 + 1]; r = pal[idx * 4 + 2];
            }
            if (mrow && ((mrow[x >> 3] >> (7 - (x & 7))) & 1)) clear = 1;
            if (clear) { r = g = bl = 255; }
            o[0] = (unsigned char)r; o[1] = (unsigned char)g; o[2] = (unsigned char)bl;
        }
    }
    img->w = w; img->h = h; img->channels = 3;
    img->data = scratch;
    img->stride = w * 3;
    return IMGDEC_OK;
}
