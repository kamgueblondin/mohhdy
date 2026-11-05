/* Lot 73: GIF animation in imgdec (imgdec_anim_begin / imgdec_anim_next):
 * every frame composed onto the canvas, compared byte for byte with the
 * canvases of an independent model (scripts/gen_gif_anim_fixtures.py):
 * frame rectangles, transparent index, disposal 1/2/3, local color table,
 * interlace, delays and the NETSCAPE2.0 loop count; the Lot 72 fixture
 * made by Pillow; cut and corrupt streams. */
#include "../../framework/unity.h"
#include "imgdec.h"
#include "imgdec_anim_samples.h"
#include "imgdec_gif_samples.h"
#include <string.h>

static unsigned char g_work[200 * 150 * 6 + 32768];
static unsigned char g_buf[4096];

static int play(const unsigned char* b, int n, imgdec_t* img, imgdec_anim_t* a) {
    int rc = imgdec_probe(b, n, img);
    if (rc) return rc;
    return imgdec_anim_begin(b, n, img, a, g_work, sizeof(g_work));
}

static void test_anim_need_and_begin(void) {
    imgdec_t img;
    imgdec_anim_t a;
    unsigned need;
    int i;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(gif_anim_a, sizeof(gif_anim_a), &img));
    need = imgdec_anim_need(&img);
    /* canvas + save copy (4-byte aligned) + LZW tables */
    TEST_ASSERT_EQUAL(2U * ((8U * 6U * 3U + 3U) & ~3U) + 4096U * 5U + 4U, need);
    TEST_ASSERT_EQUAL(IMGDEC_E_TOOBIG, imgdec_anim_begin(gif_anim_a, sizeof(gif_anim_a), &img, &a, g_work, need - 1));
    memset(g_work, 7, 200);
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_anim_begin(gif_anim_a, sizeof(gif_anim_a), &img, &a, g_work, need));
    for (i = 0; i < 8 * 6 * 3; i++) TEST_ASSERT_EQUAL(255, g_work[i]);
    TEST_ASSERT_EQUAL(7, g_work[8 * 6 * 3]);    /* only the canvas is cleared */
    TEST_ASSERT_TRUE(img.data == g_work);
    TEST_ASSERT_EQUAL(0, a.frame);
    TEST_ASSERT_EQUAL(-1, a.loop);
    /* not a GIF */
    {
        imgdec_t png;
        memset(&png, 0, sizeof(png));
        png.fmt = IMGDEC_PNG;
        TEST_ASSERT_EQUAL(0U, imgdec_anim_need(&png));
        TEST_ASSERT_EQUAL(IMGDEC_E_FORMAT, imgdec_anim_begin(gif_anim_a, sizeof(gif_anim_a), &png, &a, g_work, sizeof(g_work)));
    }
}

static void test_anim_a_every_frame(void) {
    static const int delays[GIF_ANIM_A_FRAMES] = {10, 20, 0, 5};
    static const int disposal[GIF_ANIM_A_FRAMES] = {1, 2, 3, 0};
    static const int rect[GIF_ANIM_A_FRAMES][4] = {{0, 0, 8, 6}, {2, 1, 3, 2}, {4, 2, 2, 3}, {7, 0, 1, 5}};
    imgdec_t img;
    imgdec_anim_t a;
    int k;
    TEST_ASSERT_EQUAL(IMGDEC_OK, play(gif_anim_a, sizeof(gif_anim_a), &img, &a));
    for (k = 0; k < GIF_ANIM_A_FRAMES; k++) {
        TEST_ASSERT_EQUAL(1, imgdec_anim_next(gif_anim_a, sizeof(gif_anim_a), &img, &a, g_work, sizeof(g_work)));
        TEST_ASSERT_EQUAL(k + 1, a.frame);
        TEST_ASSERT_EQUAL(delays[k], a.delay_cs);
        TEST_ASSERT_EQUAL(disposal[k], a.disposal);
        TEST_ASSERT_EQUAL(rect[k][0], a.px);
        TEST_ASSERT_EQUAL(rect[k][1], a.py);
        TEST_ASSERT_EQUAL(rect[k][2], a.pw);
        TEST_ASSERT_EQUAL(rect[k][3], a.ph);
        TEST_ASSERT_EQUAL(3, a.loop);
        TEST_ASSERT_EQUAL_MEMORY(gif_anim_a_canvas[k], g_work, GIF_ANIM_A_W * GIF_ANIM_A_H * 3);
    }
    TEST_ASSERT_EQUAL(0, imgdec_anim_next(gif_anim_a, sizeof(gif_anim_a), &img, &a, g_work, sizeof(g_work)));
    TEST_ASSERT_EQUAL(1, a.done);
    TEST_ASSERT_EQUAL(0, imgdec_anim_next(gif_anim_a, sizeof(gif_anim_a), &img, &a, g_work, sizeof(g_work)));
    TEST_ASSERT_EQUAL(GIF_ANIM_A_FRAMES, a.frame);
}

static void test_anim_disposal_details(void) {
    imgdec_t img;
    imgdec_anim_t a;
    unsigned char px[3];
    play(gif_anim_a, sizeof(gif_anim_a), &img, &a);
    imgdec_anim_next(gif_anim_a, sizeof(gif_anim_a), &img, &a, g_work, sizeof(g_work));
    imgdec_anim_next(gif_anim_a, sizeof(gif_anim_a), &img, &a, g_work, sizeof(g_work));
    /* frame 2: green rectangle, its transparent pixel (3,2) shows frame 1 red */
    imgdec_pixel(&img, 2, 1, px);
    TEST_ASSERT_EQUAL(10, px[0]); TEST_ASSERT_EQUAL(200, px[1]);
    imgdec_pixel(&img, 3, 2, px);
    TEST_ASSERT_EQUAL(220, px[0]); TEST_ASSERT_EQUAL(20, px[1]);
    imgdec_anim_next(gif_anim_a, sizeof(gif_anim_a), &img, &a, g_work, sizeof(g_work));
    /* disposal 2 of frame 2: its rectangle back to the white background */
    imgdec_pixel(&img, 2, 1, px);
    TEST_ASSERT_EQUAL(255, px[0]); TEST_ASSERT_EQUAL(255, px[1]); TEST_ASSERT_EQUAL(255, px[2]);
    imgdec_pixel(&img, 4, 2, px);
    TEST_ASSERT_EQUAL(20, px[0]); TEST_ASSERT_EQUAL(230, px[2]);
    imgdec_anim_next(gif_anim_a, sizeof(gif_anim_a), &img, &a, g_work, sizeof(g_work));
    /* disposal 3 of frame 3: (4,2) is white again (as before frame 3),
     * (5,4) red (frame 1); frame 4 interlaced local colors in column 7 */
    imgdec_pixel(&img, 4, 2, px);
    TEST_ASSERT_EQUAL(255, px[0]); TEST_ASSERT_EQUAL(255, px[2]);
    imgdec_pixel(&img, 5, 4, px);
    TEST_ASSERT_EQUAL(220, px[0]);
    imgdec_pixel(&img, 7, 1, px);
    TEST_ASSERT_EQUAL(0, px[0]); TEST_ASSERT_EQUAL(210, px[1]);
    imgdec_pixel(&img, 7, 4, px);
    TEST_ASSERT_EQUAL(250, px[0]); TEST_ASSERT_EQUAL(230, px[1]);
    imgdec_pixel(&img, 7, 5, px);      /* below the frame: frame 1 red */
    TEST_ASSERT_EQUAL(220, px[0]);
}

static void test_anim_b_no_loop_and_clip(void) {
    imgdec_t img;
    imgdec_anim_t a;
    int k;
    TEST_ASSERT_EQUAL(IMGDEC_OK, play(gif_anim_b, sizeof(gif_anim_b), &img, &a));
    for (k = 0; k < GIF_ANIM_B_FRAMES; k++) {
        TEST_ASSERT_EQUAL(1, imgdec_anim_next(gif_anim_b, sizeof(gif_anim_b), &img, &a, g_work, sizeof(g_work)));
        TEST_ASSERT_EQUAL_MEMORY(gif_anim_b_canvas[k], g_work, GIF_ANIM_B_W * GIF_ANIM_B_H * 3);
    }
    TEST_ASSERT_EQUAL(9, a.delay_cs);
    TEST_ASSERT_EQUAL(-1, a.loop);
    TEST_ASSERT_EQUAL(0, imgdec_anim_next(gif_anim_b, sizeof(gif_anim_b), &img, &a, g_work, sizeof(g_work)));
}

static void test_anim_pillow_fixture(void) {
    /* Lot 72 fixture written by Pillow: two frames, loop 0, 100 ms; the
     * first composed frame equals imgdec_open (first frame) */
    static unsigned char first[GIFS_W * GIFS_H * 3];
    imgdec_t img, one;
    imgdec_anim_t a;
    int x, y;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(gif_anim, sizeof(gif_anim), &one));
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(gif_anim, sizeof(gif_anim), g_work, sizeof(g_work), &one));
    memcpy(first, g_work, sizeof(first));
    TEST_ASSERT_EQUAL(IMGDEC_OK, play(gif_anim, sizeof(gif_anim), &img, &a));
    TEST_ASSERT_EQUAL(1, imgdec_anim_next(gif_anim, sizeof(gif_anim), &img, &a, g_work, sizeof(g_work)));
    TEST_ASSERT_EQUAL_MEMORY(first, g_work, sizeof(first));
    TEST_ASSERT_EQUAL(10, a.delay_cs);
    TEST_ASSERT_EQUAL(0, a.loop);
    TEST_ASSERT_EQUAL(1, imgdec_anim_next(gif_anim, sizeof(gif_anim), &img, &a, g_work, sizeof(g_work)));
    /* second frame: palette index (x + 2y + 5) % 16 wherever it differs
     * from the first; check a pixel that changes */
    for (y = 0; y < 2; y++)
        for (x = 0; x < 3; x++) {
            unsigned char c[3];
            int k = (x + 2 * y + 5) % 16;
            imgdec_pixel(&img, x, y, c);
            TEST_ASSERT_EQUAL(k * 16, c[0]);
            TEST_ASSERT_EQUAL(255 - k * 16, c[1]);
        }
    TEST_ASSERT_EQUAL(0, imgdec_anim_next(gif_anim, sizeof(gif_anim), &img, &a, g_work, sizeof(g_work)));
    TEST_ASSERT_EQUAL(2, a.frame);
}

static void test_anim_cut_and_corrupt(void) {
    imgdec_t img;
    imgdec_anim_t a;
    int n = (int)sizeof(gif_anim_a), cut, k, got;
    /* the file cut before frame 3 (at its graphic control extension): two
     * frames, then the end; the last picture stays (no disposal applied) */
    memcpy(g_buf, gif_anim_a, (size_t)n);
    for (cut = n - 1; cut > 0; cut--) if (g_buf[cut] == 0x2C) break;     /* last descriptor */
    for (cut = cut - 1; cut > 0; cut--) if (g_buf[cut] == 0x2C) break;     /* frame 3 */
    cut -= 8;                                                             /* its 21 F9 04 ... */
    TEST_ASSERT_EQUAL(0x21, g_buf[cut]);
    TEST_ASSERT_EQUAL(0xF9, g_buf[cut + 1]);
    TEST_ASSERT_EQUAL(IMGDEC_OK, play(g_buf, cut, &img, &a));
    got = 0;
    for (k = 0; k < 6; k++) if (imgdec_anim_next(g_buf, cut, &img, &a, g_work, sizeof(g_work)) == 1) got++;
    TEST_ASSERT_EQUAL(2, got);
    TEST_ASSERT_EQUAL(1, a.done);
    TEST_ASSERT_EQUAL_MEMORY(gif_anim_a_canvas[1], g_work, 8 * 6 * 3);
    /* cut inside the data of frame 3: three frames, the third partial and last */
    TEST_ASSERT_EQUAL(IMGDEC_OK, play(g_buf, cut + 8 + 14, &img, &a));
    got = 0;
    for (k = 0; k < 6; k++) if (imgdec_anim_next(g_buf, cut + 8 + 14, &img, &a, g_work, sizeof(g_work)) == 1) got++;
    TEST_ASSERT_EQUAL(3, got);
    TEST_ASSERT_EQUAL(1, a.done);
    /* no image at all after the header: an error on the first frame */
    memcpy(g_buf, gif_anim_a, 13 + 12);
    g_buf[25] = 0x3B;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_anim_begin(g_buf, 26, &img, &a, g_work, sizeof(g_work)));
    TEST_ASSERT_TRUE(imgdec_anim_next(g_buf, 26, &img, &a, g_work, sizeof(g_work)) < 0);
    TEST_ASSERT_EQUAL(1, a.done);
    /* a corrupt code in frame 2 (first code after clear out of range) */
    memcpy(g_buf, gif_anim_a, (size_t)n);
    for (k = 13 + 12; k < n; k++) if (g_buf[k] == 0x2C && k > 60) break;
    TEST_ASSERT_TRUE(k < n);
    g_buf[k + 12] = 0xFF;   /* first data byte: codes 0x7 (eoi+? ) */
    TEST_ASSERT_EQUAL(IMGDEC_OK, play(g_buf, n, &img, &a));
    TEST_ASSERT_EQUAL(1, imgdec_anim_next(g_buf, n, &img, &a, g_work, sizeof(g_work)));
    got = imgdec_anim_next(g_buf, n, &img, &a, g_work, sizeof(g_work));
    TEST_ASSERT_TRUE(got == 0 || got == 1);
    for (k = 0; k < 8; k++) (void)imgdec_anim_next(g_buf, n, &img, &a, g_work, sizeof(g_work));
    TEST_ASSERT_TRUE(a.frame <= GIF_ANIM_A_FRAMES);
    TEST_ASSERT_EQUAL(1, a.done);
}

static void test_anim_page_fixture(void) {
    imgdec_t img;
    imgdec_anim_t a;
    unsigned char c[3];
    static const unsigned char want[3][3] = {{220, 20, 30}, {10, 200, 40}, {20, 40, 230}};
    int k;
    TEST_ASSERT_EQUAL(IMGDEC_OK, play(gif_anim_page, sizeof(gif_anim_page), &img, &a));
    TEST_ASSERT_EQUAL(32, img.w);
    for (k = 0; k < 3; k++) {
        TEST_ASSERT_EQUAL(1, imgdec_anim_next(gif_anim_page, sizeof(gif_anim_page), &img, &a, g_work, sizeof(g_work)));
        imgdec_pixel(&img, 31, 23, c);
        TEST_ASSERT_EQUAL_MEMORY(want[k], c, 3);
        TEST_ASSERT_EQUAL(50, a.delay_cs);
    }
    TEST_ASSERT_EQUAL(0, a.loop);
    TEST_ASSERT_EQUAL(IMGDEC_OK, play(gif_anim_twice, sizeof(gif_anim_twice), &img, &a));
    TEST_ASSERT_EQUAL(1, imgdec_anim_next(gif_anim_twice, sizeof(gif_anim_twice), &img, &a, g_work, sizeof(g_work)));
    TEST_ASSERT_EQUAL(1, a.loop);
    TEST_ASSERT_EQUAL(30, a.delay_cs);
}

static void test_open_counts_frames_and_loop_ext(void) {
    /* imgdec_open still decodes the first frame only and counts the frames
     * across the application extension */
    imgdec_t img;
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_probe(gif_anim_a, sizeof(gif_anim_a), &img));
    TEST_ASSERT_EQUAL(IMGDEC_OK, imgdec_open(gif_anim_a, sizeof(gif_anim_a), g_work, sizeof(g_work), &img));
    TEST_ASSERT_EQUAL(4, img.frames);
    TEST_ASSERT_EQUAL_MEMORY(gif_anim_a_canvas[0], g_work, 8 * 6 * 3);
}

/* Patched copies of anim_a: the ANIMEXTS1.0 identifier, a loop count
 * above 255 (two bytes, little endian) and a reserved disposal (6: kept
 * as is, the 3 bits reported, not read as 2). */
static unsigned char g_patch[sizeof(gif_anim_a)];

static int find_bytes(const unsigned char* b, int n, const char* s, int len) {
    int i;
    for (i = 0; i + len <= n; i++) if (memcmp(b + i, s, (size_t)len) == 0) return i;
    return -1;
}

static void test_anim_loop_ids_and_reserved_disposal(void) {
    imgdec_t img;
    imgdec_anim_t a;
    int at = find_bytes(gif_anim_a, (int)sizeof(gif_anim_a), "NETSCAPE2.0", 11), k, gce, seen = 0;
    TEST_ASSERT_TRUE(at > 0);
    memcpy(g_patch, gif_anim_a, sizeof(g_patch));
    memcpy(g_patch + at, "ANIMEXTS1.0", 11);
    TEST_ASSERT_EQUAL(IMGDEC_OK, play(g_patch, (int)sizeof(g_patch), &img, &a));
    TEST_ASSERT_EQUAL(1, imgdec_anim_next(g_patch, (int)sizeof(g_patch), &img, &a, g_work, sizeof(g_work)));
    TEST_ASSERT_EQUAL(3, a.loop);
    /* another identifier is no loop extension */
    memcpy(g_patch + at, "NETSCAPE2.1", 11);
    TEST_ASSERT_EQUAL(IMGDEC_OK, play(g_patch, (int)sizeof(g_patch), &img, &a));
    TEST_ASSERT_EQUAL(1, imgdec_anim_next(g_patch, (int)sizeof(g_patch), &img, &a, g_work, sizeof(g_work)));
    TEST_ASSERT_EQUAL(-1, a.loop);
    /* loop 259 = 0x0103 */
    memcpy(g_patch, gif_anim_a, sizeof(g_patch));
    TEST_ASSERT_EQUAL(3, g_patch[at + 11]);
    TEST_ASSERT_EQUAL(1, g_patch[at + 12]);
    g_patch[at + 13] = 3; g_patch[at + 14] = 1;
    TEST_ASSERT_EQUAL(IMGDEC_OK, play(g_patch, (int)sizeof(g_patch), &img, &a));
    TEST_ASSERT_EQUAL(1, imgdec_anim_next(g_patch, (int)sizeof(g_patch), &img, &a, g_work, sizeof(g_work)));
    TEST_ASSERT_EQUAL(259, a.loop);
    /* frame 2 (disposal 2, transparent flag) becomes disposal 6 */
    memcpy(g_patch, gif_anim_a, sizeof(g_patch));
    for (gce = 0, k = 0; k + 3 < (int)sizeof(g_patch); k++)
        if (g_patch[k] == 0x21 && g_patch[k + 1] == 0xF9 && g_patch[k + 2] == 4 && ++gce == 2) {
            TEST_ASSERT_EQUAL(0x09, g_patch[k + 3]);
            g_patch[k + 3] = (unsigned char)((6 << 2) | 1);
            seen = 1;
            break;
        }
    TEST_ASSERT_TRUE(seen);
    TEST_ASSERT_EQUAL(IMGDEC_OK, play(g_patch, (int)sizeof(g_patch), &img, &a));
    TEST_ASSERT_EQUAL(1, imgdec_anim_next(g_patch, (int)sizeof(g_patch), &img, &a, g_work, sizeof(g_work)));
    TEST_ASSERT_EQUAL(1, imgdec_anim_next(g_patch, (int)sizeof(g_patch), &img, &a, g_work, sizeof(g_work)));
    TEST_ASSERT_EQUAL(6, a.disposal);
    TEST_ASSERT_EQUAL_MEMORY(gif_anim_a_canvas[1], g_work, GIF_ANIM_A_W * GIF_ANIM_A_H * 3);
    /* frame 3: the green rectangle of frame 2 is kept (not cleared to white) */
    TEST_ASSERT_EQUAL(1, imgdec_anim_next(g_patch, (int)sizeof(g_patch), &img, &a, g_work, sizeof(g_work)));
    {
        unsigned o = (1U * GIF_ANIM_A_W + 2U) * 3U;
        TEST_ASSERT_EQUAL(10, g_work[o]);
        TEST_ASSERT_EQUAL(200, g_work[o + 1]);
        TEST_ASSERT_EQUAL(255, gif_anim_a_canvas[2][o]);   /* with disposal 2 it is white */
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_anim_need_and_begin);
    RUN_TEST(test_anim_a_every_frame);
    RUN_TEST(test_anim_disposal_details);
    RUN_TEST(test_anim_b_no_loop_and_clip);
    RUN_TEST(test_anim_pillow_fixture);
    RUN_TEST(test_anim_cut_and_corrupt);
    RUN_TEST(test_anim_page_fixture);
    RUN_TEST(test_open_counts_frames_and_loop_ext);
    RUN_TEST(test_anim_loop_ids_and_reserved_disposal);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
