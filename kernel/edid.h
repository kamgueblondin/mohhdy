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
} edid_info_t;

/* 0 on success (info->ok = 1); -1 bad header, -2 bad checksum, -3 args */
int edid_parse(const uint8_t *block, int len, edid_info_t *info);
/* "edid ok RHT 1024x768 260x195mm 12.8in" style line; returns length */
int edid_describe(const edid_info_t *info, char *out, int cap);
uint32_t edid_isqrt(uint32_t v);
#endif
