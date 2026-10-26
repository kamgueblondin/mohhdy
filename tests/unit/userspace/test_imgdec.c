/* Lot 45: imgdec (Lot 47: PNG 1/2/4/16-bit + Adam7, JPEG baseline) (BMP 24/32, PNG, binary PNM, bounded inflate). */
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

/* ---- Lot 47 (US-G03) ---- */
static int gray_ref(int x, int y, int depth) {
    unsigned v = (unsigned)(((x + 2 * y) * 7) % (1 << depth));
    return depth == 16 ? (int)(v >> 8) : (int)((v * 255U) / ((1U << depth) - 1U));
}

static void test_png_16bit_and_interlaced_rgb(void) {
    check_rgb_image(img_mire_rgb16_png, sizeof(img_mire_rgb16_png), IMGDEC_PNG);
    check_rgb_image(img_mire_rgba16_i_png, sizeof(img_mire_rgba16_i_png), IMGDEC_PNG);
    check_rgb_image(img_mire_rgb_i_png, sizeof(img_mire_rgb_i_png), IMGDEC_PNG);
    {
        imgdec_t img;
        TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(img_mire_rgba16_i_png, sizeof(img_mire_rgba16_i_png), &img));
        TEST_ASSERT_EQUAL(16, img.depth);
        TEST_ASSERT_EQUAL(1, img.interlace);
        TEST_ASSERT_EQUAL(64, img.bpp);
    }
}

static void check_gray(const unsigned char *buf, int n, int depth) {
    imgdec_t img;
    unsigned char c[3];
    int x, y;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(buf, n, g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(depth, img.depth);
    for (y = 0; y < IMGS_H; y++)
        for (x = 0; x < IMGS_W; x++) {
            imgdec_pixel(&img, x, y, c);
            TEST_ASSERT_EQUAL(gray_ref(x, y, depth), c[0]);
        }
}

static void test_png_low_and_high_gray_depths(void) {
    check_gray(img_mire_g1_png, sizeof(img_mire_g1_png), 1);
    check_gray(img_mire_g2_i_png, sizeof(img_mire_g2_i_png), 2);
    check_gray(img_mire_g4_png, sizeof(img_mire_g4_png), 4);
    check_gray(img_mire_g16_png, sizeof(img_mire_g16_png), 16);
}

static void test_png_palette2_interlaced_and_ga16(void) {
    imgdec_t img;
    unsigned char c[3];
    int x, y;
    static const unsigned char pal[4][3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255}};
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(img_mire_pal2_i_png, sizeof(img_mire_pal2_i_png), g_scratch, sizeof(g_scratch), &img));
    for (y = 0; y < IMGS_H; y++)
        for (x = 0; x < IMGS_W; x++) {
            int k = (x / 6 + y / 8) % 4;
            imgdec_pixel(&img, x, y, c);
            TEST_ASSERT_EQUAL(pal[k][0], c[0]); TEST_ASSERT_EQUAL(pal[k][1], c[1]); TEST_ASSERT_EQUAL(pal[k][2], c[2]);
        }
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(img_mire_ga16_png, sizeof(img_mire_ga16_png), g_scratch, sizeof(g_scratch), &img));
    imgdec_pixel(&img, 7, 3, c);
    TEST_ASSERT_EQUAL(35, c[0]);
}

static void test_png_scratch_bound_new_variants(void) {
    imgdec_t img;
    unsigned need;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(img_mire_rgba16_i_png, sizeof(img_mire_rgba16_i_png), &img));
    need = imgdec_scratch_need(&img);
    TEST_ASSERT_TRUE(need > (unsigned)(IMGS_W * IMGS_H * 4));
    TEST_ASSERT_EQUAL(IMGDEC_E_TOOBIG, imgdec_open(img_mire_rgba16_i_png, sizeof(img_mire_rgba16_i_png), g_scratch, need - 1, &img));
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(img_mire_rgba16_i_png, sizeof(img_mire_rgba16_i_png), g_scratch, need, &img));
}

static int absd(int a, int b) { return a > b ? a - b : b - a; }

/* JPEG is lossy: reference pattern within tol per channel, mean within mtol/10 */
static void check_jpeg(const unsigned char *buf, int n, int ch, int tol, int mtol10) {
    imgdec_t img;
    int x, y, k;
    long sum = 0;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(buf, n, g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(IMGDEC_JPEG, img.fmt);
    TEST_ASSERT_EQUAL(IMGS_W, img.w);
    TEST_ASSERT_EQUAL(IMGS_H, img.h);
    TEST_ASSERT_EQUAL(ch, img.channels);
    for (y = 0; y < IMGS_H; y++)
        for (x = 0; x < IMGS_W; x++) {
            unsigned char c[3];
            int r = (x * 5) & 255, g = (y * 8) & 255, b = ((x + y) * 3) & 255, e[3];
            imgdec_pixel(&img, x, y, c);
            if (ch == 1) { int l = (r * 299 + g * 587 + b * 114 + 500) / 1000; e[0] = e[1] = e[2] = absd(c[0], l); }
            else { e[0] = absd(c[0], r); e[1] = absd(c[1], g); e[2] = absd(c[2], b); }
            for (k = 0; k < 3; k++) { TEST_ASSERT_TRUE(e[k] <= tol); sum += e[k]; }
        }
    TEST_ASSERT_TRUE(sum * 10 <= (long)mtol10 * 3 * IMGS_W * IMGS_H);
}

static void test_jpeg_baseline(void) {
    check_jpeg(img_mire444_jpg, sizeof(img_mire444_jpg), 3, 8, 15);
    check_jpeg(img_mire420_jpg, sizeof(img_mire420_jpg), 3, 14, 30);
    check_jpeg(img_mire_gray_jpg, sizeof(img_mire_gray_jpg), 1, 4, 5);
    check_jpeg(img_mire_rst_jpg, sizeof(img_mire_rst_jpg), 3, 14, 30);   /* DRI + RSTn */
    TEST_ASSERT_EQUAL(0, strcmp("jpeg", imgdec_fmt_name(IMGDEC_JPEG)));
}

static void test_jpeg_refusals_and_bounds(void) {
    imgdec_t img;
    static unsigned char bad[4096];
    int i;
    TEST_ASSERT_EQUAL(IMGDEC_E_UNSUP, imgdec_open(img_mire_prog_jpg, sizeof(img_mire_prog_jpg), g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(IMGDEC_JPEG, img.fmt);
    TEST_ASSERT_EQUAL(IMGS_W, img.w);         /* probe still reports the size */
    TEST_ASSERT_EQUAL(IMGDEC_E_TOOBIG, imgdec_open(img_mire444_jpg, sizeof(img_mire444_jpg), g_scratch, IMGS_W * IMGS_H * 3 - 1, &img));
    TEST_ASSERT_TRUE(imgdec_open(img_mire444_jpg, 100, g_scratch, sizeof(g_scratch), &img) < 0);
    /* corrupt entropy data: must stay bounded and never crash */
    for (i = 0; i < 64; i++) {
        memcpy(bad, img_mire444_jpg, sizeof(img_mire444_jpg));
        bad[sizeof(img_mire444_jpg) - 40 - i * 7] ^= (unsigned char)(0x5A + i);
        (void)imgdec_open(bad, sizeof(img_mire444_jpg), g_scratch, sizeof(g_scratch), &img);
    }
    /* truncated scan still terminates (zero-fill after the end) */
    (void)imgdec_open(img_mire420_jpg, sizeof(img_mire420_jpg) / 2, g_scratch, sizeof(g_scratch), &img);
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
    RUN_TEST(test_png_16bit_and_interlaced_rgb);
    RUN_TEST(test_png_low_and_high_gray_depths);
    RUN_TEST(test_png_palette2_interlaced_and_ga16);
    RUN_TEST(test_png_scratch_bound_new_variants);
    RUN_TEST(test_jpeg_baseline);
    RUN_TEST(test_jpeg_refusals_and_bounds);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
