/* userspace/webrt.c - see webrt.h (Lot 5, Phase 3 building blocks). */
#include "webrt.h"

/* ---------- small helpers (no libc in the guest) ---------- */
static int w_len(const char* s) { int n = 0; if (!s) return 0; while (s[n]) n++; return n; }
static char w_low(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c; }
static int w_space(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }
static int w_ieqn(const char* a, const char* b, int n) {
    int i;
    for (i = 0; i < n; i++) {
        if (w_low(a[i]) != w_low(b[i])) return 0;
        if (!a[i]) return 1;
    }
    return 1;
}
static int w_ieq(const char* a, const char* b) {
    int n = w_len(b);
    return w_len(a) == n && w_ieqn(a, b, n);
}
static int w_eq(const char* a, const char* b) {
    int i = 0;
    if (!a || !b) return 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}
static void w_copy(char* d, int max, const char* s, int n) {
    int i;
    if (max <= 0) return;
    if (n < 0) n = w_len(s);
    for (i = 0; i < n && i < max - 1 && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}
static int w_atoi(const char* s, int n) {
    int v = 0, i = 0;
    while (i < n && w_space(s[i])) i++;
    while (i < n && s[i] >= '0' && s[i] <= '9') { v = v * 10 + (s[i] - '0'); i++; }
    return v;
}
/* case-insensitive search of needle in s[0..n) */
static int w_find_ci(const char* s, int n, const char* needle) {
    int k = w_len(needle), i;
    for (i = 0; i + k <= n; i++) if (w_ieqn(s + i, needle, k)) return i;
    return -1;
}

typedef struct { char* o; int n, max, trunc; } w_out_t;
static void o_init(w_out_t* w, char* o, int max) { w->o = o; w->n = 0; w->max = max; w->trunc = 0; if (o && max > 0) o[0] = 0; }
static void o_putn(w_out_t* w, const char* s, int n) {
    int i;
    if (!w->o || w->max <= 0) return;
    for (i = 0; i < n; i++) {
        if (w->n >= w->max - 1) { w->trunc = 1; break; }
        w->o[w->n++] = s[i];
    }
    w->o[w->n] = 0;
}
static void o_puts(w_out_t* w, const char* s) { o_putn(w, s, w_len(s)); }

/* ---------- US-038 cache ---------- */
static void cc_parse(const char* cc, uint32_t* max_age, int* no_store, int* no_cache) {
    int i = 0, n = w_len(cc);
    *max_age = 0; *no_store = 0; *no_cache = 0;
    while (i < n) {
        int s, e;
        while (i < n && (w_space(cc[i]) || cc[i] == ',')) i++;
        s = i;
        while (i < n && cc[i] != ',') i++;
        e = i;
        while (e > s && w_space(cc[e - 1])) e--;
        if (e - s == 8 && w_ieqn(cc + s, "no-store", 8)) *no_store = 1;
        else if (e - s == 8 && w_ieqn(cc + s, "no-cache", 8)) *no_cache = 1;
        else if (e - s > 8 && w_ieqn(cc + s, "max-age=", 8)) *max_age = (uint32_t)w_atoi(cc + s + 8, e - s - 8);
    }
}

void webrt_cache_init(webrt_cache_t* c) {
    uint32_t i;
    uint8_t* p = (uint8_t*)c;
    if (!c) return;
    for (i = 0; i < sizeof(*c); i++) p[i] = 0;
}

static int cache_find(webrt_cache_t* c, const char* url) {
    int i;
    for (i = 0; i < WEBRT_CACHE_SLOTS; i++) if (c->e[i].used && w_eq(c->e[i].url, url)) return i;
    return -1;
}

int webrt_cache_store(webrt_cache_t* c, const char* url, const char* cache_control,
                      const char* etag, uint32_t bytes, uint32_t now) {
    uint32_t max_age;
    int no_store, no_cache, slot, i;
    if (!c || !url || !url[0]) return -1;
    cc_parse(cache_control, &max_age, &no_store, &no_cache);
    slot = cache_find(c, url);
    if (no_store || w_len(url) >= (int)sizeof(c->e[0].url)) {
        if (slot >= 0) c->e[slot].used = 0;
        c->refused++;
        return -1;
    }
    if (slot < 0) {
        for (i = 0; i < WEBRT_CACHE_SLOTS; i++) if (!c->e[i].used) { slot = i; break; }
    }
    if (slot < 0) {
        slot = 0;
        for (i = 1; i < WEBRT_CACHE_SLOTS; i++) if (c->e[i].last_use < c->e[slot].last_use) slot = i;
        c->evictions++;
    }
    w_copy(c->e[slot].url, sizeof(c->e[slot].url), url, -1);
    w_copy(c->e[slot].etag, sizeof(c->e[slot].etag), etag ? etag : "", -1);
    c->e[slot].stored = now;
    c->e[slot].max_age = max_age;
    c->e[slot].no_cache = (uint8_t)no_cache;
    c->e[slot].bytes = bytes;
    c->e[slot].last_use = ++c->clock;
    c->e[slot].used = 1;
    c->stores++;
    return slot;
}

int webrt_cache_lookup(webrt_cache_t* c, const char* url, uint32_t now, const char** etag) {
    int slot;
    if (etag) *etag = 0;
    if (!c || !url) return WEBRT_CACHE_MISS;
    slot = cache_find(c, url);
    if (slot < 0) { c->misses++; return WEBRT_CACHE_MISS; }
    c->e[slot].last_use = ++c->clock;
    if (c->e[slot].no_cache || now - c->e[slot].stored >= c->e[slot].max_age) {
        c->stale++;
        if (etag) *etag = c->e[slot].etag;
        return WEBRT_CACHE_STALE;
    }
    c->hits++;
    return WEBRT_CACHE_FRESH;
}

int webrt_cache_revalidated(webrt_cache_t* c, const char* url, int status, const char* cache_control,
                            const char* etag, uint32_t bytes, uint32_t now) {
    int slot;
    if (!c || !url) return 0;
    slot = cache_find(c, url);
    if (status == 304 && slot >= 0) {
        uint32_t max_age;
        int no_store, no_cache;
        if (cache_control && cache_control[0]) {
            cc_parse(cache_control, &max_age, &no_store, &no_cache);
            if (no_store) { c->e[slot].used = 0; c->refused++; return 0; }
            c->e[slot].max_age = max_age;
            c->e[slot].no_cache = (uint8_t)no_cache;
        }
        c->e[slot].stored = now;
        c->revalidated++;
        return 1;
    }
    if (status == 200) return webrt_cache_store(c, url, cache_control, etag, bytes, now) >= 0;
    if (slot >= 0) c->e[slot].used = 0;
    return 0;
}

/* ---------- US-039 security ---------- */
int webrt_csp_parse(webrt_csp_t* p, const char* header) {
    int i = 0, n = w_len(header);
    if (!p) return 0;
    p->count = 0; p->errors = 0;
    while (i < n) {
        int s, e, ns, ne, k, j, dup = 0;
        char name[16];
        while (i < n && (w_space(header[i]) || header[i] == ';')) i++;
        s = i;
        while (i < n && header[i] != ';') i++;
        e = i;
        if (e <= s) continue;
        ns = s; ne = s;
        while (ne < e && !w_space(header[ne])) ne++;
        if (ne - ns >= (int)sizeof(name)) { p->errors++; continue; }
        for (k = 0; k < ne - ns; k++) name[k] = w_low(header[ns + k]);
        name[k] = 0;
        for (j = 0; j < p->count; j++) if (w_eq(p->d[j].name, name)) dup = 1;
        if (dup) continue;                       /* first occurrence wins */
        if (p->count >= WEBRT_CSP_MAX) { p->errors++; continue; }
        w_copy(p->d[p->count].name, sizeof(p->d[0].name), name, -1);
        {   /* sources: lower case, single spaces */
            char* d = p->d[p->count].sources;
            int o = 0, sp = 0;
            for (k = ne; k < e && o < (int)sizeof(p->d[0].sources) - 1; k++) {
                if (w_space(header[k])) { sp = o > 0; continue; }
                if (sp) { d[o++] = ' '; sp = 0; if (o >= (int)sizeof(p->d[0].sources) - 1) break; }
                d[o++] = w_low(header[k]);
            }
            d[o] = 0;
        }
        p->count++;
    }
    return p->count;
}

int webrt_url_origin(const char* url, char* out, int max) {
    int i = 0, n = w_len(url), s, k;
    if (out && max > 0) out[0] = 0;
    while (i + 2 < n && !(url[i] == ':' && url[i + 1] == '/' && url[i + 2] == '/')) {
        if (url[i] == '/' || url[i] == '?' || url[i] == '#') return 0;
        i++;
    }
    if (i + 2 >= n || i == 0) return 0;
    s = i + 3;
    k = s;
    while (k < n && url[k] != '/' && url[k] != '?' && url[k] != '#') k++;
    if (out && max > 0) {
        int o = 0, j;
        for (j = 0; j < k && o < max - 1; j++) out[o++] = w_low(url[j]);
        out[o] = 0;
    }
    return 1;
}

/* host[:port] part of an absolute URL into h. */
static void url_host(const char* url, char* h, int max) {
    char origin[128];
    int i = 0;
    h[0] = 0;
    if (!webrt_url_origin(url, origin, sizeof(origin))) return;
    while (origin[i] && !(origin[i] == ':' && origin[i + 1] == '/')) i++;
    if (origin[i]) w_copy(h, max, origin + i + 3, -1);
}

static int host_match(const char* pattern, int plen, const char* host) {
    int hl = w_len(host);
    if (plen >= 2 && pattern[0] == '*' && pattern[1] == '.') {
        int sl = plen - 1;                    /* ".example.com" */
        return hl > sl && w_ieqn(host + hl - sl, pattern + 1, sl);
    }
    return hl == plen && w_ieqn(host, pattern, plen);
}

static int csp_source_allows(const char* tok, int tl, const char* page_origin, const char* url) {
    char origin[128], host[96];
    int inline_req = w_eq(url, "inline");
    int absolute = webrt_url_origin(url, origin, sizeof(origin));
    if (tl == 15 && w_ieqn(tok, "'unsafe-inline'", 15)) return inline_req;
    if (inline_req) return 0;
    if (tl == 6 && w_ieqn(tok, "'self'", 6)) return !absolute || w_ieq(origin, page_origin);
    if (tl == 6 && w_ieqn(tok, "'none'", 6)) return 0;
    if (tl == 1 && tok[0] == '*') {
        return !absolute || w_ieqn(url, "http:", 5) || w_ieqn(url, "https:", 6);
    }
    if (tl > 1 && tok[tl - 1] == ':') {       /* scheme source */
        return absolute && w_ieqn(url, tok, tl);
    }
    if (!absolute) return 0;
    url_host(url, host, sizeof(host));
    {
        int k = 0;
        while (k + 2 < tl && !(tok[k] == ':' && tok[k + 1] == '/' && tok[k + 2] == '/')) k++;
        if (k + 2 < tl) {                     /* scheme://host */
            if (!w_ieqn(url, tok, k + 3)) return 0;
            return host_match(tok + k + 3, tl - k - 3, host);
        }
    }
    /* bare host: same scheme family as the page (https only upgrades) */
    return host_match(tok, tl, host);
}

int webrt_csp_allows(const webrt_csp_t* p, const char* directive, const char* page_origin, const char* url) {
    const webrt_csp_directive_t* d = 0;
    int i;
    if (!p || !directive || !url) return 0;
    for (i = 0; i < p->count; i++) if (w_eq(p->d[i].name, directive)) d = &p->d[i];
    if (!d) {
        int dl = w_len(directive);
        if (dl > 4 && w_eq(directive + dl - 4, "-src") && !w_eq(directive, "default-src"))
            for (i = 0; i < p->count; i++) if (w_eq(p->d[i].name, "default-src")) d = &p->d[i];
    }
    if (!d) return 1;                         /* no policy for it */
    {
        const char* s = d->sources;
        int k = 0, n = w_len(s);
        while (k < n) {
            int b = k;
            while (k < n && s[k] != ' ') k++;
            if (k > b && csp_source_allows(s + b, k - b, page_origin ? page_origin : "", url)) return 1;
            while (k < n && s[k] == ' ') k++;
        }
    }
    return 0;
}

static int is_name_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == ':';
}

/* Reads a tag name at s[i] (after '<' and optional '/'); returns its length. */
static int tag_name(const char* s, int i, int n, char* name, int max) {
    int k = 0;
    while (i + k < n && is_name_char(s[i + k])) {
        if (k < max - 1) name[k] = w_low(s[i + k]);
        k++;
    }
    name[k < max - 1 ? k : max - 1] = 0;
    return k;
}

/* End of the tag starting at '<' s[i] (index of '>', or n). Quotes honoured. */
static int tag_end(const char* s, int i, int n) {
    char q = 0;
    for (i++; i < n; i++) {
        if (q) { if (s[i] == q) q = 0; continue; }
        if (s[i] == '"' || s[i] == '\'') q = s[i];
        else if (s[i] == '>') return i;
    }
    return n;
}

static int dangerous_element(const char* name) {
    return w_eq(name, "script") || w_eq(name, "iframe") || w_eq(name, "object") ||
           w_eq(name, "embed") || w_eq(name, "style");
}

int webrt_sanitize_html(const char* in, char* out, int max, int* removed) {
    w_out_t w;
    int i = 0, n = w_len(in), rm = 0;
    o_init(&w, out, max);
    while (i < n) {
        if (in[i] == '<' && i + 1 < n && (is_name_char(in[i + 1]) || in[i + 1] == '/')) {
            char name[16];
            int closing = in[i + 1] == '/';
            int ns = i + 1 + closing;
            int nl = tag_name(in, ns, n, name, sizeof(name));
            int te = tag_end(in, i, n);
            if (nl == 0) { o_putn(&w, in + i, 1); i++; continue; }
            if (dangerous_element(name)) {
                rm++;
                if (!closing) {
                    /* skip to the matching close tag */
                    char close[20];
                    int c, rest;
                    close[0] = '<'; close[1] = '/';
                    w_copy(close + 2, sizeof(close) - 2, name, -1);
                    rest = te < n ? te + 1 : n;
                    c = w_find_ci(in + rest, n - rest, close);
                    if (c < 0) { i = n; break; }
                    i = tag_end(in, rest + c, n) + 1;
                } else {
                    i = te + 1;
                }
                continue;
            }
            /* copy the tag, filtering attributes */
            o_putn(&w, in + i, ns - i + nl);
            {
                int k = ns + nl;
                while (k < te) {
                    int as, an, ae, vs = -1, ve = -1, drop = 0;
                    while (k < te && w_space(in[k])) k++;
                    if (k >= te) break;
                    if (in[k] == '/') { o_putn(&w, " /", 2); k++; continue; }
                    as = k;
                    while (k < te && !w_space(in[k]) && in[k] != '=' && in[k] != '/') k++;
                    an = k - as;
                    if (an == 0) { k++; continue; }
                    while (k < te && w_space(in[k])) k++;
                    if (k < te && in[k] == '=') {
                        k++;
                        while (k < te && w_space(in[k])) k++;
                        if (k < te && (in[k] == '"' || in[k] == '\'')) {
                            char q = in[k++];
                            vs = k;
                            while (k < te && in[k] != q) k++;
                            ve = k;
                            if (k < te) k++;
                        } else {
                            vs = k;
                            while (k < te && !w_space(in[k])) k++;
                            ve = k;
                        }
                    }
                    ae = k;
                    if (an > 2 && w_low(in[as]) == 'o' && w_low(in[as + 1]) == 'n') drop = 1;
                    if (!drop && vs >= 0 &&
                        ((an == 4 && w_ieqn(in + as, "href", 4)) || (an == 3 && w_ieqn(in + as, "src", 3)) ||
                         (an == 6 && w_ieqn(in + as, "action", 6)) || (an == 10 && w_ieqn(in + as, "formaction", 10)))) {
                        int v = vs;
                        while (v < ve && (w_space(in[v]) || (unsigned char)in[v] < 32)) v++;
                        if (ve - v >= 11 && w_ieqn(in + v, "javascript:", 11)) drop = 1;
                    }
                    if (drop) { rm++; continue; }
                    o_putn(&w, " ", 1);
                    o_putn(&w, in + as, ae - as);
                }
            }
            if (te < n) o_putn(&w, ">", 1);
            i = te + 1;
            continue;
        }
        o_putn(&w, in + i, 1);
        i++;
    }
    if (removed) *removed = rm;
    return w.n;
}

/* ---------- US-040 media queries ---------- */
static int mq_length(const char* v, int n) {
    int px = w_atoi(v, n), i = 0;
    while (i < n && (w_space(v[i]) || (v[i] >= '0' && v[i] <= '9'))) i++;
    if (i + 2 <= n && w_ieqn(v + i, "em", 2)) return px * 16;
    return px;
}

static int mq_term(const char* t, int n, const webrt_viewport_t* v) {
    while (n > 0 && w_space(t[0])) { t++; n--; }
    while (n > 0 && w_space(t[n - 1])) n--;
    if (n == 3 && w_ieqn(t, "all", 3)) return 1;
    if (n == 6 && w_ieqn(t, "screen", 6)) return 1;
    if (n >= 2 && t[0] == '(' && t[n - 1] == ')') {
        const char* f = t + 1;
        int fl = n - 2, c = 0, vs, vl, kl;
        while (c < fl && f[c] != ':') c++;
        if (c >= fl) return 0;                 /* boolean features: unsupported */
        kl = c;
        while (kl > 0 && w_space(f[kl - 1])) kl--;
        vs = c + 1;
        while (vs < fl && w_space(f[vs])) vs++;
        vl = fl - vs;
        while (vl > 0 && w_space(f[vs + vl - 1])) vl--;
        if (kl == 9 && w_ieqn(f, "min-width", 9)) return v->width >= mq_length(f + vs, vl);
        if (kl == 9 && w_ieqn(f, "max-width", 9)) return v->width <= mq_length(f + vs, vl);
        if (kl == 10 && w_ieqn(f, "min-height", 10)) return v->height >= mq_length(f + vs, vl);
        if (kl == 10 && w_ieqn(f, "max-height", 10)) return v->height <= mq_length(f + vs, vl);
        if (kl == 11 && w_ieqn(f, "orientation", 11)) {
            int portrait = v->height >= v->width;
            if (vl == 8 && w_ieqn(f + vs, "portrait", 8)) return portrait;
            if (vl == 9 && w_ieqn(f + vs, "landscape", 9)) return !portrait;
            return 0;
        }
        if (kl == 20 && w_ieqn(f, "prefers-color-scheme", 20)) {
            if (vl == 4 && w_ieqn(f + vs, "dark", 4)) return v->dark != 0;
            if (vl == 5 && w_ieqn(f + vs, "light", 5)) return v->dark == 0;
            return 0;
        }
        return 0;
    }
    return 0;                                  /* print, speech, unknown */
}

static int mq_one(const char* q, int n, const webrt_viewport_t* v) {
    int neg = 0, i = 0, ok = 1, start;
    while (i < n && w_space(q[i])) i++;
    if (n - i >= 4 && w_ieqn(q + i, "not ", 4)) { neg = 1; i += 4; }
    else if (n - i >= 5 && w_ieqn(q + i, "only ", 5)) i += 5;
    start = i;
    if (start >= n) return 0;
    while (i <= n) {
        int a = (i < n) ? w_find_ci(q + i, n - i, " and ") : -1;
        int e = a < 0 ? n : i + a;
        if (!mq_term(q + start, e - start, v)) ok = 0;
        if (a < 0) break;
        i = e + 5;
        start = i;
    }
    return neg ? !ok : ok;
}

int webrt_mq_match(const char* query, const webrt_viewport_t* v) {
    int n = w_len(query), i = 0, s = 0, depth = 0;
    if (!v) return 0;
    if (n == 0) return 1;
    for (i = 0; i <= n; i++) {
        if (i < n && query[i] == '(') depth++;
        if (i < n && query[i] == ')') depth--;
        if (i == n || (query[i] == ',' && depth == 0)) {
            if (mq_one(query + s, i - s, v)) return 1;
            s = i + 1;
        }
    }
    return 0;
}

static int block_end(const char* s, int i, int n) {   /* s[i] == '{' : index of matching '}' */
    int depth = 0;
    for (; i < n; i++) {
        if (s[i] == '{') depth++;
        else if (s[i] == '}') { depth--; if (depth == 0) return i; }
    }
    return n;
}

static void flatten(const char* css, int n, const webrt_viewport_t* v, w_out_t* w, int* kept, int* dropped, int level) {
    int i = 0;
    while (i < n) {
        if (css[i] == '@' && n - i >= 6 && w_ieqn(css + i, "@media", 6)) {
            int q = i + 6, b = q, e;
            char query[128];
            while (b < n && css[b] != '{' && css[b] != ';') b++;
            if (b >= n || css[b] == ';') { i = b + 1; continue; }
            e = block_end(css, b, n);
            w_copy(query, sizeof(query), css + q, b - q);
            if (webrt_mq_match(query, v) && level < 4) {
                (*kept)++;
                flatten(css + b + 1, e - b - 1, v, w, kept, dropped, level + 1);
            } else {
                (*dropped)++;
            }
            i = e + 1;
            continue;
        }
        o_putn(w, css + i, 1);
        i++;
    }
}

int webrt_css_flatten_media(const char* css, const webrt_viewport_t* v, char* out, int max,
                            int* kept, int* dropped) {
    w_out_t w;
    int k = 0, d = 0;
    o_init(&w, out, max);
    if (css && v) flatten(css, w_len(css), v, &w, &k, &d, 0);
    if (kept) *kept = k;
    if (dropped) *dropped = d;
    return w.n;
}

/* ---------- US-041 custom properties ---------- */
#define WEBRT_VARS 16
typedef struct { char name[32]; char value[64]; } w_var_t;

static int var_lookup(const w_var_t* vars, int count, const char* name, int nl) {
    int i;
    for (i = count - 1; i >= 0; i--)
        if (w_len(vars[i].name) == nl && w_ieqn(vars[i].name, name, nl)) return i;
    return -1;
}

/* Copies s[0..n) resolving var(); returns unresolved count. */
static int subst_vars(const char* s, int n, const w_var_t* vars, int count, w_out_t* w, int level) {
    int i = 0, unresolved = 0;
    while (i < n) {
        if (n - i >= 4 && w_ieqn(s + i, "var(", 4)) {
            int k = i + 4, ns, ne, depth = 1, fb = -1, close;
            while (k < n && w_space(s[k])) k++;
            ns = k;
            while (k < n && s[k] != ',' && s[k] != ')' && !w_space(s[k])) k++;
            ne = k;
            close = k;
            while (close < n && depth > 0) {
                if (s[close] == '(') depth++;
                else if (s[close] == ')') { depth--; if (depth == 0) break; }
                else if (s[close] == ',' && depth == 1 && fb < 0) fb = close + 1;
                close++;
            }
            {
                int vi = var_lookup(vars, count, s + ns, ne - ns);
                if (vi >= 0) {
                    o_puts(w, vars[vi].value);
                } else if (fb >= 0 && level < 3) {
                    int fs = fb, fe = close;
                    while (fs < fe && w_space(s[fs])) fs++;
                    while (fe > fs && w_space(s[fe - 1])) fe--;
                    unresolved += subst_vars(s + fs, fe - fs, vars, count, w, level + 1);
                } else {
                    unresolved++;
                }
            }
            i = close < n ? close + 1 : n;
            continue;
        }
        o_putn(w, s + i, 1);
        i++;
    }
    return unresolved;
}

int webrt_css_resolve_vars(const char* css, char* out, int max, int* defined, int* unresolved) {
    w_var_t vars[WEBRT_VARS];
    int count = 0, n = w_len(css), i, decl = 1, unres = 0;
    w_out_t w;
    /* pass 1: collect --name: value (values may use earlier vars) */
    for (i = 0; i < n; i++) {
        char c = css[i];
        if (c == '{' || c == ';' || c == '}') { decl = 1; continue; }
        if (w_space(c)) continue;
        if (decl && c == '-' && i + 1 < n && css[i + 1] == '-') {
            int ns = i, ne = i, vs, ve, slot;
            while (ne < n && css[ne] != ':' && css[ne] != ';' && css[ne] != '}' && !w_space(css[ne])) ne++;
            vs = ne;
            while (vs < n && w_space(css[vs])) vs++;
            if (vs >= n || css[vs] != ':') { decl = 0; continue; }
            vs++;
            while (vs < n && w_space(css[vs])) vs++;
            ve = vs;
            while (ve < n && css[ve] != ';' && css[ve] != '}') ve++;
            i = ve - 1;
            while (ve > vs && w_space(css[ve - 1])) ve--;
            if (ne - ns >= 32) { decl = 0; continue; }
            slot = var_lookup(vars, count, css + ns, ne - ns);
            if (slot < 0) {
                if (count >= WEBRT_VARS) { decl = 0; continue; }
                slot = count++;
            }
            w_copy(vars[slot].name, sizeof(vars[slot].name), css + ns, ne - ns);
            {
                char tmp[64];
                w_out_t tw;
                o_init(&tw, tmp, sizeof(tmp));
                (void)subst_vars(css + vs, ve - vs, vars, count, &tw, 0);
                w_copy(vars[slot].value, sizeof(vars[slot].value), tmp, -1);
            }
            decl = 0;
            continue;
        }
        decl = 0;
    }
    /* pass 2: drop the declarations, substitute the uses */
    o_init(&w, out, max);
    decl = 1;
    i = 0;
    while (i < n) {
        char c = css[i];
        if (decl && c == '-' && i + 1 < n && css[i + 1] == '-') {
            while (i < n && css[i] != ';' && css[i] != '}') i++;
            if (i < n && css[i] == ';') i++;
            continue;
        }
        if (c == '{' || c == ';' || c == '}') { decl = 1; o_putn(&w, css + i, 1); i++; continue; }
        if (w_space(c)) { o_putn(&w, css + i, 1); i++; continue; }
        decl = 0;
        {   /* copy up to the next delimiter with substitution */
            int e = i;
            while (e < n && css[e] != ';' && css[e] != '{' && css[e] != '}') e++;
            unres += subst_vars(css + i, e - i, vars, count, &w, 0);
            i = e;
        }
    }
    if (defined) *defined = count;
    if (unresolved) *unresolved = unres;
    return w.n;
}

/* ---------- US-042 Web Components ---------- */
#define WEBRT_COMPONENTS 8
typedef struct { char name[32]; int body, body_len; } w_comp_t;

/* value of attribute `attr` inside the tag s[ts..te) into val (1 if present) */
static int tag_attr(const char* s, int ts, int te, const char* attr, char* val, int max) {
    int al = w_len(attr), k = ts + 1;
    if (val && max > 0) val[0] = 0;
    while (k < te && is_name_char(s[k])) k++;        /* tag name */
    while (k < te) {
        int as, an, vs = -1, ve = -1;
        while (k < te && w_space(s[k])) k++;
        as = k;
        while (k < te && !w_space(s[k]) && s[k] != '=' && s[k] != '>' && s[k] != '/') k++;
        an = k - as;
        if (an == 0) { k++; continue; }
        while (k < te && w_space(s[k])) k++;
        if (k < te && s[k] == '=') {
            k++;
            while (k < te && w_space(s[k])) k++;
            if (k < te && (s[k] == '"' || s[k] == '\'')) {
                char q = s[k++];
                vs = k;
                while (k < te && s[k] != q) k++;
                ve = k;
                if (k < te) k++;
            } else {
                vs = k;
                while (k < te && !w_space(s[k]) && s[k] != '>') k++;
                ve = k;
            }
        }
        if (an == al && w_ieqn(s + as, attr, al)) {
            if (val && max > 0 && vs >= 0) w_copy(val, max, s + vs, ve - vs);
            return 1;
        }
    }
    return 0;
}

int webrt_components_expand(const char* html, char* out, int max, int* defined, int* expanded) {
    w_comp_t comps[WEBRT_COMPONENTS];
    int count = 0, n = w_len(html), i = 0, ex = 0;
    w_out_t w;
    /* pass 1: <template id="x-name"> bodies */
    while (i < n) {
        int t = w_find_ci(html + i, n - i, "<template");
        int ts, te, c;
        char id[32];
        if (t < 0) break;
        ts = i + t;
        te = tag_end(html, ts, n);
        if (te >= n) break;
        c = w_find_ci(html + te + 1, n - te - 1, "</template");
        if (c < 0) break;
        if (tag_attr(html, ts, te, "id", id, sizeof(id))) {
            int k, dash = 0, dup = 0;
            for (k = 0; id[k]; k++) if (id[k] == '-') dash = 1;
            for (k = 0; k < count; k++) if (w_ieq(comps[k].name, id)) dup = 1;
            if (dash && !dup && count < WEBRT_COMPONENTS && id[0] >= 'a' && id[0] <= 'z') {
                w_copy(comps[count].name, sizeof(comps[count].name), id, -1);
                comps[count].body = te + 1;
                comps[count].body_len = c;
                count++;
            }
        }
        i = te + 1 + c;
    }
    /* pass 2 */
    o_init(&w, out, max);
    i = 0;
    while (i < n) {
        if (html[i] == '<' && i + 1 < n && is_name_char(html[i + 1])) {
            char name[32];
            int nl = tag_name(html, i + 1, n, name, sizeof(name));
            int te = tag_end(html, i, n), k, found = -1;
            if (w_eq(name, "template")) {
                int c = w_find_ci(html + i, n - i, "</template");
                if (c < 0) { i = n; break; }
                i = tag_end(html, i + c, n) + 1;
                continue;
            }
            for (k = 0; k < count && nl < 32; k++) if (w_eq(comps[k].name, name)) found = k;
            if (found >= 0) {
                char close[40];
                int inner = te + 1, c, ie, after;
                const char* body = html + comps[found].body;
                int bl = comps[found].body_len, slot;
                close[0] = '<'; close[1] = '/';
                w_copy(close + 2, sizeof(close) - 2, name, -1);
                c = inner < n ? w_find_ci(html + inner, n - inner, close) : -1;
                ie = c < 0 ? inner : inner + c;
                after = c < 0 ? inner : tag_end(html, ie, n) + 1;
                slot = w_find_ci(body, bl, "<slot");
                if (slot >= 0) {
                    int se = tag_end(body, slot, bl) + 1, sc;
                    o_putn(&w, body, slot);
                    o_putn(&w, html + inner, ie - inner);
                    sc = w_find_ci(body + se, bl - se, "</slot>");
                    if (sc >= 0) se += sc + 7;
                    o_putn(&w, body + se, bl - se);
                } else {
                    o_putn(&w, body, bl);
                }
                ex++;
                i = after;
                continue;
            }
        }
        o_putn(&w, html + i, 1);
        i++;
    }
    if (defined) *defined = count;
    if (expanded) *expanded = ex;
    return w.n;
}

/* ---------- US-044 accessibility ---------- */
static int text_until_close(const char* s, int from, int n, const char* close) {
    int c = w_find_ci(s + from, n - from, close), k, in_tag = 0;
    int end = c < 0 ? n : from + c;
    for (k = from; k < end; k++) {
        if (s[k] == '<') in_tag = 1;
        else if (s[k] == '>') in_tag = 0;
        else if (!in_tag && !w_space(s[k])) return 1;
    }
    return 0;
}

int webrt_a11y_audit(const char* html, webrt_a11y_t* r) {
    char label_for[16][32];
    int labels = 0, n = w_len(html), i, prev_h = 0, label_depth = 0;
    uint32_t z;
    if (!r) return 0;
    for (z = 0; z < sizeof(*r); z++) ((uint8_t*)r)[z] = 0;
    /* pre-scan <label for=...> */
    for (i = 0; i < n; i++) {
        if (html[i] == '<' && n - i >= 6 && w_ieqn(html + i, "<label", 6)) {
            int te = tag_end(html, i, n);
            if (labels < 16 && tag_attr(html, i, te, "for", label_for[labels], 32)) labels++;
        }
    }
    for (i = 0; i < n; i++) {
        char name[16];
        int te, closing;
        if (html[i] != '<' || i + 1 >= n) continue;
        closing = html[i + 1] == '/';
        if (!is_name_char(html[i + 1 + closing])) continue;
        (void)tag_name(html, i + 1 + closing, n, name, sizeof(name));
        te = tag_end(html, i, n);
        if (closing) {
            if (w_eq(name, "label") && label_depth > 0) label_depth--;
            i = te;
            continue;
        }
        if (w_eq(name, "html")) {
            if (!tag_attr(html, i, te, "lang", 0, 0)) r->lang_missing = 1;
        } else if (w_eq(name, "label")) {
            label_depth++;
        } else if (w_eq(name, "img")) {
            r->images++;
            if (!tag_attr(html, i, te, "alt", 0, 0)) r->img_no_alt++;
        } else if (w_eq(name, "input") || w_eq(name, "textarea") || w_eq(name, "select")) {
            char type[16], id[32];
            int ok = 0, k;
            (void)tag_attr(html, i, te, "type", type, sizeof(type));
            if (!w_ieq(type, "hidden")) {
                r->inputs++;
                if (label_depth > 0 || tag_attr(html, i, te, "aria-label", 0, 0) ||
                    tag_attr(html, i, te, "aria-labelledby", 0, 0) || tag_attr(html, i, te, "title", 0, 0))
                    ok = 1;
                if ((w_ieq(type, "submit") || w_ieq(type, "button")) && tag_attr(html, i, te, "value", 0, 0)) ok = 1;
                if (!ok && tag_attr(html, i, te, "id", id, sizeof(id)) && id[0])
                    for (k = 0; k < labels; k++) if (w_eq(label_for[k], id)) ok = 1;
                if (!ok) r->input_no_label++;
            }
        } else if (w_eq(name, "button")) {
            r->buttons++;
            if (!tag_attr(html, i, te, "aria-label", 0, 0) && !text_until_close(html, te + 1, n, "</button"))
                r->button_no_text++;
        } else if (w_eq(name, "a")) {
            r->links++;
            if (!tag_attr(html, i, te, "aria-label", 0, 0) && !text_until_close(html, te + 1, n, "</a"))
                r->link_no_text++;
        } else if (name[0] == 'h' && name[1] >= '1' && name[1] <= '6' && name[2] == 0) {
            int level = name[1] - '0';
            r->headings++;
            if (prev_h > 0 && level > prev_h + 1) r->heading_skips++;
            prev_h = level;
        }
        i = te;
    }
    r->issues = r->img_no_alt + r->input_no_label + r->button_no_text + r->link_no_text +
                r->heading_skips + r->lang_missing;
    return r->issues;
}
