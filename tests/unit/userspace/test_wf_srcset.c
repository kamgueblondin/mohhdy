/* Lot 73: responsive images: srcset (x and w descriptors), sizes, media
 * and type selection (wf_srcset.c), and wf_html choosing the URL of an
 * <img> from its <picture> sources, its srcset, then src. */
#include "../../framework/unity.h"
#include "wf_html.h"
#include "wf_srcset.h"
#include <string.h>

static char g_out[16384], g_url[256];

static const char *pick(const char *set, const char *sizes, int vw, int dpr) {
    int n;
    g_url[0] = 'Z'; g_url[1] = 0;
    n = wf_srcset_pick(set, sizes, vw, dpr, g_url, (int)sizeof(g_url));
    if (n != (int)strlen(g_url)) return "LENGTH MISMATCH";
    return g_url;
}

static void test_x_descriptors(void) {
    TEST_ASSERT_EQUAL_STRING("a.png", pick("a.png 1x, b.png 2x", 0, 640, 100));
    TEST_ASSERT_EQUAL_STRING("b.png", pick("a.png 1x, b.png 2x", 0, 640, 200));
    TEST_ASSERT_EQUAL_STRING("b.png", pick("a.png 1x, b.png 2x", 0, 640, 150));    /* smallest >= 1.5 */
    TEST_ASSERT_EQUAL_STRING("b.png", pick("a.png 1x, b.png 2x", 0, 640, 300));    /* none >= 3: the largest */
    TEST_ASSERT_EQUAL_STRING("c.png", pick("b.png 2x, c.png 1.5x, a.png 1x", 0, 640, 120));
    TEST_ASSERT_EQUAL_STRING("a.png", pick("a.png, b.png 2x", 0, 640, 100));       /* no descriptor is 1x */
    TEST_ASSERT_EQUAL_STRING("a.png", pick("a.png 1x, d.png 1x", 0, 640, 100));    /* tie: the first */
    TEST_ASSERT_EQUAL_STRING("a.png", pick("  a.png   1x ,b.png 2x ", 0, 640, 100));
    TEST_ASSERT_EQUAL_STRING("b.png", pick("a.png 1x,b.png 2x", 0, 640, 200));      /* comma right after a descriptor */
    TEST_ASSERT_EQUAL_STRING("a.png,b.png", pick("a.png,b.png 2x", 0, 640, 100));  /* a comma inside a URL token */
    TEST_ASSERT_EQUAL_STRING("a.png", pick("a.png,, b.png 2x", 0, 640, 100));      /* trailing commas end the URL */
    TEST_ASSERT_EQUAL_STRING("a.png", pick("a.png 1x", 0, 640, 0));                 /* dpr 0 is 1 */
    TEST_ASSERT_EQUAL_STRING("a.png", pick("a.png 1x, b.png 0.5x", 0, 640, 0));
    TEST_ASSERT_EQUAL_STRING("a.png", pick("a.png 1x, b.png 1x", 0, 640, 200));     /* none >= 2, tie of the largest: the first */
}

static void test_bad_candidates_skipped(void) {
    TEST_ASSERT_EQUAL_STRING("b.png", pick("a.png 1x 2x, b.png 1x", 0, 640, 100));  /* two descriptors */
    TEST_ASSERT_EQUAL_STRING("b.png", pick("a.png 1x 2x, b.png 2x", 0, 640, 200));
    TEST_ASSERT_EQUAL_STRING("b.png", pick("a.png 1e2x, b.png 1x", 0, 640, 100));   /* not a plain number */
    TEST_ASSERT_EQUAL_STRING("b.png", pick("a.png 2q, b.png 1x", 0, 640, 100));
    TEST_ASSERT_EQUAL_STRING("b.png", pick("a.png 0x, b.png 1x", 0, 640, 100));
    TEST_ASSERT_EQUAL_STRING("b.png", pick("a.png 10.5w, b.png 1x", 0, 640, 100));  /* w must be whole */
    TEST_ASSERT_EQUAL_STRING("b.png", pick("a.png x, b.png 1x", 0, 640, 100));
    TEST_ASSERT_EQUAL_STRING("", pick("a.png 1y", 0, 640, 100));
    TEST_ASSERT_EQUAL_STRING("", pick("", 0, 640, 100));
    TEST_ASSERT_EQUAL_STRING("", pick(" , ,", 0, 640, 100));
    TEST_ASSERT_EQUAL(0, wf_srcset_pick(0, 0, 640, 100, g_url, (int)sizeof(g_url)));
    TEST_ASSERT_EQUAL(0, wf_srcset_pick("a.png", 0, 640, 100, g_url, 0));
    /* cut to cap */
    TEST_ASSERT_EQUAL(3, wf_srcset_pick("abcdef.png 1x", 0, 640, 100, g_url, 4));
    TEST_ASSERT_EQUAL_STRING("abc", g_url);
}

static void test_w_descriptors_and_sizes(void) {
    static const char *set = "s.jpg 320w, m.jpg 640w, l.jpg 1280w";
    /* slot 100vw of 640: densities 0.5, 1, 2 */
    TEST_ASSERT_EQUAL_STRING("m.jpg", pick(set, 0, 640, 100));
    TEST_ASSERT_EQUAL_STRING("l.jpg", pick(set, 0, 640, 200));
    TEST_ASSERT_EQUAL_STRING("s.jpg", pick(set, "320px", 640, 100));
    TEST_ASSERT_EQUAL_STRING("m.jpg", pick(set, "321px", 640, 100));
    TEST_ASSERT_EQUAL_STRING("s.jpg", pick(set, "50vw", 640, 100));
    TEST_ASSERT_EQUAL_STRING("s.jpg", pick(set, "(max-width: 700px) 50vw, 100vw", 640, 100));
    TEST_ASSERT_EQUAL_STRING("m.jpg", pick(set, "(max-width: 600px) 50vw, 100vw", 640, 100));
    TEST_ASSERT_EQUAL_STRING("s.jpg", pick(set, "20em", 640, 100));                /* 320 px */
    TEST_ASSERT_EQUAL_STRING("l.jpg", pick(set, "100vw", 1280, 100));
    TEST_ASSERT_EQUAL_STRING("s.jpg", pick(set, "0", 640, 100));                   /* slot 0: every density huge, the first */
}

static void test_sizes_width(void) {
    TEST_ASSERT_EQUAL(640, wf_sizes_width(0, 640));
    TEST_ASSERT_EQUAL(640, wf_sizes_width("", 640));
    TEST_ASSERT_EQUAL(200, wf_sizes_width("200px", 640));
    TEST_ASSERT_EQUAL(320, wf_sizes_width("50vw", 640));
    TEST_ASSERT_EQUAL(416, wf_sizes_width("65vw", 640));
    TEST_ASSERT_EQUAL(24, wf_sizes_width("1.5rem", 640));
    TEST_ASSERT_EQUAL(100, wf_sizes_width("(min-width: 600px) 100px, 300px", 640));
    TEST_ASSERT_EQUAL(300, wf_sizes_width("(min-width: 700px) 100px, 300px", 640));
    TEST_ASSERT_EQUAL(100, wf_sizes_width("(min-width: 600px) and (max-width: 800px) 100px, 300px", 640));
    TEST_ASSERT_EQUAL(300, wf_sizes_width("(min-width: 600px) and (max-width: 620px) 100px, 300px", 640));
    TEST_ASSERT_EQUAL(300, wf_sizes_width("bad, 300px", 640));                     /* a bad entry is skipped */
    TEST_ASSERT_EQUAL(300, wf_sizes_width("(min-width: 1px) 12q, 300px", 640));
    TEST_ASSERT_EQUAL(640, wf_sizes_width("calc(10px)", 640));
}

static void test_media(void) {
    TEST_ASSERT_TRUE(wf_media_match(0, 640));
    TEST_ASSERT_TRUE(wf_media_match("", 640));
    TEST_ASSERT_TRUE(wf_media_match("all", 640));
    TEST_ASSERT_TRUE(wf_media_match("screen", 640));
    TEST_ASSERT_FALSE(wf_media_match("print", 640));
    TEST_ASSERT_TRUE(wf_media_match("(min-width: 640px)", 640));
    TEST_ASSERT_FALSE(wf_media_match("(min-width: 641px)", 640));
    TEST_ASSERT_TRUE(wf_media_match("(max-width: 640px)", 640));
    TEST_ASSERT_FALSE(wf_media_match("(max-width: 639px)", 640));
    TEST_ASSERT_TRUE(wf_media_match("screen and (min-width: 600px)", 640));
    TEST_ASSERT_FALSE(wf_media_match("screen and (min-width: 700px)", 640));
    TEST_ASSERT_TRUE(wf_media_match("print, (max-width: 700px)", 640));            /* a list: any one */
    TEST_ASSERT_FALSE(wf_media_match("(orientation: landscape)", 640));
    TEST_ASSERT_FALSE(wf_media_match("(min-width 600px)", 640));
    TEST_ASSERT_FALSE(wf_media_match("(min-width: wide)", 640));
    TEST_ASSERT_TRUE(wf_media_match("( min-width :  40em )", 640));
}

static void test_types(void) {
    TEST_ASSERT_TRUE(wf_image_type_ok(0));
    TEST_ASSERT_TRUE(wf_image_type_ok(""));
    TEST_ASSERT_TRUE(wf_image_type_ok("image/png"));
    TEST_ASSERT_TRUE(wf_image_type_ok("IMAGE/JPEG"));
    TEST_ASSERT_TRUE(wf_image_type_ok("image/gif; charset=x"));
    TEST_ASSERT_TRUE(wf_image_type_ok("image/bmp"));
    TEST_ASSERT_TRUE(wf_image_type_ok("image/vnd.microsoft.icon"));
    TEST_ASSERT_FALSE(wf_image_type_ok("image/avif"));
    TEST_ASSERT_FALSE(wf_image_type_ok("image/svg+xml"));
    TEST_ASSERT_FALSE(wf_image_type_ok("image/pngx"));
}

static int reduce(const char *page, wf_html_stats_t *st) {
    wf_url_t base;
    if (wf_url_parse("https://ex.org/dir/page.html", &base) != 0) return -1;
    wf_html_set_images(1);
    return wf_html_reduce(page, (int)strlen(page), &base, g_out, (int)sizeof(g_out), st);
}

static void test_html_picture_and_srcset(void) {
    wf_html_stats_t st;
    wf_html_set_viewport(640, 100);
    reduce("<html><body><picture><source srcset=\"/x.avif\" type=\"image/avif\">"
           "<source media=\"(min-width: 900px)\" srcset=\"/wide.png\">"
           "<source srcset=\"/one.png 1x, /two.png 2x\" type=\"image/png\">"
           "<source srcset=\"/late.png\">"
           "<img src=\"/fallback.png\" alt=\"p\"></picture>"
           "<img src=\"/plain.png\" srcset=\"/s.jpg 320w, /m.jpg 640w\" sizes=\"50vw\">"
           "<img src=\"/only.png\"></body></html>", &st);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/one.png\""));
    TEST_ASSERT_NULL(strstr(g_out, "fallback.png"));
    TEST_ASSERT_NULL(strstr(g_out, "late.png"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/s.jpg\""));
    TEST_ASSERT_NULL(strstr(g_out, "plain.png"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/only.png\""));
    TEST_ASSERT_EQUAL(3, st.images);
    TEST_ASSERT_EQUAL(2, st.images_srcset);
    TEST_ASSERT_EQUAL(2, st.sources_skipped);
    /* a wide screen at 2x: the wide source, the larger srcset image */
    wf_html_set_viewport(1000, 200);
    reduce("<picture><source srcset=\"/x.avif\" type=\"image/avif\"><source media=\"(min-width: 900px)\" srcset=\"/wide.png\">"
           "<img src=\"/fallback.png\"></picture><img src=\"/plain.png\" srcset=\"/s.jpg 320w, /m.jpg 640w\" sizes=\"50vw\">", &st);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/wide.png\""));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/m.jpg\""));
    TEST_ASSERT_EQUAL(1, st.sources_skipped);
    wf_html_set_viewport(640, 100);
}

static void test_html_picture_fallback_and_scope(void) {
    wf_html_stats_t st;
    wf_html_set_viewport(0, 0);                 /* out of range: the defaults */
    /* no usable source: the img src */
    reduce("<picture><source srcset=\"/x.avif\" type=\"image/avif\"><source srcset=\"/q.png 3q\"><img src=\"/fallback.png\"></picture>", &st);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/fallback.png\""));
    TEST_ASSERT_EQUAL(2, st.sources_skipped);
    TEST_ASSERT_EQUAL(0, st.images_srcset);
    /* a source outside a picture, and an img after </picture>, use their own src */
    reduce("<source srcset=\"/stray.png\"><picture><source srcset=\"/in.png\"><img src=\"/a.png\"></picture><img src=\"/b.png\">", &st);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/in.png\""));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/b.png\""));
    TEST_ASSERT_NULL(strstr(g_out, "stray.png"));
    TEST_ASSERT_EQUAL(1, st.images_srcset);
    /* after </picture> a source is stray again */
    reduce("<picture></picture><source srcset=\"/stray.png\"><img src=\"/b.png\">", &st);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/b.png\""));
    TEST_ASSERT_EQUAL(0, st.images_srcset);
    /* the default viewport (640) after an out of range one */
    reduce("<picture><source media=\"(min-width: 600px)\" srcset=\"/w.png\"><img src=\"/n.png\"></picture>", &st);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/w.png\""));
    wf_html_set_viewport(9000, 900);           /* too large: the defaults */
    reduce("<picture><source media=\"(min-width: 600px)\" srcset=\"/w.png\"><img src=\"/n.png\"></picture>"
           "<img srcset=\"/1.png 1x, /2.png 2x\">", &st);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/w.png\""));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/1.png\""));
    wf_html_set_viewport(500, 100);
    reduce("<picture><source media=\"(min-width: 600px)\" srcset=\"/w.png\"><img src=\"/n.png\"></picture>", &st);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/n.png\""));
    wf_html_set_viewport(640, 100);
    /* a data: URL in srcset is not used; src then */
    reduce("<img src=\"/c.png\" srcset=\"data:image/png;base64,AAAA 1x\">", &st);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/c.png\""));
    /* &amp; in a srcset URL */
    reduce("<img srcset=\"/i?a=1&amp;b=2 1x\">", &st);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/i?a=1&b=2\""));
    /* a picture left open by one reduce does not leak into the next */
    reduce("<picture><source srcset=\"/open.png\">", &st);
    reduce("<img src=\"/d.png\">", &st);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "src=\"https://ex.org/d.png\""));
    TEST_ASSERT_EQUAL(0, st.images_srcset);
    /* images off: no picture logic */
    wf_html_set_images(0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_x_descriptors);
    RUN_TEST(test_bad_candidates_skipped);
    RUN_TEST(test_w_descriptors_and_sizes);
    RUN_TEST(test_sizes_width);
    RUN_TEST(test_media);
    RUN_TEST(test_types);
    RUN_TEST(test_html_picture_and_srcset);
    RUN_TEST(test_html_picture_fallback_and_scope);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
