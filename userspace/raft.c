/* userspace/raft.c - US-064 Raft-like consensus (see raft.h). */
#include "raft.h"

#define T_RV 0x60
#define T_RVR 0x61
#define T_AE 0x62
#define T_AER 0x63
#define ENTRY_WIRE (4 + RAFT_KEY + RAFT_VAL)

static int s_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static void s_copy(char* d, const char* s, int cap) { int i = 0; while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; } d[i] = 0; }
static void s_cat(char* d, const char* s, int cap) { int n = s_len(d), i = 0; while (s && s[i] && n < cap - 1) d[n++] = s[i++]; d[n] = 0; }
static void cat_u(char* d, uint32_t v, int cap) { char t[11], o[12]; int n = 0, i = 0; do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v); while (n) o[i++] = t[--n]; o[i] = 0; s_cat(d, o, cap); }
static void put32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static uint32_t get32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static const char* nm(const raft_host_t* h, uint32_t id) { return h && h->name ? h->name(h->ctx, id) : "?"; }
static void say(const raft_host_t* h, const char* s) { if (h && h->out) h->out(h->ctx, s); }
static void say2(const raft_host_t* h, const char* a, const char* who, const char* b, uint32_t v) {
    char line[96]; line[0] = 0;
    s_cat(line, a, 96); s_cat(line, who, 96); s_cat(line, b, 96); cat_u(line, v, 96); say(h, line);
}
static uint32_t term_at(const raft_t* r, uint32_t idx) { return (idx == 0 || idx > r->nlog) ? 0U : r->log[idx - 1].term; }
static int majority(const raft_t* r) { return (r->nmembers + 1) / 2 + 1; }
static void reset_timer(raft_t* r, uint32_t now) { r->last_heard = now; r->timeout = RAFT_ELECTION_MIN + (r->self % 200U) + (r->term * 37U) % 100U; }
static int idx_of(const raft_t* r, uint32_t id) { int i; for (i = 0; i < r->nmembers; i++) if (r->members[i] == id) return i; return -1; }

int raft_start(raft_t* r, uint32_t self, const uint32_t* members, int n, uint32_t now) {
    int i;
    uint8_t* p = (uint8_t*)r;
    if (!r || !self || n < 0 || n > RAFT_MAX_NODES) return -1;
    for (i = 0; i < (int)sizeof(*r); i++) p[i] = 0;
    r->self = self;
    for (i = 0; i < n; i++) if (members[i] && members[i] != self) r->members[r->nmembers++] = members[i];
    r->on = 1;
    reset_timer(r, now);
    return 0;
}

static void become_follower(raft_t* r, const raft_host_t* h, uint32_t term, uint32_t now) {
    if (r->role == RAFT_LEADER) { r->step_downs++; say2(h, "raft step down ", "", " term ", term); }
    r->role = RAFT_FOLLOWER;
    if (term > r->term) { r->term = term; r->voted_for = 0; }
    reset_timer(r, now);
}

static void send_rv(raft_t* r, const raft_host_t* h) {
    uint8_t b[16]; int i;
    b[0] = T_RV; put32(b + 1, r->term); put32(b + 5, r->nlog); put32(b + 9, term_at(r, r->nlog));
    for (i = 0; i < r->nmembers; i++) h->send(h->ctx, r->members[i], b, 13);
}

static void send_ae(raft_t* r, const raft_host_t* h, int k) {
    uint8_t b[1 + 20 + RAFT_BATCH * ENTRY_WIRE];
    uint32_t prev = r->next[k] - 1, cnt = 0, i;
    int pos;
    b[0] = T_AE; put32(b + 1, r->term); put32(b + 5, prev); put32(b + 9, term_at(r, prev)); put32(b + 13, r->commit);
    pos = 21;
    for (i = r->next[k]; i <= r->nlog && cnt < RAFT_BATCH; i++, cnt++) {
        int j;
        put32(b + pos, r->log[i - 1].term); pos += 4;
        for (j = 0; j < RAFT_KEY; j++) b[pos + j] = (uint8_t)r->log[i - 1].key[j];
        pos += RAFT_KEY;
        for (j = 0; j < RAFT_VAL; j++) b[pos + j] = (uint8_t)r->log[i - 1].val[j];
        pos += RAFT_VAL;
    }
    put32(b + 17, cnt);
    h->send(h->ctx, r->members[k], b, pos);
}

static void apply(raft_t* r, const raft_host_t* h) {
    while (r->applied < r->commit) {
        raft_entry_t* e = &r->log[r->applied];
        int i, slot = -1;
        r->applied++;
        for (i = 0; i < RAFT_KV; i++) {
            if (r->kv[i].used && s_eq(r->kv[i].key, e->key)) { slot = i; break; }
            if (!r->kv[i].used && slot < 0) slot = i;
        }
        if (slot >= 0) { s_copy(r->kv[slot].key, e->key, RAFT_KEY); s_copy(r->kv[slot].val, e->val, RAFT_VAL); r->kv[slot].index = r->applied; r->kv[slot].used = 1; }
        {
            char line[96]; line[0] = 0;
            s_cat(line, "raft applied ", 96); cat_u(line, r->applied, 96); s_cat(line, " ", 96);
            s_cat(line, e->key, 96); s_cat(line, "=", 96); s_cat(line, e->val, 96);
            s_cat(line, " term ", 96); cat_u(line, e->term, 96); say(h, line);
        }
    }
}

static void leader_commit(raft_t* r, const raft_host_t* h) {
    uint32_t n;
    for (n = r->nlog; n > r->commit; n--) {
        int i, c = 1;
        if (r->log[n - 1].term != r->term) continue; /* only current-term entries commit by count */
        for (i = 0; i < r->nmembers; i++) if (r->match[i] >= n) c++;
        if (c >= majority(r)) { r->commit = n; break; }
    }
    apply(r, h);
}

void raft_tick(raft_t* r, const raft_host_t* h, uint32_t now) {
    int i;
    if (!r || !r->on) return;
    if (r->role == RAFT_LEADER) {
        if (now - r->last_sent >= RAFT_HEARTBEAT) {
            r->last_sent = now;
            for (i = 0; i < r->nmembers; i++) send_ae(r, h, i);
        }
        return;
    }
    if (now - r->last_heard >= r->timeout) {
        r->role = RAFT_CANDIDATE; r->term++; r->voted_for = r->self; r->votes = 1; r->elections++;
        reset_timer(r, now);
        say2(h, "raft election ", "", " term ", r->term);
        if ((int)r->votes >= majority(r)) { r->role = RAFT_LEADER; r->leader = r->self; }
        else send_rv(r, h);
    }
}

static void win(raft_t* r, const raft_host_t* h, uint32_t now) {
    int i;
    r->role = RAFT_LEADER; r->leader = r->self;
    for (i = 0; i < r->nmembers; i++) { r->next[i] = r->nlog + 1; r->match[i] = 0; }
    say2(h, "raft leader ", nm(h, r->self), " term ", r->term);
    r->last_sent = now - RAFT_HEARTBEAT; /* heartbeat right away */
    raft_tick(r, h, now);
}

void raft_receive(raft_t* r, const raft_host_t* h, uint32_t from, const uint8_t* d, int len, uint32_t now) {
    uint32_t term;
    int k;
    if (!r || !r->on || len < 5) return;
    k = idx_of(r, from);
    if (k < 0) return; /* not a cluster member */
    term = get32(d + 1);
    if (term > r->term) become_follower(r, h, term, now);
    if (d[0] == T_RV && len >= 13) {
        uint32_t li = get32(d + 5), lt = get32(d + 9), mylt = term_at(r, r->nlog);
        uint8_t b[12]; int grant = 0;
        int uptodate = lt > mylt || (lt == mylt && li >= r->nlog);
        if (term == r->term && (r->voted_for == 0 || r->voted_for == from) && uptodate) {
            grant = 1; r->voted_for = from; reset_timer(r, now);
        }
        b[0] = T_RVR; put32(b + 1, r->term); b[5] = (uint8_t)grant;
        h->send(h->ctx, from, b, 6);
    } else if (d[0] == T_RVR && len >= 6) {
        if (r->role == RAFT_CANDIDATE && term == r->term && d[5]) {
            r->votes++;
            if ((int)r->votes >= majority(r)) win(r, h, now);
        }
    } else if (d[0] == T_AE && len >= 21) {
        uint32_t prev = get32(d + 5), pterm = get32(d + 9), lcommit = get32(d + 13), cnt = get32(d + 17), i;
        uint8_t b[12]; int ok = 0;
        uint32_t match = 0;
        if (term == r->term) {
            if (r->role != RAFT_FOLLOWER) become_follower(r, h, term, now);
            if (r->leader != from) { r->leader = from; say2(h, "raft follows ", nm(h, from), " term ", term); }
            reset_timer(r, now);
            if (cnt <= RAFT_BATCH && len >= 21 + (int)(cnt * ENTRY_WIRE) && prev <= r->nlog && term_at(r, prev) == pterm) {
                const uint8_t* e = d + 21;
                ok = 1;
                for (i = 0; i < cnt; i++, e += ENTRY_WIRE) {
                    uint32_t idx = prev + 1 + i, et = get32(e);
                    int j;
                    if (idx <= r->nlog && term_at(r, idx) != et) {   /* conflict: drop it and all after */
                        r->truncated += r->nlog - idx + 1; r->nlog = idx - 1;
                    }
                    if (idx > r->nlog) {
                        if (r->nlog >= RAFT_LOG) { ok = 0; break; }
                        r->log[r->nlog].term = et;
                        for (j = 0; j < RAFT_KEY; j++) r->log[r->nlog].key[j] = (char)e[4 + j];
                        for (j = 0; j < RAFT_VAL; j++) r->log[r->nlog].val[j] = (char)e[4 + RAFT_KEY + j];
                        r->log[r->nlog].key[RAFT_KEY - 1] = 0; r->log[r->nlog].val[RAFT_VAL - 1] = 0;
                        r->nlog++;
                    }
                }
                match = prev + i;
                if (ok && lcommit > r->commit) { r->commit = lcommit < match ? lcommit : match; apply(r, h); }
            } else r->rejected_appends++;
        }
        b[0] = T_AER; put32(b + 1, r->term); b[5] = (uint8_t)ok; put32(b + 6, match);
        h->send(h->ctx, from, b, 10);
    } else if (d[0] == T_AER && len >= 10) {
        if (r->role == RAFT_LEADER && term == r->term) {
            if (d[5]) { uint32_t m = get32(d + 6); if (m > r->match[k]) r->match[k] = m; r->next[k] = r->match[k] + 1; leader_commit(r, h); }
            else if (r->next[k] > 1) { r->next[k]--; send_ae(r, h, k); }
        }
    }
}

int raft_propose(raft_t* r, const raft_host_t* h, const char* key, const char* val, uint32_t now) {
    int i;
    if (!r || !r->on || !key || !val || !key[0] || s_len(key) >= RAFT_KEY || s_len(val) >= RAFT_VAL) return -3;
    if (r->role != RAFT_LEADER) return -1;
    if (r->nlog >= RAFT_LOG) return -2;
    r->log[r->nlog].term = r->term;
    for (i = 0; i < RAFT_KEY; i++) r->log[r->nlog].key[i] = 0;
    for (i = 0; i < RAFT_VAL; i++) r->log[r->nlog].val[i] = 0;
    s_copy(r->log[r->nlog].key, key, RAFT_KEY); s_copy(r->log[r->nlog].val, val, RAFT_VAL);
    r->nlog++;
    if (r->nmembers == 0) leader_commit(r, h);
    r->last_sent = now - RAFT_HEARTBEAT; /* replicate now */
    raft_tick(r, h, now);
    return (int)r->nlog;
}

const char* raft_get(const raft_t* r, const char* key) {
    int i;
    for (i = 0; i < RAFT_KV; i++) if (r->kv[i].used && s_eq(r->kv[i].key, key)) return r->kv[i].val;
    return 0;
}
uint32_t raft_digest(const raft_t* r) {
    uint32_t hsh = 2166136261U, i;
    int j;
    for (i = 0; i < r->commit; i++) {
        const uint8_t* p = (const uint8_t*)&r->log[i];
        for (j = 0; j < (int)sizeof(raft_entry_t); j++) { hsh ^= p[j]; hsh *= 16777619U; }
    }
    return hsh;
}
