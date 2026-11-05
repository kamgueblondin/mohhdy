/* Lot 73: animated GIFs in the guest browser (wf_webimg): every frame is
 * stored scaled to the slot, wf_webimg_tick copies the frame that is due
 * into the atlas (generation bumped for the compositor), with the delays
 * (0/1 played as 10 cs), the loop count (once, forever, N + 1 plays, then
 * the last frame stays), the animation and frame store budgets, the
 * packing of the store when the atlas is compacted, and the status line. */
#include "../../framework/unity.h"
#include "wf_webimg.h"
#include "webeng.h"
#include "imgdec.h"
#include "imgdec_samples.h"
#include "imgdec_anim_samples.h"
#include <string.h>
#include <stdio.h>

static wf_webimg_t g_wi;
static char g_out[32768], g_txt[1024];

typedef struct { const char* url; const unsigned char* data; int len; } fake_t;
static unsigned char g_delays[sizeof(gif_anim_a)];   /* anim_a with delays 1 and 2 (filled by the test) */
static fake_t g_files[] = {
    {"https://ex.org/d.gif", g_delays, sizeof(g_delays)},
    {"https://ex.org/page.gif", gif_anim_page, sizeof(gif_anim_page)},
    {"https://ex.org/twice.gif", gif_anim_twice, sizeof(gif_anim_twice)},
    {"https://ex.org/a.gif", gif_anim_a, sizeof(gif_anim_a)},
    {"https://ex.org/b.gif", gif_anim_b, sizeof(gif_anim_b)},
    {"https://ex.org/p1.gif", gif_anim_page, sizeof(gif_anim_page)},
    {"https://ex.org/p2.gif", gif_anim_page, sizeof(gif_anim_page)},
    {"https://ex.org/p3.gif", gif_anim_page, sizeof(gif_anim_page)},
    {"https://ex.org/p4.gif", gif_anim_page, sizeof(gif_anim_page)},
    {"https://ex.org/p5.gif", gif_anim_page, sizeof(gif_anim_page)},
    {"https://ex.org/m.png", img_mire_png, sizeof(img_mire_png)},
    {0, 0, 0}
};

static int fake_fetch(void* ctx, const char* url, uint8_t* buf, int cap) {
    int k;
    (void)ctx;
    for (k = 0; g_files[k].url; k++)
        if (strcmp(g_files[k].url, url) == 0) {
            memcpy(buf, g_files[k].data, (size_t)g_files[k].len);
            return g_files[k].len < cap ? g_files[k].len : cap + 1;
        }
    return -1;
}
static const wf_webimg_io_t g_io = {0, fake_fetch};

static void reset(void) { memset(&g_wi, 0, sizeof(g_wi)); wf_webimg_reset(&g_wi); }

static int section(const char* html, wf_webimg_stats_t* st) {
    return wf_webimg_section(&g_wi, html, (int)strlen(html), g_out, (int)sizeof(g_out), &g_io, st);
}

/* first pixel of a slot in the atlas */
static const uint8_t* px(int slot, int x, int y) {
    const os_fb_webimg_slot_t* s = &g_wi.atlas.slot[slot];
    return g_wi.atlas.rgb + (s->off + (uint32_t)y * s->w + (uint32_t)x) * 3U;
}

static void assert_rgb(const uint8_t* p, int r, int g, int b) {
    TEST_ASSERT_EQUAL(r, p[0]);
    TEST_ASSERT_EQUAL(g, p[1]);
    TEST_ASSERT_EQUAL(b, p[2]);
}

static void test_forever_three_frames(void) {
    wf_webimg_stats_t st;
    uint32_t gen;
    reset();
    TEST_ASSERT_TRUE(section("<p><img src=\"https://ex.org/page.gif\"></p>", &st) > 0);
    TEST_ASSERT_EQUAL(1, st.shown);
    TEST_ASSERT_EQUAL(1, st.animated);
    TEST_ASSERT_EQUAL(3, st.anim_frames);
    TEST_ASSERT_EQUAL(0, st.anim_cut);
    TEST_ASSERT_EQUAL(1, g_wi.anim[0].slot);
    TEST_ASSERT_EQUAL(32, g_wi.anim[0].w);
    TEST_ASSERT_EQUAL(24, g_wi.anim[0].h);
    TEST_ASSERT_EQUAL(3U * 32U * 24U, g_wi.anim_used);
    assert_rgb(px(1, 0, 0), 220, 20, 30);
    gen = g_wi.atlas.gen;
    TEST_ASSERT_EQUAL(0, wf_webimg_tick(&g_wi, 1000));     /* starts the clock */
    TEST_ASSERT_EQUAL(0, wf_webimg_tick(&g_wi, 1049));
    TEST_ASSERT_EQUAL(gen, g_wi.atlas.gen);
    TEST_ASSERT_EQUAL(1, wf_webimg_tick(&g_wi, 1050));
    TEST_ASSERT_TRUE(g_wi.atlas.gen != gen);
    TEST_ASSERT_EQUAL(g_wi.gen, g_wi.atlas.gen);
    assert_rgb(px(1, 31, 23), 10, 200, 40);
    TEST_ASSERT_EQUAL(0, wf_webimg_tick(&g_wi, 1099));
    TEST_ASSERT_EQUAL(1, wf_webimg_tick(&g_wi, 1100));
    assert_rgb(px(1, 5, 5), 20, 40, 230);
    TEST_ASSERT_EQUAL(1, wf_webimg_tick(&g_wi, 1150));       /* loops */
    assert_rgb(px(1, 5, 5), 220, 20, 30);
    TEST_ASSERT_EQUAL(1, g_wi.anim[0].plays);
    TEST_ASSERT_EQUAL(0, g_wi.anim[0].stopped);
    TEST_ASSERT_EQUAL(3U, g_wi.anim_changes);
}

static void test_loop_one_plays_twice_then_stays(void) {
    wf_webimg_stats_t st;
    int t, changes = 0;
    reset();
    section("<img src=\"https://ex.org/twice.gif\">", &st);
    TEST_ASSERT_EQUAL(1, st.animated);
    TEST_ASSERT_EQUAL(1, g_wi.anim[0].loop);
    wf_webimg_tick(&g_wi, 0);
    for (t = 1; t <= 400; t++) changes += wf_webimg_tick(&g_wi, (uint32_t)t);
    /* frames 0 1 0 1: three changes, then stopped on frame 1 (green -> red) */
    TEST_ASSERT_EQUAL(3, changes);
    TEST_ASSERT_EQUAL(1, g_wi.anim[0].stopped);
    TEST_ASSERT_EQUAL(1, g_wi.anim[0].cur);
    TEST_ASSERT_EQUAL(2, g_wi.anim[0].plays);
    assert_rgb(px(1, 0, 0), 220, 20, 30);
    TEST_ASSERT_EQUAL(0, wf_webimg_tick(&g_wi, 100000));
}

static void test_no_loop_plays_once(void) {
    wf_webimg_stats_t st;
    int t, changes = 0;
    reset();
    section("<img src=\"https://ex.org/b.gif\">", &st);
    TEST_ASSERT_EQUAL(1, st.animated);
    TEST_ASSERT_EQUAL(-1, g_wi.anim[0].loop);
    TEST_ASSERT_EQUAL(7, g_wi.anim[0].delay[0]);
    TEST_ASSERT_EQUAL(9, g_wi.anim[0].delay[1]);
    wf_webimg_tick(&g_wi, 0);
    for (t = 1; t <= 100; t++) changes += wf_webimg_tick(&g_wi, (uint32_t)t);
    TEST_ASSERT_EQUAL(1, changes);
    TEST_ASSERT_EQUAL(1, g_wi.anim[0].stopped);
    TEST_ASSERT_EQUAL(1, g_wi.anim[0].cur);
    assert_rgb(px(1, 3, 3), 20, 40, 230);      /* frame 2 blue, clipped corner */
    assert_rgb(px(1, 0, 0), 10, 200, 40);
}

static void test_scaled_frames_min_delay_and_text(void) {
    wf_webimg_stats_t st;
    int n;
    reset();
    section("<img src=\"https://ex.org/a.gif\" width=\"16\" height=\"12\">", &st);
    TEST_ASSERT_EQUAL(4, st.anim_frames);
    TEST_ASSERT_EQUAL(16, g_wi.anim[0].w);
    TEST_ASSERT_EQUAL(12, g_wi.anim[0].h);
    TEST_ASSERT_EQUAL(10, g_wi.anim[0].delay[0]);
    TEST_ASSERT_EQUAL(20, g_wi.anim[0].delay[1]);
    TEST_ASSERT_EQUAL(WF_WEBIMG_ANIM_MIN_DELAY, g_wi.anim[0].delay[2]);   /* 0 in the file */
    TEST_ASSERT_EQUAL(5, g_wi.anim[0].delay[3]);
    /* frame 2 of the store, scaled 2x: green rectangle at (4..9, 2..5) */
    {
        const uint8_t* f1 = g_wi.anim_rgb + (g_wi.anim[0].off[1] + 2U * 16U + 4U) * 3U;
        assert_rgb(f1, 10, 200, 40);
        TEST_ASSERT_EQUAL(16U * 12U, g_wi.anim[0].off[1]);
    }
    n = wf_webimg_anim_text(&g_wi, g_txt, (int)sizeof(g_txt));
    TEST_ASSERT_EQUAL((int)strlen(g_txt), n);
    TEST_ASSERT_EQUAL_STRING("anim slot 1 frames 4 cur 0 plays 0 loop 3 delays 10,20,10,5 stopped 0 size 16x12\n", g_txt);
    reset();
    section("<img src=\"https://ex.org/b.gif\">", &st);
    wf_webimg_anim_text(&g_wi, g_txt, (int)sizeof(g_txt));
    TEST_ASSERT_NOT_NULL(strstr(g_txt, " loop once "));
    reset();
    section("<img src=\"https://ex.org/page.gif\">", &st);
    wf_webimg_anim_text(&g_wi, g_txt, (int)sizeof(g_txt));
    TEST_ASSERT_NOT_NULL(strstr(g_txt, " loop forever "));
    TEST_ASSERT_EQUAL(0, wf_webimg_anim_text(&g_wi, g_txt, 0));
}

static void test_budgets(void) {
    wf_webimg_stats_t st;
    reset();
    /* five animations: four entries, the fifth stays still */
    section("<img src=\"https://ex.org/p1.gif\"><img src=\"https://ex.org/p2.gif\"><img src=\"https://ex.org/p3.gif\">"
            "<img src=\"https://ex.org/p4.gif\"><img src=\"https://ex.org/p5.gif\">", &st);
    TEST_ASSERT_EQUAL(5, st.shown);
    TEST_ASSERT_EQUAL(4, st.animated);
    TEST_ASSERT_EQUAL(1, st.anim_cut);
    TEST_ASSERT_EQUAL(12, st.anim_frames);
    /* store nearly full: only one frame fits, the image stays still and
     * the store is given back */
    reset();
    g_wi.anim_used = WF_WEBIMG_ANIM_PIXELS - 32U * 24U - 10U;
    section("<img src=\"https://ex.org/page.gif\">", &st);
    TEST_ASSERT_EQUAL(1, st.shown);
    TEST_ASSERT_EQUAL(0, st.animated);
    TEST_ASSERT_EQUAL(1, st.anim_cut);
    TEST_ASSERT_EQUAL(WF_WEBIMG_ANIM_PIXELS - 32U * 24U - 10U, g_wi.anim_used);
    TEST_ASSERT_EQUAL(0, g_wi.anim[0].slot);
    /* two frames fit, the third does not: kept with two (cut counted) */
    reset();
    g_wi.anim_used = WF_WEBIMG_ANIM_PIXELS - 2U * 32U * 24U;
    section("<img src=\"https://ex.org/page.gif\">", &st);
    TEST_ASSERT_EQUAL(1, st.animated);
    TEST_ASSERT_EQUAL(2, st.anim_frames);
    TEST_ASSERT_EQUAL(1, st.anim_cut);
    TEST_ASSERT_EQUAL(WF_WEBIMG_ANIM_PIXELS, g_wi.anim_used);
}

static void test_reset_and_compaction_drop_animations(void) {
    wf_webimg_stats_t st;
    reset();
    section("<img src=\"https://ex.org/m.png\" width=\"240\" height=\"180\"><img src=\"https://ex.org/a.gif\">"
            "<img src=\"https://ex.org/page.gif\">", &st);
    TEST_ASSERT_EQUAL(2, st.animated);
    TEST_ASSERT_EQUAL(4U * 8U * 6U + 3U * 32U * 24U, g_wi.anim_used);
    TEST_ASSERT_TRUE(g_wi.used > OS_FB_WEBIMG_PIXELS / 2);
    /* the next section keeps page.gif only: the store is packed */
    section("<img src=\"https://ex.org/page.gif\">", &st);
    TEST_ASSERT_EQUAL(1, st.reused);
    TEST_ASSERT_EQUAL(0, g_wi.anim[0].slot);
    TEST_ASSERT_EQUAL(3, g_wi.anim[1].slot);
    TEST_ASSERT_EQUAL(0U, g_wi.anim[1].off[0]);
    TEST_ASSERT_EQUAL(32U * 24U, g_wi.anim[1].off[1]);
    TEST_ASSERT_EQUAL(3U * 32U * 24U, g_wi.anim_used);
    /* packed frames are the same pixels */
    TEST_ASSERT_EQUAL(10, g_wi.anim_rgb[g_wi.anim[1].off[1] * 3U]);
    TEST_ASSERT_EQUAL(200, g_wi.anim_rgb[g_wi.anim[1].off[1] * 3U + 1]);
    /* still animated after the move */
    wf_webimg_tick(&g_wi, 0);
    TEST_ASSERT_EQUAL(1, wf_webimg_tick(&g_wi, 50));
    assert_rgb(px(3, 0, 0), 10, 200, 40);
    /* a new page */
    wf_webimg_reset(&g_wi);
    TEST_ASSERT_EQUAL(0U, g_wi.anim_used);
    TEST_ASSERT_EQUAL(0, g_wi.anim[1].slot);
    TEST_ASSERT_EQUAL(0, wf_webimg_tick(&g_wi, 100));
    /* not a web atlas: nothing */
    g_wi.atlas.magic = 0;
    TEST_ASSERT_EQUAL(0, wf_webimg_tick(&g_wi, 200));
}

static void test_tick_wrap_and_no_burst(void) {
    wf_webimg_stats_t st;
    reset();
    section("<img src=\"https://ex.org/page.gif\">", &st);
    wf_webimg_tick(&g_wi, 0xFFFFFFF0U);
    TEST_ASSERT_EQUAL(0, wf_webimg_tick(&g_wi, 0xFFFFFFFFU));
    TEST_ASSERT_EQUAL(1, wf_webimg_tick(&g_wi, 0x22U));          /* 0xFFFFFFF0 + 50 wraps */
    TEST_ASSERT_EQUAL(1, g_wi.anim[0].cur);
    /* a long pause: one change, the next one a delay after now */
    TEST_ASSERT_EQUAL(1, wf_webimg_tick(&g_wi, 100000));
    TEST_ASSERT_EQUAL(2, g_wi.anim[0].cur);
    TEST_ASSERT_EQUAL(100050U, g_wi.anim[0].due);
    TEST_ASSERT_EQUAL(0, wf_webimg_tick(&g_wi, 100001));
    TEST_ASSERT_EQUAL(1, wf_webimg_tick(&g_wi, 100050));
    TEST_ASSERT_EQUAL(100100U, g_wi.anim[0].due);
}

static void test_still_gif_and_other_formats_not_animated(void) {
    wf_webimg_stats_t st;
    reset();
    section("<img src=\"https://ex.org/m.png\">", &st);
    TEST_ASSERT_EQUAL(1, st.shown);
    TEST_ASSERT_EQUAL(0, st.animated);
    TEST_ASSERT_EQUAL(0, st.anim_cut);
    TEST_ASSERT_EQUAL(0, wf_webimg_tick(&g_wi, 10));
    TEST_ASSERT_EQUAL(0, wf_webimg_tick(&g_wi, 1000));
}

/* Delays of 1 cs play at the minimum (as 0), 2 cs stay 2 cs. */
static void test_delay_one_is_minimum(void) {
    wf_webimg_stats_t st;
    int k, gce = 0;
    memcpy(g_delays, gif_anim_a, sizeof(g_delays));
    for (k = 0; k + 5 < (int)sizeof(g_delays); k++)
        if (g_delays[k] == 0x21 && g_delays[k + 1] == 0xF9 && g_delays[k + 2] == 4) {
            gce++;
            if (gce == 3) { TEST_ASSERT_EQUAL(0, g_delays[k + 4]); g_delays[k + 4] = 1; }
            if (gce == 4) { TEST_ASSERT_EQUAL(5, g_delays[k + 4]); g_delays[k + 4] = 2; }
        }
    TEST_ASSERT_EQUAL(4, gce);
    reset();
    section("<img src=\"https://ex.org/d.gif\">", &st);
    TEST_ASSERT_EQUAL(4, st.anim_frames);
    TEST_ASSERT_EQUAL(WF_WEBIMG_ANIM_MIN_DELAY, g_wi.anim[0].delay[2]);
    TEST_ASSERT_EQUAL(2, g_wi.anim[0].delay[3]);
}

int main(void) {
    unity_init();
    RUN_TEST(test_forever_three_frames);
    RUN_TEST(test_loop_one_plays_twice_then_stays);
    RUN_TEST(test_no_loop_plays_once);
    RUN_TEST(test_scaled_frames_min_delay_and_text);
    RUN_TEST(test_budgets);
    RUN_TEST(test_reset_and_compaction_drop_animations);
    RUN_TEST(test_tick_wrap_and_no_burst);
    RUN_TEST(test_still_gif_and_other_formats_not_animated);
    RUN_TEST(test_delay_one_is_minimum);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
