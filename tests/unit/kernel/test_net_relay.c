#include "../../framework/unity.h"
#include "../../../kernel/net_relay.h"
#include "../../../kernel/net_socket.h"

/* Tranche 5 slice 2: single-slot net IPC relay (pure logic) and the socket
 * loopback sequence the QEMU proof client runs through the relay. */

static void test_supported_subset(void) {
    uint32_t n;
    for (n = SYS_SOCKET_OPEN; n <= SYS_SOCKET_ACCEPT_ACK; n++) TEST_ASSERT_TRUE(net_relay_supported(n));
    TEST_ASSERT_FALSE(net_relay_supported(SYS_LLM_POLL_TLS));
    TEST_ASSERT_FALSE(net_relay_supported(SYS_LLM_ACQUIRE_START));
    TEST_ASSERT_FALSE(net_relay_supported(SYS_PEER_LISTEN));
    TEST_ASSERT_FALSE(net_relay_supported(SYS_PEER_TLS_POLL));
    TEST_ASSERT_FALSE(net_relay_supported(SYS_NET_STATUS));
    TEST_ASSERT_FALSE(net_relay_supported(SYS_NET_RELAY_REPLY));
    TEST_ASSERT_TRUE(sizeof(os_net_relay_request_t) <= OS_IPC_MAX_DATA);
    TEST_ASSERT_TRUE(OS_NET_RELAY_TIMEOUT != OS_NET_WORKER_REQUIRED);
    TEST_ASSERT_TRUE(OS_NET_RELAY_ABORTED != OS_ATA_JOB_STALE);
    TEST_ASSERT_TRUE(OS_NET_RELAY_TIMEOUT != OS_ATA_JOB_STALE);
}

static void test_roundtrip_and_stale(void) {
    uint8_t out[8] = {0};
    uint8_t data[3] = {1, 2, 3};
    uint32_t op = 0U, n = 0U;
    os_net_relay_status_t st;
    int32_t job;
    net_relay_init();
    /* Worker itself, bad pids and unsupported ops are never relayed. */
    TEST_ASSERT_EQUAL(-1, net_relay_begin(5, 5, SYS_SOCKET_OPEN, 0U));
    TEST_ASSERT_EQUAL(-1, net_relay_begin(0, 5, SYS_SOCKET_OPEN, 0U));
    TEST_ASSERT_EQUAL(-1, net_relay_begin(4, 5, SYS_PEER_LISTEN, 0U));
    job = net_relay_begin(4, 5, SYS_SOCKET_SEND, 10U);
    TEST_ASSERT_TRUE(job > 0);
    TEST_ASSERT_EQUAL(NET_RELAY_SENT, (int)net_relay_state_for(4));
    TEST_ASSERT_EQUAL(NET_RELAY_FREE, (int)net_relay_state_for(6));
    TEST_ASSERT_EQUAL(4, net_relay_owner());
    /* One request at a time. */
    TEST_ASSERT_EQUAL(-1, net_relay_begin(6, 5, SYS_SOCKET_OPEN, 10U));
    /* Only the worker, with the right job id, may answer. */
    TEST_ASSERT_EQUAL(-1, net_relay_complete(6, (uint32_t)job, 0, data, 3U));
    TEST_ASSERT_EQUAL(-1, net_relay_complete(5, (uint32_t)job + 1U, 0, data, 3U));
    TEST_ASSERT_EQUAL(-1, net_relay_complete(5, (uint32_t)job, 0, data, OS_NET_RELAY_MAX_OUT + 1U));
    TEST_ASSERT_EQUAL(0, net_relay_complete(5, (uint32_t)job, 0, data, 3U));
    TEST_ASSERT_EQUAL(-1, net_relay_complete(5, (uint32_t)job, 0, data, 3U)); /* twice */
    TEST_ASSERT_EQUAL(NET_RELAY_DONE, (int)net_relay_state_for(4));
    TEST_ASSERT_EQUAL(OS_NET_RELAY_ABORTED, net_relay_take(6, &op, out, sizeof(out), &n));
    TEST_ASSERT_EQUAL(0, net_relay_take(4, &op, out, sizeof(out), &n));
    TEST_ASSERT_EQUAL(SYS_SOCKET_SEND, (int)op);
    TEST_ASSERT_EQUAL(3, (int)n);
    TEST_ASSERT_EQUAL(3, (int)out[2]);
    TEST_ASSERT_EQUAL(NET_RELAY_FREE, (int)net_relay_state_for(4));
    net_relay_note_denied();
    net_relay_fill_status(&st, 5);
    TEST_ASSERT_EQUAL(1, (int)st.forwarded);
    TEST_ASSERT_EQUAL(1, (int)st.completed);
    TEST_ASSERT_EQUAL(4, (int)st.stale);
    TEST_ASSERT_EQUAL(1, (int)st.denied);
    TEST_ASSERT_EQUAL(0, (int)st.pending);
    TEST_ASSERT_EQUAL(5, st.worker_pid);
}

static void test_timeout_abort_cancel_drop(void) {
    uint32_t op = 0U, n = 0U;
    os_net_relay_status_t st;
    int32_t job;
    net_relay_init();
    job = net_relay_begin(4, 5, SYS_SOCKET_RECEIVE, 100U);
    TEST_ASSERT_FALSE(net_relay_expired(100U + NET_RELAY_TIMEOUT_TICKS));
    /* Time alone is not enough: the caller must have had turns. */
    TEST_ASSERT_FALSE(net_relay_expired(101U + NET_RELAY_TIMEOUT_TICKS));
    net_relay_note_poll();
    net_relay_note_poll();
    TEST_ASSERT_FALSE(net_relay_expired(101U + NET_RELAY_TIMEOUT_TICKS));
    net_relay_note_poll();
    TEST_ASSERT_FALSE(net_relay_expired(100U + NET_RELAY_TIMEOUT_TICKS));
    TEST_ASSERT_TRUE(net_relay_expired(101U + NET_RELAY_TIMEOUT_TICKS));
    net_relay_fail(OS_NET_RELAY_TIMEOUT);
    /* A late reply is stale; the caller gets the error once. */
    TEST_ASSERT_EQUAL(-1, net_relay_complete(5, (uint32_t)job, 0, 0, 0U));
    TEST_ASSERT_EQUAL(OS_NET_RELAY_TIMEOUT, net_relay_take(4, &op, 0, 0U, &n));
    /* Worker lost after taking the request: aborted, not replayed. */
    job = net_relay_begin(4, 5, SYS_SOCKET_SEND, 200U);
    TEST_ASSERT_TRUE(job > 0);
    net_relay_fail(OS_NET_RELAY_ABORTED);
    TEST_ASSERT_EQUAL(OS_NET_RELAY_ABORTED, net_relay_take(4, &op, 0, 0U, &n));
    /* IPC send failed: slot back, no forward counted. */
    TEST_ASSERT_TRUE(net_relay_begin(4, 5, SYS_SOCKET_OPEN, 300U) > 0);
    net_relay_cancel();
    TEST_ASSERT_EQUAL(0, net_relay_owner());
    /* Owner died: slot freed, reply stale. */
    job = net_relay_begin(7, 5, SYS_SOCKET_CLOSE, 400U);
    net_relay_drop_owner();
    TEST_ASSERT_EQUAL(-1, net_relay_complete(5, (uint32_t)job, 0, 0, 0U));
    TEST_ASSERT_TRUE(net_relay_begin(8, 5, SYS_SOCKET_CLOSE, 500U) > 0);
    net_relay_fill_status(&st, 0);
    TEST_ASSERT_EQUAL(4, (int)st.forwarded);
    TEST_ASSERT_EQUAL(0, (int)st.completed);
    TEST_ASSERT_EQUAL(1, (int)st.timeouts);
    TEST_ASSERT_EQUAL(1, (int)st.aborted);
    TEST_ASSERT_EQUAL(1, (int)st.pending);
    TEST_ASSERT_EQUAL(0, st.worker_pid);
}

/* Same sequence as userspace/net_relay_client.c, straight on the registry. */
static void test_socket_loopback_sequence(void) {
    uint8_t synack[64], seg[128], got[16];
    uint16_t synack_len = 0U, seg_len = 0U, got_len = 0U;
    net_tcp_view_t syn = {40001U, 40002U, 100U, 0U, NET_TCP_FLAG_SYN, 0, 0};
    net_tcp_view_t sa, ack;
    int a, b;
    net_socket_reset_all();
    a = net_socket_open(40001U, 40002U, 100U);
    b = net_socket_listen(40002U, 1234U);
    TEST_ASSERT_TRUE(a >= 0 && b >= 0);
    TEST_ASSERT_EQUAL(0, net_socket_accept_syn(b, &syn));
    TEST_ASSERT_EQUAL(0, net_socket_build_syn_ack(b, synack, sizeof(synack), &synack_len));
    TEST_ASSERT_EQUAL(0x12, synack[13]);
    TEST_ASSERT_EQUAL(0, net_tcp_parse(synack, synack_len, &sa));
    TEST_ASSERT_EQUAL(0, net_socket_accept_syn_ack(a, &sa));
    ack = (net_tcp_view_t){40001U, 40002U, sa.acknowledgment, sa.sequence + 1U, NET_TCP_FLAG_ACK, 0, 0};
    TEST_ASSERT_EQUAL(0, net_socket_accept_ack(b, &ack));
    TEST_ASSERT_EQUAL(0, net_socket_send(a, (const uint8_t*)"ping", 4U, seg, sizeof(seg), &seg_len));
    TEST_ASSERT_EQUAL(24, seg_len);
    TEST_ASSERT_EQUAL(0, net_socket_feed(b, seg, seg_len));
    TEST_ASSERT_EQUAL(0, net_socket_receive(b, got, sizeof(got), &got_len));
    TEST_ASSERT_EQUAL(4, got_len);
    TEST_ASSERT_EQUAL('g', got[3]);
    TEST_ASSERT_EQUAL(0, net_socket_send(b, (const uint8_t*)"pong", 4U, seg, sizeof(seg), &seg_len));
    TEST_ASSERT_EQUAL(0, net_socket_feed(a, seg, seg_len));
    TEST_ASSERT_EQUAL(0, net_socket_receive(a, got, sizeof(got), &got_len));
    TEST_ASSERT_EQUAL(4, got_len);
    TEST_ASSERT_EQUAL('o', got[1]);
    TEST_ASSERT_EQUAL(0, net_socket_close(a));
    TEST_ASSERT_EQUAL(0, net_socket_close(b));
}

int main(void) {
    unity_init();
    RUN_TEST(test_supported_subset);
    RUN_TEST(test_roundtrip_and_stale);
    RUN_TEST(test_timeout_abort_cancel_drop);
    RUN_TEST(test_socket_loopback_sequence);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
