#include "../../framework/unity.h"
#include "../../../kernel/net_llm_socket.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void init_socket_session(int* socket_id, net_tls_aes_gcm_session_t* session,
                                uint8_t key_material[40]) {
    net_tls_aes128_gcm_key_block_t block;
    net_tcp_view_t syn_ack;
    uint8_t i;
    for (i = 0U; i < 40U; i++) key_material[i] = (uint8_t)(i + 1U);
    block = (net_tls_aes128_gcm_key_block_t){key_material, key_material + 16U,
                                             key_material + 32U, key_material + 36U};
    TEST_ASSERT_EQUAL(0, net_tls_aes_gcm_session_init(session, &block, 1U));
    net_socket_reset_all();
    *socket_id = net_socket_open(49152U, 443U, 100U);
    TEST_ASSERT_TRUE(*socket_id >= 0);
    syn_ack = (net_tcp_view_t){443U, 49152U, 700U, 101U,
                                NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, 0, 0};
    TEST_ASSERT_EQUAL(0, net_socket_accept_syn_ack(*socket_id, &syn_ack));
}

void test_llm_socket_builds_openai_stream_request(void) {
    int socket_id;
    net_tls_aes_gcm_session_t session;
    uint8_t key_material[40], json[256] = {0}, request[512] = {0};
    uint8_t record[640] = {0}, segment[700] = {0};
    net_tcp_view_t view;
    int built;

    init_socket_session(&socket_id, &session, key_material);
    built = net_llm_socket_build_request(socket_id, &session,
                                         NET_LLM_SOCKET_PROVIDER_OPENAI, 1U,
                                         json, sizeof(json), request, sizeof(request),
                                         "api.example.test", "/v1/chat/completions", "sk-test",
                                         "gpt-4o-mini", (const uint8_t*)"bonjour", 7U,
                                         record, sizeof(record), segment, sizeof(segment), 2U);
    TEST_ASSERT_GREATER_THAN(20U, built);
    TEST_ASSERT_NOT_NULL(strstr((const char*)json, "\"stream\":true"));
    TEST_ASSERT_NOT_NULL(strstr((const char*)request, "Authorization: Bearer sk-test\r\n"));
    TEST_ASSERT_NOT_NULL(strstr((const char*)request, "POST /v1/chat/completions HTTP/1.1\r\n"));
    TEST_ASSERT_EQUAL(1U, session.write_sequence);
    TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, (uint16_t)built, &view));
    TEST_ASSERT_GREATER_THAN(7U, view.payload_length);
    TEST_ASSERT_EQUAL(0, net_socket_close(socket_id));
}

void test_llm_socket_rejects_missing_openai_bearer(void) {
    int socket_id;
    net_tls_aes_gcm_session_t session;
    uint8_t key_material[40], json[64] = {0}, request[128] = {0};
    uint8_t record[128] = {0}, segment[160] = {0};

    init_socket_session(&socket_id, &session, key_material);
    TEST_ASSERT_EQUAL(-3, net_llm_socket_build_request(socket_id, &session,
                                                         NET_LLM_SOCKET_PROVIDER_OPENAI, 0U,
                                                         json, sizeof(json), request, sizeof(request),
                                                         "api.example.test", "/v1/chat", 0,
                                                         "model", (const uint8_t*)"x", 1U,
                                                         record, sizeof(record), segment, sizeof(segment), 1U));
    TEST_ASSERT_EQUAL(0U, session.write_sequence);
    TEST_ASSERT_EQUAL(0, net_socket_close(socket_id));
}

void test_llm_socket_builds_sse_resume_request(void) {
    int socket_id;
    net_tls_aes_gcm_session_t session;
    net_llm_sse_response_t response;
    uint8_t key_material[40], request[256] = {0}, record[320] = {0}, segment[380] = {0};
    uint8_t http_buffer[64] = {0}, sse_buffer[64] = {0};
    net_tcp_view_t view;
    int built;

    init_socket_session(&socket_id, &session, key_material);
    TEST_ASSERT_EQUAL(0, net_llm_sse_response_init(&response, http_buffer, sizeof(http_buffer),
                                                   sse_buffer, sizeof(sse_buffer)));
    memcpy(response.sse.event_id, "evt-42", 6U);
    response.sse.event_id_length = 6U;
    response.sse.event_id_valid = 1U;
    built = net_llm_socket_build_sse_resume(socket_id, &session, request, sizeof(request),
                                             "api.example.test", "/v1/chat", &response,
                                             record, sizeof(record), segment, sizeof(segment), 2U);
    TEST_ASSERT_GREATER_THAN(20U, built);
    TEST_ASSERT_NOT_NULL(strstr((const char*)request, "GET /v1/chat HTTP/1.1\r\n"));
    TEST_ASSERT_NOT_NULL(strstr((const char*)request, "Last-Event-ID: evt-42\r\n"));
    TEST_ASSERT_EQUAL(1U, session.write_sequence);
    TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, (uint16_t)built, &view));
    TEST_ASSERT_GREATER_THAN(7U, view.payload_length);
    response.sse.event_id_valid = 0U;
    TEST_ASSERT_EQUAL(-2, net_llm_socket_build_sse_resume(socket_id, &session, request, sizeof(request),
                                                           "api.example.test", "/v1/chat", &response,
                                                           record, sizeof(record), segment, sizeof(segment), 2U));
    TEST_ASSERT_EQUAL(1U, session.write_sequence);
    TEST_ASSERT_EQUAL(0, net_socket_close(socket_id));
}

void test_llm_socket_opens_http_response(void) {
    int socket_id, record_length, segment_length; net_tls_aes_gcm_session_t client, server;
    net_tls_aes128_gcm_key_block_t block; net_tcp_view_t view; net_http_response_accumulator_t accumulator;
    net_http_response_view_t response; uint8_t key_material[40], encrypted[160] = {0}, segment[200] = {0};
    uint8_t plaintext[160] = {0}, http_buffer[160] = {0}; uint16_t consumed = 0U; uint8_t i;
    static const uint8_t http[] = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
    init_socket_session(&socket_id, &client, key_material);
    block = (net_tls_aes128_gcm_key_block_t){key_material, key_material + 16U, key_material + 32U, key_material + 36U};
    TEST_ASSERT_EQUAL(0, net_tls_aes_gcm_session_init(&server, &block, 0U));
    record_length = net_tls_aes_gcm_session_build(&server, encrypted, sizeof(encrypted), NET_TLS_CONTENT_APPLICATION_DATA, http, sizeof(http) - 1U);
    TEST_ASSERT_GREATER_THAN(0, record_length);
    segment_length = net_tcp_build_data(segment, sizeof(segment), 443U, 49152U, 701U, 101U, encrypted, (uint16_t)record_length);
    TEST_ASSERT_GREATER_THAN(0, segment_length); TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, (uint16_t)segment_length, &view));
    TEST_ASSERT_EQUAL(0, net_http_response_accumulator_init(&accumulator, http_buffer, sizeof(http_buffer)));
    TEST_ASSERT_EQUAL(0, net_llm_socket_open_response(socket_id, &client, &view, plaintext, sizeof(plaintext), &accumulator, &response, &consumed));
    TEST_ASSERT_EQUAL(200U, response.status_code); TEST_ASSERT_EQUAL(2U, response.body_length); TEST_ASSERT_EQUAL('O', response.body[0]); TEST_ASSERT_EQUAL('K', response.body[1]);
    TEST_ASSERT_EQUAL((uint16_t)record_length, consumed); TEST_ASSERT_EQUAL(1U, client.read_sequence);
    for (i = 0U; i < sizeof(key_material); i++) TEST_ASSERT_EQUAL((uint8_t)(i + 1U), key_material[i]);
    TEST_ASSERT_EQUAL(0, net_socket_close(socket_id));
}

void test_llm_socket_propagates_peer_close_notify(void) {
    int socket_id, record_length, segment_length;
    net_tls_aes_gcm_session_t client, server;
    net_tls_aes128_gcm_key_block_t block;
    net_tcp_view_t view;
    net_llm_sse_response_t response;
    uint8_t key_material[40], encrypted[48] = {0}, segment[80] = {0}, plaintext[8] = {0};
    uint8_t http_buffer[32] = {0}, sse_buffer[32] = {0}, text[8] = {0};
    uint16_t consumed = 0U, text_length = 7U;

    init_socket_session(&socket_id, &client, key_material);
    block = (net_tls_aes128_gcm_key_block_t){key_material, key_material + 16U,
                                             key_material + 32U, key_material + 36U};
    TEST_ASSERT_EQUAL(0, net_tls_aes_gcm_session_init(&server, &block, 0U));
    record_length = net_tls_close_notify_build(&server, encrypted, sizeof(encrypted));
    TEST_ASSERT_EQUAL(31, record_length);
    segment_length = net_tcp_build_data(segment, sizeof(segment), 443U, 49152U, 701U, 101U,
                                        encrypted, (uint16_t)record_length);
    TEST_ASSERT_GREATER_THAN(0, segment_length);
    TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, (uint16_t)segment_length, &view));
    TEST_ASSERT_EQUAL(0, net_llm_sse_response_init(&response, http_buffer, sizeof(http_buffer),
                                                   sse_buffer, sizeof(sse_buffer)));
    TEST_ASSERT_EQUAL(NET_HTTP_TLS_STATUS_CLOSE_NOTIFY,
                      net_llm_socket_open_sse(socket_id, &client, &view, plaintext,
                                              sizeof(plaintext), &response,
                                              NET_LLM_SOCKET_PROVIDER_OPENAI, text, sizeof(text),
                                              &text_length, &consumed));
    TEST_ASSERT_EQUAL(1U, client.read_sequence);
    TEST_ASSERT_EQUAL(31U, consumed);
    TEST_ASSERT_EQUAL(7U, text_length);
    TEST_ASSERT_EQUAL(0U, response.http.length);
    TEST_ASSERT_EQUAL(0, net_socket_close(socket_id));
}

void test_llm_socket_opens_openai_sse(void) {
    int socket_id, record_length, segment_length; net_tls_aes_gcm_session_t client, server;
    net_tls_aes128_gcm_key_block_t block; net_tcp_view_t view; net_llm_sse_response_t response;
    uint8_t key_material[40], encrypted[256] = {0}, segment[300] = {0}, plaintext[256] = {0};
    uint8_t http_buffer[160] = {0}, sse_buffer[96] = {0}, text[32] = {0}; uint16_t consumed = 0U, text_length = 0U;
    static const uint8_t input[] = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n33\r\ndata: {\"choices\":[{\"delta\":{\"content\":\"salut\"}}]}\n\n\r\n0\r\n\r\n";
    init_socket_session(&socket_id, &client, key_material);
    block = (net_tls_aes128_gcm_key_block_t){key_material, key_material + 16U, key_material + 32U, key_material + 36U};
    TEST_ASSERT_EQUAL(0, net_tls_aes_gcm_session_init(&server, &block, 0U));
    record_length = net_tls_aes_gcm_session_build(&server, encrypted, sizeof(encrypted), NET_TLS_CONTENT_APPLICATION_DATA, input, sizeof(input) - 1U);
    TEST_ASSERT_GREATER_THAN(0, record_length);
    segment_length = net_tcp_build_data(segment, sizeof(segment), 443U, 49152U, 701U, 101U, encrypted, (uint16_t)record_length);
    TEST_ASSERT_GREATER_THAN(0, segment_length); TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, (uint16_t)segment_length, &view));
    TEST_ASSERT_EQUAL(0, net_llm_sse_response_init(&response, http_buffer, sizeof(http_buffer), sse_buffer, sizeof(sse_buffer)));
    TEST_ASSERT_EQUAL(0, net_llm_socket_open_sse(socket_id, &client, &view, plaintext, sizeof(plaintext), &response, NET_LLM_SOCKET_PROVIDER_OPENAI, text, sizeof(text), &text_length, &consumed));
    TEST_ASSERT_EQUAL(5U, text_length); TEST_ASSERT_EQUAL_MEMORY("salut", text, 5U); TEST_ASSERT_EQUAL(1U, client.read_sequence);
    TEST_ASSERT_EQUAL(0, net_socket_close(socket_id));
}

int main(void) {
    unity_init();
    RUN_TEST(test_llm_socket_builds_openai_stream_request);
    RUN_TEST(test_llm_socket_rejects_missing_openai_bearer);
    RUN_TEST(test_llm_socket_builds_sse_resume_request);
    RUN_TEST(test_llm_socket_opens_http_response);
    RUN_TEST(test_llm_socket_propagates_peer_close_notify);
    RUN_TEST(test_llm_socket_opens_openai_sse);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
