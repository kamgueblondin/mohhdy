/* Lot 87 (US-080): CEA-861 extension parser in kernel/edid.c. The blocks
 * are built by hand from the CTA-861-G layout; QEMU is not needed. */
#include "../../framework/unity.h"
#include "kernel/edid.h"
#include <string.h>

static uint8_t g_x[2 * EDID_BLOCK];
static int g_pos;
static uint8_t *g_c;

static void sum_fix(uint8_t *b) {
    unsigned s = 0;
    int i;
    for (i = 0; i < 127; i++) s += b[i];
    b[127] = (uint8_t)((256u - (s & 0xFFu)) & 0xFFu);
}

/* base block with one extension, a 1024x768 preferred timing */
static void base(int ext) {
    static const uint8_t hdr[8] = { 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0 };
    uint8_t *d = g_x + 54;
    memset(g_x, 0, sizeof(g_x));
    memcpy(g_x, hdr, 8);
    g_x[8] = 0x49; g_x[9] = 0x14;   /* RHT */
    g_x[18] = 1; g_x[19] = 4;
    d[0] = 0x10; d[1] = 0x27; d[2] = 0x00; d[4] = 0x40; d[5] = 0x00; d[7] = 0x30;
    g_x[126] = (uint8_t)ext;
    sum_fix(g_x);
}

static void cea_begin(int rev, int flags) {
    g_c = g_x + EDID_BLOCK;
    memset(g_c, 0, EDID_BLOCK);
    g_c[0] = 0x02;
    g_c[1] = (uint8_t)rev;
    g_c[3] = (uint8_t)flags;
    g_pos = 4;
}
static void blk(int tag, const uint8_t *p, int n) {
    int i;
    g_c[g_pos++] = (uint8_t)((tag << 5) | n);
    for (i = 0; i < n; i++) g_c[g_pos++] = p[i];
}
/* 1920x1080@60 (148.5 MHz, 2200x1125 total) */
static void dtd1080(uint8_t *d) {
    d[0] = 0x02; d[1] = 0x3A;                 /* 14850 x 10 kHz */
    d[2] = 0x80; d[3] = 0x18; d[4] = 0x71;    /* 1920 active, 280 blank */
    d[5] = 0x38; d[6] = 0x2D; d[7] = 0x40;    /* 1080 active, 45 blank */
}
static void cea_end(int ndtd) {
    int i;
    g_c[2] = (uint8_t)g_pos;
    for (i = 0; i < ndtd; i++) { dtd1080(g_c + g_pos); g_pos += 18; }
    sum_fix(g_c);
}

static void tv_block(void) {
    static const uint8_t svd[] = { 0x90, 4, 3, 31, 97, 0 };          /* 16 native, 720p, 480p, 1080p50, 2160p60, reserved */
    static const uint8_t sad[] = { 0x09, 0x07, 0x07,                /* LPCM 2ch 32/44.1/48, 16/20/24 bit */
                                   0x15, 0x07, 0x50 };              /* AC-3 6ch, 640 kbps */
    static const uint8_t hdmi[] = { 0x03, 0x0C, 0x00, 0x10, 0x00, 0x30, 0x2D };   /* 1.0.0.0, DC30+36, 225 MHz */
    static const uint8_t hf[] = { 0xD8, 0x5D, 0xC4, 0x01, 0x78 };    /* 600 MHz */
    static const uint8_t spk[] = { 0x0F, 0x00, 0x00 };
    static const uint8_t vcdb[] = { 0x00, 0xC0 };
    static const uint8_t hdr[] = { 0x06, 0x05, 0x01, 0x78 };
    static const uint8_t y420[] = { 0x0E, 96, 97 };
    base(1);
    cea_begin(3, 0xF1);
    blk(2, svd, (int)sizeof(svd));
    blk(1, sad, (int)sizeof(sad));
    blk(3, hdmi, (int)sizeof(hdmi));
    blk(3, hf, (int)sizeof(hf));
    blk(4, spk, (int)sizeof(spk));
    blk(7, vcdb, (int)sizeof(vcdb));
    blk(7, hdr, (int)sizeof(hdr));
    blk(7, y420, (int)sizeof(y420));
    cea_end(1);
}

static void test_tv_block(void) {
    edid_cea_t c;
    edid_info_t e;
    int w = 0, h = 0, hz = 0, tag = -1;
    tv_block();
    TEST_ASSERT_EQUAL(0, edid_parse_ext(g_x, (int)sizeof(g_x), &e, &c, &tag));
    TEST_ASSERT_EQUAL(0x02, tag);
    TEST_ASSERT_EQUAL(1, e.ext_blocks);
    TEST_ASSERT_EQUAL(1024, e.native_w);
    TEST_ASSERT_EQUAL(1, c.ok);
    TEST_ASSERT_EQUAL(3, c.revision);
    TEST_ASSERT_EQUAL(1, c.underscan);
    TEST_ASSERT_EQUAL(1, c.basic_audio);
    TEST_ASSERT_EQUAL(1, c.ycbcr444);
    TEST_ASSERT_EQUAL(1, c.ycbcr422);
    TEST_ASSERT_EQUAL(1, c.native_dtds);
    TEST_ASSERT_EQUAL(5, c.svd_n);
    TEST_ASSERT_EQUAL(5, c.svd_total);
    TEST_ASSERT_EQUAL(16, c.vic[0]);
    TEST_ASSERT_EQUAL(1, c.svd_native[0]);
    TEST_ASSERT_EQUAL(0, c.svd_native[1]);
    TEST_ASSERT_EQUAL(97, c.vic[4]);
    TEST_ASSERT_EQUAL(2, c.sad_n);
    TEST_ASSERT_EQUAL(EDID_AUDIO_LPCM, c.sad_format[0]);
    TEST_ASSERT_EQUAL(2, c.sad_channels[0]);
    TEST_ASSERT_EQUAL(7, c.sad_rates[0]);
    TEST_ASSERT_EQUAL(7, c.sad_extra[0]);
    TEST_ASSERT_EQUAL(EDID_AUDIO_AC3, c.sad_format[1]);
    TEST_ASSERT_EQUAL(6, c.sad_channels[1]);
    TEST_ASSERT_EQUAL(640, c.sad_extra[1]);
    TEST_ASSERT_EQUAL(1, c.hdmi);
    TEST_ASSERT_EQUAL(0x1000, c.phys_addr);
    TEST_ASSERT_EQUAL(3, c.hdmi_dc);
    TEST_ASSERT_EQUAL(225, c.hdmi_tmds_mhz);
    TEST_ASSERT_EQUAL(1, c.hf);
    TEST_ASSERT_EQUAL(600, c.hf_tmds_mhz);
    TEST_ASSERT_EQUAL(0x0F, c.speakers);
    TEST_ASSERT_EQUAL(1, c.vcdb);
    TEST_ASSERT_EQUAL(1, c.quant_rgb);
    TEST_ASSERT_EQUAL(1, c.quant_ycc);
    TEST_ASSERT_EQUAL(1, c.hdr);
    TEST_ASSERT_EQUAL(5, c.hdr_eotf);
    TEST_ASSERT_EQUAL(0x78, c.hdr_max_lum);
    TEST_ASSERT_EQUAL(2, c.y420_n);
    TEST_ASSERT_EQUAL(0, c.unknown_blocks);
    TEST_ASSERT_EQUAL(1, c.dtd_n);
    TEST_ASSERT_EQUAL(1920, c.dtd_w[0]);
    TEST_ASSERT_EQUAL(1080, c.dtd_h[0]);
    TEST_ASSERT_EQUAL(60, c.dtd_hz[0]);
    /* the native SVD wins over the DTD and the larger 2160p VIC */
    TEST_ASSERT_EQUAL(0, edid_cea_best(&c, &w, &h, &hz));
    TEST_ASSERT_EQUAL(1920, w);
    TEST_ASSERT_EQUAL(1080, h);
    TEST_ASSERT_EQUAL(60, hz);
}


/* Lot 87: the summary SYS_DISPLAY_INFO carries */
static void test_summary_tv(void) {
    edid_cea_t c;
    edid_info_t e;
    edid_cea_sum_t s;
    int tag = 0, rc;
    tv_block();
    rc = edid_parse_ext(g_x, (int)sizeof(g_x), &e, &c, &tag);
    TEST_ASSERT_EQUAL(0, rc);
    edid_cea_summarize(&c, rc, tag, &s);
    TEST_ASSERT_EQUAL(0, s.status);
    TEST_ASSERT_EQUAL(3, s.rev);
    TEST_ASSERT_EQUAL(EDID_SUM_UNDERSCAN | EDID_SUM_AUDIO | EDID_SUM_YCC444 | EDID_SUM_YCC422 |
                      EDID_SUM_HDMI | EDID_SUM_HF | EDID_SUM_HDR | EDID_SUM_VCDB, s.flags);
    TEST_ASSERT_EQUAL(5, s.svd_n);
    TEST_ASSERT_EQUAL(16, s.native_vic);
    TEST_ASSERT_EQUAL(16, s.vic[0]);
    TEST_ASSERT_EQUAL(97, s.vic[4]);
    TEST_ASSERT_EQUAL(0, s.vic[5]);
    TEST_ASSERT_EQUAL(2, s.sad_n);
    TEST_ASSERT_EQUAL(6, s.sad_max_ch);
    TEST_ASSERT_EQUAL(1, s.dtd_n);
    TEST_ASSERT_EQUAL(2, s.y420_n);
    TEST_ASSERT_EQUAL(0x0F, s.speakers);
    TEST_ASSERT_EQUAL(5, s.hdr_eotf);
    TEST_ASSERT_EQUAL(1920, s.best_w);
    TEST_ASSERT_EQUAL(1080, s.best_h);
    TEST_ASSERT_EQUAL(60, s.best_hz);
    TEST_ASSERT_EQUAL(1, s.best_from);
    TEST_ASSERT_EQUAL(0x1000, s.phys_addr);
    TEST_ASSERT_EQUAL(600, s.tmds_mhz);            /* the HF block's clock is higher */
    TEST_ASSERT_EQUAL(1, edid_cea_is_tv(&s, 600));  /* underscan */
}

static void test_summary_monitor_and_sources(void) {
    static const uint8_t svd[] = { 4, 16, 97 };    /* no native bit */
    static const uint8_t hdmi[] = { 0x03, 0x0C, 0x00, 0x21, 0x00 };   /* 2.1.0.0 */
    edid_cea_t c;
    edid_info_t e;
    edid_cea_sum_t s;
    int tag = 0, rc;
    /* a DTD and no native SVD: best is the DTD */
    base(1);
    cea_begin(3, 0x40);
    blk(2, svd, (int)sizeof(svd));
    blk(3, hdmi, (int)sizeof(hdmi));
    cea_end(1);
    rc = edid_parse_ext(g_x, (int)sizeof(g_x), &e, &c, &tag);
    TEST_ASSERT_EQUAL(0, rc);
    edid_cea_summarize(&c, rc, tag, &s);
    TEST_ASSERT_EQUAL(0, s.native_vic);
    TEST_ASSERT_EQUAL(1920, s.best_w);
    TEST_ASSERT_EQUAL(2, s.best_from);
    TEST_ASSERT_EQUAL(0x2100, s.phys_addr);
    TEST_ASSERT_EQUAL(EDID_SUM_AUDIO | EDID_SUM_HDMI, s.flags);
    TEST_ASSERT_EQUAL(0, s.tmds_mhz);
    /* HDMI, no underscan, no native TV VIC, a physical size: a monitor */
    TEST_ASSERT_EQUAL(0, edid_cea_is_tv(&s, 600));
    /* the same sink without a physical size (a TV or a projector) */
    TEST_ASSERT_EQUAL(1, edid_cea_is_tv(&s, 0));
    /* no DTD either: the largest VIC (2160p) */
    base(1);
    cea_begin(3, 0x40);
    blk(2, svd, (int)sizeof(svd));
    cea_end(0);
    rc = edid_parse_ext(g_x, (int)sizeof(g_x), &e, &c, &tag);
    edid_cea_summarize(&c, rc, tag, &s);
    TEST_ASSERT_EQUAL(3840, s.best_w);
    TEST_ASSERT_EQUAL(3, s.best_from);
    TEST_ASSERT_EQUAL(0, s.phys_addr);
    /* without an HDMI block nothing is a TV */
    TEST_ASSERT_EQUAL(0, edid_cea_is_tv(&s, 0));
}

static void test_summary_status(void) {
    edid_cea_t c;
    edid_info_t e;
    edid_cea_sum_t s;
    int tag = 0, rc;
    /* no extension at all */
    base(0);
    rc = edid_parse_ext(g_x, (int)sizeof(g_x), &e, &c, &tag);
    TEST_ASSERT_EQUAL(-1, rc);
    edid_cea_summarize(&c, rc, tag, &s);
    TEST_ASSERT_EQUAL(1, s.status);
    TEST_ASSERT_EQUAL(0, edid_cea_is_tv(&s, 0));
    /* an extension with another tag (0x70 DisplayID) */
    base(1);
    memset(g_x + EDID_BLOCK, 0, EDID_BLOCK);
    g_x[EDID_BLOCK] = 0x70;
    sum_fix(g_x + EDID_BLOCK);
    rc = edid_parse_ext(g_x, (int)sizeof(g_x), &e, &c, &tag);
    TEST_ASSERT_EQUAL(1, rc);
    edid_cea_summarize(&c, rc, tag, &s);
    TEST_ASSERT_EQUAL(2, s.status);
    TEST_ASSERT_EQUAL(0x70, s.ext_tag);
    /* a CEA block with a bad checksum: refused */
    tv_block();
    g_x[EDID_BLOCK + 127] ^= 0x55;
    rc = edid_parse_ext(g_x, (int)sizeof(g_x), &e, &c, &tag);
    TEST_ASSERT_EQUAL(1, rc);
    edid_cea_summarize(&c, rc, tag, &s);
    TEST_ASSERT_EQUAL(3, s.status);
    TEST_ASSERT_EQUAL(0, s.best_w);
    /* null arguments do not crash */
    edid_cea_summarize(0, 0, 0, &s);
    TEST_ASSERT_EQUAL(2, s.status);
    edid_cea_summarize(&c, 0, 0, 0);
    TEST_ASSERT_EQUAL(0, edid_cea_is_tv(0, 0));
}

static void test_describe(void) {
    edid_cea_t c;
    edid_info_t e;
    char line[160];
    tv_block();
    TEST_ASSERT_EQUAL(0, edid_parse_ext(g_x, (int)sizeof(g_x), &e, &c, 0));
    TEST_ASSERT(edid_cea_describe(&c, line, (int)sizeof(line)) > 0);
    TEST_ASSERT_EQUAL_STRING("cea ok rev 3 svd 5 best 1920x1080@60 audio 2 hdmi 1.0.0.0 hdr dtd 1", line);
    memset(&c, 0, sizeof(c));
    TEST_ASSERT(edid_cea_describe(&c, line, (int)sizeof(line)) > 0);
    TEST_ASSERT_EQUAL_STRING("cea none", line);
    /* a short buffer is cut, never overrun */
    tv_block();
    TEST_ASSERT_EQUAL(0, edid_cea_parse(g_x + EDID_BLOCK, EDID_BLOCK, &c));
    TEST_ASSERT_EQUAL(11, edid_cea_describe(&c, line, 12));
    TEST_ASSERT_EQUAL_STRING("cea ok rev ", line);
}

static void test_best_without_native(void) {
    static const uint8_t svd[] = { 4, 5, 97, 16 };     /* 720p, 1080i, 2160p60, 1080p */
    edid_cea_t c;
    int w, h, hz;
    base(1);
    cea_begin(3, 0x00);
    blk(2, svd, (int)sizeof(svd));
    cea_end(0);
    TEST_ASSERT_EQUAL(0, edid_cea_parse(g_c, EDID_BLOCK, &c));
    TEST_ASSERT_EQUAL(0, c.dtd_n);
    TEST_ASSERT_EQUAL(0, edid_cea_best(&c, &w, &h, &hz));
    TEST_ASSERT_EQUAL(3840, w);
    TEST_ASSERT_EQUAL(2160, h);
    TEST_ASSERT_EQUAL(60, hz);
    /* only interlaced or unknown codes: nothing to pick */
    {
        static const uint8_t svd2[] = { 5, 20, 120 };
        base(1);
        cea_begin(3, 0x00);
        blk(2, svd2, (int)sizeof(svd2));
        cea_end(0);
        TEST_ASSERT_EQUAL(0, edid_cea_parse(g_c, EDID_BLOCK, &c));
        TEST_ASSERT_EQUAL(-1, edid_cea_best(&c, &w, &h, &hz));
    }
    /* the first DTD when no SVD is native */
    {
        static const uint8_t svd3[] = { 4 };
        base(1);
        cea_begin(3, 0x00);
        blk(2, svd3, 1);
        cea_end(2);
        TEST_ASSERT_EQUAL(0, edid_cea_parse(g_c, EDID_BLOCK, &c));
        TEST_ASSERT_EQUAL(2, c.dtd_n);
        TEST_ASSERT_EQUAL(0, edid_cea_best(&c, &w, &h, &hz));
        TEST_ASSERT_EQUAL(1920, w);
    }
}

static void test_vic_table(void) {
    int w, h, hz, il;
    TEST_ASSERT_EQUAL(0, edid_vic_mode(1, &w, &h, &hz, &il));
    TEST_ASSERT_EQUAL(640, w); TEST_ASSERT_EQUAL(480, h); TEST_ASSERT_EQUAL(60, hz); TEST_ASSERT_EQUAL(0, il);
    TEST_ASSERT_EQUAL(0, edid_vic_mode(5, &w, &h, &hz, &il));
    TEST_ASSERT_EQUAL(1, il);
    TEST_ASSERT_EQUAL(0, edid_vic_mode(19, &w, &h, &hz, 0));
    TEST_ASSERT_EQUAL(1280, w); TEST_ASSERT_EQUAL(50, hz);
    TEST_ASSERT_EQUAL(0, edid_vic_mode(102, &w, &h, &hz, 0));
    TEST_ASSERT_EQUAL(4096, w);
    TEST_ASSERT_EQUAL(0, edid_vic_mode(32, 0, 0, &hz, 0));
    TEST_ASSERT_EQUAL(24, hz);
    TEST_ASSERT_EQUAL(-1, edid_vic_mode(0, &w, &h, &hz, &il));
    TEST_ASSERT_EQUAL(-1, edid_vic_mode(8, &w, &h, &hz, &il));
    TEST_ASSERT_EQUAL(-1, edid_vic_mode(255, &w, &h, &hz, &il));
}

static void test_svd_codes(void) {
    /* 128, 254 and 255 are reserved; 193+ are 8-bit VICs, never native */
    static const uint8_t svd[] = { 128, 0x81, 193, 254, 255, 0xE0 };
    edid_cea_t c;
    base(1);
    cea_begin(3, 0);
    blk(2, svd, (int)sizeof(svd));
    cea_end(0);
    TEST_ASSERT_EQUAL(0, edid_cea_parse(g_c, EDID_BLOCK, &c));
    TEST_ASSERT_EQUAL(3, c.svd_n);
    TEST_ASSERT_EQUAL(1, c.vic[0]);
    TEST_ASSERT_EQUAL(1, c.svd_native[0]);
    TEST_ASSERT_EQUAL(193, c.vic[1]);
    TEST_ASSERT_EQUAL(0, c.svd_native[1]);
    TEST_ASSERT_EQUAL(224, c.vic[2]);
}

static void test_svd_limit(void) {
    uint8_t svd[30];
    edid_cea_t c;
    int i;
    for (i = 0; i < 30; i++) svd[i] = (uint8_t)(1 + i);
    base(1);
    cea_begin(3, 0);
    blk(2, svd, 30);
    cea_end(0);
    TEST_ASSERT_EQUAL(0, edid_cea_parse(g_c, EDID_BLOCK, &c));
    TEST_ASSERT_EQUAL(EDID_CEA_SVD, c.svd_n);
    TEST_ASSERT_EQUAL(30, c.svd_total);
    TEST_ASSERT_EQUAL(16, c.vic[EDID_CEA_SVD - 1]);
}

static void test_errors(void) {
    edid_cea_t c;
    edid_info_t e;
    int tag = 0;
    TEST_ASSERT_EQUAL(-3, edid_cea_parse(0, EDID_BLOCK, &c));
    TEST_ASSERT_EQUAL(-3, edid_cea_parse(g_x, 64, &c));
    TEST_ASSERT_EQUAL(-3, edid_cea_parse(g_x, EDID_BLOCK, 0));
    tv_block();
    g_c[60] ^= 1;
    TEST_ASSERT_EQUAL(-2, edid_cea_parse(g_c, EDID_BLOCK, &c));
    TEST_ASSERT_EQUAL(0, c.ok);
    /* a data block running into the DTD area */
    tv_block();
    g_c[2] = 10;     /* the 6 byte SVD block at 4 ends at 11 */
    sum_fix(g_c);
    TEST_ASSERT_EQUAL(-4, edid_cea_parse(g_c, EDID_BLOCK, &c));
    /* offsets 1..3 are not a layout */
    tv_block();
    g_c[2] = 2;
    sum_fix(g_c);
    TEST_ASSERT_EQUAL(-4, edid_cea_parse(g_c, EDID_BLOCK, &c));
    /* a DisplayID extension (0x70) is reported, not parsed */
    base(1);
    memset(g_x + EDID_BLOCK, 0, EDID_BLOCK);
    g_x[EDID_BLOCK] = 0x70;
    sum_fix(g_x + EDID_BLOCK);
    TEST_ASSERT_EQUAL(-1, edid_cea_parse(g_x + EDID_BLOCK, EDID_BLOCK, &c));
    TEST_ASSERT_EQUAL(1, edid_parse_ext(g_x, (int)sizeof(g_x), &e, &c, &tag));
    TEST_ASSERT_EQUAL(0x70, tag);
    TEST_ASSERT_EQUAL(0, c.ok);
    /* no extension, or the buffer holds only the base block */
    base(0);
    TEST_ASSERT_EQUAL(-1, edid_parse_ext(g_x, (int)sizeof(g_x), &e, &c, &tag));
    tv_block();
    TEST_ASSERT_EQUAL(-1, edid_parse_ext(g_x, EDID_BLOCK, &e, &c, &tag));
    TEST_ASSERT_EQUAL(1, e.ext_blocks);
    /* a bad base block stops before the extension */
    tv_block();
    g_x[0] = 1;
    TEST_ASSERT_EQUAL(-1, edid_parse_ext(g_x, (int)sizeof(g_x), &e, &c, &tag));
}

static void test_rev1_and_rev2(void) {
    edid_cea_t c;
    /* revision 1: DTDs only, byte 3 is reserved */
    base(1);
    cea_begin(1, 0xF0);
    g_c[4] = 0x55;   /* would be a data block header in rev 3 */
    g_pos = 4;
    cea_end(1);
    TEST_ASSERT_EQUAL(0, edid_cea_parse(g_c, EDID_BLOCK, &c));
    TEST_ASSERT_EQUAL(0, c.underscan);
    TEST_ASSERT_EQUAL(0, c.svd_n);
    TEST_ASSERT_EQUAL(1, c.dtd_n);
    /* revision 2: flags but no data blocks */
    base(1);
    cea_begin(2, 0x42);
    cea_end(0);
    TEST_ASSERT_EQUAL(0, edid_cea_parse(g_c, EDID_BLOCK, &c));
    TEST_ASSERT_EQUAL(1, c.basic_audio);
    TEST_ASSERT_EQUAL(2, c.native_dtds);
    TEST_ASSERT_EQUAL(0, c.dtd_n);
    /* offset 0: no DTD, no data block */
    base(1);
    cea_begin(3, 0);
    g_c[2] = 0;
    sum_fix(g_c);
    TEST_ASSERT_EQUAL(0, edid_cea_parse(g_c, EDID_BLOCK, &c));
    TEST_ASSERT_EQUAL(0, c.dtd_n);
    TEST_ASSERT_EQUAL(0, c.svd_n);
}

static void test_unknown_blocks(void) {
    static const uint8_t vs[] = { 0x01, 0x02, 0x03, 0x04 };   /* some vendor */
    static const uint8_t ext[] = { 0x20, 0x01 };              /* extended tag 32 */
    static const uint8_t res[] = { 0x00 };
    static const uint8_t sad[] = { 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x09, 0x7F, 0x07, 0x01 };
    edid_cea_t c;
    base(1);
    cea_begin(3, 0);
    blk(3, vs, (int)sizeof(vs));
    blk(7, ext, (int)sizeof(ext));
    blk(5, res, 1);
    blk(6, res, 1);
    blk(4, res, 0);
    blk(1, sad, (int)sizeof(sad));
    cea_end(0);
    TEST_ASSERT_EQUAL(0, edid_cea_parse(g_c, EDID_BLOCK, &c));
    TEST_ASSERT_EQUAL(5, c.unknown_blocks);
    /* reserved format 0 and extended 15 are skipped; the trailing byte is not a SAD */
    TEST_ASSERT_EQUAL(1, c.sad_n);
    TEST_ASSERT_EQUAL(EDID_AUDIO_LPCM, c.sad_format[0]);
    TEST_ASSERT_EQUAL(0x7F, c.sad_rates[0]);
}

int main(void) {
    unity_init();
    RUN_TEST(test_tv_block);
    RUN_TEST(test_summary_tv);
    RUN_TEST(test_summary_monitor_and_sources);
    RUN_TEST(test_summary_status);
    RUN_TEST(test_describe);
    RUN_TEST(test_best_without_native);
    RUN_TEST(test_vic_table);
    RUN_TEST(test_svd_codes);
    RUN_TEST(test_svd_limit);
    RUN_TEST(test_errors);
    RUN_TEST(test_rev1_and_rev2);
    RUN_TEST(test_unknown_blocks);
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
