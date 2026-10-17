/* Lot 5 (US-001): Ring 3 COM1 driver (binary serialdriver).
 *
 * Registers "serial-driver" (name pinned to this binary), claims COM1 (the
 * kernel opens 0x3F8-0x3FF in its TSS IOPB). Every kernel and user write
 * to the serial line then lands in one kernel FIFO (order kept); this
 * task takes it in chunks (OS_DEV_OP_SERIAL_TAKE, sleeps when empty),
 * writes each byte itself at CPL 3 once THR is empty, and forwards the
 * received bytes (host display bridge, nographic input) to the keyboard
 * queue (OS_DEV_OP_SERIAL_RX). The kernel writes from Ring 0 itself before
 * the claim, on a kernel exception, when the FIFO is full, when this task
 * is not scheduled for 5 ticks and after its death (all counted); it then
 * respawns bin/serialdriver (budget 4). This task prints nothing on
 * success: its own output would feed its FIFO. */
#include "ipc_wait_common.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static int dev_op(uint32_t op, uint32_t a, uint32_t b, uint32_t c) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_DEVICE_SERVICE), "b"(op), "c"(a), "d"(b), "S"(c) : "memory");
    return rc;
}

static uint8_t tx[OS_SERIAL_CHUNK];
static uint8_t rx[OS_SERIAL_CHUNK];

static void poll_rx(void) {
    uint32_t n = 0U;
    while (n < OS_SERIAL_CHUNK && (inb(0x3FD) & 0x01U)) rx[n++] = inb(0x3F8);
    if (n) (void)dev_op(OS_DEV_OP_SERIAL_RX, (uint32_t)rx, n, 0U);
}

void main(void) {
    int rc = iw_service_register("serial-driver");
    if (rc < 0) {
        iw_puts("serialdriver register refused rc ");
        iw_print_int(rc);
        iw_putc('\n');
        return;
    }
    rc = dev_op(OS_DEV_OP_SERIAL_CLAIM, 0U, 0U, 0U);
    if (rc != 0) {
        iw_puts("serialdriver claim refused rc ");
        iw_print_int(rc);
        iw_putc('\n');
        return;
    }
    for (;;) {
        int i, n;
        poll_rx();
        /* short sleep: RX has no IRQ here, it is polled between chunks */
        n = dev_op(OS_DEV_OP_SERIAL_TAKE, (uint32_t)tx, OS_SERIAL_CHUNK, 2U);
        for (i = 0; i < n; i++) {
            while (!(inb(0x3FD) & 0x20U)) { }
            outb(0x3F8, tx[i]);
        }
    }
}
