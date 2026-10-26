/* Lot 45: imgdec (BMP 24/32, PNG, binary PNM, bounded inflate). */
#include "../../framework/unity.h"
#include "imgdec.h"
#include "imgdec_samples.h"
#include <string.h>

static unsigned char g_scratch[64 * 1024];
static unsigned char g_rgb[320 * 240 * 3];

static void expect_px(const imgdec_t *img, int x, int y) {
    unsigned char c[3];
    imgdec_pixel(img, x, y, c);
    TEST_ASSERT_EQUAL((x * 5) & 255, c[0]);
    TEST_ASSERT_EQUAL((y * 8) & 255, c[1]);
    TEST_ASSERT_EQUAL(((x + y) * 3) & 255, c[2]);
}

static void check_rgb_image(const unsigned char *buf, int n, int fmt) {
    imgdec_t img;
    int x, y;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(buf, n, g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(fmt, img.fmt);
    TEST_ASSERT_EQUAL(IMGS_W, img.w);
    TEST_ASSERT_EQUAL(IMGS_H, img.h);
    for (y = 0; y < IMGS_H; y += 3) for (x = 0; x < IMGS_W; x += 5) expect_px(&img, x, y);
    expect_px(&img, IMGS_W - 1, IMGS_H - 1);
}

static void test_bmp24(void) { check_rgb_image(img_mire24_bmp, sizeof(img_mire24_bmp), IMGDEC_BMP); }
static void test_bmp32_top_down(void) { check_rgb_image(img_mire32_bmp, sizeof(img_mire32_bmp), IMGDEC_BMP); }
static void test_png_rgb_split_idat(void) { check_rgb_image(img_mire_png, sizeof(img_mire_png), IMGDEC_PNG); }
static void test_png_rgba(void) { check_rgb_image(img_mire_rgba_png, sizeof(img_mire_rgba_png), IMGDEC_PNG); }

static void test_png_palette_and_gray(void) {
    imgdec_t img;
    unsigned char c[3];
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(img_mire_pal_png, sizeof(img_mire_pal_png), g_scratch, sizeof(g_scratch), &img));
    imgdec_pixel(&img, 0, 0, c);
    TEST_ASSERT_EQUAL(255, c[0]); TEST_ASSERT_EQUAL(0, c[1]);
    imgdec_pixel(&img, 6, 0, c);          /* index 1: green */
    TEST_ASSERT_EQUAL(0, c[0]); TEST_ASSERT_EQUAL(255, c[1]);
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(img_mire_gray_png, sizeof(img_mire_gray_png), g_scratch, sizeof(g_scratch), &img));
    imgdec_pixel(&img, 7, 3, c);
    TEST_ASSERT_EQUAL(35, c[0]); TEST_ASSERT_EQUAL(35, c[2]);
}

static void test_bounds_and_errors(void) {
    imgdec_t img;
    static unsigned char bad[2048];
    TEST_ASSERT_EQUAL(IMGDEC_E_TOOBIG, imgdec_open(img_mire_png, sizeof(img_mire_png), g_scratch, 100, &img));
    TEST_ASSERT_EQUAL(IMGDEC_E_TRUNC, imgdec_open(img_mire24_bmp, 200, g_scratch, sizeof(g_scratch), &img));
    memcpy(bad, img_mire_png, sizeof(img_mire_png));
    bad[60] ^= 0x55;                       /* inside IDAT: CRC catches it */
    TEST_ASSERT_EQUAL(IMGDEC_E_CRC, imgdec_open(bad, sizeof(img_mire_png), g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(IMGDEC_E_FORMAT, imgdec_open((const unsigned char *)"hello", 5, g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(IMGDEC_E_TRUNC, imgdec_open(img_mire_png, 40, g_scratch, sizeof(g_scratch), &img));
}

static void test_inflate_fixed_and_bound(void) {
    static unsigned char out[1024];
    TEST_ASSERT_EQUAL(ZFIXED_LEN, imgdec_inflate_zlib(zfixed, sizeof(zfixed), out, sizeof(out)));
    TEST_ASSERT_EQUAL(0, memcmp(out, ZFIXED_TEXT, 28));
    TEST_ASSERT_EQUAL(IMGDEC_E_TOOBIG, imgdec_inflate_zlib(zfixed, sizeof(zfixed), out, 100));
}

static void test_render_fit_zoom_pan(void) {
    imgdec_t img;
    int w, h;
    unsigned char c[3];
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(img_mire24_bmp, sizeof(img_mire24_bmp), g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(0, imgdec_render(&img, g_rgb, 320, 240, 0, 0, 0, &w, &h));
    TEST_ASSERT_EQUAL(48, w); TEST_ASSERT_EQUAL(32, h);       /* fit never upscales */
    TEST_ASSERT_EQUAL(0, imgdec_render(&img, g_rgb, 24, 240, 0, 0, 0, &w, &h));
    TEST_ASSERT_EQUAL(24, w); TEST_ASSERT_EQUAL(16, h);       /* aspect kept */
    TEST_ASSERT_EQUAL(0, imgdec_render(&img, g_rgb, 320, 240, 400, 0, 0, &w, &h));
    TEST_ASSERT_EQUAL(192, w); TEST_ASSERT_EQUAL(128, h);
    TEST_ASSERT_EQUAL(0, imgdec_render(&img, g_rgb, 40, 40, 200, 10, 4, &w, &h));
    TEST_ASSERT_EQUAL(40, w);
    imgdec_pixel(&img, 10, 4, c);
    TEST_ASSERT_EQUAL(c[0], g_rgb[0]); TEST_ASSERT_EQUAL(c[1], g_rgb[1]);
}

int main(void) {
    unity_init();
    RUN_TEST(test_bmp24);
    RUN_TEST(test_bmp32_top_down);
    RUN_TEST(test_png_rgb_split_idat);
    RUN_TEST(test_png_rgba);
    RUN_TEST(test_png_palette_and_gray);
    RUN_TEST(test_bounds_and_errors);
    RUN_TEST(test_inflate_fixed_and_bound);
    RUN_TEST(test_render_fit_zoom_pan);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
