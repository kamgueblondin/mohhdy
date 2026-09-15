#include "net_socket.h"

static net_socket_slot_t sockets[NET_SOCKET_CAPACITY];

static int valid_id(int socket_id) {
    return socket_id >= 0 && (uint32_t)socket_id < NET_SOCKET_CAPACITY && sockets[socket_id].used;
}

int net_socket_open(uint16_t local_port, uint16_t remote_port, uint32_t local_sequence) {
    uint32_t i;
    for (i = 0U; i < NET_SOCKET_CAPACITY; i++) {
        if (!sockets[i].used) {
            sockets[i].used = 1U;
            sockets[i].receive_length = 0U;
            sockets[i].receive_offset = 0U;
            if (net_tcp_connection_open(&sockets[i].connection, local_port, remote_port, local_sequence) != 0) {
                sockets[i].used = 0U;
                return NET_SOCKET_PROTOCOL;
            }
            return (int)i;
        }
    }
    return NET_SOCKET_NO_SLOT;
}

int net_socket_build_syn(int socket_id, uint8_t* segment, uint16_t capacity,
                         uint16_t* out_length) {
    int built;
    if (!valid_id(socket_id) || !segment || !out_length) return NET_SOCKET_BAD_ARGUMENT;
    if (sockets[socket_id].connection.state != NET_TCP_STATE_SYN_SENT) return NET_SOCKET_NOT_CONNECTED;
    built = net_tcp_build_syn(segment, capacity, sockets[socket_id].connection.local_port,
                              sockets[socket_id].connection.remote_port,
                              sockets[socket_id].connection.local_sequence - 1U);
    if (built < 0) return NET_SOCKET_BUFFER_SMALL;
    *out_length = (uint16_t)built;
    return 0;
}

int net_socket_close(int socket_id) {
    if (!valid_id(socket_id)) return NET_SOCKET_NOT_OPEN;
    sockets[socket_id].used = 0U;
    sockets[socket_id].receive_length = 0U;
    sockets[socket_id].receive_offset = 0U;
    sockets[socket_id].connection.state = NET_TCP_STATE_CLOSED;
    return 0;
}

int net_socket_accept_syn_ack(int socket_id, const net_tcp_view_t* view) {
    if (!valid_id(socket_id) || !view) return NET_SOCKET_BAD_ARGUMENT;
    return net_tcp_connection_accept_syn_ack(&sockets[socket_id].connection, view) == 0 ? 0 : NET_SOCKET_PROTOCOL;
}

int net_socket_send_limit(int socket_id, const uint8_t* payload, uint16_t length,
                          uint8_t* segment, uint16_t capacity, uint16_t* out_length,
                          uint8_t retransmit_limit) {
    int built;
    if (!valid_id(socket_id) || !payload || !segment || !out_length) return NET_SOCKET_BAD_ARGUMENT;
    if (sockets[socket_id].connection.state != NET_TCP_STATE_ESTABLISHED) return NET_SOCKET_NOT_CONNECTED;
    if (length == 0U || length > NET_SOCKET_TX_CAPACITY) return NET_SOCKET_BUFFER_SMALL;
    built = net_tcp_connection_build_data(&sockets[socket_id].connection, segment, capacity,
                                          payload, length, retransmit_limit);
    if (built < 0) return NET_SOCKET_PROTOCOL;
    if (net_tcp_connection_commit_send(&sockets[socket_id].connection, length) != 0) return NET_SOCKET_PROTOCOL;
    *out_length = (uint16_t)built;
    return 0;
}

int net_socket_send(int socket_id, const uint8_t* payload, uint16_t length,
                    uint8_t* segment, uint16_t capacity, uint16_t* out_length) {
    return net_socket_send_limit(socket_id, payload, length, segment, capacity, out_length, 3U);
}

int net_socket_feed(int socket_id, const uint8_t* segment, uint16_t length) {
    net_tcp_view_t view;
    uint16_t accepted = 0U;
    uint16_t available;
    if (!valid_id(socket_id) || !segment) return NET_SOCKET_BAD_ARGUMENT;
    if (net_tcp_parse(segment, length, &view) != 0) return NET_SOCKET_PROTOCOL;
    if (sockets[socket_id].connection.state == NET_TCP_STATE_LISTEN) {
        return net_tcp_connection_accept_syn(&sockets[socket_id].connection, &view) == 0 ? 0 : NET_SOCKET_PROTOCOL;
    }
    if (sockets[socket_id].connection.state == NET_TCP_STATE_SYN_RECEIVED) {
        return net_tcp_connection_accept_ack(&sockets[socket_id].connection, &view) == 0 ? 0 : NET_SOCKET_PROTOCOL;
    }
    if (net_tcp_connection_accept_data(&sockets[socket_id].connection, &view, &accepted) != 0) return NET_SOCKET_PROTOCOL;
    available = (uint16_t)(NET_SOCKET_RX_CAPACITY - sockets[socket_id].receive_length);
    if (accepted > available) return NET_SOCKET_BUFFER_SMALL;
    for (uint16_t i = 0U; i < accepted; i++) sockets[socket_id].receive_buffer[sockets[socket_id].receive_length + i] = view.payload[i];
    sockets[socket_id].receive_length = (uint16_t)(sockets[socket_id].receive_length + accepted);
    return 0;
}

int net_socket_receive(int socket_id, uint8_t* buffer, uint16_t capacity, uint16_t* out_length) {
    uint16_t available, amount;
    if (!valid_id(socket_id) || !buffer || !out_length) return NET_SOCKET_BAD_ARGUMENT;
    available = (uint16_t)(sockets[socket_id].receive_length - sockets[socket_id].receive_offset);
    amount = available < capacity ? available : capacity;
    for (uint16_t i = 0U; i < amount; i++) buffer[i] = sockets[socket_id].receive_buffer[sockets[socket_id].receive_offset + i];
    sockets[socket_id].receive_offset = (uint16_t)(sockets[socket_id].receive_offset + amount);
    if (sockets[socket_id].receive_offset == sockets[socket_id].receive_length) {
        sockets[socket_id].receive_offset = 0U;
        sockets[socket_id].receive_length = 0U;
    }
    *out_length = amount;
    return 0;
}

int net_socket_send_tls(int socket_id, net_tls_aes_gcm_session_t* session, uint8_t content_type,
                        const uint8_t* plaintext, uint16_t plaintext_length,
                        uint8_t* record, uint32_t record_capacity, uint8_t* segment,
                        uint16_t segment_capacity, uint16_t* out_segment_length,
                        uint8_t retransmit_limit) {
    int built;
    uint16_t record_length;
    if (!valid_id(socket_id) || !session || !plaintext || !record || !segment || !out_segment_length) return NET_SOCKET_BAD_ARGUMENT;
    if (sockets[socket_id].connection.state != NET_TCP_STATE_ESTABLISHED) return NET_SOCKET_NOT_CONNECTED;
    if (plaintext_length == 0U || plaintext_length > NET_SOCKET_TX_CAPACITY) return NET_SOCKET_BUFFER_SMALL;
    built = net_tcp_connection_build_tls_aes_gcm(&sockets[socket_id].connection, session, segment, segment_capacity,
                                                  record, record_capacity, content_type, plaintext, plaintext_length,
                                                  retransmit_limit);
    if (built < 0) return NET_SOCKET_PROTOCOL;
    record_length = sockets[socket_id].connection.pending_length;
    if (record_length == 0U || net_tcp_connection_commit_send(&sockets[socket_id].connection, record_length) != 0) return NET_SOCKET_PROTOCOL;
    *out_segment_length = (uint16_t)built;
    return 0;
}

int net_socket_receive_tls(int socket_id, net_tls_aes_gcm_session_t* session,
                           const net_tcp_view_t* view, uint8_t* plaintext,
                           uint16_t plaintext_capacity, net_tls_record_view_t* out_record,
                           uint16_t* consumed) {
    if (!valid_id(socket_id) || !session || !view || !plaintext || !out_record || !consumed) return NET_SOCKET_BAD_ARGUMENT;
    if (sockets[socket_id].connection.state != NET_TCP_STATE_ESTABLISHED) return NET_SOCKET_NOT_CONNECTED;
    return net_tcp_connection_accept_tls_aes_gcm(&sockets[socket_id].connection, session, view,
                                                  plaintext, plaintext_capacity, out_record, consumed) == 0
           ? 0 : NET_SOCKET_PROTOCOL;
}

int net_socket_get_state(int socket_id, uint8_t* out_state) {
    if (!valid_id(socket_id)) return NET_SOCKET_NOT_OPEN;
    if (!out_state) return NET_SOCKET_BAD_ARGUMENT;
    *out_state = sockets[socket_id].connection.state;
    return 0;
}

int net_socket_connection_snapshot(int socket_id, net_tcp_connection_t* out_connection) {
    if (!valid_id(socket_id)) return NET_SOCKET_NOT_OPEN;
    if (!out_connection) return NET_SOCKET_BAD_ARGUMENT;
    *out_connection = sockets[socket_id].connection;
    return 0;
}

int net_socket_connection_restore(int socket_id, const net_tcp_connection_t* connection) {
    if (!valid_id(socket_id)) return NET_SOCKET_NOT_OPEN;
    if (!connection) return NET_SOCKET_BAD_ARGUMENT;
    sockets[socket_id].connection = *connection;
    return 0;
}

int net_socket_build_ack(int socket_id, uint8_t* segment, uint16_t capacity, uint16_t* out_length) {
    int built;
    if (!valid_id(socket_id) || !segment || !out_length) return NET_SOCKET_BAD_ARGUMENT;
    built = net_tcp_connection_build_ack(&sockets[socket_id].connection, segment, capacity);
    if (built < 0) return NET_SOCKET_PROTOCOL;
    *out_length = (uint16_t)built;
    return 0;
}

int net_socket_begin_close(int socket_id, uint8_t* segment, uint16_t capacity, uint16_t* out_length) {
    int built;
    if (!valid_id(socket_id) || !segment || !out_length) return NET_SOCKET_BAD_ARGUMENT;
    built = net_tcp_connection_begin_close(&sockets[socket_id].connection, segment, capacity);
    if (built < 0) return NET_SOCKET_PROTOCOL;
    *out_length = (uint16_t)built;
    return 0;
}

int net_socket_commit_send(int socket_id, uint16_t payload_length) {
    if (!valid_id(socket_id)) return NET_SOCKET_BAD_ARGUMENT;
    return net_tcp_connection_commit_send(&sockets[socket_id].connection, payload_length) == 0 ? 0 : NET_SOCKET_PROTOCOL;
}

int net_socket_accept_tls_authenticated_fragment(int socket_id, const net_tcp_view_t* view,
                                                 net_tcp_tls_stream_t* stream, net_tls_handshake_t* handshake,
                                                 const uint8_t client_random[32], net_tls_transcript_t* transcript,
                                                 uint32_t* rsa_workspace, uint16_t rsa_workspace_length,
                                                 uint16_t* consumed) {
    int status;
    if (!valid_id(socket_id)) return NET_SOCKET_BAD_ARGUMENT;
    status = net_tcp_connection_accept_tls_authenticated_fragment(&sockets[socket_id].connection, view, stream,
                                                                    handshake, client_random, transcript,
                                                                    rsa_workspace, rsa_workspace_length, consumed);
    return status < 0 ? NET_SOCKET_PROTOCOL : status;
}

int net_socket_build_tls_x25519_flight(int socket_id, net_tls_handshake_t* handshake,
                                       net_tls_x25519_context_t* context,
                                       const uint8_t client_private[NET_TLS_X25519_KEY_LENGTH],
                                       const uint8_t client_random[32], net_tls_transcript_t* transcript,
                                       uint8_t master_secret[48],
                                       uint8_t key_block[NET_TLS_AES_128_GCM_KEY_BLOCK_LENGTH],
                                       net_tls_aes_gcm_session_t* session, uint8_t* segment,
                                       uint16_t segment_capacity, uint8_t* records,
                                       uint32_t records_capacity, uint32_t* records_length,
                                       uint32_t* x25519_workspace, uint16_t x25519_workspace_length,
                                       uint8_t* prf_workspace, uint32_t prf_workspace_capacity,
                                       uint8_t retransmit_limit) {
    int status;
    if (!valid_id(socket_id)) return NET_SOCKET_BAD_ARGUMENT;
    status = net_tcp_connection_build_tls_x25519_flight(&sockets[socket_id].connection, handshake, context,
                                                         client_private, client_random, transcript, master_secret,
                                                         key_block, session, segment, segment_capacity, records,
                                                         records_capacity, records_length, x25519_workspace,
                                                         x25519_workspace_length, prf_workspace,
                                                         prf_workspace_capacity, retransmit_limit);
    return status < 0 ? NET_SOCKET_PROTOCOL : status;
}

int net_socket_accept_tls_x25519_postflight(int socket_id, net_tls_handshake_t* handshake,
                                            net_tls_transcript_t* transcript,
                                            const uint8_t master_secret[48],
                                            net_tls_aes_gcm_session_t* session,
                                            const net_tcp_view_t* view, uint8_t* plaintext,
                                            uint16_t plaintext_capacity, uint8_t* prf_workspace,
                                            uint32_t prf_workspace_capacity, uint16_t* consumed) {
    int status;
    if (!valid_id(socket_id)) return NET_SOCKET_BAD_ARGUMENT;
    status = net_tcp_connection_accept_tls_x25519_postflight(&sockets[socket_id].connection, handshake,
                                                              transcript, master_secret, session, view, plaintext,
                                                              plaintext_capacity, prf_workspace,
                                                              prf_workspace_capacity, consumed);
    return status < 0 ? NET_SOCKET_PROTOCOL : status;
}

void net_socket_reset_all(void) {
    uint32_t i;
    for (i = 0U; i < NET_SOCKET_CAPACITY; i++) {
        sockets[i].used = 0U;
        sockets[i].receive_length = 0U;
        sockets[i].receive_offset = 0U;
        sockets[i].connection.state = NET_TCP_STATE_CLOSED;
    }
}

int net_socket_listen(uint16_t local_port, uint32_t local_sequence) {
    uint32_t i;
    for (i = 0U; i < NET_SOCKET_CAPACITY; i++) if (!sockets[i].used) {
        sockets[i].used = 1U; sockets[i].receive_length = 0U; sockets[i].receive_offset = 0U;
        if (net_tcp_connection_listen(&sockets[i].connection, local_port, local_sequence) != 0) {
            sockets[i].used = 0U; return NET_SOCKET_BAD_ARGUMENT;
        }
        return (int)i;
    }
    return NET_SOCKET_NO_SLOT;
}

int net_socket_accept_syn(int socket_id, const net_tcp_view_t* view) {
    if (!valid_id(socket_id) || !view) return NET_SOCKET_BAD_ARGUMENT;
    return net_tcp_connection_accept_syn(&sockets[socket_id].connection, view) == 0 ? 0 : NET_SOCKET_PROTOCOL;
}

int net_socket_build_syn_ack(int socket_id, uint8_t* segment, uint16_t capacity, uint16_t* out_length) {
    int built;
    if (!valid_id(socket_id) || !segment || !out_length) return NET_SOCKET_BAD_ARGUMENT;
    built = net_tcp_connection_build_syn_ack(&sockets[socket_id].connection, segment, capacity);
    if (built < 0) return NET_SOCKET_PROTOCOL;
    *out_length = (uint16_t)built;
    return 0;
}

int net_socket_accept_ack(int socket_id, const net_tcp_view_t* view) {
    if (!valid_id(socket_id) || !view) return NET_SOCKET_BAD_ARGUMENT;
    return net_tcp_connection_accept_ack(&sockets[socket_id].connection, view) == 0 ? 0 : NET_SOCKET_PROTOCOL;
}
