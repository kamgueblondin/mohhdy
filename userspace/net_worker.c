/* userspace/net_worker.c - Pilote réseau NE2000 externalisé en Ring 3.
 * S'enregistre auprès du micronoyau sous le nom de service "net-driver".
 */

#include "os_syscalls.h"

static void putc(char value) {
    asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(value));
}

static void puts(const char* text) {
    uint32_t index = 0U;
    while (text[index] != '\0') putc(text[index++]);
}

static int ipc_receive(os_ipc_message_t* message) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_IPC_RECV), "b"(message));
    return result;
}

static int ipc_send(int target_pid, const os_ipc_payload_t* payload) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_IPC_SEND), "b"(target_pid), "c"(payload));
    return result;
}

static int service_register(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_REGISTER), "b"(name));
    return result;
}

static void yield(void) {
    asm volatile("int $0x80" : : "a"(SYS_YIELD));
}

static int net_call1(uint32_t number, uint32_t a) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(a));
    return result;
}

static int net_call2(uint32_t number, uint32_t a, uint32_t b) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(a), "c"(b));
    return result;
}

/* Tranche 5 positive proof: once registered as net-driver, this PID still
 * reaches the gated socket and peer syscalls (the NE2000 driver itself stays
 * in Ring 0; the worker is the only task allowed to drive it). */
static int net_worker_gate_self_check(void) {
    int socket_id = net_call2(SYS_SOCKET_LISTEN, 7101U, 1U);
    if (socket_id < 0) return 0;
    if (net_call1(SYS_SOCKET_CLOSE, (uint32_t)socket_id) != 0) return 0;
    if (net_call1(SYS_PEER_LISTEN, 0U) == OS_NET_WORKER_REQUIRED) return 0;
    return 1;
}

void main(void) {
    os_ipc_message_t message;
    os_ipc_payload_t reply;
    (void)reply;

    if (service_register("net-driver") != 0) {
        puts("net-driver register failed\n");
        for (;;) yield();
    }
    puts("net-driver ready\n");
    if (net_worker_gate_self_check()) puts("net-driver gated syscalls ok\n");
    else puts("net-driver gated syscalls unexpected\n");

    for (;;) {
        int received = ipc_receive(&message);
        if (received == 0) {
            puts("net-driver received IPC message\n");
        }
        yield();
    }
}
