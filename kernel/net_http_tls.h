#ifndef MOHHDY_NET_HTTP_TLS_H
#define MOHHDY_NET_HTTP_TLS_H

#include <stdint.h>
#include "net_tcp.h"

typedef struct {
    uint16_t status_code;
    const uint8_t* body;
    uint16_t body_length;
    uint16_t header_length;
} net_http_response_view_t;
#define NET_LLM_HTTP_STATUS_SUCCESS 0
#define NET_LLM_HTTP_STATUS_RETRYABLE 1
#define NET_LLM_HTTP_STATUS_AUTH 2
#define NET_LLM_HTTP_STATUS_PERMANENT 3
#define NET_LLM_HTTP_STATUS_PROTOCOL 4
/* Retour de polling : le pair a envoyé une alerte TLS warning/close_notify valide. */
#define NET_HTTP_TLS_STATUS_CLOSE_NOTIFY 2
typedef struct {
    uint8_t* buffer;
    uint16_t capacity;
    uint16_t length;
    uint16_t header_length;
    uint16_t expected_body_length;
    uint16_t status_code;
    uint8_t headers_complete;
} net_http_response_accumulator_t;
typedef struct {
    uint8_t* buffer;
    uint16_t capacity;
    uint16_t length;
    uint16_t raw_length;
    uint16_t header_length;
    uint16_t status_code;
    uint32_t chunk_remaining;
    uint8_t line[10];
    uint8_t line_length;
    uint8_t state;
} net_http_chunked_accumulator_t;
#define NET_LLM_SSE_EVENT_ID_MAX 32U
#define NET_LLM_SSE_RETRY_MAX_MS 600000U
typedef struct { uint8_t* buffer; uint16_t capacity; uint16_t length; uint8_t done; uint8_t event_id[NET_LLM_SSE_EVENT_ID_MAX]; uint8_t event_id_length; uint8_t event_id_valid; uint32_t retry_delay_ms; uint8_t retry_valid; } net_llm_sse_accumulator_t;
typedef struct { net_http_chunked_accumulator_t http; net_llm_sse_accumulator_t sse; uint16_t decoded_consumed; } net_llm_sse_response_t;
typedef struct { uint8_t retries_used; uint8_t retry_limit; uint32_t next_tick; uint8_t scheduled; } net_llm_sse_reconnect_t;

/* Construit une requête HTTP/1.1 GET avec Host et Connection: close dans un buffer caller-owned. */
int net_http_build_get(uint8_t* request,uint16_t capacity,const char* host,const char* path);
/* Construit un GET SSE de reprise avec Last-Event-ID borné et caller-owned. */
int net_http_build_sse_resume_get(uint8_t* request,uint16_t capacity,const char* host,const char* path,const uint8_t* last_event_id,uint8_t last_event_id_length);
/* Extrait et décode une valeur JSON string associée à une clé ASCII, dans un buffer caller-owned. */
int net_json_extract_string(const uint8_t* json,uint16_t json_length,const char* key,uint8_t* output,uint16_t output_capacity,uint16_t* output_length);
/* Adapteurs non-streaming : champ response d’Ollama et champ content d’une réponse OpenAI compatible. */
int net_llm_ollama_response_extract(const uint8_t* json,uint16_t json_length,uint8_t* output,uint16_t output_capacity,uint16_t* output_length);
int net_llm_openai_response_extract(const uint8_t* json,uint16_t json_length,uint8_t* output,uint16_t output_capacity,uint16_t* output_length);
/* Construit les bodies JSON non-streaming pour Ollama generate et OpenAI chat dans un buffer caller-owned. */
int net_llm_build_ollama_generate_json(uint8_t* output,uint16_t output_capacity,const char* model,const uint8_t* prompt,uint16_t prompt_length);
int net_llm_build_openai_chat_json(uint8_t* output,uint16_t output_capacity,const char* model,const uint8_t* prompt,uint16_t prompt_length);
/* Variantes streaming : corps JSON avec `stream:true`, toujours construit dans le buffer caller-owned. */
int net_llm_build_ollama_generate_stream_json(uint8_t* output,uint16_t output_capacity,const char* model,const uint8_t* prompt,uint16_t prompt_length);
int net_llm_build_openai_chat_stream_json(uint8_t* output,uint16_t output_capacity,const char* model,const uint8_t* prompt,uint16_t prompt_length);
/* Accumule des événements SSE `data:` fragmentés et extrait les deltas Ollama/OpenAI. Retourne 1 si aucun événement complet n’est disponible, 0 après progression, négatif si framing ou capacité invalide. */
int net_llm_sse_accumulator_init(net_llm_sse_accumulator_t* accumulator,uint8_t* buffer,uint16_t capacity);
int net_llm_sse_accumulator_feed(net_llm_sse_accumulator_t* accumulator,uint8_t provider,const uint8_t* fragment,uint16_t fragment_length,uint8_t* text,uint16_t text_capacity,uint16_t* text_length);
/* Combine le décodage HTTP chunked et SSE. Les deux buffers restent caller-owned.
 * feed retourne 1 tant que le flux chunked n'est pas clos et que [DONE] n'est pas vu,
 * même si un delta a déjà été produit. Retourne 0 si sse.done ou HTTP état 8. */
int net_llm_sse_response_init(net_llm_sse_response_t* response,uint8_t* http_buffer,uint16_t http_capacity,uint8_t* sse_buffer,uint16_t sse_capacity);
int net_llm_sse_response_feed(net_llm_sse_response_t* response,uint8_t provider,const uint8_t* fragment,uint16_t fragment_length,uint8_t* text,uint16_t text_capacity,uint16_t* text_length);
/* Variante transactionnelle : restaure l’état SSE/HTTP et text_length sur erreur, sans copier les buffers caller-owned. */
int net_llm_sse_response_feed_transactional(net_llm_sse_response_t* response,uint8_t provider,const uint8_t* fragment,uint16_t fragment_length,uint8_t* text,uint16_t text_capacity,uint16_t* text_length);
/* Réinitialise les accumulateurs SSE en conservant les buffers caller-owned. */
int net_llm_sse_response_reset(net_llm_sse_response_t* response);
/* Planifie une reconnexion SSE retryable ; le scheduler ne bloque jamais et expose une deadline caller-owned. */
int net_llm_sse_reconnect_init(net_llm_sse_reconnect_t* reconnect,uint8_t retry_limit);
int net_llm_sse_reconnect_schedule(net_llm_sse_reconnect_t* reconnect,net_llm_sse_response_t* response,uint16_t status_code,uint32_t base_delay,uint32_t max_delay,uint32_t now);
int net_llm_sse_reconnect_ready(const net_llm_sse_reconnect_t* reconnect,uint32_t now);
/* Construit le GET de reprise depuis l’identifiant mémorisé par l’accumulateur. */
int net_llm_sse_build_resume_get(uint8_t* request,uint16_t capacity,const char* host,const char* path,const net_llm_sse_response_t* response);
/* Classe un statut HTTP LLM : succès, retryable, authentification, permanent ou protocole. */
int net_llm_http_status_classify(uint16_t status_code);
/* Consomme au plus retry_limit tentatives caller-owned ; retourne 1 si une nouvelle émission est autorisée. */
int net_llm_http_retry_consume(uint16_t status_code,uint8_t retry_limit,uint8_t* retries_used);
/* Planifie une nouvelle tentative retryable avec backoff borné ; aucune horloge implicite ni attente active. */
int net_llm_http_retry_schedule(uint16_t status_code,uint8_t retry_limit,uint32_t base_delay,uint32_t max_delay,uint32_t now,uint8_t* retries_used,uint32_t* retry_at);
#define NET_LLM_BEARER_MAX 128U
typedef struct { uint8_t token[NET_LLM_BEARER_MAX]; uint16_t length; uint8_t provisioned; } net_llm_bearer_store_t;
/* Copie un bearer ASCII imprimable dans un store fixe ; le secret ne peut pas être lu via l’API. */
int net_llm_bearer_store_provision(net_llm_bearer_store_t* store,const uint8_t* token,uint16_t token_length);
/* Efface le bearer et son état, sans libérer de mémoire. */
void net_llm_bearer_store_clear(net_llm_bearer_store_t* store);
/* Construit un POST Bearer sans exposer le token à l’appelant après provisionnement. */
int net_http_build_post_json_bearer_store(uint8_t* request,uint16_t capacity,const char* host,const char* path,const net_llm_bearer_store_t* store,const uint8_t* json,uint16_t json_length);

/* Chiffre une requête GET dans un record TLS applicatif et l’encapsule dans un segment TCP. */
int net_http_tls_build_get(net_tcp_connection_t* connection,net_tls_aes_gcm_session_t* session,
                           uint8_t* tcp_segment,uint32_t tcp_capacity,uint8_t* tls_record,uint32_t tls_capacity,
                           uint8_t* request,uint16_t request_capacity,const char* host,const char* path,
                           uint8_t retransmit_limit);

/* Construit un POST HTTP/1.1 JSON, avec Content-Type, Content-Length et Connection: close. */
int net_http_build_post_json(uint8_t* request,uint16_t capacity,const char* host,const char* path,
                             const uint8_t* json,uint16_t json_length);
/* Construit un POST JSON avec Authorization: Bearer, le token restant uniquement une entrée caller-owned. */
int net_http_build_post_json_bearer(uint8_t* request,uint16_t capacity,const char* host,const char* path,
                                    const char* bearer_token,const uint8_t* json,uint16_t json_length);
/* Chiffre un POST JSON dans un record TLS applicatif et l’encapsule dans un segment TCP. */
int net_http_tls_build_post_json(net_tcp_connection_t* connection,net_tls_aes_gcm_session_t* session,
                                 uint8_t* tcp_segment,uint32_t tcp_capacity,uint8_t* tls_record,uint32_t tls_capacity,
                                 uint8_t* request,uint16_t request_capacity,const char* host,const char* path,
                                 const uint8_t* json,uint16_t json_length,uint8_t retransmit_limit);
/* Variante HTTPS POST Bearer : le token reste dans le store fixe et n’est jamais passé par l’appelant réseau. */
int net_http_tls_build_post_json_bearer_store(net_tcp_connection_t* connection,net_tls_aes_gcm_session_t* session,
                                              uint8_t* tcp_segment,uint32_t tcp_capacity,uint8_t* tls_record,uint32_t tls_capacity,
                                              uint8_t* request,uint16_t request_capacity,const char* host,const char* path,
                                              const net_llm_bearer_store_t* store,const uint8_t* json,uint16_t json_length,
                                              uint8_t retransmit_limit);

/* Parse une réponse HTTP/1.1 complète déjà déchiffrée. Le body reste une vue sur plaintext. */
int net_http_response_parse(const uint8_t* plaintext,uint16_t plaintext_length,net_http_response_view_t* out);
/* Accumule une réponse HTTP/1.1 Content-Length à travers plusieurs plaintexts TLS.
 * Retourne 1 si le body est incomplet, 0 seulement lorsque la réponse complète est disponible. */
int net_http_response_accumulator_init(net_http_response_accumulator_t* accumulator,uint8_t* buffer,uint16_t capacity);
int net_http_response_accumulator_feed(net_http_response_accumulator_t* accumulator,const uint8_t* fragment,uint16_t fragment_length,net_http_response_view_t* out);
/* Décode Transfer-Encoding: chunked à travers plusieurs plaintexts TLS dans un buffer caller-owned. */
int net_http_chunked_accumulator_init(net_http_chunked_accumulator_t* accumulator,uint8_t* buffer,uint16_t capacity);
int net_http_chunked_accumulator_feed(net_http_chunked_accumulator_t* accumulator,const uint8_t* fragment,uint16_t fragment_length,net_http_response_view_t* out);

/* Ouvre transactionnellement un record TLS applicatif TCP et parse sa réponse HTTP/1.1. */
int net_http_tls_open_response(net_tcp_connection_t* connection,net_tls_aes_gcm_session_t* session,
                               const net_tcp_view_t* view,uint8_t* plaintext,uint16_t plaintext_capacity,
                               net_http_response_view_t* response,uint16_t* consumed);
/* Ouvre un record TLS applicatif et alimente un body HTTP Content-Length progressif. */
int net_http_tls_open_response_stream(net_tcp_connection_t* connection,net_tls_aes_gcm_session_t* session,
                                      const net_tcp_view_t* view,uint8_t* plaintext,uint16_t plaintext_capacity,
                                      net_http_response_accumulator_t* accumulator,net_http_response_view_t* response,
                                      uint16_t* consumed);
/* Ouvre un record applicatif TLS puis alimente une réponse HTTP chunked/SSE LLM. */
int net_http_tls_open_sse_stream(net_tcp_connection_t* connection,net_tls_aes_gcm_session_t* session,
                                 const net_tcp_view_t* view,uint8_t* plaintext,uint16_t plaintext_capacity,
                                 net_llm_sse_response_t* response,uint8_t provider,
                                 uint8_t* text,uint16_t text_capacity,uint16_t* text_length,uint16_t* consumed);

int net_llm_sse_reconnect_schedule_jittered(net_llm_sse_reconnect_t* reconnect,net_llm_sse_response_t* response,uint16_t status_code,uint32_t base_delay,uint32_t max_delay,uint32_t now,uint32_t jitter_window,uint32_t* jitter_seed);
#define NET_LLM_SSE_PERSIST_MAGIC 0x53534531U
#define NET_LLM_SSE_PERSIST_VERSION 1U
typedef struct { uint32_t magic; uint8_t version; uint8_t provider; uint8_t retries_used; uint8_t event_id_length; uint8_t event_id[NET_LLM_SSE_EVENT_ID_MAX]; uint32_t checksum; } net_llm_sse_persisted_state_t;
int net_llm_sse_persist_save(net_llm_sse_persisted_state_t* persisted,uint8_t provider,uint8_t retries_used,const net_llm_sse_response_t* response);
int net_llm_sse_persist_load(const net_llm_sse_persisted_state_t* persisted,uint8_t* provider,uint8_t* retries_used,net_llm_sse_response_t* response);



#define NET_LLM_MODEL_NAME_MAX 32U
typedef struct { uint8_t provider; const char* model; uint8_t model_length; uint8_t allow_rotation; } net_llm_model_policy_t;
int net_llm_model_policy_validate(const net_llm_model_policy_t* policy);
int net_https_application_ready(const net_tls_handshake_t* handshake,const net_tls_aes_gcm_session_t* session);
int net_https_build_application_record_if_ready(const net_tls_handshake_t* handshake,net_tls_aes_gcm_session_t* session,uint8_t* record,uint32_t capacity,uint8_t content_type,const uint8_t* plaintext,uint16_t plaintext_length);
int net_https_open_application_record_if_ready(const net_tls_handshake_t* handshake,net_tls_aes_gcm_session_t* session,const uint8_t* record,uint32_t length,uint8_t* plaintext,uint16_t plaintext_capacity,net_tls_record_view_t* out);
#endif
