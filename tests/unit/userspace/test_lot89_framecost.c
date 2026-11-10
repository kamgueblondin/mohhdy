/* Lot 89 (US-081): what the idle paints of the desktop cost, per
 * dim level, and the paint time the pacer did not spend. */
#include "../../framework/unity.h"
#include <string.h>
#include "osui_frame.h"

static osui_frame_t g_f;
static char g_o[256];

static void reset(void) { memset(&g_f, 0, sizeof(g_f)); memset(g_o, 0, sizeof(g_o)); }

/* one loop pass as osui_gui_run does it: ask, then paint and time it */
static int pass(uint32_t now, int input, uint32_t hz, int dim, uint32_t cost) {
    int p = osui_frame_due(&g_f, now, input, hz, dim);
    if (p) osui_frame_cost(&g_f, now, now + cost, !input);
    return p;
}

/* saver with a 3 s dim: 15 Hz, 7 Hz after 3 s, 5 Hz after 6 s; the loop
 * wakes every 100 ms and a frame costs 40 ms */
static uint32_t saver_run(uint32_t t0) {
    uint32_t t = t0;
    int i;
    osui_frame_init(&g_f, t0);
    (void)pass(t, 1, 15, 0, 40);   /* the key: always painted */
    for (i = 0; i < 30; i++) { t += 100U; (void)pass(t, 0, 15, 0, 40); }
    for (i = 0; i < 30; i++) { t += 100U; (void)pass(t, 0, 7, 1, 40); }
    for (i = 0; i < 40; i++) { t += 100U; (void)pass(t, 0, 5, 2, 40); }
    return t;
}

static void test_cost_per_level(void) {
    reset();
    (void)saver_run(1000U);
    TEST_ASSERT_EQUAL(1U, g_f.input_frames);
    /* 30 paints awake, 15 dimmed (every other wake), 20 deep */
    TEST_ASSERT_EQUAL(30U, g_f.paint_frames[0]);
    TEST_ASSERT_EQUAL(15U, g_f.paint_frames[1]);
    TEST_ASSERT_EQUAL(20U, g_f.paint_frames[2]);
    TEST_ASSERT_EQUAL(1200U, g_f.paint_ms[0]);
    TEST_ASSERT_EQUAL(600U, g_f.paint_ms[1]);
    TEST_ASSERT_EQUAL(800U, g_f.paint_ms[2]);
    /* the interval before a call belongs to the level of the call before */
    TEST_ASSERT_EQUAL(3100U, g_f.wall_ms[0]);
    TEST_ASSERT_EQUAL(3000U, g_f.wall_ms[1]);
    TEST_ASSERT_EQUAL(3900U, g_f.wall_ms[2]);
    TEST_ASSERT_EQUAL(387U, osui_frame_busy_permille(&g_f, 0));
    TEST_ASSERT_EQUAL(200U, osui_frame_busy_permille(&g_f, 1));
    TEST_ASSERT_EQUAL(205U, osui_frame_busy_permille(&g_f, 2));
    TEST_ASSERT_EQUAL(0U, osui_frame_busy_permille(&g_f, 3));
    TEST_ASSERT_EQUAL(0U, osui_frame_busy_permille(&g_f, -1));
}

static void test_saved_and_average(void) {
    reset();
    (void)saver_run(1000U);
    /* 66 frames (the input one too) at 40 ms; 35 idle wakes skipped */
    TEST_ASSERT_EQUAL(66U, g_f.cost_frames);
    TEST_ASSERT_EQUAL(40000U, osui_frame_cost_us(&g_f));
    TEST_ASSERT_EQUAL(35U, g_f.idle_skipped);
    TEST_ASSERT_EQUAL(1400U, osui_frame_saved_ms(&g_f));
    TEST_ASSERT(osui_frame_cost_report(&g_f, g_o, (int)sizeof(g_o)) > 0);
    TEST_ASSERT_EQUAL_STRING("paint_ms 1200 600 800 wall_ms 3100 3000 3900 busy_permille 387 200 205 "
                             "cost_us 40000 saved_ms 1400", g_o);
}

static void test_input_resets_levels_not_average(void) {
    uint32_t t;
    reset();
    t = saver_run(1000U);
    /* a key: the per level figures start again, the frame cost stays */
    TEST_ASSERT_EQUAL(1, pass(t + 50U, 1, 15, 0, 20));
    TEST_ASSERT_EQUAL(0U, g_f.paint_ms[2]);
    TEST_ASSERT_EQUAL(0U, g_f.wall_ms[2]);
    TEST_ASSERT_EQUAL(0U, g_f.paint_frames[0]);
    TEST_ASSERT_EQUAL(67U, g_f.cost_frames);
    TEST_ASSERT_EQUAL(2660U, g_f.cost_ms);
    TEST_ASSERT_EQUAL(39701U, osui_frame_cost_us(&g_f));
    TEST_ASSERT_EQUAL(0U, osui_frame_saved_ms(&g_f));
    /* new content painted as input is not an idle frame */
    osui_frame_cost(&g_f, t + 60U, t + 70U, 0);
    TEST_ASSERT_EQUAL(0U, g_f.paint_frames[0]);
}

static void test_unpaced_is_busier(void) {
    uint32_t t = 0;
    int i;
    reset();
    osui_frame_init(&g_f, t);
    (void)pass(t, 1, 0, 0, 40);
    /* no budget (no tick source or performance): every wake paints */
    for (i = 0; i < 40; i++) { t += 100U; (void)pass(t, 0, 0, 2, 40); }
    TEST_ASSERT_EQUAL(40U, g_f.paint_frames[2]);
    TEST_ASSERT_EQUAL(0U, osui_frame_saved_ms(&g_f));
    /* 40 paints in 3900 ms at level 2 (the first interval was level 0) */
    TEST_ASSERT_EQUAL(410U, osui_frame_busy_permille(&g_f, 2));
    /* the paced deep dim of the saver run spends half of that */
    reset();
    (void)saver_run(0U);
    TEST_ASSERT(osui_frame_busy_permille(&g_f, 2) * 2U <= 410U + 2U);
}

static void test_bounds(void) {
    reset();
    /* nothing before init */
    osui_frame_cost(&g_f, 0U, 10U, 1);
    TEST_ASSERT_EQUAL(0U, g_f.cost_frames);
    TEST_ASSERT_EQUAL(0U, osui_frame_cost_us(&g_f));
    osui_frame_init(&g_f, 0xFFFFFFF0U);
    /* a tick wrap between the reads is still 32 ms */
    osui_frame_cost(&g_f, 0xFFFFFFF0U, 0x10U, 1);
    TEST_ASSERT_EQUAL(32U, g_f.paint_ms[0]);
    /* an end before the start is a clock that went back: 0 */
    osui_frame_cost(&g_f, 500U, 100U, 1);
    TEST_ASSERT_EQUAL(32U, g_f.paint_ms[0]);
    TEST_ASSERT_EQUAL(2U, g_f.paint_frames[0]);
    /* paint charged past the wall time reads 1000, not more */
    g_f.wall_ms[0] = 10U;
    TEST_ASSERT_EQUAL(1000U, osui_frame_busy_permille(&g_f, 0));
    /* huge counters keep 32-bit arithmetic */
    g_f.paint_ms[1] = 3000000000U;
    g_f.wall_ms[1] = 4000000000U;
    /* (halving both rounds down: 749, not 750) */
    TEST_ASSERT_EQUAL(749U, osui_frame_busy_permille(&g_f, 1));
    g_f.paint_ms[1] = 0xFFFFFFF0U;
    osui_frame_cost(&g_f, 0U, 40U, 1);
    g_f.dim = 1;
    osui_frame_cost(&g_f, 0U, 40U, 1);
    TEST_ASSERT_EQUAL(0xFFFFFFFFU, g_f.paint_ms[1]);
    /* the running average halves before it overflows */
    g_f.cost_frames = 0x100000U;
    g_f.cost_ms = 0x100000U * 5U;
    osui_frame_cost(&g_f, 0U, 5U, 0);
    TEST_ASSERT_EQUAL(0x80001U, g_f.cost_frames);
    TEST_ASSERT_EQUAL(5000U, osui_frame_cost_us(&g_f));
    g_f.cost_ms = 0xFFFFFFF0U;
    g_f.cost_frames = 1000U;
    TEST_ASSERT_EQUAL(4294967000U, osui_frame_cost_us(&g_f));
    /* saved time saturates */
    g_f.idle_skipped = 0xFFFFFFFFU;
    TEST_ASSERT_EQUAL(0xFFFFFFFFU / 1000U, osui_frame_saved_ms(&g_f));
    /* a short buffer is cut, not overrun */
    g_o[12] = 'X';
    (void)osui_frame_cost_report(&g_f, g_o, 9);
    TEST_ASSERT_EQUAL_STRING("paint_ms", g_o);
    TEST_ASSERT_EQUAL('X', g_o[12]);
    TEST_ASSERT_EQUAL(0, osui_frame_cost_report(0, g_o, 9));
    TEST_ASSERT_EQUAL(0, osui_frame_cost_report(&g_f, 0, 9));
    TEST_ASSERT_EQUAL(0U, osui_frame_saved_ms(0));
}

int main(void) {
    unity_init();
    RUN_TEST(test_cost_per_level);
    RUN_TEST(test_saved_and_average);
    RUN_TEST(test_input_resets_levels_not_average);
    RUN_TEST(test_unpaced_is_busier);
    RUN_TEST(test_bounds);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
