/* Lot 8 (US-001): UHCI host controller glue. The tablet driver itself
 * (enumeration, report parsing, data toggles) runs in Ring 3
 * (userspace/usb_driver.c). The kernel finds the controller at boot, then
 * only writes the DMA structures the controller reads, after usb_dma.c
 * validated them, and runs or stops the controller. */
#include "usb_tablet.h"
#include "../pci.h"
#include "../gfx_fb.h"
#include "../gfx_desktop.h"
#include "../usb_dma.h"
#include "../device_service.h"
#include <stdint.h>
#include <stddef.h>

extern void print_string_serial(const char* str);

#ifndef KERNEL_TEST
#include "../mem/vmm.h"

static inline void outw(uint16_t port, uint16_t val) { __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port)); }
static inline uint16_t inw(uint16_t port) { uint16_t v; __asm__ volatile ("inw %1, %0" : "=a"(v) : "Nd"(port)); return v; }
static inline void outl(uint16_t port, uint32_t val) { __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port)); }

#define UHCI_CMD_RUN 0x0001U
#define UHCI_CMD_HCRESET 0x0002U
#define UHCI_CMD_MAXP 0x0080U
#define UHCI_STS_HALTED 0x0020U

static uint32_t g_frame_list[1024] __attribute__((aligned(4096)));
/* A whole page each: the schedule page is mapped read-only in the driver,
 * so nothing else of the kernel may share it. */
static union { os_usb_sched_t s; uint8_t page[4096]; } g_sched_page __attribute__((aligned(4096)));
#define g_sched (g_sched_page.s)
static uint8_t g_data[OS_USB_DATA_SIZE] __attribute__((aligned(4096)));
static uint16_t g_io_base;
static uint8_t g_watch = OS_USB_TD_END;

#define SCHED_PHYS ((uint32_t)&g_sched)
#define DATA_PHYS ((uint32_t)g_data)

static void pci_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t addr = pci_config_address(bus, slot, func, offset);
    __asm__ volatile ("outl %0, %1" : : "a"(addr), "Nd"((uint16_t)0xCF8));
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"((uint16_t)0xCFC));
}

static void sched_clear(void) {
    uint32_t i;
    for (i = 0U; i < sizeof(g_sched_page.page); i++) g_sched_page.page[i] = 0U;
    for (i = 0U; i < OS_USB_TD_SLOTS; i++) g_sched.td[i].link = USBDMA_LINK_TERM;
    g_sched.head = USBDMA_LINK_TERM;
    g_sched.element = USBDMA_LINK_TERM;
    g_watch = OS_USB_TD_END;
}

static void hc_halt(void) {
    uint32_t spin;
    if (!g_io_base) return;
    outw((uint16_t)(g_io_base + USBDMA_REG_CMD), 0U);
    for (spin = 0U; spin < 100000U; spin++)
        if (inw((uint16_t)(g_io_base + USBDMA_REG_STS)) & UHCI_STS_HALTED) break;
    g_sched.element = USBDMA_LINK_TERM;
}

void usb_tablet_init(void) {
    pci_device_t dev;
    uint32_t bar4, cmd;
    volatile uint32_t d;
    int rc;
    devsvc_usb_set_present(0);
    rc = pci_find_device(0x8086, 0x7020, &dev);
    if (rc != 0) rc = pci_find_class_progif(0x0C, 0x03, 0x00, &dev);
    if (rc != 0) { print_string_serial("[USB] no UHCI controller\n"); return; }
    bar4 = pci_config_read32(dev.bus, dev.slot, dev.function, 0x20);
    if (!(bar4 & 1U)) { print_string_serial("[USB] UHCI BAR4 is not I/O\n"); return; }
    g_io_base = (uint16_t)(bar4 & 0xFFE0U);
    cmd = pci_config_read32(dev.bus, dev.slot, dev.function, 0x04);
    pci_write32(dev.bus, dev.slot, dev.function, 0x04, cmd | 0x05U);   /* I/O + bus master */
    outw((uint16_t)(g_io_base + USBDMA_REG_CMD), UHCI_CMD_HCRESET);
    for (d = 0; d < 1000U; d++) { }
    outw((uint16_t)(g_io_base + USBDMA_REG_CMD), 0U);
    outw((uint16_t)(g_io_base + USBDMA_REG_STS), 0xFFFFU);
    sched_clear();
    usbdma_frame_list_init(g_frame_list, SCHED_PHYS);
    outl((uint16_t)(g_io_base + USBDMA_REG_FLBASE), (uint32_t)g_frame_list);
    outw((uint16_t)(g_io_base + USBDMA_REG_FRNUM), 0U);
    devsvc_usb_set_present(1);
    print_string_serial("[USB] UHCI controller found, stopped; tablet left to the Ring 3 usbdriver\n");
}

int usb_hc_present(void) { return devsvc_usb_present(); }
uint16_t usb_hc_io_base(void) { return g_io_base; }
int usb_tablet_present(void) { return devsvc_owner(DEV_USB) > 0; }

int usb_hc_claim(void* dir) {
    uint32_t i;
    if (!g_io_base || !dir) return -1;
    hc_halt();
    sched_clear();
    for (i = 0U; i < OS_USB_DATA_SIZE; i++) g_data[i] = 0U;
    if (vmm_map_borrowed_user_page_rw((vmm_directory_t*)dir, (void*)DATA_PHYS, (void*)OS_USB_DMA_WINDOW) != 0)
        return -2;
    if (vmm_map_borrowed_user_page((vmm_directory_t*)dir, (void*)SCHED_PHYS, (void*)OS_USB_SCHED_WINDOW) != 0)
        return -3;
    return 0;
}

int usb_hc_td(const os_usb_td_req_t* req) {
    os_usb_td_t td;
    os_usb_td_t* slot;
    int rc = usbdma_build_td(req, DATA_PHYS, SCHED_PHYS, &td);
    devsvc_usb_note_td(rc == USBDMA_OK);
    if (rc != USBDMA_OK) return rc;
    slot = &g_sched.td[req->slot];
    slot->status = 0U;                 /* inactive while it is rewritten */
    slot->link = td.link;
    slot->token = td.token;
    slot->buffer = td.buffer;
    slot->reserved[0] = slot->reserved[1] = slot->reserved[2] = slot->reserved[3] = 0U;
    slot->status = td.status;          /* active last */
    return 0;
}

int usb_hc_run(uint8_t first, uint8_t watch) {
    int rc;
    if (!g_io_base) return -1;
    if (first == OS_USB_TD_END) { g_sched.element = USBDMA_LINK_TERM; g_watch = OS_USB_TD_END; return 0; }
    rc = usbdma_chain_check(&g_sched, SCHED_PHYS, DATA_PHYS, first);
    if (rc <= 0 || watch >= OS_USB_TD_SLOTS) { devsvc_usb_note_td(0); return rc < 0 ? rc : USBDMA_E_SLOT; }
    g_watch = watch;
    devsvc_usb_set_done(0);
    g_sched.element = SCHED_PHYS + 16U + (uint32_t)first * (uint32_t)sizeof(os_usb_td_t);
    outl((uint16_t)(g_io_base + USBDMA_REG_FLBASE), (uint32_t)g_frame_list);
    outw((uint16_t)(g_io_base + USBDMA_REG_CMD), UHCI_CMD_RUN | UHCI_CMD_MAXP);
    return rc;
}

int usb_hc_watch_done(void) {
    if (g_watch >= OS_USB_TD_SLOTS || devsvc_usb_done()) return 0;
    if (g_sched.td[g_watch].status & OS_USB_TD_ACTIVE) return 0;
    devsvc_usb_set_done(1);
    return devsvc_usb_done();
}

void usb_hc_stop(void) {
    hc_halt();
    sched_clear();
    devsvc_usb_note_stop();
}

void usb_hc_report(uint32_t x, uint32_t y, uint32_t buttons) {
    int w = gfx_fb_width(), h = gfx_fb_height(), sx, sy;
    if (w < 100) w = GFX_FB_WIDTH;
    if (h < 100) h = GFX_FB_HEIGHT;
    usbdma_scale(x, y, w, h, &sx, &sy);
    gfx_desktop_set_mouse(sx, sy, (uint8_t)(buttons & 7U));
    devsvc_usb_note_report();
}
#else
void usb_tablet_init(void) {}
int usb_tablet_present(void) { return 0; }
#endif
