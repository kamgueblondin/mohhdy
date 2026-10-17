#ifndef USB_TABLET_H
#define USB_TABLET_H

#include <stdint.h>
#include <stddef.h>
#include "../../include/os_syscalls.h"

/* Lot 8 (US-001): the USB tablet is driven by the Ring 3 usbdriver. The
 * kernel keeps only the UHCI host controller glue: the boot probe (find,
 * enable I/O + bus mastering, reset, stop) and the bounded DMA schedule the
 * driver fills through SYS_DEVICE_SERVICE (see usb_dma.h). */
void usb_tablet_init(void);           /* boot probe, no enumeration */
int usb_tablet_present(void);         /* a Ring 3 usbdriver owns a tablet */
int usb_hc_present(void);
uint16_t usb_hc_io_base(void);
/* Claim: stop, clear the schedule and the data page, map the data page
 * writable at OS_USB_DMA_WINDOW and the schedule read-only at
 * OS_USB_SCHED_WINDOW in `dir`. 0 or <0. */
int usb_hc_claim(void* dir);
int usb_hc_td(const os_usb_td_req_t* req);
int usb_hc_run(uint8_t first, uint8_t watch);
/* IRQ0: 1 when the watched TD just finished. */
int usb_hc_watch_done(void);
/* Owner lost: stop the controller (no DMA after death), unlink the chain. */
void usb_hc_stop(void);
void usb_hc_report(uint32_t x, uint32_t y, uint32_t buttons);

#endif
