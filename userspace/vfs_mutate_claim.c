#include "os_syscalls.h"

static void putc(char c) { asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c)); }
static void puts(const char* text) { int i = 0; while (text[i] != '\0') putc(text[i++]); }
static void yield(void) { asm volatile("int $0x80" : : "a"(SYS_YIELD)); }

static int backend_read(const char* path, char* buffer, uint32_t max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_BACKEND_READ), "b"(path), "c"(buffer), "d"(max));
    return result;
}

static int backend_write(const char* path, const char* data, uint32_t size) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_BACKEND_WRITE), "b"(path), "c"(data), "d"(size));
    return result;
}

static int backend_unlink(const char* path) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_OVERLAY_UNLINK), "b"(path));
    return result;
}

void main(void) {
    char data[16];
    int announced = 0;
    for (;;) {
        int read_rc = backend_read("hello.txt", data, sizeof(data));
        int write_rc = backend_write("mutate.txt", "yes", 3U);
        if (read_rc == OS_VFS_BACKEND_DENIED && write_rc >= 0) {
            if (backend_unlink("mutate.txt") >= 0) {
                puts("vfsmutateclaim mutate-only enforced\n");
                for (;;) yield();
            }
            puts("vfsmutateclaim cleanup unexpectedly denied\n");
            for (;;) yield();
        }
        if (read_rc >= 0) {
            puts("vfsmutateclaim read unexpectedly allowed\n");
            for (;;) yield();
        }
        if (write_rc != OS_VFS_BACKEND_DENIED) {
            puts("vfsmutateclaim mutation unexpectedly denied\n");
            for (;;) yield();
        }
        if (!announced) { puts("vfsmutateclaim waiting mutate\n"); announced = 1; }
        yield();
    }
}
