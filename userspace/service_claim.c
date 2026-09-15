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

static int string_equal(const char* left, const char* right) {
    int i = 0;
    while (left[i] != '\0' && right[i] != '\0') {
        if (left[i] != right[i]) return 0;
        i++;
    }
    return left[i] == right[i];
}

static int service_register(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_REGISTER), "b"(name));
    return result;
}

static int service_notify(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_NOTIFY), "b"(name));
    return result;
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
    os_service_event_t event;
    int rc = service_register("demo");
    if (rc == 0) {
        puts("serviceclaim claimed demo\n");
        for (;;) yield();
    }
    if (rc != OS_SERVICE_TAKEN) {
        puts("serviceclaim register rc ");
        print_int(rc);
        puts("\n");
        for (;;) yield();
    }
    rc = service_notify("demo");
    if (rc != 0) {
        puts("serviceclaim watch rc ");
        print_int(rc);
        puts("\n");
        for (;;) yield();
    }
    puts("serviceclaim waiting demo\n");
    for (;;) {
        rc = ipc_receive(&message);
        if (rc == 0 && os_service_parse_event(&message, &event) == 0 &&
            string_equal(event.name, "demo")) {
            puts("serviceclaim notified demo\n");
            if (event.new_owner_pid > 0) {
                rc = service_register("demo");
                if (rc == 0) {
                    puts("serviceclaim claimed demo\n");
                    for (;;) yield();
                }
            }
        }
        yield();
    }
}
