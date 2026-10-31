/* Lot 69 (US-068/US-073): per-peer flow fed by per-packet data acks
 * (userspace/p2p_flow.c). Times are in microseconds. */
#include "../../framework/unity.h"
#include "../../../userspace/p2p_flow.h"
#include <string.h>

static void test_init_window_before_samples(void) {
    p2p_flow_t f;
    uint32_t i;
    p2p_flow_init(&f);
    TEST_ASSERT_EQUAL(0U, p2p_flow_rate(&f));
    for (i = 0; i < P2P_FLOW_INIT_WINDOW; i++) {
        TEST_ASSERT_EQUAL(1, p2p_flow_admit(&f, 100, 1000));
        TEST_ASSERT_EQUAL(0, p2p_flow_on_send(&f, i + 1, 100, 1000));
    }
    TEST_ASSERT_EQUAL(P2P_FLOW_INIT_WINDOW, f.inflight);
    TEST_ASSERT_EQUAL(0, p2p_flow_admit(&f, 100, 1000));   /* window full, no sample yet */
    TEST_ASSERT_EQUAL(1U, f.window_full);
}

static void test_ack_gives_rtt_and_rate(void) {
    p2p_flow_t f;
    p2p_flow_init(&f);
    TEST_ASSERT_EQUAL(0, p2p_flow_on_send(&f, 7, 1000, 0));
    TEST_ASSERT_EQUAL(0, p2p_flow_on_ack(&f, 7, 20000));     /* 20 ms later */
    TEST_ASSERT_EQUAL(20000U, f.srtt_us);
    TEST_ASSERT_EQUAL(20000U, f.min_rtt_us);
    TEST_ASSERT_EQUAL(50000U, f.btl_bps);                     /* 1000 B / 20 ms */
    TEST_ASSERT_EQUAL(62500U, p2p_flow_rate(&f));             /* x 5/4 */
    TEST_ASSERT_EQUAL(0U, f.inflight);
    TEST_ASSERT_EQUAL(1U, f.acked);
    TEST_ASSERT_EQUAL(1, p2p_flow_on_ack(&f, 7, 21000));      /* duplicate */
    TEST_ASSERT_EQUAL(1U, f.dup_acks);
}

static void test_delivery_rate_counts_bytes_between(void) {
    /* two datagrams in flight: the second ack sees both deliveries */
    p2p_flow_t f;
    p2p_flow_init(&f);
    p2p_flow_on_send(&f, 1, 1000, 0);
    p2p_flow_on_send(&f, 2, 1000, 0);
    TEST_ASSERT_EQUAL(0, p2p_flow_on_ack(&f, 1, 10000));
    TEST_ASSERT_EQUAL(100000U, f.btl_bps);
    TEST_ASSERT_EQUAL(0, p2p_flow_on_ack(&f, 2, 10000));
    TEST_ASSERT_EQUAL(200000U, f.btl_bps);   /* both left at 0: 2000 B in 10 ms */
    p2p_flow_on_send(&f, 3, 1000, 10000);
    p2p_flow_on_send(&f, 4, 1000, 10000);
    p2p_flow_on_ack(&f, 3, 30000);
    p2p_flow_on_ack(&f, 4, 30000);           /* 2000 B over 20 ms = 100000 */
    TEST_ASSERT_EQUAL(100000U, f.sample[3]);
    TEST_ASSERT_EQUAL(50000U, f.sample[2]);
    TEST_ASSERT_EQUAL(200000U, f.btl_bps);   /* windowed max */
    TEST_ASSERT_EQUAL(4U, f.nsample);
}

static void test_windowed_max_forgets_old_samples(void) {
    p2p_flow_t f;
    uint32_t i, t = 0;
    p2p_flow_init(&f);
    p2p_flow_on_send(&f, 1, 4000, t);
    p2p_flow_on_ack(&f, 1, t + 10000);       /* 400000 B/s */
    TEST_ASSERT_EQUAL(400000U, f.btl_bps);
    for (i = 0; i < P2P_FLOW_SAMPLES; i++) {
        t += 20000;
        p2p_flow_on_send(&f, 10 + i, 1000, t);
        p2p_flow_on_ack(&f, 10 + i, t + 50000);   /* 20000 B/s */
    }
    TEST_ASSERT_EQUAL(20000U, f.btl_bps);
}

static void test_pacer_spaces_at_rate(void) {
    p2p_flow_t f;
    uint32_t t, ok = 0;
    p2p_flow_init(&f);
    p2p_flow_on_send(&f, 1, 1000, 0);
    p2p_flow_on_ack(&f, 1, 100000);          /* 10000 B/s -> 12500 paced */
    TEST_ASSERT_EQUAL(12500U, p2p_flow_rate(&f));
    for (t = 100000; t < 1100000; t += 1000) {
        if (p2p_flow_admit(&f, 200, t)) ok++;
    }
    /* one second at 12500 B/s of (200 + overhead) datagrams, plus the burst */
    TEST_ASSERT_TRUE(ok >= 40 && ok <= 60 + P2P_PACE_BURST_DEFAULT);
    TEST_ASSERT_TRUE(f.pace.deferred > 0);
}

static void test_loss_timeout_and_backoff(void) {
    p2p_flow_t f;
    uint32_t r0;
    p2p_flow_init(&f);
    TEST_ASSERT_EQUAL(1000000U, p2p_flow_loss_timeout_us(&f));  /* no RTT yet */
    p2p_flow_on_send(&f, 1, 1000, 0);
    p2p_flow_on_ack(&f, 1, 50000);
    TEST_ASSERT_EQUAL(P2P_FLOW_LOSS_MIN_US, p2p_flow_loss_timeout_us(&f)); /* 4 x 50 ms < 400 ms */
    r0 = p2p_flow_rate(&f);
    p2p_flow_on_send(&f, 2, 1000, 100000);
    p2p_flow_on_send(&f, 3, 1000, 100000);
    TEST_ASSERT_EQUAL(0, p2p_flow_expire(&f, 300000));
    TEST_ASSERT_EQUAL(2, p2p_flow_expire(&f, 600000));
    TEST_ASSERT_EQUAL(2U, f.lost);
    TEST_ASSERT_EQUAL(0U, f.inflight);
    TEST_ASSERT_EQUAL(1U, f.loss_backoffs);
    TEST_ASSERT_EQUAL(r0 * 7U / 8U, p2p_flow_rate(&f));
    /* a late ack of a lost datagram is a duplicate, not a sample */
    TEST_ASSERT_EQUAL(1, p2p_flow_on_ack(&f, 2, 700000));
    /* a faster sample lifts the cap */
    p2p_flow_on_send(&f, 4, 4000, 800000);
    p2p_flow_on_ack(&f, 4, 810000);
    TEST_ASSERT_EQUAL(0U, f.cap_bps);
    TEST_ASSERT_TRUE(p2p_flow_rate(&f) > r0);
}

static void test_timeout_bounds(void) {
    p2p_flow_t f;
    p2p_flow_init(&f);
    f.srtt_us = 2000000U;
    TEST_ASSERT_EQUAL(P2P_FLOW_LOSS_MAX_US, p2p_flow_loss_timeout_us(&f));
    f.srtt_us = 200000U;
    TEST_ASSERT_EQUAL(800000U, p2p_flow_loss_timeout_us(&f));
}

static void test_table_full_retires_oldest(void) {
    p2p_flow_t f;
    uint32_t i;
    p2p_flow_init(&f);
    for (i = 0; i < P2P_FLOW_INFLIGHT; i++) p2p_flow_on_send(&f, i + 1, 10, i);
    TEST_ASSERT_EQUAL(P2P_FLOW_INFLIGHT, f.inflight);
    TEST_ASSERT_EQUAL(0, p2p_flow_admit(&f, 10, 100));
    TEST_ASSERT_EQUAL(0, p2p_flow_on_send(&f, 999, 10, 100));
    TEST_ASSERT_EQUAL(P2P_FLOW_INFLIGHT, f.inflight);
    TEST_ASSERT_EQUAL(1U, f.lost);
    TEST_ASSERT_EQUAL(1, p2p_flow_on_ack(&f, 1, 200));        /* retired one */
    TEST_ASSERT_EQUAL(0, p2p_flow_on_ack(&f, 999, 200));
}

static void test_ack_batch_roundtrip(void) {
    p2p_flow_t rx, tx;
    uint8_t body[1 + 4 * P2P_FLOW_ACK_BATCH];
    uint32_t i;
    int len;
    p2p_flow_init(&rx);
    p2p_flow_init(&tx);
    for (i = 0; i < 10; i++) p2p_flow_on_send(&tx, 100 + i, 50, 0);
    for (i = 0; i < 7; i++) TEST_ASSERT_EQUAL(0, p2p_flow_note_rx(&rx, 100 + i));
    TEST_ASSERT_EQUAL(1, p2p_flow_note_rx(&rx, 107));         /* batch full: flush now */
    TEST_ASSERT_EQUAL(1, p2p_flow_note_rx(&rx, 108));         /* bounded: dropped, still full */
    len = p2p_flow_take_acks(&rx, body, (int)sizeof(body));
    TEST_ASSERT_EQUAL(1 + 4 * 8, len);
    TEST_ASSERT_EQUAL(8, body[0]);
    TEST_ASSERT_EQUAL(0U, rx.nack_pend);
    TEST_ASSERT_EQUAL(0, p2p_flow_take_acks(&rx, body, (int)sizeof(body)));
    TEST_ASSERT_EQUAL(8, p2p_flow_apply_acks(&tx, body, len, 30000));
    TEST_ASSERT_EQUAL(8U, tx.acked);
    TEST_ASSERT_EQUAL(2U, tx.inflight);
    /* small buffer takes part of the batch, the rest stays */
    p2p_flow_note_rx(&rx, 1); p2p_flow_note_rx(&rx, 2); p2p_flow_note_rx(&rx, 3);
    TEST_ASSERT_EQUAL(9, p2p_flow_take_acks(&rx, body, 9));
    TEST_ASSERT_EQUAL(1U, rx.nack_pend);
    TEST_ASSERT_EQUAL(3U, rx.ack_pend[0]);
}

static void test_apply_rejects_malformed(void) {
    p2p_flow_t f;
    uint8_t b[40];
    p2p_flow_init(&f);
    memset(b, 0, sizeof(b));
    TEST_ASSERT_EQUAL(-1, p2p_flow_apply_acks(&f, b, 0, 0));
    b[0] = 0;
    TEST_ASSERT_EQUAL(-1, p2p_flow_apply_acks(&f, b, 1, 0));
    b[0] = 2;
    TEST_ASSERT_EQUAL(-1, p2p_flow_apply_acks(&f, b, 5, 0));   /* short */
    b[0] = 9;
    TEST_ASSERT_EQUAL(-1, p2p_flow_apply_acks(&f, b, 37, 0));  /* above batch */
    b[0] = 1;
    TEST_ASSERT_EQUAL(0, p2p_flow_apply_acks(&f, b, 5, 0));    /* valid, unknown ctr */
    TEST_ASSERT_EQUAL(1U, f.unknown_acks);
}

static void test_status_line(void) {
    p2p_flow_t f;
    char out[320];
    p2p_flow_init(&f);
    p2p_flow_on_send(&f, 1, 1000, 0);
    p2p_flow_on_ack(&f, 1, 20000);
    p2p_flow_status(&f, out, (int)sizeof(out));
    TEST_ASSERT_TRUE(strstr(out, "sent 1 acked 1 lost 0 inflight 0 rate_bps 62500 btl_bps 50000 srtt_ms 20") != 0);
    p2p_flow_status(&f, out, 8);
    TEST_ASSERT_EQUAL(7, (int)strlen(out));
}

static void test_operator_cap_applies(void) {
    p2p_flow_t f;
    p2p_flow_init(&f);
    p2p_flow_on_send(&f, 1, 1000, 0);
    p2p_flow_on_ack(&f, 1, 1000);            /* 1 MB/s */
    f.pace.cap_bps = 3000U;
    TEST_ASSERT_EQUAL(3000U, p2p_flow_rate(&f));
    f.pace.cap_bps = 0;
    f.btl_bps = 1;
    TEST_ASSERT_EQUAL(P2P_PACE_FLOOR_BPS, p2p_flow_rate(&f));
}

int main(void) {
    unity_init();
    RUN_TEST(test_init_window_before_samples);
    RUN_TEST(test_ack_gives_rtt_and_rate);
    RUN_TEST(test_delivery_rate_counts_bytes_between);
    RUN_TEST(test_windowed_max_forgets_old_samples);
    RUN_TEST(test_pacer_spaces_at_rate);
    RUN_TEST(test_loss_timeout_and_backoff);
    RUN_TEST(test_timeout_bounds);
    RUN_TEST(test_table_full_retires_oldest);
    RUN_TEST(test_ack_batch_roundtrip);
    RUN_TEST(test_apply_rejects_malformed);
    RUN_TEST(test_status_line);
    RUN_TEST(test_operator_cap_applies);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
