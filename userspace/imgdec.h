/* imgdec.h - Lot 45: shared image decoders (Ring 3, freestanding, no libc).
 * Used by the shell (img-info) and the desktop viewer (osui_tools.c).
 *
 * Formats: binary PNM (P5, P6), BMP (uncompressed 24 and 32 bits, bottom-up
 * or top-down), PNG (color types 0 2 3 4 6, every bit depth the spec allows
 * (1 2 4 8 16), Adam7 interlace; 16-bit samples keep their high byte) with a
 * bounded inflate, and JPEG: baseline (SOF0/SOF1) and progressive Huffman
 * (SOF2, spectral selection and successive approximation), gray or YCbCr,
 * sampling 1..2, restart markers. Arithmetic, lossless, 12-bit and CMYK
 * JPEG are refused with E_UNSUP.
 * Lot 72: GIF87a/89a (first frame, global or local palette, transparency
 * shown white, interlace, LZW up to 12 bits) and .ico favicons (entry
 * nearest 32 px, PNG or DIB 1/4/8/24/32 bits with AND mask).
 * Memory: PNG and JPEG need a caller-owned scratch of imgdec_scratch_need()
 * bytes (after probe); larger images are refused with IMGDEC_E_TOOBIG.
 * Nothing is allocated. */
#ifndef IMGDEC_H
#define IMGDEC_H

enum { IMGDEC_NONE = 0, IMGDEC_PNM = 1, IMGDEC_BMP = 2, IMGDEC_PNG = 3, IMGDEC_JPEG = 4, IMGDEC_GIF = 5, IMGDEC_ICO = 6 };
enum {
    IMGDEC_OK = 0, IMGDEC_E_FORMAT = -1, IMGDEC_E_TRUNC = -2, IMGDEC_E_UNSUP = -3,
    IMGDEC_E_TOOBIG = -4, IMGDEC_E_INFLATE = -5, IMGDEC_E_CRC = -6
};
#define IMGDEC_MAX_DIM 4096
/* legacy: raw scanlines of an 8-bit non-interlaced PNG */
#define IMGDEC_PNG_RAW(w, h, ch) ((unsigned)(h) * (1U + (unsigned)(w) * (unsigned)(ch)))

typedef struct {
    int fmt, w, h;
    int channels;          /* PNG/JPEG: 8-bit samples per decoded pixel */
    int bpp;               /* BMP bits per pixel, PNG depth * channels, JPEG 8 * channels, PNM 8 or 24 */
    int color_type;        /* PNG only */
    int depth;             /* PNG bit depth (1 2 4 8 16) */
    int interlace;         /* PNG Adam7 */
    int jpeg_prog;         /* JPEG SOF2: coefficient buffer follows the pixels */
    int jpeg_hmax, jpeg_vmax, jpeg_mcu; /* sampling and blocks per MCU */
    /* decoded source */
    const unsigned char *data;     /* BMP / PNM pixel data, PNG raw scanlines */
    int stride;                    /* bytes per row of data */
    int top_down;                  /* BMP */
    unsigned char pal[256][3];     /* PNG palette */
    int npal;
    int frames;            /* Lot 72: GIF images in the file (first one decoded) */
    int gif_pixels;
    int full_w, full_h;    /* Lot 72: size in the file (w, h are the decoded size) */
    int scale_shift;
    int ico_off, ico_len, ico_png;   /* Lot 72: chosen .ico entry (a PNG entry opens as IMGDEC_PNG) */
    unsigned ico_need;       /* Lot 72: JPEG decoded at 1/2^shift (imgdec_open_scaled) */        /* Lot 72: GIF frame pixels decoded (less if the stream is cut) */
} imgdec_t;

/* format and size only (no pixel decoding, no scratch) */
int imgdec_probe(const unsigned char *buf, int n, imgdec_t *img);
/* scratch bytes imgdec_open needs for a probed image (0: none) */
unsigned imgdec_scratch_need(const imgdec_t *img);
/* full decode; scratch holds PNG rows, or JPEG pixels plus SOF2 coefficients */
int imgdec_open(const unsigned char *buf, int n, unsigned char *scratch, unsigned scratch_max, imgdec_t *img);
/* Lot 72: JPEG decoded at 1/2^shift (0..3) of its size, point sampled,
 * so a large photo fits a small scratch; other formats ignore shift.
 * img->w / img->h are the decoded size, full_w / full_h the file size. */
unsigned imgdec_scratch_need_scaled(const imgdec_t *img, int shift);
int imgdec_open_scaled(const unsigned char *buf, int n, unsigned char *scratch, unsigned scratch_max, int shift, imgdec_t *img);
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
