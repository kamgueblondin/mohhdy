/* imgdec.h - Lot 45: shared image decoders (Ring 3, freestanding, no libc).
 * Used by the shell (img-info) and the desktop viewer (osui_tools.c).
 *
 * Formats: binary PNM (P5, P6), BMP (uncompressed 24 and 32 bits, bottom-up
 * or top-down) and PNG (bit depth 8, color types 0 2 3 4 6, no interlace)
 * with a bounded inflate (stored, fixed and dynamic Huffman blocks).
 * Memory: PNG needs a caller-owned scratch of IMGDEC_PNG_RAW(w, h, ch)
 * bytes for the unfiltered scanlines; larger images are refused with
 * IMGDEC_E_TOOBIG. Nothing is allocated. */
#ifndef IMGDEC_H
#define IMGDEC_H

enum { IMGDEC_NONE = 0, IMGDEC_PNM = 1, IMGDEC_BMP = 2, IMGDEC_PNG = 3 };
enum {
    IMGDEC_OK = 0, IMGDEC_E_FORMAT = -1, IMGDEC_E_TRUNC = -2, IMGDEC_E_UNSUP = -3,
    IMGDEC_E_TOOBIG = -4, IMGDEC_E_INFLATE = -5, IMGDEC_E_CRC = -6
};
#define IMGDEC_MAX_DIM 4096
#define IMGDEC_PNG_RAW(w, h, ch) ((unsigned)(h) * (1U + (unsigned)(w) * (unsigned)(ch)))

typedef struct {
    int fmt, w, h;
    int channels;          /* PNG: bytes per pixel of the scanlines */
    int bpp;               /* BMP bits per pixel, PNG 8 * channels, PNM 8 or 24 */
    int color_type;        /* PNG only */
    /* decoded source */
    const unsigned char *data;     /* BMP / PNM pixel data, PNG raw scanlines */
    int stride;                    /* bytes per row of data */
    int top_down;                  /* BMP */
    unsigned char pal[256][3];     /* PNG palette */
    int npal;
} imgdec_t;

/* format and size only (no pixel decoding, no scratch) */
int imgdec_probe(const unsigned char *buf, int n, imgdec_t *img);
/* full decode; scratch is used for PNG only */
int imgdec_open(const unsigned char *buf, int n, unsigned char *scratch, unsigned scratch_max, imgdec_t *img);
/* one pixel of an opened image */
void imgdec_pixel(const imgdec_t *img, int x, int y, unsigned char rgb[3]);
/* nearest-neighbour render into rgb (max_w x max_h, row-major RGB).
 * zoom_pct 0: fit (aspect kept, never upscaled); else that zoom, the
 * window showing the region at (pan_x, pan_y) source pixels, clamped.
 * Returns 0 and the output size. */
int imgdec_render(const imgdec_t *img, unsigned char *rgb, int max_w, int max_h,
                  int zoom_pct, int pan_x, int pan_y, int *w_out, int *h_out);
const char *imgdec_fmt_name(int fmt);
const char *imgdec_strerror(int rc);
/* zlib stream (RFC 1950/1951) into out; returns bytes or < 0 */
int imgdec_inflate_zlib(const unsigned char *in, int n, unsigned char *out, unsigned out_max);
#endif
