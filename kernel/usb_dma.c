#include "usb_dma.h"

static uint32_t td_phys(uint32_t sched_phys, uint8_t slot) {
    return sched_phys + 16U + (uint32_t)slot * (uint32_t)sizeof(os_usb_td_t);
}

int usbdma_build_td(const os_usb_td_req_t* req, uint32_t data_phys, uint32_t sched_phys, os_usb_td_t* out) {
    uint32_t maxlen, i;
    if (!req || !out) return USBDMA_E_SLOT;
    if (req->slot >= OS_USB_TD_SLOTS) return USBDMA_E_SLOT;
    if (req->pid != OS_USB_PID_SETUP && req->pid != OS_USB_PID_IN && req->pid != OS_USB_PID_OUT)
        return USBDMA_E_PID;
    if (req->dev_addr > 127U || req->endpoint > 15U || req->toggle > 1U || req->low_speed > 1U)
        return USBDMA_E_ADDR;
    if (req->length > OS_USB_TD_MAX_LEN || (uint32_t)req->buf_offset + req->length > OS_USB_DATA_SIZE)
        return USBDMA_E_BOUNDS;
    if (req->next != OS_USB_TD_END && (req->next >= OS_USB_TD_SLOTS || req->next == req->slot))
        return USBDMA_E_LINK;
    maxlen = req->length ? (uint32_t)req->length - 1U : 0x7FFU;
    out->link = req->next == OS_USB_TD_END ? USBDMA_LINK_TERM : (td_phys(sched_phys, req->next) | USBDMA_LINK_DEPTH);
    out->status = OS_USB_TD_ACTIVE | (3U << 27) | (req->low_speed ? (1U << 26) : 0U);
    out->token = (maxlen << 21) | ((uint32_t)req->toggle << 19) | ((uint32_t)req->endpoint << 15) |
                 ((uint32_t)req->dev_addr << 8) | req->pid;
    out->buffer = req->length ? data_phys + req->buf_offset : 0U;
    for (i = 0U; i < 4U; i++) out->reserved[i] = 0U;
    return USBDMA_OK;
}

int usbdma_chain_check(const os_usb_sched_t* s, uint32_t sched_phys, uint32_t data_phys, uint8_t first) {
    uint8_t seen[OS_USB_TD_SLOTS];
    uint32_t i, count = 0U;
    uint8_t slot = first;
    if (!s) return USBDMA_E_SLOT;
    if (first == OS_USB_TD_END) return 0;
    for (i = 0U; i < OS_USB_TD_SLOTS; i++) seen[i] = 0U;
    for (;;) {
        const os_usb_td_t* td;
        uint32_t link, len, maxlen;
        if (slot >= OS_USB_TD_SLOTS) return USBDMA_E_SLOT;
        if (seen[slot]) return USBDMA_E_CYCLE;
        seen[slot] = 1U;
        count++;
        td = &s->td[slot];
        maxlen = td->token >> 21;
        len = maxlen == 0x7FFU ? 0U : maxlen + 1U;
        if (len > OS_USB_TD_MAX_LEN) return USBDMA_E_BOUNDS;
        if (len && (td->buffer < data_phys || td->buffer - data_phys + len > OS_USB_DATA_SIZE))
            return USBDMA_E_BOUNDS;
        link = td->link;
        if (link & USBDMA_LINK_TERM) return (int)count;
        if (link & USBDMA_LINK_QH) return USBDMA_E_LINK;
        link &= ~0xFU;
        if (link < td_phys(sched_phys, 0U) || link > td_phys(sched_phys, OS_USB_TD_SLOTS - 1U) ||
            (link - td_phys(sched_phys, 0U)) % (uint32_t)sizeof(os_usb_td_t) != 0U)
            return USBDMA_E_LINK;
        slot = (uint8_t)((link - td_phys(sched_phys, 0U)) / (uint32_t)sizeof(os_usb_td_t));
    }
}

int usbdma_port_allowed(uint16_t offset) {
    switch (offset) {
    case USBDMA_REG_STS: case USBDMA_REG_STS + 1U:
    case USBDMA_REG_INTR: case USBDMA_REG_INTR + 1U:
    case USBDMA_REG_FRNUM: case USBDMA_REG_FRNUM + 1U:
    case USBDMA_REG_PORTSC1: case USBDMA_REG_PORTSC1 + 1U:
    case USBDMA_REG_PORTSC2: case USBDMA_REG_PORTSC2 + 1U:
        return 1;
    default:
        return 0;
    }
}

void usbdma_frame_list_init(uint32_t* frame_list, uint32_t qh_phys) {
    uint32_t i;
    if (!frame_list) return;
    for (i = 0U; i < 1024U; i++) frame_list[i] = qh_phys | USBDMA_LINK_QH;
}

void usbdma_scale(uint32_t x, uint32_t y, int w, int h, int* sx, int* sy) {
    int ox, oy;
    if (x > 32767U) x = 32767U;
    if (y > 32767U) y = 32767U;
    ox = (int)((x * (uint32_t)w) / 32767U);
    oy = (int)((y * (uint32_t)h) / 32767U);
    if (ox >= w) ox = w - 1;
    if (oy >= h) oy = h - 1;
    if (sx) *sx = ox;
    if (sy) *sy = oy;
}
