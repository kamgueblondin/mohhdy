/* Lot 7 (US-001): Ring 3 VGA text console driver (binary vgadriver).
 *
 * Registers "vga-driver" (name pinned to this binary) and claims the text
 * console: the kernel maps the text page 0xB8000 writable at
 * OS_VGA_DRIVER_WINDOW in this task only and opens the CRTC ports
 * 0x3D4/0x3D5 in its TSS IOPB. The kernel keeps the console model (rows,
 * scrollback, cursor) and renders frames; this task takes each new frame
 * (OS_DEV_OP_VGA_TAKE, sleeps while none is pending), writes the changed
 * cells into video memory and programs the hardware cursor at CPL 3.
 * Ring 0 paints before the claim (early boot), after this task dies (until
 * the supervised respawn claims), on a stall and after a kernel exception
 * (all counted after the handoff). This task prints nothing on success. */
#include "ipc_wait_common.h"

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static int dev_op(uint32_t op, uint32_t a, uint32_t b, uint32_t c) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_DEVICE_SERVICE), "b"(op), "c"(a), "d"(b), "S"(c) : "memory");
    return rc;
}

static os_vga_driver_frame_t g_frame;
static uint32_t g_epoch;
static uint16_t g_shown[OS_VGA_CELLS];

static void cursor(const os_vga_driver_frame_t* f) {
    uint16_t pos;
    if (!f->cursor_visible) {
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x20);
        return;
    }
    pos = (uint16_t)(f->cursor_y * 80U + f->cursor_x);
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x00);
    outb(0x3D4, 0x0B);
    outb(0x3D5, 0x0F);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFFU));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFFU));
}

void main(void) {
    volatile uint16_t* vram = (volatile uint16_t*)OS_VGA_DRIVER_WINDOW;
    uint32_t i;
    int rc = iw_service_register("vga-driver");
    if (rc < 0) {
        iw_puts("vgadriver register refused rc ");
        iw_print_int(rc);
        iw_putc('\n');
        return;
    }
    rc = dev_op(OS_DEV_OP_VGA_CLAIM, 0U, 0U, 0U);
    if (rc != 0) {
        iw_puts("vgadriver claim refused rc ");
        iw_print_int(rc);
        iw_putc('\n');
        return;
    }
    /* what Ring 0 painted during the boot is on screen now */
    for (i = 0; i < OS_VGA_CELLS; i++) g_shown[i] = vram[i];
    for (;;) {
        if (dev_op(OS_DEV_OP_VGA_TAKE, (uint32_t)&g_frame, 0U, 50U) != 1) continue;
        if (g_frame.vram_epoch != g_epoch) {
            /* video RAM was reused by the VBE desktop: forget the cache */
            g_epoch = g_frame.vram_epoch;
            for (i = 0; i < OS_VGA_CELLS; i++) g_shown[i] = (uint16_t)~g_frame.cells[i];
        }
        for (i = 0; i < OS_VGA_CELLS; i++) {
            if (g_shown[i] != g_frame.cells[i]) {
                vram[i] = g_frame.cells[i];
                g_shown[i] = g_frame.cells[i];
            }
        }
        cursor(&g_frame);
    }
}
