/* Lot 45: imgdec (Lot 47: PNG 1/2/4/16-bit + Adam7, JPEG baseline) (BMP 24/32, PNG, binary PNM, bounded inflate). */
#include "../../framework/unity.h"
#include "imgdec.h"
#include "imgdec_samples.h"
#include "imgdec_gif_samples.h"
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
    check_jpeg(img_mire_prog_jpg, sizeof(img_mire_prog_jpg), 3, 14, 30); /* SOF2, same 4:2:0 mire */
    TEST_ASSERT_EQUAL(0, strcmp("jpeg", imgdec_fmt_name(IMGDEC_JPEG)));
}

static void test_jpeg_refusals_and_bounds(void) {
    imgdec_t img;
    static unsigned char bad[4096];
    /* arithmetic SOF9: still refused, size reported */
    static const unsigned char arith[] = {
        0xFF, 0xD8, 0xFF, 0xC9, 0x00, 0x0B, 8, 0x00, 8, 0x00, 8, 1, 1, 0x11, 0, 0xFF, 0xD9
    };
    unsigned need;
    int i;
    TEST_ASSERT_EQUAL(IMGDEC_E_UNSUP, imgdec_open(arith, sizeof(arith), g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(IMGDEC_JPEG, img.fmt);
    TEST_ASSERT_EQUAL(8, img.w);
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(img_mire_prog_jpg, sizeof(img_mire_prog_jpg), &img));
    TEST_ASSERT_EQUAL(1, img.jpeg_prog);
    need = imgdec_scratch_need(&img);
    TEST_ASSERT_TRUE(need > (unsigned)(IMGS_W * IMGS_H * 3));   /* pixels plus coefficient blocks */
    TEST_ASSERT_EQUAL(IMGDEC_E_TOOBIG, imgdec_open(img_mire_prog_jpg, sizeof(img_mire_prog_jpg), g_scratch, need - 1, &img));
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



/* Lot 72: scaled JPEG decode = point samples of the full decode */
static void jpeg_scaled_matches(const unsigned char *buf, int n) {
    static unsigned char full[64 * 1024], sc[64 * 1024];
    imgdec_t a, b;
    int shift, x, y;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(buf, n, full, sizeof(full), &a));
    for (shift = 1; shift <= 3; shift++) {
        TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(buf, n, &b));
        TEST_ASSERT_TRUE(imgdec_scratch_need_scaled(&b, shift) < imgdec_scratch_need(&b));
        TEST_ASSERT_EQUAL(IMGDEC_E_TOOBIG, imgdec_open_scaled(buf, n, sc, imgdec_scratch_need_scaled(&b, shift) - 1, shift, &b));
        TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open_scaled(buf, n, sc, sizeof(sc), shift, &b));
        TEST_ASSERT_EQUAL((a.w + (1 << shift) - 1) >> shift, b.w);
        TEST_ASSERT_EQUAL((a.h + (1 << shift) - 1) >> shift, b.h);
        TEST_ASSERT_EQUAL(a.w, b.full_w);
        TEST_ASSERT_EQUAL(shift, b.scale_shift);
        for (y = 0; y < b.h; y++)
            for (x = 0; x < b.w; x++) {
                unsigned char p[3], q[3];
                imgdec_pixel(&a, x << shift, y << shift, p);
                imgdec_pixel(&b, x, y, q);
                TEST_ASSERT_EQUAL(p[0], q[0]); TEST_ASSERT_EQUAL(p[1], q[1]); TEST_ASSERT_EQUAL(p[2], q[2]);
            }
    }
}

static void test_jpeg_scaled_decode(void) {
    jpeg_scaled_matches(img_mire444_jpg, sizeof(img_mire444_jpg));
    jpeg_scaled_matches(img_mire420_jpg, sizeof(img_mire420_jpg));
    jpeg_scaled_matches(img_mire_gray_jpg, sizeof(img_mire_gray_jpg));
    jpeg_scaled_matches(img_mire_rst_jpg, sizeof(img_mire_rst_jpg));
    jpeg_scaled_matches(img_mire_prog_jpg, sizeof(img_mire_prog_jpg));
}

/* ---- Lot 72: GIF ------------------------------------------------------ */
static void gif_expect(const imgdec_t *img, int x, int y, int shift) {
    unsigned char c[3];
    int k = (x + 2 * y + shift) % 16;
    imgdec_pixel(img, x, y, c);
    TEST_ASSERT_EQUAL(k * 16, c[0]);
    TEST_ASSERT_EQUAL(255 - k * 16, c[1]);
    TEST_ASSERT_EQUAL((k * 37) & 255, c[2]);
}

static void gif_check_all(const unsigned char *buf, int n, int w, int h) {
    imgdec_t img;
    int x, y;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(buf, n, &img));
    TEST_ASSERT_EQUAL(IMGDEC_GIF, img.fmt);
    TEST_ASSERT_TRUE(imgdec_scratch_need(&img) <= sizeof(g_scratch));
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(buf, n, g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(w, img.w);
    TEST_ASSERT_EQUAL(h, img.h);
    TEST_ASSERT_EQUAL(w * h, img.gif_pixels);
    for (y = 0; y < h; y++) for (x = 0; x < w; x++) gif_expect(&img, x, y, 0);
}

static void test_gif_plain(void) {
    imgdec_t img;
    gif_check_all(gif_plain, sizeof(gif_plain), GIFS_W, GIFS_H);
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(gif_plain, sizeof(gif_plain), g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(1, img.frames);
    TEST_ASSERT_EQUAL(0, img.interlace);
    TEST_ASSERT_EQUAL_STRING("gif", imgdec_fmt_name(img.fmt));
}

static void test_gif_interlaced(void) {
    imgdec_t img;
    gif_check_all(gif_inter, sizeof(gif_inter), GIFS_W, GIFS_H);
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(gif_inter, sizeof(gif_inter), &img));
    TEST_ASSERT_EQUAL(1, img.interlace);
}

static void test_gif_local_table(void) { gif_check_all(gif_local, sizeof(gif_local), GIFS_W, GIFS_H); }

static void test_gif_big_12bit_codes(void) {
    static unsigned char big_scratch[200 * 150 * 3 + 32768];
    imgdec_t img;
    int x, y;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(gif_big, sizeof(gif_big), &img));
    TEST_ASSERT_EQUAL(IMGDEC_E_TOOBIG, imgdec_open(gif_big, sizeof(gif_big), g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(gif_big, sizeof(gif_big), big_scratch, sizeof(big_scratch), &img));
    TEST_ASSERT_EQUAL(200 * 150, img.gif_pixels);
    for (y = 0; y < 150; y += 7) for (x = 0; x < 200; x += 3) gif_expect(&img, x, y, 0);
    gif_expect(&img, 199, 149, 0);
}

static void test_gif_transparent_is_white(void) {
    imgdec_t img;
    unsigned char c[3];
    int x, y, seen = 0;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(gif_transp, sizeof(gif_transp), g_scratch, sizeof(g_scratch), &img));
    for (y = 0; y < GIFS_H; y++)
        for (x = 0; x < GIFS_W; x++) {
            if ((x + 2 * y) % 16 == 3) {
                imgdec_pixel(&img, x, y, c);
                TEST_ASSERT_EQUAL(255, c[0]); TEST_ASSERT_EQUAL(255, c[1]); TEST_ASSERT_EQUAL(255, c[2]);
                seen++;
            } else gif_expect(&img, x, y, 0);
        }
    TEST_ASSERT_TRUE(seen > 10);
}

static void test_gif_animated_first_frame(void) {
    imgdec_t img;
    int x, y;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(gif_anim, sizeof(gif_anim), g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(2, img.frames);
    for (y = 0; y < GIFS_H; y += 2) for (x = 0; x < GIFS_W; x += 3) gif_expect(&img, x, y, 0);
}

static void test_gif_truncated_and_corrupt(void) {
    static unsigned char bad[sizeof(gif_plain)];
    imgdec_t img;
    unsigned char c[3];
    int k;
    TEST_ASSERT_EQUAL(IMGDEC_E_TRUNC, imgdec_open(gif_plain, 10, g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(IMGDEC_E_FORMAT, imgdec_probe((const unsigned char *)"GIF88a....", 10, &img));
    /* cut in the middle of the code stream: the decoded part is kept */
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(gif_plain, (int)sizeof(gif_plain) - 60, g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_TRUE(img.gif_pixels > 0 && img.gif_pixels < GIFS_W * GIFS_H);
    gif_expect(&img, 0, 0, 0);
    imgdec_pixel(&img, GIFS_W - 1, GIFS_H - 1, c);
    TEST_ASSERT_EQUAL(255, c[0]);
    /* random bytes in the stream never write outside the image */
    for (k = 0; k < 64; k++) {
        int i;
        memcpy(bad, gif_plain, sizeof(gif_plain));
        for (i = 0; i < 8; i++) bad[60 + ((k * 13 + i * 7) % ((int)sizeof(gif_plain) - 62))] ^= (unsigned char)(k * 31 + i + 1);
        (void)imgdec_open(bad, sizeof(bad), g_scratch, sizeof(g_scratch), &img);
    }
    /* scratch too small and huge logical screen */
    TEST_ASSERT_EQUAL(IMGDEC_E_TOOBIG, imgdec_open(gif_plain, sizeof(gif_plain), g_scratch, 100, &img));
    memcpy(bad, gif_plain, sizeof(gif_plain));
    bad[6] = 0xFF; bad[7] = 0x7F;
    TEST_ASSERT_EQUAL(IMGDEC_E_TOOBIG, imgdec_probe(bad, sizeof(bad), &img));
}

static void test_gif_render_scaled(void) {
    imgdec_t img;
    int w, h;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(gif_plain, sizeof(gif_plain), g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_render(&img, g_rgb, 11, 11, 0, 0, 0, &w, &h));
    TEST_ASSERT_EQUAL(11, w);
    TEST_ASSERT_EQUAL(8, h);
    TEST_ASSERT_EQUAL(0, g_rgb[0]);
    TEST_ASSERT_EQUAL(255, g_rgb[1]);
}


/* ---- Lot 72: ICO (favicons) ------------------------------------------- */
static void ico_check(const unsigned char *buf, int n, int want_png) {
    imgdec_t img;
    unsigned char c[3];
    int x, y;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(buf, n, &img));
    TEST_ASSERT_EQUAL(IMGDEC_ICO, img.fmt);
    TEST_ASSERT_EQUAL(32, img.w);                      /* entry nearest 32 px */
    TEST_ASSERT_EQUAL(want_png, img.ico_png);
    TEST_ASSERT_EQUAL(IMGDEC_E_TOOBIG, imgdec_open(buf, n, g_scratch, imgdec_scratch_need(&img) - 1, &img));
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(buf, n, g_scratch, sizeof(g_scratch), &img));
    for (y = 0; y < 32; y += 3)
        for (x = 4; x < 32; x += 3) {
            imgdec_pixel(&img, x, y, c);
            TEST_ASSERT_EQUAL((x * 8) & 255, c[0]);
            TEST_ASSERT_EQUAL((y * 8) & 255, c[1]);
            TEST_ASSERT_EQUAL(((x + y) * 4) & 255, c[2]);
        }
    if (!want_png) {                                   /* alpha 0: white */
        imgdec_pixel(&img, 1, 5, c);
        TEST_ASSERT_EQUAL(255, c[0]); TEST_ASSERT_EQUAL(255, c[1]); TEST_ASSERT_EQUAL(255, c[2]);
    }
}

static void test_ico_png_and_dib(void) {
    static unsigned char bad[sizeof(gif_ico_bmp)];
    imgdec_t img;
    int k;
    ico_check(gif_ico_png, sizeof(gif_ico_png), 1);
    ico_check(gif_ico_bmp, sizeof(gif_ico_bmp), 0);
    TEST_ASSERT_EQUAL_STRING("ico", imgdec_fmt_name(IMGDEC_ICO));
    TEST_ASSERT_EQUAL(IMGDEC_E_TRUNC, imgdec_probe(gif_ico_bmp, 30, &img));
    /* corrupt directory and DIB headers stay bounded */
    for (k = 0; k < 200; k++) {
        memcpy(bad, gif_ico_bmp, sizeof(bad));
        bad[(k * 7) % 80] ^= (unsigned char)(k + 1);
        bad[(k * 131) % (int)sizeof(bad)] ^= 0x5A;
        if (imgdec_probe(bad, sizeof(bad), &img) == IMGDEC_OK && imgdec_scratch_need(&img) <= sizeof(g_scratch))
            (void)imgdec_open(bad, sizeof(bad), g_scratch, sizeof(g_scratch), &img);
    }
}


/* Lot 72: 2x1 palette PNG, index 0 black fully transparent (tRNS), index 1 red */
static const unsigned char png_pal_trns[99] = {
137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,3,0,0,0,195,252,143,184,0,0,0,6,80,76,84,69,0,0,0,255,0,0,27,255,141,34,0,0,0,1,116,82,78,83,0,64,230,216,102,0,0,0,11,73,68,65,84,120,156,99,96,96,4,0,0,4,0,2,191,122,63,74,0,0,0,0,73,69,78,68,174,66,96,130
};

/* Lot 72: PNG alpha composed over white when the caller asks (web pages) */
static void test_png_alpha_over_white(void) {
    imgdec_t img;
    int x, y, seen_partial = 0;
    unsigned char raw[3], out[3];
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(img_mire_rgba_png, sizeof(img_mire_rgba_png), &img));
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(img_mire_rgba_png, sizeof(img_mire_rgba_png), g_scratch, sizeof(g_scratch), &img));
    TEST_ASSERT_EQUAL(4, img.channels);
    for (y = 0; y < img.h; y++)
        for (x = 0; x < img.w; x++) {
            unsigned a = img.data[y * img.stride + x * 4 + 3], k;
            img.over_white = 0;
            imgdec_pixel(&img, x, y, raw);
            img.over_white = 1;
            imgdec_pixel(&img, x, y, out);
            if (a != 255) seen_partial++;
            for (k = 0; k < 3; k++)
                TEST_ASSERT_EQUAL((raw[k] * a + 255U * (255U - a) + 127U) / 255U, out[k]);
        }
    TEST_ASSERT_TRUE(seen_partial > 0);
}

/* Lot 72: palette PNG with tRNS: index 0 transparent becomes white */
static void test_png_palette_trns(void) {
    imgdec_t img;
    unsigned char rgb[3];
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(png_pal_trns, sizeof(png_pal_trns), &img));
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(png_pal_trns, sizeof(png_pal_trns), g_scratch, sizeof(g_scratch), &img));
    imgdec_pixel(&img, 0, 0, rgb);
    TEST_ASSERT_EQUAL(0, rgb[0]); TEST_ASSERT_EQUAL(0, rgb[1]); TEST_ASSERT_EQUAL(0, rgb[2]);
    img.over_white = 1;
    imgdec_pixel(&img, 0, 0, rgb);
    TEST_ASSERT_EQUAL(255, rgb[0]); TEST_ASSERT_EQUAL(255, rgb[1]); TEST_ASSERT_EQUAL(255, rgb[2]);
    imgdec_pixel(&img, 1, 0, rgb);                       /* opaque red */
    TEST_ASSERT_EQUAL(255, rgb[0]); TEST_ASSERT_EQUAL(0, rgb[1]); TEST_ASSERT_EQUAL(0, rgb[2]);
}

int main(void) {
    unity_init();
    RUN_TEST(test_bmp24);
    RUN_TEST(test_bmp32_top_down);
    RUN_TEST(test_png_rgb_split_idat);
    RUN_TEST(test_png_rgba);
    RUN_TEST(test_png_alpha_over_white);
    RUN_TEST(test_png_palette_trns);
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
    RUN_TEST(test_jpeg_scaled_decode);
    RUN_TEST(test_gif_plain);
    RUN_TEST(test_ico_png_and_dib);
    RUN_TEST(test_gif_interlaced);
    RUN_TEST(test_gif_local_table);
    RUN_TEST(test_gif_big_12bit_codes);
    RUN_TEST(test_gif_transparent_is_white);
    RUN_TEST(test_gif_animated_first_frame);
    RUN_TEST(test_gif_truncated_and_corrupt);
    RUN_TEST(test_gif_render_scaled);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
