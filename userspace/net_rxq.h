/* net_rxq.h - Lot 90 (Lot 89 gap 6): a frame FIFO between the NE2000
 * receive ring and the Ring 3 network stack.
 *
 * The Ring 3 net worker used to read the card only while a relay op ran,
 * so a burst that arrived between two ops could fill the card's 26 page
 * ring (about four full frames) and QEMU then wrote over unread packets.
 * The worker now drains the card into this FIFO when it has nothing else
 * to do (no relay request waiting, no LLM session in progress), and the
 * stack's poll serves the FIFO first, then the card, so the order of the
 * frames is kept. A full FIFO stops the drain (the rest waits on the card
 * as before); nothing is cut. Byte ring of frames, each "length (2 bytes,
 * little endian) + bytes"; no heap. Kernel, ABI and rights unchanged. */
#ifndef NET_RXQ_H
#define NET_RXQ_H
#include <stdint.h>

#define NET_RXQ_FRAME_MAX 1536U          /* OS_NET_NIC_FRAME_MAX */
#define NET_RXQ_BYTES (16U * (NET_RXQ_FRAME_MAX + 2U))
#define NET_RXQ_IDLE_MAX 8U              /* frames moved per idle round */

typedef struct {
    uint8_t* buf;
    uint32_t cap;
    uint32_t head, tail, used;   /* bytes; head = next read, tail = next write */
    uint32_t frames;             /* frames held now */
    uint32_t pushed, popped;     /* frames in and out */
    uint32_t full;               /* frames refused because the FIFO was full */
    uint32_t oversize;           /* frames dropped: longer than the reader's buffer */
    uint32_t peak;               /* most frames held at once */
    uint32_t drains;             /* drain calls that moved at least one frame */
} net_rxq_t;

/* Reads one frame from the card: 0 and *length set, nonzero when none. */
typedef int (*net_rxq_poll_t)(void* context, uint8_t* frame, uint16_t capacity, uint16_t* length);

/* buf of cap bytes (at least one frame of NET_RXQ_FRAME_MAX + 2). 0, or -1. */
int net_rxq_init(net_rxq_t* q, uint8_t* buf, uint32_t cap);
/* 0 stored, -1 refused (no room, an empty or too long frame; counted in
 * full when there was no room). */
int net_rxq_push(net_rxq_t* q, const uint8_t* frame, uint16_t length);
/* 0 and the oldest frame in frame / *length; -1 empty. A frame longer
 * than capacity is dropped (counted in oversize) and the next one tried. */
int net_rxq_pop(net_rxq_t* q, uint8_t* frame, uint16_t capacity, uint16_t* length);
/* Lot 90: every reader of the card goes through this: the oldest frame
 * of the FIFO, else one from poll (0 and *length set), -1 when neither
 * has one. A reader that polled the card directly would skip the frames
 * the idle drain moved into the FIFO (they would wait for another reader,
 * out of order, or forever). */
int net_rxq_read(net_rxq_t* q, net_rxq_poll_t poll, void* context, uint8_t* frame, uint16_t capacity,
                 uint16_t* length);
/* Room for one more frame of length bytes: 1 or 0. */
int net_rxq_room(const net_rxq_t* q, uint16_t length);
/* Moves up to max frames from poll into the FIFO while there is room for
 * a full frame; returns the frames moved. scratch holds one frame. */
uint32_t net_rxq_drain(net_rxq_t* q, net_rxq_poll_t poll, void* context, uint8_t* scratch, uint32_t max);
/* "rxq held H peak P in I out O full F oversize Z drains D" (no newline);
 * length, or -1 when out is too small. */
int net_rxq_text(const net_rxq_t* q, char* out, int cap);
#endif
