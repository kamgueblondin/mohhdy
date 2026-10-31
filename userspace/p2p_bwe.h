/* p2p_bwe.h - Lot 67 (US-068/US-073): bandwidth estimator and pacer for the
 * P2P mesh, model-based (the BBR idea, simplified and bounded):
 *  - delivery-rate samples per acknowledged packet: bytes delivered between
 *    the send of a packet and its ack, over the longer of the send and ack
 *    intervals (draft-cheng-iccrg-delivery-rate-estimation);
 *  - bottleneck bandwidth = windowed max of non app-limited samples over
 *    the last P2P_BWE_BW_ROUNDS round trips (max filter, 3 entries);
 *  - min RTT = windowed min over P2P_BWE_RTT_WIN_MS;
 *  - states STARTUP (gain 2.89 until the bandwidth stops growing by 25 %
 *    for 3 rounds), DRAIN, PROBE_BW (gain cycle 5/4, 3/4, 1 x6) and
 *    PROBE_RTT (window of 4 packets for 200 ms when min RTT is stale);
 *  - pacing rate = gain * bandwidth, congestion window = 2 * BDP (bytes);
 *  - a token-bucket pacer that says when a datagram may leave.
 * Integer arithmetic only (gains in 1/256), no heap, all state in the
 * struct. Time is milliseconds from the caller (p2p ticks or a test clock).
 * Honest limits: this estimates what the path delivered; it does not see
 * cross traffic directly and it is not a TCP implementation. */
#ifndef P2P_BWE_H
#define P2P_BWE_H
#include <stdint.h>

#define P2P_BWE_INFLIGHT 32U       /* packets tracked between send and ack */
#define P2P_BWE_BW_ROUNDS 10U      /* max filter window, in round trips */
#define P2P_BWE_RTT_WIN_MS 10000U  /* min RTT window */
#define P2P_BWE_PROBE_RTT_MS 200U
#define P2P_BWE_MIN_CWND 4U        /* packets */
#define P2P_BWE_GAIN_UNIT 256U

typedef enum {
    P2P_BWE_STARTUP = 0,
    P2P_BWE_DRAIN = 1,
    P2P_BWE_PROBE_BW = 2,
    P2P_BWE_PROBE_RTT = 3
} p2p_bwe_state_t;

typedef struct {
    uint32_t seq;
    uint32_t bytes;
    uint32_t sent_ms;
    uint64_t delivered;        /* total delivered when this one was sent */
    uint32_t delivered_ms;     /* time of that delivered count */
    uint32_t first_sent_ms;    /* send time of the packet acked then */
    uint8_t app_limited;
    uint8_t used;
} p2p_bwe_pkt_t;

typedef struct {
    uint32_t value;            /* bytes per second */
    uint32_t round;
} p2p_bwe_maxent_t;

typedef struct {
    /* configuration */
    uint32_t mss;              /* bytes per datagram (cwnd unit) */
    uint32_t initial_bps;      /* pacing before any sample */
    /* delivery accounting */
    uint64_t delivered;
    uint32_t delivered_ms;
    uint32_t first_sent_ms;
    uint32_t inflight_bytes;
    uint32_t app_limited_until; /* delivered count (low 32 bits) */
    p2p_bwe_pkt_t pkt[P2P_BWE_INFLIGHT];
    /* rounds */
    uint32_t round;
    uint64_t next_round_delivered;
    /* filters */
    p2p_bwe_maxent_t bw[3];
    uint32_t min_rtt_ms;
    uint32_t min_rtt_stamp;
    uint32_t last_rtt_ms;
    /* state machine */
    p2p_bwe_state_t state;
    uint32_t full_bw;
    uint32_t full_bw_count;
    uint8_t full_bw_reached;
    uint8_t cycle_idx;
    uint32_t cycle_stamp;
    uint32_t probe_rtt_done;
    uint8_t probe_rtt_round_done;
    uint32_t pacing_gain;      /* /256 */
    uint32_t cwnd_gain;        /* /256 */
    /* outputs */
    uint32_t pacing_bps;
    uint32_t cwnd_bytes;
    /* pacer */
    uint32_t pace_next_us;     /* next departure, microseconds */
    /* counters */
    uint32_t samples, samples_app_limited, losses, sent, acked, paced_wait;
    uint32_t last_sample_bps;
} p2p_bwe_t;

void p2p_bwe_init(p2p_bwe_t* b, uint32_t mss, uint32_t initial_bps, uint32_t now_ms);
/* A datagram of `bytes` leaves now with sequence `seq`. Returns 0, or -1
 * when the tracking table is full (the packet is then not sampled). */
int p2p_bwe_on_send(p2p_bwe_t* b, uint32_t seq, uint32_t bytes, uint32_t now_ms);
/* The sender has nothing more to send: the next samples are app limited
 * (they must not lower the bandwidth estimate). */
void p2p_bwe_on_app_limited(p2p_bwe_t* b);
/* Ack of `seq`. Returns the delivery-rate sample (bytes/s) or 0. */
uint32_t p2p_bwe_on_ack(p2p_bwe_t* b, uint32_t seq, uint32_t now_ms);
/* `seq` is declared lost (timeout). */
void p2p_bwe_on_loss(p2p_bwe_t* b, uint32_t seq, uint32_t now_ms);
/* A RTT sample from another source (PING/PONG). */
void p2p_bwe_on_rtt(p2p_bwe_t* b, uint32_t rtt_ms, uint32_t now_ms);
/* Pacer: 1 when `bytes` may be sent now (and they are charged), else 0. */
int p2p_bwe_can_send(p2p_bwe_t* b, uint32_t bytes, uint32_t now_ms);
uint32_t p2p_bwe_btlbw(const p2p_bwe_t* b);
uint32_t p2p_bwe_bdp(const p2p_bwe_t* b);
/* cwnd in datagrams, at least P2P_BWE_MIN_CWND */
uint32_t p2p_bwe_cwnd_pkts(const p2p_bwe_t* b);
const char* p2p_bwe_state_name(p2p_bwe_state_t s);
#endif
