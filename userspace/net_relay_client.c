#include "os_syscalls.h"

/* Tranche 5 slice 2 proof client (netrelay). Opens two TCP sockets of the
 * kernel socket registry and connects them to each other inside the guest
 * (active open -> passive listen, SYN / SYN-ACK / ACK, "ping" one way,
 * "pong" the other way, close both), using only the socket syscalls 99-108.
 * With net-driver live those 14 calls are relayed to the worker over IPC;
 * without it they run locally (degraded mode). The socket syscalls are a
 * TCP segment codec over the Ring 0 registry: no frame goes on the NIC. */

static void putc(char c) { asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c)); }
static void puts(const char* text) { int i = 0; while (text[i] != '\0') putc(text[i++]); }
static void yield(void) { asm volatile("int $0x80" : : "a"(SYS_YIELD)); }
static int call0(uint32_t n) {
    int r; asm volatile("int $0x80" : "=a"(r) : "a"(n)); return r;
}
static int call1(uint32_t n, uint32_t a) {
    int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a)); return r;
}
static int call2(uint32_t n, uint32_t a, uint32_t b) {
    int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b)); return r;
}
static int call3(uint32_t n, uint32_t a, uint32_t b, uint32_t c) {
    int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c)); return r;
}
static int call4(uint32_t n, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c), "S"(d)); return r;
}

static void put_uint(uint32_t value) {
    char digits[11];
    int n = 0;
    if (value == 0U) { putc('0'); return; }
    while (value > 0U && n < 10) { digits[n++] = (char)('0' + value % 10U); value /= 10U; }
    while (n > 0) putc(digits[--n]);
}

static void put_int(int value) {
    if (value < 0) { putc('-'); put_uint((uint32_t)(-value)); }
    else put_uint((uint32_t)value);
}

static uint32_t be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}
static uint16_t be16(const uint8_t* p) { return (uint16_t)(((uint16_t)p[0] << 8) | p[1]); }

static int fail(const char* step, int rc) {
    puts("netrelay tcp loopback failed at ");
    puts(step);
    puts(" rc ");
    put_int(rc);
    putc('\n');
    return -1;
}

static int send_feed_receive(int from, int to, const char* word, const char* tag) {
    static uint8_t segment[128];
    static uint8_t got[16];
    uint16_t seg_len = 0U, got_len = 0U;
    os_socket_send_request_t s;
    os_socket_feed_request_t f;
    os_socket_receive_request_t r;
    int rc, i;
    s.socket_id = from; s.payload = (const uint8_t*)word; s.length = 4U;
    s.segment = segment; s.capacity = sizeof(segment); s.out_length = &seg_len;
    rc = call1(SYS_SOCKET_SEND, (uint32_t)&s);
    if (rc != 0 || seg_len != 24U) return fail(tag, rc);
    f.socket_id = to; f.segment = segment; f.length = seg_len;
    rc = call1(SYS_SOCKET_FEED, (uint32_t)&f);
    if (rc != 0) return fail(tag, rc);
    r.socket_id = to; r.buffer = got; r.capacity = sizeof(got); r.out_length = &got_len;
    rc = call1(SYS_SOCKET_RECEIVE, (uint32_t)&r);
    if (rc != 0 || got_len != 4U) return fail(tag, rc);
    for (i = 0; i < 4; i++) if (got[i] != (uint8_t)word[i]) return fail(tag, -1000 - i);
    return 0;
}

static int loopback(void) {
    static uint8_t synack[64];
    uint16_t synack_len = 0U;
    os_socket_passive_view_t syn, ack;
    os_socket_syn_ack_t sa;
    int a, b, rc;
    a = call3(SYS_SOCKET_OPEN, 40001U, 40002U, 100U);
    if (a < 0) return fail("open", a);
    b = call2(SYS_SOCKET_LISTEN, 40002U, 1234U);
    if (b < 0) return fail("listen", b);
    syn.source_port = 40001U; syn.destination_port = 40002U;
    syn.sequence = 100U; syn.acknowledgment = 0U; syn.flags = 0x02U;
    rc = call2(SYS_SOCKET_ACCEPT_SYN, (uint32_t)b, (uint32_t)&syn);
    if (rc != 0) return fail("accept-syn", rc);
    rc = call4(SYS_SOCKET_BUILD_SYN_ACK, (uint32_t)b, (uint32_t)synack, sizeof(synack),
               (uint32_t)&synack_len);
    if (rc != 0 || synack_len < 20U || synack[13] != 0x12U) return fail("build-syn-ack", rc);
    sa.source_port = be16(synack); sa.destination_port = be16(synack + 2);
    sa.sequence = be32(synack + 4); sa.acknowledgment = be32(synack + 8);
    sa.flags = synack[13];
    rc = call2(SYS_SOCKET_ACCEPT_SYN_ACK, (uint32_t)a, (uint32_t)&sa);
    if (rc != 0) return fail("accept-syn-ack", rc);
    ack.source_port = 40001U; ack.destination_port = 40002U;
    ack.sequence = sa.acknowledgment; ack.acknowledgment = sa.sequence + 1U; ack.flags = 0x10U;
    rc = call2(SYS_SOCKET_ACCEPT_ACK, (uint32_t)b, (uint32_t)&ack);
    if (rc != 0) return fail("accept-ack", rc);
    if (send_feed_receive(a, b, "ping", "ping") != 0) return -1;
    if (send_feed_receive(b, a, "pong", "pong") != 0) return -1;
    rc = call1(SYS_SOCKET_CLOSE, (uint32_t)a);
    if (rc != 0) return fail("close-a", rc);
    rc = call1(SYS_SOCKET_CLOSE, (uint32_t)b);
    if (rc != 0) return fail("close-b", rc);
    return 0;
}

void main(void) {
    os_net_relay_status_t before, after;
    os_net_relay_reply_t forged;
    int peer_rc, llm_rc, forge_rc;
    uint32_t i;
    for (i = 0U; i < sizeof(before); i++) ((uint8_t*)&before)[i] = 0U;
    for (i = 0U; i < sizeof(forged); i++) ((uint8_t*)&forged)[i] = 0U;
    (void)call1(SYS_NET_RELAY_STATUS, (uint32_t)&before);
    if (loopback() == 0) {
        (void)call1(SYS_NET_RELAY_STATUS, (uint32_t)&after);
        puts("netrelay tcp loopback ok ping pong mode ");
        puts(before.worker_pid > 0 ? "relay" : "local");
        puts(" forwarded ");
        put_uint(after.forwarded - before.forwarded);
        puts(" completed ");
        put_uint(after.completed - before.completed);
        putc('\n');
    }
    /* Negatives kept: LLM / peer are not relayed, and only the worker may
     * post a relay reply. */
    peer_rc = call1(SYS_PEER_LISTEN, 0U);
    llm_rc = call0(SYS_LLM_POLL_TLS);
    forged.job_id = 1U;
    forge_rc = call1(SYS_NET_RELAY_REPLY, (uint32_t)&forged);
    if (before.worker_pid > 0) {
        if (peer_rc == OS_NET_WORKER_REQUIRED && llm_rc == OS_NET_WORKER_REQUIRED)
            puts("netrelay unsupported still worker-required\n");
        else
            puts("netrelay unsupported unexpected\n");
    }
    if (forge_rc == OS_NET_WORKER_REQUIRED) puts("netrelay forged reply refused\n");
    else puts("netrelay forged reply unexpected\n");
    for (;;) yield();
}
