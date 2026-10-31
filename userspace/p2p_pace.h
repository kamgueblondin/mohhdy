/* p2p_pace.h - Lot 68 (US-068/US-073): the bandwidth estimator drives the
 * mesh send rate. A token bucket paces user data (messages, KV puts and
 * the shaping queue) at the estimator's pacing rate (p2p_bwe: gain x
 * bottleneck bandwidth), bounded by a floor, a ceiling and an optional
 * operator cap. When the estimator has no sample yet (no probe train),
 * auto mode keeps the older per-second window budget. Control frames are
 * never paced. Integer arithmetic, no heap, host testable. */
#ifndef P2P_PACE_H
#define P2P_PACE_H
#include <stdint.h>
#include "p2p_bwe.h"

#define P2P_PACE_FLOOR_BPS 1000U      /* never pace below 1 KB/s */
#define P2P_PACE_CEIL_BPS 8000000U    /* nor above 8 MB/s */
#define P2P_PACE_BURST_DEFAULT 4U     /* datagrams of credit after idle */
#define P2P_PACE_BURST_MAX 32U
#define P2P_PACE_OVERHEAD 48U         /* seal header + tag + UDP/IP, bytes */

typedef enum { P2P_PACE_OFF = 0, P2P_PACE_AUTO = 1, P2P_PACE_ON = 2 } p2p_pace_mode_t;

typedef struct {
    uint8_t mode;          /* p2p_pace_mode_t */
    uint8_t source;        /* 0 none (window), 1 estimator, 2 initial rate */
    uint32_t cap_bps;      /* operator cap, 0 = none */
    uint32_t burst_pkts;   /* credit after idle, datagrams */
    uint32_t rate_bps;     /* last rate used */
    uint32_t next_us;      /* next departure time */
    uint32_t admitted, deferred, bytes;
    uint32_t last_cost_us;
} p2p_pace_t;

void p2p_pace_init(p2p_pace_t* p);
/* Rate for the next send: 0 means "not paced, use the window budget". */
uint32_t p2p_pace_rate(p2p_pace_t* p, const p2p_bwe_t* bwe, int bwe_ready);
/* 1 when `bytes` of payload may leave now at `rate` (charged), else 0. */
int p2p_pace_admit(p2p_pace_t* p, uint32_t rate, uint32_t bytes, uint32_t now_us);
/* Microseconds until the next datagram of `bytes` could leave (0: now). */
uint32_t p2p_pace_wait_us(const p2p_pace_t* p, uint32_t now_us);
const char* p2p_pace_mode_name(int mode);
const char* p2p_pace_source_name(int source);
/* "p2p-pace [status|on|off|auto|cap BPS|burst N]" (argv without the
 * command). Writes one line; returns 0, or 1 on a usage error. */
int p2p_pace_command(p2p_pace_t* p, int argc, const char* const* argv, char* out, int cap);
/* status line "p2p pace mode M source S rate_bps R cap_bps C burst B
 * admitted A deferred D bytes N" */
void p2p_pace_status(const p2p_pace_t* p, char* out, int cap);
#endif
