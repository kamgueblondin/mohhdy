#ifndef MOHHDY_NET_STACK_EXEC_H
#define MOHHDY_NET_STACK_EXEC_H

/* Tranche 5 pile: executes one relayed network syscall against the stack
 * linked in the caller (socket registry, TCP, ARP/IPv4 framing, wire
 * engine, LLM TLS client). The Ring 3 networker links it with its own copy
 * of net_socket/net_tcp/net_wire/ne2k/net_llm_client and answers every
 * relayed call with it: nothing of the request is built or decoded in
 * Ring 0 any more while the worker owns the NE2000. Unit tested with a
 * scripted NIC (emit/poll) and fake LLM hooks. */

#include <stdint.h>
#include "net_wire.h"
#include "../include/os_syscalls.h"

typedef struct {
    int (*acquire_start)(const os_llm_acquire_start_request_t* request);
    int (*poll_tls)(void);
    int (*request)(const os_llm_request_t* request);
    int (*poll_text)(os_llm_text_result_t* result);
    int (*poll_sse)(os_llm_text_result_t* result);
    int (*reset_for_request)(void);
    int (*close)(void);
    int (*configure_openai)(const os_llm_openai_credential_request_t* request);
    uint32_t (*session_status)(void);
} net_stack_llm_ops_t;

typedef struct {
    /* NIC: emit == NULL means no card (wire ops OS_NET_WIRE_UNAVAILABLE). */
    net_wire_emit_fn emit;          /* frame to the driver, 0 = sent */
    void* emit_context;
    int (*poll)(void* user, uint8_t* frame, uint16_t capacity, uint16_t* length); /* 0 = frame */
    void (*idle)(void* user);        /* no frame this round (wait a tick) */
    void (*after_round)(void* user); /* ack ISR, collect IRQ */
    void* user;
    ne2k_device_t* device;           /* local MAC for framing */
    net_arp_cache_t* cache;
    uint8_t* tx;
    uint8_t* rx;
    uint16_t capacity;
    const net_stack_llm_ops_t* llm;  /* NULL: LLM ops answer OS_LLM_TLS_UNCONFIGURED */
    os_net_stack_report_t report;
} net_stack_t;

void net_stack_init(net_stack_t* stack);
/* Expected bulk bytes each way for a relayed op (0 = none). */
uint32_t net_stack_bulk_in_size(uint32_t op);
uint32_t net_stack_bulk_out_size(uint32_t op);
/* One relayed op. out/out_length: the IPC reply bytes (<= OS_NET_RELAY_MAX_OUT);
 * bulk_out/bulk_out_length: the bulk reply (<= OS_NET_RELAY_BULK_MAX). */
int32_t net_stack_exec(net_stack_t* stack, const os_net_relay_request_t* request,
                       const uint8_t* bulk_in, uint32_t bulk_in_length,
                       uint8_t* out, uint16_t* out_length,
                       uint8_t* bulk_out, uint32_t* bulk_out_length);

#endif
