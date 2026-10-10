/* Strict default image (NET_RING0_FALLBACK=0): the Ring 0 network stack
 * (ARP/IPv4/UDP/DHCP/DNS/TCP sockets, TLS client and server, X.509, the LLM
 * HTTP client, the wire engine and the NE2000 LLM glue) is NOT linked. The
 * Ring 3 networker carries its own copy of that code. This file provides
 * the few symbols the rest of the kernel still names: every Ring 0 network
 * operation answers "unavailable" and is logged once; the bookkeeping that
 * the strict kernel really uses (refusal counter, wire status, UTC for the
 * worker's TLS, the NE2000 I/O and IRQ hooks) is implemented here for real.
 * build/mohhdy-netlegacy.bin keeps the full Ring 0 stack. */
#include "net_llm_client.h"
#include "net_wire.h"
#include "net_stack_exec.h"
#include "net_socket.h"
#include "ne2k.h"
#include "rtc.h"
#include "../include/os_syscalls.h"

extern void print_string_serial(const char* s);
extern int kernel_net_nic_present(void);

net_arp_cache_t boot_llm_arp_cache;
uint8_t boot_llm_test_trust_anchor_ready;

static uint32_t g_absent_calls;
static os_net_wire_status_t g_wire;

static int absent(void) {
    if (g_absent_calls++ == 0U)
        print_string_serial("[NET] Ring 0 network stack not linked (strict image); call refused\n");
    return OS_NET_WIRE_UNAVAILABLE;
}

uint32_t net_ring0_absent_calls(void) { return g_absent_calls; }

/* NE2000 hooks: port I/O helpers and IRQ bookkeeping (no stack). */
static uint8_t absent_inb(void* c, uint16_t port) {
    uint8_t v;
    (void)c;
    __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
static void absent_outb(void* c, uint16_t port, uint8_t v) {
    (void)c;
    __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(port));
}
int ne2k_i386_io(ne2k_io_t* io) {
    if (!io) return -1;
    io->context = (void*)0;
    io->inb = absent_inb;
    io->outb = absent_outb;
    return 0;
}
int ne2k_irq_attach(ne2k_device_t* device, const ne2k_io_t* io) {
    return (device && io) ? 0 : -1;
}
void ne2k_irq_service(void) {}

/* LLM / peer client: not in this image. */
void net_llm_client_bind(ne2k_device_t* device, const ne2k_io_t* io, int present) {
    (void)device; (void)io; (void)present;
}
void net_llm_client_reset(void) {}
int net_llm_client_utc(rtc_io_t* io, char* out, uint16_t capacity) {
    if (rtc_i386_io(io) != 0) return -1;
    return rtc_read_utc(io, out, capacity);
}
/* Bit 31: this kernel has no Ring 0 network stack (strict image). */
uint32_t kernel_llm_session_status(void) {
    return 0x80000000U | (kernel_net_nic_present() ? 1U : 0U);
}
int kernel_llm_acquire_start(const os_llm_acquire_start_request_t* r) { (void)r; return absent(); }
int kernel_llm_poll_tls(void) { return absent(); }
int kernel_llm_request(const os_llm_request_t* r) { (void)r; return absent(); }
int kernel_llm_poll_text(os_llm_text_result_t* r) { (void)r; return absent(); }
int kernel_llm_poll_sse(os_llm_text_result_t* r) { (void)r; return absent(); }
int kernel_llm_reset_for_request(void) { return absent(); }
int kernel_llm_close(void) { return 0; }
int kernel_llm_configure_openai(const os_llm_openai_credential_request_t* r) { (void)r; return absent(); }
int kernel_llm_dhcp_maintenance(uint32_t now) { (void)now; return 0; }
int kernel_peer_listen(const os_peer_listen_request_t* r) { (void)r; return absent(); }
int kernel_peer_accept(const os_peer_accept_request_t* r) { (void)r; return absent(); }
int kernel_peer_tls_poll(const os_peer_tls_poll_request_t* r) { (void)r; return absent(); }
int kernel_peer_data(os_peer_data_request_t* r) { (void)r; return absent(); }

/* Kernel socket registry: not in this image. */
int net_socket_open(uint16_t a, uint16_t b, uint32_t c) { (void)a; (void)b; (void)c; return absent(); }
int net_socket_listen(uint16_t a, uint32_t b) { (void)a; (void)b; return absent(); }
int net_socket_accept_syn(int id, const net_tcp_view_t* v) { (void)id; (void)v; return absent(); }
int net_socket_build_syn_ack(int id, uint8_t* s, uint16_t c, uint16_t* o) {
    (void)id; (void)s; (void)c; (void)o; return absent();
}
int net_socket_accept_ack(int id, const net_tcp_view_t* v) { (void)id; (void)v; return absent(); }
int net_socket_accept_syn_ack(int id, const net_tcp_view_t* v) { (void)id; (void)v; return absent(); }
int net_socket_close(int id) { (void)id; return absent(); }
int net_socket_send(int id, const uint8_t* p, uint16_t l, uint8_t* s, uint16_t c, uint16_t* o) {
    (void)id; (void)p; (void)l; (void)s; (void)c; (void)o; return absent();
}
int net_socket_feed(int id, const uint8_t* s, uint16_t l) { (void)id; (void)s; (void)l; return absent(); }
int net_socket_receive(int id, uint8_t* b, uint16_t c, uint16_t* o) {
    (void)id; (void)b; (void)c; (void)o; return absent();
}

/* Relay bulk sizes (pure ABI tables, shared with the legacy stack). */
uint32_t net_stack_bulk_in_size(uint32_t op) {
    switch (op) {
        case SYS_LLM_ACQUIRE_START: return (uint32_t)sizeof(os_llm_acquire_start_request_t);
        case SYS_LLM_REQUEST: return (uint32_t)sizeof(os_llm_request_t);
        case SYS_LLM_OPENAI_CREDENTIAL: return (uint32_t)sizeof(os_llm_openai_credential_request_t);
        case SYS_PEER_DATA: return (uint32_t)sizeof(os_peer_data_request_t);
        default: return 0U;
    }
}
uint32_t net_stack_bulk_out_size(uint32_t op) {
    if (op == SYS_PEER_DATA) return (uint32_t)sizeof(os_peer_data_request_t);
    return (op == SYS_LLM_POLL_TEXT || op == SYS_LLM_POLL_SSE) ? (uint32_t)sizeof(os_llm_text_result_t) : 0U;
}

/* Wire engine: bookkeeping only (refusals are real), no Ring 0 operation. */
void net_wire_note_refused(void) { g_wire.refused++; }
void net_wire_fill_status(os_net_wire_status_t* out, int32_t worker_pid) {
    if (!out) return;
    *out = g_wire;
    out->bound = 0U;
    out->worker_pid = worker_pid;
}
int net_wire_unbind(int id) { (void)id; return 0; }
int net_wire_is_bound(int id) { (void)id; return 0; }
int net_wire_connect(const net_wire_ctx_t* c, const os_net_wire_connect_t* r) { (void)c; (void)r; return absent(); }
int net_wire_send(const net_wire_ctx_t* c, int id, const uint8_t* d, uint16_t l, uint8_t* s,
                  uint16_t sc, uint16_t* sl, uint16_t a) {
    (void)c; (void)id; (void)d; (void)l; (void)s; (void)sc; (void)sl; (void)a; return absent();
}
int net_wire_recv(const net_wire_ctx_t* c, int id, uint8_t* b, uint16_t cap, uint16_t* o, uint16_t a) {
    (void)c; (void)id; (void)b; (void)cap; (void)o; (void)a; return absent();
}
int net_wire_close(const net_wire_ctx_t* c, int id, uint16_t a) { (void)c; (void)id; (void)a; return absent(); }
int net_wire_op_connect(const net_wire_ctx_t* c, const os_net_wire_connect_t* r) { (void)c; (void)r; return absent(); }
int net_wire_op_send(const net_wire_ctx_t* c, int id, const uint8_t* d, uint16_t l, uint8_t* s,
                     uint16_t sc, uint16_t* sl, uint16_t a) {
    (void)c; (void)id; (void)d; (void)l; (void)s; (void)sc; (void)sl; (void)a; return absent();
}
int net_wire_op_recv(const net_wire_ctx_t* c, int id, uint8_t* b, uint16_t cap, uint16_t* o, uint16_t a) {
    (void)c; (void)id; (void)b; (void)cap; (void)o; (void)a; return absent();
}
int net_wire_op_close(const net_wire_ctx_t* c, int id, uint16_t a) { (void)c; (void)id; (void)a; return absent(); }
int net_wire_op_step(const net_wire_ctx_t* c, int f, uint16_t l) { (void)c; (void)f; (void)l; return 0; }
int net_wire_op_active(void) { return 0; }
int32_t net_wire_op_result(void) { return OS_NET_WIRE_UNAVAILABLE; }
void net_wire_op_cancel(void) {}
