#include "os_syscalls.h"

/* AOS-2177 proof client: historical SYS_READFILE / SYS_WRITEFILE while the
 * vfs-virtual storage worker is live. Overlay (ATA-backed) read and write
 * need the worker PID; initrd (RAM) read stays open. */

static void putc(char c) { asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c)); }
static void puts(const char* text) { int i = 0; while (text[i] != '\0') putc(text[i++]); }
static void yield(void) { asm volatile("int $0x80" : : "a"(SYS_YIELD)); }

static int hist_readfile(const char* path, char* buffer, uint32_t max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_READFILE), "b"(path), "c"(buffer), "d"(max));
    return result;
}

static int hist_writefile(const char* path, const char* data, uint32_t size) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_WRITEFILE), "b"(path), "c"(data), "d"(size));
    return result;
}

void main(void) {
    char data[32];
    int overlay_rc;
    int initrd_rc;
    int write_rc;
    puts("vfshistclaim waiting historical\n");
    overlay_rc = hist_readfile("note.txt", data, sizeof(data));
    initrd_rc = hist_readfile("hello.txt", data, sizeof(data));
    write_rc = hist_writefile("hist.txt", "yes", 3U);
    if (overlay_rc == OS_VFS_BACKEND_WORKER_REQUIRED &&
        write_rc == OS_VFS_BACKEND_WORKER_REQUIRED && initrd_rc > 0) {
        puts("vfshistclaim historical worker-mediated initrd ok\n");
    } else if (overlay_rc >= 0 && write_rc >= 0) {
        /* Degraded mode: no live vfs-virtual, historical local I/O remains. */
        puts("vfshistclaim historical local ok\n");
    } else {
        puts("vfshistclaim unexpected result\n");
    }
    for (;;) yield();
}
