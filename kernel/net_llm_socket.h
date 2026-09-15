#ifndef MOHHDY_NET_LLM_SOCKET_H
#define MOHHDY_NET_LLM_SOCKET_H

#include <stdint.h>
#include "net_socket.h"
#include "net_http_tls.h"

#define NET_LLM_SOCKET_PROVIDER_OLLAMA 0U
#define NET_LLM_SOCKET_PROVIDER_OPENAI 1U

/*
 * Construit un POST LLM JSON dans les buffers caller-owned puis l’encapsule
 * dans un record TLS AES-GCM et un segment TCP du socket établi.
 * Retourne la longueur du segment TCP, ou une valeur négative en cas d’erreur.
 */
int net_llm_socket_build_request(int socket_id, net_tls_aes_gcm_session_t* session,
                                 uint8_t provider, uint8_t stream,
                                 uint8_t* json, uint16_t json_capacity,
                                 uint8_t* request, uint16_t request_capacity,
                                 const char* host, const char* path,
                                 const char* bearer_token, const char* model,
                                 const uint8_t* prompt, uint16_t prompt_length,
                                 uint8_t* tls_record, uint32_t tls_capacity,
                                 uint8_t* tcp_segment, uint16_t tcp_capacity,
                                 uint8_t retransmit_limit);
/* Construit un GET SSE `Last-Event-ID` dans request puis l’envoie sur TLS.
 * Tous les buffers et l’identifiant restent caller-owned. */
int net_llm_socket_build_sse_resume(int socket_id, net_tls_aes_gcm_session_t* session,
                                    uint8_t* request, uint16_t request_capacity,
                                    const char* host, const char* path,
                                    const net_llm_sse_response_t* response,
                                    uint8_t* tls_record, uint32_t tls_capacity,
                                    uint8_t* tcp_segment, uint16_t tcp_capacity,
                                    uint8_t retransmit_limit);
/* Ouvre un record TLS socket et alimente l’accumulateur HTTP caller-owned. */
int net_llm_socket_open_response(int socket_id, net_tls_aes_gcm_session_t* session,
                                 const net_tcp_view_t* view, uint8_t* plaintext,
                                 uint16_t plaintext_capacity,
                                 net_http_response_accumulator_t* accumulator,
                                 net_http_response_view_t* response,
                                 uint16_t* consumed);
/* Ouvre un record TLS socket et publie un delta SSE LLM caller-owned. */
int net_llm_socket_open_sse(int socket_id, net_tls_aes_gcm_session_t* session,
                            const net_tcp_view_t* view, uint8_t* plaintext,
                            uint16_t plaintext_capacity, net_llm_sse_response_t* response,
                            uint8_t provider, uint8_t* text, uint16_t text_capacity,
                            uint16_t* text_length, uint16_t* consumed);

#endif
