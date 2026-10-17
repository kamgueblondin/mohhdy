#include "devsync.h"

static int s_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static void s_copy(char* d, const char* s, int cap) { int i = 0; while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; } d[i] = 0; }

static int token_ok(const char* s, int cap) {
    int i, n = s_len(s);
    if (n == 0 || n >= cap) return 0;
    for (i = 0; i < n; i++) if (s[i] <= ' ' || s[i] > '~') return 0;
    return 1;
}

int ds_init(ds_replica_t* r, int id) {
    int i;
    if (!r || id < 0 || id >= DS_DEVICES) return -1;
    r->id = (uint8_t)id; r->clock = 0U; r->n = 0;
    r->applied = r->ignored = r->conflicts = 0U;
    for (i = 0; i < DS_DEVICES; i++) r->vv[i] = 0U;
    return 0;
}

static int find(const ds_replica_t* r, const char* key) {
    int i;
    for (i = 0; i < r->n; i++) if (s_eq(r->e[i].key, key)) return i;
    return -1;
}

/* a wins over b */
static int wins(uint32_t ac, uint8_t ad, uint32_t bc, uint8_t bd) {
    return ac > bc || (ac == bc && ad > bd);
}

int ds_apply(ds_replica_t* r, const ds_entry_t* e) {
    int i;
    if (!r || !e || e->dev >= DS_DEVICES || e->clock == 0U || !token_ok(e->key, DS_KEY_LEN)) return -1;
    if (!e->dead && !token_ok(e->val, DS_VAL_LEN)) return -1;
    if (e->clock > r->clock) r->clock = e->clock;
    if (e->clock > r->vv[e->dev]) r->vv[e->dev] = e->clock;
    i = find(r, e->key);
    if (i < 0) {
        if (r->n >= DS_KEYS) return -1;
        i = r->n++;
    } else {
        ds_entry_t* cur = &r->e[i];
        if (!wins(e->clock, e->dev, cur->clock, cur->dev)) {
            if (!(cur->clock == e->clock && cur->dev == e->dev) && cur->dev != e->dev) r->conflicts++;
            r->ignored++;
            return 0;
        }
        if (cur->dev != e->dev) r->conflicts++;
    }
    s_copy(r->e[i].key, e->key, DS_KEY_LEN);
    if (e->dead) r->e[i].val[0] = 0; else s_copy(r->e[i].val, e->val, DS_VAL_LEN);
    r->e[i].clock = e->clock;
    r->e[i].dev = e->dev;
    r->e[i].dead = e->dead ? 1U : 0U;
    r->applied++;
    return 1;
}

static int local_write(ds_replica_t* r, const char* key, const char* val, int dead) {
    ds_entry_t e;
    int rc;
    if (!r || !key || !token_ok(key, DS_KEY_LEN)) return -1;
    if (!dead && (!val || !token_ok(val, DS_VAL_LEN))) return -1;
    if (dead && find(r, key) < 0) return -1;
    s_copy(e.key, key, DS_KEY_LEN);
    s_copy(e.val, dead ? "" : val, DS_VAL_LEN);
    e.clock = r->clock + 1U;
    e.dev = r->id;
    e.dead = (uint8_t)dead;
    rc = ds_apply(r, &e);
    if (rc == 1) { r->applied--; return 0; }    /* local writes are not "applied" */
    return -1;
}

int ds_set(ds_replica_t* r, const char* key, const char* val) { return local_write(r, key, val, 0); }
int ds_del(ds_replica_t* r, const char* key) { return local_write(r, key, 0, 1); }

const char* ds_get(const ds_replica_t* r, const char* key) {
    int i;
    if (!r || !key) return 0;
    i = find(r, key);
    if (i < 0 || r->e[i].dead) return 0;
    return r->e[i].val;
}

int ds_live(const ds_replica_t* r) {
    int i, n = 0;
    if (!r) return 0;
    for (i = 0; i < r->n; i++) if (!r->e[i].dead) n++;
    return n;
}

static uint32_t fnv(const char* s, int n) {
    uint32_t h = 2166136261U;
    int i;
    for (i = 0; i < n; i++) h = (h ^ (uint8_t)s[i]) * 16777619U;
    return h;
}

static int put(char* out, int pos, int cap, const char* s) {
    while (*s) { if (pos >= cap - 1) return -1; out[pos++] = *s++; }
    out[pos] = 0;
    return pos;
}
static int put_u(char* out, int pos, int cap, uint32_t v) {
    char t[11], r[11];
    int n = 0, i = 0;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v);
    while (n) r[i++] = t[--n];
    r[i] = 0;
    return put(out, pos, cap, r);
}

/* Format: "ds1 FROM COUNT\n" then per entry "e CLOCK DEV DEAD KEY VAL\n"
 * (VAL "-" for a tombstone), then "sum HEX\n" over everything before. */
int ds_export(const ds_replica_t* r, const uint32_t* peer_vv, char* out, int cap) {
    int i, pos = 0, count = 0;
    uint32_t h;
    static const char hx[] = "0123456789abcdef";
    char sum[16];
    if (!r || !out || cap < 32) return -1;
    for (i = 0; i < r->n; i++) if (!peer_vv || r->e[i].clock > peer_vv[r->e[i].dev]) count++;
    pos = put(out, pos, cap, "ds1 ");
    if (pos >= 0) pos = put_u(out, pos, cap, r->id);
    if (pos >= 0) pos = put(out, pos, cap, " ");
    if (pos >= 0) pos = put_u(out, pos, cap, (uint32_t)count);
    if (pos >= 0) pos = put(out, pos, cap, "\n");
    for (i = 0; i < r->n && pos >= 0; i++) {
        const ds_entry_t* e = &r->e[i];
        if (peer_vv && e->clock <= peer_vv[e->dev]) continue;
        pos = put(out, pos, cap, "e ");
        if (pos >= 0) pos = put_u(out, pos, cap, e->clock);
        if (pos >= 0) pos = put(out, pos, cap, " ");
        if (pos >= 0) pos = put_u(out, pos, cap, e->dev);
        if (pos >= 0) pos = put(out, pos, cap, e->dead ? " 1 " : " 0 ");
        if (pos >= 0) pos = put(out, pos, cap, e->key);
        if (pos >= 0) pos = put(out, pos, cap, " ");
        if (pos >= 0) pos = put(out, pos, cap, e->dead ? "-" : e->val);
        if (pos >= 0) pos = put(out, pos, cap, "\n");
    }
    if (pos < 0) return -1;
    h = fnv(out, pos);
    for (i = 0; i < 8; i++) sum[i] = hx[(h >> (28 - 4 * i)) & 15U];
    sum[8] = 0;
    pos = put(out, pos, cap, "sum ");
    if (pos >= 0) pos = put(out, pos, cap, sum);
    if (pos >= 0) pos = put(out, pos, cap, "\n");
    return pos;
}

static const char* word(const char* s, char* w, int cap) {
    int n = 0;
    while (*s == ' ') s++;
    while (*s && *s != ' ' && *s != '\n') { if (n < cap - 1) w[n++] = *s; else return 0; s++; }
    w[n] = 0;
    return s;
}
static int num(const char* w, uint32_t* v) {
    uint32_t x = 0U;
    if (!*w) return 0;
    while (*w) { if (*w < '0' || *w > '9' || x > 400000000U) return 0; x = x * 10U + (uint32_t)(*w++ - '0'); }
    *v = x;
    return 1;
}

int ds_import(ds_replica_t* r, const char* in) {
    const char* p = in;
    const char* sum_at;
    char w[DS_VAL_LEN + 2];
    uint32_t from, count, i, h = 0U;
    int applied = 0, k;
    ds_entry_t batch[DS_KEYS];
    if (!r || !in) return -1;
    /* locate and check the sum line first: nothing is applied on a bad sum */
    sum_at = in;
    while (*sum_at) {
        if ((sum_at == in || sum_at[-1] == '\n') && sum_at[0] == 's' && sum_at[1] == 'u' && sum_at[2] == 'm' && sum_at[3] == ' ') break;
        sum_at++;
    }
    if (!*sum_at) return -1;
    for (k = 0; k < 8; k++) {
        char c = sum_at[4 + k];
        uint32_t d = (c >= '0' && c <= '9') ? (uint32_t)(c - '0') : (c >= 'a' && c <= 'f') ? (uint32_t)(c - 'a' + 10) : 99U;
        if (d == 99U) return -1;
        h = (h << 4) | d;
    }
    if (fnv(in, (int)(sum_at - in)) != h) return -2;
    p = word(p, w, (int)sizeof(w));
    if (!p || !s_eq(w, "ds1")) return -1;
    p = word(p, w, (int)sizeof(w)); if (!p || !num(w, &from) || from >= DS_DEVICES) return -1;
    p = word(p, w, (int)sizeof(w)); if (!p || !num(w, &count) || count > DS_KEYS) return -1;
    if (*p == '\n') p++;
    for (i = 0; i < count; i++) {
        uint32_t clock, dev, dead;
        ds_entry_t* e = &batch[i];
        p = word(p, w, (int)sizeof(w)); if (!p || !s_eq(w, "e")) return -1;
        p = word(p, w, (int)sizeof(w)); if (!p || !num(w, &clock) || clock == 0U) return -1;
        p = word(p, w, (int)sizeof(w)); if (!p || !num(w, &dev) || dev >= DS_DEVICES) return -1;
        p = word(p, w, (int)sizeof(w)); if (!p || !num(w, &dead) || dead > 1U) return -1;
        p = word(p, e->key, DS_KEY_LEN); if (!p || !e->key[0]) return -1;
        p = word(p, e->val, DS_VAL_LEN); if (!p || !e->val[0]) return -1;
        if (*p != '\n') return -1;
        p++;
        e->clock = clock; e->dev = (uint8_t)dev; e->dead = (uint8_t)dead;
        if (dead) e->val[0] = 0;
    }
    if (p != sum_at) return -1;
    for (i = 0; i < count; i++) {
        int rc = ds_apply(r, &batch[i]);
        if (rc < 0) return -1;
        applied += rc;
    }
    return applied;
}

uint32_t ds_digest(const ds_replica_t* r) {
    uint32_t acc = 0U;
    int i;
    if (!r) return 0U;
    for (i = 0; i < r->n; i++) {
        uint32_t h;
        if (r->e[i].dead) continue;
        h = fnv(r->e[i].key, s_len(r->e[i].key));
        h = (h ^ 0x3DU) * 16777619U;
        {
            int j, n = s_len(r->e[i].val);
            for (j = 0; j < n; j++) h = (h ^ (uint8_t)r->e[i].val[j]) * 16777619U;
        }
        acc += h * 2654435761U + 1U;        /* sum: independent of entry order */
    }
    return acc;
}
