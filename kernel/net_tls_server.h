#ifndef MOHHDY_NET_TLS_SERVER_H
#define MOHHDY_NET_TLS_SERVER_H

#include <stdint.h>
#include "net_tls_record.h"
#include "x25519.h"

#define NET_TLS_HANDSHAKE_CLIENT_HELLO 1U
#define NET_TLS_SERVER_PHASE_IDLE 0U
#define NET_TLS_SERVER_PHASE_HELLO_SENT 1U
#define NET_TLS_SERVER_PHASE_WAIT_CLIENT_FLIGHT 2U
#define NET_TLS_SERVER_PHASE_COMPLETE 3U

typedef struct {
    const uint8_t* random;
    uint16_t cipher_suite;
} net_tls_client_hello_view_t;

typedef struct {
    uint8_t phase;
    uint8_t client_random[32];
    uint8_t server_random[32];
    uint8_t server_private[NET_TLS_X25519_KEY_LENGTH];
    uint8_t server_public[NET_TLS_X25519_KEY_LENGTH];
    uint8_t master_secret[48];
    uint8_t key_block[NET_TLS_AES_128_GCM_KEY_BLOCK_LENGTH];
    net_tls_aes_gcm_session_t session;
    net_tls_transcript_t transcript;
    uint8_t transcript_storage[2048];
    uint8_t complete;
} net_tls_server_t;

int net_tls_client_hello_parse(const uint8_t* handshake, uint16_t length,
                               net_tls_client_hello_view_t* out);

int net_tls_server_hello_build(uint8_t* record, uint32_t capacity,
                               const uint8_t server_random[32], uint16_t cipher_suite);

int net_tls_server_certificate_build(uint8_t* record, uint32_t capacity,
                                     const uint8_t* leaf_der, uint16_t leaf_length);

int net_tls_server_key_exchange_ecdhe_rsa_build(
    uint8_t* record, uint32_t capacity,
    const uint8_t client_random[32], const uint8_t server_random[32],
    const uint8_t server_public[NET_TLS_X25519_KEY_LENGTH],
    const uint8_t* modulus, uint16_t modulus_length,
    const uint8_t* private_exponent, uint16_t private_exponent_length,
    uint32_t* rsa_workspace, uint16_t rsa_workspace_length);

int net_tls_server_hello_done_build(uint8_t* record, uint32_t capacity);

int net_tls_server_init(net_tls_server_t* server, const uint8_t server_random[32],
                        const uint8_t server_private[NET_TLS_X25519_KEY_LENGTH],
                        uint32_t* x25519_workspace, uint16_t x25519_workspace_length);

/* Consomme un record ClientHello, met a jour le transcript. */
int net_tls_server_accept_client_hello(net_tls_server_t* server,
                                       const uint8_t* record, uint16_t record_length);

/* Construit le handshake plaintext (sans record) et l ajoute au transcript. */
int net_tls_server_note_handshake_message(net_tls_server_t* server,
                                          const uint8_t* handshake, uint16_t length);

int net_tls_server_accept_client_flight(
    net_tls_server_t* server, const uint8_t* flight, uint16_t flight_length,
    uint32_t* x25519_workspace, uint16_t x25519_workspace_length,
    uint8_t* prf_workspace, uint32_t prf_workspace_capacity,
    uint8_t* plaintext, uint16_t plaintext_capacity);

int net_tls_server_finished_record_build(net_tls_server_t* server, uint8_t* record,
                                         uint32_t capacity, uint8_t* prf_workspace,
                                         uint32_t prf_workspace_capacity);

#endif
