/* Lot 72: images of the guest browser: wf_html keeps <img>, wf_webimg
 * fetches / decodes / scales them into the web atlas and rewrites the tags,
 * the web engine paints one OS_FB_WEB_IMAGE item per image. */
#include "../../framework/unity.h"
#include "wf_html.h"
#include "wf_webimg.h"
#include "webeng.h"
#include "imgdec.h"
#include "imgdec_samples.h"
#include "imgdec_gif_samples.h"
#include <string.h>
#include <stdio.h>

static wf_webimg_t g_wi;
static char g_out[32768];
static int g_fetches;
static char g_last_url[256];

typedef struct { const char* url; const unsigned char* data; int len; } fake_t;
static fake_t g_files[] = {
    {"https://ex.org/a.gif", gif_plain, sizeof(gif_plain)},
    {"https://ex.org/b.png", img_mire_png, sizeof(img_mire_png)},
    {"https://ex.org/c.jpg", img_mire444_jpg, sizeof(img_mire444_jpg)},
    {"https://ex.org/big.gif", gif_big, sizeof(gif_big)},
    {"https://ex.org/favicon.ico", gif_ico_bmp, sizeof(gif_ico_bmp)},
    {"https://ex.org/icon.png", gif_ico_png, sizeof(gif_ico_png)},
    {"https://ex.org/text.txt", (const unsigned char*)"hello world", 11},
    {0, 0, 0}
};

static int fake_fetch(void* ctx, const char* url, uint8_t* buf, int cap) {
    int k;
    (void)ctx;
    g_fetches++;
    snprintf(g_last_url, sizeof(g_last_url), "%s", url);
    for (k = 0; g_files[k].url; k++)
        if (strcmp(g_files[k].url, url) == 0) {
            if (g_files[k].len > cap) return cap + 1;
            memcpy(buf, g_files[k].data, (size_t)g_files[k].len);
            return g_files[k].len;
        }
    return -1;
}
static const wf_webimg_io_t g_io = {0, fake_fetch};

static void reset(void) { memset(&g_wi, 0, sizeof(g_wi)); wf_webimg_reset(&g_wi); g_fetches = 0; }

/* ---- wf_html keeps images --------------------------------------------- */
static void test_html_keeps_img_when_enabled(void) {
    static const char page[] =
        "<html><head><title>T</title><link rel=\"stylesheet\" href=\"/s.css\">"
        "<link rel=\"shortcut icon\" href=\"/static/fav.png\"></head><body>"
        "<p>a<img src=\"/a.gif\" width=\"40\" height=\"30\" alt=\"Logo &amp; co\">b</p>"
        "<img src=\"data:image/gif;base64,AAAA\" data-src=\"/lazy.png\">"
        "<img src=\"https://t.example/p.gif\" width=1 height=1>"
        "<img srcset=\"/s1.jpg 1x, /s2.jpg 2x\">"
        "<img src=\"javascript:alert(1)\"><img alt=\"none\">"
        "</body></html>";
    wf_url_t base;
    wf_html_stats_t st;
    int n;
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://ex.org/dir/page.html", &base));
    wf_html_set_images(0);
    n = wf_html_reduce(page, (int)strlen(page), &base, g_out, (int)sizeof(g_out), &st);
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_NULL(strstr(g_out, "<img"));
    TEST_ASSERT_EQUAL(0, (int)st.icon[0]);
    wf_html_set_images(1);
    n = wf_html_reduce(page, (int)strlen(page), &base, g_out, (int)sizeof(g_out), &st);
    wf_html_set_images(0);
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<img src=\"https://ex.org/a.gif\" width=\"40\" height=\"30\" alt=\"Logo   co\">"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<img src=\"https://ex.org/lazy.png\""));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<img src=\"https://ex.org/s1.jpg\""));
    TEST_ASSERT_NULL(strstr(g_out, "t.example"));
    TEST_ASSERT_NULL(strstr(g_out, "javascript"));
    TEST_ASSERT_NULL(strstr(g_out, "data:"));
    TEST_ASSERT_EQUAL(3, st.images);
    TEST_ASSERT_EQUAL(3, st.images_dropped);
    TEST_ASSERT_EQUAL_STRING("https://ex.org/static/fav.png", st.icon);
}

static void test_html_image_cap(void) {
    static char page[16384];
    wf_url_t base;
    wf_html_stats_t st;
    int k, p = 0;
    p += snprintf(page + p, sizeof(page) - (size_t)p, "<html><body>");
    for (k = 0; k < WF_HTML_IMG_MAX + 10; k++) p += snprintf(page + p, sizeof(page) - (size_t)p, "<img src=\"/i%d.png\">", k);
    p += snprintf(page + p, sizeof(page) - (size_t)p, "</body></html>");
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://ex.org/", &base));
    wf_html_set_images(1);
    wf_html_reduce_ex(page, p, &base, g_out, (int)sizeof(g_out), &st, 6000, 0);
    wf_html_set_images(0);
    TEST_ASSERT_EQUAL(WF_HTML_IMG_MAX, st.images);
    TEST_ASSERT_EQUAL(10, st.images_dropped);
}

/* ---- wf_webimg -------------------------------------------------------- */
static void slot_pixel(int slot, int x, int y, unsigned char* c) {
    const os_fb_webimg_slot_t* s = &g_wi.atlas.slot[slot];
    const uint8_t* p = g_wi.atlas.rgb + ((unsigned)s->off + (unsigned)y * s->w + (unsigned)x) * 3U;
    c[0] = p[0]; c[1] = p[1]; c[2] = p[2];
}

static void test_section_rewrites_and_fills_atlas(void) {
    static const char sec[] = "<html><body><p>x<img src=\"https://ex.org/a.gif\" alt=\"A\">"
                              "<img src=\"https://ex.org/b.png\" width=\"24\" alt=\"B\">"
                              "<img src=\"https://ex.org/c.jpg\" width=\"96\" height=\"64\">"
                              "<img src=\"https://ex.org/missing.png\" alt=\"Absent\">"
                              "<img src=\"https://ex.org/text.txt\"></p></body></html>";
    wf_webimg_stats_t st;
    unsigned char c[3];
    int n, w, h;
    reset();
    n = wf_webimg_section(&g_wi, sec, (int)strlen(sec), g_out, (int)sizeof(g_out), &g_io, &st);
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_EQUAL(5, st.found);
    TEST_ASSERT_EQUAL(3, st.shown);
    TEST_ASSERT_EQUAL(2, st.failed);
    TEST_ASSERT_EQUAL(5, st.fetched);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<img src=\"mohhdy-img:1\" width=\"23\" height=\"17\" style=\"width:23px;height:17px\">"));
    /* width only: height keeps the 48x32 ratio */
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<img src=\"mohhdy-img:2\" width=\"24\" height=\"16\""));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<img src=\"mohhdy-img:3\" width=\"96\" height=\"64\""));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "[image: Absent]"));
    TEST_ASSERT_NULL(strstr(g_out, "https://"));
    TEST_ASSERT_EQUAL(23, g_wi.atlas.slot[1].w);
    TEST_ASSERT_EQUAL(17, g_wi.atlas.slot[1].h);
    /* GIF stored 1:1: palette rule of the fixture */
    slot_pixel(1, 5, 3, c);
    TEST_ASSERT_EQUAL(((5 + 6) % 16) * 16, c[0]);
    TEST_ASSERT_EQUAL(255 - ((5 + 6) % 16) * 16, c[1]);
    TEST_ASSERT_EQUAL(0, wf_webimg_dims(&g_wi, 3, &w, &h));
    TEST_ASSERT_EQUAL(96, w);
    TEST_ASSERT_EQUAL(64, h);
    TEST_ASSERT_TRUE(wf_webimg_dims(&g_wi, 7, &w, &h) < 0);
    /* slots never overlap and stay inside the pixel array */
    TEST_ASSERT_TRUE(g_wi.atlas.slot[2].off >= g_wi.atlas.slot[1].off + 23U * 17U);
    TEST_ASSERT_TRUE(g_wi.used <= OS_FB_WEBIMG_PIXELS);
    TEST_ASSERT_EQUAL(OS_FB_WEBIMG_MAGIC, g_wi.atlas.magic);
}

static void test_section_reuses_by_url_and_budget(void) {
    static char sec[4096];
    wf_webimg_stats_t st;
    uint32_t gen;
    int p = 0, k;
    reset();
    p += snprintf(sec + p, sizeof(sec) - (size_t)p, "<p><img src=\"https://ex.org/a.gif\"><img src=\"https://ex.org/a.gif\"></p>");
    wf_webimg_section(&g_wi, sec, p, g_out, (int)sizeof(g_out), &g_io, &st);
    TEST_ASSERT_EQUAL(1, g_fetches);
    TEST_ASSERT_EQUAL(1, st.reused);
    TEST_ASSERT_EQUAL(2, st.shown);
    gen = g_wi.atlas.gen;
    wf_webimg_section(&g_wi, sec, p, g_out, (int)sizeof(g_out), &g_io, &st);
    TEST_ASSERT_EQUAL(1, g_fetches);          /* second section: no network */
    TEST_ASSERT_EQUAL(gen, g_wi.atlas.gen);   /* nothing changed: compositor not resent */
    /* fetch budget per section */
    reset();
    p = 0;
    for (k = 0; k < WF_WEBIMG_FETCH_MAX + 3; k++) p += snprintf(sec + p, sizeof(sec) - (size_t)p, "<img src=\"https://ex.org/n%d.png\" alt=\"n%d\">", k, k);
    wf_webimg_section(&g_wi, sec, p, g_out, (int)sizeof(g_out), &g_io, &st);
    TEST_ASSERT_EQUAL(WF_WEBIMG_FETCH_MAX, g_fetches);
    TEST_ASSERT_EQUAL(3, st.skipped);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "[image: n10]"));
}

static void test_big_image_box_and_pixel_budget(void) {
    static char sec[4096];
    wf_webimg_stats_t st;
    int p = 0, k, w, h, total = 0;
    reset();
    /* 200x150 in a 1000x750 box: shrunk to 480x360, stored smaller if needed */
    for (k = 0; k < 3; k++) p += snprintf(sec + p, sizeof(sec) - (size_t)p, "<img src=\"https://ex.org/big.gif?%d\" width=\"1000\" height=\"750\">", k);
    g_files[3].url = "https://ex.org/big.gif?0";
    wf_webimg_section(&g_wi, sec, p, g_out, (int)sizeof(g_out), &g_io, &st);
    TEST_ASSERT_EQUAL(1, st.shown);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "width=\"480\" height=\"360\""));
    TEST_ASSERT_EQUAL(0, wf_webimg_dims(&g_wi, 1, &w, &h));
    TEST_ASSERT_EQUAL(480, w);
    TEST_ASSERT_TRUE((int)g_wi.atlas.slot[1].w * g_wi.atlas.slot[1].h <= OS_FB_WEBIMG_PIXELS - 256);
    for (k = 0; k < OS_FB_WEBIMG_SLOTS; k++) total += g_wi.atlas.slot[k].w * g_wi.atlas.slot[k].h;
    TEST_ASSERT_TRUE(total <= OS_FB_WEBIMG_PIXELS);
    g_files[3].url = "https://ex.org/big.gif";
    wf_webimg_box(0, 0, 100, 50, &w, &h);  TEST_ASSERT_EQUAL(100, w); TEST_ASSERT_EQUAL(50, h);
    wf_webimg_box(0, 20, 100, 50, &w, &h); TEST_ASSERT_EQUAL(40, w); TEST_ASSERT_EQUAL(20, h);
    wf_webimg_box(2000, 0, 100, 50, &w, &h); TEST_ASSERT_EQUAL(480, w); TEST_ASSERT_EQUAL(240, h);
}

static void test_compaction_keeps_used_slots(void) {
    static char s1[1024], s2[1024];
    wf_webimg_stats_t st;
    unsigned char before[3], after[3];
    int p1, p2;
    reset();
    p1 = snprintf(s1, sizeof(s1), "<img src=\"https://ex.org/big.gif\" width=\"220\" height=\"160\"><img src=\"https://ex.org/a.gif\">");
    wf_webimg_section(&g_wi, s1, p1, g_out, (int)sizeof(g_out), &g_io, &st);
    TEST_ASSERT_EQUAL(2, st.shown);
    TEST_ASSERT_TRUE(g_wi.used > OS_FB_WEBIMG_PIXELS / 2);
    slot_pixel(2, 4, 4, before);
    /* next section only needs a.gif: big.gif is dropped and a.gif moved down */
    p2 = snprintf(s2, sizeof(s2), "<img src=\"https://ex.org/a.gif\"><img src=\"https://ex.org/b.png\">");
    wf_webimg_section(&g_wi, s2, p2, g_out, (int)sizeof(g_out), &g_io, &st);
    TEST_ASSERT_EQUAL(1, st.reused);
    TEST_ASSERT_EQUAL(256U, g_wi.atlas.slot[2].off);
    TEST_ASSERT_EQUAL(256U, g_wi.atlas.slot[2].off);
    slot_pixel(2, 4, 4, after);
    TEST_ASSERT_EQUAL(0, memcmp(before, after, 3));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "mohhdy-img:2"));
}

static void test_favicon(void) {
    wf_webimg_stats_t st;
    unsigned char c[3];
    reset();
    memset(&st, 0, sizeof(st));
    TEST_ASSERT_EQUAL(0, wf_webimg_favicon(&g_wi, "https://ex.org/dir/x.html?q=1", 0, &g_io, &st));
    TEST_ASSERT_EQUAL_STRING("https://ex.org/favicon.ico", g_last_url);
    TEST_ASSERT_EQUAL(16, g_wi.atlas.slot[0].w);
    TEST_ASSERT_EQUAL(0U, g_wi.atlas.slot[0].off);
    slot_pixel(0, 0, 0, c);                      /* transparent column: white */
    TEST_ASSERT_EQUAL(255, c[0]);
    TEST_ASSERT_EQUAL(0, wf_webimg_favicon(&g_wi, "https://ex.org/", "https://ex.org/icon.png", &g_io, &st));
    TEST_ASSERT_EQUAL_STRING("https://ex.org/icon.png", g_last_url);
    TEST_ASSERT_TRUE(wf_webimg_favicon(&g_wi, "https://none.org/", 0, &g_io, &st) < 0);
    TEST_ASSERT_EQUAL(0, g_wi.atlas.slot[0].w);
    TEST_ASSERT_TRUE(wf_webimg_favicon(&g_wi, "nourl", 0, &g_io, &st) < 0);
}

static void test_output_too_small(void) {
    static const char sec[] = "<p>hello<img src=\"https://ex.org/a.gif\"></p>";
    char small[12];
    reset();
    TEST_ASSERT_EQUAL(-1, wf_webimg_section(&g_wi, sec, (int)strlen(sec), small, (int)sizeof(small), &g_io, 0));
    TEST_ASSERT_TRUE(strlen(small) < sizeof(small));
}

/* ---- web engine: one atlas item per image ---------------------------- */
static we_doc_t g_doc;
static int atlas_dims(void* ctx, int slot, int* w, int* h) {
    (void)ctx;
    if (slot != 3) return -1;
    *w = 120; *h = 80;
    return 0;
}

static void test_engine_paints_atlas_item(void) {
    static const char html[] = "<html><body><p>avant</p><img src=\"mohhdy-img:3\" style=\"width:60px;height:40px\">"
                               "<img src=\"mohhdy-img:3\"><img src=\"mohhdy-img:5\" width=\"10\" height=\"10\" alt=\"vide\"></body></html>";
    int k, found = 0, big = 0;
    we_set_image_atlas(atlas_dims, 0);
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)strlen(html), 640));
    for (k = 0; k < (int)g_doc.nitems; k++) {
        const os_fb_web_item_t* it = &g_doc.items[k];
        if (it->kind != OS_FB_WEB_IMAGE) continue;
        TEST_ASSERT_EQUAL(3, it->pad);
        if (it->w == 60 && it->h == 40) found++;
        if (it->w == 120 && it->h == 80) big++;
    }
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(1, big);
    TEST_ASSERT_EQUAL(2, (int)g_doc.perf.atlas_images);
    TEST_ASSERT_TRUE(g_doc.perf.image_refused >= 1);   /* slot 5 empty: refused, alt shown */
    we_set_image_atlas(0, 0);
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)strlen(html), 640));
    TEST_ASSERT_EQUAL(0, (int)g_doc.perf.atlas_images);
}

int main(void) {
    unity_init();
    RUN_TEST(test_html_keeps_img_when_enabled);
    RUN_TEST(test_html_image_cap);
    RUN_TEST(test_section_rewrites_and_fills_atlas);
    RUN_TEST(test_section_reuses_by_url_and_budget);
    RUN_TEST(test_big_image_box_and_pixel_budget);
    RUN_TEST(test_compaction_keeps_used_slots);
    RUN_TEST(test_favicon);
    RUN_TEST(test_output_too_small);
    RUN_TEST(test_engine_paints_atlas_item);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
