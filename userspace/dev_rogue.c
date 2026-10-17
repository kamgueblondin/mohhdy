/* Lot 5 (US-001) negative proof (binary devrogue): a task that is not the
 * live ps2driver/serialdriver is refused the device claims and faults (#GP,
 * killed) on its first IN from the PS/2 status port. */
#include "ipc_wait_common.h"

static int dev_op(uint32_t op, uint32_t a, uint32_t b, uint32_t c) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_DEVICE_SERVICE), "b"(op), "c"(a), "d"(b), "S"(c) : "memory");
    return rc;
}

void main(void) {
    uint8_t v;
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
