/* Shared helpers of the SYS_IPC_RECV_WAIT proof programs (ipcwait, ipcpoke). */
#ifndef IPC_WAIT_COMMON_H
#define IPC_WAIT_COMMON_H

#include "os_syscalls.h"

static void iw_putc(char c) {
    asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c));
}

static void iw_puts(const char* text) {
    int i = 0;
    while (text[i] != '\0') iw_putc(text[i++]);
}

static void iw_print_int(int value) {
    char digits[12];
    int n = 0;
    unsigned int number;
    if (value < 0) {
        iw_putc('-');
        number = (unsigned int)(-value);
    } else {
        number = (unsigned int)value;
    }
    if (number == 0U) {
        iw_putc('0');
        return;
    }
    while (number > 0U && n < 11) {
        digits[n++] = (char)('0' + (number % 10U));
        number /= 10U;
    }
    while (n > 0) iw_putc(digits[--n]);
}

static int iw_recv_wait(os_ipc_message_t* message, unsigned int timeout) {
    int result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SYS_IPC_RECV_WAIT), "b"(message), "c"(timeout) : "memory");
    return result;
}

static int iw_send(int pid, const os_ipc_payload_t* payload) {
    int result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SYS_IPC_SEND), "b"(pid), "c"(payload) : "memory");
    return result;
}

static int iw_service_register(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_REGISTER), "b"(name));
    return result;
}

static int iw_service_lookup(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_LOOKUP), "b"(name));
    return result;
}

#endif
