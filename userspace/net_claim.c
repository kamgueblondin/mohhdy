#include "os_syscalls.h"

/* Tranche 5 proof client. With net-driver registered, socket / LLM-network /
 * peer syscalls from this (non-worker) task are refused with
 * OS_NET_WORKER_REQUIRED while SYS_NET_STATUS stays readable. Without the
 * worker (degraded mode) the historical local path still works. */

static void putc(char c) { asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c)); }
static void puts(const char* text) { int i = 0; while (text[i] != '\0') putc(text[i++]); }
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
    int status_rc;
    int socket_rc;
    int peer_rc;
    int llm_rc;
    puts("netclaim waiting net\n");
    status_rc = call0(SYS_NET_STATUS);
    socket_rc = call2(SYS_SOCKET_LISTEN, 7102U, 1U);
    peer_rc = call1(SYS_PEER_LISTEN, 0U);
    llm_rc = call0(SYS_LLM_POLL_TLS);
    if (status_rc == OS_NET_WORKER_REQUIRED) {
        puts("netclaim status unexpectedly gated\n");
    } else if (socket_rc == OS_NET_WORKER_REQUIRED && peer_rc == OS_NET_WORKER_REQUIRED &&
               llm_rc == OS_NET_WORKER_REQUIRED) {
        puts("netclaim worker-required enforced\n");
    } else if (socket_rc >= 0 && peer_rc != OS_NET_WORKER_REQUIRED &&
               llm_rc != OS_NET_WORKER_REQUIRED) {
        (void)call1(SYS_SOCKET_CLOSE, (uint32_t)socket_rc);
        puts("netclaim local ok\n");
    } else {
        puts("netclaim unexpected result\n");
    }
    for (;;) yield();
}
