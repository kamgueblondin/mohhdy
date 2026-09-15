#include "os_syscalls.h"

static void putc(char c) {
    asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c));
}

static void puts(const char* text) {
    int i = 0;
    while (text[i] != '\0') putc(text[i++]);
}

static void yield(void) {
    asm volatile("int $0x80" : : "a"(SYS_YIELD));
}

static int service_register(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_REGISTER), "b"(name));
    return result;
}

static int backend_read(const char* path, char* buffer, uint32_t max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_BACKEND_READ), "b"(path), "c"(buffer), "d"(max));
    return result;
}

void main(void) {
    char data[16];
    int announced = 0;
    for (;;) {
        int rc = service_register("vfs");
        if (rc == 0) {
            rc = backend_read("hello.txt", data, sizeof(data));
            if (rc >= 0) {
                puts("vfsclaim backend granted\n");
                for (;;) yield();
            }
        }
        if (!announced) {
            puts("vfsclaim waiting vfs\n");
            announced = 1;
        }
        yield();
    }
}
