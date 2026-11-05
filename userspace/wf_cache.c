/* wf_cache.c - Lot 73: bounded in-memory HTTP cache (see wf_cache.h). */
#include "wf_cache.h"

static char low(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int sp(char c) { return c == ' ' || c == '\t'; }
static int s_eq(const char* a, const char* b) { int i = 0; while (a[i] && a[i] == b[i]) i++; return a[i] == b[i]; }
static void s_cpy(char* d, const char* s, int cap) {
    int i = 0;
    if (cap <= 0) return;
    while (s[i] && i < cap - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}

void wf_cache_init(wf_cache_t* c) {
    int k;
    for (k = 0; k < WF_CACHE_ENTRIES; k++) c->e[k].used = 0;
    c->pool_used = 0;
    c->stamp = 0;
    c->hits = c->revalidated = c->misses = c->stored = c->evicted = c->refused = 0;
}

/* s[0..n) starts with the lower case word w (case-insensitive) */
static int starts(const char* s, int n, const char* w) {
    int k;
    for (k = 0; w[k]; k++) if (k >= n || low(s[k]) != w[k]) return 0;
    return 1;
}

/* the directives of one Cache-Control value */
static void cache_control(const char* v, int n, wf_cache_meta_t* m) {
    int k = 0;
    while (k < n) {
        int a, e;
        while (k < n && (sp(v[k]) || v[k] == ',')) k++;
        a = k;
        while (k < n && v[k] != ',') k++;
        e = k;
        while (e > a && sp(v[e - 1])) e--;
        if (e - a == 8 && starts(v + a, e - a, "no-store")) m->store = 0;
        else if (e - a == 8 && starts(v + a, e - a, "no-cache")) m->no_cache = 1;
        else if (starts(v + a, e - a, "max-age=")) {
            long x = 0;
            int j = a + 8, digits = 0;
            if (j < e && v[j] == '"') j++;
            while (j < e && v[j] >= '0' && v[j] <= '9') { if (x < 100000000L) x = x * 10 + (v[j] - '0'); j++; digits++; }
            if (digits) m->max_age = x;
        }
    }
}

void wf_cache_meta(const char* head, int len, wf_cache_meta_t* m) {
    int i = 0, cc_seen = 0, pragma_nc = 0;
    m->store = 1; m->no_cache = 0; m->max_age = -1;
    m->etag[0] = 0; m->last_modified[0] = 0;
    while (i < len) {
        int a = i, e, colon = -1, v, ve;
        while (i < len && head[i] != '\n') i++;
        e = i;
        if (i < len) i++;
        if (e > a && head[e - 1] == '\r') e--;
        for (v = a; v < e; v++) if (head[v] == ':') { colon = v; break; }
        if (colon < 0) continue;
        v = colon + 1;
        while (v < e && sp(head[v])) v++;
        ve = e;
        while (ve > v && sp(head[ve - 1])) ve--;
        if (colon - a == 13 && starts(head + a, colon - a, "cache-control")) { cc_seen = 1; cache_control(head + v, ve - v, m); }
        else if (colon - a == 6 && starts(head + a, colon - a, "pragma") && starts(head + v, ve - v, "no-cache")) pragma_nc = 1;
        else if (colon - a == 4 && starts(head + a, colon - a, "etag")) {
            int n = ve - v, k;
            if (n > 0 && n < WF_CACHE_TAG) { for (k = 0; k < n; k++) m->etag[k] = head[v + k]; m->etag[n] = 0; }
        } else if (colon - a == 13 && starts(head + a, colon - a, "last-modified")) {
            int n = ve - v, k;
            if (n > 0 && n < WF_CACHE_DATE) { for (k = 0; k < n; k++) m->last_modified[k] = head[v + k]; m->last_modified[n] = 0; }
        }
    }
    if (!cc_seen && pragma_nc) m->store = 0;
    /* nothing to reuse: no lifetime and no validator */
    if (m->store && m->max_age <= 0 && !m->etag[0] && !m->last_modified[0]) m->store = 0;
}

static int find(const wf_cache_t* c, const char* url) {
    int k;
    for (k = 0; k < WF_CACHE_ENTRIES; k++) if (c->e[k].used && s_eq(c->e[k].url, url)) return k;
    return -1;
}

int wf_cache_lookup(wf_cache_t* c, const char* url, uint32_t now, int* idx) {
    int k = find(c, url);
    wf_cache_entry_t* e;
    *idx = k;
    if (k < 0) { c->misses++; return WF_CACHE_MISS; }
    e = &c->e[k];
    e->last_use = ++c->stamp;
    if (!e->meta.no_cache && e->meta.max_age > 0 && now - e->stored_at < (uint32_t)e->meta.max_age) {   /* a clock gone back wraps: stale */
        c->hits++;
        return WF_CACHE_FRESH;
    }
    return WF_CACHE_STALE;
}

int wf_cache_validators(const wf_cache_t* c, int idx, char* out, int cap) {
    static const char* const names[2] = { "If-None-Match: ", "If-Modified-Since: " };
    int n = 0, k;
    if (cap < 1) return -1;
    out[0] = 0;
    if (idx < 0 || idx >= WF_CACHE_ENTRIES || !c->e[idx].used) return 0;
    for (k = 0; k < 2; k++) {
        const char* v = k == 0 ? c->e[idx].meta.etag : c->e[idx].meta.last_modified;
        const char* p;
        if (!v[0]) continue;
        for (p = names[k]; *p; p++) { if (n >= cap - 1) return -1; out[n++] = *p; }
        for (p = v; *p; p++) { if (n >= cap - 1) return -1; out[n++] = *p; }
        if (n + 2 >= cap) return -1;
        out[n++] = '\r'; out[n++] = '\n';
    }
    out[n] = 0;
    return n;
}

/* removes entry k and packs the pool */
static void remove_entry(wf_cache_t* c, int k) {
    uint32_t off = c->e[k].off, len = c->e[k].len, i;
    int j;
    for (i = off; i + len < c->pool_used; i++) c->pool[i] = c->pool[i + len];
    c->pool_used -= len;
    c->e[k].used = 0;
    for (j = 0; j < WF_CACHE_ENTRIES; j++) if (c->e[j].used && c->e[j].off > off) c->e[j].off -= len;
}

int wf_cache_store(wf_cache_t* c, const char* url, const wf_cache_meta_t* m, const char* ctype, const uint8_t* body,
                   int len, uint32_t now) {
    int k, slot = -1, guard;
    wf_cache_entry_t* e;
    if (!m->store || len < 0 || len > WF_CACHE_POOL / 4) {
        if ((k = find(c, url)) >= 0) remove_entry(c, k);      /* an older copy is not served any more */
        c->refused++;
        return -1;
    }
    {
        int u = 0;
        while (url[u]) u++;
        if (u >= WF_CACHE_URL) { c->refused++; return -1; }
    }
    if ((k = find(c, url)) >= 0) remove_entry(c, k);
    for (guard = 0; guard <= WF_CACHE_ENTRIES; guard++) {
        int lru = -1, free_slot = -1;
        for (k = 0; k < WF_CACHE_ENTRIES; k++) {
            if (!c->e[k].used) { if (free_slot < 0) free_slot = k; continue; }
            if (lru < 0 || c->e[k].last_use < c->e[lru].last_use) lru = k;
        }
        if (free_slot >= 0 && c->pool_used + (uint32_t)len <= WF_CACHE_POOL) { slot = free_slot; break; }
        if (lru < 0) break;
        remove_entry(c, lru);
        c->evicted++;
    }
    if (slot < 0) { c->refused++; return -1; }
    e = &c->e[slot];
    e->used = 1;
    s_cpy(e->url, url, WF_CACHE_URL);
    s_cpy(e->ctype, ctype ? ctype : "", WF_CACHE_CTYPE);
    e->meta = *m;
    e->stored_at = now;
    e->last_use = ++c->stamp;
    e->off = c->pool_used;
    e->len = (uint32_t)len;
    for (k = 0; k < len; k++) c->pool[e->off + (uint32_t)k] = body[k];
    c->pool_used += (uint32_t)len;
    c->stored++;
    return slot;
}

void wf_cache_refresh(wf_cache_t* c, int idx, const wf_cache_meta_t* m, uint32_t now) {
    wf_cache_entry_t* e;
    if (idx < 0 || idx >= WF_CACHE_ENTRIES || !c->e[idx].used) return;
    e = &c->e[idx];
    e->stored_at = now;
    e->meta.max_age = m->max_age;
    e->meta.no_cache = m->no_cache;
    if (m->etag[0]) s_cpy(e->meta.etag, m->etag, WF_CACHE_TAG);
    if (m->last_modified[0]) s_cpy(e->meta.last_modified, m->last_modified, WF_CACHE_DATE);
    c->revalidated++;
}

int wf_cache_body(const wf_cache_t* c, int idx, uint8_t* out, int cap) {
    uint32_t k;
    if (idx < 0 || idx >= WF_CACHE_ENTRIES || !c->e[idx].used || (int)c->e[idx].len > cap) return -1;
    for (k = 0; k < c->e[idx].len; k++) out[k] = c->pool[c->e[idx].off + k];
    return (int)c->e[idx].len;
}

int wf_cache_drop(wf_cache_t* c, const char* url) {
    int k = find(c, url);
    if (k < 0) return 0;
    remove_entry(c, k);
    return 1;
}

static int put_s(char* o, int n, int cap, const char* s) { while (*s && n < cap - 1) o[n++] = *s++; return n; }
static int put_u(char* o, int n, int cap, uint32_t v) {
    char t[12];
    int k = 0;
    do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v && k < 11);
    while (k && n < cap - 1) o[n++] = t[--k];
    return n;
}

int wf_cache_text(const wf_cache_t* c, char* out, int cap) {
    int n = 0, k, used = 0;
    if (cap < 1) return 0;
    for (k = 0; k < WF_CACHE_ENTRIES; k++) used += c->e[k].used;
    n = put_s(out, n, cap, "entries "); n = put_u(out, n, cap, (uint32_t)used);
    n = put_s(out, n, cap, " bytes "); n = put_u(out, n, cap, c->pool_used);
    n = put_s(out, n, cap, " hits "); n = put_u(out, n, cap, c->hits);
    n = put_s(out, n, cap, " revalidated "); n = put_u(out, n, cap, c->revalidated);
    n = put_s(out, n, cap, " misses "); n = put_u(out, n, cap, c->misses);
    n = put_s(out, n, cap, " stored "); n = put_u(out, n, cap, c->stored);
    n = put_s(out, n, cap, " evicted "); n = put_u(out, n, cap, c->evicted);
    n = put_s(out, n, cap, " refused "); n = put_u(out, n, cap, c->refused);
    out[n] = 0;
    return n;
}
