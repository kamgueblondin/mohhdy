/* Lot 5 (US-001): Ring 3 PS/2 and serial driver bookkeeping; Lot 7 adds
 * the VGA text console driver. See device_service.h. */
#include "device_service.h"

typedef struct {
    int32_t owner;
    uint32_t handed_off;
    uint32_t respawns;
} dev_owner_t;

static dev_owner_t g_dev[DEV_COUNT];

static struct {
    uint32_t seq, served_seq, pending_since, pending;
    uint32_t irqs, events, ring0_bytes, stalls, last_progress;
} g_ps2;

static struct {
    uint8_t ring[DEVICE_SERIAL_RING];
    uint32_t head, tail;          /* head: next write, tail: next read */
    uint32_t inflight, inflight_since, last_take;
    uint32_t ring3_bytes, ring0_bytes, stalls, overflows, rx_bytes;
} g_ser;

static struct {
    uint32_t pending, pending_since, seq;
    uint32_t ring3_frames, ring0_frames, stalls;
} g_vga;

static struct {
    uint32_t present, tds, rejected, reports, stops, done;
} g_usb;

static uint32_t g_last_owner_check;

void devsvc_reset(void) {
    uint8_t* p;
    uint32_t i;
    p = (uint8_t*)g_dev; for (i = 0; i < sizeof(g_dev); i++) p[i] = 0;
    p = (uint8_t*)&g_ps2; for (i = 0; i < sizeof(g_ps2); i++) p[i] = 0;
    p = (uint8_t*)&g_ser; for (i = 0; i < sizeof(g_ser); i++) p[i] = 0;
    p = (uint8_t*)&g_vga; for (i = 0; i < sizeof(g_vga); i++) p[i] = 0;
    p = (uint8_t*)&g_usb; for (i = 0; i < sizeof(g_usb); i++) p[i] = 0;
    g_last_owner_check = 0;
}

void devsvc_set_owner(int dev, int32_t pid) {
    if (dev < 0 || dev >= DEV_COUNT) return;
    g_dev[dev].owner = pid;
    if (pid > 0) g_dev[dev].handed_off = 1U;
    if (dev == DEV_PS2) g_ps2.pending = 0;
    if (dev == DEV_SERIAL) g_ser.inflight = 0;
    if (dev == DEV_VGA) g_vga.pending = pid > 0 ? 1U : 0U;   /* the new owner paints the current frame */
}

int32_t devsvc_owner(int dev) { return (dev >= 0 && dev < DEV_COUNT) ? g_dev[dev].owner : 0; }

void devsvc_owner_gone(int dev) {
    if (dev < 0 || dev >= DEV_COUNT) return;
    g_dev[dev].owner = 0;
    if (dev == DEV_SERIAL) g_ser.inflight = 0;
    if (dev == DEV_VGA) g_vga.pending = 0;
    if (dev == DEV_USB) g_usb.done = 0;
}

int devsvc_handed_off(int dev) { return (dev >= 0 && dev < DEV_COUNT) ? (int)g_dev[dev].handed_off : 0; }

int devsvc_should_respawn(int dev, int pending_alive) {
    if (dev < 0 || dev >= DEV_COUNT) return 0;
    if (g_dev[dev].owner > 0 || pending_alive) return 0;
    if (!g_dev[dev].handed_off) return 0;   /* never claimed: boot spawn failed or not there yet */
    return g_dev[dev].respawns < DEVICE_RESPAWN_LIMIT;
}

void devsvc_note_respawn(int dev) { if (dev >= 0 && dev < DEV_COUNT) g_dev[dev].respawns++; }

uint32_t devsvc_ps2_irq(uint32_t now) {
    g_ps2.seq++;
    g_ps2.irqs++;
    if (!g_ps2.pending) { g_ps2.pending = 1U; g_ps2.pending_since = now; }
    return g_ps2.seq;
}

uint32_t devsvc_ps2_seq(void) { return g_ps2.seq; }

int devsvc_ps2_served(uint32_t seq, uint32_t events, uint32_t now) {
    g_ps2.events += events;
    g_ps2.last_progress = now;
    if (seq != g_ps2.seq) {
        /* a newer IRQ came while the driver read: keep it pending but
         * restart the stall clock from this progress */
        if (g_ps2.pending) g_ps2.pending_since = now;
        return 0;
    }
    g_ps2.served_seq = seq;
    g_ps2.pending = 0;
    return 1;
}

int devsvc_ps2_pending(void) { return (int)g_ps2.pending; }

void devsvc_ps2_note_ring0(uint32_t bytes) { g_ps2.ring0_bytes += bytes; }

int devsvc_serial_put(uint8_t c) {
    uint32_t next = (g_ser.head + 1U) & (DEVICE_SERIAL_RING - 1U);
    if (g_dev[DEV_SERIAL].owner <= 0)                /* leftovers go out first */
        return g_ser.head == g_ser.tail ? DEVSVC_DIRECT : DEVSVC_FLUSH;
    if (next == g_ser.tail) return DEVSVC_FLUSH;
    g_ser.ring[g_ser.head] = c;
    g_ser.head = next;
    return DEVSVC_QUEUED;
}

uint32_t devsvc_serial_queued(void) { return (g_ser.head - g_ser.tail) & (DEVICE_SERIAL_RING - 1U); }

uint32_t devsvc_serial_take(uint8_t* out, uint32_t cap, uint32_t now) {
    uint32_t n = 0;
    g_ser.inflight = 0;
    g_ser.last_take = now;
    while (out && n < cap && g_ser.tail != g_ser.head) {
        out[n++] = g_ser.ring[g_ser.tail];
        g_ser.tail = (g_ser.tail + 1U) & (DEVICE_SERIAL_RING - 1U);
    }
    if (n) { g_ser.inflight = 1U; g_ser.inflight_since = now; g_ser.ring3_bytes += n; }
    return n;
}

void devsvc_serial_done(uint32_t now) { g_ser.inflight = 0; g_ser.last_take = now; }

int devsvc_serial_pop_ring0(uint8_t* c) {
    if (g_ser.tail == g_ser.head || !c) return 0;
    *c = g_ser.ring[g_ser.tail];
    g_ser.tail = (g_ser.tail + 1U) & (DEVICE_SERIAL_RING - 1U);
    g_ser.ring0_bytes++;
    return 1;
}

void devsvc_serial_note_overflow(void) { g_ser.overflows++; }
void devsvc_serial_note_rx(uint32_t bytes) { g_ser.rx_bytes += bytes; }

int devsvc_wants_cpu(int dev, uint32_t now) {
    if (dev < 0 || dev >= DEV_COUNT || g_dev[dev].owner <= 0) return 0;
    if (dev == DEV_PS2) return g_ps2.pending ? 1 : 0;
    /* frames coalesce: hand over the CPU once one waited 2 ticks */
    if (dev == DEV_VGA) return g_vga.pending && now - g_vga.pending_since >= 2U;
    if (dev == DEV_USB) return g_usb.done ? 1 : 0;
    if (g_ser.head == g_ser.tail || g_ser.inflight) return 0;
    return devsvc_serial_queued() >= DEVSVC_SERIAL_BATCH || now - g_ser.last_take >= 2U;
}

uint32_t devsvc_watchdog(uint32_t now) {
    uint32_t act = 0;
    if (g_dev[DEV_PS2].owner > 0 && g_ps2.pending && now - g_ps2.pending_since >= DEVICE_STALL_TICKS) {
        act |= DEVSVC_W_PS2_DRAIN;
        g_ps2.stalls++;
        g_ps2.pending = 0;
    }
    if (g_ser.head != g_ser.tail) {
        if (g_dev[DEV_SERIAL].owner <= 0) {
            act |= DEVSVC_W_SERIAL_FLUSH;              /* driver gone: leftovers */
        } else if (g_ser.inflight) {
            /* A chunk the driver took and has not finished: flushing the
             * queue from Ring 0 now would print later bytes before it.
             * Count the stall and keep the order; a dead driver is found
             * by the owner check, which flushes the leftovers. */
            if (now - g_ser.inflight_since >= DEVICE_INFLIGHT_STALE) {
                g_ser.stalls++;
                g_ser.inflight_since = now;
            }
        } else if (now - g_ser.last_take >= DEVICE_STALL_TICKS) {
            act |= DEVSVC_W_SERIAL_FLUSH;
            g_ser.stalls++;
        }
    }
    if (g_dev[DEV_VGA].owner > 0 && g_vga.pending && now - g_vga.pending_since >= DEVICE_STALL_TICKS) {
        act |= DEVSVC_W_VGA_REPAINT;    /* frame not taken: Ring 0 paints it */
        g_vga.stalls++;
        g_vga.pending = 0;
    }
    if (now - g_last_owner_check >= 10U) {
        g_last_owner_check = now;
        if (g_dev[DEV_PS2].owner > 0 || g_dev[DEV_SERIAL].owner > 0 || g_dev[DEV_VGA].owner > 0 ||
            g_dev[DEV_USB].owner > 0)
            act |= DEVSVC_W_CHECK_OWNERS;
    }
    return act;
}

int devsvc_vga_present(uint32_t now) {
    g_vga.seq++;
    if (g_dev[DEV_VGA].owner <= 0) {
        if (g_dev[DEV_VGA].handed_off) g_vga.ring0_frames++;
        return DEVSVC_DIRECT;
    }
    if (!g_vga.pending) { g_vga.pending = 1U; g_vga.pending_since = now; }
    return DEVSVC_QUEUED;
}

int devsvc_vga_pending(void) { return (int)g_vga.pending; }

int devsvc_vga_take(uint32_t now) {
    (void)now;
    if (!g_vga.pending || g_dev[DEV_VGA].owner <= 0) return 0;
    g_vga.pending = 0;
    g_vga.ring3_frames++;
    return 1;
}

void devsvc_vga_note_ring0(void) { g_vga.ring0_frames++; }

void devsvc_usb_set_present(int present) { g_usb.present = present ? 1U : 0U; }
int devsvc_usb_present(void) { return (int)g_usb.present; }
void devsvc_usb_note_td(int accepted) { if (accepted) g_usb.tds++; else g_usb.rejected++; }
void devsvc_usb_note_report(void) { g_usb.reports++; }
void devsvc_usb_note_stop(void) { g_usb.stops++; }
void devsvc_usb_set_done(int done) { g_usb.done = (done && g_dev[DEV_USB].owner > 0) ? 1U : 0U; }
int devsvc_usb_done(void) { return (int)g_usb.done; }

void devsvc_status(os_device_status_t* out) {
    if (!out) return;
    out->mouse_packets = 0; out->mouse_dx = 0; out->mouse_dy = 0; out->mouse_buttons = 0; out->kbd_mods = 0;
    out->ps2_owner = g_dev[DEV_PS2].owner;
    out->serial_owner = g_dev[DEV_SERIAL].owner;
    out->ps2_irqs = g_ps2.irqs;
    out->ps2_events = g_ps2.events;
    out->ps2_ring0_bytes = g_ps2.ring0_bytes;
    out->ps2_stalls = g_ps2.stalls;
    out->ps2_respawns = g_dev[DEV_PS2].respawns;
    out->serial_ring3_bytes = g_ser.ring3_bytes;
    out->serial_ring0_bytes = g_ser.ring0_bytes;
    out->serial_stalls = g_ser.stalls;
    out->serial_overflows = g_ser.overflows;
    out->serial_rx_bytes = g_ser.rx_bytes;
    out->serial_respawns = g_dev[DEV_SERIAL].respawns;
    out->ps2_handed_off = g_dev[DEV_PS2].handed_off;
    out->serial_handed_off = g_dev[DEV_SERIAL].handed_off;
    out->vga_owner = g_dev[DEV_VGA].owner;
    out->vga_handed_off = g_dev[DEV_VGA].handed_off;
    out->vga_ring3_frames = g_vga.ring3_frames;
    out->vga_ring0_frames = g_vga.ring0_frames;
    out->vga_stalls = g_vga.stalls;
    out->vga_respawns = g_dev[DEV_VGA].respawns;
    out->usb_owner = g_dev[DEV_USB].owner;
    out->usb_handed_off = g_dev[DEV_USB].handed_off;
    out->usb_present = g_usb.present;
    out->usb_tds = g_usb.tds;
    out->usb_rejected = g_usb.rejected;
    out->usb_reports = g_usb.reports;
    out->usb_stops = g_usb.stops;
    out->usb_respawns = g_dev[DEV_USB].respawns;
}
