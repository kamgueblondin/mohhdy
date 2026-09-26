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

static int sc1(int nr, const char* a) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(nr), "b"(a));
    return result;
}

static int sc2(int nr, const char* a, const void* b) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(nr), "b"(a), "c"(b));
    return result;
}

static int sc3(int nr, const char* a, const void* b, uint32_t c) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(nr), "b"(a), "c"(b), "d"(c));
    return result;
}

static int same(const char* a, const char* b) {
    int i = 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}

/* AOS-2178: remaining historical overlay entry points while the worker is
 * live. Returns 1 when every overlay path is refused with WORKER_REQUIRED and
 * initrd stat/list stay open without leaking overlay entries. */
static int hist_overlay_entry_points_mediated(void) {
    os_dirent_t st;
    os_dirent_t list[16];
    int n;
    int i;
    if (sc2(SYS_STAT, "note.txt", &st) != OS_VFS_BACKEND_WORKER_REQUIRED) return 0;
    if (sc2(SYS_STAT, "hello.txt", &st) != 0) return 0;
    n = sc3(SYS_LISTDIR, "", list, 16U);
    if (n <= 0) return 0;
    for (i = 0; i < n; i++) if (same(list[i].name, "note.txt")) return 0;
    if (sc1(SYS_MKDIR, "histdir") != OS_VFS_BACKEND_WORKER_REQUIRED) return 0;
    if (sc1(SYS_UNLINK, "note.txt") != OS_VFS_BACKEND_WORKER_REQUIRED) return 0;
    if (sc2(SYS_RENAME, "note.txt", "histmv.txt") != OS_VFS_BACKEND_WORKER_REQUIRED) return 0;
    if (sc2(SYS_COPY, "hello.txt", "histcp.txt") != OS_VFS_BACKEND_WORKER_REQUIRED) return 0;
    if (sc3(SYS_APPEND, "note.txt", "z", 1U) != OS_VFS_BACKEND_WORKER_REQUIRED) return 0;
    return 1;
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
        if (hist_overlay_entry_points_mediated())
            puts("vfshistclaim overlay entry points worker-mediated\n");
        else
            puts("vfshistclaim overlay entry points unexpected\n");
    } else if (overlay_rc >= 0 && write_rc >= 0) {
        /* Degraded mode: no live vfs-virtual, historical local I/O remains. */
        puts("vfshistclaim historical local ok\n");
    } else {
        puts("vfshistclaim unexpected result\n");
    }
    for (;;) yield();
}
