#ifndef MOHHDY_USB_DMA_H
#define MOHHDY_USB_DMA_H

/* Lot 8 (US-001): bounded DMA for the Ring 3 UHCI driver. Pure logic (no
 * port I/O, no paging) so the host unit tests cover every check; the glue
 * in syscall.c and usb_hc.c applies it. The controller only reads what the
 * kernel wrote with these helpers: a TD buffer pointer always lies inside
 * the driver's data page and a link always points at a TD of the schedule
 * page (or terminates). */
#include <stdint.h>
#include "../include/os_syscalls.h"

#define USBDMA_LINK_TERM 0x00000001U
#define USBDMA_LINK_QH 0x00000002U
#define USBDMA_LINK_DEPTH 0x00000004U

#define USBDMA_OK 0
#define USBDMA_E_SLOT (-1)
#define USBDMA_E_PID (-2)
#define USBDMA_E_ADDR (-3)
#define USBDMA_E_BOUNDS (-4)
#define USBDMA_E_LINK (-5)
#define USBDMA_E_CYCLE (-6)

/* UHCI register offsets. */
#define USBDMA_REG_CMD 0x00U
#define USBDMA_REG_STS 0x02U
#define USBDMA_REG_INTR 0x04U
#define USBDMA_REG_FRNUM 0x06U
#define USBDMA_REG_FLBASE 0x08U
#define USBDMA_REG_SOF 0x0CU
#define USBDMA_REG_PORTSC1 0x10U
#define USBDMA_REG_PORTSC2 0x12U
#define USBDMA_IO_SPAN 0x20U

/* Validates a driver request and fills the TD the controller will read. */
int usbdma_build_td(const os_usb_td_req_t* req, uint32_t data_phys, uint32_t sched_phys, os_usb_td_t* out);
/* Walks the chain from `first` in the schedule `s` (physical base
 * sched_phys): every link inside the schedule, every buffer inside the
 * data page, no slot twice. Returns the slot count or a USBDMA_E_* code. */
int usbdma_chain_check(const os_usb_sched_t* s, uint32_t sched_phys, uint32_t data_phys, uint8_t first);
/* 1 when the driver may touch base + offset (STS, INTR, FRNUM, PORTSC1/2). */
int usbdma_port_allowed(uint16_t offset);
/* Fills the frame list (1024 entries) so every frame points at the QH. */
void usbdma_frame_list_init(uint32_t* frame_list, uint32_t qh_phys);
/* Scales a 0..32767 absolute report to a w x h screen. */
void usbdma_scale(uint32_t x, uint32_t y, int w, int h, int* sx, int* sy);

#endif
