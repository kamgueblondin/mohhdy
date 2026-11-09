/* Lot 87 (US-068/US-073): delay-gradient overuse detector (p2p_trend).
 * Synthetic probe trains: a steady path, a filling queue, a draining
 * queue, reordering, the adaptive threshold and the rate cap. */
#include "../../framework/unity.h"
#include "p2p_trend.h"
#include <string.h>

#define PKT 200U

/* n probes, one every `gap` ms from send time s0; each one's one-way
 * delay is base + extra_per_pkt * i (us precision lost to ms) */
static uint32_t train(p2p_trend_t *t, uint32_t s0, uint32_t n, uint32_t gap, uint32_t base, int32_t grow_num,
                      int32_t grow_den) {
    uint32_t i, last = s0;
    for (i = 0; i < n; i++) {
        uint32_t s = s0 + i * gap;
        int32_t q = (int32_t)base + (int32_t)i * grow_num / grow_den;
        if (q < 1) q = 1;
        p2p_trend_on_packet(t, s, s + (uint32_t)q, PKT);
        last = s;
    }
    return last;
}

static void test_steady_path_normal(void) {
    p2p_trend_t t;
    p2p_trend_init(&t, 0);
    (void)train(&t, 0, 200, 10, 20, 0, 1);
    TEST_ASSERT_EQUAL(P2P_TREND_NORMAL, t.state);
    TEST_ASSERT_EQUAL(0, t.overuses);
    TEST_ASSERT_EQUAL(0, t.trend_milli);
    TEST_ASSERT(t.deltas >= 190);
    /* no overuse seen: no cap, the pacer rate goes through */
    TEST_ASSERT_EQUAL(0, p2p_trend_cap(&t));
    TEST_ASSERT_EQUAL(12345, p2p_trend_limit(&t, 12345, 2000));
    /* one rate window per 500 ms: 50 probes x 200 bytes per 0.5 s */
    TEST_ASSERT_EQUAL(20000, t.acked_bps);
}

static void test_queue_builds_overuse_and_cap(void) {
    p2p_trend_t t;
    uint32_t cap1;
    p2p_trend_init(&t, 0);
    (void)train(&t, 0, 100, 10, 20, 0, 1);
    TEST_ASSERT_EQUAL(P2P_TREND_NORMAL, t.state);
    /* the queue grows by 1 ms per probe (10 % of the spacing) */
    (void)train(&t, 1000, 60, 10, 20, 1, 1);
    TEST_ASSERT_EQUAL(P2P_TREND_OVERUSE, t.state);
    TEST_ASSERT(t.overuses >= 1);
    TEST_ASSERT(t.trend_milli > 0);
    TEST_ASSERT(t.modified_milli > t.gamma_milli);
    /* the cap is 85 % of the acked rate at the overuse */
    cap1 = p2p_trend_cap(&t);
    TEST_ASSERT(cap1 > 0);
    TEST_ASSERT(cap1 <= t.acked_bps * 85u / 100u + 1u);
    TEST_ASSERT_EQUAL(cap1, p2p_trend_limit(&t, 1000000, t.last_update_ms + 10));
    TEST_ASSERT_EQUAL(500, p2p_trend_limit(&t, 500, t.last_update_ms + 10));   /* a lower rate is kept */
    TEST_ASSERT_EQUAL(0, p2p_trend_limit(&t, 0, t.last_update_ms + 10));       /* 0 = window budget, unchanged */
    /* the train ended long ago: the old overuse no longer caps the data */
    TEST_ASSERT_EQUAL(1000000, p2p_trend_limit(&t, 1000000, t.last_update_ms + P2P_TREND_STALE_MS + 1));
    TEST_ASSERT_EQUAL(cap1, p2p_trend_limit(&t, 1000000, t.last_update_ms + P2P_TREND_STALE_MS));
}

static void test_drain_underuse_then_recovery(void) {
    p2p_trend_t t;
    uint32_t cap_over, cap_after, last;
    p2p_trend_init(&t, 0);
    (void)train(&t, 0, 100, 10, 20, 0, 1);
    last = train(&t, 1000, 60, 10, 20, 1, 1);    /* queue up to 80 ms */
    TEST_ASSERT_EQUAL(P2P_TREND_OVERUSE, t.state);
    cap_over = p2p_trend_cap(&t);
    /* the queue drains 1 ms per probe */
    last = train(&t, last + 10, 50, 10, 80, -1, 1);
    TEST_ASSERT_EQUAL(P2P_TREND_UNDERUSE, t.state);
    TEST_ASSERT(t.underuses >= 1);
    TEST_ASSERT(t.trend_milli < 0);
    /* the drain passes a short normal phase (the cap may grow by a few
     * percent), then underuse holds it */
    TEST_ASSERT(p2p_trend_cap(&t) >= cap_over);
    TEST_ASSERT(p2p_trend_cap(&t) <= cap_over + cap_over / 10u);
    {
        uint32_t held = p2p_trend_cap(&t);
        last = train(&t, last + 10, 10, 10, 30, -1, 1);
        TEST_ASSERT_EQUAL(P2P_TREND_UNDERUSE, t.state);
        TEST_ASSERT_EQUAL(held, p2p_trend_cap(&t));
    }
    /* steady again for 3 s: back to normal, the cap grows (8 %/s) */
    (void)train(&t, last + 10, 300, 10, 30, 0, 1);
    TEST_ASSERT_EQUAL(P2P_TREND_NORMAL, t.state);
    cap_after = p2p_trend_cap(&t);
    TEST_ASSERT(cap_after > cap_over);
    /* never past 1.5 x the acked rate */
    TEST_ASSERT(cap_after <= t.acked_bps + t.acked_bps / 2u);
}

static void test_small_jitter_stays_normal(void) {
    p2p_trend_t t;
    uint32_t i;
    p2p_trend_init(&t, 0);
    for (i = 0; i < 400; i++) {
        uint32_t s = i * 10u;
        uint32_t q = 20u + ((i * 7u) % 3u);     /* 20..22 ms, no trend */
        p2p_trend_on_packet(&t, s, s + q, PKT);
    }
    TEST_ASSERT_EQUAL(0, t.overuses);
    TEST_ASSERT(t.state != P2P_TREND_OVERUSE);
    /* the threshold adapted down toward its floor on a quiet path */
    TEST_ASSERT(t.gamma_milli < P2P_TREND_GAMMA0);
    TEST_ASSERT(t.gamma_milli >= P2P_TREND_GAMMA_MIN);
}

static void test_burst_grouping(void) {
    p2p_trend_t t;
    p2p_trend_init(&t, 0);
    /* three probes inside 5 ms make one group */
    p2p_trend_on_packet(&t, 0, 20, PKT);
    p2p_trend_on_packet(&t, 2, 22, PKT);
    p2p_trend_on_packet(&t, 5, 25, PKT);
    TEST_ASSERT_EQUAL(0, t.groups);
    TEST_ASSERT_EQUAL(3 * PKT, t.g_bytes);
    p2p_trend_on_packet(&t, 6, 26, PKT);     /* opens the second group */
    TEST_ASSERT_EQUAL(1, t.groups);
    TEST_ASSERT_EQUAL(0, t.deltas);
    p2p_trend_on_packet(&t, 20, 40, PKT);
    TEST_ASSERT_EQUAL(2, t.groups);
    TEST_ASSERT_EQUAL(1, t.deltas);
    TEST_ASSERT_EQUAL(4, t.samples - 1);
}

static void test_reordering_dropped(void) {
    p2p_trend_t t;
    p2p_trend_init(&t, 0);
    p2p_trend_on_packet(&t, 100, 120, PKT);
    /* sent before the open group: ignored */
    p2p_trend_on_packet(&t, 90, 125, PKT);
    TEST_ASSERT_EQUAL(1, t.reordered);
    p2p_trend_on_packet(&t, 110, 130, PKT);
    /* a later group whose last arrival is before the previous one */
    p2p_trend_on_packet(&t, 120, 128, PKT);
    p2p_trend_on_packet(&t, 130, 150, PKT);
    TEST_ASSERT_EQUAL(2, t.reordered);
    TEST_ASSERT_EQUAL(P2P_TREND_NORMAL, t.state);
}

static void test_step_detected_before_threshold_follows(void) {
    p2p_trend_t t;
    uint32_t i, seen = 0;
    p2p_trend_init(&t, 0);
    (void)train(&t, 0, 50, 10, 20, 0, 1);
    /* a quiet path brought gamma to its floor */
    TEST_ASSERT_EQUAL(P2P_TREND_GAMMA_MIN, t.gamma_milli);
    /* the queue grows 5 ms per probe: overuse within ten probes, while
     * the threshold has not run after the trend (k_up is slow) */
    for (i = 0; i < 10 && !seen; i++) {
        uint32_t s0 = 500u + i * 10u;
        p2p_trend_on_packet(&t, s0, s0 + 20u + 5u * i, PKT);
        if (t.state == P2P_TREND_OVERUSE) seen = i + 1;
    }
    TEST_ASSERT(seen > 0);
    TEST_ASSERT(seen <= 10);
    TEST_ASSERT(t.gamma_milli < 2 * P2P_TREND_GAMMA_MIN);
    TEST_ASSERT_EQUAL(1, t.overuses);
}

static void test_sdiv64_and_names(void) {
    char line[256];
    p2p_trend_t t;
    TEST_ASSERT_EQUAL(3, (int)p2p_trend_sdiv64(10, 3));
    TEST_ASSERT_EQUAL(-3, (int)p2p_trend_sdiv64(-10, 3));
    TEST_ASSERT_EQUAL(-3, (int)p2p_trend_sdiv64(10, -3));
    TEST_ASSERT_EQUAL(3, (int)p2p_trend_sdiv64(-10, -3));
    TEST_ASSERT_EQUAL(0, (int)p2p_trend_sdiv64(5, 0));
    TEST_ASSERT(p2p_trend_sdiv64((int64_t)1 << 40, 1 << 20) == ((int64_t)1 << 20));
    TEST_ASSERT_EQUAL_STRING("normal", p2p_trend_state_name(P2P_TREND_NORMAL));
    TEST_ASSERT_EQUAL_STRING("overuse", p2p_trend_state_name(P2P_TREND_OVERUSE));
    TEST_ASSERT_EQUAL_STRING("underuse", p2p_trend_state_name(P2P_TREND_UNDERUSE));
    p2p_trend_init(&t, 0);
    p2p_trend_status(&t, line, (int)sizeof(line));
    TEST_ASSERT_EQUAL_STRING("p2p trend state normal trend_milli 0 modified 0 gamma 12500 deltas 0 cap_bps 0 "
                             "acked_bps 0 overuses 0 groups 0", line);
    p2p_trend_status(&t, line, 12);
    TEST_ASSERT_EQUAL(11, (int)strlen(line));
    TEST_ASSERT_EQUAL(0, p2p_trend_cap(0));
    TEST_ASSERT_EQUAL(7, p2p_trend_limit(0, 7, 0));
    p2p_trend_on_packet(0, 1, 2, 3);   /* no crash */
}

int main(void) {
    unity_init();
    RUN_TEST(test_steady_path_normal);
    RUN_TEST(test_queue_builds_overuse_and_cap);
    RUN_TEST(test_drain_underuse_then_recovery);
    RUN_TEST(test_small_jitter_stays_normal);
    RUN_TEST(test_burst_grouping);
    RUN_TEST(test_reordering_dropped);
    RUN_TEST(test_step_detected_before_threshold_follows);
    RUN_TEST(test_sdiv64_and_names);
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
