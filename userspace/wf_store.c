/* wf_store.c - Lot 86: see wf_store.h. */
#include "wf_store.h"

static int slen(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static void smove(void *d, const void *s, uint32_t n) {
    uint8_t *a = (uint8_t *)d;
    const uint8_t *b = (const uint8_t *)s;
    while (n--) *a++ = *b++;
}
static void scpy(char *d, int cap, const char *s, int n) {
    int i;
    if (cap <= 0) return;
    if (n >= cap) n = cap - 1;
    for (i = 0; i < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}
static int app(char *o, int cap, int p, const char *s) {
    while (s && *s) { if (p >= cap - 1) return -1; o[p++] = *s++; }
    return p;
}
static int app_u(char *o, int cap, int p, uint32_t v) {
    char t[12];
    int n = 0;
    do { t[n++] = (char)('0' + (int)(v % 10U)); v /= 10U; } while (v && n < 11);
    while (n) { if (p >= cap - 1) return -1; o[p++] = t[--n]; }
    return p;
}
static int app_d(char *o, int cap, int p, long v) {
    if (v < 0) {
        if (p >= cap - 1) return -1;
        o[p++] = '-';
        v = -v;
    }
    return app_u(o, cap, p, (uint32_t)v);
}
static int hex_nib(int v) { return v < 10 ? '0' + v : 'a' + v - 10; }
static int from_hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int cookie_line(const wf_cookie_t *c, char *out, int cap) {
    int p = 0;
    char flags[8];
    flags[0] = c->host_only ? '1' : '0';
    flags[1] = c->secure ? '1' : '0';
    flags[2] = c->http_only ? '1' : '0';
    flags[3] = (char)('0' + (c->same_site > 3 ? 0 : c->same_site));
    flags[4] = 0;
    p = app(out, cap, p, c->name);
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\t';
    p = app(out, cap, p, c->domain);
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\t';
    p = app(out, cap, p, c->path);
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\t';
    p = app(out, cap, p, flags);
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\t';
    p = app(out, cap, p, c->expires);
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\t';
    p = app(out, cap, p, c->value);
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\n';
    out[p] = 0;
    return p;
}

int wf_store_cookies_save(const wf_cookie_jar_t *j, char *out, int cap) {
    int p = 0, i, n = 0;
    if (!j || !out || cap < 24) return -1;
    p = app(out, cap, p, "MOHHDY-COOKIES 1\n");
    if (p < 0) return -1;
    for (i = 0; i < WF_COOKIE_MAX; i++) {
        char line[WF_COOKIE_NAME + WF_COOKIE_VALUE + WF_COOKIE_DOMAIN + WF_COOKIE_PATH + 48];
        int L;
        if (!j->c[i].used) continue;
        L = cookie_line(&j->c[i], line, (int)sizeof(line));
        if (L < 0) continue;
        if (p + L >= cap) break;
        { int k; for (k = 0; k < L; k++) out[p++] = line[k]; }
        n++;
    }
    out[p] = 0;
    (void)n;
    return p;
}

static int field(char **p, char **out) {
    char *s = *p, *t;
    if (!s) return -1;
    t = s;
    while (*t && *t != '\t' && *t != '\n') t++;
    *out = s;
    if (*t == '\t') { *t = 0; *p = t + 1; return 0; }
    *t = 0;
    *p = t;
    return 0;
}

int wf_store_cookies_load(wf_cookie_jar_t *j, const char *text, int len) {
    static wf_cookie_jar_t tmp;
    int i = 0, line_no = 0;
    if (!j || !text || len < 0) return -1;
    wf_cookie_jar_init(&tmp);
    while (i < len) {
        char line[WF_COOKIE_NAME + WF_COOKIE_VALUE + WF_COOKIE_DOMAIN + WF_COOKIE_PATH + 64];
        int k = 0;
        char *p, *name, *dom, *path, *flags, *exp, *val;
        wf_cookie_t c;
        int z;
        while (i < len && text[i] != '\n' && k < (int)sizeof(line) - 1) line[k++] = text[i++];
        if (i < len && text[i] == '\n') i++;
        line[k] = 0;
        line_no++;
        if (k == 0) continue;
        if (line_no == 1) {
            if (slen(line) < 16 || line[0] != 'M') return -1;
            continue;
        }
        p = line;
        if (field(&p, &name) || field(&p, &dom) || field(&p, &path) || field(&p, &flags) ||
            field(&p, &exp) || field(&p, &val)) return -1;
        if (!name[0] || slen(name) >= WF_COOKIE_NAME || slen(val) >= WF_COOKIE_VALUE) return -1;
        if (slen(dom) >= WF_COOKIE_DOMAIN || slen(path) >= WF_COOKIE_PATH) return -1;
        if (slen(flags) < 4) return -1;
        for (z = 0; z < (int)sizeof(c); z++) ((uint8_t *)&c)[z] = 0;
        scpy(c.name, WF_COOKIE_NAME, name, slen(name));
        scpy(c.value, WF_COOKIE_VALUE, val, slen(val));
        scpy(c.domain, WF_COOKIE_DOMAIN, dom, slen(dom));
        scpy(c.path, WF_COOKIE_PATH, path, slen(path));
        c.host_only = flags[0] == '1';
        c.secure = flags[1] == '1';
        c.http_only = flags[2] == '1';
        c.same_site = (uint8_t)(flags[3] >= '0' && flags[3] <= '3' ? flags[3] - '0' : 0);
        if (slen(exp) == 14) scpy(c.expires, 15, exp, 14);
        c.used = 1;
        c.order = ++tmp.clock;
        if (tmp.set >= WF_COOKIE_MAX) return -1;
        tmp.c[tmp.set++] = c;
    }
    tmp.enabled = j->enabled;
    smove(j, &tmp, (uint32_t)sizeof(tmp));
    return 0;
}

static int cache_line(const wf_cache_t *c, int idx, char *out, int cap) {
    const wf_cache_entry_t *e = &c->e[idx];
    int p = 0, k;
    if ((int)e->len > WF_STORE_BODY) return 0;
    p = app(out, cap, p, e->url);
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\t';
    p = app_u(out, cap, p, e->stored_at);
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\t';
    p = app_d(out, cap, p, e->meta.max_age);
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\t';
    p = app_u(out, cap, p, (uint32_t)(e->meta.no_cache ? 1 : 0));
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\t';
    p = app_u(out, cap, p, (uint32_t)(e->meta.age > 0 ? e->meta.age : 0));
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\t';
    p = app(out, cap, p, e->meta.etag);
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\t';
    p = app(out, cap, p, e->meta.last_modified);
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\t';
    p = app(out, cap, p, e->ctype);
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\t';
    for (k = 0; k < (int)e->len; k++) {
        unsigned char b = c->pool[e->off + (uint32_t)k];
        if (p + 2 >= cap) return -1;
        out[p++] = (char)hex_nib(b >> 4);
        out[p++] = (char)hex_nib(b & 15);
    }
    if (p >= cap - 1) return -1;
    out[p++] = '\n';
    out[p] = 0;
    return p;
}

int wf_store_cache_save(const wf_cache_t *c, char *out, int cap) {
    int p = 0, i;
    if (!c || !out || cap < 20) return -1;
    p = app(out, cap, p, "MOHHDY-CACHE 1\n");
    if (p < 0) return -1;
    for (i = 0; i < WF_CACHE_ENTRIES; i++) {
        /* static: a hex body is a few KiB, too large for the shell stack */
        static char line[WF_CACHE_URL + WF_CACHE_TAG + WF_CACHE_DATE + WF_CACHE_CTYPE + WF_STORE_BODY * 2 + 64];
        int L;
        if (!c->e[i].used) continue;
        L = cache_line(c, i, line, (int)sizeof(line));
        if (L <= 0) continue;
        if (p + L >= cap) break;
        { int k; for (k = 0; k < L; k++) out[p++] = line[k]; }
    }
    out[p] = 0;
    return p;
}

static int parse_i(const char *s, long *out) {
    int i = 0, neg = 0;
    long v = 0;
    if (s[0] == '-') { neg = 1; i++; }
    if (s[i] < '0' || s[i] > '9') return -1;
    while (s[i] >= '0' && s[i] <= '9') {
        if (v > 100000000L) return -1;
        v = v * 10 + (s[i] - '0');
        i++;
    }
    if (s[i]) return -1;
    *out = neg ? -v : v;
    return 0;
}

int wf_store_cache_load(wf_cache_t *c, const char *text, int len) {
    /* The pool is 512 KiB. A second copy stays in BSS, not on the shell stack. */
    static wf_cache_t tmp;
    int i = 0, line_no = 0;
    uint8_t body[WF_STORE_BODY];
    if (!c || !text || len < 0) return -1;
    wf_cache_init(&tmp);
    while (i < len) {
        static char line[WF_CACHE_URL + WF_CACHE_TAG + WF_CACHE_DATE + WF_CACHE_CTYPE + WF_STORE_BODY * 2 + 80];
        int k = 0;
        char *p, *url, *st, *age, *nc, *ag, *etag, *lm, *ctype, *hex;
        long stored, max_age, no_cache, age_s;
        wf_cache_meta_t m;
        int bl = 0, z;
        while (i < len && text[i] != '\n' && k < (int)sizeof(line) - 1) line[k++] = text[i++];
        if (i < len && text[i] == '\n') i++;
        line[k] = 0;
        line_no++;
        if (k == 0) continue;
        if (line_no == 1) {
            if (line[0] != 'M') return -1;
            continue;
        }
        p = line;
        if (field(&p, &url) || field(&p, &st) || field(&p, &age) || field(&p, &nc) || field(&p, &ag) ||
            field(&p, &etag) || field(&p, &lm) || field(&p, &ctype) || field(&p, &hex)) return -1;
        if (parse_i(st, &stored) || parse_i(age, &max_age) || parse_i(nc, &no_cache) || parse_i(ag, &age_s)) return -1;
        if ((int)slen(hex) % 2) return -1;
        if (slen(hex) / 2 > WF_STORE_BODY) return -1;
        for (z = 0; hex[z]; z += 2) {
            int h = from_hex(hex[z]), l = from_hex(hex[z + 1]);
            if (h < 0 || l < 0) return -1;
            body[bl++] = (uint8_t)((h << 4) | l);
        }
        for (z = 0; z < (int)sizeof(m); z++) ((uint8_t *)&m)[z] = 0;
        m.store = 1;
        m.no_cache = no_cache ? 1 : 0;
        m.max_age = max_age;
        m.age = (int)age_s;
        scpy(m.etag, WF_CACHE_TAG, etag, slen(etag));
        scpy(m.last_modified, WF_CACHE_DATE, lm, slen(lm));
        if (wf_cache_store(&tmp, url, &m, ctype, body, bl, (uint32_t)stored) < 0) return -1;
    }
    smove(c, &tmp, (uint32_t)sizeof(tmp));
    return 0;
}
