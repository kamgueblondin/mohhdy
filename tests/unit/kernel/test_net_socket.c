#include "../../framework/unity.h"
#include "../../../kernel/net_socket.h"

void setUp(void) {}
void tearDown(void) {}
void test_socket_feed_routes_passive_segments(void);

void test_socket_passive_lifecycle(void) {
    int id; uint8_t segment[32] = {0}; uint16_t segment_length = 0U; uint8_t state = 0U;
    net_tcp_view_t syn = {40000U, 8080U, 900U, 0U, NET_TCP_FLAG_SYN, 0, 0};
    net_tcp_view_t ack = {40000U, 8080U, 901U, 1235U, NET_TCP_FLAG_ACK, 0, 0};
    net_socket_reset_all();
    id = net_socket_listen(8080U, 1234U); TEST_ASSERT_TRUE(id >= 0);
    TEST_ASSERT_EQUAL(0, net_socket_get_state(id, &state)); TEST_ASSERT_EQUAL(NET_TCP_STATE_LISTEN, state);
    TEST_ASSERT_EQUAL(0, net_socket_accept_syn(id, &syn));
    TEST_ASSERT_EQUAL(0, net_socket_build_syn_ack(id, segment, sizeof(segment), &segment_length));
    TEST_ASSERT_EQUAL(NET_TCP_HEADER_SIZE, segment_length); TEST_ASSERT_EQUAL(NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, segment[13]);
    TEST_ASSERT_EQUAL(0, net_socket_accept_ack(id, &ack));
    TEST_ASSERT_EQUAL(0, net_socket_get_state(id, &state)); TEST_ASSERT_EQUAL(NET_TCP_STATE_ESTABLISHED, state);
    TEST_ASSERT_EQUAL(0, net_socket_close(id));
}

void test_socket_lifecycle_send_receive(void) {
    int id;
    uint8_t segment[64] = {0};
    uint8_t received[8] = {0};
    uint8_t payload[4] = {'P', 'I', 'N', 'G'};
    uint16_t segment_length = 0U, received_length = 0U;
    uint8_t state = 0U;
    net_tcp_view_t view;

    net_socket_reset_all();
    id = net_socket_open(49152U, 443U, 100U);
    TEST_ASSERT_TRUE(id >= 0);
    TEST_ASSERT_EQUAL(0, net_socket_get_state(id, &state));
    TEST_ASSERT_EQUAL(NET_TCP_STATE_SYN_SENT, state);
    view = (net_tcp_view_t){443U, 49152U, 700U, 101U, NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, 0, 0};
    TEST_ASSERT_EQUAL(0, net_socket_accept_syn_ack(id, &view));
    TEST_ASSERT_EQUAL(0, net_socket_get_state(id, &state));
    TEST_ASSERT_EQUAL(NET_TCP_STATE_ESTABLISHED, state);
    TEST_ASSERT_EQUAL(0, net_socket_send(id, payload, sizeof(payload), segment, sizeof(segment), &segment_length));
    TEST_ASSERT_EQUAL(24U, segment_length);
    TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, segment_length, &view));
    TEST_ASSERT_EQUAL(4U, view.payload_length);
    TEST_ASSERT_EQUAL('P', view.payload[0]);
    TEST_ASSERT_EQUAL(24, net_tcp_build_data(segment, sizeof(segment), 443U, 49152U,
                                              701U, 104U, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL(0, net_socket_feed(id, segment, 24U));
    TEST_ASSERT_EQUAL(0, net_socket_receive(id, received, sizeof(received), &received_length));
    TEST_ASSERT_EQUAL(4U, received_length);
    TEST_ASSERT_EQUAL('P', received[0]);
    TEST_ASSERT_EQUAL('G', received[3]);
    TEST_ASSERT_EQUAL(0, net_socket_close(id));
    TEST_ASSERT_EQUAL(NET_SOCKET_NOT_OPEN, net_socket_get_state(id, &state));
}

void test_socket_tls_send_wrapper(void) {
    int id; uint8_t segment[128] = {0}, record[128] = {0}, payload[3] = {'A','I','!' };
    uint16_t segment_length = 0U; uint8_t key_material[40]; uint8_t i;
    net_tls_aes128_gcm_key_block_t block; net_tls_aes_gcm_session_t session; net_tcp_view_t view;
    for (i = 0U; i < sizeof(key_material); i++) key_material[i] = (uint8_t)(i + 1U);
    block = (net_tls_aes128_gcm_key_block_t){key_material, key_material + 16U, key_material + 32U, key_material + 36U};
    TEST_ASSERT_EQUAL(0, net_tls_aes_gcm_session_init(&session, &block, 1U));
    net_socket_reset_all(); id = net_socket_open(49152U, 443U, 100U); TEST_ASSERT_TRUE(id >= 0);
    view = (net_tcp_view_t){443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};
    TEST_ASSERT_EQUAL(0, net_socket_accept_syn_ack(id, &view));
    TEST_ASSERT_EQUAL(0, net_socket_send_tls(id, &session, NET_TLS_CONTENT_APPLICATION_DATA, payload, sizeof(payload), record, sizeof(record), segment, sizeof(segment), &segment_length, 2U));
    TEST_ASSERT_EQUAL(1U, session.write_sequence); TEST_ASSERT_GREATER_THAN(24U, segment_length);
    TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, segment_length, &view)); TEST_ASSERT_GREATER_THAN(3U, view.payload_length);
    TEST_ASSERT_EQUAL(0, net_socket_close(id));
}

void test_socket_build_syn_active(void) {
    int id; uint8_t segment[NET_TCP_HEADER_SIZE] = {0}; uint16_t segment_length = 0U; net_tcp_view_t view;
    net_socket_reset_all(); id = net_socket_open(49152U, 443U, 100U); TEST_ASSERT_TRUE(id >= 0);
    TEST_ASSERT_EQUAL(0, net_socket_build_syn(id, segment, sizeof(segment), &segment_length));
    TEST_ASSERT_EQUAL(NET_TCP_HEADER_SIZE, segment_length); TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, segment_length, &view));
    TEST_ASSERT_EQUAL(49152U, view.source_port); TEST_ASSERT_EQUAL(443U, view.destination_port);
    TEST_ASSERT_EQUAL(100U, view.sequence); TEST_ASSERT_EQUAL(NET_TCP_FLAG_SYN, view.flags);
    TEST_ASSERT_EQUAL(NET_SOCKET_BUFFER_SMALL, net_socket_build_syn(id, segment, NET_TCP_HEADER_SIZE - 1U, &segment_length));
    TEST_ASSERT_EQUAL(0, net_socket_close(id));
}

void test_socket_begin_close_is_bounded(void) {
    int id; uint8_t segment[NET_TCP_HEADER_SIZE] = {0}, state = 0U; uint16_t length = 0U; net_tcp_view_t view;
    net_socket_reset_all(); id = net_socket_open(49152U, 443U, 100U); TEST_ASSERT_TRUE(id >= 0);
    TEST_ASSERT_EQUAL(NET_SOCKET_PROTOCOL, net_socket_begin_close(id, segment, sizeof(segment), &length));
    view = (net_tcp_view_t){443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};
    TEST_ASSERT_EQUAL(0, net_socket_accept_syn_ack(id, &view));
    TEST_ASSERT_EQUAL(0, net_socket_begin_close(id, segment, sizeof(segment), &length));
    TEST_ASSERT_EQUAL(NET_TCP_HEADER_SIZE, length); TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, length, &view));
    TEST_ASSERT_EQUAL(NET_TCP_FLAG_FIN|NET_TCP_FLAG_ACK, view.flags);
    TEST_ASSERT_EQUAL(0, net_socket_get_state(id, &state)); TEST_ASSERT_EQUAL(NET_TCP_STATE_FIN_WAIT_1, state);
    TEST_ASSERT_EQUAL(0, net_socket_close(id));
}

void test_socket_tls_poll_primitives_are_bounded(void) {
    int id; uint8_t segment[128] = {0}, records[128] = {0}, plaintext[32] = {0};
    uint8_t client_random[32] = {0}, client_private[NET_TLS_X25519_KEY_LENGTH] = {0};
    uint8_t master_secret[48] = {0}, key_block[NET_TLS_AES_128_GCM_KEY_BLOCK_LENGTH] = {0};
    uint8_t prf_workspace[128] = {0}; uint32_t rsa_workspace[64] = {0}, x25519_workspace[64] = {0};
    uint16_t segment_length = 0U, consumed = 0U; uint32_t records_length = 0U;
    net_tcp_view_t syn_ack = {443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};
    net_tcp_view_t empty_ack = {443U,49152U,701U,101U,NET_TCP_FLAG_ACK,0,0};
    net_tcp_view_t parsed; net_tcp_connection_t before, after;
    net_tcp_tls_stream_t stream = {0}; net_tls_handshake_t handshake = {0};
    net_tls_transcript_t transcript = {0}; net_tls_x25519_context_t x25519 = {0};
    net_tls_aes_gcm_session_t session = {0};

    net_socket_reset_all(); id = net_socket_open(49152U, 443U, 100U); TEST_ASSERT_TRUE(id >= 0);
    TEST_ASSERT_EQUAL(0, net_socket_accept_syn_ack(id, &syn_ack));
    TEST_ASSERT_EQUAL(0, net_socket_build_ack(id, segment, sizeof(segment), &segment_length));
    TEST_ASSERT_EQUAL(NET_TCP_HEADER_SIZE, segment_length);
    TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, segment_length, &parsed));
    TEST_ASSERT_EQUAL(NET_TCP_FLAG_ACK, parsed.flags); TEST_ASSERT_EQUAL(101U, parsed.sequence);
    TEST_ASSERT_EQUAL(701U, parsed.acknowledgment);
    TEST_ASSERT_EQUAL(NET_SOCKET_PROTOCOL, net_socket_commit_send(id, 0U));
    TEST_ASSERT_EQUAL(NET_SOCKET_BAD_ARGUMENT, net_socket_build_ack(-1, segment, sizeof(segment), &segment_length));

    TEST_ASSERT_EQUAL(0, net_socket_connection_snapshot(id, &before));
    TEST_ASSERT_EQUAL(NET_SOCKET_PROTOCOL, net_socket_accept_tls_authenticated_fragment(id, &empty_ack, &stream,
                      &handshake, client_random, &transcript, rsa_workspace, 64U, &consumed));
    TEST_ASSERT_EQUAL(0, net_socket_connection_snapshot(id, &after));
    TEST_ASSERT_EQUAL(before.remote_sequence, after.remote_sequence);
    TEST_ASSERT_EQUAL(NET_SOCKET_PROTOCOL, net_socket_build_tls_x25519_flight(id, &handshake, &x25519,
                      client_private, client_random, &transcript, master_secret, key_block, &session,
                      segment, sizeof(segment), records, sizeof(records), &records_length, x25519_workspace,
                      64U, prf_workspace, sizeof(prf_workspace), 1U));
    TEST_ASSERT_EQUAL(NET_SOCKET_PROTOCOL, net_socket_accept_tls_x25519_postflight(id, &handshake, &transcript,
                      master_secret, &session, &empty_ack, plaintext, sizeof(plaintext), prf_workspace,
                      sizeof(prf_workspace), &consumed));
    TEST_ASSERT_EQUAL(0, net_socket_close(id));
}

void test_multisocket_concurrency_and_reuse(void) {
    int ids[NET_SOCKET_CAPACITY];
    int extra_id;
    uint32_t i;
    uint8_t state = 0U;

    net_socket_reset_all();
    for (i = 0U; i < NET_SOCKET_CAPACITY; i++) {
        ids[i] = net_socket_open((uint16_t)(50000U + i), 443U, 1000U + i);
        TEST_ASSERT_TRUE(ids[i] >= 0);
        TEST_ASSERT_EQUAL(0, net_socket_get_state(ids[i], &state));
        TEST_ASSERT_EQUAL(NET_TCP_STATE_SYN_SENT, state);
    }

    extra_id = net_socket_open(50100U, 443U, 2000U);
    TEST_ASSERT_EQUAL(NET_SOCKET_NO_SLOT, extra_id);

    TEST_ASSERT_EQUAL(0, net_socket_close(ids[1]));
    TEST_ASSERT_EQUAL(NET_SOCKET_NOT_OPEN, net_socket_get_state(ids[1], &state));

    extra_id = net_socket_open(50100U, 443U, 2000U);
    TEST_ASSERT_EQUAL(ids[1], extra_id);
    TEST_ASSERT_EQUAL(0, net_socket_get_state(extra_id, &state));
    TEST_ASSERT_EQUAL(NET_TCP_STATE_SYN_SENT, state);

    net_socket_reset_all();
    for (i = 0U; i < NET_SOCKET_CAPACITY; i++) {
        TEST_ASSERT_EQUAL(NET_SOCKET_NOT_OPEN, net_socket_get_state((int)i, &state));
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_socket_lifecycle_send_receive);
    RUN_TEST(test_socket_build_syn_active);
    RUN_TEST(test_socket_tls_send_wrapper);
    RUN_TEST(test_socket_passive_lifecycle);
    RUN_TEST(test_socket_feed_routes_passive_segments);
    RUN_TEST(test_socket_tls_poll_primitives_are_bounded);
    RUN_TEST(test_socket_begin_close_is_bounded);
    RUN_TEST(test_multisocket_concurrency_and_reuse);
    unity_print_results();
    unity_cleanup();
    return 0;
}

void test_socket_feed_routes_passive_segments(void) {
    int id; uint8_t syn_segment[20] = {0}; uint8_t ack_segment[20] = {0}; uint8_t state = 0U;
    TEST_ASSERT_EQUAL(NET_TCP_HEADER_SIZE, net_tcp_build_syn(syn_segment, sizeof(syn_segment), 40000U, 8080U, 900U));
    TEST_ASSERT_EQUAL(NET_TCP_HEADER_SIZE, net_tcp_build_ack(ack_segment, sizeof(ack_segment), 40000U, 8080U, 901U, 1235U));
    net_socket_reset_all(); id = net_socket_listen(8080U, 1234U); TEST_ASSERT_TRUE(id >= 0);
    TEST_ASSERT_EQUAL(0, net_socket_feed(id, syn_segment, sizeof(syn_segment)));
    TEST_ASSERT_EQUAL(0, net_socket_get_state(id, &state)); TEST_ASSERT_EQUAL(NET_TCP_STATE_SYN_RECEIVED, state);
    TEST_ASSERT_EQUAL(0, net_socket_feed(id, ack_segment, sizeof(ack_segment)));
    TEST_ASSERT_EQUAL(0, net_socket_get_state(id, &state)); TEST_ASSERT_EQUAL(NET_TCP_STATE_ESTABLISHED, state);
    TEST_ASSERT_EQUAL(0, net_socket_close(id));
}
