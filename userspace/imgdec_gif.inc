/* imgdec_gif.inc - Lot 72: GIF decoder, included by imgdec.c only.
 * Freestanding, no libc, no allocation, bounded loops.
 * GIF87a and GIF89a: logical screen, global and local color tables,
 * graphic control extension (transparent index), LZW codes 2..12 bits,
 * interlaced or sequential rows. imgdec_open decodes the first image
 * (frame); later frames are counted (img->frames) and skipped.
 * Lot 73: animation (imgdec_anim_begin / imgdec_anim_next): every frame is
 * composed onto a full-screen canvas in order, with its delay, its
 * disposal method (0/1 keep, 2 restore to background, 3 restore to the
 * previous picture) and the loop count of the NETSCAPE2.0 / ANIMEXTS1.0
 * application extension. Transparent pixels
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

typedef struct {
    int transparent;          /* index or -1 */
    int delay;                /* 1/100 s */
    int disposal;             /* 0..7 (3 bits of the packed field) */
} gif_gce_t;

/* NETSCAPE2.0 / ANIMEXTS1.0 loop sub-block at r (after the 11-byte
 * application identifier); -1 when absent. */
static int gif_loop_block(gif_rd_t *r) {
    if (r->pos + 4 <= r->n && r->b[r->pos] == 3 && r->b[r->pos + 1] == 1)
        return r->b[r->pos + 2] | (r->b[r->pos + 3] << 8);
    return -1;
}

static int gif_app_is_loop(const unsigned char *id) {
    static const char a[] = "NETSCAPE2.0", b2[] = "ANIMEXTS1.0";
    int k, ea = 1, eb = 1;
    for (k = 0; k < 11; k++) { if (id[k] != (unsigned char)a[k]) ea = 0; if (id[k] != (unsigned char)b2[k]) eb = 0; }
    return ea || eb;
}

/* Reads extensions and descriptors up to the next image. Records the
 * graphic control extension that applies (transparent index, delay,
 * disposal) and, when loop is given, the application loop count.
 * IMGDEC_E_FORMAT at the trailer. */
static int gif_find_frame_ext(gif_rd_t *r, gif_frame_t *f, gif_gce_t *g, int *loop) {
    int guard;
    g->transparent = -1;
    g->delay = 0;
    g->disposal = 0;
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
                g->transparent = (flags & 1) ? r->b[r->pos + 3] : -1;
                g->delay = r->b[r->pos + 1] | (r->b[r->pos + 2] << 8);
                g->disposal = (flags >> 2) & 7;
                r->pos += len;
            } else if (label == 0xFF) {
                int len = gif_u8(r);
                if (len < 0 || r->pos + len > r->n) return IMGDEC_E_TRUNC;
                if (len == 11 && gif_app_is_loop(r->b + r->pos)) {
                    r->pos += len;
                    if (loop) { int v = gif_loop_block(r); if (v >= 0) *loop = v; }
                } else r->pos += len;
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
        return IMGDEC_OK;
    }
    return IMGDEC_E_FORMAT;
}

static int gif_find_frame(gif_rd_t *r, imgdec_t *img, gif_frame_t *f, int *transparent) {
    gif_gce_t g;
    int rc = gif_find_frame_ext(r, f, &g, 0);
    (void)img;
    *transparent = g.transparent;
    return rc;
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

/* LZW tables of one frame decode (in the caller work area). */
typedef struct {
    unsigned short *prefix;
    unsigned char *suffix, *first, *stack;
} gif_lzw_t;

static void gif_lzw_at(unsigned char *at, gif_lzw_t *t) {
    t->prefix = (unsigned short *)(void *)at;
    t->suffix = (unsigned char *)(t->prefix + GIF_LZW_MAX);
    t->first = t->suffix + GIF_LZW_MAX;
    t->stack = t->first + GIF_LZW_MAX;   /* a code expands to at most GIF_LZW_MAX bytes */
}

/* Decodes the image data of frame f onto canvas (img->w x img->h RGB):
 * pixels of the transparent index and outside the screen are left as
 * they are. r->pos ends after the frame data (terminator included) when
 * *term_seen. Returns the frame pixels decoded. */
static int gif_decode_frame(gif_rd_t *r, const imgdec_t *img, const gif_frame_t *f, int tr,
                            unsigned char *canvas, const gif_lzw_t *t, int *term_seen) {
    const unsigned char *b = r->b, *pal;
    unsigned char pal_local[256][3];
    unsigned short *prefix = t->prefix;
    unsigned char *suffix = t->suffix, *first = t->first, *stack = t->stack;
    int k, ncolors, clear, eoi, next, width, prev, nbits = 0, sub_left = 0, written = 0, total, done = 0;
    unsigned acc = 0;
    *term_seen = 0;
    if (f->lct_size) {
        for (k = 0; k < f->lct_size; k++) { pal_local[k][0] = b[f->lct + 3 * k]; pal_local[k][1] = b[f->lct + 3 * k + 1]; pal_local[k][2] = b[f->lct + 3 * k + 2]; }
        pal = &pal_local[0][0];
        ncolors = f->lct_size;
    } else {
        pal = &img->pal[0][0];
        ncolors = img->npal;
    }
    if (ncolors == 0) return -1;
    clear = 1 << f->min_code;
    eoi = clear + 1;
    next = clear + 2;
    width = f->min_code + 1;
    prev = -1;
    total = f->w * f->h;
    for (k = 0; k < clear; k++) { prefix[k] = 0xFFFF; suffix[k] = (unsigned char)k; first[k] = (unsigned char)k; }
    r->pos = f->data_pos;
    while (!done && written < total) {
        int code, cur, len;
        while (nbits < width) {
            int c;
            if (sub_left == 0) {
                sub_left = gif_u8(r);
                if (sub_left <= 0) { done = 1; *term_seen = sub_left == 0; break; }
            }
            c = gif_u8(r);
            if (c < 0) { done = 1; break; }
            sub_left--;
            acc |= (unsigned)c << nbits;
            nbits += 8;
        }
        if (done) break;
        code = (int)(acc & ((1U << width) - 1U));
        acc >>= width;
        nbits -= width;
        if (code == clear) { next = clear + 2; width = f->min_code + 1; prev = -1; continue; }
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
                int idx = st[--len], x = written % f->w, ky = written / f->w;
                int y = f->interlace ? gif_interlace_row(ky, f->h) : ky;
                int sx = f->left + x, sy = f->top + y;
                written++;
                if (idx == tr || sx >= img->w || sy >= img->h) continue;
                if (idx >= ncolors) idx = 0;
                {
                    unsigned char *p = canvas + ((unsigned)sy * (unsigned)img->w + (unsigned)sx) * 3U;
                    p[0] = pal[idx * 3]; p[1] = pal[idx * 3 + 1]; p[2] = pal[idx * 3 + 2];
                }
            }
        }
    }
    /* skip the rest of the frame data */
    if (!*term_seen && !done && r->pos + sub_left <= r->n) {
        r->pos += sub_left;
        *term_seen = gif_skip_blocks(r) == 0;
    }
    return written;
}

static int open_gif(const unsigned char *b, int n, unsigned char *scratch, unsigned smax, imgdec_t *img) {
    gif_rd_t r;
    gif_frame_t f;
    gif_lzw_t t;
    int tr, rc, written, term_seen;
    unsigned px_bytes, i;
    if (!scratch || gif_scratch_need(img) > smax) return IMGDEC_E_TOOBIG;
    r.b = b; r.n = n; r.pos = 13 + img->npal * 3;
    rc = gif_find_frame(&r, img, &f, &tr);
    if (rc) return rc;
    px_bytes = (unsigned)img->w * (unsigned)img->h * 3U;
    for (i = 0; i < px_bytes; i++) scratch[i] = 255;
    if (scratch + ((px_bytes + 3U) & ~3U) + GIF_TABLE_BYTES > scratch + smax) return IMGDEC_E_TOOBIG;
    gif_lzw_at(scratch + ((px_bytes + 3U) & ~3U), &t);
    written = gif_decode_frame(&r, img, &f, tr, scratch, &t, &term_seen);
    if (written < 0) return IMGDEC_E_FORMAT;
    img->gif_pixels = written;
    /* count later frames */
    img->frames = 1;
    if (term_seen) img->frames = gif_count_frames(&r);
    img->data = scratch;
    img->stride = img->w * 3;
    return written > 0 ? IMGDEC_OK : IMGDEC_E_TRUNC;
}

/* ---------------------------------------------------------------- Lot 73: animation */

static unsigned gif_anim_need(const imgdec_t *img) {
    unsigned px = (unsigned)img->w * (unsigned)img->h * 3U;
    return ((px + 3U) & ~3U) * 2U + GIF_TABLE_BYTES;
}

unsigned imgdec_anim_need(const imgdec_t *img) {
    if (!img || img->fmt != IMGDEC_GIF) return 0;
    return gif_anim_need(img);
}

int imgdec_anim_begin(const unsigned char *b, int n, imgdec_t *img, imgdec_anim_t *a, unsigned char *work, unsigned max) {
    unsigned px, i;
    if (!b || !img || !a || img->fmt != IMGDEC_GIF) return IMGDEC_E_FORMAT;
    if (!work || gif_anim_need(img) > max) return IMGDEC_E_TOOBIG;
    px = (unsigned)img->w * (unsigned)img->h * 3U;
    for (i = 0; i < px; i++) work[i] = 255;
    a->pos = 13 + img->npal * 3;
    a->frame = 0;
    a->loop = -1;
    a->delay_cs = 0;
    a->disposal = 0;
    a->px = a->py = a->pw = a->ph = 0;
    a->done = 0;
    img->data = work;
    img->stride = img->w * 3;
    (void)n;
    return IMGDEC_OK;
}

/* rectangle of the frame clipped to the screen: 0 when empty */
static int gif_clip(const imgdec_t *img, int x, int y, int w, int h, int *x1, int *y1) {
    *x1 = x + w > img->w ? img->w : x + w;
    *y1 = y + h > img->h ? img->h : y + h;
    return x < *x1 && y < *y1;
}

int imgdec_anim_next(const unsigned char *b, int n, imgdec_t *img, imgdec_anim_t *a, unsigned char *work, unsigned max) {
    gif_rd_t r;
    gif_frame_t f;
    gif_gce_t g;
    gif_lzw_t t;
    unsigned px, half;
    unsigned char *canvas = work, *save;
    int rc, written, term_seen, x, y, x1, y1;
    if (!b || !img || !a || !work || gif_anim_need(img) > max) return IMGDEC_E_TOOBIG;
    if (a->done) return 0;
    px = (unsigned)img->w * (unsigned)img->h * 3U;
    half = (px + 3U) & ~3U;
    save = work + half;
    gif_lzw_at(work + 2U * half, &t);
    r.b = b; r.n = n; r.pos = a->pos;
    rc = gif_find_frame_ext(&r, &f, &g, &a->loop);
    if (rc == IMGDEC_E_FORMAT || rc == IMGDEC_E_TRUNC) { a->done = 1; return a->frame > 0 ? 0 : rc; }
    if (rc) { a->done = 1; return rc; }
    /* disposal of the frame shown until now (only once a next frame
     * exists: at the end of the file the last picture stays) */
    if (a->frame > 0 && (a->disposal == 2 || a->disposal == 3) && gif_clip(img, a->px, a->py, a->pw, a->ph, &x1, &y1)) {
        for (y = a->py; y < y1; y++)
            for (x = a->px; x < x1; x++) {
                unsigned o = ((unsigned)y * (unsigned)img->w + (unsigned)x) * 3U;
                if (a->disposal == 2) { canvas[o] = 255; canvas[o + 1] = 255; canvas[o + 2] = 255; }
                else { canvas[o] = save[o]; canvas[o + 1] = save[o + 1]; canvas[o + 2] = save[o + 2]; }
            }
    }
    /* disposal 3: keep what the frame covers to restore it afterwards */
    if (g.disposal == 3 && gif_clip(img, f.left, f.top, f.w, f.h, &x1, &y1))
        for (y = f.top; y < y1; y++)
            for (x = f.left; x < x1; x++) {
                unsigned o = ((unsigned)y * (unsigned)img->w + (unsigned)x) * 3U;
                save[o] = canvas[o]; save[o + 1] = canvas[o + 1]; save[o + 2] = canvas[o + 2];
            }
    written = gif_decode_frame(&r, img, &f, g.transparent, canvas, &t, &term_seen);
    if (written <= 0) { a->done = 1; return a->frame > 0 ? 0 : IMGDEC_E_TRUNC; }
    a->delay_cs = g.delay;
    a->disposal = g.disposal;
    a->px = f.left; a->py = f.top; a->pw = f.w; a->ph = f.h;
    a->frame++;
    a->pos = r.pos;
    if (!term_seen) a->done = 1;        /* cut stream: this frame is the last */
    img->data = canvas;
    img->stride = img->w * 3;
    return 1;
}
