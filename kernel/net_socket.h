#ifndef MOHHDY_NET_SOCKET_H
#define MOHHDY_NET_SOCKET_H

#include <stdint.h>
#include "net_tcp.h"
#include "net_tls_record.h"

#define NET_SOCKET_CAPACITY 4U
#define NET_SOCKET_RX_CAPACITY 1024U
#define NET_SOCKET_TX_CAPACITY 1500U

#define NET_SOCKET_CLOSED 0
#define NET_SOCKET_BAD_ARGUMENT -1
#define NET_SOCKET_NO_SLOT -2
#define NET_SOCKET_NOT_OPEN -3
#define NET_SOCKET_NOT_CONNECTED -4
#define NET_SOCKET_BUFFER_SMALL -5
#define NET_SOCKET_PROTOCOL -6

typedef struct {
    uint8_t used;
    net_tcp_connection_t connection;
    uint8_t receive_buffer[NET_SOCKET_RX_CAPACITY];
    uint16_t receive_length;
    uint16_t receive_offset;
} net_socket_slot_t;

int net_socket_open(uint16_t local_port, uint16_t remote_port, uint32_t local_sequence);
/* Construit le SYN actif du socket ouvert, sans modifier ses buffers statiques. */
int net_socket_build_syn(int socket_id, uint8_t* segment, uint16_t capacity,
                         uint16_t* out_length);
int net_socket_listen(uint16_t local_port, uint32_t local_sequence);
int net_socket_accept_syn(int socket_id, const net_tcp_view_t* view);
int net_socket_build_syn_ack(int socket_id, uint8_t* segment, uint16_t capacity, uint16_t* out_length);
int net_socket_accept_ack(int socket_id, const net_tcp_view_t* view);
int net_socket_close(int socket_id);
int net_socket_send(int socket_id, const uint8_t* payload, uint16_t length,
                    uint8_t* segment, uint16_t capacity, uint16_t* out_length);
/* Variante d’émission TCP avec budget de retransmission fourni par l’appelant. */
int net_socket_send_limit(int socket_id, const uint8_t* payload, uint16_t length,
                          uint8_t* segment, uint16_t capacity, uint16_t* out_length,
                          uint8_t retransmit_limit);
int net_socket_feed(int socket_id, const uint8_t* segment, uint16_t length);
int net_socket_receive(int socket_id, uint8_t* buffer, uint16_t capacity,
                       uint16_t* out_length);
int net_socket_send_tls(int socket_id, net_tls_aes_gcm_session_t* session, uint8_t content_type,
                        const uint8_t* plaintext, uint16_t plaintext_length,
                        uint8_t* record, uint32_t record_capacity, uint8_t* segment,
                        uint16_t segment_capacity, uint16_t* out_segment_length,
                        uint8_t retransmit_limit);
int net_socket_receive_tls(int socket_id, net_tls_aes_gcm_session_t* session,
                           const net_tcp_view_t* view, uint8_t* plaintext,
                           uint16_t plaintext_capacity, net_tls_record_view_t* out_record,
                           uint16_t* consumed);
int net_socket_accept_syn_ack(int socket_id, const net_tcp_view_t* view);
int net_socket_get_state(int socket_id, uint8_t* out_state);
int net_socket_connection_snapshot(int socket_id, net_tcp_connection_t* out_connection);
int net_socket_connection_restore(int socket_id, const net_tcp_connection_t* connection);
int net_socket_build_ack(int socket_id, uint8_t* segment, uint16_t capacity, uint16_t* out_length);
/* Construit FIN+ACK et publie FIN_WAIT_1 ; le pont doit restaurer le snapshot en cas d’échec TX. */
int net_socket_begin_close(int socket_id, uint8_t* segment, uint16_t capacity, uint16_t* out_length);
int net_socket_commit_send(int socket_id, uint16_t payload_length);
int net_socket_accept_tls_authenticated_fragment(int socket_id, const net_tcp_view_t* view,
                                                 net_tcp_tls_stream_t* stream, net_tls_handshake_t* handshake,
                                                 const uint8_t client_random[32], net_tls_transcript_t* transcript,
                                                 uint32_t* rsa_workspace, uint16_t rsa_workspace_length,
                                                 uint16_t* consumed);
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
                                       uint8_t retransmit_limit);
int net_socket_accept_tls_x25519_postflight(int socket_id, net_tls_handshake_t* handshake,
                                            net_tls_transcript_t* transcript,
                                            const uint8_t master_secret[48],
                                            net_tls_aes_gcm_session_t* session,
                                            const net_tcp_view_t* view, uint8_t* plaintext,
                                            uint16_t plaintext_capacity, uint8_t* prf_workspace,
                                            uint32_t prf_workspace_capacity, uint16_t* consumed);
void net_socket_reset_all(void);

#endif
