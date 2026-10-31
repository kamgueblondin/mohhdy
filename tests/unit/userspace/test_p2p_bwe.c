/* Lot 67 (US-068/US-073): bandwidth estimator and pacer against a simulated
 * bottleneck link (fixed capacity, propagation delay, FIFO queue). */
#include "../../framework/unity.h"
#include <string.h>
#include "p2p_bwe.h"

#define MSS 1000U
#define QMAX 256

typedef struct {
    uint32_t cap_bps;        /* bottleneck capacity, bytes per second */
    uint32_t one_way_ms;     /* propagation delay each way */
    uint32_t qlimit;         /* packets in the bottleneck queue; 0 = no limit */
    /* packets in flight: departure from the bottleneck, then ack arrival */
    uint32_t seq[QMAX], ack_at[QMAX];
    int n;
    uint32_t link_free_ms_x1000; /* when the bottleneck is free, in us */
    uint32_t max_queue, drops;
} link_t;

static p2p_bwe_t g_b;
static link_t g_l;

static void link_init(link_t* l, uint32_t cap, uint32_t owd, uint32_t qlimit) {
    memset(l, 0, sizeof(*l));
    l->cap_bps = cap; l->one_way_ms = owd; l->qlimit = qlimit;
}

/* send one packet at time t (ms): it queues behind the others at the
 * bottleneck, leaves after its transmission time, the ack comes back after
 * the two propagation delays. */
static int link_send(link_t* l, uint32_t seq, uint32_t t) {
    uint32_t now_us = t * 1000U, tx_us = (uint32_t)((uint64_t)MSS * 1000000U / l->cap_bps), depart_us;
    uint32_t queued = 0; int i;
    for (i = 0; i < l->n; i++) if (l->ack_at[i] * 1000U > now_us + l->one_way_ms * 1000U) queued++;
    if (queued > l->max_queue) l->max_queue = queued;
    if (l->qlimit && queued >= l->qlimit) { l->drops++; return -1; }
    if (l->n >= QMAX) return -1;
    if (l->link_free_ms_x1000 < now_us + l->one_way_ms * 1000U) l->link_free_ms_x1000 = now_us + l->one_way_ms * 1000U;
    depart_us = l->link_free_ms_x1000 + tx_us;
    l->link_free_ms_x1000 = depart_us;
    l->seq[l->n] = seq;
    l->ack_at[l->n] = (depart_us + l->one_way_ms * 1000U + 999U) / 1000U;
    l->n++;
    return 0;
}

static void link_deliver(link_t* l, p2p_bwe_t* b, uint32_t t) {
    int i = 0;
    while (i < l->n) {
        if (l->ack_at[i] <= t) {
            (void)p2p_bwe_on_ack(b, l->seq[i], t);
            l->seq[i] = l->seq[l->n - 1]; l->ack_at[i] = l->ack_at[l->n - 1]; l->n--;
        } else i++;
    }
}

/* greedy sender for `ms` milliseconds starting at *t */
static void run(uint32_t* t, uint32_t ms, uint32_t* seq, int app_rate_pps) {
    uint32_t end = *t + ms, next_app = *t;
    for (; *t < end; (*t)++) {
        link_deliver(&g_l, &g_b, *t);
        for (;;) {
            if (app_rate_pps > 0) {
                if (*t < next_app) { p2p_bwe_on_app_limited(&g_b); break; }
            }
            if (!p2p_bwe_can_send(&g_b, MSS, *t)) break;
            if (p2p_bwe_on_send(&g_b, *seq, MSS, *t) != 0) break;
            if (link_send(&g_l, *seq, *t) != 0) p2p_bwe_on_loss(&g_b, *seq, *t);
            (*seq)++;
            if (app_rate_pps > 0) next_app += 1000U / (uint32_t)app_rate_pps;
        }
    }
}

static int within(uint32_t v, uint32_t ref, uint32_t pct) {
    uint64_t lo = (uint64_t)ref * (100U - pct) / 100U, hi = (uint64_t)ref * (100U + pct) / 100U;
    return v >= lo && v <= hi;
}

static void test_converges_to_capacity(void) {
    uint32_t t = 0, seq = 1;
    link_init(&g_l, 500000U, 20U, 0U);                 /* 500 KB/s, RTT 40 ms */
    p2p_bwe_init(&g_b, MSS, 20000U, 0U);
    TEST_ASSERT_EQUAL(P2P_BWE_STARTUP, g_b.state);
    run(&t, 3000U, &seq, 0);
    TEST_ASSERT_TRUE(within(p2p_bwe_btlbw(&g_b), 500000U, 12U));
    TEST_ASSERT_TRUE(g_b.min_rtt_ms >= 40U && g_b.min_rtt_ms <= 44U);
    TEST_ASSERT_EQUAL(P2P_BWE_PROBE_BW, g_b.state);
    TEST_ASSERT_TRUE(g_b.full_bw_reached);
    /* BDP = 500000 * 0.040 = 20000 bytes, cwnd about 2 BDP */
    TEST_ASSERT_TRUE(within(p2p_bwe_bdp(&g_b), 20000U, 15U));
    TEST_ASSERT_TRUE(p2p_bwe_cwnd_pkts(&g_b) >= 30U && p2p_bwe_cwnd_pkts(&g_b) <= 50U);
    TEST_ASSERT_TRUE(g_b.samples > 100U);
}

static void test_bounded_queue_in_steady_state(void) {
    uint32_t t = 0, seq = 1, qpeak;
    link_init(&g_l, 250000U, 10U, 0U);                 /* 250 KB/s, RTT 20 ms, BDP 5 pkts */
    p2p_bwe_init(&g_b, MSS, 20000U, 0U);
    run(&t, 2000U, &seq, 0);
    g_l.max_queue = 0;
    run(&t, 3000U, &seq, 0);
    qpeak = g_l.max_queue;
    /* paced at about the capacity: the standing queue stays near the cwnd
     * excess (cwnd 2 BDP), far below what an unpaced window would build */
    TEST_ASSERT_TRUE(qpeak <= 8U);
    TEST_ASSERT_TRUE(within(p2p_bwe_btlbw(&g_b), 250000U, 12U));
    /* about the link rate was delivered: 250 pkt/s over 5 s = 1250 */
    TEST_ASSERT_TRUE(g_b.acked > 1150U);
}

static void test_tracks_capacity_drop_and_rise(void) {
    uint32_t t = 0, seq = 1;
    link_init(&g_l, 400000U, 15U, 64U);
    p2p_bwe_init(&g_b, MSS, 20000U, 0U);
    run(&t, 2500U, &seq, 0);
    TEST_ASSERT_TRUE(within(p2p_bwe_btlbw(&g_b), 400000U, 12U));
    g_l.cap_bps = 100000U;                             /* link drops to a quarter */
    run(&t, 4000U, &seq, 0);
    TEST_ASSERT_TRUE(within(p2p_bwe_btlbw(&g_b), 100000U, 20U));
    TEST_ASSERT_TRUE(within(g_b.pacing_bps, 100000U, 30U));
    g_l.cap_bps = 300000U;                             /* and comes back */
    run(&t, 4000U, &seq, 0);
    TEST_ASSERT_TRUE(within(p2p_bwe_btlbw(&g_b), 300000U, 15U));
}

static void test_app_limited_keeps_estimate(void) {
    uint32_t t = 0, seq = 1, before;
    link_init(&g_l, 300000U, 10U, 0U);
    p2p_bwe_init(&g_b, MSS, 20000U, 0U);
    run(&t, 2000U, &seq, 0);
    before = p2p_bwe_btlbw(&g_b);
    TEST_ASSERT_TRUE(within(before, 300000U, 12U));
    run(&t, 600U, &seq, 20);                            /* 20 pkt/s = 20 KB/s of real need */
    TEST_ASSERT_TRUE(g_b.samples_app_limited > 5U);
    /* 600 ms is beyond 10 rounds of 20 ms: without the app-limited rule
     * the estimate would fall to about 20 KB/s */
    TEST_ASSERT_TRUE(p2p_bwe_btlbw(&g_b) >= before / 2U);
}

static void test_probe_rtt_after_stale_min(void) {
    uint32_t t = 0, seq = 1;
    link_init(&g_l, 200000U, 10U, 0U);
    p2p_bwe_init(&g_b, MSS, 20000U, 0U);
    run(&t, 1500U, &seq, 0);
    TEST_ASSERT_EQUAL(P2P_BWE_PROBE_BW, g_b.state);
    /* the path delay grows: every RTT is now 30 ms higher, the 20 ms
     * minimum becomes stale after 10 s and PROBE_RTT measures again */
    g_l.one_way_ms = 25U;
    run(&t, 10500U, &seq, 0);
    TEST_ASSERT_TRUE(g_b.min_rtt_ms >= 50U);
    TEST_ASSERT_TRUE(g_b.state == P2P_BWE_PROBE_BW || g_b.state == P2P_BWE_PROBE_RTT);
    run(&t, 1000U, &seq, 0);
    TEST_ASSERT_EQUAL(P2P_BWE_PROBE_BW, g_b.state);
}

static void test_loss_and_table_bounds(void) {
    uint32_t i;
    p2p_bwe_init(&g_b, MSS, 50000U, 0U);
    for (i = 0; i < P2P_BWE_INFLIGHT; i++) TEST_ASSERT_EQUAL(0, p2p_bwe_on_send(&g_b, i, MSS, 0U));
    TEST_ASSERT_EQUAL(-1, p2p_bwe_on_send(&g_b, 999U, MSS, 0U));
    TEST_ASSERT_EQUAL(P2P_BWE_INFLIGHT * MSS, g_b.inflight_bytes);
    p2p_bwe_on_loss(&g_b, 3U, 5U);
    TEST_ASSERT_EQUAL(1U, g_b.losses);
    TEST_ASSERT_EQUAL((P2P_BWE_INFLIGHT - 1U) * MSS, g_b.inflight_bytes);
    TEST_ASSERT_EQUAL(0U, p2p_bwe_on_ack(&g_b, 3U, 6U));    /* already gone */
    TEST_ASSERT_EQUAL(0U, p2p_bwe_on_ack(&g_b, 12345U, 6U)); /* unknown */
    TEST_ASSERT_EQUAL(0, p2p_bwe_on_send(&g_b, 999U, MSS, 1U));
    TEST_ASSERT_TRUE(strcmp(p2p_bwe_state_name(P2P_BWE_DRAIN), "drain") == 0);
    TEST_ASSERT_TRUE(p2p_bwe_cwnd_pkts(&g_b) >= P2P_BWE_MIN_CWND);
}

static void test_pacer_spacing(void) {
    uint32_t t, sent = 0;
    p2p_bwe_init(&g_b, MSS, 10000U, 0U);
    /* before samples: pacing = 2.885 * 10000 B/s, about 28 packets in 1 s;
     * the cwnd (10 MSS) also limits since nothing is acked here */
    for (t = 0; t < 1000U; t++) {
        while (p2p_bwe_can_send(&g_b, MSS, t)) {
            (void)p2p_bwe_on_send(&g_b, t * 100U + sent, MSS, t);
            sent++;
            if (sent > 100U) break;
        }
    }
    TEST_ASSERT_TRUE(sent >= 9U && sent <= 11U);
    TEST_ASSERT_TRUE(g_b.paced_wait > 0U);
    /* the pacer alone (cwnd not binding): 1 s at 28850 B/s */
    p2p_bwe_init(&g_b, 100U, 10000U, 0U);
    sent = 0;
    for (t = 0; t < 1000U; t++) {
        if (p2p_bwe_can_send(&g_b, 100U, t)) {
            (void)p2p_bwe_on_send(&g_b, sent, 100U, t);
            (void)p2p_bwe_on_ack(&g_b, sent, t);
            sent++;
        }
    }
    TEST_ASSERT_TRUE(sent * 100U >= 25000U && sent * 100U <= 32000U);
}

int main(void) {
    unity_init();
    RUN_TEST(test_converges_to_capacity);
    RUN_TEST(test_bounded_queue_in_steady_state);
    RUN_TEST(test_tracks_capacity_drop_and_rise);
    RUN_TEST(test_app_limited_keeps_estimate);
    RUN_TEST(test_probe_rtt_after_stale_min);
    RUN_TEST(test_loss_and_table_bounds);
    RUN_TEST(test_pacer_spacing);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
