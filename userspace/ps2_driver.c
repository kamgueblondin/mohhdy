/* Lot 5 (US-001): Ring 3 PS/2 keyboard and mouse driver (binary ps2driver).
 *
 * Registers "ps2-driver" (name pinned to this binary), claims the
 * controller (the kernel opens ports 0x60 and 0x64 in its TSS IOPB), then
 * loops: sleep until IRQ1 (OS_DEV_OP_PS2_WAIT), read every byte the
 * controller holds at CPL 3, decode scan code set 1 and mouse packets
 * (ps2_decode.c) and submit the events. The kernel only counts the IRQ
 * and wakes this task; it no longer touches the ports while this driver
 * lives. If it stalls (not scheduled for 5 ticks) or dies, Ring 0 reads
 * the controller again (counted) and the kernel respawns bin/ps2driver
 * (budget 4). Success is logged by the kernel on the serial line only, so
 * the shell screen is not disturbed. */
#include "ipc_wait_common.h"
#include "ps2_decode.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static int dev_op(uint32_t op, uint32_t a, uint32_t b, uint32_t c) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_DEVICE_SERVICE), "b"(op), "c"(a), "d"(b), "S"(c) : "memory");
    return rc;
}

static os_input_event_t events[OS_INPUT_MAX_EVENTS];

void main(void) {
    ps2_decoder_t dec;
    int rc = iw_service_register("ps2-driver");
    if (rc < 0) {
        iw_puts("ps2driver register refused rc ");
        iw_print_int(rc);
        iw_putc('\n');
        return;
    }
    rc = dev_op(OS_DEV_OP_PS2_CLAIM, 0U, 0U, 0U);
    if (rc != 0) {
        iw_puts("ps2driver claim refused rc ");
        iw_print_int(rc);
        iw_putc('\n');
        return;
    }
    ps2_decoder_init(&dec);
    /* Lot 6: a respawned driver starts from the live modifier state (Shift
     * held across the death of the previous driver stays held). */
    rc = dev_op(OS_DEV_OP_PS2_MODS, 0U, 0U, 0U);
    if (rc >= 0) ps2_decoder_seed(&dec, (uint32_t)rc);
    for (;;) {
        uint32_t n = 0U, reads = 0U;
        int seq = dev_op(OS_DEV_OP_PS2_WAIT, 50U, 0U, 0U);
        if (seq < 0) continue;
        while (reads < 64U) {
            uint8_t status = inb(0x64);
            uint8_t byte;
            if (!(status & 0x01U)) break;
            byte = inb(0x60);
            reads++;
            (void)ps2_decode_byte(&dec, byte, (status & 0x20U) ? 1 : 0, events, &n, OS_INPUT_MAX_EVENTS);
            if (n == OS_INPUT_MAX_EVENTS) {
                /* full batch: hand it over, keep the IRQ pending (old seq) */
                (void)dev_op(OS_DEV_OP_PS2_SUBMIT, (uint32_t)events, n, (uint32_t)seq - 1U);
                n = 0U;
            }
        }
        (void)dev_op(OS_DEV_OP_PS2_SUBMIT, (uint32_t)events, n, (uint32_t)seq);
    }
}
