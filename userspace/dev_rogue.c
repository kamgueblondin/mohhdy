/* Lot 5 (US-001) negative proof (binary devrogue): a task that is not the
 * live ps2driver/serialdriver is refused the device claims and faults (#GP,
 * killed) on its first IN from the PS/2 status port.
 * Lot 6 (US-059/060): it also tries to unseal /etc/pm_ca.key; the kernel
 * binds the blob to the shell task name, so this must fail (OS_SEAL_AUTH). */
#include "ipc_wait_common.h"

static int dev_op(uint32_t op, uint32_t a, uint32_t b, uint32_t c) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_DEVICE_SERVICE), "b"(op), "c"(a), "d"(b), "S"(c) : "memory");
    return rc;
}

static int hexv(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static void seal_probe(void) {
    static char kbuf[160];
    static uint8_t blob[80], out[80];
    int n, i, len = 0, rc;
    asm volatile("int $0x80" : "=a"(n) : "a"(SYS_READFILE), "b"("/etc/pm_ca.key"), "c"(kbuf), "d"((int)sizeof(kbuf) - 1) : "memory");
    if (n < 13 || kbuf[0] != 'p' || kbuf[4] != '2') { iw_puts("devrogue unseal nokey\n"); return; }
    for (i = 13; i + 1 < n && len < (int)sizeof(blob); i += 2) {
        int hi = hexv(kbuf[i]), lo = hexv(kbuf[i + 1]);
        if (hi < 0 || lo < 0) break;
        blob[len++] = (uint8_t)(hi * 16 + lo);
    }
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_KEY_SEAL), "b"(OS_SEAL_OP_UNSEAL), "c"(blob), "d"(len), "S"(out), "D"((int)sizeof(out)) : "memory");
    iw_puts("devrogue unseal rc ");
    iw_print_int(rc);
    iw_putc('\n');
}

void main(void) {
    uint8_t v;
    seal_probe();
    iw_puts("devrogue ps2 claim rc ");
    iw_print_int(dev_op(OS_DEV_OP_PS2_CLAIM, 0U, 0U, 0U));
    iw_puts(" serial claim rc ");
    iw_print_int(dev_op(OS_DEV_OP_SERIAL_CLAIM, 0U, 0U, 0U));
    iw_puts(" submit rc ");
    iw_print_int(dev_op(OS_DEV_OP_PS2_SUBMIT, 0U, 0U, 0U));
    iw_putc('\n');
    iw_puts("devrogue in 0x64\n");
    asm volatile("inb %1, %0" : "=a"(v) : "Nd"((uint16_t)0x64));
    iw_puts("devrogue unexpected port access ");
    iw_print_int(v);
    iw_putc('\n');
}
