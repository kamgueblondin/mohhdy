/* p2p_rtx.h - Lot 71 (US-068/US-073): retransmission of lost user data.
 * The sender keeps a bounded copy of each user data datagram (message, KV
 * put) by its sealed counter until its DACK comes back (p2p_flow.h). When
 * p2p_flow declares the counter lost, the copy leaves again as an RTX
 * inner frame under a new sealed counter: body = original counter (4
 * bytes, big endian), original inner type (1 byte), original body. A copy
 * goes out at most P2P_RTX_TRIES times, then it is given up and counted.
 * The receiver keeps a sliding window of the original counters it has
 * delivered, so a retransmission whose original did arrive (only the ack
 * was lost) is dropped instead of delivered twice. Bodies longer than
 * P2P_RTX_BODY are not kept (counted as too_big). Static memory, no heap,
 * integer code, host tested (tests/unit/userspace/test_p2p_rtx.c) and
 * built unchanged for ARM (arch/arm/arm_portable.c). */
#ifndef P2P_RTX_H
#define P2P_RTX_H
#include <stdint.h>

#define P2P_RTX_SLOTS 8U
#define P2P_RTX_BODY 256U
#define P2P_RTX_TRIES 3U
#define P2P_RTX_HDR 5U       /* original counter + original inner type */
#define P2P_RTX_WINDOW 256U  /* receiver dedup window, in sealed counters */
#define P2P_RTX_WORDS (P2P_RTX_WINDOW / 32U)

typedef struct {
    uint32_t ctr;            /* sealed counter of the latest copy on the wire */
    uint32_t orig;           /* sealed counter of the first send */
    uint32_t age;            /* insertion order, for eviction */
    uint16_t len;
    uint8_t inner, tries, used;
    uint8_t body[P2P_RTX_BODY];
} p2p_rtx_slot_t;

typedef struct {
    p2p_rtx_slot_t s[P2P_RTX_SLOTS];
    uint32_t clock;
    /* sender */
    uint32_t kept, acked, resent, gave_up, evicted, too_big;
    /* receiver: highest original counter delivered and the window below
     * it (bit d of the mask: seen_hi - d was delivered). Pings, acks and
     * other sealed frames share the counter, hence a wide window. */
    uint32_t seen_hi;
    uint32_t seen_mask[P2P_RTX_WORDS];
    uint32_t delivered, dup_rx, old_rx;
} p2p_rtx_t;

void p2p_rtx_init(p2p_rtx_t* r);
/* keeps a copy of a datagram sent with counter ctr: 0 kept, -1 too big.
 * A full table evicts its oldest copy (counted). */
int p2p_rtx_keep(p2p_rtx_t* r, uint32_t ctr, uint8_t inner, const uint8_t* body, int len);
/* the datagram with counter ctr was acknowledged: 1 copy released, 0 none */
int p2p_rtx_acked(p2p_rtx_t* r, uint32_t ctr);
/* the datagram with counter ctr was lost: slot index to send again, or -1
 * (no copy, or tries used up: the copy is given up) */
int p2p_rtx_lost(p2p_rtx_t* r, uint32_t ctr);
/* writes the RTX body of slot idx; returns its length or -1 */
int p2p_rtx_frame(const p2p_rtx_t* r, int idx, uint8_t* out, int cap);
/* the copy of slot idx left again with counter ctr */
void p2p_rtx_resent(p2p_rtx_t* r, int idx, uint32_t ctr);
/* receiver: a data datagram with original counter orig arrived; 1 deliver
 * it (first time), 0 drop it (already delivered or older than the window) */
int p2p_rtx_accept(p2p_rtx_t* r, uint32_t orig);
/* receiver: parses an RTX body; 0 ok (orig, inner, body, len set), -1 bad */
int p2p_rtx_parse(const uint8_t* m, int len, uint32_t* orig, uint8_t* inner, const uint8_t** body, int* blen);
/* "kept K acked A resent R gave_up G evicted E too_big T pending P delivered D dup_rx X old_rx O" */
void p2p_rtx_status(const p2p_rtx_t* r, char* out, int cap);
#endif
