#include "os_syscalls.h"

static void putc(char c) { asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c)); }
static void puts(const char* text) { int i = 0; while (text[i] != '\0') putc(text[i++]); }
static void yield(void) { asm volatile("int $0x80" : : "a"(SYS_YIELD)); }

static int backend_read(const char* path, char* buffer, uint32_t max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_BACKEND_READ), "b"(path), "c"(buffer), "d"(max));
    return result;
}

static int backend_release(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_BACKEND_RELEASE), "b"(name));
    return result;
}

void main(void) {
    char data[16];
    int announced_waiting = 0;
    for (;;) {
        if (backend_read("hello.txt", data, sizeof(data)) >= 0) {
            puts("vfsreleaseclaim backend granted\n");
            if (backend_release("vfs") == 0) puts("vfsreleaseclaim backend self-released\n");
            else puts("vfsreleaseclaim backend release failed\n");
            for (;;) yield();
        }
        if (!announced_waiting) {
            puts("vfsreleaseclaim waiting backend\n");
            announced_waiting = 1;
        }
        yield();
    }
}
