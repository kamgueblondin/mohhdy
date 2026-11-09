/* edid.h - Lot 69 (US-080): EDID 1.x base block parser.
 * The physical size of the screen for the form-factor plan. Pure code
 * (no I/O), host tested in tests/unit/kernel/test_edid.c. The block comes
 * from the Bochs/QEMU std VGA MMIO BAR (kernel/gfx_fb.c); real monitors
 * would give it over DDC, which this kernel does not drive. */
#ifndef EDID_H
#define EDID_H
#include <stdint.h>

#define EDID_BLOCK 128

typedef struct {
    int ok;              /* header and checksum valid */
    int version, revision;
    char mfg[4];         /* PNP id, e.g. "RHT" for QEMU */
    uint16_t product;
    int year;            /* manufacture year, 0 unknown */
    int cm_w, cm_h;      /* basic display size, cm (0 = unknown/aspect) */
    int mm_w, mm_h;      /* preferred detailed timing image size, mm */
    int native_w, native_h; /* preferred timing active pixels */
    int size_mm_w, size_mm_h; /* best size: DTD mm, else cm * 10 */
    int diag_tenths;     /* diagonal in 0.1 inch from size_mm, 0 unknown */
    /* Lot 85: more of the base block. 0 means the block did not say. */
    int digital;         /* 1 digital input, 0 analog or absent */
    int gamma_100;       /* 100 = gamma 1.00; 0 unknown (stored byte was 0 or 255) */
    int refresh_hz;      /* preferred detailed timing, 0 when blanking is absent */
    int range_vmin, range_vmax; /* Hz */
    int range_hmin, range_hmax; /* kHz */
    int range_clock;     /* max pixel clock, MHz */
    char name[14];       /* monitor descriptor 0xFC, or "" */
    int standard_n;      /* 0..8 standard timings */
    int standard_w[8], standard_h[8], standard_hz[8];
    int established_n;   /* how many of the 17 VESA established modes are set */
    int established;     /* bit i set => established mode i (0..16) */
    /* Lot 86: still the base block read at boot, not a DDC transaction. */
    int week;            /* manufacture week 1..54, else 0 */
    uint32_t serial;     /* binary serial, bytes 12..15, 0 if the block says 0 */
    char serial_text[14]; /* descriptor 0xFF, or "" */
    int chroma_ok;       /* 1 when any chromaticity byte was non-zero */
    int chroma_wx, chroma_wy; /* white point, 0..1023 (10-bit, /1024) */
    int dtd_n;           /* detailed timings found, including the preferred one */
    int dtd_w[4], dtd_h[4], dtd_hz[4];
    /* Lot 86: extension count from byte 126. The bytes after the base
     * block are not read. This is still not a DDC transaction. */
    int ext_blocks;
} edid_info_t;

/* 0 on success (info->ok = 1); -1 bad header, -2 bad checksum, -3 args */
int edid_parse(const uint8_t *block, int len, edid_info_t *info);
/* Established timing bit 0..16 -> width and height. 0 ok, -1 out of range.
 * Bit 0 is 720x400@70, bit 16 is 1152x870@75 (EDID byte 35 bit 7 first). */
int edid_established_mode(int bit, int *w, int *h);
/* Refresh of established timing bit 0..16. 0 ok, -1 out of range. */
int edid_established_hz(int bit, int *hz);
/* Preferred detailed timing when the block has one, otherwise the
 * established mode with the most pixels. 0 ok, -1 none. */
int edid_best_mode(const edid_info_t *e, int *w, int *h, int *hz);
/* "edid ok RHT 1024x768 260x195mm 12.8in" style line; returns length */
int edid_describe(const edid_info_t *info, char *out, int cap);
uint32_t edid_isqrt(uint32_t v);

/* Lot 87 (US-080): CEA-861 extension block (tag 0x02), the block after
 * the base block when byte 126 says there is one. QEMU's std VGA may or
 * may not expose one depending on its version; the parser does not
 * depend on it and is host tested with hand-built blocks. */
#define EDID_CEA_SVD 16
#define EDID_CEA_SAD 8
#define EDID_CEA_DTD 6
enum { EDID_AUDIO_LPCM = 1, EDID_AUDIO_AC3 = 2, EDID_AUDIO_AAC = 6, EDID_AUDIO_DTS = 7, EDID_AUDIO_EAC3 = 10 };
typedef struct {
    int ok;
    int revision;                /* 1..3 */
    int dtd_offset;              /* byte 2: 0 = no DTD and no data block */
    int underscan, basic_audio, ycbcr444, ycbcr422;
    int native_dtds;             /* byte 3 low nibble */
    int svd_n;                   /* short video descriptors kept (<= EDID_CEA_SVD) */
    int svd_total;               /* seen, including the ones not kept */
    uint8_t vic[EDID_CEA_SVD];
    uint8_t svd_native[EDID_CEA_SVD];
    int sad_n;
    int sad_format[EDID_CEA_SAD], sad_channels[EDID_CEA_SAD];
    int sad_rates[EDID_CEA_SAD]; /* bit 0 32 kHz .. bit 6 192 kHz */
    int sad_extra[EDID_CEA_SAD]; /* LPCM: bit depth mask (16/20/24); else max kbps */
    int hdmi;                    /* HDMI 1.x vendor block (OUI 00-0C-03) */
    uint16_t phys_addr;          /* a.b.c.d nibbles */
    int hdmi_dc;                 /* deep colour bits: 30, 36, 48 as 1/2/4 */
    int hdmi_tmds_mhz;           /* 0 when not given */
    int hf;                      /* HDMI Forum block (OUI C4-5D-D8) */
    int hf_tmds_mhz;
    int speakers;                /* speaker allocation mask, 0 none */
    int vcdb, quant_rgb, quant_ycc; /* video capability block */
    int colorimetry;             /* extended tag 5 mask */
    int hdr, hdr_eotf;           /* extended tag 6 */
    int hdr_max_lum;             /* code value, 0 not given */
    int y420_n;                  /* VICs in the YCbCr 4:2:0 video block */
    int unknown_blocks;
    int dtd_n;
    int dtd_w[EDID_CEA_DTD], dtd_h[EDID_CEA_DTD], dtd_hz[EDID_CEA_DTD];
} edid_cea_t;

/* 0 ok; -1 not a CEA block; -2 checksum; -3 args; -4 bad layout
 * (offset or data block past the DTD area). */
int edid_cea_parse(const uint8_t *blk, int len, edid_cea_t *c);
/* Base block and up to the extensions the buffer holds: 0 a CEA block was
 * parsed, 1 extension present but not CEA (c->ok = 0, *tag set), -1 none
 * or the base block is bad. */
int edid_parse_ext(const uint8_t *data, int len, edid_info_t *e, edid_cea_t *c, int *tag);
/* CTA VIC -> mode for the common progressive and interlaced codes.
 * 0 ok, -1 unknown. *interlaced may be 0. */
int edid_vic_mode(int vic, int *w, int *h, int *hz, int *interlaced);
/* native SVD, else the first DTD, else the largest known VIC. 0 / -1 */
int edid_cea_best(const edid_cea_t *c, int *w, int *h, int *hz);
/* "cea ok rev 3 svd 4 best 1920x1080@60 audio 1 hdmi 1.0.0.0 dtd 1" */
int edid_cea_describe(const edid_cea_t *c, char *out, int cap);
#endif
