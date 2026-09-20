#include "usb_tablet.h"
#include "../pci.h"
#include "../gfx_fb.h"
#include "../gfx_desktop.h"
#include <stdint.h>
#include <stddef.h>

extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char value);
extern void print_string_serial(const char* str);

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t val;
    __asm__ volatile ("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t val;
    __asm__ volatile ("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

#define UHCI_CMD         0x00
#define UHCI_STS         0x02
#define UHCI_INTR        0x04
#define UHCI_FRNUM       0x06
#define UHCI_FLBASEADD   0x08
#define UHCI_PORTSC1     0x10
#define UHCI_PORTSC2     0x12

#define UHCI_CMD_RUN     0x0001
#define UHCI_CMD_HCRESET 0x0002
#define UHCI_CMD_MAXP    0x0080

#define UHCI_TD_LINK_TERM 0x00000001
#define UHCI_TD_LINK_DEPTH 0x00000004

typedef struct __attribute__((packed, aligned(16))) {
    uint32_t link;
    uint32_t status;
    uint32_t token;
    uint32_t buffer;
    uint32_t reserved[4];
} uhci_td_t;

typedef struct __attribute__((packed, aligned(16))) {
    uint32_t head;
    uint32_t element;
} uhci_qh_t;

typedef struct __attribute__((packed)) {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_pkt_t;

#ifndef KERNEL_TEST
static uint32_t g_frame_list[1024] __attribute__((aligned(4096)));
static uhci_qh_t g_qh __attribute__((aligned(16)));
static uhci_td_t g_td __attribute__((aligned(16)));
static uhci_td_t g_ctrl_td[2] __attribute__((aligned(16)));
static usb_setup_pkt_t g_setup_pkt __attribute__((aligned(16)));
static uint8_t g_report_buf[8] __attribute__((aligned(16)));

static int g_tablet_present = 0;
static uint16_t g_io_base = 0;
static uint8_t g_toggle = 0;
static uint8_t g_dev_addr = 2;

static void pci_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t addr = pci_config_address(bus, slot, func, offset);
    __asm__ volatile ("outl %0, %1" : : "a"(addr), "Nd"((uint16_t)0xCF8));
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"((uint16_t)0xCFC));
}

static int uhci_control_transfer(uint8_t dev_addr, uint8_t req_type, uint8_t req, uint16_t val, uint16_t idx) {
    g_setup_pkt.bmRequestType = req_type;
    g_setup_pkt.bRequest = req;
    g_setup_pkt.wValue = val;
    g_setup_pkt.wIndex = idx;
    g_setup_pkt.wLength = 0;

    g_ctrl_td[0].link = (uint32_t)(uint32_t)&g_ctrl_td[1] | UHCI_TD_LINK_DEPTH;
    g_ctrl_td[0].status = 0x80000000 | (3 << 27);
    g_ctrl_td[0].token = (7 << 21) | (0x00 << 19) | (0 << 15) | ((uint32_t)dev_addr << 8) | 0x2D;
    g_ctrl_td[0].buffer = (uint32_t)(uint32_t)&g_setup_pkt;

    g_ctrl_td[1].link = UHCI_TD_LINK_TERM;
    g_ctrl_td[1].status = 0x80000000 | (3 << 27);
    g_ctrl_td[1].token = (0x7FF << 21) | (0x01 << 19) | (0 << 15) | ((uint32_t)dev_addr << 8) | 0x69;
    g_ctrl_td[1].buffer = 0;

    g_qh.element = (uint32_t)(uint32_t)&g_ctrl_td[0];

    for (int timeout = 0; timeout < 50; timeout++) {
        if (!(g_ctrl_td[1].status & 0x80000000)) {
            uint32_t err = g_ctrl_td[1].status & 0x007E0000;
            return (err == 0) ? 0 : -1;
        }
        for (volatile int d = 0; d < 100; d++);
    }
    return -2;
}

void usb_tablet_init(void) {
    pci_device_t dev;
    int rc;
    int i;

    g_tablet_present = 0;
    rc = pci_find_device(0x8086, 0x7020, &dev);
    if (rc != 0) {
        rc = pci_find_class(0x0C, 0x03, &dev);
    }
    if (rc != 0) {
        print_string_serial("USB Tablet: Controller UHCI non trouve\n");
        return;
    }

    uint32_t bar4 = pci_config_read32(dev.bus, dev.slot, dev.function, 0x20);
    if (!(bar4 & 1)) {
        print_string_serial("USB Tablet: BAR4 non-IO\n");
        return;
    }
    g_io_base = (uint16_t)(bar4 & 0xFFFE);

    uint32_t pci_cmd = pci_config_read32(dev.bus, dev.slot, dev.function, 0x04);
    pci_cmd |= 0x05;
    pci_write32(dev.bus, dev.slot, dev.function, 0x04, pci_cmd);

    outw(g_io_base + UHCI_CMD, UHCI_CMD_HCRESET);
    for (volatile int d = 0; d < 1000; d++);
    outw(g_io_base + UHCI_CMD, 0);
    outw(g_io_base + UHCI_STS, 0xFFFF);

    uint16_t p1 = inw(g_io_base + UHCI_PORTSC1);
    uint16_t p2 = inw(g_io_base + UHCI_PORTSC2);

    if (!(p1 & 0x0001) && !(p2 & 0x0001)) {
        print_string_serial("USB Tablet: Aucun peripherique USB detecte sur les ports UHCI\n");
        return;
    }

    if (p1 & 0x0001) {
        outw(g_io_base + UHCI_PORTSC1, 0x0200);
        for (volatile int d = 0; d < 2000; d++);
        outw(g_io_base + UHCI_PORTSC1, 0x0004);
    }
    if (p2 & 0x0001) {
        outw(g_io_base + UHCI_PORTSC2, 0x0200);
        for (volatile int d = 0; d < 2000; d++);
        outw(g_io_base + UHCI_PORTSC2, 0x0004);
    }

    g_qh.head = UHCI_TD_LINK_TERM;
    g_qh.element = UHCI_TD_LINK_TERM;

    for (i = 0; i < 1024; i++) {
        g_frame_list[i] = (uint32_t)(uint32_t)&g_qh | 0x02;
    }

    outl(g_io_base + UHCI_FLBASEADD, (uint32_t)(uint32_t)g_frame_list);
    outw(g_io_base + UHCI_FRNUM, 0);
    outw(g_io_base + UHCI_CMD, UHCI_CMD_RUN | UHCI_CMD_MAXP);

    // Enumerate USB Tablet with short timeouts
    int r1 = uhci_control_transfer(0, 0x00, 0x05, g_dev_addr, 0);
    int r2 = uhci_control_transfer(g_dev_addr, 0x00, 0x09, 1, 0);

    if (r1 != 0 || r2 != 0) {
        print_string_serial("USB Tablet: Enumeration non terminee, fallback PS/2\n");
        return;
    }

    for (i = 0; i < 8; i++) g_report_buf[i] = 0;

    g_td.link = UHCI_TD_LINK_TERM;
    g_td.status = 0x80000000 | (3 << 27);
    g_td.token = (7 << 21) | (0x00 << 19) | (1 << 15) | ((uint32_t)g_dev_addr << 8) | 0x69;
    g_td.buffer = (uint32_t)(uint32_t)g_report_buf;

    g_qh.element = (uint32_t)(uint32_t)&g_td;

    g_tablet_present = 1;
    print_string_serial("USB Tablet: Controller UHCI initialise et enumere\n");
}

int usb_tablet_present(void) {
    return g_tablet_present;
}

void usb_tablet_poll(void) {
    if (!g_tablet_present || !g_io_base) return;

    uint32_t st = g_td.status;
    if (st & 0x80000000) return; // Still active

    uint32_t err_bits = st & 0x007E0000;
    uint32_t actual_len = (st + 1) & 0x07FF;

    if (err_bits == 0 && actual_len >= 5 && actual_len <= 8) {
        uint8_t buttons = g_report_buf[0] & 0x07;
        uint16_t rx = (uint16_t)g_report_buf[1] | ((uint16_t)g_report_buf[2] << 8);
        uint16_t ry = (uint16_t)g_report_buf[3] | ((uint16_t)g_report_buf[4] << 8);

        int w = gfx_fb_width();
        int h = gfx_fb_height();
        if (w < 100) w = GFX_FB_WIDTH;
        if (h < 100) h = GFX_FB_HEIGHT;

        int sx = ((uint32_t)rx * (uint32_t)w) / 32767u;
        int sy = ((uint32_t)ry * (uint32_t)h) / 32767u;

        if (sx >= w) sx = w - 1;
        if (sy >= h) sy = h - 1;

        gfx_desktop_set_mouse(sx, sy, buttons);
    }

    g_toggle ^= 1;
    g_td.status = 0x80000000 | (3 << 27);
    g_td.token = (7 << 21) | ((uint32_t)g_toggle << 19) | (1 << 15) | ((uint32_t)g_dev_addr << 8) | 0x69;
    g_qh.element = (uint32_t)(uint32_t)&g_td;
}
#else
void usb_tablet_init(void) {}
int usb_tablet_present(void) { return 0; }
void usb_tablet_poll(void) {}
#endif
