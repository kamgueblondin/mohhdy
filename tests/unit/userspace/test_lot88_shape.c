/* Lot 88 (US-068/US-073): the fair shaping queue (p2p_shape.c).
 * The send callback stands in for the mesh: a per-peer "open" flag says
 * whether the pacer admits that peer, and every datagram that leaves is
 * logged as peer:first-byte so the order can be checked. */
#include "../../framework/unity.h"
#include <string.h>
#include "p2p_shape.h"

#define A 3U
#define B 5U
#define C 7U

static p2p_shape_t g_q;
static char g_log[256];
static int g_open[16];
static int g_drop_peer;
static int g_budget;     /* sends allowed in this pass (-1: no limit) */
static char g_o[512];

static void reset(void) {
    p2p_shape_init(&g_q);
    memset(g_log, 0, sizeof(g_log));
    memset(g_open, 0, sizeof(g_open));
    g_drop_peer = 0;
    g_budget = -1;
}

static int send_cb(void* ctx, uint32_t peer, int inner, const uint8_t* body, int len) {
    char t[8];
    (void)ctx; (void)inner; (void)len;
    if ((int)peer == g_drop_peer) return P2P_SHAPE_DROP;
    if (!g_open[peer] || g_budget == 0) return P2P_SHAPE_WAIT;
    if (g_budget > 0) g_budget--;
    t[0] = (char)('0' + peer); t[1] = ':'; t[2] = (char)body[0]; t[3] = ' '; t[4] = 0;
    strcat(g_log, t);
    return P2P_SHAPE_SENT;
}

static int push(uint32_t peer, int cls, char tag, int len, uint32_t now) {
    uint8_t b[P2P_SHAPE_BODY];
    memset(b, tag, sizeof(b));
    return p2p_shape_push(&g_q, peer, cls, cls == P2P_SHAPE_C_BULK ? 2 : 1, b, len, now);
}

static int drain(uint32_t now) {
    int d = 0;
    return p2p_shape_drain(&g_q, send_cb, 0, now, &d);
}

/* The lot 6 queue sent in slot order: with a1 sent and slot 0 reused by
 * a3 while a2 waited in slot 1, a3 left before a2. Order is now by
 * enqueue sequence. */
static void test_peer_order_survives_slot_reuse(void) {
    reset();
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, '1', 10, 0));
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, '2', 10, 0));
    g_open[A] = 1; g_budget = 1;
    TEST_ASSERT_EQUAL(1, drain(10));
    TEST_ASSERT_EQUAL_STRING("3:1 ", g_log);
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, '3', 10, 20));   /* takes slot 0 */
    TEST_ASSERT_EQUAL(0, g_q.e[0].used ? 0 : 1);
    g_budget = -1;
    TEST_ASSERT_EQUAL(2, drain(30));
    TEST_ASSERT_EQUAL_STRING("3:1 3:2 3:3 ", g_log);
    TEST_ASSERT_EQUAL(0, p2p_shape_depth(&g_q, 0, -1));
}

/* A peer the pacer refuses keeps its entries and does not hold the
 * others back. */
static void test_refused_peer_does_not_block_others(void) {
    reset();
    push(A, 0, 'a', 10, 0);
    push(A, 0, 'b', 10, 0);
    push(B, 0, 'x', 10, 0);
    push(B, 0, 'y', 10, 0);
    g_open[B] = 1;
    TEST_ASSERT_EQUAL(2, drain(5));
    TEST_ASSERT_EQUAL_STRING("5:x 5:y ", g_log);
    TEST_ASSERT_EQUAL(2, p2p_shape_depth(&g_q, A, -1));
    TEST_ASSERT_EQUAL(1U, g_q.skips);   /* A tried once in the pass, not twice */
    g_open[A] = 1;
    TEST_ASSERT_EQUAL(2, drain(6));
    TEST_ASSERT_EQUAL_STRING("5:x 5:y 3:a 3:b ", g_log);
}

/* Peers of one class take turns, and the turn moves on between passes
 * (the lot 6 queue gave the lowest slot the token every time). */
static void test_round_robin_across_passes(void) {
    int i;
    reset();
    for (i = 0; i < 3; i++) { push(A, 0, (char)('a' + i), 10, 0); push(B, 0, (char)('x' + i), 10, 0); }
    g_open[A] = g_open[B] = 1;
    for (i = 0; i < 6; i++) { g_budget = 1; TEST_ASSERT_EQUAL(1, drain((uint32_t)i)); }
    TEST_ASSERT_EQUAL_STRING("3:a 5:x 3:b 5:y 3:c 5:z ", g_log);
}

/* Interactive messages get three quanta of bytes for one of bulk, and
 * bulk still moves while messages wait. */
static void test_drr_weights_classes(void) {
    int i, m = 0, k = 0;
    reset();
    for (i = 0; i < 4; i++) push(A, P2P_SHAPE_C_MSG, 'm', 400, 0);
    for (i = 0; i < 4; i++) push(A, P2P_SHAPE_C_BULK, 'k', 400, 0);
    g_open[A] = 1;
    g_budget = 4;
    TEST_ASSERT_EQUAL(4, drain(1));
    for (i = 0; g_log[i]; i++) { if (g_log[i] == 'm') m++; if (g_log[i] == 'k') k++; }
    TEST_ASSERT_EQUAL(3, m);
    TEST_ASSERT_EQUAL(1, k);
    TEST_ASSERT_EQUAL_STRING("3:m 3:m 3:m 3:k ", g_log);
    g_budget = -1;
    TEST_ASSERT_EQUAL(4, drain(2));
    TEST_ASSERT_EQUAL(4U, g_q.sent_cls[P2P_SHAPE_C_MSG]);
    TEST_ASSERT_EQUAL(4U, g_q.sent_cls[P2P_SHAPE_C_BULK]);
    TEST_ASSERT_EQUAL(1600U, g_q.bytes_cls[P2P_SHAPE_C_BULK]);
}

/* Bulk alone: no credit is banked while the class was idle or refused. */
static void test_credit_not_banked(void) {
    int i;
    reset();
    for (i = 0; i < 6; i++) push(A, P2P_SHAPE_C_BULK, 'k', 400, 0);
    TEST_ASSERT_EQUAL(0, drain(1));      /* refused: credit capped */
    TEST_ASSERT_EQUAL(0, drain(2));
    TEST_ASSERT(g_q.deficit[P2P_SHAPE_C_BULK] <= P2P_SHAPE_QUANTUM_BULK);
    g_open[A] = 1;
    TEST_ASSERT_EQUAL(6, drain(3));
    TEST_ASSERT_EQUAL(0U, g_q.deficit[P2P_SHAPE_C_BULK]);
    TEST_ASSERT_EQUAL(0U, g_q.deficit[P2P_SHAPE_C_MSG]);
}

/* A full queue held by one busy peer makes room for a quieter one. */
static void test_full_queue_evicts_busiest_peer(void) {
    int i;
    reset();
    for (i = 0; i < P2P_SHAPE_Q; i++) TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, (char)('a' + i), 10, 0));
    /* A alone: refused, as before lot 88 */
    TEST_ASSERT_EQUAL(P2P_SHAPE_REFUSED, push(A, 0, 'z', 10, 0));
    TEST_ASSERT_EQUAL(1U, g_q.refused);
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED_EVICT, push(B, 0, 'x', 10, 0));
    TEST_ASSERT_EQUAL(1U, g_q.evicted);
    TEST_ASSERT_EQUAL(7, p2p_shape_depth(&g_q, A, -1));
    TEST_ASSERT_EQUAL(1, p2p_shape_depth(&g_q, B, -1));
    /* B keeps pushing until both hold four */
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED_EVICT, push(B, 0, 'y', 10, 0));
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED_EVICT, push(B, 0, 'z', 10, 0));
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED_EVICT, push(B, 0, 'w', 10, 0));
    TEST_ASSERT_EQUAL(4, p2p_shape_depth(&g_q, A, -1));
    TEST_ASSERT_EQUAL(P2P_SHAPE_REFUSED, push(B, 0, 'v', 10, 0));
    /* the newest of A went: a..d are left, in order */
    g_open[A] = 1;
    TEST_ASSERT_EQUAL(4, drain(1));
    TEST_ASSERT_EQUAL_STRING("3:a 3:b 3:c 3:d ", g_log);
}

/* A message takes the place of a queued put; a put from a quiet peer
 * takes the newest entry of the busiest peer (its put first). */
static void test_message_pushes_out_bulk(void) {
    int i;
    reset();
    for (i = 0; i < 6; i++) push(A, P2P_SHAPE_C_MSG, 'm', 10, 0);
    push(A, P2P_SHAPE_C_BULK, '1', 10, 0);
    push(C, P2P_SHAPE_C_BULK, '2', 10, 0);
    /* B (none queued) against A (seven): A gives up its put, not a message */
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED_EVICT, push(B, P2P_SHAPE_C_BULK, 'k', 10, 0));
    TEST_ASSERT_EQUAL(0, p2p_shape_depth(&g_q, A, P2P_SHAPE_C_BULK));
    TEST_ASSERT_EQUAL(6, p2p_shape_depth(&g_q, A, P2P_SHAPE_C_MSG));
    /* a message: B and C tie on one put each, the lowest id gives it up */
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED_EVICT, push(A, P2P_SHAPE_C_MSG, 'n', 10, 0));
    TEST_ASSERT_EQUAL(0, p2p_shape_depth(&g_q, B, -1));
    TEST_ASSERT_EQUAL(1, p2p_shape_depth(&g_q, C, P2P_SHAPE_C_BULK));
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED_EVICT, push(A, P2P_SHAPE_C_MSG, 'o', 10, 0));
    TEST_ASSERT_EQUAL(0, p2p_shape_depth(&g_q, 0, P2P_SHAPE_C_BULK));
    /* no put left: A alone holds all eight, a ninth message is refused */
    TEST_ASSERT_EQUAL(P2P_SHAPE_REFUSED, push(A, P2P_SHAPE_C_MSG, 'p', 10, 0));
    /* a put from C still gets in: A gives up its newest message */
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED_EVICT, push(C, P2P_SHAPE_C_BULK, '3', 10, 0));
    TEST_ASSERT_EQUAL(7, p2p_shape_depth(&g_q, A, P2P_SHAPE_C_MSG));
    TEST_ASSERT_EQUAL(4U, g_q.evicted);
    TEST_ASSERT_EQUAL(1U, g_q.refused);
    g_open[A] = g_open[C] = 1;
    TEST_ASSERT_EQUAL(8, drain(1));
    /* the quantum is in bytes: seven 10-byte messages fit in one turn */
    TEST_ASSERT_EQUAL_STRING("3:m 3:m 3:m 3:m 3:m 3:m 3:n 7:3 ", g_log);
}

/* Drops (peer gone) free the slot, are counted, and do not stop the pass. */
static void test_drop_and_forget(void) {
    int d = 0;
    reset();
    push(A, 0, 'a', 10, 0);
    push(B, 0, 'b', 10, 0);
    push(C, 0, 'c', 10, 0);
    g_drop_peer = (int)A;
    g_open[B] = 1;
    TEST_ASSERT_EQUAL(1, p2p_shape_drain(&g_q, send_cb, 0, 1, &d));
    TEST_ASSERT_EQUAL(1, d);
    TEST_ASSERT_EQUAL(1U, g_q.dropped);
    TEST_ASSERT_EQUAL(1, p2p_shape_forget(&g_q, C));
    TEST_ASSERT_EQUAL(2U, g_q.dropped);
    TEST_ASSERT_EQUAL(0, p2p_shape_depth(&g_q, 0, -1));
}

/* Wait times, bounds and the status line. */
static void test_wait_bounds_status(void) {
    uint8_t b[4] = { 'q', 0, 0, 0 };
    reset();
    TEST_ASSERT_EQUAL(P2P_SHAPE_REFUSED, p2p_shape_push(&g_q, 0U, 0, 1, b, 4, 0));
    TEST_ASSERT_EQUAL(P2P_SHAPE_REFUSED, p2p_shape_push(&g_q, A, 0, 1, b, P2P_SHAPE_BODY + 1, 0));
    TEST_ASSERT_EQUAL(P2P_SHAPE_REFUSED, p2p_shape_push(&g_q, A, 0, 1, 0, 4, 0));
    TEST_ASSERT_EQUAL(P2P_SHAPE_REFUSED, p2p_shape_push(0, A, 0, 1, b, 4, 0));
    TEST_ASSERT_EQUAL(3U, g_q.refused);
    push(A, 0, 'a', 10, 0xFFFFFF00U);    /* clock wraps while it waits */
    push(B, 1, 'b', 10, 100);
    g_open[A] = g_open[B] = 1;
    TEST_ASSERT_EQUAL(2, drain(300));
    TEST_ASSERT_EQUAL(556U, g_q.max_wait_ms);
    TEST_ASSERT_EQUAL(200U, g_q.last_wait_ms);
    p2p_shape_status(&g_q, g_o, (int)sizeof(g_o));
    TEST_ASSERT_EQUAL_STRING("depth 0 msg 0 bulk 0 queued 2 sent 2 dropped 0 evicted 0 refused 3 "
                             "msg_sent 1 bulk_sent 1 last_wait_ms 200 max_wait_ms 556 skips 0", g_o);
    p2p_shape_status(&g_q, g_o, 12);
    TEST_ASSERT_EQUAL(11, (int)strlen(g_o));
    TEST_ASSERT_EQUAL(0, drain(301));
    TEST_ASSERT_EQUAL(0, p2p_shape_drain(0, send_cb, 0, 0, 0));
    TEST_ASSERT_EQUAL(P2P_SHAPE_C_BULK, p2p_shape_class(1));
    TEST_ASSERT_EQUAL(P2P_SHAPE_C_MSG, p2p_shape_class(0));
}

/* Sequence numbers wrap without breaking the order. */
static void test_sequence_wrap(void) {
    reset();
    g_q.seq = 0xFFFFFFFEU;
    push(A, 0, '1', 10, 0);
    push(A, 0, '2', 10, 0);
    push(A, 0, '3', 10, 0);   /* seq 0 */
    g_open[A] = 1;
    TEST_ASSERT_EQUAL(3, drain(1));
    TEST_ASSERT_EQUAL_STRING("3:1 3:2 3:3 ", g_log);
}

int main(void) {
    unity_init();
    RUN_TEST(test_peer_order_survives_slot_reuse);
    RUN_TEST(test_refused_peer_does_not_block_others);
    RUN_TEST(test_round_robin_across_passes);
    RUN_TEST(test_drr_weights_classes);
    RUN_TEST(test_credit_not_banked);
    RUN_TEST(test_full_queue_evicts_busiest_peer);
    RUN_TEST(test_message_pushes_out_bulk);
    RUN_TEST(test_drop_and_forget);
    RUN_TEST(test_wait_bounds_status);
    RUN_TEST(test_sequence_wrap);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
