/* imgdec_gif.inc - Lot 72: GIF decoder, included by imgdec.c only.
 * Freestanding, no libc, no allocation, bounded loops.
 * GIF87a and GIF89a: logical screen, global and local color tables,
 * graphic control extension (transparent index), LZW codes 2..12 bits,
 * interlaced or sequential rows. Only the first image (frame) is decoded;
 * later frames are counted (img->frames) and skipped. Transparent pixels
 * and the part of the screen the frame does not cover are white (the
 * page background of the web pane and the viewer).
 * Output: 8-bit RGB in the caller scratch, then the LZW tables
 * (imgdec_scratch_need). Corrupt code streams stop at the first bad code;
 * the rows already decoded are kept (as browsers do), the rest stays white. */

#define GIF_LZW_MAX 4096
#define GIF_TABLE_BYTES (GIF_LZW_MAX * 2U + GIF_LZW_MAX * 3U + 4U)   /* prefix, suffix, first, stack */

typedef struct {
    const unsigned char *b;
    int n, pos;
} gif_rd_t;

static int gif_u8(gif_rd_t *r) { return r->pos < r->n ? r->b[r->pos++] : -1; }
static int gif_u16(gif_rd_t *r) {
    int a = gif_u8(r), b = gif_u8(r);
    return (a < 0 || b < 0) ? -1 : (a | (b << 8));
}

/* Skips a chain of data sub-blocks. 0 at the terminator, -1 if truncated. */
static int gif_skip_blocks(gif_rd_t *r) {
    int guard = 0;
    for (;;) {
        int len = gif_u8(r);
        if (len < 0) return -1;
        if (len == 0) return 0;
        if (r->pos + len > r->n) return -1;
        r->pos += len;
        if (++guard > 1 << 20) return -1;
    }
}

typedef struct {
    int left, top, w, h, interlace;
    int lct, lct_size;        /* local color table offset in the file, entries */
    int min_code;
    int data_pos;             /* first sub-block of the image data */
} gif_frame_t;

/* Reads extensions and descriptors up to the first image. Records the
 * transparent index of the graphic control extension that applies. */
static int gif_find_frame(gif_rd_t *r, imgdec_t *img, gif_frame_t *f, int *transparent) {
    int guard;
    *transparent = -1;
    for (guard = 0; guard < 4096; guard++) {
        int c = gif_u8(r);
        if (c < 0) return IMGDEC_E_TRUNC;
        if (c == 0x3B) return IMGDEC_E_FORMAT;          /* trailer before any image */
        if (c == 0x21) {
            int label = gif_u8(r);
            if (label < 0) return IMGDEC_E_TRUNC;
            if (label == 0xF9) {
                int len = gif_u8(r), flags;
                if (len < 4 || r->pos + len > r->n) return IMGDEC_E_TRUNC;
                flags = r->b[r->pos];
                *transparent = (flags & 1) ? r->b[r->pos + 3] : -1;
                r->pos += len;
            }
            if (gif_skip_blocks(r)) return IMGDEC_E_TRUNC;
            continue;
        }
        if (c != 0x2C) return IMGDEC_E_FORMAT;
        f->left = gif_u16(r); f->top = gif_u16(r); f->w = gif_u16(r); f->h = gif_u16(r);
        c = gif_u8(r);
        if (f->h < 0 || c < 0) return IMGDEC_E_TRUNC;
        if (f->w > IMGDEC_MAX_DIM || f->h > IMGDEC_MAX_DIM) return IMGDEC_E_TOOBIG;
        f->interlace = (c >> 6) & 1;
        f->lct = 0; f->lct_size = 0;
        if (c & 0x80) {
            f->lct_size = 2 << (c & 7);
            f->lct = r->pos;
            if (r->pos + f->lct_size * 3 > r->n) return IMGDEC_E_TRUNC;
            r->pos += f->lct_size * 3;
        }
        f->min_code = gif_u8(r);
        if (f->min_code < 0) return IMGDEC_E_TRUNC;
        if (f->min_code < 2 || f->min_code > 8) return IMGDEC_E_FORMAT;
        f->data_pos = r->pos;
        (void)img;
        return IMGDEC_OK;
    }
    return IMGDEC_E_FORMAT;
}

static int probe_gif(const unsigned char *b, int n, imgdec_t *img) {
    gif_rd_t r;
    gif_frame_t f;
    int flags, tr, rc;
    if (n < 6 || b[0] != 'G' || b[1] != 'I' || b[2] != 'F' || b[3] != '8' || (b[4] != '7' && b[4] != '9') || b[5] != 'a')
        return IMGDEC_E_FORMAT;
    img->fmt = IMGDEC_GIF;
    if (n < 13) return IMGDEC_E_TRUNC;
    img->w = (int)le16(b + 6);
    img->h = (int)le16(b + 8);
    if (!sane(img->w, img->h)) return IMGDEC_E_TOOBIG;
    flags = b[10];
    img->channels = 3;
    img->bpp = 24;
    img->depth = (flags & 7) + 1;
    r.b = b; r.n = n; r.pos = 13;
    img->npal = 0;
    if (flags & 0x80) {
        int k, cnt = 2 << (flags & 7);
        if (13 + cnt * 3 > n) return IMGDEC_E_TRUNC;
        for (k = 0; k < cnt; k++) { img->pal[k][0] = b[13 + 3 * k]; img->pal[k][1] = b[14 + 3 * k]; img->pal[k][2] = b[15 + 3 * k]; }
        img->npal = cnt;
        r.pos += cnt * 3;
    }
    rc = gif_find_frame(&r, img, &f, &tr);
    if (rc) return rc;
    img->interlace = f.interlace;
    return IMGDEC_OK;
}

static unsigned gif_scratch_need(const imgdec_t *img) {
    return (unsigned)img->w * (unsigned)img->h * 3U + GIF_TABLE_BYTES;
}

/* Counts the image descriptors after the first one (animated GIF). */
static int gif_count_frames(gif_rd_t *r) {
    int frames = 1, guard;
    for (guard = 0; guard < 65536; guard++) {
        int c = gif_u8(r);
        if (c < 0 || c == 0x3B) break;
        if (c == 0x21) {
            if (gif_u8(r) < 0 || gif_skip_blocks(r)) break;
        } else if (c == 0x2C) {
            int fl;
            if (r->pos + 9 > r->n) break;
            fl = r->b[r->pos + 8];
            r->pos += 9;
            if (fl & 0x80) r->pos += (2 << (fl & 7)) * 3;
            if (r->pos >= r->n) break;
            r->pos++;                                 /* LZW min code size */
            if (gif_skip_blocks(r)) break;
            frames++;
        } else break;
    }
    return frames;
}

/* Row of the k-th decoded line of an interlaced frame (4 passes). */
static int gif_interlace_row(int k, int h) {
    int n1 = (h + 7) / 8, n2 = (h + 3) / 8, n3 = (h + 1) / 4;
    if (k < n1) return k * 8;
    k -= n1;
    if (k < n2) return 4 + k * 8;
    k -= n2;
    if (k < n3) return 2 + k * 4;
    k -= n3;
    return 1 + k * 2;
}

static int open_gif(const unsigned char *b, int n, unsigned char *scratch, unsigned smax, imgdec_t *img) {
    gif_rd_t r;
    gif_frame_t f;
    int tr, rc, k, ncolors;
    unsigned px_bytes, i;
    unsigned short *prefix;
    unsigned char *suffix, *stack, *first;
    const unsigned char *pal;
    int clear, eoi, next, width, prev;
    unsigned acc = 0;
    int nbits = 0, sub_left = 0, written = 0, total, done = 0, term_seen = 0;
    unsigned char pal_local[256][3];
    if (!scratch || gif_scratch_need(img) > smax) return IMGDEC_E_TOOBIG;
    r.b = b; r.n = n; r.pos = 13 + img->npal * 3;
    rc = gif_find_frame(&r, img, &f, &tr);
    if (rc) return rc;
    px_bytes = (unsigned)img->w * (unsigned)img->h * 3U;
    for (i = 0; i < px_bytes; i++) scratch[i] = 255;
    prefix = (unsigned short *)(void *)(scratch + ((px_bytes + 3U) & ~3U));
    if ((unsigned char *)prefix + GIF_TABLE_BYTES > scratch + smax) return IMGDEC_E_TOOBIG;
    suffix = (unsigned char *)(prefix + GIF_LZW_MAX);
    first = suffix + GIF_LZW_MAX;
    stack = first + GIF_LZW_MAX;   /* a code expands to at most GIF_LZW_MAX bytes */
    if (f.lct_size) {
        for (k = 0; k < f.lct_size; k++) { pal_local[k][0] = b[f.lct + 3 * k]; pal_local[k][1] = b[f.lct + 3 * k + 1]; pal_local[k][2] = b[f.lct + 3 * k + 2]; }
        pal = &pal_local[0][0];
        ncolors = f.lct_size;
    } else {
        pal = &img->pal[0][0];
        ncolors = img->npal;
    }
    if (ncolors == 0) return IMGDEC_E_FORMAT;
    clear = 1 << f.min_code;
    eoi = clear + 1;
    next = clear + 2;
    width = f.min_code + 1;
    prev = -1;
    total = f.w * f.h;
    for (k = 0; k < clear; k++) { prefix[k] = 0xFFFF; suffix[k] = (unsigned char)k; first[k] = (unsigned char)k; }
    r.pos = f.data_pos;
    while (!done && written < total) {
        int code, cur, len;
        while (nbits < width) {
            int c;
            if (sub_left == 0) {
                sub_left = gif_u8(&r);
                if (sub_left <= 0) { done = 1; term_seen = sub_left == 0; break; }
            }
            c = gif_u8(&r);
            if (c < 0) { done = 1; break; }
            sub_left--;
            acc |= (unsigned)c << nbits;
            nbits += 8;
        }
        if (done) break;
        code = (int)(acc & ((1U << width) - 1U));
        acc >>= width;
        nbits -= width;
        if (code == clear) { next = clear + 2; width = f.min_code + 1; prev = -1; continue; }
        if (code == eoi) break;
        if (prev < 0) {
            if (code >= clear) break;                   /* corrupt: first code after clear */
            cur = code;
        } else if (code < next) {
            cur = code;
            if (next < GIF_LZW_MAX) {
                prefix[next] = (unsigned short)prev;
                suffix[next] = first[code];
                first[next] = first[prev];
                next++;
            }
        } else if (code == next && next < GIF_LZW_MAX) {
            prefix[next] = (unsigned short)prev;
            suffix[next] = first[prev];
            first[next] = first[prev];
            next++;
            cur = code;
        } else break;                                    /* corrupt code */
        if (next == (1 << width) && width < 12) width++;
        prev = cur;
        /* expand cur backwards into the stack, then emit */
        {
            unsigned char *st = stack;
            int c2 = cur;
            len = 0;
            while (c2 != 0xFFFF && len < GIF_LZW_MAX) {
                st[len++] = suffix[c2];
                c2 = prefix[c2];
            }
            if (c2 != 0xFFFF) break;
            while (len > 0 && written < total) {
                int idx = st[--len], x = written % f.w, ky = written / f.w;
                int y = f.interlace ? gif_interlace_row(ky, f.h) : ky;
                int sx = f.left + x, sy = f.top + y;
                written++;
                if (idx == tr || sx >= img->w || sy >= img->h) continue;
                if (idx >= ncolors) idx = 0;
                {
                    unsigned char *p = scratch + ((unsigned)sy * (unsigned)img->w + (unsigned)sx) * 3U;
                    p[0] = pal[idx * 3]; p[1] = pal[idx * 3 + 1]; p[2] = pal[idx * 3 + 2];
                }
            }
        }
    }
    img->gif_pixels = written;
    /* skip the rest of the frame data and count later frames */
    img->frames = 1;
    if (!term_seen && r.pos + sub_left <= r.n) {
        r.pos += sub_left;
        term_seen = gif_skip_blocks(&r) == 0;
    }
    if (term_seen) img->frames = gif_count_frames(&r);
    img->data = scratch;
    img->stride = img->w * 3;
    return written > 0 ? IMGDEC_OK : IMGDEC_E_TRUNC;
}
