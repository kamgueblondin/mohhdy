/* Lot 71 (US-068/US-073): retransmission copies and receiver dedup
 * (userspace/p2p_rtx.c). */
#include "../../framework/unity.h"
#include "../../../userspace/p2p_rtx.h"
#include <string.h>

static p2p_rtx_t g_r;

static void test_keep_ack_release(void) {
    char st[200];
    p2p_rtx_init(&g_r);
    TEST_ASSERT_EQUAL(0, p2p_rtx_keep(&g_r, 5, 12, (const uint8_t*)"hello", 5));
    TEST_ASSERT_EQUAL(1, p2p_rtx_acked(&g_r, 5));
    TEST_ASSERT_EQUAL(0, p2p_rtx_acked(&g_r, 5));          /* already released */
    TEST_ASSERT_EQUAL(-1, p2p_rtx_lost(&g_r, 5));          /* no copy left */
    p2p_rtx_status(&g_r, st, (int)sizeof(st));
    TEST_ASSERT_NOT_NULL(strstr(st, "kept 1 acked 1 resent 0 gave_up 0"));
    TEST_ASSERT_NOT_NULL(strstr(st, " pending 0 "));
}

static void test_lost_frame_and_resend(void) {
    uint8_t f[300];
    uint32_t orig;
    uint8_t inner;
    const uint8_t* body;
    int len, blen, idx;
    p2p_rtx_init(&g_r);
    TEST_ASSERT_EQUAL(0, p2p_rtx_keep(&g_r, 9, 13, (const uint8_t*)"k=v", 3));
    idx = p2p_rtx_lost(&g_r, 9);
    TEST_ASSERT_TRUE(idx >= 0);
    len = p2p_rtx_frame(&g_r, idx, f, (int)sizeof(f));
    TEST_ASSERT_EQUAL(8, len);
    TEST_ASSERT_EQUAL(0, p2p_rtx_parse(f, len, &orig, &inner, &body, &blen));
    TEST_ASSERT_EQUAL(9U, orig);
    TEST_ASSERT_EQUAL(13, inner);
    TEST_ASSERT_EQUAL(3, blen);
    TEST_ASSERT_EQUAL(0, memcmp(body, "k=v", 3));
    p2p_rtx_resent(&g_r, idx, 20);
    TEST_ASSERT_EQUAL(-1, p2p_rtx_lost(&g_r, 9));          /* the old counter is gone */
    TEST_ASSERT_EQUAL(1, p2p_rtx_acked(&g_r, 20));         /* the new one is acked */
    TEST_ASSERT_EQUAL(1U, g_r.resent);
    TEST_ASSERT_EQUAL(-1, p2p_rtx_frame(&g_r, idx, f, (int)sizeof(f)));
    TEST_ASSERT_EQUAL(-1, p2p_rtx_frame(&g_r, 99, f, (int)sizeof(f)));
    TEST_ASSERT_EQUAL(-1, p2p_rtx_parse(f, 4, &orig, &inner, &body, &blen));
}

static void test_gives_up_after_tries(void) {
    uint32_t ctr = 30, t;
    int idx;
    p2p_rtx_init(&g_r);
    TEST_ASSERT_EQUAL(0, p2p_rtx_keep(&g_r, ctr, 12, (const uint8_t*)"x", 1));
    for (t = 0; t < P2P_RTX_TRIES; t++) {
        idx = p2p_rtx_lost(&g_r, ctr);
        TEST_ASSERT_TRUE(idx >= 0);
        p2p_rtx_resent(&g_r, idx, ++ctr);
    }
    TEST_ASSERT_EQUAL(-1, p2p_rtx_lost(&g_r, ctr));
    TEST_ASSERT_EQUAL(1U, g_r.gave_up);
    TEST_ASSERT_EQUAL(P2P_RTX_TRIES, g_r.resent);
}

static void test_bounded_table(void) {
    uint8_t big[P2P_RTX_BODY + 1];
    uint32_t i;
    memset(big, 'a', sizeof(big));
    p2p_rtx_init(&g_r);
    TEST_ASSERT_EQUAL(-1, p2p_rtx_keep(&g_r, 1, 12, big, (int)sizeof(big)));
    TEST_ASSERT_EQUAL(1U, g_r.too_big);
    TEST_ASSERT_EQUAL(0, p2p_rtx_keep(&g_r, 2, 12, big, (int)P2P_RTX_BODY));
    for (i = 0; i < P2P_RTX_SLOTS; i++) TEST_ASSERT_EQUAL(0, p2p_rtx_keep(&g_r, 10 + i, 12, big, 4));
    TEST_ASSERT_EQUAL(1U, g_r.evicted);                    /* the oldest (2) went */
    TEST_ASSERT_EQUAL(0, p2p_rtx_acked(&g_r, 2));
    TEST_ASSERT_EQUAL(1, p2p_rtx_acked(&g_r, 10));
    TEST_ASSERT_EQUAL(-1, p2p_rtx_keep(0, 1, 12, big, 1));
    TEST_ASSERT_EQUAL(-1, p2p_rtx_keep(&g_r, 1, 12, 0, 3));
}

static void test_receiver_dedup_window(void) {
    p2p_rtx_init(&g_r);
    TEST_ASSERT_EQUAL(0, p2p_rtx_accept(&g_r, 0));
    TEST_ASSERT_EQUAL(1, p2p_rtx_accept(&g_r, 1));
    TEST_ASSERT_EQUAL(1, p2p_rtx_accept(&g_r, 3));         /* 2 lost on the way */
    TEST_ASSERT_EQUAL(0, p2p_rtx_accept(&g_r, 3));         /* duplicate */
    TEST_ASSERT_EQUAL(1, p2p_rtx_accept(&g_r, 2));         /* its retransmission */
    TEST_ASSERT_EQUAL(0, p2p_rtx_accept(&g_r, 2));         /* the ack was lost, resent again */
    TEST_ASSERT_EQUAL(2U, g_r.dup_rx);
    TEST_ASSERT_EQUAL(1, p2p_rtx_accept(&g_r, 3 + P2P_RTX_WINDOW));
    TEST_ASSERT_EQUAL(0, p2p_rtx_accept(&g_r, 3));         /* out of the window */
    TEST_ASSERT_EQUAL(1U, g_r.old_rx);
    TEST_ASSERT_EQUAL(1, p2p_rtx_accept(&g_r, 1000));      /* a jump clears the window */
    TEST_ASSERT_EQUAL(1, p2p_rtx_accept(&g_r, 999));
    TEST_ASSERT_EQUAL(6U, g_r.delivered);
}

/* the window crosses its 32-bit words when it moves by odd amounts */
static void test_receiver_window_words(void) {
    uint32_t c;
    p2p_rtx_init(&g_r);
    for (c = 1; c <= 200; c += 3) TEST_ASSERT_EQUAL(1, p2p_rtx_accept(&g_r, c));
    for (c = 1; c <= 200; c += 3) TEST_ASSERT_EQUAL(0, p2p_rtx_accept(&g_r, c));
    for (c = 2; c <= 200; c += 3) TEST_ASSERT_EQUAL(1, p2p_rtx_accept(&g_r, c));
    TEST_ASSERT_EQUAL(1, p2p_rtx_accept(&g_r, 237));
    TEST_ASSERT_EQUAL(0, p2p_rtx_accept(&g_r, 100));
    TEST_ASSERT_EQUAL(1, p2p_rtx_accept(&g_r, 102));       /* never seen, still in the window */
    TEST_ASSERT_EQUAL(1, p2p_rtx_accept(&g_r, 240 + P2P_RTX_WINDOW));
    TEST_ASSERT_EQUAL(0, p2p_rtx_accept(&g_r, 240));
}

int main(void) {
    unity_init();
    RUN_TEST(test_keep_ack_release);
    RUN_TEST(test_lost_frame_and_resend);
    RUN_TEST(test_gives_up_after_tries);
    RUN_TEST(test_bounded_table);
    RUN_TEST(test_receiver_dedup_window);
    RUN_TEST(test_receiver_window_words);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
