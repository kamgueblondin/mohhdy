/* Lot 87 (US-080): the CEA line of display-info and the TV scale hint,
 * from an os_display_info_t filled as the kernel fills it. */
#include "../../framework/unity.h"
#include "plat_form.h"
#include <string.h>

static os_display_info_t g_d;
static char g_o[512];

static void tv(void) {
    static const uint8_t vics[8] = { 16, 4, 3, 31, 97, 0, 0, 0 };
    memset(&g_d, 0, sizeof(g_d));
    g_d.fb_w = 1280; g_d.fb_h = 800;
    g_d.native_w = 1024; g_d.native_h = 768;
    g_d.mm_w = 600;
    g_d.cea_status = 0;
    g_d.cea_rev = 3;
    g_d.cea_flags = OS_DISPLAY_CEA_UNDERSCAN | OS_DISPLAY_CEA_AUDIO | OS_DISPLAY_CEA_YCC444 |
                    OS_DISPLAY_CEA_HDMI | OS_DISPLAY_CEA_HF | OS_DISPLAY_CEA_HDR;
    g_d.cea_svd_n = 5;
    memcpy(g_d.cea_vic, vics, 8);
    g_d.cea_native_vic = 16;
    g_d.cea_sad_n = 2; g_d.cea_sad_max_ch = 6;
    g_d.cea_dtd_n = 1;
    g_d.cea_y420_n = 2;
    g_d.cea_best_w = 1920; g_d.cea_best_h = 1080; g_d.cea_best_hz = 60; g_d.cea_best_from = 1;
    g_d.cea_phys_addr = 0x1000;
    g_d.cea_tmds_mhz = 600;
    g_d.cea_tv = 1;
}

static void test_tv_line(void) {
    int n;
    tv();
    n = plat_form_cea_line(&g_d, g_o, (int)sizeof(g_o));
    TEST_ASSERT_EQUAL((int)strlen(g_o), n);
    TEST_ASSERT_EQUAL_STRING("display cea rev 3 svd 5 vics 16,4,3,31,97 best 1920x1080@60 (native) audio 6ch "
                             "hdmi 1.0.0.0 hf tmds 600 hdr ycc444 y420 2 underscan dtd 1 sink tv\n", g_o);
    /* a 1080p TV: 2x */
    TEST_ASSERT_EQUAL(8, plat_form_tv_scale_q(&g_d));
    /* 4K native: 3x */
    g_d.cea_best_w = 3840; g_d.cea_best_h = 2160;
    TEST_ASSERT_EQUAL(12, plat_form_tv_scale_q(&g_d));
    /* not native: the base block's preferred timing (1024) decides: 1.5x */
    g_d.cea_best_from = 3;
    TEST_ASSERT_EQUAL(6, plat_form_tv_scale_q(&g_d));
}

static void test_monitor_line(void) {
    tv();
    g_d.cea_tv = 0;
    g_d.cea_flags = OS_DISPLAY_CEA_AUDIO;
    g_d.cea_sad_n = 0;
    g_d.cea_svd_n = 10;
    g_d.cea_y420_n = 0;
    g_d.cea_dtd_n = 0;
    g_d.cea_tmds_mhz = 0;
    g_d.cea_best_from = 3; g_d.cea_best_w = 4096; g_d.cea_best_h = 2160; g_d.cea_best_hz = 50;
    plat_form_cea_line(&g_d, g_o, (int)sizeof(g_o));
    TEST_ASSERT_EQUAL_STRING("display cea rev 3 svd 10 vics 16,4,3,31,97,0,0,0,... best 4096x2160@50 (largest vic) "
                             "audio basic sink monitor\n", g_o);
    TEST_ASSERT_EQUAL(0, plat_form_tv_scale_q(&g_d));
    /* no audio at all, best from a DTD */
    g_d.cea_flags = 0;
    g_d.cea_svd_n = 0;
    g_d.cea_best_from = 2; g_d.cea_best_w = 1920; g_d.cea_best_h = 1080; g_d.cea_best_hz = 60;
    plat_form_cea_line(&g_d, g_o, (int)sizeof(g_o));
    TEST_ASSERT_EQUAL_STRING("display cea rev 3 svd 0 best 1920x1080@60 (dtd) audio 0 sink monitor\n", g_o);
}

static void test_status_lines(void) {
    memset(&g_d, 0, sizeof(g_d));
    g_d.cea_status = 1;
    plat_form_cea_line(&g_d, g_o, (int)sizeof(g_o));
    TEST_ASSERT_EQUAL_STRING("display cea none\n", g_o);
    g_d.cea_status = 2;
    g_d.cea_ext_tag = 0x70;
    plat_form_cea_line(&g_d, g_o, (int)sizeof(g_o));
    TEST_ASSERT_EQUAL_STRING("display cea not-cea tag 0x70\n", g_o);
    g_d.cea_status = 3;
    plat_form_cea_line(&g_d, g_o, (int)sizeof(g_o));
    TEST_ASSERT_EQUAL_STRING("display cea refused\n", g_o);
    TEST_ASSERT_EQUAL(0, plat_form_tv_scale_q(&g_d));
    /* a TV flag without a parsed block is ignored */
    g_d.cea_tv = 1;
    TEST_ASSERT_EQUAL(0, plat_form_tv_scale_q(&g_d));
    TEST_ASSERT_EQUAL(0, plat_form_tv_scale_q(0));
    TEST_ASSERT_EQUAL(0, plat_form_cea_line(0, g_o, (int)sizeof(g_o)));
    /* a short buffer truncates, never overflows */
    tv();
    memset(g_o, 'Z', sizeof(g_o));
    plat_form_cea_line(&g_d, g_o, 20);
    TEST_ASSERT(strlen(g_o) < 20);
    TEST_ASSERT_EQUAL('Z', g_o[25]);
}

int main(void) {
    unity_init();
    RUN_TEST(test_tv_line);
    RUN_TEST(test_monitor_line);
    RUN_TEST(test_status_lines);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
