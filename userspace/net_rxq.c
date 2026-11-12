/* net_rxq.c - Lot 90: see net_rxq.h. */
#include "net_rxq.h"

int net_rxq_init(net_rxq_t* q, uint8_t* buf, uint32_t cap) {
    uint8_t* p;
    uint32_t k;
    if (!q || !buf || cap < NET_RXQ_FRAME_MAX + 2U) return -1;
    p = (uint8_t*)q;
    for (k = 0U; k < (uint32_t)sizeof(*q); k++) p[k] = 0U;
    q->buf = buf;
    q->cap = cap;
    return 0;
}

int net_rxq_room(const net_rxq_t* q, uint16_t length) {
    if (!q || !q->buf) return 0;
    return q->cap - q->used >= (uint32_t)length + 2U;
}

static void put_byte(net_rxq_t* q, uint8_t v) {
    q->buf[q->tail] = v;
    q->tail++;
    if (q->tail == q->cap) q->tail = 0U;
}

static uint8_t get_byte(net_rxq_t* q) {
    uint8_t v = q->buf[q->head];
    q->head++;
    if (q->head == q->cap) q->head = 0U;
    return v;
}

int net_rxq_push(net_rxq_t* q, const uint8_t* frame, uint16_t length) {
    uint16_t k;
    if (!q || !q->buf || !frame || length == 0U || length > NET_RXQ_FRAME_MAX) return -1;
    if (!net_rxq_room(q, length)) { q->full++; return -1; }
    put_byte(q, (uint8_t)(length & 0xffU));
    put_byte(q, (uint8_t)(length >> 8));
    for (k = 0U; k < length; k++) put_byte(q, frame[k]);
    q->used += (uint32_t)length + 2U;
    q->frames++;
    q->pushed++;
    if (q->frames > q->peak) q->peak = q->frames;
    return 0;
}

int net_rxq_pop(net_rxq_t* q, uint8_t* frame, uint16_t capacity, uint16_t* length) {
    if (!q || !q->buf || !frame || !length) return -1;
    while (q->frames) {
        uint16_t n, k;
        n = get_byte(q);
        n = (uint16_t)(n | ((uint16_t)get_byte(q) << 8));
        q->used -= (uint32_t)n + 2U;
        q->frames--;
        if (n > capacity) {
            /* skip it: the next frame may fit */
            for (k = 0U; k < n; k++) (void)get_byte(q);
            q->oversize++;
            continue;
        }
        for (k = 0U; k < n; k++) frame[k] = get_byte(q);
        *length = n;
        q->popped++;
        return 0;
    }
    return -1;
}

uint32_t net_rxq_drain(net_rxq_t* q, net_rxq_poll_t poll, void* context, uint8_t* scratch, uint32_t max) {
    uint32_t moved = 0U;
    uint16_t len;
    if (!q || !q->buf || !poll || !scratch) return 0U;
    /* only while a full frame fits: a frame read from the card cannot be
     * put back, so never read one the FIFO might refuse */
    while (moved < max && net_rxq_room(q, (uint16_t)NET_RXQ_FRAME_MAX)) {
        len = 0U;
        if (poll(context, scratch, (uint16_t)NET_RXQ_FRAME_MAX, &len) != 0) break;
        if (len == 0U || net_rxq_push(q, scratch, len) != 0) continue;
        moved++;
    }
    if (moved) q->drains++;
    return moved;
}

static int app(char* o, int cap, int p, const char* s) {
    while (p >= 0 && *s) { if (p >= cap - 1) return -1; o[p++] = *s++; }
    return p;
}

static int app_u(char* o, int cap, int p, uint32_t v) {
    char b[12];
    int n = 0;
    if (p < 0) return -1;
    do { b[n++] = (char)('0' + v % 10U); v /= 10U; } while (v && n < 11);
    while (n) { if (p >= cap - 1) return -1; o[p++] = b[--n]; }
    return p;
}

int net_rxq_text(const net_rxq_t* q, char* out, int cap) {
    int p = 0;
    if (!q || !out || cap < 2) return -1;
    p = app(out, cap, p, "rxq held ");
    p = app_u(out, cap, p, q->frames);
    p = app(out, cap, p, " peak ");
    p = app_u(out, cap, p, q->peak);
    p = app(out, cap, p, " in ");
    p = app_u(out, cap, p, q->pushed);
    p = app(out, cap, p, " out ");
    p = app_u(out, cap, p, q->popped);
    p = app(out, cap, p, " full ");
    p = app_u(out, cap, p, q->full);
    p = app(out, cap, p, " oversize ");
    p = app_u(out, cap, p, q->oversize);
    p = app(out, cap, p, " drains ");
    p = app_u(out, cap, p, q->drains);
    if (p < 0) { out[0] = 0; return -1; }
    out[p] = 0;
    return p;
}
