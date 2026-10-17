/* Lot 8 (US-001): Ring 3 UHCI USB tablet driver (binary usbdriver).
 *
 * Registers "usb-driver" (name pinned to this binary) and claims the UHCI
 * controller the kernel found and stopped at boot. After the claim:
 *   - the data page is writable at OS_USB_DMA_WINDOW (setup packets,
 *     reports), the schedule page read-only at OS_USB_SCHED_WINDOW (TD
 *     status words);
 *   - USBSTS, USBINTR, FRNUM and PORTSC1/2 are open in this task's IOPB
 *     (USBCMD and FLBASEADD are not: the kernel runs the controller).
 * This task resets the port, enumerates the tablet (SET_ADDRESS 2,
 * SET_CONFIGURATION 1) with control transfers, then keeps one interrupt IN
 * TD armed and turns each report into an absolute pointer update
 * (OS_DEV_OP_USB_REPORT). Every TD is written by the kernel after it
 * checked that its buffer lies in the data page (bounded DMA). */
#include "ipc_wait_common.h"

#define DEV_ADDR 2U
#define SETUP_OFF 0x000U
#define REPORT_OFF 0x100U

static inline void outw(uint16_t port, uint16_t value) { asm volatile("outw %0, %1" : : "a"(value), "Nd"(port)); }
static inline uint16_t inw(uint16_t port) { uint16_t v; asm volatile("inw %1, %0" : "=a"(v) : "Nd"(port)); return v; }

static int dev_op(uint32_t op, uint32_t a, uint32_t b, uint32_t c) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_DEVICE_SERVICE), "b"(op), "c"(a), "d"(b), "S"(c) : "memory");
    return rc;
}

static volatile uint8_t* const g_data = (volatile uint8_t*)OS_USB_DMA_WINDOW;
static volatile const os_usb_sched_t* const g_sched = (volatile const os_usb_sched_t*)OS_USB_SCHED_WINDOW;
static uint8_t g_low_speed;

static void say(const char* text, int value) {
    iw_puts("usbdriver ");
    iw_puts(text);
    iw_print_int(value);
    iw_putc('\n');
}

static int td(uint8_t slot, uint8_t pid, uint8_t addr, uint8_t ep, uint8_t toggle,
              uint16_t off, uint16_t len, uint8_t next) {
    os_usb_td_req_t r;
    r.slot = slot; r.pid = pid; r.dev_addr = addr; r.endpoint = ep; r.toggle = toggle;
    r.low_speed = g_low_speed; r.next = next; r.pad = 0U; r.buf_offset = off; r.length = len;
    return dev_op(OS_DEV_OP_USB_TD, (uint32_t)&r, 0U, 0U);
}

static void sleep_ticks(uint32_t ticks) { (void)dev_op(OS_DEV_OP_USB_WAIT, ticks, 0U, 0U); }

/* SETUP (8 bytes, no data stage) + IN status stage; 0 when both are done
 * without error. */
static int control(uint8_t addr, uint8_t req_type, uint8_t req, uint16_t value) {
    uint32_t i, st0, st1;
    g_data[SETUP_OFF + 0] = req_type;
    g_data[SETUP_OFF + 1] = req;
    g_data[SETUP_OFF + 2] = (uint8_t)(value & 0xFFU);
    g_data[SETUP_OFF + 3] = (uint8_t)(value >> 8);
    for (i = 4U; i < 8U; i++) g_data[SETUP_OFF + i] = 0U;
    if (td(0U, OS_USB_PID_SETUP, addr, 0U, 0U, SETUP_OFF, 8U, 1U) != 0) return -10;
    if (td(1U, OS_USB_PID_IN, addr, 0U, 1U, 0U, 0U, OS_USB_TD_END) != 0) return -11;
    if (dev_op(OS_DEV_OP_USB_RUN, 0U, 1U, 0U) < 0) return -12;
    for (i = 0U; i < 20U; i++) {
        st0 = g_sched->td[0].status;
        st1 = g_sched->td[1].status;
        if ((st0 & OS_USB_TD_ACTIVE) == 0U && (st0 & OS_USB_TD_ERRORS) != 0U) return -1;
        if ((st0 & OS_USB_TD_ACTIVE) == 0U && (st1 & OS_USB_TD_ACTIVE) == 0U)
            return (st1 & OS_USB_TD_ERRORS) ? -1 : 0;
        (void)dev_op(OS_DEV_OP_USB_WAIT, 5U, 0U, 0U);
    }
    return -2;
}

static int port_reset(uint16_t base) {
    uint16_t ports[2] = { (uint16_t)(base + 0x10U), (uint16_t)(base + 0x12U) };
    uint32_t i;
    for (i = 0U; i < 2U; i++) {
        uint16_t sc = inw(ports[i]);
        if (!(sc & 0x0001U)) continue;          /* nothing connected */
        outw(ports[i], 0x0200U);                /* port reset */
        sleep_ticks(2U);
        outw(ports[i], 0x0004U);                /* port enable */
        sleep_ticks(2U);
        sc = inw(ports[i]);
        g_low_speed = (sc & 0x0100U) ? 1U : 0U;
        return (int)i + 1;
    }
    return 0;
}

static int arm_report(uint8_t toggle) {
    if (td(2U, OS_USB_PID_IN, DEV_ADDR, 1U, toggle, REPORT_OFF, 8U, OS_USB_TD_END) != 0) return -1;
    return dev_op(OS_DEV_OP_USB_RUN, 2U, 2U, 0U) < 0 ? -1 : 0;
}

void main(void) {
    int rc, port, r1, r2;
    uint16_t base;
    uint8_t toggle = 0U;
    uint32_t last = 0xFFFFFFFFU, reports = 0U;
    rc = iw_service_register("usb-driver");
    if (rc < 0) { say("register refused rc ", rc); return; }
    rc = dev_op(OS_DEV_OP_USB_CLAIM, 0U, 0U, 0U);
    if (rc <= 0) { say("claim refused rc ", rc); return; }
    base = (uint16_t)rc;
    /* Bounds probe: the kernel must refuse a buffer that leaves the data
     * page and a schedule that loops (3 -> 4 -> 3), before any transfer. */
    r1 = td(3U, OS_USB_PID_IN, DEV_ADDR, 1U, 0U, (uint16_t)(OS_USB_DATA_SIZE - 32U), 64U, OS_USB_TD_END);
    (void)td(3U, OS_USB_PID_IN, DEV_ADDR, 1U, 0U, REPORT_OFF, 8U, 4U);
    (void)td(4U, OS_USB_PID_IN, DEV_ADDR, 1U, 0U, REPORT_OFF, 8U, 3U);
    r2 = dev_op(OS_DEV_OP_USB_RUN, 3U, 3U, 0U);
    iw_puts("usbdriver dma bounds probe outside rc ");
    iw_print_int(r1);
    iw_puts(" loop rc ");
    iw_print_int(r2);
    iw_putc('\n');
    port = port_reset(base);
    if (port == 0) { say("no device on the root ports ", 0); for (;;) sleep_ticks(1000U); }
    r1 = control(0U, 0x00U, 0x05U, DEV_ADDR);       /* SET_ADDRESS */
    sleep_ticks(1U);
    r2 = control(DEV_ADDR, 0x00U, 0x09U, 1U);        /* SET_CONFIGURATION */
    if (r1 != 0 || r2 != 0) {
        say("enumeration failed r1 ", r1);
        say("enumeration failed r2 ", r2);
        for (;;) sleep_ticks(1000U);
    }
    iw_puts("usbdriver tablet enumerated port ");
    iw_print_int(port);
    iw_puts(" addr 2 (Ring 3, kernel-checked DMA)\n");
    if (arm_report(toggle) != 0) { say("arm refused ", -1); for (;;) sleep_ticks(1000U); }
    for (;;) {
        uint32_t st, len;
        (void)dev_op(OS_DEV_OP_USB_WAIT, 50U, 0U, 0U);
        st = g_sched->td[2].status;
        if (st & OS_USB_TD_ACTIVE) continue;
        len = (st + 1U) & 0x7FFU;
        if ((st & OS_USB_TD_ERRORS) == 0U && len >= 5U && len <= 8U) {
            uint32_t buttons = g_data[REPORT_OFF] & 7U;
            uint32_t x = (uint32_t)g_data[REPORT_OFF + 1] | ((uint32_t)g_data[REPORT_OFF + 2] << 8);
            uint32_t y = (uint32_t)g_data[REPORT_OFF + 3] | ((uint32_t)g_data[REPORT_OFF + 4] << 8);
            uint32_t key = (x << 17) ^ (y << 2) ^ buttons;
            if (key != last) {
                (void)dev_op(OS_DEV_OP_USB_REPORT, x, y, buttons);
                last = key;
                if (++reports == 1U) iw_puts("usbdriver first report applied\n");
            }
            toggle ^= 1U;
        }
        (void)arm_report(toggle);
    }
}
