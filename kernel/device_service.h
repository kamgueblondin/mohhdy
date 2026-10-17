#ifndef MOHHDY_DEVICE_SERVICE_H
#define MOHHDY_DEVICE_SERVICE_H

/* Lot 5 (US-001): bookkeeping of the Ring 3 PS/2 and serial drivers. Pure
 * logic (no port I/O, no task table) so the host unit tests cover it; the
 * kernel glue (syscall.c, keyboard.c, kernel.c, timer.c) applies it. */
#include <stdint.h>
#include "../include/os_syscalls.h"

#define DEVICE_RESPAWN_LIMIT 4U
#define DEVICE_STALL_TICKS 5U         /* driver silent this long: Ring 0 takes over */
#define DEVICE_INFLIGHT_STALE 50U     /* a taken chunk never completed (driver dead) */
#define DEVICE_SERIAL_RING 8192U      /* power of two */

enum { DEV_PS2 = 0, DEV_SERIAL = 1, DEV_VGA = 2, DEV_USB = 3, DEV_COUNT = 4 };

/* Serial queue verdicts for devsvc_serial_put. */
#define DEVSVC_QUEUED 0
#define DEVSVC_DIRECT 1   /* no live driver: write the byte from Ring 0 */
#define DEVSVC_FLUSH 2    /* queue full: Ring 0 drains it, then writes the byte */

/* Watchdog actions (bit mask) for devsvc_watchdog. */
#define DEVSVC_W_PS2_DRAIN 1U     /* read pending PS/2 bytes from Ring 0 */
#define DEVSVC_W_SERIAL_FLUSH 2U  /* drain the serial queue from Ring 0 */
#define DEVSVC_W_CHECK_OWNERS 4U  /* re-check that the owners are alive */
#define DEVSVC_W_VGA_REPAINT 8U   /* Ring 0 paints the current VGA frame */

void devsvc_reset(void);
/* Owner management: claim by the live service owner, loss on death. */
void devsvc_set_owner(int dev, int32_t pid);
int32_t devsvc_owner(int dev);
void devsvc_owner_gone(int dev);
int devsvc_handed_off(int dev);
/* Supervisor: 1 = spawn the driver now (no owner, none starting, budget left). */
int devsvc_should_respawn(int dev, int pending_alive);
void devsvc_note_respawn(int dev);

/* PS/2: IRQ1 counted for the driver (returns the new sequence number). */
uint32_t devsvc_ps2_irq(uint32_t now);
uint32_t devsvc_ps2_seq(void);
/* Driver submitted events after reading up to `seq`: clears the pending
 * flag only when no newer IRQ arrived. Returns 1 when cleared. */
int devsvc_ps2_served(uint32_t seq, uint32_t events, uint32_t now);
int devsvc_ps2_pending(void);
void devsvc_ps2_note_ring0(uint32_t bytes);

/* Serial transmit queue. */
int devsvc_serial_put(uint8_t c);
/* Driver takes up to cap bytes (marks the chunk in flight until its next take). */
uint32_t devsvc_serial_take(uint8_t* out, uint32_t cap, uint32_t now);
/* Driver is back for more: the previous chunk is written. */
void devsvc_serial_done(uint32_t now);
uint32_t devsvc_serial_queued(void);
/* Ring 0 drain: pops one byte (1) or reports empty (0); counts it. */
int devsvc_serial_pop_ring0(uint8_t* c);
void devsvc_serial_note_overflow(void);
void devsvc_serial_note_rx(uint32_t bytes);

/* 1 when the driver of `dev` has work and should get the CPU at the end
 * of the current syscall (a long kernel path otherwise starves it into the
 * Ring 0 fallback): PS/2 IRQ pending, or serial bytes waiting for a chunk
 * (at least DEVSVC_SERIAL_BATCH bytes or idle for 2 ticks). */
#define DEVSVC_SERIAL_BATCH 64U
int devsvc_wants_cpu(int dev, uint32_t now);
uint32_t devsvc_watchdog(uint32_t now);

/* Lot 7 VGA text console: the kernel renders a frame; with a live owner it
 * is marked pending for the driver (DEVSVC_QUEUED), otherwise Ring 0 paints
 * it (DEVSVC_DIRECT, counted once the device was handed off). */
int devsvc_vga_present(uint32_t now);
int devsvc_vga_pending(void);
/* Driver takes the pending frame: 1 when there was one. */
int devsvc_vga_take(uint32_t now);
void devsvc_vga_note_ring0(void);
/* Lot 8 UHCI USB driver: no Ring 0 fallback. When the watched TD is done
 * (seen by the IRQ0 glue) the driver is woken; when the owner dies the glue
 * stops the controller. */
void devsvc_usb_set_present(int present);
int devsvc_usb_present(void);
void devsvc_usb_note_td(int accepted);
void devsvc_usb_note_report(void);
void devsvc_usb_note_stop(void);
/* IRQ0 glue: the watched TD finished (1) or a new one was armed (0). */
void devsvc_usb_set_done(int done);
int devsvc_usb_done(void);
void devsvc_status(os_device_status_t* out);

#endif
