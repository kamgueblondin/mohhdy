/* p2p_shape.c - Lot 88 (US-068/US-073): fair shaping queue, see p2p_shape.h. */
#include "p2p_shape.h"

static void cat(char* out, int cap, const char* s) {
    int n = 0;
    while (n < cap - 1 && out[n]) n++;
    while (*s && n < cap - 1) out[n++] = *s++;
    out[n] = 0;
}

static void cat_u(char* out, int cap, uint32_t v) {
    char t[12];
    int i = 11;
    t[i] = 0;
    do { t[--i] = (char)('0' + v % 10U); v /= 10U; } while (v && i > 0);
    cat(out, cap, t + i);
}

static void copy(uint8_t* d, const uint8_t* s, int n) {
    int i;
    for (i = 0; i < n; i++) d[i] = s[i];
}

void p2p_shape_init(p2p_shape_t* q) {
    uint8_t* p = (uint8_t*)q;
    uint32_t i;
    if (!q) return;
    for (i = 0; i < (uint32_t)sizeof(*q); i++) p[i] = 0;
}

int p2p_shape_class(int is_put) {
    return is_put ? P2P_SHAPE_C_BULK : P2P_SHAPE_C_MSG;
}

int p2p_shape_depth(const p2p_shape_t* q, uint32_t peer, int cls) {
    int i, n = 0;
    if (!q) return 0;
    for (i = 0; i < P2P_SHAPE_Q; i++) {
        const p2p_shape_ent_t* e = &q->e[i];
        if (!e->used) continue;
        if (peer && e->peer != peer) continue;
        if (cls >= 0 && e->cls != (uint8_t)cls) continue;
        n++;
    }
    return n;
}

/* Sequence order that survives the 32-bit wrap: a before b. */
static int older(uint32_t a, uint32_t b) {
    return (int32_t)(a - b) < 0;
}

/* newest entry of a peer (cls -1: any class); -1 when none */
static int newest_of(const p2p_shape_t* q, uint32_t peer, int cls) {
    int i, best = -1;
    for (i = 0; i < P2P_SHAPE_Q; i++) {
        const p2p_shape_ent_t* e = &q->e[i];
        if (!e->used || e->peer != peer) continue;
        if (cls >= 0 && e->cls != (uint8_t)cls) continue;
        if (best < 0 || older(q->e[best].seq, e->seq)) best = i;
    }
    return best;
}

/* the peer with the most entries (cls -1: any class); ties go to the peer
 * with the most bytes queued, then to the lowest id, so the choice is
 * deterministic */
static uint32_t heaviest(const p2p_shape_t* q, int cls, int* count) {
    uint32_t best = 0, best_bytes = 0;
    int i, j, best_n = 0;
    for (i = 0; i < P2P_SHAPE_Q; i++) {
        const p2p_shape_ent_t* e = &q->e[i];
        uint32_t bytes = 0;
        int n = 0;
        if (!e->used) continue;
        if (cls >= 0 && e->cls != (uint8_t)cls) continue;
        for (j = 0; j < P2P_SHAPE_Q; j++) {
            const p2p_shape_ent_t* f = &q->e[j];
            if (!f->used || f->peer != e->peer) continue;
            if (cls >= 0 && f->cls != (uint8_t)cls) continue;
            n++;
            bytes += f->len;
        }
        if (n > best_n || (n == best_n && (bytes > best_bytes || (bytes == best_bytes && e->peer < best)))) {
            best = e->peer;
            best_n = n;
            best_bytes = bytes;
        }
    }
    if (count) *count = best_n;
    return best;
}

/* Which entry a full queue gives up for a newcomer, or -1 to refuse it.
 * 1. An interactive newcomer takes the place of the newest bulk entry of
 *    the peer holding the most bulk entries.
 * 2. Otherwise the peer holding the most entries gives up its newest one
 *    (bulk first) when, after the swap, it still holds at least as many as
 *    the newcomer's peer.
 * A peer alone in the queue is therefore refused, as before lot 88. */
static int victim(const p2p_shape_t* q, uint32_t peer, int cls) {
    int n_heavy = 0, n_mine, v;
    uint32_t heavy;
    if (cls == P2P_SHAPE_C_MSG && p2p_shape_depth(q, 0, P2P_SHAPE_C_BULK) > 0) {
        heavy = heaviest(q, P2P_SHAPE_C_BULK, 0);
        return newest_of(q, heavy, P2P_SHAPE_C_BULK);
    }
    heavy = heaviest(q, -1, &n_heavy);
    if (heavy == peer) return -1;
    n_mine = p2p_shape_depth(q, peer, -1);
    if (n_heavy - 1 < n_mine + 1) return -1;
    v = newest_of(q, heavy, P2P_SHAPE_C_BULK);
    if (v < 0) v = newest_of(q, heavy, -1);
    return v;
}

int p2p_shape_push(p2p_shape_t* q, uint32_t peer, int cls, int inner, const uint8_t* body, int len, uint32_t now_ms) {
    int i, slot = -1, rc = P2P_SHAPE_QUEUED;
    if (!q || !body || len < 0 || len > P2P_SHAPE_BODY || peer == 0U) {
        if (q) q->refused++;
        return P2P_SHAPE_REFUSED;
    }
    if (cls != P2P_SHAPE_C_BULK) cls = P2P_SHAPE_C_MSG;
    for (i = 0; i < P2P_SHAPE_Q; i++) {
        if (!q->e[i].used) { slot = i; break; }
    }
    if (slot < 0) {
        slot = victim(q, peer, cls);
        if (slot < 0) { q->refused++; return P2P_SHAPE_REFUSED; }
        q->e[slot].used = 0;
        q->evicted++;
        rc = P2P_SHAPE_QUEUED_EVICT;
    }
    q->e[slot].used = 1;
    q->e[slot].cls = (uint8_t)cls;
    q->e[slot].inner = (uint8_t)inner;
    q->e[slot].len = (uint16_t)len;
    q->e[slot].peer = peer;
    q->e[slot].seq = q->seq++;
    q->e[slot].at_ms = now_ms;
    copy(q->e[slot].body, body, len);
    q->queued++;
    return rc;
}

int p2p_shape_forget(p2p_shape_t* q, uint32_t peer) {
    int i, n = 0;
    if (!q) return 0;
    for (i = 0; i < P2P_SHAPE_Q; i++) {
        if (q->e[i].used && q->e[i].peer == peer) { q->e[i].used = 0; n++; }
    }
    q->dropped += (uint32_t)n;
    return n;
}

static int is_skipped(const uint32_t* skip, int nskip, uint32_t peer) {
    int i;
    for (i = 0; i < nskip; i++) if (skip[i] == peer) return 1;
    return 0;
}

/* Next peer of class cls in round-robin order after rr (wrapping to the
 * lowest id), skipping the refused ones. Returns the slot of that peer's
 * oldest entry of the class, or -1. */
static int next_slot(const p2p_shape_t* q, int cls, const uint32_t* skip, int nskip) {
    uint32_t after = q->rr_set[cls] ? q->rr_peer[cls] : 0U;
    uint32_t pick = 0, low = 0;
    int i, have_pick = 0, have_low = 0, best = -1;
    for (i = 0; i < P2P_SHAPE_Q; i++) {
        const p2p_shape_ent_t* e = &q->e[i];
        if (!e->used || e->cls != (uint8_t)cls || is_skipped(skip, nskip, e->peer)) continue;
        if (!have_low || e->peer < low) { low = e->peer; have_low = 1; }
        if (e->peer > after && (!have_pick || e->peer < pick)) { pick = e->peer; have_pick = 1; }
    }
    if (!have_low) return -1;
    if (!have_pick) pick = low;
    for (i = 0; i < P2P_SHAPE_Q; i++) {
        const p2p_shape_ent_t* e = &q->e[i];
        if (!e->used || e->cls != (uint8_t)cls || e->peer != pick) continue;
        if (best < 0 || older(e->seq, q->e[best].seq)) best = i;
    }
    return best;
}

int p2p_shape_drain(p2p_shape_t* q, p2p_shape_send_fn fn, void* ctx, uint32_t now_ms, int* dropped) {
    static const uint32_t quantum[P2P_SHAPE_CLASSES] = { P2P_SHAPE_QUANTUM_MSG, P2P_SHAPE_QUANTUM_BULK };
    uint32_t skip[P2P_SHAPE_Q];
    int nskip = 0, sent = 0, drops = 0, rounds, c;
    if (dropped) *dropped = 0;
    if (!q || !fn) return 0;
    q->passes++;
    /* every round tops each busy class up by its quantum; a pass ends when
     * a round moved nothing (all refused or empty). Bounded: each round
     * either removes an entry or adds a peer to skip, or ends the pass. */
    for (rounds = 0; rounds < 4 * P2P_SHAPE_Q + 4; rounds++) {
        int moved = 0;
        for (c = 0; c < P2P_SHAPE_CLASSES; c++) {
            int slot = next_slot(q, c, skip, nskip);
            if (slot < 0) {
                /* an idle class keeps no credit (deficit round robin) */
                if (p2p_shape_depth(q, 0, c) == 0) q->deficit[c] = 0;
                continue;
            }
            q->deficit[c] += quantum[c];
            while (slot >= 0 && q->deficit[c] >= q->e[slot].len) {
                p2p_shape_ent_t* e = &q->e[slot];
                int rc = fn(ctx, e->peer, e->inner, e->body, e->len);
                q->rr_peer[c] = e->peer;
                q->rr_set[c] = 1;
                if (rc == P2P_SHAPE_WAIT) {
                    if (nskip < P2P_SHAPE_Q) skip[nskip++] = e->peer;
                    q->skips++;
                    moved = 1;
                } else {
                    uint32_t wait = now_ms - e->at_ms;
                    q->deficit[c] -= e->len;
                    e->used = 0;
                    moved = 1;
                    if (rc == P2P_SHAPE_SENT) {
                        sent++;
                        q->sent++;
                        q->sent_cls[c]++;
                        q->bytes_cls[c] += e->len;
                        q->last_wait_ms = wait;
                        if (wait > q->max_wait_ms) q->max_wait_ms = wait;
                    } else {
                        drops++;
                        q->dropped++;
                    }
                }
                slot = next_slot(q, c, skip, nskip);
            }
            if (slot < 0 && p2p_shape_depth(q, 0, c) == 0) q->deficit[c] = 0;
        }
        if (!moved) break;
    }
    /* credit left over by refused peers is not banked across passes beyond
     * one quantum, so a long refusal does not turn into a burst later */
    for (c = 0; c < P2P_SHAPE_CLASSES; c++) {
        if (q->deficit[c] > quantum[c]) q->deficit[c] = quantum[c];
    }
    if (dropped) *dropped = drops;
    return sent;
}

void p2p_shape_status(const p2p_shape_t* q, char* out, int cap) {
    if (!out || cap <= 0) return;
    out[0] = 0;
    if (!q) return;
    cat(out, cap, "depth "); cat_u(out, cap, (uint32_t)p2p_shape_depth(q, 0, -1));
    cat(out, cap, " msg "); cat_u(out, cap, (uint32_t)p2p_shape_depth(q, 0, P2P_SHAPE_C_MSG));
    cat(out, cap, " bulk "); cat_u(out, cap, (uint32_t)p2p_shape_depth(q, 0, P2P_SHAPE_C_BULK));
    cat(out, cap, " queued "); cat_u(out, cap, q->queued);
    cat(out, cap, " sent "); cat_u(out, cap, q->sent);
    cat(out, cap, " dropped "); cat_u(out, cap, q->dropped);
    cat(out, cap, " evicted "); cat_u(out, cap, q->evicted);
    cat(out, cap, " refused "); cat_u(out, cap, q->refused);
    cat(out, cap, " msg_sent "); cat_u(out, cap, q->sent_cls[P2P_SHAPE_C_MSG]);
    cat(out, cap, " bulk_sent "); cat_u(out, cap, q->sent_cls[P2P_SHAPE_C_BULK]);
    cat(out, cap, " last_wait_ms "); cat_u(out, cap, q->last_wait_ms);
    cat(out, cap, " max_wait_ms "); cat_u(out, cap, q->max_wait_ms);
    cat(out, cap, " skips "); cat_u(out, cap, q->skips);
}
