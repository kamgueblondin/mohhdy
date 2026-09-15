#include "os_syscalls.h"

static void putc(char c) {
    asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c));
}

static void puts(const char* text) {
    int i = 0;
    while (text[i] != '\0') putc(text[i++]);
}

static void print_int(int value) {
    char digits[12];
    int n = 0;
    unsigned int number;
    if (value < 0) {
        putc('-');
        number = (unsigned int)(-value);
    } else {
        number = (unsigned int)value;
    }
    if (number == 0U) {
        putc('0');
        return;
    }
    while (number > 0U && n < 11) {
        digits[n++] = (char)('0' + (number % 10U));
        number /= 10U;
    }
    while (n > 0) putc(digits[--n]);
}

static int ipc_receive(os_ipc_message_t* message) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_IPC_RECV), "b"(message));
    return result;
}

static void yield(void) {
    asm volatile("int $0x80" : : "a"(SYS_YIELD));
}

void main(void) {
    os_ipc_message_t message;
    puts("ipc_server ready\n");
    for (;;) {
        int rc = ipc_receive(&message);
        if (rc == 0) {
            uint32_t i;
            puts("ipc recv from ");
            print_int(message.sender_pid);
            puts(" type ");
            print_int((int)message.type);
            puts(" data ");
            for (i = 0U; i < message.size; i++) putc((char)message.data[i]);
            putc('\n');
        }
        yield();
    }
}
