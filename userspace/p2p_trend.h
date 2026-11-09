/* p2p_trend.h - Lot 87 (US-068/US-073): delay-gradient overuse detector
 * for the P2P probe train, next to the delivery-rate estimator (p2p_bwe).
 * The idea is the one of the Google congestion control draft
 * (draft-ietf-rmcat-gcc-02) and of its trendline filter:
 *  - probes are grouped by send time (bursts of P2P_TREND_BURST_MS);
 *  - for consecutive groups, the delay variation is the change of the
 *    arrival spacing against the send spacing; it is accumulated and
 *    smoothed (exponential, alpha 0.9);
 *  - a least-squares slope over the last P2P_TREND_WIN groups is the
 *    delay trend; it is scaled by the number of deltas (up to 60) and a
 *    gain of 4, then compared with an adaptive threshold gamma (12.5 ms
 *    at start, k_up 0.0087, k_down 0.039 per ms, clamped to 6..600);
 *  - over the threshold for P2P_TREND_OVER_MS and still rising: OVERUSE;
 *    under -gamma: UNDERUSE; else NORMAL;
 *  - a rate controller: OVERUSE sets the cap to 85 % of the acked rate,
 *    NORMAL raises it by 8 % per second (multiplicative), UNDERUSE holds.
 * Integer arithmetic only (milli units, no 64-bit division call), no heap.
 * Honest limit: this sees the queue the probes build on the guest path
 * (slirp or the lab mesh). It is not a WAN measurement. */
#ifndef P2P_TREND_H
#define P2P_TREND_H
#include <stdint.h>

#define P2P_TREND_BURST_MS 5U
#define P2P_TREND_WIN 20U
#define P2P_TREND_OVER_MS 10U
#define P2P_TREND_GAMMA0 12500     /* milli: 12.5 */
#define P2P_TREND_GAMMA_MIN 6000
#define P2P_TREND_GAMMA_MAX 600000
#define P2P_TREND_STALE_MS 2000U   /* no probe group for that long: no cap */

typedef enum { P2P_TREND_NORMAL = 0, P2P_TREND_OVERUSE = 1, P2P_TREND_UNDERUSE = 2 } p2p_trend_state_t;

typedef struct {
    /* current group */
    uint8_t has_group;
    uint32_t g_first_send, g_last_send, g_last_arrival, g_bytes;
    /* previous complete group */
    uint8_t has_prev;
    uint32_t p_last_send, p_last_arrival;
    uint32_t first_arrival;          /* x origin of the regression */
    /* trendline */
    int32_t acc_us;                  /* accumulated delay, microseconds */
    int32_t smooth_us;               /* smoothed accumulated delay */
    uint32_t xs[P2P_TREND_WIN];      /* arrival ms since first_arrival */
    int32_t ys[P2P_TREND_WIN];       /* smoothed delay, microseconds */
    uint8_t nwin, wpos;
    uint32_t deltas;                 /* group deltas seen */
    int32_t trend_milli;             /* slope x 1000 (us per ms) */
    int32_t modified_milli;          /* min(deltas,60) * trend * 4 */
    int32_t prev_modified;
    /* detector */
    int32_t gamma_milli;
    uint32_t last_update_ms;
    uint32_t over_ms;                /* time spent over the threshold */
    uint32_t over_count;
    p2p_trend_state_t state;
    /* rate controller */
    uint32_t cap_bps;                /* 0 = no cap yet */
    uint32_t cap_stamp_ms;
    uint32_t ack_bytes, ack_win_start; /* acked rate over ~500 ms */
    uint32_t acked_bps;
    /* counters */
    uint32_t samples, groups, overuses, underuses, reordered;
} p2p_trend_t;

void p2p_trend_init(p2p_trend_t* t, uint32_t now_ms);
/* One acked probe: its send time, its ack (arrival) time, its size. */
void p2p_trend_on_packet(p2p_trend_t* t, uint32_t send_ms, uint32_t arrival_ms, uint32_t bytes);
/* Cap on the pacing rate (bytes/s); 0 when the detector has no opinion. */
uint32_t p2p_trend_cap(const p2p_trend_t* t);
/* min(rate, cap) with a 0 cap meaning "no cap". The cap only binds while
 * probe groups are recent (P2P_TREND_STALE_MS): an old overuse does not
 * hold the data rate down after the probe train has ended. */
uint32_t p2p_trend_limit(const p2p_trend_t* t, uint32_t rate_bps, uint32_t now_ms);
const char* p2p_trend_state_name(p2p_trend_state_t s);
/* "p2p trend state S trend_milli T modified M gamma G deltas D cap_bps C
 * acked_bps A overuses O groups N" */
void p2p_trend_status(const p2p_trend_t* t, char* out, int cap);
/* signed 64 / 64 bit division without the libgcc helper (exposed for tests) */
int64_t p2p_trend_sdiv64(int64_t n, int64_t d);
#endif
