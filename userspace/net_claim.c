#include "os_syscalls.h"

/* Tranche 5 proof client. With net-driver registered, LLM-network and peer
 * syscalls from this (non-worker) task are refused with
 * OS_NET_WORKER_REQUIRED while SYS_NET_STATUS stays readable; since slice 2
 * the socket syscalls are relayed to the worker over IPC instead (the relay
 * status must show the forward). Without the worker (degraded mode) the
 * historical local path still works. */

static void putc(char c) { asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c)); }
static void puts(const char* text) { int i = 0; while (text[i] != '\0') putc(text[i++]); }
static void put_int(int value) {
    char digits[11];
    int n = 0;
    uint32_t v;
    if (value < 0) { putc('-'); v = (uint32_t)(-value); } else v = (uint32_t)value;
    if (v == 0U) { putc('0'); return; }
    while (v > 0U && n < 10) { digits[n++] = (char)('0' + v % 10U); v /= 10U; }
    while (n > 0) putc(digits[--n]);
}
static void yield(void) { asm volatile("int $0x80" : : "a"(SYS_YIELD)); }

static int call0(uint32_t number) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number));
    return result;
}

static int call1(uint32_t number, uint32_t a) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(a));
    return result;
}

static int call2(uint32_t number, uint32_t a, uint32_t b) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(a), "c"(b));
    return result;
}

void main(void) {
    os_net_relay_status_t before, after;
    int status_rc;
    int socket_rc;
    int peer_rc;
    int llm_rc;
    uint32_t i;
    for (i = 0U; i < sizeof(before); i++) ((uint8_t*)&before)[i] = 0U;
    after = before;
    puts("netclaim waiting net\n");
    (void)call1(SYS_NET_RELAY_STATUS, (uint32_t)&before);
    status_rc = call0(SYS_NET_STATUS);
    socket_rc = call2(SYS_SOCKET_LISTEN, 7102U, 1U);
    (void)call1(SYS_NET_RELAY_STATUS, (uint32_t)&after);
    peer_rc = call1(SYS_PEER_LISTEN, 0U);
    llm_rc = call0(SYS_LLM_POLL_TLS);
    if (status_rc == OS_NET_WORKER_REQUIRED) {
        puts("netclaim status unexpectedly gated\n");
    } else if (before.worker_pid > 0) {
        if (peer_rc == OS_NET_WORKER_REQUIRED && llm_rc == OS_NET_WORKER_REQUIRED)
            puts("netclaim worker-required enforced\n");
        else
            puts("netclaim unexpected result\n");
        if (socket_rc >= 0 && after.forwarded == before.forwarded + 1U) {
            (void)call1(SYS_SOCKET_CLOSE, (uint32_t)socket_rc);
            puts("netclaim socket relayed\n");
        } else {
            puts("netclaim socket not relayed\n");
        }
        puts("netclaim socket rc ");
        put_int(socket_rc);
        putc('\n');
    } else if (socket_rc >= 0 && peer_rc != OS_NET_WORKER_REQUIRED &&
               llm_rc != OS_NET_WORKER_REQUIRED && after.forwarded == before.forwarded) {
        (void)call1(SYS_SOCKET_CLOSE, (uint32_t)socket_rc);
        puts("netclaim local ok\n");
    } else {
        puts("netclaim unexpected result\n");
    }
    for (;;) yield();
}
