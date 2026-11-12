/* Lot 90 (US-068/US-073): shaping queue settings (p2p_shape.c): sixteen
 * slots, class weights set at run time, an age limit and a per-peer cap,
 * and the p2p-shape command parser. Same callback model as
 * test_lot88_shape.c: a per-peer "open" flag, sends logged peer:byte. */
#include "../../framework/unity.h"
#include <string.h>
#include "p2p_shape.h"

#define A 3U
#define B 5U
#define C 7U

static p2p_shape_t g_q;
static char g_log[1024];
static int g_open[16];
static char g_o[512];

static void reset(void) {
    p2p_shape_init(&g_q);
    memset(g_log, 0, sizeof(g_log));
    memset(g_open, 0, sizeof(g_open));
}

static int send_cb(void* ctx, uint32_t peer, int inner, const uint8_t* body, int len) {
    char t[8];
    (void)ctx; (void)inner; (void)len;
    if (!g_open[peer]) return P2P_SHAPE_WAIT;
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

static int cmd(const char* a0, const char* a1, const char* a2) {
    const char* av[3];
    int ac = 0;
    if (a0) av[ac++] = a0;
    if (a1) av[ac++] = a1;
    if (a2) av[ac++] = a2;
    return p2p_shape_command(&g_q, ac, av, g_o, (int)sizeof(g_o));
}

static void test_sixteen_slots(void) {
    int i;
    reset();
    TEST_ASSERT_EQUAL(16, P2P_SHAPE_Q);
    for (i = 0; i < 16; i++) TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, (char)('a' + i), 10, 0));
    TEST_ASSERT_EQUAL(16, p2p_shape_depth(&g_q, 0, -1));
    TEST_ASSERT_EQUAL(P2P_SHAPE_REFUSED, push(A, 0, 'z', 10, 0));
    g_open[A] = 1;
    TEST_ASSERT_EQUAL(16, drain(5));
    TEST_ASSERT_EQUAL_STRING("3:a 3:b 3:c 3:d 3:e 3:f 3:g 3:h 3:i 3:j 3:k 3:l 3:m 3:n 3:o 3:p ", g_log);
}

static void test_default_weights(void) {
    reset();
    TEST_ASSERT_EQUAL(3, p2p_shape_weight(&g_q, P2P_SHAPE_C_MSG));
    TEST_ASSERT_EQUAL(1, p2p_shape_weight(&g_q, P2P_SHAPE_C_BULK));
    TEST_ASSERT_EQUAL(0, p2p_shape_weight(&g_q, 2));
    TEST_ASSERT_EQUAL(0, p2p_shape_weight(0, 0));
}

static void test_weight_bounds(void) {
    reset();
    TEST_ASSERT_EQUAL(-1, p2p_shape_set_weight(&g_q, 0, 0));
    TEST_ASSERT_EQUAL(-1, p2p_shape_set_weight(&g_q, 0, 17));
    TEST_ASSERT_EQUAL(-1, p2p_shape_set_weight(&g_q, 2, 4));
    TEST_ASSERT_EQUAL(-1, p2p_shape_set_weight(&g_q, -1, 4));
    TEST_ASSERT_EQUAL(0, p2p_shape_set_weight(&g_q, 0, 16));
    TEST_ASSERT_EQUAL(16, p2p_shape_weight(&g_q, 0));
    TEST_ASSERT_EQUAL(0, p2p_shape_set_weight(&g_q, 1, 1));
    TEST_ASSERT_EQUAL(1, p2p_shape_weight(&g_q, 1));
}

/* full-size bodies: with weights 3/1 a pass of one round sends three
 * messages per put; with 1/3 the reverse */
static void fill_mixed(void) {
    int i;
    for (i = 0; i < 6; i++) TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, (char)('a' + i), P2P_SHAPE_BODY, 0));
    for (i = 0; i < 6; i++) TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(B, 1, (char)('A' + i), P2P_SHAPE_BODY, 0));
}

static void test_weights_change_the_share(void) {
    reset();
    fill_mixed();
    g_open[A] = 1; g_open[B] = 1;
    TEST_ASSERT_EQUAL(12, drain(1));
    /* messages first in each round: 3 msgs, 1 put, 3 msgs, 1 put, then puts */
    TEST_ASSERT_EQUAL(0, strncmp(g_log, "3:a 3:b 3:c 5:A 3:d 3:e 3:f 5:B ", 32));
    reset();
    TEST_ASSERT_EQUAL(0, p2p_shape_set_weight(&g_q, P2P_SHAPE_C_MSG, 1));
    TEST_ASSERT_EQUAL(0, p2p_shape_set_weight(&g_q, P2P_SHAPE_C_BULK, 3));
    fill_mixed();
    g_open[A] = 1; g_open[B] = 1;
    TEST_ASSERT_EQUAL(12, drain(1));
    TEST_ASSERT_EQUAL(0, strncmp(g_log, "3:a 5:A 5:B 5:C 3:b 5:D 5:E 5:F ", 32));
}

static void test_equal_weights_alternate(void) {
    reset();
    TEST_ASSERT_EQUAL(0, p2p_shape_set_weight(&g_q, P2P_SHAPE_C_MSG, 1));
    fill_mixed();
    g_open[A] = 1; g_open[B] = 1;
    TEST_ASSERT_EQUAL(12, drain(1));
    TEST_ASSERT_EQUAL_STRING("3:a 5:A 3:b 5:B 3:c 5:C 3:d 5:D 3:e 5:E 3:f 5:F ", g_log);
}

static void test_age_limit_drops_stale(void) {
    reset();
    TEST_ASSERT_EQUAL(0, p2p_shape_set_max_age(&g_q, 100));
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, 'o', 10, 0));
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, 'n', 10, 60));
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(B, 0, 'k', 10, 100));
    g_open[A] = 1; g_open[B] = 1;
    /* at 101: 'o' waited 101 > 100 ms, 'n' 41, 'k' 1 */
    TEST_ASSERT_EQUAL(2, drain(101));
    TEST_ASSERT_EQUAL_STRING("3:n 5:k ", g_log);
    TEST_ASSERT_EQUAL(1, (int)g_q.expired);
    TEST_ASSERT_EQUAL(2, (int)g_q.sent);
    TEST_ASSERT_EQUAL(0, (int)g_q.dropped);
}

static void test_age_limit_boundary(void) {
    reset();
    TEST_ASSERT_EQUAL(0, p2p_shape_set_max_age(&g_q, 100));
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, 'x', 10, 0));
    /* exactly 100 ms is still in time */
    TEST_ASSERT_EQUAL(0, p2p_shape_expire(&g_q, 100));
    TEST_ASSERT_EQUAL(1, p2p_shape_expire(&g_q, 101));
    TEST_ASSERT_EQUAL(0, p2p_shape_depth(&g_q, 0, -1));
}

static void test_age_off_keeps_everything(void) {
    reset();
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, 'x', 10, 0));
    TEST_ASSERT_EQUAL(0, p2p_shape_expire(&g_q, 500000));
    g_open[A] = 1;
    TEST_ASSERT_EQUAL(1, drain(500000));
    TEST_ASSERT_EQUAL(500000, (int)g_q.max_wait_ms);
    TEST_ASSERT_EQUAL(-1, p2p_shape_set_max_age(&g_q, P2P_SHAPE_AGE_MAX + 1U));
    TEST_ASSERT_EQUAL(0, p2p_shape_set_max_age(&g_q, P2P_SHAPE_AGE_MAX));
}

static void test_age_wraps(void) {
    /* the clock wraps: 0xfffffff0 then 0x10 is 32 ms later */
    reset();
    TEST_ASSERT_EQUAL(0, p2p_shape_set_max_age(&g_q, 40));
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, 'w', 10, 0xfffffff0U));
    TEST_ASSERT_EQUAL(0, p2p_shape_expire(&g_q, 0x10U));
    TEST_ASSERT_EQUAL(1, p2p_shape_expire(&g_q, 0x20U));
}

static void test_peer_cap(void) {
    int i;
    reset();
    TEST_ASSERT_EQUAL(0, p2p_shape_set_peer_cap(&g_q, 4));
    for (i = 0; i < 4; i++) TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, (char)('a' + i), 10, 0));
    TEST_ASSERT_EQUAL(P2P_SHAPE_REFUSED, push(A, 0, 'e', 10, 0));
    TEST_ASSERT_EQUAL(P2P_SHAPE_REFUSED, push(A, 1, 'f', 10, 0));
    TEST_ASSERT_EQUAL(2, (int)g_q.capped);
    TEST_ASSERT_EQUAL(2, (int)g_q.refused);
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(B, 0, 'g', 10, 0));
    TEST_ASSERT_EQUAL(4, p2p_shape_depth(&g_q, A, -1));
    g_open[A] = 1;
    TEST_ASSERT_EQUAL(4, drain(1));
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, 'h', 10, 2));
}

static void test_peer_cap_bounds_and_off(void) {
    int i;
    reset();
    TEST_ASSERT_EQUAL(-1, p2p_shape_set_peer_cap(&g_q, -1));
    TEST_ASSERT_EQUAL(-1, p2p_shape_set_peer_cap(&g_q, 17));
    TEST_ASSERT_EQUAL(0, p2p_shape_set_peer_cap(&g_q, 1));
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, 'a', 10, 0));
    TEST_ASSERT_EQUAL(P2P_SHAPE_REFUSED, push(A, 0, 'b', 10, 0));
    TEST_ASSERT_EQUAL(0, p2p_shape_set_peer_cap(&g_q, 0));
    for (i = 1; i < 16; i++) TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 0, 'c', 10, 0));
    TEST_ASSERT_EQUAL(16, p2p_shape_depth(&g_q, A, -1));
}

static void test_cap_beats_eviction(void) {
    /* a capped peer may not push another one out either */
    int i;
    reset();
    for (i = 0; i < 16; i++) TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(A, 1, 'p', 10, 0));
    TEST_ASSERT_EQUAL(0, p2p_shape_set_peer_cap(&g_q, 16));
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED_EVICT, push(B, 0, 'm', 10, 0));
    TEST_ASSERT_EQUAL(0, p2p_shape_set_peer_cap(&g_q, 1));
    TEST_ASSERT_EQUAL(P2P_SHAPE_REFUSED, push(B, 0, 'n', 10, 0));
    TEST_ASSERT_EQUAL(1, p2p_shape_depth(&g_q, B, -1));
    TEST_ASSERT_EQUAL(1, (int)g_q.evicted);
}

static void test_status_fields(void) {
    reset();
    p2p_shape_status(&g_q, g_o, (int)sizeof(g_o));
    TEST_ASSERT(strstr(g_o, " skips 0 weight 3/1 age_ms 0 cap 16 expired 0 capped 0") != 0);
    TEST_ASSERT_EQUAL(0, p2p_shape_set_weight(&g_q, 0, 5));
    TEST_ASSERT_EQUAL(0, p2p_shape_set_max_age(&g_q, 250));
    TEST_ASSERT_EQUAL(0, p2p_shape_set_peer_cap(&g_q, 6));
    p2p_shape_status(&g_q, g_o, (int)sizeof(g_o));
    TEST_ASSERT(strstr(g_o, " weight 5/1 age_ms 250 cap 6 expired 0 capped 0") != 0);
    TEST_ASSERT_EQUAL(0, strncmp(g_o, "depth 0 msg 0 bulk 0 queued 0 sent 0 dropped 0 evicted 0 refused 0 ", 67));
}

static void test_command_parser(void) {
    reset();
    TEST_ASSERT_EQUAL(0, cmd(0, 0, 0));
    TEST_ASSERT_EQUAL(0, strncmp(g_o, "p2p shape depth 0 ", 18));
    TEST_ASSERT(strstr(g_o, "weight 3/1") != 0);
    TEST_ASSERT_EQUAL('\n', g_o[strlen(g_o) - 1]);
    TEST_ASSERT_EQUAL(0, cmd("status", 0, 0));
    TEST_ASSERT_EQUAL(0, cmd("weight", "msg", "8"));
    TEST_ASSERT(strstr(g_o, "weight 8/1") != 0);
    TEST_ASSERT_EQUAL(0, cmd("weight", "bulk", "2"));
    TEST_ASSERT(strstr(g_o, "weight 8/2") != 0);
    TEST_ASSERT_EQUAL(0, cmd("age", "1500", 0));
    TEST_ASSERT(strstr(g_o, "age_ms 1500 ") != 0);
    TEST_ASSERT_EQUAL(0, cmd("age", "off", 0));
    TEST_ASSERT(strstr(g_o, "age_ms 0 ") != 0);
    TEST_ASSERT_EQUAL(0, cmd("cap", "3", 0));
    TEST_ASSERT(strstr(g_o, "cap 3 ") != 0);
    TEST_ASSERT_EQUAL(0, cmd("cap", "off", 0));
    TEST_ASSERT(strstr(g_o, "cap 16 ") != 0);
}

static void test_command_errors_change_nothing(void) {
    reset();
    TEST_ASSERT_EQUAL(1, cmd("weight", "msg", "0"));
    TEST_ASSERT_EQUAL_STRING("p2p shape error weight 1..16\n", g_o);
    TEST_ASSERT_EQUAL(1, cmd("weight", "msg", "x"));
    TEST_ASSERT_EQUAL(1, cmd("weight", "fast", "2"));
    TEST_ASSERT_EQUAL_STRING("p2p shape error usage weight msg|bulk N\n", g_o);
    TEST_ASSERT_EQUAL(1, cmd("weight", "2", 0));
    TEST_ASSERT_EQUAL(1, cmd("age", "600001", 0));
    TEST_ASSERT_EQUAL_STRING("p2p shape error age 0..600000 ms or off\n", g_o);
    TEST_ASSERT_EQUAL(1, cmd("age", "-5", 0));
    TEST_ASSERT_EQUAL(1, cmd("age", "9999999999", 0));
    TEST_ASSERT_EQUAL(1, cmd("cap", "17", 0));
    TEST_ASSERT_EQUAL_STRING("p2p shape error cap 0..16 or off\n", g_o);
    TEST_ASSERT_EQUAL(1, cmd("cap", 0, 0));
    TEST_ASSERT_EQUAL(1, cmd("drop", 0, 0));
    TEST_ASSERT(strstr(g_o, "p2p shape error usage p2p-shape") == g_o);
    TEST_ASSERT_EQUAL(3, p2p_shape_weight(&g_q, 0));
    TEST_ASSERT_EQUAL(1, p2p_shape_weight(&g_q, 1));
    TEST_ASSERT_EQUAL(0, (int)g_q.max_age_ms);
    TEST_ASSERT_EQUAL(0, (int)g_q.peer_cap);
}

static void test_expired_not_sent_when_closed(void) {
    /* a refused peer keeps its entries until they expire */
    reset();
    TEST_ASSERT_EQUAL(0, p2p_shape_set_max_age(&g_q, 50));
    TEST_ASSERT_EQUAL(P2P_SHAPE_QUEUED, push(C, 0, 'c', 10, 0));
    TEST_ASSERT_EQUAL(0, drain(10));
    TEST_ASSERT_EQUAL(1, p2p_shape_depth(&g_q, C, -1));
    TEST_ASSERT_EQUAL(0, drain(51));
    TEST_ASSERT_EQUAL(0, p2p_shape_depth(&g_q, C, -1));
    TEST_ASSERT_EQUAL(1, (int)g_q.expired);
    g_open[C] = 1;
    TEST_ASSERT_EQUAL(0, drain(60));
    TEST_ASSERT_EQUAL_STRING("", g_log);
}

int main(void) {
    unity_init();
    RUN_TEST(test_sixteen_slots);
    RUN_TEST(test_default_weights);
    RUN_TEST(test_weight_bounds);
    RUN_TEST(test_weights_change_the_share);
    RUN_TEST(test_equal_weights_alternate);
    RUN_TEST(test_age_limit_drops_stale);
    RUN_TEST(test_age_limit_boundary);
    RUN_TEST(test_age_off_keeps_everything);
    RUN_TEST(test_age_wraps);
    RUN_TEST(test_peer_cap);
    RUN_TEST(test_peer_cap_bounds_and_off);
    RUN_TEST(test_cap_beats_eviction);
    RUN_TEST(test_status_fields);
    RUN_TEST(test_command_parser);
    RUN_TEST(test_command_errors_change_nothing);
    RUN_TEST(test_expired_not_sent_when_closed);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
