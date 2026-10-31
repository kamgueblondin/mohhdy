/* Lot 69 (US-080): EDID base block parser (kernel/edid.c). The blocks
 * are built like the one QEMU's std VGA exposes (vendor RHT, 100 dpi). */
#include "../../framework/unity.h"
#include "kernel/edid.h"
#include <string.h>

static uint8_t g_b[EDID_BLOCK];

static void fix_sum(uint8_t *b) {
    unsigned s = 0;
    int i;
    for (i = 0; i < 127; i++) s += b[i];
    b[127] = (uint8_t)((256u - (s & 0xFFu)) & 0xFFu);
}

/* QEMU-like block: RHT, w x h pixels, mm_w x mm_h, cm in the basic block */
static void make_block(int w, int h, int mm_w, int mm_h, int cm_w, int cm_h) {
    static const uint8_t hdr[8] = { 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0 };
    uint8_t *d = g_b + 54;
    uint16_t id = (uint16_t)((('R' - 'A' + 1) << 10) | (('H' - 'A' + 1) << 5) | ('T' - 'A' + 1));
    memset(g_b, 0, sizeof(g_b));
    memcpy(g_b, hdr, 8);
    g_b[8] = (uint8_t)(id >> 8); g_b[9] = (uint8_t)id;
    g_b[10] = 0x34; g_b[11] = 0x12;
    g_b[17] = 24;       /* 2014 */
    g_b[18] = 1; g_b[19] = 4;
    g_b[21] = (uint8_t)cm_w; g_b[22] = (uint8_t)cm_h;
    d[0] = 0x10; d[1] = 0x27;              /* 100 MHz clock: a timing descriptor */
    d[2] = (uint8_t)(w & 0xFF); d[4] = (uint8_t)(((w >> 8) & 0x0F) << 4);
    d[5] = (uint8_t)(h & 0xFF); d[7] = (uint8_t)(((h >> 8) & 0x0F) << 4);
    d[12] = (uint8_t)(mm_w & 0xFF); d[13] = (uint8_t)(mm_h & 0xFF);
    d[14] = (uint8_t)((((mm_w >> 8) & 0x0F) << 4) | ((mm_h >> 8) & 0x0F));
    fix_sum(g_b);
}

static void test_qemu_like_block(void) {
    edid_info_t e;
    make_block(1024, 768, 260, 195, 26, 20);
    TEST_ASSERT_EQUAL(0, edid_parse(g_b, EDID_BLOCK, &e));
    TEST_ASSERT_EQUAL(1, e.ok);
    TEST_ASSERT_EQUAL_STRING("RHT", e.mfg);
    TEST_ASSERT_EQUAL(0x1234, e.product);
    TEST_ASSERT_EQUAL(2014, e.year);
    TEST_ASSERT_EQUAL(1, e.version); TEST_ASSERT_EQUAL(4, e.revision);
    TEST_ASSERT_EQUAL(1024, e.native_w); TEST_ASSERT_EQUAL(768, e.native_h);
    TEST_ASSERT_EQUAL(260, e.size_mm_w); TEST_ASSERT_EQUAL(195, e.size_mm_h);
    TEST_ASSERT_EQUAL(128, e.diag_tenths);   /* 325 mm = 12.8 in */
}

static void test_large_values_use_high_nibbles(void) {
    edid_info_t e;
    make_block(3840, 2160, 1210, 680, 121, 68);
    TEST_ASSERT_EQUAL(0, edid_parse(g_b, EDID_BLOCK, &e));
    TEST_ASSERT_EQUAL(3840, e.native_w); TEST_ASSERT_EQUAL(2160, e.native_h);
    TEST_ASSERT_EQUAL(1210, e.mm_w); TEST_ASSERT_EQUAL(680, e.mm_h);
    TEST_ASSERT_EQUAL(546, e.diag_tenths);   /* a 55 in TV (54.6) */
}

static void test_cm_fallback_and_unknown(void) {
    edid_info_t e;
    make_block(1920, 1080, 0, 0, 53, 30);
    TEST_ASSERT_EQUAL(0, edid_parse(g_b, EDID_BLOCK, &e));
    TEST_ASSERT_EQUAL(530, e.size_mm_w); TEST_ASSERT_EQUAL(300, e.size_mm_h);
    TEST_ASSERT_EQUAL(240, e.diag_tenths);
    make_block(1920, 1080, 1, 1, 0, 0);       /* projector placeholder */
    TEST_ASSERT_EQUAL(0, edid_parse(g_b, EDID_BLOCK, &e));
    TEST_ASSERT_EQUAL(0, e.size_mm_w);
    TEST_ASSERT_EQUAL(0, e.diag_tenths);
    make_block(1920, 1080, 0, 0, 16, 0);      /* aspect ratio only */
    TEST_ASSERT_EQUAL(0, edid_parse(g_b, EDID_BLOCK, &e));
    TEST_ASSERT_EQUAL(0, e.cm_w);
    TEST_ASSERT_EQUAL(0, e.size_mm_w);
}

static void test_refusals(void) {
    edid_info_t e;
    make_block(1024, 768, 260, 195, 26, 20);
    g_b[1] = 0x00;
    TEST_ASSERT_EQUAL(-1, edid_parse(g_b, EDID_BLOCK, &e));
    TEST_ASSERT_EQUAL(0, e.ok);
    make_block(1024, 768, 260, 195, 26, 20);
    g_b[60] ^= 0x55;
    TEST_ASSERT_EQUAL(-2, edid_parse(g_b, EDID_BLOCK, &e));
    TEST_ASSERT_EQUAL(0, e.ok);
    TEST_ASSERT_EQUAL(-3, edid_parse(g_b, 64, &e));
    TEST_ASSERT_EQUAL(-3, edid_parse(0, EDID_BLOCK, &e));
    TEST_ASSERT_EQUAL(-3, edid_parse(g_b, EDID_BLOCK, 0));
}

static void test_descriptor_skips_display_descriptors(void) {
    edid_info_t e;
    make_block(1280, 800, 286, 179, 29, 18);
    /* move the timing to slot 2, slot 1 becomes a monitor name descriptor */
    memcpy(g_b + 72, g_b + 54, 18);
    memset(g_b + 54, 0, 18);
    g_b[57] = 0xFC;
    memcpy(g_b + 59, "QEMU Monitor", 12);
    fix_sum(g_b);
    TEST_ASSERT_EQUAL(0, edid_parse(g_b, EDID_BLOCK, &e));
    TEST_ASSERT_EQUAL(1280, e.native_w);
    TEST_ASSERT_EQUAL(286, e.size_mm_w);
}

static void test_describe(void) {
    edid_info_t e;
    char out[96];
    make_block(1024, 768, 260, 195, 26, 20);
    (void)edid_parse(g_b, EDID_BLOCK, &e);
    TEST_ASSERT_TRUE(edid_describe(&e, out, (int)sizeof(out)) > 0);
    TEST_ASSERT_EQUAL_STRING("edid ok RHT 1024x768 260x195mm 12.8in", out);
    memset(&e, 0, sizeof(e));
    edid_describe(&e, out, (int)sizeof(out));
    TEST_ASSERT_EQUAL_STRING("edid absent", out);
    make_block(800, 600, 0, 0, 0, 0);
    (void)edid_parse(g_b, EDID_BLOCK, &e);
    edid_describe(&e, out, (int)sizeof(out));
    TEST_ASSERT_EQUAL_STRING("edid ok RHT 800x600 size unknown", out);
    edid_describe(&e, out, 8);
    TEST_ASSERT_EQUAL(7, (int)strlen(out));
}

static void test_isqrt(void) {
    TEST_ASSERT_EQUAL(0, edid_isqrt(0));
    TEST_ASSERT_EQUAL(1, edid_isqrt(3));
    TEST_ASSERT_EQUAL(325, edid_isqrt(105625));
    TEST_ASSERT_EQUAL(65535, edid_isqrt(0xFFFFFFFFu));
}

int main(void) {
    unity_init();
    RUN_TEST(test_qemu_like_block);
    RUN_TEST(test_large_values_use_high_nibbles);
    RUN_TEST(test_cm_fallback_and_unknown);
    RUN_TEST(test_refusals);
    RUN_TEST(test_descriptor_skips_display_descriptors);
    RUN_TEST(test_describe);
    RUN_TEST(test_isqrt);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
