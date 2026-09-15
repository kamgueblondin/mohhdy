#ifndef MOHHDY_NET_TCP_H
#define MOHHDY_NET_TCP_H

#include <stdint.h>
#include "net_tls_record.h"

#define NET_TCP_HEADER_SIZE 20U
#define NET_TCP_PROTOCOL 6U
#define NET_TCP_FLAG_FIN 0x01U
#define NET_TCP_FLAG_SYN 0x02U
#define NET_TCP_FLAG_RST 0x04U
#define NET_TCP_FLAG_ACK 0x10U

#define NET_TCP_STATE_CLOSED 0U
#define NET_TCP_STATE_SYN_SENT 1U
#define NET_TCP_STATE_ESTABLISHED 2U
#define NET_TCP_STATE_FIN_WAIT_1 3U
#define NET_TCP_STATE_FIN_WAIT_2 4U
#define NET_TCP_STATE_CLOSE_WAIT 5U
#define NET_TCP_STATE_LAST_ACK 6U
#define NET_TCP_STATE_LISTEN 7U
#define NET_TCP_STATE_SYN_RECEIVED 8U

typedef struct { uint16_t source_port; uint16_t destination_port; uint32_t sequence; uint32_t acknowledgment; uint8_t flags; const uint8_t* payload; uint16_t payload_length; } net_tcp_view_t;
typedef struct { net_tls_record_accumulator_t record_accumulator; net_tls_handshake_accumulator_t handshake_accumulator; } net_tcp_tls_stream_t;
typedef struct { uint8_t retry_limit; uint8_t retries_used; } net_tcp_connection_retry_t;
typedef struct { uint32_t deadline; uint32_t delay; uint8_t armed; } net_tcp_rto_timer_t;


typedef struct {
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t local_sequence;
    uint32_t remote_sequence;
    const uint8_t* pending_payload;
    uint16_t pending_length;
    uint8_t retransmit_count;
    uint8_t retransmit_limit;
    uint16_t receive_window;
    uint8_t state;
} net_tcp_connection_t;

int net_tcp_build_syn(uint8_t* segment, uint32_t capacity,
                      uint16_t source_port, uint16_t destination_port,
                      uint32_t sequence);
int net_tcp_build_syn_ack(uint8_t* segment, uint32_t capacity,
                          uint16_t source_port, uint16_t destination_port,
                          uint32_t sequence, uint32_t acknowledgment);
int net_tcp_build_ack(uint8_t* segment, uint32_t capacity,
                      uint16_t source_port, uint16_t destination_port,
                      uint32_t sequence, uint32_t acknowledgment);
int net_tcp_build_data(uint8_t* segment, uint32_t capacity,
                       uint16_t source_port, uint16_t destination_port,
                       uint32_t sequence, uint32_t acknowledgment,
                       const uint8_t* payload, uint16_t payload_length);
int net_tcp_parse(const uint8_t* segment, uint32_t length,
                  net_tcp_view_t* out);
/* Calcule le checksum TCP IPv4 sur le segment caller-owned. */
uint16_t net_tcp_checksum_ipv4(const uint8_t source_ip[4], const uint8_t destination_ip[4],
                               const uint8_t* segment, uint16_t length);
int net_tcp_build_syn_ipv4(uint8_t* packet, uint32_t capacity,
                           const uint8_t source_ip[4], const uint8_t destination_ip[4],
                           uint16_t source_port, uint16_t destination_port,
                           uint32_t sequence);
int net_tcp_build_syn_ack_ipv4(uint8_t* packet, uint32_t capacity,
                               const uint8_t source_ip[4], const uint8_t destination_ip[4],
                               uint16_t source_port, uint16_t destination_port,
                               uint32_t sequence, uint32_t acknowledgment);
int net_tcp_is_syn_ack_for(const net_tcp_view_t* view, uint16_t local_port,
                           uint16_t remote_port, uint32_t local_sequence,
                           uint32_t* remote_sequence);
int net_tcp_connection_open(net_tcp_connection_t* connection,
                            uint16_t local_port, uint16_t remote_port,
                            uint32_t local_sequence);
int net_tcp_connection_listen(net_tcp_connection_t* connection, uint16_t local_port,
                              uint32_t local_sequence);
int net_tcp_connection_accept_syn(net_tcp_connection_t* connection,
                                  const net_tcp_view_t* view);
int net_tcp_connection_build_syn_ack(const net_tcp_connection_t* connection,
                                     uint8_t* segment, uint32_t capacity);
/* Initialise un budget de reprises appartenant à l’appelant. */
int net_tcp_connection_retry_init(net_tcp_connection_retry_t* retry,uint8_t retry_limit);
/* Consomme une reprise ; retourne 1 si une nouvelle tentative est autorisée, 0 si le budget est épuisé. */
int net_tcp_connection_retry_consume(net_tcp_connection_retry_t* retry);
/* Réinitialise la connexion en SYN_SENT après consommation atomique d’un budget de reprise. */
int net_tcp_connection_retry_reopen(net_tcp_connection_t* connection,net_tcp_connection_retry_t* retry,uint32_t local_sequence);
int net_tcp_connection_accept_syn_ack(net_tcp_connection_t* connection,
                                      const net_tcp_view_t* view);
int net_tcp_connection_build_ack(const net_tcp_connection_t* connection,
                                 uint8_t* segment, uint32_t capacity);
int net_tcp_connection_build_data(net_tcp_connection_t* connection,
                                  uint8_t* segment, uint32_t capacity,
                                  const uint8_t* payload, uint16_t payload_length,
                                  uint8_t retransmit_limit);
int net_tcp_connection_build_tls_record(net_tcp_connection_t* connection,
                                        uint8_t* segment, uint32_t capacity,
                                        uint8_t* record, uint32_t record_capacity,
                                        uint8_t content_type, const uint8_t* payload,
                                        uint16_t payload_length, uint8_t retransmit_limit);
int net_tcp_connection_commit_send(net_tcp_connection_t* connection, uint16_t payload_length);
int net_tcp_connection_accept_data(net_tcp_connection_t* connection,
                                   const net_tcp_view_t* view,
                                   uint16_t* accepted_length);
int net_tcp_connection_accept_tls_record(net_tcp_connection_t* connection,
                                         const net_tcp_view_t* view,
                                         net_tls_record_view_t* record,
                                         uint16_t* consumed);
int net_tcp_tls_stream_init(net_tcp_tls_stream_t* stream,uint8_t* record_buffer,uint16_t record_capacity,uint8_t* handshake_buffer,uint16_t handshake_capacity);
/* Accepte un fragment TCP, réassemble record et handshake TLS, puis applique le dispatch RSA authentifié.
 * Retourne 1 tant que le message TLS reste incomplet, 0 après un message serveur authentifié. */
int net_tcp_connection_accept_tls_authenticated_fragment(net_tcp_connection_t* connection,const net_tcp_view_t* view,
                                                         net_tcp_tls_stream_t* stream,net_tls_handshake_t* handshake,
                                                         const uint8_t client_random[32],net_tls_transcript_t* transcript,
                                                         uint32_t* rsa_workspace,uint16_t rsa_workspace_length,uint16_t* consumed);
/* Traite un record TLS deja present dans l'accumulateur, sans nouvelle donnee TCP. */
int net_tcp_tls_stream_accept_pending(net_tcp_tls_stream_t* stream,net_tls_handshake_t* handshake,
                                      const uint8_t client_random[32],net_tls_transcript_t* transcript,
                                      uint32_t* rsa_workspace,uint16_t rsa_workspace_length);
int net_tcp_connection_accept_tls_handshake(net_tcp_connection_t* connection,
                                            const net_tcp_view_t* view,
                                            net_tls_handshake_t* handshake,
                                            net_tls_transcript_t* transcript,
                                            uint16_t* consumed);
int net_tcp_connection_build_tls_x25519_flight(net_tcp_connection_t* connection,
                                                 net_tls_handshake_t* handshake,net_tls_x25519_context_t* context,
                                                 const uint8_t client_private[NET_TLS_X25519_KEY_LENGTH],const uint8_t client_random[32],
                                                 net_tls_transcript_t* transcript,uint8_t master_secret[48],
                                                 uint8_t key_block[NET_TLS_AES_128_GCM_KEY_BLOCK_LENGTH],net_tls_aes_gcm_session_t* session,
                                                 uint8_t* segment,uint32_t segment_capacity,uint8_t* records,uint32_t records_capacity,
                                                 uint32_t* records_length,uint32_t* x25519_workspace,uint16_t x25519_workspace_length,
                                                 uint8_t* prf_workspace,uint32_t prf_workspace_capacity,uint8_t retransmit_limit);
int net_tcp_connection_accept_tls_postflight(net_tcp_connection_t* connection,
                                             const net_tcp_view_t* view,
                                             net_tls_handshake_t* handshake,
                                             net_tls_transcript_t* transcript,
                                             const uint8_t expected_verify_data[12],
                                             uint16_t* consumed);
int net_tcp_connection_accept_tls_x25519_postflight(net_tcp_connection_t* connection,
                                                      net_tls_handshake_t* handshake,net_tls_transcript_t* transcript,
                                                      const uint8_t master_secret[48],net_tls_aes_gcm_session_t* session,
                                                      const net_tcp_view_t* view,uint8_t* plaintext,uint16_t plaintext_capacity,
                                                      uint8_t* prf_workspace,uint32_t prf_workspace_capacity,uint16_t* consumed);
int net_tcp_connection_build_tls_aes_gcm(net_tcp_connection_t* connection,
                                         net_tls_aes_gcm_session_t* session,
                                         uint8_t* segment, uint32_t segment_capacity,
                                         uint8_t* record, uint32_t record_capacity,
                                         uint8_t content_type, const uint8_t* plaintext,
                                         uint16_t plaintext_length, uint8_t retransmit_limit);
int net_tcp_connection_accept_tls_aes_gcm(net_tcp_connection_t* connection,
                                          net_tls_aes_gcm_session_t* session,
                                          const net_tcp_view_t* view, uint8_t* plaintext,
                                          uint16_t plaintext_capacity, net_tls_record_view_t* out,
                                          uint16_t* consumed);
int net_tcp_connection_set_receive_window(net_tcp_connection_t* connection,
                                           uint16_t receive_window);
int net_tcp_connection_track_send(net_tcp_connection_t* connection,
                                  const uint8_t* payload, uint16_t payload_length,
                                  uint8_t retransmit_limit);
int net_tcp_connection_retransmit_allowed(const net_tcp_connection_t* connection);
int net_tcp_connection_note_retransmit(net_tcp_connection_t* connection);
/* RTO caller-owned : planifie une échéance bornée sans sleep ni timer global. */
int net_tcp_rto_init(net_tcp_rto_timer_t* timer,uint32_t initial_delay);
int net_tcp_rto_arm(net_tcp_rto_timer_t* timer,uint32_t now,uint32_t max_delay);
int net_tcp_rto_ready(const net_tcp_rto_timer_t* timer,uint32_t now);
int net_tcp_rto_consume(net_tcp_rto_timer_t* timer,net_tcp_connection_t* connection,uint32_t now,uint32_t max_delay);
int net_tcp_connection_accept_ack(net_tcp_connection_t* connection,
                                  const net_tcp_view_t* view);
int net_tcp_build_fin_ack(uint8_t* segment, uint32_t capacity,
                          uint16_t source_port, uint16_t destination_port,
                          uint32_t sequence, uint32_t acknowledgment);
int net_tcp_connection_begin_close(net_tcp_connection_t* connection,
                                   uint8_t* segment, uint32_t capacity);
int net_tcp_connection_accept_fin(net_tcp_connection_t* connection,
                                  const net_tcp_view_t* view);

#endif
