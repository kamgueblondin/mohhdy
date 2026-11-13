/* p2p_shape.h - Lot 88 (US-068/US-073): the mesh shaping queue.
 * A data datagram the pacer refuses waits here (Lot 6 shaping). Until lot
 * 88 the queue was eight slots drained in slot order: a slot freed at the
 * front was reused by a newer datagram, which then left before the older
 * ones of the same peer (reordering), the peer in the lowest slot always
 * got the first token, and one busy peer could fill every slot so the
 * others' messages were refused.
 * Now:
 * - each entry carries an enqueue sequence; a peer's datagrams of one
 *   class always leave oldest first;
 * - two classes, 0 interactive (messages) and 1 bulk (KV puts), served by
 *   deficit round robin (quantum 3 x 404 bytes against 404) so puts never
 *   starve and messages get three times the bytes;
 * - inside a class, peers take turns (round robin by peer id), and a peer
 *   the pacer refuses is skipped for the rest of the pass, so it does not
 *   block the others (no head-of-line blocking across peers);
 * - when the queue is full, a message may push out a queued put, and a
 *   peer with fewer queued datagrams may push out the newest datagram of
 *   the peer holding the most; a peer alone in the queue is refused as
 *   before;
 * - the time an entry waited is measured (last and largest, ms).
 * Lot 90 (US-068/US-073):
 * - sixteen slots instead of eight;
 * - the class weights are set at run time (p2p_shape_set_weight: 1..16
 *   bodies of 404 bytes per round; defaults 3 for messages, 1 for puts);
 * - an optional age limit: an entry that waited longer than max_age_ms is
 *   dropped at the next drain instead of being sent late (counted as
 *   expired; off by default);
 * - an optional per-peer cap: a peer may hold at most peer_cap entries; a
 *   push beyond it is refused (counted as capped) even when slots are
 *   free, so one peer cannot take the whole queue (off by default: cap =
 *   P2P_SHAPE_Q);
 * - p2p_shape_command parses "status", "weight msg|bulk N",
 *   "age MS|off" and "cap N|off" for the p2p-shape shell command.
 * Fixed size, no heap, integer only, host tested
 * (tests/unit/userspace/test_lot88_shape.c, test_lot90_shape.c). */
#ifndef P2P_SHAPE_H
#define P2P_SHAPE_H
#include <stdint.h>

#define P2P_SHAPE_Q 16              /* Lot 90: 16 (Lot 88: 8) */
#define P2P_SHAPE_BODY 404
#define P2P_SHAPE_CLASSES 2
#define P2P_SHAPE_C_MSG 0
#define P2P_SHAPE_C_BULK 1
#define P2P_SHAPE_QUANTUM_MSG (3U * P2P_SHAPE_BODY)
#define P2P_SHAPE_QUANTUM_BULK P2P_SHAPE_BODY
#define P2P_SHAPE_WEIGHT_MAX 16     /* Lot 90: largest class weight */
#define P2P_SHAPE_AGE_MAX 600000U   /* Lot 90: largest age limit, 10 minutes */

/* enqueue results */
#define P2P_SHAPE_QUEUED 1
#define P2P_SHAPE_QUEUED_EVICT 2  /* queued after pushing one entry out */
#define P2P_SHAPE_REFUSED (-2)

/* send results returned by the drain callback */
#define P2P_SHAPE_SENT 1
#define P2P_SHAPE_WAIT 0          /* not admitted now: keep, skip the peer */
#define P2P_SHAPE_DROP (-1)       /* peer gone or send failed: drop */

typedef struct {
    uint8_t used;
    uint8_t cls;
    uint8_t inner;
    uint16_t len;
    uint32_t peer;
    uint32_t seq;
    uint32_t at_ms;
    uint8_t body[P2P_SHAPE_BODY];
} p2p_shape_ent_t;

typedef struct {
    p2p_shape_ent_t e[P2P_SHAPE_Q];
    uint32_t seq;
    uint32_t deficit[P2P_SHAPE_CLASSES];
    uint32_t rr_peer[P2P_SHAPE_CLASSES];
    uint32_t rr_set[P2P_SHAPE_CLASSES];
    uint32_t queued, sent, dropped, evicted, refused;
    uint32_t sent_cls[P2P_SHAPE_CLASSES];
    uint32_t bytes_cls[P2P_SHAPE_CLASSES];
    uint32_t last_wait_ms, max_wait_ms;
    uint32_t passes, skips;
    /* Lot 90 */
    uint32_t weight[P2P_SHAPE_CLASSES];   /* bodies per round; 0 = default */
    uint32_t max_age_ms;                  /* 0 = no age limit */
    uint32_t peer_cap;                    /* 0 = P2P_SHAPE_Q */
    uint32_t expired, capped;
} p2p_shape_t;

typedef int (*p2p_shape_send_fn)(void* ctx, uint32_t peer, int inner, const uint8_t* body, int len);

void p2p_shape_init(p2p_shape_t* q);
/* class of an inner frame type: bulk for KV puts, interactive otherwise */
int p2p_shape_class(int is_put);
/* P2P_SHAPE_QUEUED, P2P_SHAPE_QUEUED_EVICT or P2P_SHAPE_REFUSED */
int p2p_shape_push(p2p_shape_t* q, uint32_t peer, int cls, int inner, const uint8_t* body, int len, uint32_t now_ms);
/* one pass; returns the datagrams sent. *dropped (may be 0) gets the drops */
int p2p_shape_drain(p2p_shape_t* q, p2p_shape_send_fn fn, void* ctx, uint32_t now_ms, int* dropped);
/* entries waiting: all (peer 0 and cls -1), or one peer and/or class */
int p2p_shape_depth(const p2p_shape_t* q, uint32_t peer, int cls);
/* drops every entry of a peer; returns the count */
int p2p_shape_forget(p2p_shape_t* q, uint32_t peer);
/* "depth D msg M bulk B queued Q sent S dropped X evicted E refused R
 *  msg_sent A bulk_sent K last_wait_ms L max_wait_ms W skips N
 *  weight WM/WB age_ms A cap C expired E capped P" (Lot 90 adds the
 *  last five fields; age_ms 0 = off) */
void p2p_shape_status(const p2p_shape_t* q, char* out, int cap);
/* Lot 90: 0 ok, -1 out of range (weight 1..P2P_SHAPE_WEIGHT_MAX,
 * age 0..P2P_SHAPE_AGE_MAX ms with 0 = off, cap 0..P2P_SHAPE_Q with
 * 0 = off) */
int p2p_shape_set_weight(p2p_shape_t* q, int cls, int weight);
int p2p_shape_weight(const p2p_shape_t* q, int cls);
int p2p_shape_set_max_age(p2p_shape_t* q, uint32_t ms);
int p2p_shape_set_peer_cap(p2p_shape_t* q, int cap);
/* drops the entries older than the age limit; returns the count */
int p2p_shape_expire(p2p_shape_t* q, uint32_t now_ms);
/* "status" (or nothing), "weight msg|bulk N", "age MS|off", "cap N|off".
 * Writes "p2p shape ..." status (or "p2p shape error ...") into out;
 * returns 0, or 1 on a bad command. */
int p2p_shape_command(p2p_shape_t* q, int argc, const char* const* argv, char* out, int cap);
#endif
