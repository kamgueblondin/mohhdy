#ifndef MOHHDY_NET_LLM_CLIENT_H
#define MOHHDY_NET_LLM_CLIENT_H

#include <stdint.h>
#include "ne2k.h"
#include "net_socket.h"
#include "ecdsa_p256.h"
#include "rtc.h"
#include "../include/os_syscalls.h"

#define KERNEL_LLM_FRAME_CAPACITY NE2K_ETHERNET_MAX_FRAME
#define KERNEL_LLM_TLS_RECORD_CAPACITY 8192U
#define KERNEL_LLM_TLS_HELLO_CAPACITY 512U
/* Une même zone caller-owned sert à RSA ou ECDSA ; P-256 impose 2 048 mots. */
#define KERNEL_LLM_TLS_WORKSPACE_WORDS ECDSA_P256_WORKSPACE_WORDS

void net_llm_client_bind(ne2k_device_t* device, const ne2k_io_t* io, int present);
void net_llm_client_reset(void);
/* UTC for certificate validity: CMOS in Ring 0, SYS_NET_NIC UTC in Ring 3. */
int net_llm_client_utc(rtc_io_t* io, char* out, uint16_t capacity);

uint32_t kernel_llm_session_status(void);
int kernel_llm_acquire_start(const os_llm_acquire_start_request_t* request);
int kernel_llm_poll_tls(void);
int kernel_llm_request(const os_llm_request_t* request);
int kernel_llm_poll_text(os_llm_text_result_t* result);
int kernel_llm_poll_sse(os_llm_text_result_t* result);
int kernel_llm_reset_for_request(void);
int kernel_llm_close(void);
int kernel_llm_configure_openai(const os_llm_openai_credential_request_t* request);
int kernel_llm_dhcp_maintenance(uint32_t now);
int kernel_llm_rdrand_word(uint32_t* output);

/* Shared with the Ring 0 peer and wire paths in kernel/kernel.c. */
extern net_dhcp_lease_t boot_llm_lease;
extern net_arp_cache_t boot_llm_arp_cache;
extern uint8_t boot_llm_dhcp_tx[KERNEL_LLM_FRAME_CAPACITY];
extern uint8_t boot_llm_frame[KERNEL_LLM_FRAME_CAPACITY];
extern uint8_t boot_llm_rdrand_supported;
extern uint8_t boot_llm_test_trust_anchor_ready;
extern uint32_t boot_llm_rsa_workspace[KERNEL_LLM_TLS_WORKSPACE_WORDS];
extern uint32_t boot_llm_x25519_workspace[KERNEL_LLM_TLS_WORKSPACE_WORDS];
extern uint8_t boot_llm_prf_workspace[KERNEL_LLM_TLS_RECORD_CAPACITY];
extern uint8_t boot_llm_plaintext[KERNEL_LLM_TLS_RECORD_CAPACITY];

#endif
