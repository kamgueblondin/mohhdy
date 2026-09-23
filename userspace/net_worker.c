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

void main(void) {
    os_ipc_message_t message;
    os_ipc_payload_t reply;
    (void)reply;

    if (service_register("net-driver") != 0) {
        puts("net-driver register failed\n");
        for (;;) yield();
    }
    puts("net-driver ready\n");

    for (;;) {
        int received = ipc_receive(&message);
        if (received == 0) {
            puts("net-driver received IPC message\n");
        }
        yield();
    }
}
