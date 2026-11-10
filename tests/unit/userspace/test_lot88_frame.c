/* Lot 88 (US-081): the desktop frame pacer and the idle dim timeout. */
#include "../../framework/unity.h"
#include <string.h>
#include "osui_frame.h"
#include "plat_energy.h"
#include "platform.h"

static osui_frame_t g_f;
static char g_o[256];

static void reset(void) { memset(&g_f, 0, sizeof(g_f)); memset(g_o, 0, sizeof(g_o)); }

/* the loop wakes every 100 ms with no key, as under the kernel getc */
static int idle_run(uint32_t *now, int passes, uint32_t hz, int dim) {
    int i, painted = 0;
    for (i = 0; i < passes; i++) {
        *now += 100U;
        painted += osui_frame_due(&g_f, *now, 0, hz, dim);
    }
    return painted;
}

static void test_unpaced_paints_every_pass(void) {
    reset();
    uint32_t now = 5000U;
    osui_frame_init(&g_f, now);
    TEST_ASSERT_EQUAL(20, idle_run(&now, 20, 0, 0));
    TEST_ASSERT_EQUAL(0, g_f.skipped);
    /* a budget above the loop rate (balanced 30 Hz) skips nothing either */
    TEST_ASSERT_EQUAL(20, idle_run(&now, 20, 30, 0));
    TEST_ASSERT_EQUAL(0, g_f.skipped);
}

static void test_dim_budget_skips_idle_wakes(void) {
    reset();
    uint32_t now = 0U;
    osui_frame_init(&g_f, now);
    /* saver dimmed once: 7 Hz is a 142 ms period, every second pass */
    TEST_ASSERT_EQUAL(10, idle_run(&now, 20, 7, 1));
    TEST_ASSERT_EQUAL(10, g_f.skipped);
    /* 5 Hz (deep dim floor): 200 ms, every second pass too */
    TEST_ASSERT_EQUAL(10, idle_run(&now, 20, 5, 2));
    TEST_ASSERT_EQUAL(20, g_f.idle_skipped);
    TEST_ASSERT_EQUAL(2, g_f.dim);
    /* 2 Hz would be every fifth pass */
    TEST_ASSERT_EQUAL(4, idle_run(&now, 20, 2, 2));
}

static void test_input_always_paints_and_wakes(void) {
    reset();
    uint32_t now = 1000U;
    osui_frame_init(&g_f, now);
    (void)idle_run(&now, 30, 5, 2);
    TEST_ASSERT(g_f.idle_skipped > 0);
    /* a key right after a painted frame still paints */
    now += 1U;
    TEST_ASSERT_EQUAL(1, osui_frame_due(&g_f, now, 1, 5, 0));
    TEST_ASSERT_EQUAL(1, g_f.wakes);
    TEST_ASSERT_EQUAL(0, g_f.idle_skipped);
    TEST_ASSERT_EQUAL(0, g_f.idle_drawn);
    TEST_ASSERT_EQUAL(1, g_f.input_frames);
    TEST_ASSERT_EQUAL(0U, osui_frame_idle_ms(&g_f, now));
    TEST_ASSERT_EQUAL(250U, osui_frame_idle_ms(&g_f, now + 250U));
    /* a second key without a dim between is not a wake */
    TEST_ASSERT_EQUAL(1, osui_frame_due(&g_f, now + 5U, 1, 15, 0));
    TEST_ASSERT_EQUAL(1, g_f.wakes);
}

static void test_tick_wrap(void) {
    reset();
    uint32_t now = 0xFFFFFF00U;
    osui_frame_init(&g_f, now);
    /* the 100 ms passes cross 2^32: the period still holds */
    TEST_ASSERT_EQUAL(5, idle_run(&now, 10, 5, 2));
    TEST_ASSERT(now < 0x1000U);
    TEST_ASSERT_EQUAL(1000U, osui_frame_idle_ms(&g_f, now));
}

static void test_idle_fps_and_report(void) {
    reset();
    uint32_t now = 0U;
    osui_frame_init(&g_f, now);
    TEST_ASSERT_EQUAL(0U, osui_frame_idle_fps10(&g_f, 500U));
    (void)idle_run(&now, 40, 5, 2);           /* 4 s, 20 frames */
    TEST_ASSERT_EQUAL(50U, osui_frame_idle_fps10(&g_f, now));
    TEST_ASSERT(osui_frame_report(&g_f, g_o, (int)sizeof(g_o)) > 0);
    TEST_ASSERT_EQUAL_STRING("frames drawn 20 skipped 20 input 0 idle_drawn 20 idle_skipped 20", g_o);
    /* a short buffer is cut, not overrun */
    g_o[10] = 'X';
    (void)osui_frame_report(&g_f, g_o, 8);
    TEST_ASSERT_EQUAL_STRING("frames ", g_o);
    TEST_ASSERT_EQUAL('X', g_o[10]);
    TEST_ASSERT_EQUAL(1, osui_frame_due(0, 0U, 0, 5, 0));
}

static void test_energy_dim_after(void) {
    plat_energy_t e;
    plat_energy_init(&e);
    TEST_ASSERT_EQUAL(0, plat_energy_apply(&e, PLAT_PWR_SAVER));
    /* the profile's 60 s */
    TEST_ASSERT_EQUAL(60U, plat_energy_dim_after_s(&e));
    TEST_ASSERT_EQUAL(0, plat_energy_idle(&e, 10000U));
    /* the user's 4 s: dim at 4 s, deep at 8 s */
    TEST_ASSERT_EQUAL(0, plat_energy_set_dim(&e, 4U));
    TEST_ASSERT_EQUAL(4U, plat_energy_dim_after_s(&e));
    TEST_ASSERT_EQUAL(0, plat_energy_idle(&e, 3999U));
    TEST_ASSERT_EQUAL(15U, e.refresh_hz);
    TEST_ASSERT_EQUAL(1, plat_energy_idle(&e, 4000U));
    TEST_ASSERT_EQUAL(1, e.dim);
    TEST_ASSERT_EQUAL(7U, e.refresh_hz);
    TEST_ASSERT_EQUAL(1, plat_energy_idle(&e, 8000U));
    TEST_ASSERT_EQUAL(2, e.dim);
    TEST_ASSERT_EQUAL(5U, e.refresh_hz);
    /* a profile change keeps the user's timeout */
    TEST_ASSERT_EQUAL(0, plat_energy_apply(&e, PLAT_PWR_BALANCED));
    TEST_ASSERT_EQUAL(4U, plat_energy_dim_after_s(&e));
    TEST_ASSERT_EQUAL(1, plat_energy_idle(&e, 8000U));
    TEST_ASSERT_EQUAL(7U, e.refresh_hz);      /* 30 / 4 */
    /* performance never dims, a user timeout included */
    TEST_ASSERT_EQUAL(0, plat_energy_apply(&e, PLAT_PWR_PERFORMANCE));
    TEST_ASSERT_EQUAL(0U, plat_energy_dim_after_s(&e));
    TEST_ASSERT_EQUAL(0, plat_energy_idle(&e, 100000U));
    TEST_ASSERT_EQUAL(60U, e.refresh_hz);
    /* back to the profile default; out of range refused */
    TEST_ASSERT_EQUAL(0, plat_energy_apply(&e, PLAT_PWR_SAVER));
    TEST_ASSERT_EQUAL(0, plat_energy_set_dim(&e, 0U));
    TEST_ASSERT_EQUAL(60U, plat_energy_dim_after_s(&e));
    TEST_ASSERT_EQUAL(-1, plat_energy_set_dim(&e, 3601U));
    TEST_ASSERT_EQUAL(-1, plat_energy_set_dim(0, 5U));
    TEST_ASSERT(plat_energy_report(&e, g_o, (int)sizeof(g_o)) > 0);
    TEST_ASSERT(strstr(g_o, "dim_after 60s") != 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_unpaced_paints_every_pass);
    RUN_TEST(test_dim_budget_skips_idle_wakes);
    RUN_TEST(test_input_always_paints_and_wakes);
    RUN_TEST(test_tick_wrap);
    RUN_TEST(test_idle_fps_and_report);
    RUN_TEST(test_energy_dim_after);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
