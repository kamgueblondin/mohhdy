/* Lot 73: WebP lossless (VP8L) in imgdec: every fixture of
 * scripts/gen_webp_fixtures.py decoded and compared byte for byte with
 * libwebp's own decode (predictor, cross color, subtract green, color
 * indexing at 1/2/4/8 bits, color cache, backward references, meta
 * prefix codes, alpha, VP8X with EXIF); lossy and animated files are
 * refused; cut and corrupt streams fail without reading out of bounds. */
#include "../../framework/unity.h"
#include "imgdec.h"
#include "imgdec_webp_samples.h"
#include <string.h>

static unsigned char g_scratch[64 * 48 * 4 + 300 * 1024 + 8];
static unsigned char g_buf[4096];

static int decode(const unsigned char *b, int n, imgdec_t *img) {
    return imgdec_open(b, n, g_scratch, sizeof(g_scratch), img);
}

static void check(const unsigned char *b, int n, const unsigned char *rgba, int w, int h) {
    imgdec_t img;
    int y;
    TEST_ASSERT_EQUAL(IMGDEC_OK, decode(b, n, &img));
    TEST_ASSERT_EQUAL(IMGDEC_WEBP, img.fmt);
    TEST_ASSERT_EQUAL(w, img.w);
    TEST_ASSERT_EQUAL(h, img.h);
    TEST_ASSERT_EQUAL(4, img.channels);
    for (y = 0; y < h; y++) TEST_ASSERT_EQUAL(0, memcmp(img.data + y * img.stride, rgba + y * w * 4, (size_t)(w * 4)));
}
#define CHECK(name, NAME) check(webp_##name, (int)sizeof(webp_##name), webp_##name##_rgba, WEBP_##NAME##_W, WEBP_##NAME##_H)

static void test_webp_transforms(void) {
    CHECK(grad, GRAD);
    CHECK(alpha, ALPHA);
}
static void test_webp_color_indexing(void) {
    CHECK(pal2, PAL2);
    CHECK(pal4, PAL4);
    CHECK(pal16, PAL16);
    CHECK(pal40, PAL40);
}
static void test_webp_cache_backrefs_meta(void) {
    CHECK(tiles, TILES);
    CHECK(mixed, MIXED);
    CHECK(fast, FAST);
}

static void test_webp_probe_and_pixel(void) {
    imgdec_t img;
    unsigned char px[3];
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(webp_alpha, (int)sizeof(webp_alpha), &img));
    TEST_ASSERT_EQUAL(IMGDEC_WEBP, img.fmt);
    TEST_ASSERT_EQUAL(10, img.w);
    TEST_ASSERT_EQUAL(1, img.webp_alpha);
    TEST_ASSERT_EQUAL_STRING("webp", imgdec_fmt_name(img.fmt));
    TEST_ASSERT_TRUE(imgdec_scratch_need(&img) >= 10U * 10U * 4U);
    TEST_ASSERT_EQUAL(IMGDEC_E_TOOBIG, imgdec_open(webp_alpha, (int)sizeof(webp_alpha), g_scratch, 100, &img));
    TEST_ASSERT_EQUAL(IMGDEC_OK, decode(webp_alpha, (int)sizeof(webp_alpha), &img));
    /* pixel (3, 2): raw samples, then over a white page */
    imgdec_pixel(&img, 3, 2, px);
    TEST_ASSERT_EQUAL(webp_alpha_rgba[(2 * 10 + 3) * 4], px[0]);
    TEST_ASSERT_EQUAL(webp_alpha_rgba[(2 * 10 + 3) * 4 + 2], px[2]);
    img.over_white = 1;
    imgdec_pixel(&img, 3, 2, px);
    {
        unsigned a = webp_alpha_rgba[(2 * 10 + 3) * 4 + 3], r = webp_alpha_rgba[(2 * 10 + 3) * 4];
        TEST_ASSERT_TRUE(a != 255);
        TEST_ASSERT_EQUAL((r * a + 255U * (255U - a) + 127U) / 255U, px[0]);
    }
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(webp_grad, (int)sizeof(webp_grad), &img));
    TEST_ASSERT_EQUAL(0, img.webp_alpha);
}

static void test_webp_container(void) {
    imgdec_t img;
    check(webp_vp8x, (int)sizeof(webp_vp8x), webp_vp8x_rgba, 6, 4);   /* VP8X + EXIF around VP8L */
    TEST_ASSERT_EQUAL(IMGDEC_E_UNSUP, decode(webp_lossy, (int)sizeof(webp_lossy), &img));
    TEST_ASSERT_EQUAL(IMGDEC_WEBP, img.fmt);
    TEST_ASSERT_EQUAL(1, img.webp_lossy);
    TEST_ASSERT_EQUAL(16, img.w);
    TEST_ASSERT_EQUAL(12, img.h);
    TEST_ASSERT_EQUAL(IMGDEC_E_UNSUP, decode(webp_anim, (int)sizeof(webp_anim), &img));
    TEST_ASSERT_EQUAL(IMGDEC_WEBP, img.fmt);
    /* not WebP: RIFF of another kind */
    memcpy(g_buf, webp_grad, sizeof(webp_grad));
    g_buf[8] = 'A';
    TEST_ASSERT_TRUE(imgdec_probe(g_buf, (int)sizeof(webp_grad), &img) != IMGDEC_OK);
    TEST_ASSERT_TRUE(img.fmt != IMGDEC_WEBP);
    /* bad signature byte, version, chunk size over the file */
    memcpy(g_buf, webp_grad, sizeof(webp_grad));
    g_buf[20] = 0x2e;
    TEST_ASSERT_EQUAL(IMGDEC_E_FORMAT, imgdec_probe(g_buf, (int)sizeof(webp_grad), &img));
    memcpy(g_buf, webp_grad, sizeof(webp_grad));
    g_buf[24] |= 0x20;
    TEST_ASSERT_EQUAL(IMGDEC_E_UNSUP, imgdec_probe(g_buf, (int)sizeof(webp_grad), &img));
    memcpy(g_buf, webp_grad, sizeof(webp_grad));
    g_buf[16] = 0xff;
    TEST_ASSERT_EQUAL(IMGDEC_E_TRUNC, imgdec_probe(g_buf, (int)sizeof(webp_grad), &img));
    TEST_ASSERT_EQUAL(IMGDEC_E_TRUNC, imgdec_probe(webp_grad, 14, &img));
    TEST_ASSERT_EQUAL(IMGDEC_E_TRUNC, imgdec_probe(webp_grad, 22, &img));
}

static void test_webp_cut_and_corrupt(void) {
    imgdec_t img;
    int n, k, ok = 0, fail = 0;
    /* every cut of the payload fails cleanly (the last byte only holds
     * padding bits) */
    for (n = 25; n <= (int)sizeof(webp_tiles); n++) {
        memcpy(g_buf, webp_tiles, (size_t)n);
        g_buf[16] = (unsigned char)(n - 20); g_buf[17] = g_buf[18] = g_buf[19] = 0;
        if (n >= (int)sizeof(webp_tiles) - 1) TEST_ASSERT_EQUAL(IMGDEC_OK, decode(g_buf, n, &img));
        else TEST_ASSERT_TRUE(decode(g_buf, n, &img) != IMGDEC_OK);
    }
    /* flipped bits decode or fail, never crash (ASan) */
    for (k = 21; k < (int)sizeof(webp_pal40); k++) {
        memcpy(g_buf, webp_pal40, sizeof(webp_pal40));
        g_buf[k] ^= 0x5a;
        if (decode(g_buf, (int)sizeof(webp_pal40), &img) == IMGDEC_OK) ok++; else fail++;
    }
    TEST_ASSERT_TRUE(fail > 0);
    TEST_ASSERT_EQUAL((int)sizeof(webp_pal40) - 21, ok + fail);
}

/* ---- crafted VP8L streams: a small bit writer and canonical codes ---- */
typedef struct { unsigned char b[2048]; int bit; } bw_t;
static bw_t g_bw;
static void put(unsigned v, int n) {
    int i;
    for (i = 0; i < n; i++, g_bw.bit++)
        if ((v >> i) & 1U) g_bw.b[g_bw.bit >> 3] |= (unsigned char)(1U << (g_bw.bit & 7));
}
/* RIFF + VP8L chunk around the bits written after start(w, h) */
static void start(int w, int h) {
    memset(&g_bw, 0, sizeof(g_bw));
    g_bw.bit = 20 * 8;
    put(0x2f, 8);
    put((unsigned)(w - 1), 14);
    put((unsigned)(h - 1), 14);
    put(0, 1);   /* alpha hint */
    put(0, 3);   /* version */
}
static int finish(void) {
    int n = (g_bw.bit + 7) / 8, len = n - 20;
    memcpy(g_bw.b, "RIFF", 4);
    g_bw.b[4] = (unsigned char)(n - 8); g_bw.b[5] = (unsigned char)((n - 8) >> 8);
    memcpy(g_bw.b + 8, "WEBPVP8L", 8);
    g_bw.b[16] = (unsigned char)len; g_bw.b[17] = (unsigned char)(len >> 8);
    return n;
}
static void simple1(int sym) { put(1, 1); put(0, 1); put(1, 1); put((unsigned)sym, 8); }
/* a normal code: the code-length code gives values 0..15 four bits each */
static void normal(const unsigned char *len, int n) {
    static const unsigned char order[19] = { 17, 18, 0, 1, 2, 3, 4, 5, 16, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    int i, k;
    put(0, 1);
    put(15, 4);
    for (i = 0; i < 19; i++) put(order[i] <= 15 ? 4U : 0U, 3);
    put(0, 1);
    for (i = 0; i < n; i++) for (k = 3; k >= 0; k--) put((len[i] >> k) & 1U, 1);
}
static void sym(const unsigned char *len, int n, int s) {
    int count[16] = {0}, next[16], code = 0, i, l, used = 0;
    for (i = 0; i < n; i++) if (len[i]) { count[len[i]]++; used++; }
    if (used == 1) return;
    for (l = 1; l < 16; l++) { code = (code + count[l - 1]) << 1; next[l] = code; }
    next[1] = 0;
    for (l = 2; l < 16; l++) next[l] = (next[l - 1] + count[l - 1]) << 1;
    code = next[len[s]];
    for (i = 0; i < s; i++) if (len[i] == len[s]) code++;
    for (l = len[s] - 1; l >= 0; l--) put((unsigned)(code >> l) & 1U, 1);
}
static unsigned char g_px[64 * 4];
static int run(const unsigned char *b, int n, int w, int h) {
    imgdec_t img;
    int rc;
    memset(g_scratch, 0, sizeof(g_scratch));
    rc = decode(b, n, &img);
    if (rc == IMGDEC_OK) memcpy(g_px, img.data, (size_t)(w * h * 4));
    return rc;
}
static unsigned g_at(int k) { return ((unsigned)g_px[k * 4 + 3] << 24) | ((unsigned)g_px[k * 4] << 16) | ((unsigned)g_px[k * 4 + 1] << 8) | g_px[k * 4 + 2]; }

static unsigned char g_green[256 + 24 + 2048];

static void test_webp_crafted_codes(void) {
    int n, k;
    /* an incomplete code (two symbols of length 2) is refused */
    start(1, 1);
    put(0, 1); put(0, 1); put(0, 1);
    memset(g_green, 0, sizeof(g_green));
    g_green[0] = 2; g_green[1] = 2;
    normal(g_green, 280);
    n = finish();
    TEST_ASSERT_EQUAL(IMGDEC_E_FORMAT, run(g_bw.b, n, 1, 1));
    /* code 16 before any non-zero length repeats 8: 256 lengths of 8 */
    start(1, 1);
    put(0, 1); put(0, 1); put(0, 1);
    put(0, 1);
    put(8, 4);                                        /* 12 code-length lengths */
    {
        static const unsigned char cl[12] = { 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1 };   /* 17, 18, 0..5, 16, 6, 7, 8 */
        for (k = 0; k < 12; k++) put(cl[k], 3);
    }
    put(0, 1);
    put(1, 1); put(0, 1); put(0, 2);                  /* "10": 16, three times 8 */
    for (k = 0; k < 253; k++) put(0, 1);              /* "0": 8 */
    put(1, 1); put(1, 1); put(7, 3);                  /* "11": 17, ten zeros */
    put(1, 1); put(1, 1); put(7, 3);
    put(1, 1); put(1, 1); put(1, 3);                  /* four zeros: 280 */
    simple1(0); simple1(0); simple1(255); simple1(0);
    for (k = 7; k >= 0; k--) put((200U >> k) & 1U, 1);  /* green 200: code 200 */
    n = finish();
    TEST_ASSERT_EQUAL(IMGDEC_OK, run(g_bw.b, n, 1, 1));
    TEST_ASSERT_EQUAL(0xff00c800U, g_at(0));
}

/* green code of 8 symbols of length 3 over the given ones (padded) */
static void green8(const int *syms, int ns, int alphabet) {
    int k, have = 0;
    memset(g_green, 0, sizeof(g_green));
    for (k = 0; k < ns; k++) if (!g_green[syms[k]]) { g_green[syms[k]] = 3; have++; }
    for (k = 0; have < 8; k++) if (!g_green[k]) { g_green[k] = 3; have++; }
    normal(g_green, alphabet);
}

static void test_webp_crafted_backrefs(void) {
    int n;
    /* distance code 4 is (-1, 1): 0 on a one pixel wide image, read as 1 */
    {
        static const int gs[] = { 10, 257 };
        start(1, 3);
        put(0, 1); put(0, 1); put(0, 1);
        green8(gs, 2, 280); simple1(0); simple1(0); simple1(255); simple1(3);
        sym(g_green, 280, 10);
        sym(g_green, 280, 257);                       /* length 2 */
        n = finish();
        TEST_ASSERT_EQUAL(IMGDEC_OK, run(g_bw.b, n, 1, 3));
        TEST_ASSERT_EQUAL(0xff000a00U, g_at(1));
        TEST_ASSERT_EQUAL(0xff000a00U, g_at(2));
    }
    /* distance code 121 is the plain distance 1 */
    {
        static const int gs[] = { 10, 257 };
        start(3, 1);
        put(0, 1); put(0, 1); put(0, 1);
        green8(gs, 2, 280); simple1(0); simple1(0); simple1(255); simple1(13);
        sym(g_green, 280, 10);
        sym(g_green, 280, 257); put(24, 5);           /* prefix 13: 96 + 24 + 1 */
        n = finish();
        TEST_ASSERT_EQUAL(IMGDEC_OK, run(g_bw.b, n, 3, 1));
        TEST_ASSERT_EQUAL(0xff000a00U, g_at(2));
    }
}

static unsigned slot1(unsigned argb) { return (0x1e35a7bdU * argb) >> 31; }

static void test_webp_crafted_cache(void) {
    /* every copied pixel enters the color cache, not only the last */
    unsigned a = 0, d = 0, e = 0, ga, gd, ge;
    int n, gs[5];
    for (ga = 10; ga < 60 && !d; ga++) {
        a = 0xff000000U | (ga << 8);
        for (gd = ga + 1; gd < 60; gd++) if (slot1(0xff000000U | (gd << 8)) == slot1(a)) { d = 0xff000000U | (gd << 8); break; }
    }
    ga--;
    gd = (d >> 8) & 255;
    for (ge = 10; ge < 60; ge++) if (ge != ga && ge != gd && slot1(0xff000000U | (ge << 8)) != slot1(a)) break;
    e = 0xff000000U | (ge << 8);
    gs[0] = (int)ga; gs[1] = (int)ge; gs[2] = (int)gd; gs[3] = 257; gs[4] = 280 + (int)slot1(a);
    start(6, 1);
    put(0, 1);
    put(1, 1); put(1, 4);                             /* cache of 2 */
    put(0, 1);
    green8(gs, 5, 282); simple1(0); simple1(0); simple1(255); simple1(13);
    sym(g_green, 282, (int)ga); sym(g_green, 282, (int)ge); sym(g_green, 282, (int)gd);
    sym(g_green, 282, 257); put(26, 5);               /* copy 2 from 3 back */
    sym(g_green, 282, 280 + (int)slot1(a));
    n = finish();
    TEST_ASSERT_EQUAL(IMGDEC_OK, run(g_bw.b, n, 6, 1));
    TEST_ASSERT_EQUAL(a, g_at(3));
    TEST_ASSERT_EQUAL(e, g_at(4));
    TEST_ASSERT_EQUAL(a, g_at(5));
}

static void test_webp_crafted_predictor_half(void) {
    /* mode 13 at (1, 1): L 10, T 21, TL 20: 15 + (15 - 20) / 2 = 13 */
    int n;
    start(2, 2);
    put(1, 1); put(0, 2); put(0, 3);                  /* predictor, blocks of 4 */
    put(0, 1);                                        /* its sub-image: no cache */
    simple1(13); simple1(0); simple1(0); simple1(0); simple1(0);
    put(0, 1);                                        /* no more transforms */
    put(0, 1); put(0, 1);
    memset(g_green, 0, sizeof(g_green));
    g_green[20] = 2; g_green[1] = 2; g_green[246] = 2; g_green[0] = 2;
    normal(g_green, 280); simple1(0); simple1(0); simple1(0); simple1(0);
    sym(g_green, 280, 20); sym(g_green, 280, 1); sym(g_green, 280, 246); sym(g_green, 280, 0);
    n = finish();
    TEST_ASSERT_EQUAL(IMGDEC_OK, run(g_bw.b, n, 2, 2));
    TEST_ASSERT_EQUAL(0xff001400U, g_at(0));
    TEST_ASSERT_EQUAL(0xff001500U, g_at(1));
    TEST_ASSERT_EQUAL(0xff000a00U, g_at(2));
    TEST_ASSERT_EQUAL(0xff000d00U, g_at(3));
}

static void test_webp_crafted_indexing(void) {
    int n;
    /* 3 colors (2 bits): index 3 is past the table, transparent black */
    start(3, 1);
    put(1, 1); put(3, 2); put(2, 8);                  /* color indexing, 3 colors */
    put(0, 1);
    simple1(10); simple1(0); simple1(0);
    put(1, 1); put(1, 1); put(1, 1); put(0, 8); put(255, 8);   /* alpha 0 or 255 */
    simple1(0);
    put(1, 1); put(0, 1); put(0, 1);                  /* alpha 255, 0, 0 */
    put(0, 1);
    put(0, 1); put(0, 1);
    simple1(44); simple1(0); simple1(0); simple1(0); simple1(0);
    n = finish();
    TEST_ASSERT_EQUAL(IMGDEC_OK, run(g_bw.b, n, 3, 1));
    TEST_ASSERT_EQUAL(0xff000a00U, g_at(0));
    TEST_ASSERT_EQUAL(0U, g_at(1));
    TEST_ASSERT_EQUAL(0xff001e00U, g_at(2));
    /* 16 colors pack two pixels per byte */
    {
        int k;
        start(2, 1);
        put(1, 1); put(3, 2); put(15, 8);
        put(0, 1);
        put(1, 1); put(1, 1); put(0, 1); put(0, 1); put(1, 8);   /* green 0 or 1 */
        simple1(0); simple1(0);
        put(1, 1); put(1, 1); put(1, 1); put(0, 8); put(255, 8);
        simple1(0);
        put(0, 1); put(1, 1);                         /* entry 0: green 0, alpha 255 */
        for (k = 1; k < 16; k++) { put(1, 1); put(0, 1); }   /* +1 green each */
        put(0, 1);
        put(0, 1); put(0, 1);
        simple1(3 | (5 << 4)); simple1(0); simple1(0); simple1(0); simple1(0);
        n = finish();
        TEST_ASSERT_EQUAL(IMGDEC_OK, run(g_bw.b, n, 2, 1));
        TEST_ASSERT_EQUAL(0xff000300U, g_at(0));
        TEST_ASSERT_EQUAL(0xff000500U, g_at(1));
    }
    /* a transform used twice is refused */
    start(1, 1);
    put(1, 1); put(2, 2); put(1, 1); put(2, 2);
    n = finish();
    TEST_ASSERT_EQUAL(IMGDEC_E_FORMAT, run(g_bw.b, n, 1, 1));
}

static void test_webp_odd_chunk(void) {
    /* VP8X, an odd EXIF chunk and its pad byte, then the VP8L chunk */
    static unsigned char f[sizeof(webp_pal2) + 32];
    int n = 0, len = (int)sizeof(webp_pal2) - 12;
    memcpy(f, "RIFF\0\0\0\0WEBPVP8X", 16); n = 16;
    f[n] = 10; f[n + 1] = f[n + 2] = f[n + 3] = 0; n += 4;
    memset(f + n, 0, 10); f[n + 4] = 8; f[n + 7] = 2; n += 10;
    memcpy(f + n, "EXIF", 4); f[n + 4] = 1; f[n + 5] = f[n + 6] = f[n + 7] = 0; f[n + 8] = 0x41; f[n + 9] = 0; n += 10;
    memcpy(f + n, webp_pal2 + 12, (size_t)len); n += len;
    f[4] = (unsigned char)(n - 8);
    check(f, n, webp_pal2_rgba, WEBP_PAL2_W, WEBP_PAL2_H);
}

int main(void) {
    unity_init();
    RUN_TEST(test_webp_transforms);
    RUN_TEST(test_webp_color_indexing);
    RUN_TEST(test_webp_cache_backrefs_meta);
    RUN_TEST(test_webp_probe_and_pixel);
    RUN_TEST(test_webp_container);
    RUN_TEST(test_webp_cut_and_corrupt);
    RUN_TEST(test_webp_crafted_codes);
    RUN_TEST(test_webp_crafted_backrefs);
    RUN_TEST(test_webp_crafted_cache);
    RUN_TEST(test_webp_crafted_predictor_half);
    RUN_TEST(test_webp_crafted_indexing);
    RUN_TEST(test_webp_odd_chunk);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
