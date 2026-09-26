#include "os_syscalls.h"

/* Tranche 5 slice 3 proof client (netwire). A plain (non-worker) task opens
 * a real TCP connection over the NE2000 wire to an on-link peer
 * (10.32.0.2:7, an echo service run by the QEMU test harness), sends a
 * payload, reads the echo and closes, using only the public socket
 * syscalls SYS_SOCKET_CONNECT / SEND / RECEIVE / CLOSE. While net-driver is
 * live these are relayed to the worker, which drives the frames through the
 * worker-only SYS_NET_WIRE_* path. Direct SYS_NET_WIRE_* calls from this
 * task are refused (OS_NET_WORKER_REQUIRED). The NE2000 driver, IRQ handler
 * and TCP state machine stay in Ring 0. */

static void putc(char c) { asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c)); }
static void puts(const char* text) { int i = 0; while (text[i] != '\0') putc(text[i++]); }
static void yield(void) { asm volatile("int $0x80" : : "a"(SYS_YIELD)); }
static int call1(uint32_t n, uint32_t a) {
    int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a)); return r;
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

static void zero(void* p, uint32_t n) {
    uint32_t i;
    for (i = 0U; i < n; i++) ((uint8_t*)p)[i] = 0U;
}

static int fail(const char* step, int rc) {
    puts("netwire failed at ");
    puts(step);
    puts(" rc ");
    put_int(rc);
    putc('\n');
    return -1;
}

static const char k_message[] = "mohhdy-wire-echo";

static int echo_session(uint32_t* echoed) {
    static uint8_t segment[128];
    static uint8_t got[64];
    os_socket_connect_request_t c;
    os_socket_send_request_t s;
    os_socket_receive_request_t r;
    uint16_t seg_len = 0U, got_len = 0U, total = 0U;
    uint16_t length = (uint16_t)(sizeof(k_message) - 1U);
    int sid, rc, round, i;

    zero(&c, sizeof(c));
    c.local_port = 40007U;
    c.remote_port = 7U;
    c.local_ip[0] = 10U; c.local_ip[1] = 32U; c.local_ip[2] = 0U; c.local_ip[3] = 15U;
    c.remote_ip[0] = 10U; c.remote_ip[1] = 32U; c.remote_ip[2] = 0U; c.remote_ip[3] = 2U;
    c.local_sequence = 0x57A10000U;
    c.attempts = 300U;
    sid = call1(SYS_SOCKET_CONNECT, (uint32_t)&c);
    if (sid == OS_NET_WORKER_REQUIRED) {
        puts("netwire connect worker-required\n");
        return 1;
    }
    if (sid < 0) return fail("connect", sid);
    puts("netwire connect ok socket ");
    put_int(sid);
    putc('\n');

    s.socket_id = sid; s.payload = (const uint8_t*)k_message; s.length = length;
    s.segment = segment; s.capacity = sizeof(segment); s.out_length = &seg_len;
    rc = call1(SYS_SOCKET_SEND, (uint32_t)&s);
    if (rc != 0) return fail("send", rc);
    puts("netwire send ok segment ");
    put_uint(seg_len);
    putc('\n');

    for (round = 0; round < 6 && total < length; round++) {
        got_len = 0U;
        r.socket_id = sid; r.buffer = got + total; r.capacity = (uint16_t)(sizeof(got) - total);
        r.out_length = &got_len;
        rc = call1(SYS_SOCKET_RECEIVE, (uint32_t)&r);
        if (rc != 0 && rc != OS_NET_WIRE_TIMEOUT) return fail("receive", rc);
        total = (uint16_t)(total + got_len);
    }
    if (total != length) return fail("echo-length", (int)total);
    for (i = 0; i < (int)length; i++)
        if (got[i] != (uint8_t)k_message[i]) return fail("echo-bytes", i);
    rc = call1(SYS_SOCKET_CLOSE, (uint32_t)sid);
    if (rc != 0) return fail("close", rc);
    *echoed = total;
    return 0;
}

void main(void) {
    static os_net_wire_status_t w0, w1;
    static os_net_relay_status_t r0, r1;
    static os_net_wire_connect_t raw_connect;
    static os_net_wire_io_t raw_io;
    static uint8_t raw_buf[8];
    static uint16_t raw_len;
    uint32_t echoed = 0U;
    int refused = 0;
    int rc;

    (void)call1(SYS_NET_WIRE_STATUS, (uint32_t)&w0);
    (void)call1(SYS_NET_RELAY_STATUS, (uint32_t)&r0);

    /* Negative proof: raw wire access from a non-worker task. */
    zero(&raw_connect, sizeof(raw_connect));
    raw_connect.local_port = 40008U; raw_connect.remote_port = 7U;
    raw_connect.local_ip[0] = 10U; raw_connect.local_ip[1] = 32U; raw_connect.local_ip[3] = 15U;
    raw_connect.remote_ip[0] = 10U; raw_connect.remote_ip[1] = 32U; raw_connect.remote_ip[3] = 2U;
    zero(&raw_io, sizeof(raw_io));
    raw_io.socket_id = 0; raw_io.data = raw_buf; raw_io.length = 1U;
    raw_io.rx = raw_buf; raw_io.rx_capacity = sizeof(raw_buf); raw_io.rx_length = &raw_len;
    if (call1(SYS_NET_WIRE_CONNECT, (uint32_t)&raw_connect) == OS_NET_WORKER_REQUIRED) refused++;
    if (call1(SYS_NET_WIRE_SEND, (uint32_t)&raw_io) == OS_NET_WORKER_REQUIRED) refused++;
    if (call1(SYS_NET_WIRE_RECV, (uint32_t)&raw_io) == OS_NET_WORKER_REQUIRED) refused++;
    if (call1(SYS_NET_WIRE_CLOSE, 0U) == OS_NET_WORKER_REQUIRED) refused++;
    puts("netwire raw wire refused ");
    put_int(refused);
    puts(" of 4\n");

    rc = echo_session(&echoed);
    if (rc == 0) {
        (void)call1(SYS_NET_WIRE_STATUS, (uint32_t)&w1);
        (void)call1(SYS_NET_RELAY_STATUS, (uint32_t)&r1);
        puts("netwire echo ok bytes ");
        put_uint(echoed);
        puts(" relayed ");
        put_uint(r1.completed - r0.completed);
        puts(" frames tx ");
        put_uint(w1.frames_tx - w0.frames_tx);
        puts(" rx ");
        put_uint(w1.frames_rx - w0.frames_rx);
        puts(" demuxed ");
        put_uint(w1.demuxed - w0.demuxed);
        puts(" refused ");
        put_uint(w1.refused - w0.refused);
        puts(" bound ");
        put_uint(w1.bound);
        putc('\n');
    }
    puts("netwire done\n");
    for (;;) yield();
}
