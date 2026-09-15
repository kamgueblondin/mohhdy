#include "net_llm_socket.h"

int net_llm_socket_build_request(int socket_id, net_tls_aes_gcm_session_t* session,
                                 uint8_t provider, uint8_t stream,
                                 uint8_t* json, uint16_t json_capacity,
                                 uint8_t* request, uint16_t request_capacity,
                                 const char* host, const char* path,
                                 const char* bearer_token, const char* model,
                                 const uint8_t* prompt, uint16_t prompt_length,
                                 uint8_t* tls_record, uint32_t tls_capacity,
                                 uint8_t* tcp_segment, uint16_t tcp_capacity,
                                 uint8_t retransmit_limit) {
    int json_length;
    int request_length;
    uint16_t segment_length = 0U;

    if (!session || !json || !request || !host || !path || !model ||
        (!prompt && prompt_length) || !tls_record || !tcp_segment) return -1;
    if (provider != NET_LLM_SOCKET_PROVIDER_OLLAMA && provider != NET_LLM_SOCKET_PROVIDER_OPENAI) return -2;

    if (provider == NET_LLM_SOCKET_PROVIDER_OLLAMA) {
        json_length = stream
            ? net_llm_build_ollama_generate_stream_json(json, json_capacity, model, prompt, prompt_length)
            : net_llm_build_ollama_generate_json(json, json_capacity, model, prompt, prompt_length);
    } else {
        if (!bearer_token || !bearer_token[0]) return -3;
        json_length = stream
            ? net_llm_build_openai_chat_stream_json(json, json_capacity, model, prompt, prompt_length)
            : net_llm_build_openai_chat_json(json, json_capacity, model, prompt, prompt_length);
    }
    if (json_length < 0) return -4;

    request_length = provider == NET_LLM_SOCKET_PROVIDER_OPENAI
        ? net_http_build_post_json_bearer(request, request_capacity, host, path, bearer_token,
                                           json, (uint16_t)json_length)
        : net_http_build_post_json(request, request_capacity, host, path, json, (uint16_t)json_length);
    if (request_length < 0) return -5;

    if (net_socket_send_tls(socket_id, session, NET_TLS_CONTENT_APPLICATION_DATA,
                            request, (uint16_t)request_length, tls_record, tls_capacity,
                            tcp_segment, tcp_capacity, &segment_length,
                            retransmit_limit) != 0) return -6;
    return (int)segment_length;
}

int net_llm_socket_build_sse_resume(int socket_id, net_tls_aes_gcm_session_t* session,
                                    uint8_t* request, uint16_t request_capacity,
                                    const char* host, const char* path,
                                    const net_llm_sse_response_t* response,
                                    uint8_t* tls_record, uint32_t tls_capacity,
                                    uint8_t* tcp_segment, uint16_t tcp_capacity,
                                    uint8_t retransmit_limit) {
    int request_length;
    uint16_t segment_length = 0U;

    if (!session || !request || !host || !path || !response || !tls_record || !tcp_segment) return -1;
    request_length = net_llm_sse_build_resume_get(request, request_capacity, host, path, response);
    if (request_length < 0) return -2;
    if (net_socket_send_tls(socket_id, session, NET_TLS_CONTENT_APPLICATION_DATA,
                            request, (uint16_t)request_length, tls_record, tls_capacity,
                            tcp_segment, tcp_capacity, &segment_length,
                            retransmit_limit) != 0) return -3;
    return (int)segment_length;
}

int net_llm_socket_open_response(int socket_id, net_tls_aes_gcm_session_t* session,
                                 const net_tcp_view_t* view, uint8_t* plaintext,
                                 uint16_t plaintext_capacity,
                                 net_http_response_accumulator_t* accumulator,
                                 net_http_response_view_t* response,
                                 uint16_t* consumed) {
    net_tcp_connection_t previous_connection;
    net_tls_aes_gcm_session_t previous_session;
    net_http_response_accumulator_t previous_accumulator;
    net_tls_record_view_t record;
    int status;
    if (!session || !view || !plaintext || !accumulator || !response || !consumed) return -1;
    if (net_socket_connection_snapshot(socket_id, &previous_connection) != 0) return -2;
    previous_session = *session; previous_accumulator = *accumulator;
    status = net_socket_receive_tls(socket_id, session, view, plaintext, plaintext_capacity, &record, consumed);
    if (status != 0 || record.content_type != NET_TLS_CONTENT_APPLICATION_DATA) goto rollback;
    status = net_http_response_accumulator_feed(accumulator, record.payload, record.payload_length, response);
    if (status >= 0) return status;
rollback:
    (void)net_socket_connection_restore(socket_id, &previous_connection);
    *session = previous_session; *accumulator = previous_accumulator; *consumed = 0U;
    return -3;
}

int net_llm_socket_open_sse(int socket_id, net_tls_aes_gcm_session_t* session,
                            const net_tcp_view_t* view, uint8_t* plaintext,
                            uint16_t plaintext_capacity, net_llm_sse_response_t* response,
                            uint8_t provider, uint8_t* text, uint16_t text_capacity,
                            uint16_t* text_length, uint16_t* consumed) {
    net_tcp_connection_t previous_connection;
    net_tls_aes_gcm_session_t previous_session;
    net_llm_sse_response_t previous_response;
    net_tls_record_view_t record;
    int status;
    if (!session || !view || !plaintext || !response || !text || !text_length || !consumed) return -1;
    if (net_socket_connection_snapshot(socket_id, &previous_connection) != 0) return -2;
    previous_session = *session; previous_response = *response;
    status = net_socket_receive_tls(socket_id, session, view, plaintext, plaintext_capacity, &record, consumed);
    if (status != 0) goto rollback;
    if (record.content_type == NET_TLS_CONTENT_ALERT) {
        if (net_tls_close_notify_parse(&record) == 0) return NET_HTTP_TLS_STATUS_CLOSE_NOTIFY;
        goto rollback;
    }
    if (record.content_type != NET_TLS_CONTENT_APPLICATION_DATA) goto rollback;
    status = net_llm_sse_response_feed(response, provider, record.payload, record.payload_length,
                                       text, text_capacity, text_length);
    if (status >= 0) return status;
rollback:
    (void)net_socket_connection_restore(socket_id, &previous_connection);
    *session = previous_session; *response = previous_response; *text_length = 0U; *consumed = 0U;
    return -3;
}
