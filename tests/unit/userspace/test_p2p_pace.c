/* Lot 68 (US-068/US-073): the estimator's pacing rate drives the mesh
 * data pacer (userspace/p2p_pace.c). */
#include "../../framework/unity.h"
#include "../../../userspace/p2p_pace.h"
#include <string.h>

static p2p_bwe_t bwe;

static void fake_bwe(uint32_t samples, uint32_t pacing) {
    memset(&bwe, 0, sizeof(bwe));
    bwe.samples = samples;
    bwe.pacing_bps = pacing;
    bwe.initial_bps = 20000U;
}

static void test_rate_source_and_bounds(void) {
    p2p_pace_t p;
    p2p_pace_init(&p);
    TEST_ASSERT_EQUAL(P2P_PACE_AUTO, p.mode);
    fake_bwe(0, 0);
    TEST_ASSERT_EQUAL(0U, p2p_pace_rate(&p, &bwe, 1));        /* no sample: window */
    TEST_ASSERT_EQUAL_STRING("window", p2p_pace_source_name(p.source));
    TEST_ASSERT_EQUAL(0U, p2p_pace_rate(&p, &bwe, 0));
    p.mode = P2P_PACE_ON;
    TEST_ASSERT_EQUAL(20000U, p2p_pace_rate(&p, &bwe, 1));    /* forced: initial rate */
    TEST_ASSERT_EQUAL_STRING("initial", p2p_pace_source_name(p.source));
    p.mode = P2P_PACE_AUTO;
    fake_bwe(5, 123456U);
    TEST_ASSERT_EQUAL(123456U, p2p_pace_rate(&p, &bwe, 1));
    TEST_ASSERT_EQUAL_STRING("estimator", p2p_pace_source_name(p.source));
    fake_bwe(5, 10U);
    TEST_ASSERT_EQUAL(P2P_PACE_FLOOR_BPS, p2p_pace_rate(&p, &bwe, 1));
    fake_bwe(5, 900000000U);
    TEST_ASSERT_EQUAL(P2P_PACE_CEIL_BPS, p2p_pace_rate(&p, &bwe, 1));
    p.cap_bps = 5000U;
    TEST_ASSERT_EQUAL(5000U, p2p_pace_rate(&p, &bwe, 1));
    p.mode = P2P_PACE_OFF;
    TEST_ASSERT_EQUAL(0U, p2p_pace_rate(&p, &bwe, 1));
}

static void test_burst_then_spacing(void) {
    p2p_pace_t p;
    uint32_t t = 1000000U, i, ok = 0;
    p2p_pace_init(&p);
    /* 452 + 48 overhead = 500 bytes at 10 000 B/s: 50 ms each */
    for (i = 0; i < 10; i++) ok += (uint32_t)p2p_pace_admit(&p, 10000U, 452U, t);
    TEST_ASSERT_EQUAL(P2P_PACE_BURST_DEFAULT, ok);
    TEST_ASSERT_EQUAL(50000U, p.last_cost_us);
    TEST_ASSERT_EQUAL(6U, p.deferred);
    TEST_ASSERT_EQUAL(50000U, p2p_pace_wait_us(&p, t));
    TEST_ASSERT_EQUAL(0, p2p_pace_admit(&p, 10000U, 452U, t + 49000U));
    TEST_ASSERT_EQUAL(1, p2p_pace_admit(&p, 10000U, 452U, t + 50000U));
    TEST_ASSERT_EQUAL(0, p2p_pace_admit(&p, 10000U, 452U, t + 50000U));
    /* long idle: credit is bounded to the burst, not the idle time */
    t += 10000000U;
    for (ok = 0, i = 0; i < 20; i++) ok += (uint32_t)p2p_pace_admit(&p, 10000U, 452U, t);
    TEST_ASSERT_EQUAL(P2P_PACE_BURST_DEFAULT, ok);
}

static void test_long_run_matches_rate(void) {
    static const uint32_t rates[] = {1000U, 8000U, 64000U, 1000000U};
    uint32_t k;
    for (k = 0; k < 4; k++) {
        p2p_pace_t p;
        uint32_t t, start = 5000000U, bytes = 0, r = rates[k];
        p2p_pace_init(&p);
        for (t = start; t < start + 2000000U; t += 1000U)
            while (p2p_pace_admit(&p, r, 252U, t)) bytes += 300U;    /* 252 + 48 */
        /* 2 s at rate r, plus at most the burst credit and one datagram */
        TEST_ASSERT_TRUE(bytes <= 2U * r + (P2P_PACE_BURST_DEFAULT + 1U) * 300U);
        TEST_ASSERT_TRUE(bytes + 2U * 300U >= (2U * r) * 9U / 10U);
    }
}

static void test_cost_has_no_overflow(void) {
    p2p_pace_t p;
    p2p_pace_init(&p);
    TEST_ASSERT_EQUAL(1, p2p_pace_admit(&p, P2P_PACE_CEIL_BPS, 65536U, 100U));
    TEST_ASSERT_TRUE(p.last_cost_us >= 8000U && p.last_cost_us <= 8300U);  /* 65584 B at 8 MB/s */
    p2p_pace_init(&p);
    TEST_ASSERT_EQUAL(1, p2p_pace_admit(&p, P2P_PACE_FLOOR_BPS, 65536U, 100U));
    TEST_ASSERT_EQUAL(65584000U, p.last_cost_us);
    /* time wrap: still paced */
    p2p_pace_init(&p);
    TEST_ASSERT_EQUAL(1, p2p_pace_admit(&p, 10000U, 452U, 0xFFFFFF00U));
    TEST_ASSERT_TRUE(p.admitted == 1U);
}

static void test_command(void) {
    p2p_pace_t p;
    char out[256];
    const char* a1[] = {"status"};
    const char* a2[] = {"cap", "4000"};
    const char* a3[] = {"cap", "10"};
    const char* a4[] = {"burst", "8"};
    const char* a5[] = {"off"};
    const char* a6[] = {"fast"};
    p2p_pace_init(&p);
    TEST_ASSERT_EQUAL(0, p2p_pace_command(&p, 0, 0, out, (int)sizeof(out)));
    TEST_ASSERT_TRUE(strstr(out, "p2p pace mode auto source window") != 0);
    TEST_ASSERT_EQUAL(0, p2p_pace_command(&p, 1, a1, out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(0, p2p_pace_command(&p, 2, a2, out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(4000U, p.cap_bps);
    TEST_ASSERT_TRUE(strstr(out, "cap_bps 4000") != 0);
    TEST_ASSERT_EQUAL(1, p2p_pace_command(&p, 2, a3, out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(4000U, p.cap_bps);
    TEST_ASSERT_TRUE(strstr(out, "usage") != 0);
    TEST_ASSERT_EQUAL(0, p2p_pace_command(&p, 2, a4, out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(8U, p.burst_pkts);
    TEST_ASSERT_EQUAL(0, p2p_pace_command(&p, 1, a5, out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(P2P_PACE_OFF, p.mode);
    TEST_ASSERT_EQUAL(1, p2p_pace_command(&p, 1, a6, out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(1, p2p_pace_command(&p, 1, a1, out, 0));
}

int main(void) {
    unity_init();
    RUN_TEST(test_rate_source_and_bounds);
    RUN_TEST(test_burst_then_spacing);
    RUN_TEST(test_long_run_matches_rate);
    RUN_TEST(test_cost_has_no_overflow);
    RUN_TEST(test_command);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
