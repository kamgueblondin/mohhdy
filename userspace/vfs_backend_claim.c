#include "os_syscalls.h"

static void putc(char c) { asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c)); }
static void puts(const char* text) { int i = 0; while (text[i] != '\0') putc(text[i++]); }
static void yield(void) { asm volatile("int $0x80" : : "a"(SYS_YIELD)); }

static int backend_read(const char* path, char* buffer, uint32_t max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_BACKEND_READ), "b"(path), "c"(buffer), "d"(max));
    return result;
}

void main(void) {
    char data[16];
    int announced_waiting = 0;
    int had_access = 0;
    for (;;) {
        int rc = backend_read("hello.txt", data, sizeof(data));
        if (rc >= 0) {
            if (!had_access) puts("vfscapclaim backend granted\n");
            had_access = 1;
        } else if (had_access) {
            puts("vfscapclaim backend revoked\n");
            for (;;) yield();
        } else if (!announced_waiting) {
            puts("vfscapclaim waiting backend\n");
            announced_waiting = 1;
        }
        yield();
    }
}
