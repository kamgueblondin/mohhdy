#ifndef MOHHDY_NET_TLS_RECORD_H
#define MOHHDY_NET_TLS_RECORD_H
#include <stdint.h>
#include "x509_der.h"
#define NET_TLS_RECORD_HEADER 5U
#define NET_TLS_CONTENT_HANDSHAKE 22U
#define NET_TLS_CONTENT_APPLICATION_DATA 23U
#define NET_TLS_VERSION_1_2_MAJOR 3U
#define NET_TLS_VERSION_1_2_MINOR 3U
#define NET_TLS_HANDSHAKE_HEADER 4U
#define NET_TLS_HANDSHAKE_SERVER_HELLO 2U
#define NET_TLS_HANDSHAKE_CERTIFICATE 11U
#define NET_TLS_HANDSHAKE_SERVER_KEY_EXCHANGE 12U
#define NET_TLS_HANDSHAKE_CERTIFICATE_REQUEST 13U
#define NET_TLS_HANDSHAKE_SERVER_HELLO_DONE 14U
#define NET_TLS_HANDSHAKE_CLIENT_KEY_EXCHANGE 16U
#define NET_TLS_HANDSHAKE_FINISHED 20U
#define NET_TLS_CONTENT_CHANGE_CIPHER_SPEC 20U
#define NET_TLS_CONTENT_ALERT 21U
#define NET_TLS_AES_128_GCM_KEY_BLOCK_LENGTH 40U
#define NET_TLS_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256 0xc02fU
#define NET_TLS_CIPHER_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256 0xc02bU
#define NET_TLS_HASH_SHA256 4U
#define NET_TLS_SIGNATURE_RSA 1U
#define NET_TLS_SIGNATURE_ECDSA 3U
#define NET_TLS_NAMED_CURVE_X25519 29U
#define NET_TLS_X25519_KEY_LENGTH 32U
#define NET_TLS_X25519_CLIENT_FLIGHT_MINIMUM 93U
typedef struct { uint8_t content_type; uint8_t major; uint8_t minor; const uint8_t* payload; uint16_t payload_length; } net_tls_record_view_t;
typedef struct { uint8_t* buffer; uint16_t capacity; uint16_t length; } net_tls_record_accumulator_t;
typedef struct { uint8_t type; const uint8_t* body; uint32_t body_length; } net_tls_handshake_view_t;
typedef struct { uint8_t* buffer; uint16_t capacity; uint16_t length; } net_tls_handshake_accumulator_t;
typedef struct { uint8_t* buffer; uint16_t capacity; uint16_t length; } net_tls_transcript_t;
typedef struct { const uint8_t* client_write_key; const uint8_t* server_write_key; const uint8_t* client_fixed_iv; const uint8_t* server_fixed_iv; } net_tls_aes128_gcm_key_block_t;
typedef struct { const uint8_t* write_key; const uint8_t* read_key; const uint8_t* write_fixed_iv; const uint8_t* read_fixed_iv; uint64_t write_sequence; uint64_t read_sequence; } net_tls_aes_gcm_session_t;
typedef struct { const uint8_t* random; const uint8_t* session_id; uint8_t session_id_length; uint16_t cipher_suite; uint8_t compression_method; const uint8_t* extensions; uint16_t extensions_length; } net_tls_server_hello_view_t;
typedef struct { const uint8_t* certificate; uint32_t certificate_length; const uint8_t* intermediate; uint32_t intermediate_length; const uint8_t* intermediate_two; uint32_t intermediate_two_length; uint32_t certificate_list_length; } net_tls_certificate_view_t;
/* Sélection statique de la chaîne serveur validée cryptographiquement. */
typedef struct { const x509_certificate_view_t* intermediate_one; const x509_certificate_view_t* intermediate_two; uint8_t depth; } net_tls_certificate_chain_selection_t;
typedef struct { uint16_t named_curve; const uint8_t* public_key; uint8_t public_key_length; uint8_t hash_algorithm; uint8_t signature_algorithm; const uint8_t* signature; uint16_t signature_length; } net_tls_server_key_exchange_view_t;
typedef struct { const uint8_t* certificate_types; uint8_t certificate_types_length; const uint8_t* signature_algorithms; uint16_t signature_algorithms_length; const uint8_t* certificate_authorities; uint16_t certificate_authorities_length; } net_tls_certificate_request_view_t;
typedef enum { NET_TLS_HANDSHAKE_IDLE=0, NET_TLS_HANDSHAKE_CLIENT_HELLO_SENT=1, NET_TLS_HANDSHAKE_SERVER_HELLO_RECEIVED=2, NET_TLS_HANDSHAKE_CERTIFICATE_RECEIVED=3, NET_TLS_HANDSHAKE_SERVER_KEY_EXCHANGE_RECEIVED=4, NET_TLS_HANDSHAKE_CERTIFICATE_REQUEST_RECEIVED=5, NET_TLS_HANDSHAKE_SERVER_HELLO_DONE_RECEIVED=6, NET_TLS_HANDSHAKE_CLIENT_CERTIFICATE_SENT=7, NET_TLS_HANDSHAKE_CLIENT_KEY_EXCHANGE_SENT=8, NET_TLS_HANDSHAKE_CHANGE_CIPHER_SPEC_SENT=9, NET_TLS_HANDSHAKE_FINISHED_SENT=10, NET_TLS_HANDSHAKE_SERVER_CHANGE_CIPHER_SPEC_RECEIVED=11, NET_TLS_HANDSHAKE_SERVER_FINISHED_RECEIVED=12 } net_tls_handshake_state_t;
#define NET_TLS_SERVER_PUBLIC_KEY_MAX 65U
typedef struct { net_tls_handshake_state_t state; uint16_t cipher_suite; const uint8_t* server_random; uint8_t server_random_bytes[32]; const uint8_t* server_certificate; uint32_t server_certificate_length; const uint8_t* server_intermediate; uint32_t server_intermediate_length; const uint8_t* server_intermediate_two; uint32_t server_intermediate_two_length; x509_certificate_view_t server_x509; x509_certificate_view_t server_intermediate_x509; x509_certificate_view_t server_intermediate_two_x509; uint8_t server_x509_valid; uint8_t server_intermediate_x509_valid; uint8_t server_intermediate_two_x509_valid; uint16_t server_named_curve; const uint8_t* server_public_key; uint8_t server_public_key_bytes[NET_TLS_SERVER_PUBLIC_KEY_MAX]; uint8_t server_public_key_length; uint8_t certificate_requested; } net_tls_handshake_t;
typedef struct { uint8_t client_public[NET_TLS_X25519_KEY_LENGTH]; uint8_t shared_secret[NET_TLS_X25519_KEY_LENGTH]; uint8_t ready; } net_tls_x25519_context_t;
int net_tls_record_build(uint8_t* record, uint32_t capacity, uint8_t content_type, const uint8_t* payload, uint16_t payload_length);
int net_tls_record_parse(const uint8_t* record,uint32_t length,net_tls_record_view_t* out);
int net_tls_aes_gcm_record_build(uint8_t* record, uint32_t capacity, uint8_t content_type,
                                 uint64_t sequence_number, const uint8_t key[16], const uint8_t fixed_iv[4],
                                 const uint8_t* plaintext, uint16_t plaintext_length);
int net_tls_aes_gcm_record_open(const uint8_t* record, uint32_t length, uint64_t sequence_number,
                                const uint8_t key[16], const uint8_t fixed_iv[4],
                                uint8_t* plaintext, uint16_t plaintext_capacity,
                                net_tls_record_view_t* out);
int net_tls_aes_gcm_session_init(net_tls_aes_gcm_session_t* session,
                                 const net_tls_aes128_gcm_key_block_t* key_block, uint8_t is_client);
int net_tls_aes_gcm_session_build(net_tls_aes_gcm_session_t* session, uint8_t* record,
                                  uint32_t capacity, uint8_t content_type,
                                  const uint8_t* plaintext, uint16_t plaintext_length);
int net_tls_aes_gcm_session_open(net_tls_aes_gcm_session_t* session, const uint8_t* record,
                                 uint32_t length, uint8_t* plaintext, uint16_t plaintext_capacity,
                                 net_tls_record_view_t* out);
/* Construit un record TLS 1.2 chiffré `warning/close_notify` et avance la séquence seulement si le record est prêt. */
int net_tls_close_notify_build(net_tls_aes_gcm_session_t* session,uint8_t* record,uint32_t capacity);
/* Valide l’alerte TLS `warning/close_notify` déjà déchiffrée, sans mutation de session. */
int net_tls_close_notify_parse(const net_tls_record_view_t* record);
/* Parse le premier record d’un flux TCP et publie les octets consommés. */
int net_tls_record_parse_stream(const uint8_t* stream, uint32_t length,
                                net_tls_record_view_t* out, uint16_t* consumed);
int net_tls_record_accumulator_init(net_tls_record_accumulator_t* accumulator,
                                    uint8_t* buffer, uint16_t capacity);
/* Retourne 1 si le record est incomplet, 0 lorsqu’il est complet.
 * Les octets suivant un record complet restent dans l'accumulateur. */
int net_tls_record_accumulator_feed(net_tls_record_accumulator_t* accumulator,
                                    const uint8_t* fragment, uint16_t fragment_length,
                                    net_tls_record_view_t* out);
/* Retire un record deja copie hors de l'accumulateur ; conserve le reste. */
int net_tls_record_accumulator_consume(net_tls_record_accumulator_t* accumulator,uint16_t consumed);
int net_tls_handshake_parse(const uint8_t* handshake, uint16_t length,
                            net_tls_handshake_view_t* out);
int net_tls_handshake_accumulator_init(net_tls_handshake_accumulator_t* accumulator,
                                       uint8_t* buffer, uint16_t capacity);
int net_tls_handshake_accumulator_feed(net_tls_handshake_accumulator_t* accumulator,
                                       const uint8_t* fragment, uint16_t fragment_length,
                                       net_tls_handshake_view_t* out);
int net_tls_transcript_init(net_tls_transcript_t* transcript, uint8_t* buffer, uint16_t capacity);
int net_tls_transcript_append(net_tls_transcript_t* transcript, const uint8_t* handshake, uint16_t length);
int net_tls_transcript_sha256(const net_tls_transcript_t* transcript, uint8_t digest[32]);
int net_tls_prf_sha256(uint8_t* output, uint16_t output_length,
                       const uint8_t* secret, uint16_t secret_length,
                       const uint8_t* label, uint16_t label_length,
                       const uint8_t* seed_a, uint16_t seed_a_length,
                       const uint8_t* seed_b, uint16_t seed_b_length,
                       uint8_t* workspace, uint32_t workspace_capacity);
int net_tls_derive_master_secret(uint8_t master_secret[48],
                                 const uint8_t* premaster_secret, uint16_t premaster_length,
                                 const uint8_t client_random[32], const uint8_t server_random[32],
                                 uint8_t* workspace, uint32_t workspace_capacity);
int net_tls_finished_verify_data(uint8_t verify_data[12], const uint8_t master_secret[48],
                                 const net_tls_transcript_t* transcript, uint8_t transcript_hash[32],
                                 uint8_t* workspace, uint32_t workspace_capacity);
int net_tls_server_finished_verify_data(uint8_t verify_data[12], const uint8_t master_secret[48],
                                        const net_tls_transcript_t* transcript, uint8_t transcript_hash[32],
                                        uint8_t* workspace, uint32_t workspace_capacity);
int net_tls_derive_aes128_gcm_key_block(uint8_t* key_block, uint32_t key_block_capacity,
                                        const uint8_t master_secret[48],
                                        const uint8_t client_random[32], const uint8_t server_random[32],
                                        net_tls_aes128_gcm_key_block_t* out,
                                        uint8_t* workspace, uint32_t workspace_capacity);
int net_tls_server_hello_parse(const uint8_t* handshake, uint16_t length,
                               net_tls_server_hello_view_t* out);
int net_tls_cipher_suite_is_ecdhe_rsa_aes128_gcm(uint16_t cipher_suite);
int net_tls_cipher_suite_is_ecdhe_ecdsa_aes128_gcm(uint16_t cipher_suite);
/* Vrai pour les deux suites AES-128-GCM authentifiées par ECDHE prises en charge. */
int net_tls_cipher_suite_is_ecdhe_aes128_gcm(uint16_t cipher_suite);
int net_tls_certificate_parse(const uint8_t* handshake, uint16_t length,
                              net_tls_certificate_view_t* out);
int net_tls_server_key_exchange_parse(const uint8_t* handshake, uint16_t length,
                                      net_tls_server_key_exchange_view_t* out);
/* Valide les formes de clés éphémères actuellement acceptées, sans calcul ECDHE. */
int net_tls_server_key_exchange_params_validate(const net_tls_server_key_exchange_view_t* view);
int net_tls_certificate_request_parse(const uint8_t* handshake, uint16_t length,
                                      net_tls_certificate_request_view_t* out);
int net_tls_server_hello_done_parse(const uint8_t* handshake, uint16_t length);
int net_tls_change_cipher_spec_parse(const uint8_t* payload, uint16_t length);
int net_tls_finished_parse(const uint8_t* handshake, uint16_t length,
                           const uint8_t expected_verify_data[12]);
int net_tls_handshake_init(net_tls_handshake_t* handshake);
int net_tls_handshake_note_client_hello(net_tls_handshake_t* handshake);
int net_tls_handshake_accept_server_hello(net_tls_handshake_t* handshake,
                                          const uint8_t* message, uint16_t length);
int net_tls_handshake_accept_certificate(net_tls_handshake_t* handshake,
                                         const uint8_t* message, uint16_t length);
int net_tls_handshake_parse_server_certificate_x509(net_tls_handshake_t* handshake);
/* Analyse seulement la structure ServerKeyExchange ; ne l’utilisez pas comme validation cryptographique. */
int net_tls_handshake_accept_server_key_exchange(net_tls_handshake_t* handshake,
                                               const uint8_t* message, uint16_t length);
/* Vérifie SHA-256(client_random || server_random || ServerECDHParams) avec la clé RSA du certificat, puis accepte le message. */
int net_tls_handshake_accept_server_key_exchange_rsa(net_tls_handshake_t* handshake,
                                                     const uint8_t client_random[32],
                                                     const uint8_t* message,uint16_t length,
                                                     uint32_t* rsa_workspace,uint16_t rsa_workspace_length);
/* Vérifie ECDSA/SHA-256 de ServerKeyExchange avec la clé P-256 du certificat serveur. */
int net_tls_handshake_accept_server_key_exchange_ecdsa(net_tls_handshake_t* handshake,
                                                       const uint8_t client_random[32],
                                                       const uint8_t* message,uint16_t length,
                                                       uint32_t* ecdsa_workspace,uint16_t ecdsa_workspace_length);
int net_tls_handshake_accept_certificate_request(net_tls_handshake_t* handshake,
                                                 const uint8_t* message, uint16_t length);
int net_tls_handshake_accept_server_hello_done(net_tls_handshake_t* handshake,
                                               const uint8_t* message, uint16_t length);
int net_tls_handshake_accept_server_message(net_tls_handshake_t* handshake,
                                            const uint8_t* message, uint16_t length,
                                            net_tls_transcript_t* transcript);
/* Dispatch authentifié : analyse X.509 et exige une signature RSA ou ECDSA cohérente avec la suite négociée. */
int net_tls_handshake_accept_server_message_authenticated(net_tls_handshake_t* handshake,
                                                          const uint8_t client_random[32],
                                                          const uint8_t* message,uint16_t length,
                                                          net_tls_transcript_t* transcript,
                                                          uint32_t* rsa_workspace,uint16_t rsa_workspace_length);
int net_tls_client_certificate_empty_build(uint8_t* handshake, uint32_t capacity);
int net_tls_client_key_exchange_build(uint8_t* handshake, uint32_t capacity,
                                      const uint8_t* public_key, uint8_t public_key_length);
int net_tls_change_cipher_spec_build(uint8_t* record, uint32_t capacity);
int net_tls_finished_build(uint8_t* record, uint32_t capacity, const uint8_t verify_data[12]);
/* Produit clé publique et secret X25519 après un ServerKeyExchange X25519 authentifié. */
int net_tls_x25519_prepare_client(net_tls_x25519_context_t* context,const net_tls_handshake_t* handshake,
                                  const uint8_t client_private[NET_TLS_X25519_KEY_LENGTH],
                                  uint32_t* x25519_workspace,uint16_t x25519_workspace_length);
int net_tls_derive_x25519_master_secret(uint8_t master_secret[48],const net_tls_x25519_context_t* context,
                                        const uint8_t client_random[32],const net_tls_handshake_t* handshake,
                                        uint8_t* prf_workspace,uint32_t prf_workspace_capacity);
/* Construit transactionnellement le flight client TLS 1.2 à partir de X25519.
 * `records` contient ClientKeyExchange, ChangeCipherSpec puis Finished chiffré;
 * la longueur est publiée uniquement en cas de succès. */
int net_tls_x25519_client_flight_build(net_tls_handshake_t* handshake,
                                       net_tls_x25519_context_t* context,
                                       const uint8_t client_private[NET_TLS_X25519_KEY_LENGTH],
                                       const uint8_t client_random[32],
                                       net_tls_transcript_t* transcript,
                                       uint8_t master_secret[48],uint8_t key_block[NET_TLS_AES_128_GCM_KEY_BLOCK_LENGTH],
                                       net_tls_aes_gcm_session_t* session,
                                       uint8_t* records,uint32_t records_capacity,uint32_t* records_length,
                                       uint32_t* x25519_workspace,uint16_t x25519_workspace_length,
                                       uint8_t* prf_workspace,uint32_t prf_workspace_capacity);
int net_tls_handshake_note_client_certificate(net_tls_handshake_t* handshake);
int net_tls_handshake_note_client_key_exchange(net_tls_handshake_t* handshake);
int net_tls_handshake_note_change_cipher_spec(net_tls_handshake_t* handshake);
int net_tls_handshake_note_finished(net_tls_handshake_t* handshake);
int net_tls_handshake_accept_server_change_cipher_spec(net_tls_handshake_t* handshake,
                                                        const uint8_t* payload, uint16_t length);
int net_tls_handshake_accept_server_finished(net_tls_handshake_t* handshake,
                                             const uint8_t* message, uint16_t length,
                                             const uint8_t expected_verify_data[12]);
int net_tls_handshake_is_complete(const net_tls_handshake_t* handshake);
/* Construit un ClientHello TLS 1.2 minimal dans un record caller-owned. */
int net_tls_client_hello_build(uint8_t* record, uint32_t capacity,
                               const uint8_t random[32]);
/* Ajoute optionnellement SNI et un protocole ALPN ASCII bornés ; les deux chaînes peuvent être nulles. */
int net_tls_client_hello_sni_alpn_build(uint8_t* record,uint32_t capacity,const uint8_t random[32],const char* hostname,const char* alpn);
/* Choisit une chaîne directe, à un ou à deux intermédiaires ; aucune allocation ni mutation de handshake. */
int net_tls_handshake_chain_select(const net_tls_handshake_t* handshake,const x509_certificate_view_t* trust_anchor,uint32_t* workspace,uint16_t workspace_length,net_tls_certificate_chain_selection_t* out);
int net_tls_handshake_validate_server_identity(const net_tls_handshake_t* handshake,const x509_certificate_view_t* trust_anchor,const char* hostname,const char* utc_time,uint32_t* workspace,uint16_t workspace_length);
int net_tls_handshake_accept_server_postflight(net_tls_handshake_t* handshake,const uint8_t* change_cipher_spec,uint16_t change_cipher_spec_length,const uint8_t* finished,uint16_t finished_length,const uint8_t expected_verify_data[12]);
#endif
