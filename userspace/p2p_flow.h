/* p2p_flow.h - Lot 69 (US-068/US-073): one flow per peer.
 * Every user data datagram (message, KV put) sent to a peer is recorded
 * by its sealed counter (the per-peer tx_counter already in the frame
 * header, so the data format does not change). The receiver answers with
 * DACK frames listing the counters it accepted (batched, up to
 * P2P_FLOW_ACK_BATCH per frame). Each ack gives an RTT and a delivery
 * rate sample (bytes delivered between the send and the ack of that
 * datagram, as in BBR); a datagram not acked within the loss timeout is a
 * loss. The peer's own pacer (token bucket, p2p_pace.c) then runs at
 * gain x the windowed maximum of those samples, backed off on loss, so a
 * slow peer no longer holds the node-wide rate of the fast ones. Before
 * the first sample the flow only bounds the datagrams in flight. Integer
 * code (32-bit division only), rates in bytes/s as in p2p_pace, no heap, host tested (tests/unit/userspace/test_p2p_flow.c). */
#ifndef P2P_FLOW_H
#define P2P_FLOW_H
#include <stdint.h>
#include "p2p_pace.h"

#define P2P_FLOW_INFLIGHT 32U        /* datagrams tracked per peer */
#define P2P_FLOW_INIT_WINDOW 8U      /* in flight before the first sample */
#define P2P_FLOW_SAMPLES 8U          /* windowed max filter length */
#define P2P_FLOW_ACK_BATCH 8U        /* counters per DACK frame */
#define P2P_FLOW_LOSS_MIN_US 400000U /* loss timeout bounds */
#define P2P_FLOW_LOSS_MAX_US 3000000U
#define P2P_FLOW_GAIN_NUM 5U         /* pacing gain 5/4 */
#define P2P_FLOW_GAIN_DEN 4U

typedef struct {
    uint32_t ctr;           /* sealed counter of the datagram */
    uint32_t sent_us;
    uint32_t delivered_at;  /* flow delivered bytes when it left */
    uint16_t bytes;
    uint8_t used;
} p2p_flow_pkt_t;

typedef struct {
    p2p_flow_pkt_t pkt[P2P_FLOW_INFLIGHT];
    uint32_t sent, acked, lost, dup_acks, unknown_acks, window_full;
    uint32_t inflight;
    uint32_t delivered;              /* bytes acked so far */
    uint32_t sample[P2P_FLOW_SAMPLES];
    uint32_t nsample, sample_pos;
    uint32_t btl_bps;                /* windowed max delivery rate */
    uint32_t srtt_us, min_rtt_us;
    uint32_t loss_backoffs;
    uint32_t cap_bps;                /* after a loss: 7/8 of the rate then */
    p2p_pace_t pace;                 /* this peer's token bucket */
    /* receiver side: counters accepted and not yet acknowledged */
    uint32_t ack_pend[P2P_FLOW_ACK_BATCH];
    uint32_t nack_pend;
    uint32_t acks_sent, ack_frames;
} p2p_flow_t;

void p2p_flow_init(p2p_flow_t* f);
/* 1 when a datagram of `bytes` may leave now (per-peer pacer or window) */
int p2p_flow_admit(p2p_flow_t* f, uint32_t bytes, uint32_t now_us);
/* records a data datagram that left with sealed counter ctr; 0 or -1 */
int p2p_flow_on_send(p2p_flow_t* f, uint32_t ctr, uint32_t bytes, uint32_t now_us);
/* an ack of ctr: 0 sample taken, 1 duplicate (already acked or lost), -1 unknown */
int p2p_flow_on_ack(p2p_flow_t* f, uint32_t ctr, uint32_t now_us);
/* retires datagrams older than the loss timeout; returns the losses */
int p2p_flow_expire(p2p_flow_t* f, uint32_t now_us);
/* Lot 71: same, and writes up to cap lost counters to lost (may be 0) */
int p2p_flow_expire_list(p2p_flow_t* f, uint32_t now_us, uint32_t* lost, int cap);
uint32_t p2p_flow_loss_timeout_us(const p2p_flow_t* f);
/* pacing rate, 0 while there is no sample */
uint32_t p2p_flow_rate(const p2p_flow_t* f);
/* receiver: note an accepted data counter; 1 when a DACK should go now */
int p2p_flow_note_rx(p2p_flow_t* f, uint32_t ctr);
/* receiver: writes a DACK body (count byte + 4-byte counters); length */
int p2p_flow_take_acks(p2p_flow_t* f, uint8_t* body, int cap);
/* sender: applies a DACK body; returns the counters applied or -1 */
int p2p_flow_apply_acks(p2p_flow_t* f, const uint8_t* body, int len, uint32_t now_us);
/* "sent S acked A lost L inflight I rate_bps R btl_bps B srtt_ms M ..." */
void p2p_flow_status(const p2p_flow_t* f, char* out, int cap);
#endif
