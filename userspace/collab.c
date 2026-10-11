/* userspace/collab.c - Phase 7 collaborative ledger. See collab.h. */
#include "collab.h"
#include "../kernel/sha256.h"
#include "csig.h"

static const char* const k_types[CE_COUNT] = {
    "?", "join", "transfer", "offer", "reserve", "task", "claim", "done", "accept", "reject",
    "rate", "propose", "vote", "profile", "redact", "ticket", "answer"
};
static const char* const k_reasons[] = {
    "ok", "not-joined", "insufficient-points", "unknown-ref", "capacity-full", "bad-state",
    "not-allowed", "wrong-result", "duplicate", "no-rating-right", "bad-amount", "table-full"
};
const char* collab_type_name(int t) { return (t > 0 && t < CE_COUNT) ? k_types[t] : "?"; }
const char* collab_reason(int r) { return (r >= 0 && r <= 11) ? k_reasons[r] : "?"; }

static int s_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static void s_copy(char* d, const char* s, int cap) { int i = 0; while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; } d[i] = 0; }
static void s_cat(char* d, const char* s, int cap) { int n = s_len(d), i = 0; while (s && s[i] && n < cap - 1) d[n++] = s[i++]; d[n] = 0; }
static void cat_u(char* d, uint32_t v, int cap) {
    char t[11], r[12]; int n = 0, i = 0;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v);
    while (n) r[i++] = t[--n];
    r[i] = 0; s_cat(d, r, cap);
}
static void cat_i(char* d, int32_t v, int cap) { if (v < 0) { s_cat(d, "-", cap); v = -v; } cat_u(d, (uint32_t)v, cap); }
static void mzero(void* p, int n) { int i; for (i = 0; i < n; i++) ((uint8_t*)p)[i] = 0; }
static void put32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static uint32_t get32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static void say(const collab_host_t* h, const char* s) { if (h && h->out) h->out(h->ctx, s); }
static const char* nm(const collab_host_t* h, uint32_t id) { return (h && h->name) ? h->name(h->ctx, id) : "?"; }

int collab_encode(const collab_entry_t* e, uint8_t* o) {
    int i;
    o[0] = e->type; o[1] = o[2] = o[3] = 0;
    put32(o + 4, e->origin); put32(o + 8, e->seq); put32(o + 12, e->lamport); put32(o + 16, e->peer);
    put32(o + 20, e->ref); put32(o + 24, e->amount); put32(o + 28, e->aux);
    for (i = 0; i < COLLAB_TEXT; i++) o[32 + i] = (uint8_t)e->text[i];
    return COLLAB_WIRE;
}
int collab_decode(const uint8_t* in, int len, collab_entry_t* e) {
    int i;
    if (len < COLLAB_WIRE || in[0] == 0 || in[0] >= CE_COUNT) return -1;
    e->type = in[0]; e->origin = get32(in + 4); e->seq = get32(in + 8); e->lamport = get32(in + 12);
    e->peer = get32(in + 16); e->ref = get32(in + 20); e->amount = get32(in + 24); e->aux = get32(in + 28);
    for (i = 0; i < COLLAB_TEXT; i++) e->text[i] = (char)in[32 + i];
    e->text[COLLAB_TEXT - 1] = 0;
    for (i = 0; e->text[i]; i++) if ((unsigned char)e->text[i] < 32 || (unsigned char)e->text[i] > 126) e->text[i] = '?';
    if (e->origin == 0 || e->seq == 0) return -1;
    return 0;
}

void collab_init(collab_t* c, uint32_t self) { mzero(c, (int)sizeof(*c)); c->self = self; }

static void redact(collab_t* c, uint32_t origin, uint32_t lamport) {
    int i;
    for (i = 0; i < c->n; i++)
        if (c->e[i].type == CE_PROFILE && c->e[i].origin == origin && c->e[i].lamport < lamport) {
            mzero(c->e[i].text, COLLAB_TEXT);
            s_copy(c->e[i].text, "[redacted]", COLLAB_TEXT);
        }
}
static int find(const collab_t* c, uint32_t origin, uint32_t seq) {
    int i;
    for (i = 0; i < c->n; i++) if (c->e[i].origin == origin && c->e[i].seq == seq) return i;
    return -1;
}
/* 1 stored, 0 duplicate, -1 full */
static int store(collab_t* c, const collab_entry_t* e) {
    int i;
    if (find(c, e->origin, e->seq) >= 0) { c->duplicates++; return 0; }
    if (c->n >= COLLAB_ENTRIES) { c->dropped++; return -1; }
    c->e[c->n++] = *e;
    if (e->lamport > c->lamport) c->lamport = e->lamport;
    if (e->type == CE_REDACT) redact(c, e->origin, e->lamport);
    if (e->type == CE_PROFILE)
        for (i = 0; i < c->n; i++)
            if (c->e[i].type == CE_REDACT && c->e[i].origin == e->origin && c->e[i].lamport > e->lamport) {
                redact(c, e->origin, c->e[i].lamport);
                break;
            }
    return 1;
}


/* ------------------------------------------------------- signatures */
#define SIGN_LEN 64
#define MSG3_LEN (1 + COLLAB_WIRE + 32 + 40 + 128)
static void text_hash(const char* text, uint8_t th[32]) {
    sha256_ctx_t h;
    sha256_init(&h); sha256_update(&h, (const uint8_t*)text, (uint32_t)s_len(text)); sha256_final(&h, th);
}
/* signed bytes: the 32 fixed header bytes of the wire entry + SHA-256 of
 * the original text, so redacting a profile text keeps the signature valid */
static void sign_input(const collab_entry_t* e, const uint8_t th[32], uint8_t* out) {
    uint8_t w[COLLAB_WIRE];
    int i;
    collab_encode(e, w);
    for (i = 0; i < 32; i++) { out[i] = w[i]; out[32 + i] = th[i]; }
}
const uint8_t* collab_key_of(const collab_t* c, uint32_t origin) {
    int i;
    for (i = 0; i < c->nkeys; i++) if (c->key_origin[i] == origin) return c->key_pk[i];
    return 0;
}
static int bind_key(collab_t* c, uint32_t origin, const uint8_t* pk) {
    int i;
    if (collab_key_of(c, origin)) return 0;
    if (c->nkeys >= COLLAB_NODES) return -1;
    c->key_origin[c->nkeys] = origin;
    for (i = 0; i < 128; i++) c->key_pk[c->nkeys][i] = pk[i];
    c->nkeys++;
    return 0;
}
int collab_keys_set(collab_t* c, const uint8_t seed[32]) {
    if (csig_keypair(seed, c->sk, c->pk) != 0) return -1;
    c->signing = 1;
    if (collab_key_of(c, c->self)) { int i; for (i = 0; i < c->nkeys; i++) if (c->key_origin[i] == c->self) { int j; for (j = 0; j < 128; j++) c->key_pk[i][j] = c->pk[j]; } return 0; }
    return bind_key(c, c->self, c->pk);
}
void collab_key_fpr(const uint8_t pk[128], char out[17]) {
    sha256_ctx_t h; uint8_t d[32]; int i;
    sha256_init(&h); sha256_update(&h, pk, 128); sha256_final(&h, d);
    for (i = 0; i < 8; i++) { out[2 * i] = "0123456789abcdef"[d[i] >> 4]; out[2 * i + 1] = "0123456789abcdef"[d[i] & 15]; }
    out[16] = 0;
}
static void send_entry(collab_t* c, const collab_host_t* h, uint32_t to, int i) {
    uint8_t b[MSG3_LEN];
    const uint8_t* pk = collab_key_of(c, c->e[i].origin);
    int k;
    if (c->has_sig[i] && pk) {
        b[0] = 3; collab_encode(&c->e[i], b + 1);
        for (k = 0; k < 32; k++) b[1 + COLLAB_WIRE + k] = c->th[i][k];
        for (k = 0; k < 40; k++) b[1 + COLLAB_WIRE + 32 + k] = c->sig[i][k];
        for (k = 0; k < 128; k++) b[1 + COLLAB_WIRE + 72 + k] = pk[k];
        if (h && h->send) (void)h->send(h->ctx, to, b, MSG3_LEN);
        return;
    }
    b[0] = 1; collab_encode(&c->e[i], b + 1);
    if (h && h->send) (void)h->send(h->ctx, to, b, 1 + COLLAB_WIRE);
}
static int is_redacted(const collab_entry_t* e) { return e->type == CE_PROFILE && s_eq(e->text, "[redacted]"); }
static void reject(collab_t* c, const collab_host_t* h, const char* why, const collab_entry_t* e, uint32_t from) {
    char line[120];
    c->forged++;
    line[0] = 0;
    s_cat(line, "collab reject ", 120); s_cat(line, why, 120); s_cat(line, " ", 120);
    s_cat(line, collab_type_name(e->type), 120); s_cat(line, " claimed from ", 120); s_cat(line, nm(h, e->origin), 120);
    s_cat(line, " seq ", 120); cat_u(line, e->seq, 120); s_cat(line, " via ", 120); s_cat(line, nm(h, from), 120);
    say(h, line);
}
int collab_forge(collab_t* c, const collab_host_t* h, uint32_t victim, uint32_t amount, int mode) {
    collab_entry_t e;
    uint8_t b[MSG3_LEN], th[32], in[SIGN_LEN];
    uint32_t maxseq = 0;
    const uint8_t* vpk = collab_key_of(c, victim);
    int i;
    if (!c->signing || victim == c->self) return -1;
    for (i = 0; i < c->n; i++) if (c->e[i].origin == victim && c->e[i].seq > maxseq) maxseq = c->e[i].seq;
    mzero(&e, (int)sizeof(e));
    e.type = CE_TRANSFER; e.origin = victim; e.seq = maxseq + 1U; e.lamport = c->lamport + 1U;
    e.peer = c->self; e.amount = amount; s_copy(e.text, "forged", COLLAB_TEXT);
    collab_encode(&e, b + 1);
    if (mode == 2) { b[0] = 1; return (h && h->send) ? h->send(h->ctx, 0, b, 1 + COLLAB_WIRE) : 0; }
    if (mode == 0 && !vpk) return -1;
    text_hash(e.text, th); sign_input(&e, th, in);
    b[0] = 3;
    for (i = 0; i < 32; i++) b[1 + COLLAB_WIRE + i] = th[i];
    if (csig_sign(c->sk, in, SIGN_LEN, b + 1 + COLLAB_WIRE + 32) != 0) return -1;
    for (i = 0; i < 128; i++) b[1 + COLLAB_WIRE + 72 + i] = mode == 0 ? vpk[i] : c->pk[i];
    return (h && h->send) ? h->send(h->ctx, 0, b, MSG3_LEN) : 0;
}

/* ------------------------------------------------------- persistence */
#define SAVE_MAGIC 0x31424c43U /* "CLB1" */
int collab_save(const collab_t* c, uint8_t* o, int cap) {
    int pos = 0, i, k;
    int need = 32 + 128 + 20 + c->nkeys * 132 + c->n * (COLLAB_WIRE + 40 + 1 + 32);
    if (need > cap) return -1;
    put32(o, SAVE_MAGIC); put32(o + 4, c->self); put32(o + 8, c->seq); put32(o + 12, c->lamport);
    put32(o + 16, (uint32_t)c->n); put32(o + 20, (uint32_t)c->nkeys); put32(o + 24, c->signing); put32(o + 28, c->forged);
    pos = 32;
    for (k = 0; k < 20; k++) o[pos++] = c->sk[k];
    for (k = 0; k < 128; k++) o[pos++] = c->pk[k];
    for (i = 0; i < c->nkeys; i++) {
        put32(o + pos, c->key_origin[i]); pos += 4;
        for (k = 0; k < 128; k++) o[pos++] = c->key_pk[i][k];
    }
    for (i = 0; i < c->n; i++) {
        collab_encode(&c->e[i], o + pos); pos += COLLAB_WIRE;
        o[pos++] = (uint8_t)(c->has_sig[i] | (is_redacted(&c->e[i]) ? 2 : 0));
        for (k = 0; k < 40; k++) o[pos++] = c->sig[i][k];
        if (is_redacted(&c->e[i])) for (k = 0; k < 32; k++) o[pos++] = c->th[i][k];
    }
    return pos;
}
int collab_load(collab_t* c, const uint8_t* in, int len) {
    int pos = 32, i, k, n, nk;
    collab_field_t keep[COLLAB_FIELDS];
    if (len < 32 + 148 || get32(in) != SAVE_MAGIC) return -1;
    n = (int)get32(in + 16); nk = (int)get32(in + 20);
    if (n < 0 || n > COLLAB_ENTRIES || nk < 0 || nk > COLLAB_NODES) return -1;
    for (i = 0; i < COLLAB_FIELDS; i++) keep[i] = c->profile[i];
    collab_init(c, get32(in + 4));
    for (i = 0; i < COLLAB_FIELDS; i++) c->profile[i] = keep[i];
    c->seq = get32(in + 8); c->lamport = get32(in + 12); c->signing = (uint8_t)get32(in + 24); c->forged = get32(in + 28);
    for (k = 0; k < 20; k++) c->sk[k] = in[pos++];
    for (k = 0; k < 128; k++) c->pk[k] = in[pos++];
    for (i = 0; i < nk; i++) {
        if (pos + 132 > len) return -1;
        c->key_origin[i] = get32(in + pos); pos += 4;
        for (k = 0; k < 128; k++) c->key_pk[i][k] = in[pos++];
    }
    c->nkeys = nk;
    for (i = 0; i < n; i++) {
        uint8_t f;
        if (pos + COLLAB_WIRE + 41 > len || collab_decode(in + pos, COLLAB_WIRE, &c->e[i]) != 0) return -1;
        pos += COLLAB_WIRE;
        f = in[pos++];
        c->has_sig[i] = f & 1U;
        for (k = 0; k < 40; k++) c->sig[i][k] = in[pos++];
        if (f & 2U) { if (pos + 32 > len) return -1; for (k = 0; k < 32; k++) c->th[i][k] = in[pos++]; }
        else text_hash(c->e[i].text, c->th[i]);
    }
    c->n = n;
    return n;
}

int collab_emit(collab_t* c, const collab_host_t* h, uint8_t type, uint32_t peer, uint32_t ref,
                uint32_t amount, uint32_t aux, const char* text) {
    collab_entry_t e;
    uint8_t b[1 + COLLAB_WIRE];
    if (!c || !c->self || type == 0 || type >= CE_COUNT || s_len(text) >= COLLAB_TEXT) return -1;
    mzero(&e, (int)sizeof(e));
    e.type = type; e.origin = c->self; e.seq = c->seq + 1U; e.lamport = c->lamport + 1U;
    e.peer = peer; e.ref = ref; e.amount = amount; e.aux = aux;
    s_copy(e.text, text ? text : "", COLLAB_TEXT);
    if (store(c, &e) != 1) return -2;
    c->seq = e.seq;
    if (c->signing) {
        int i = c->n - 1;
        text_hash(e.text, c->th[i]);
        sign_input(&e, c->th[i], b);
        if (csig_sign(c->sk, b, SIGN_LEN, c->sig[i]) == 0) c->has_sig[i] = 1;
    }
    send_entry(c, h, 0, c->n - 1);
    return (int)e.seq;
}

int collab_sync(collab_t* c, const collab_host_t* h) {
    uint8_t b[2 + COLLAB_NODES * 8];
    uint32_t origins[COLLAB_NODES], maxs[COLLAB_NODES];
    int i, j, k = 0;
    for (i = 0; i < c->n; i++) {
        for (j = 0; j < k && origins[j] != c->e[i].origin; j++) {}
        if (j == k) { if (k == COLLAB_NODES) continue; origins[k] = c->e[i].origin; maxs[k++] = 0; }
        if (c->e[i].seq > maxs[j]) maxs[j] = c->e[i].seq;
    }
    b[0] = 2; b[1] = (uint8_t)k;
    for (j = 0; j < k; j++) { put32(b + 2 + j * 8, origins[j]); put32(b + 6 + j * 8, maxs[j]); }
    return (h && h->send) ? h->send(h->ctx, 0, b, 2 + k * 8) : 0;
}

void collab_receive(collab_t* c, const collab_host_t* h, uint32_t from, const uint8_t* d, int len) {
    char line[120];
    if (!c || !d || len < 1) return;
    if (d[0] == 1 || d[0] == 3) {
        collab_entry_t e;
        uint8_t th[32], in[SIGN_LEN];
        const uint8_t *sig = 0, *pk = 0;
        if (collab_decode(d + 1, len - 1, &e) != 0) return;
        if (find(c, e.origin, e.seq) >= 0) { c->duplicates++; return; } /* no re-verification */
        if (d[0] == 1 && c->signing) { reject(c, h, "unsigned", &e, from); return; }
        if (d[0] == 3) {
            const uint8_t* known;
            int k;
            if (len < MSG3_LEN) return;
            sig = d + 1 + COLLAB_WIRE + 32; pk = d + 1 + COLLAB_WIRE + 72;
            if (is_redacted(&e)) for (k = 0; k < 32; k++) th[k] = d[1 + COLLAB_WIRE + k];
            else text_hash(e.text, th);
            known = collab_key_of(c, e.origin);
            if (known) { for (k = 0; k < 128 && known[k] == pk[k]; k++) {} if (k < 128) { reject(c, h, "key-mismatch", &e, from); return; } }
            else if (!csig_pk_valid(pk)) { reject(c, h, "bad-key", &e, from); return; }
            sign_input(&e, th, in);
            if (!csig_verify(pk, in, SIGN_LEN, sig)) { reject(c, h, "bad-signature", &e, from); return; }
            if (!known) (void)bind_key(c, e.origin, pk); /* first verified key of this origin */
        }
        if (store(c, &e) == 1) {
            if (sig) { int k, i = c->n - 1; for (k = 0; k < 40; k++) c->sig[i][k] = sig[k]; for (k = 0; k < 32; k++) c->th[i][k] = th[k]; c->has_sig[i] = 1; }
            line[0] = 0;
            s_cat(line, "collab got ", 120); s_cat(line, collab_type_name(e.type), 120);
            s_cat(line, " from ", 120); s_cat(line, nm(h, e.origin), 120);
            s_cat(line, " seq ", 120); cat_u(line, e.seq, 120);
            if (e.origin != from) { s_cat(line, " via ", 120); s_cat(line, nm(h, from), 120); }
            say(h, line);
        }
    } else if (d[0] == 2 && len >= 2) {
        int k = d[1], i, j, sent = 0;
        if (len < 2 + k * 8) return;
        for (i = 0; i < c->n; i++) {
            int newer = 1;
            for (j = 0; j < k; j++)
                if (get32(d + 2 + j * 8) == c->e[i].origin && c->e[i].seq <= get32(d + 6 + j * 8)) newer = 0;
            if (newer) { send_entry(c, h, from, i); sent++; }
        }
        line[0] = 0;
        s_cat(line, "collab sync to ", 120); s_cat(line, nm(h, from), 120);
        s_cat(line, " sent ", 120); cat_u(line, (uint32_t)sent, 120);
        say(h, line);
    }
}

/* ---------------------------------------------------------- task eval */
static int parse_u(const char* s, uint32_t* v) {
    uint32_t r = 0; int any = 0;
    while (*s >= '0' && *s <= '9') { r = r * 10U + (uint32_t)(*s++ - '0'); any = 1; if (r > 100000000U) return -1; }
    if (!any || *s) return -1;
    *v = r;
    return 0;
}
int collab_task_eval(const char* spec, uint32_t* result) {
    uint32_t n, i, j, count = 0;
    if (!spec || !result) return -1;
    if (spec[0] == 's' && spec[1] == 'u' && spec[2] == 'm' && spec[3] == ' ') {
        if (parse_u(spec + 4, &n) || n > 60000U) return -1;
        *result = n * (n + 1U) / 2U;
        return 0;
    }
    if (spec[0] == 'p' && spec[1] == 'r' && spec[2] == 'i' && spec[3] == 'm' && spec[4] == 'e' && spec[5] == 's' && spec[6] == ' ') {
        if (parse_u(spec + 7, &n) || n > 20000U) return -1;
        for (i = 2; i <= n; i++) { int prime = 1; for (j = 2; j * j <= i; j++) if (i % j == 0) { prime = 0; break; } count += (uint32_t)prime; }
        *result = count;
        return 0;
    }
    if (spec[0] == 'f' && spec[1] == 'n' && spec[2] == 'v' && spec[3] == ' ' && spec[4]) {
        uint32_t hsh = 2166136261U;
        for (i = 4; spec[i]; i++) { hsh ^= (uint8_t)spec[i]; hsh *= 16777619U; }
        *result = hsh;
        return 0;
    }
    return -1;
}

/* ------------------------------------------------------------- fold */
typedef struct { uint32_t owner, seq, price, cap, used; } offer_t;
typedef struct { uint32_t req, seq, reward, worker, result; int state; } task_t; /* 1 open 2 claimed 3 done 4 paid 5 refused */
typedef struct { uint32_t origin, seq, yes, no; uint32_t voters[COLLAB_NODES]; int nv; } prop_t;
typedef struct { uint32_t a, b; } right_t;
typedef struct { uint32_t rater, target, stars; } rating_t;
static offer_t g_off[16]; static int g_noff;
static task_t g_task[16]; static int g_ntask;
static prop_t g_prop[8]; static int g_nprop;
static right_t g_right[32]; static int g_nright;
static rating_t g_rate[32]; static int g_nrate;
static uint32_t g_tickets, g_answered;
typedef struct { uint32_t origin, seq; int answered; } ticket_t;
static ticket_t g_tick[16]; static int g_ntick;
static int g_order[COLLAB_ENTRIES];

static collab_account_t* acct(collab_state_t* s, uint32_t id, int create) {
    int i;
    for (i = 0; i < s->nacc; i++) if (s->acc[i].id == id) return &s->acc[i];
    if (!create || s->nacc >= COLLAB_NODES) return 0;
    mzero(&s->acc[s->nacc], (int)sizeof(s->acc[0]));
    s->acc[s->nacc].id = id;
    return &s->acc[s->nacc++];
}
const collab_account_t* collab_account(const collab_state_t* s, uint32_t id) {
    int i;
    for (i = 0; i < s->nacc; i++) if (s->acc[i].id == id) return &s->acc[i];
    return 0;
}
static int before(const collab_entry_t* a, const collab_entry_t* b) {
    if (a->lamport != b->lamport) return a->lamport < b->lamport;
    if (a->origin != b->origin) return a->origin < b->origin;
    return a->seq < b->seq;
}
static int has_right(uint32_t a, uint32_t b) {
    int i;
    for (i = 0; i < g_nright; i++) if ((g_right[i].a == a && g_right[i].b == b) || (g_right[i].a == b && g_right[i].b == a)) return 1;
    return 0;
}
static void add_right(uint32_t a, uint32_t b) { if (!has_right(a, b) && g_nright < 32) { g_right[g_nright].a = a; g_right[g_nright].b = b; g_nright++; } }
static task_t* task_of(uint32_t req, uint32_t seq) { int i; for (i = 0; i < g_ntask; i++) if (g_task[i].req == req && g_task[i].seq == seq) return &g_task[i]; return 0; }
static void profile_put(collab_account_t* a, const char* kv) {
    char out[COLLAB_TEXT * 2], key[COLLAB_TEXT];
    int i = 0, k = 0, p = 0;
    while (kv[k] && kv[k] != '=' && k < COLLAB_TEXT - 1) { key[k] = kv[k]; k++; }
    key[k] = 0;
    if (kv[k] != '=' || k == 0) return;
    out[0] = 0;
    while (a->shared[i]) { /* copy segments with another key */
        char seg[COLLAB_TEXT * 2]; int n = 0, kl = 0;
        while (a->shared[i] && a->shared[i] != ';' && n < (int)sizeof(seg) - 1) seg[n++] = a->shared[i++];
        seg[n] = 0;
        if (a->shared[i] == ';') i++;
        while (seg[kl] && seg[kl] != '=') kl++;
        seg[kl] = 0;
        if (s_eq(seg, key)) continue;
        seg[kl] = '=';
        s_cat(out, seg, (int)sizeof(out)); s_cat(out, ";", (int)sizeof(out));
        p++;
    }
    s_cat(out, kv, (int)sizeof(out)); s_cat(out, ";", (int)sizeof(out));
    s_copy(a->shared, out, (int)sizeof(a->shared));
}

static int apply(collab_state_t* s, const collab_entry_t* e) {
    collab_account_t* o = acct(s, e->origin, e->type == CE_JOIN);
    collab_account_t* p;
    int i;
    if (e->type == CE_JOIN) {
        if (!o) return 11;
        if (o->joined) return 8;
        o->joined = 1; o->balance += (int32_t)COLLAB_GRANT;
        s_copy(o->shared, "", 2);
        return 0;
    }
    if (!o || !o->joined) return 1;
    switch (e->type) {
    case CE_TRANSFER:
        p = acct(s, e->peer, 0);
        if (!p || !p->joined || p == o) return 6;
        if (e->amount == 0 || e->amount > 1000000U) return 10;
        if (o->balance < (int32_t)e->amount) return 2;
        o->balance -= (int32_t)e->amount; p->balance += (int32_t)e->amount;
        return 0;
    case CE_OFFER:
        if (e->aux == 0 || e->amount > 1000000U || !e->text[0]) return 10;
        if (g_noff >= 16) return 11;
        g_off[g_noff].owner = e->origin; g_off[g_noff].seq = e->seq; g_off[g_noff].price = e->amount;
        g_off[g_noff].cap = e->aux; g_off[g_noff].used = 0; g_noff++;
        return 0;
    case CE_RESERVE:
        for (i = 0; i < g_noff; i++) if (g_off[i].owner == e->peer && g_off[i].seq == e->ref) break;
        if (i == g_noff) return 3;
        if (e->peer == e->origin) return 6;
        if (g_off[i].used >= g_off[i].cap) return 4;
        if (o->balance < (int32_t)g_off[i].price) return 2;
        p = acct(s, e->peer, 0);
        if (!p) return 3;
        o->balance -= (int32_t)g_off[i].price; p->balance += (int32_t)g_off[i].price; g_off[i].used++;
        add_right(e->origin, e->peer);
        return 0;
    case CE_TASK: {
        uint32_t r;
        if (e->amount == 0 || e->amount > 1000000U) return 10;
        if (collab_task_eval(e->text, &r) != 0) return 10;
        if (o->balance < (int32_t)e->amount) return 2;
        if (g_ntask >= 16) return 11;
        o->balance -= (int32_t)e->amount; /* escrow */
        g_task[g_ntask].req = e->origin; g_task[g_ntask].seq = e->seq; g_task[g_ntask].reward = e->amount;
        g_task[g_ntask].worker = 0; g_task[g_ntask].result = r; g_task[g_ntask].state = 1; g_ntask++;
        return 0;
    }
    case CE_CLAIM: {
        task_t* t = task_of(e->peer, e->ref);
        if (!t) return 3;
        if (t->req == e->origin) return 6;
        if (t->state != 1) return 5;
        t->state = 2; t->worker = e->origin;
        return 0;
    }
    case CE_DONE: {
        task_t* t = task_of(e->peer, e->ref);
        if (!t) return 3;
        if (t->worker != e->origin) return 6;
        if (t->state != 2) return 5;
        t->state = 3; t->worker = e->origin;
        /* the claimed result travels in aux; correctness decided at accept/reject */
        if (e->aux == t->result) t->state = 3; else t->state = 6; /* 6: done with a wrong result */
        return 0;
    }
    case CE_ACCEPT:
    case CE_REJECT: {
        task_t* t = task_of(e->origin, e->ref);
        if (!t) return 3;
        if (t->state != 3 && t->state != 6) return 5;
        if (e->type == CE_ACCEPT && t->state != 3) return 7;   /* cannot pay a wrong result */
        if (e->type == CE_REJECT && t->state != 6) return 7;   /* cannot refuse a right result */
        if (e->type == CE_ACCEPT) {
            p = acct(s, t->worker, 0);
            if (!p) return 3;
            p->balance += (int32_t)t->reward; t->state = 4;
            add_right(t->req, t->worker);
        } else {
            o->balance += (int32_t)t->reward; t->state = 5;
        }
        return 0;
    }
    case CE_RATE:
        if (e->amount < 1 || e->amount > 5) return 10;
        if (!has_right(e->origin, e->peer)) return 9;
        p = acct(s, e->peer, 0);
        if (!p) return 3;
        for (i = 0; i < g_nrate; i++) if (g_rate[i].rater == e->origin && g_rate[i].target == e->peer) break;
        if (i < g_nrate) { p->rating_sum -= g_rate[i].stars; p->rating_sum += e->amount; g_rate[i].stars = e->amount; return 0; }
        if (g_nrate >= 32) return 11;
        g_rate[g_nrate].rater = e->origin; g_rate[g_nrate].target = e->peer; g_rate[g_nrate].stars = e->amount; g_nrate++;
        p->rating_sum += e->amount; p->ratings++;
        return 0;
    case CE_PROPOSE:
        if (!e->text[0]) return 10;
        if (g_nprop >= 8) return 11;
        mzero(&g_prop[g_nprop], (int)sizeof(g_prop[0]));
        g_prop[g_nprop].origin = e->origin; g_prop[g_nprop].seq = e->seq; g_nprop++;
        return 0;
    case CE_VOTE:
        for (i = 0; i < g_nprop; i++) if (g_prop[i].origin == e->peer && g_prop[i].seq == e->ref) break;
        if (i == g_nprop) return 3;
        {
            int v;
            for (v = 0; v < g_prop[i].nv; v++) if (g_prop[i].voters[v] == e->origin) return 8;
            if (g_prop[i].nv >= COLLAB_NODES) return 11;
            g_prop[i].voters[g_prop[i].nv++] = e->origin;
            if (e->amount) g_prop[i].yes++; else g_prop[i].no++;
        }
        return 0;
    case CE_PROFILE:
        profile_put(o, e->text);
        return 0;
    case CE_REDACT:
        o->shared[0] = 0;
        return 0;
    case CE_TICKET:
        if (!e->text[0]) return 10;
        if (g_ntick >= 16) return 11;
        g_tick[g_ntick].origin = e->origin; g_tick[g_ntick].seq = e->seq; g_tick[g_ntick].answered = 0; g_ntick++;
        g_tickets++;
        return 0;
    case CE_ANSWER:
        for (i = 0; i < g_ntick; i++) if (g_tick[i].origin == e->peer && g_tick[i].seq == e->ref) break;
        if (i == g_ntick) return 3;
        if (e->peer == e->origin) return 6;
        if (!g_tick[i].answered) g_answered++;
        g_tick[i].answered = 1;
        return 0;
    default:
        return 5;
    }
}

void collab_fold(collab_t* c, collab_state_t* s) {
    sha256_ctx_t h;
    uint8_t w[COLLAB_WIRE + 1];
    int i, j;
    mzero(s, (int)sizeof(*s));
    g_noff = g_ntask = g_nprop = g_nright = g_nrate = g_ntick = 0; g_tickets = g_answered = 0;
    for (i = 0; i < c->n; i++) g_order[i] = i;
    for (i = 1; i < c->n; i++) { /* insertion sort, canonical order */
        int k = g_order[i];
        for (j = i - 1; j >= 0 && before(&c->e[k], &c->e[g_order[j]]); j--) g_order[j + 1] = g_order[j];
        g_order[j + 1] = k;
    }
    sha256_init(&h);
    for (i = 0; i < c->n; i++) {
        int k = g_order[i], r = apply(s, &c->e[k]);
        c->status[k] = (uint8_t)r;
        if (r) s->rejected++; else s->applied++;
        collab_encode(&c->e[k], w); w[COLLAB_WIRE] = (uint8_t)r;
        sha256_update(&h, w, COLLAB_WIRE + 1);
    }
    sha256_final(&h, s->digest);
}

/* -------------------------------------------------------------- reports */
static void cat_hex8(char* d, const uint8_t* b, int n, int cap) {
    int i; char t[3];
    for (i = 0; i < n; i++) { t[0] = "0123456789abcdef"[b[i] >> 4]; t[1] = "0123456789abcdef"[b[i] & 15]; t[2] = 0; s_cat(d, t, cap); }
}
static uint32_t joined_count(const collab_state_t* s) { int i; uint32_t n = 0; for (i = 0; i < s->nacc; i++) n += (uint32_t)s->acc[i].joined; return n; }
static const char* task_state(int st) {
    return st == 1 ? "open" : st == 2 ? "claimed" : st == 3 ? "done" : st == 4 ? "paid" : st == 5 ? "refused" : st == 6 ? "done-wrong" : "?";
}
int collab_report(collab_t* c, const collab_host_t* h, const char* what, uint32_t arg, char* out, int cap) {
    static collab_state_t st;
    int i;
    collab_fold(c, &st);
    out[0] = 0;
    if (s_eq(what, "balances")) {
        for (i = 0; i < st.nacc; i++) {
            const collab_account_t* a = &st.acc[i];
            if (!a->joined) continue;
            s_cat(out, "collab acct ", cap); s_cat(out, nm(h, a->id), cap);
            s_cat(out, " balance ", cap); cat_i(out, a->balance, cap);
            s_cat(out, " rep ", cap);
            if (a->ratings) {
                uint32_t tenth = a->rating_sum * 10U / a->ratings;
                cat_u(out, tenth / 10U, cap); s_cat(out, ".", cap); cat_u(out, tenth % 10U, cap);
                s_cat(out, " ratings ", cap); cat_u(out, a->ratings, cap);
            } else s_cat(out, "none", cap);
            s_cat(out, "\n", cap);
        }
        s_cat(out, "collab balances ok members ", cap); cat_u(out, joined_count(&st), cap); s_cat(out, "\n", cap);
    } else if (s_eq(what, "audit")) {
        for (i = 0; i < c->n; i++) {
            if (!c->status[i]) continue;
            s_cat(out, "collab rejected ", cap); s_cat(out, collab_type_name(c->e[i].type), cap);
            s_cat(out, " ", cap); s_cat(out, nm(h, c->e[i].origin), cap); s_cat(out, " seq ", cap); cat_u(out, c->e[i].seq, cap);
            s_cat(out, " reason ", cap); s_cat(out, collab_reason(c->status[i]), cap); s_cat(out, "\n", cap);
        }
        s_cat(out, "collab audit entries ", cap); cat_u(out, (uint32_t)c->n, cap);
        s_cat(out, " applied ", cap); cat_u(out, st.applied, cap);
        s_cat(out, " rejected ", cap); cat_u(out, st.rejected, cap);
        s_cat(out, " digest ", cap); cat_hex8(out, st.digest, 8, cap); s_cat(out, "\n", cap);
    } else if (s_eq(what, "tasks")) {
        for (i = 0; i < g_ntask; i++) {
            int k = find(c, g_task[i].req, g_task[i].seq);
            s_cat(out, "collab task ", cap); s_cat(out, nm(h, g_task[i].req), cap); s_cat(out, "#", cap); cat_u(out, g_task[i].seq, cap);
            s_cat(out, " '", cap); s_cat(out, k >= 0 ? c->e[k].text : "?", cap); s_cat(out, "' reward ", cap); cat_u(out, g_task[i].reward, cap);
            s_cat(out, " state ", cap); s_cat(out, task_state(g_task[i].state), cap);
            if (g_task[i].worker) { s_cat(out, " worker ", cap); s_cat(out, nm(h, g_task[i].worker), cap); }
            s_cat(out, "\n", cap);
        }
        s_cat(out, "collab tasks ok ", cap); cat_u(out, (uint32_t)g_ntask, cap); s_cat(out, "\n", cap);
    } else if (s_eq(what, "offers")) {
        for (i = 0; i < g_noff; i++) {
            int k = find(c, g_off[i].owner, g_off[i].seq);
            s_cat(out, "collab offer ", cap); s_cat(out, nm(h, g_off[i].owner), cap); s_cat(out, "#", cap); cat_u(out, g_off[i].seq, cap);
            s_cat(out, " ", cap); s_cat(out, k >= 0 ? c->e[k].text : "?", cap);
            s_cat(out, " price ", cap); cat_u(out, g_off[i].price, cap);
            s_cat(out, " used ", cap); cat_u(out, g_off[i].used, cap); s_cat(out, "/", cap); cat_u(out, g_off[i].cap, cap); s_cat(out, "\n", cap);
        }
        s_cat(out, "collab offers ok ", cap); cat_u(out, (uint32_t)g_noff, cap); s_cat(out, "\n", cap);
    } else if (s_eq(what, "votes")) {
        uint32_t m = joined_count(&st);
        for (i = 0; i < g_nprop; i++) {
            int k = find(c, g_prop[i].origin, g_prop[i].seq);
            s_cat(out, "collab proposal ", cap); s_cat(out, nm(h, g_prop[i].origin), cap); s_cat(out, "#", cap); cat_u(out, g_prop[i].seq, cap);
            s_cat(out, " '", cap); s_cat(out, k >= 0 ? c->e[k].text : "?", cap);
            s_cat(out, "' yes ", cap); cat_u(out, g_prop[i].yes, cap); s_cat(out, " no ", cap); cat_u(out, g_prop[i].no, cap);
            s_cat(out, " members ", cap); cat_u(out, m, cap);
            s_cat(out, g_prop[i].yes * 2U > m ? " status passed\n" : g_prop[i].no * 2U >= m ? " status rejected\n" : " status open\n", cap);
        }
        s_cat(out, "collab votes ok ", cap); cat_u(out, (uint32_t)g_nprop, cap); s_cat(out, "\n", cap);
    } else if (s_eq(what, "profile")) {
        const collab_account_t* a = collab_account(&st, arg);
        s_cat(out, "collab profile ", cap); s_cat(out, nm(h, arg), cap); s_cat(out, " shared ", cap);
        s_cat(out, (a && a->shared[0]) ? a->shared : "(none)", cap); s_cat(out, "\n", cap);
    } else if (s_eq(what, "tickets")) {
        for (i = 0; i < g_ntick; i++) {
            int k = find(c, g_tick[i].origin, g_tick[i].seq);
            s_cat(out, "collab ticket ", cap); s_cat(out, nm(h, g_tick[i].origin), cap); s_cat(out, "#", cap); cat_u(out, g_tick[i].seq, cap);
            s_cat(out, g_tick[i].answered ? " answered '" : " open '", cap); s_cat(out, k >= 0 ? c->e[k].text : "?", cap); s_cat(out, "'\n", cap);
        }
        for (i = 0; i < c->n; i++) if (c->e[i].type == CE_ANSWER && !c->status[i]) {
            s_cat(out, "collab answer ", cap); s_cat(out, nm(h, c->e[i].origin), cap); s_cat(out, " to ", cap);
            s_cat(out, nm(h, c->e[i].peer), cap); s_cat(out, "#", cap); cat_u(out, c->e[i].ref, cap);
            s_cat(out, " '", cap); s_cat(out, c->e[i].text, cap); s_cat(out, "'\n", cap);
        }
        s_cat(out, "collab tickets ok open ", cap); cat_u(out, g_tickets - g_answered, cap);
        s_cat(out, " answered ", cap); cat_u(out, g_answered, cap); s_cat(out, "\n", cap);
    } else if (s_eq(what, "export")) {
        int n = 0;
        for (i = 0; i < c->n; i++) if (c->e[i].origin == c->self) {
            n++;
            s_cat(out, "collab export ", cap); s_cat(out, collab_type_name(c->e[i].type), cap);
            s_cat(out, " seq ", cap); cat_u(out, c->e[i].seq, cap);
            if (c->e[i].text[0]) { s_cat(out, " '", cap); s_cat(out, c->e[i].text, cap); s_cat(out, "'", cap); }
            s_cat(out, "\n", cap);
        }
        for (i = 0; i < COLLAB_FIELDS; i++) if (c->profile[i].key[0]) {
            s_cat(out, "collab export field ", cap); s_cat(out, c->profile[i].key, cap); s_cat(out, "=", cap);
            s_cat(out, c->profile[i].value, cap); s_cat(out, c->profile[i].shared ? " shared\n" : " private\n", cap);
        }
        s_cat(out, "collab export ok entries ", cap); cat_u(out, (uint32_t)n, cap); s_cat(out, "\n", cap);
    } else return -1;
    return s_len(out);
}

int collab_profile_set(collab_t* c, const char* key, const char* value, int shared) {
    int i, slot = -1;
    if (!key[0] || s_len(key) >= 12 || s_len(value) >= 24) return -1;
    for (i = 0; key[i]; i++) if (key[i] == '=' || key[i] == ';' || key[i] == ' ') return -1;
    for (i = 0; value[i]; i++) if (value[i] == ';') return -1;
    for (i = 0; i < COLLAB_FIELDS; i++) {
        if (c->profile[i].key[0] && s_eq(c->profile[i].key, key)) { slot = i; break; }
        if (!c->profile[i].key[0] && slot < 0) slot = i;
    }
    if (slot < 0) return -1;
    s_copy(c->profile[slot].key, key, 12); s_copy(c->profile[slot].value, value, 24);
    c->profile[slot].shared = (uint8_t)(shared ? 1 : 0);
    return 0;
}

int collab_forget(collab_t* c, const collab_host_t* h) {
    mzero(c->profile, (int)sizeof(c->profile));
    return collab_emit(c, h, CE_REDACT, 0, 0, 0, 0, "");
}
