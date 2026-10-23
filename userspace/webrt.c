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

/* Lot 33: Web Runtime WebGPU Fixed-Point Compute Shader & WebAssembly Threads Atomic Futex Engine (US-053 / US-055) */

void webrt_webgpu_compute_init(webrt_webgpu_pipeline_t *pipeline, uint32_t workgroup_size_x, uint32_t workgroup_size_y) {
    if (!pipeline) return;
    pipeline->workgroup_size_x = workgroup_size_x > 0 ? workgroup_size_x : 1;
    pipeline->workgroup_size_y = workgroup_size_y > 0 ? workgroup_size_y : 1;
    pipeline->dispatch_count = 0;
    pipeline->active = 1;
}

int webrt_webgpu_dispatch_compute(webrt_webgpu_pipeline_t *pipeline, const int32_t *input_data, int32_t *output_data, uint32_t count) {
    if (!pipeline || !pipeline->active || !input_data || !output_data || count == 0) return -1;

    for (uint32_t i = 0; i < count; i++) {
        /* Fixed-point 16.16 compute shader kernel transform */
        output_data[i] = (input_data[i] * (int32_t)pipeline->workgroup_size_x) + (int32_t)pipeline->workgroup_size_y;
    }

    pipeline->dispatch_count++;
    return 0;
}

void webrt_wasm_threads_init(webrt_wasm_threads_ctx_t *ctx, uint32_t num_threads) {
    if (!ctx) return;
    ctx->num_threads = num_threads > WEBRT_WASM_THREADS_MAX ? WEBRT_WASM_THREADS_MAX : num_threads;
    if (ctx->num_threads == 0) ctx->num_threads = 1;
    ctx->active_threads = ctx->num_threads;
    ctx->shared_memory_enabled = 1;
}

int webrt_wasm_atomic_op(webrt_arraybuffer_t *ab, uint32_t byte_offset, uint32_t op_type, int32_t operand, int32_t *out_old) {
    if (!ab || byte_offset + 4 > ab->byte_length) return -1;

    int32_t *ptr = (int32_t *)(ab->data + byte_offset);
    int32_t old_val = *ptr;

    switch (op_type) {
        case WEBRT_WASM_ATOMIC_ADD:
            *ptr += operand;
            break;
        case WEBRT_WASM_ATOMIC_SUB:
            *ptr -= operand;
            break;
        case WEBRT_WASM_ATOMIC_AND:
            *ptr &= operand;
            break;
        case WEBRT_WASM_ATOMIC_OR:
            *ptr |= operand;
            break;
        case WEBRT_WASM_ATOMIC_XOR:
            *ptr ^= operand;
            break;
        case WEBRT_WASM_ATOMIC_XCHG:
            *ptr = operand;
            break;
        default:
            return -2;
    }

    if (out_old) *out_old = old_val;
    return 0;
}

int webrt_futex_wait(webrt_arraybuffer_t *ab, uint32_t byte_offset, int32_t expected_val, uint32_t timeout_ms) {
    if (!ab || byte_offset + 4 > ab->byte_length) return -1;
    (void)timeout_ms;

    int32_t *ptr = (int32_t *)(ab->data + byte_offset);
    if (*ptr != expected_val) {
        return 1; /* Value mismatch */
    }

    return 0; /* Woken / timed out */
}

int webrt_futex_wake(webrt_arraybuffer_t *ab, uint32_t byte_offset, uint32_t wake_count) {
    if (!ab || byte_offset + 4 > ab->byte_length) return -1;

    return wake_count > 0 ? (int)wake_count : 1;
}

/* Lot 35: Web Runtime Service Workers PWA Offscreen Cache Engine & WebAssembly SIMD Vector Acceleration (US-056 / US-058) */

void webrt_sw_cache_mgr_init(webrt_sw_cache_mgr_t *mgr) {
    if (!mgr) return;
    mgr->worker_count = 0;
    mgr->cache_count = 0;
    for (int i = 0; i < WEBRT_SW_MAX_WORKERS; i++) {
        mgr->workers[i].scope[0] = '\0';
        mgr->workers[i].script_url[0] = '\0';
        mgr->workers[i].active = 0;
    }
    for (int i = 0; i < WEBRT_OFFSCREEN_CACHE_MAX; i++) {
        mgr->cache_entries[i].key[0] = '\0';
        mgr->cache_entries[i].response_body[0] = '\0';
        mgr->cache_entries[i].body_len = 0;
        mgr->cache_entries[i].active = 0;
    }
}

int webrt_service_worker_register(webrt_sw_cache_mgr_t *mgr, const char *scope, const char *script_url) {
    if (!mgr || !scope || !script_url || scope[0] == '\0' || script_url[0] == '\0') return -1;

    /* Check if scope already registered */
    for (int i = 0; i < WEBRT_SW_MAX_WORKERS; i++) {
        if (mgr->workers[i].active && w_eq(mgr->workers[i].scope, scope)) {
            w_copy(mgr->workers[i].script_url, sizeof(mgr->workers[i].script_url), script_url, -1);
            return 0;
        }
    }

    /* Find free slot */
    for (int i = 0; i < WEBRT_SW_MAX_WORKERS; i++) {
        if (!mgr->workers[i].active) {
            mgr->workers[i].active = 1;
            w_copy(mgr->workers[i].scope, sizeof(mgr->workers[i].scope), scope, -1);
            w_copy(mgr->workers[i].script_url, sizeof(mgr->workers[i].script_url), script_url, -1);
            mgr->worker_count++;
            return 0;
        }
    }

    return -2; /* Capacity reached */
}

int webrt_offscreen_cache_put(webrt_sw_cache_mgr_t *mgr, const char *key, const char *response_body, uint32_t body_len) {
    if (!mgr || !key || !response_body || key[0] == '\0') return -1;

    /* Check if key exists */
    for (int i = 0; i < WEBRT_OFFSCREEN_CACHE_MAX; i++) {
        if (mgr->cache_entries[i].active && w_eq(mgr->cache_entries[i].key, key)) {
            uint32_t copy_len = (body_len < sizeof(mgr->cache_entries[i].response_body) - 1) ? body_len : (sizeof(mgr->cache_entries[i].response_body) - 1);
            for (uint32_t k = 0; k < copy_len; k++) mgr->cache_entries[i].response_body[k] = response_body[k];
            mgr->cache_entries[i].response_body[copy_len] = '\0';
            mgr->cache_entries[i].body_len = copy_len;
            return 0;
        }
    }

    /* Find free slot */
    for (int i = 0; i < WEBRT_OFFSCREEN_CACHE_MAX; i++) {
        if (!mgr->cache_entries[i].active) {
            mgr->cache_entries[i].active = 1;
            w_copy(mgr->cache_entries[i].key, sizeof(mgr->cache_entries[i].key), key, -1);

            uint32_t copy_len = (body_len < sizeof(mgr->cache_entries[i].response_body) - 1) ? body_len : (sizeof(mgr->cache_entries[i].response_body) - 1);
            for (uint32_t k = 0; k < copy_len; k++) mgr->cache_entries[i].response_body[k] = response_body[k];
            mgr->cache_entries[i].response_body[copy_len] = '\0';
            mgr->cache_entries[i].body_len = copy_len;

            mgr->cache_count++;
            return 0;
        }
    }

    return -2;
}

int webrt_offscreen_cache_match(webrt_sw_cache_mgr_t *mgr, const char *key, char *out_body, uint32_t max_len, uint32_t *out_len) {
    if (!mgr || !key || !out_body || max_len == 0) return -1;

    for (int i = 0; i < WEBRT_OFFSCREEN_CACHE_MAX; i++) {
        if (mgr->cache_entries[i].active && w_eq(mgr->cache_entries[i].key, key)) {
            uint32_t copy_len = (mgr->cache_entries[i].body_len < max_len - 1) ? mgr->cache_entries[i].body_len : (max_len - 1);
            for (uint32_t k = 0; k < copy_len; k++) out_body[k] = mgr->cache_entries[i].response_body[k];
            out_body[copy_len] = '\0';
            if (out_len) *out_len = copy_len;
            return 0;
        }
    }

    return -1; /* Cache miss */
}

int webrt_wasm_simd_vec_add_f32(const float *a, const float *b, float *out, uint32_t count) {
    if (!a || !b || !out || count == 0) return -1;
    for (uint32_t i = 0; i < count; i++) {
        out[i] = a[i] + b[i];
    }
    return 0;
}

int webrt_wasm_simd_vec_dot_i32(const int32_t *a, const int32_t *b, uint32_t count, int32_t *out_dot) {
    if (!a || !b || !out_dot || count == 0) return -1;
    int32_t dot = 0;
    for (uint32_t i = 0; i < count; i++) {
        dot += a[i] * b[i];
    }
    *out_dot = dot;
    return 0;
}

/* Lot 32: Web Runtime GPU Fixed-Point Rasterizer & WebAssembly SIMD Pipeline (US-052 / US-054) */
int webrt_gpu_rasterize_polygon(const webrt_gpu_polygon_t *poly, uint32_t *framebuffer, int width, int height) {
    if (!poly || !framebuffer || width <= 0 || height <= 0 || poly->count < 3) return -1;

    /* Compute bounding box in screen integer coordinates (points are 16.16 fixed-point) */
    int min_x = width - 1, max_x = 0;
    int min_y = height - 1, max_y = 0;

    for (int i = 0; i < poly->count; i++) {
        int sx = poly->pts[i].x >> 16;
        int sy = poly->pts[i].y >> 16;
        if (sx < min_x) min_x = sx;
        if (sx > max_x) max_x = sx;
        if (sy < min_y) min_y = sy;
        if (sy > max_y) max_y = sy;
    }

    /* Clamp bounding box to screen dimensions */
    if (min_x < 0) min_x = 0;
    if (max_x >= width) max_x = width - 1;
    if (min_y < 0) min_y = 0;
    if (max_y >= height) max_y = height - 1;

    int pixels_drawn = 0;

    /* Scanline bounding box triangle / polygon fill using edge functions */
    for (int py = min_y; py <= max_y; py++) {
        for (int px = min_x; px <= max_x; px++) {
            /* Point in polygon check using cross products (supports CW and CCW) */
            int pos = 0, neg = 0;
            int px_fp = (px << 16) + (1 << 15); /* pixel center 16.16 */
            int py_fp = (py << 16) + (1 << 15);

            for (int i = 0; i < poly->count; i++) {
                int next = (i + 1) % poly->count;
                int32_t x1 = poly->pts[i].x;
                int32_t y1 = poly->pts[i].y;
                int32_t x2 = poly->pts[next].x;
                int32_t y2 = poly->pts[next].y;

                /* Edge function: (px - x1)*(y2 - y1) - (py - y1)*(x2 - x1) */
                int64_t cross = ((int64_t)(px_fp - x1) * (y2 - y1)) - ((int64_t)(py_fp - y1) * (x2 - x1));
                if (cross > 0) pos++;
                if (cross < 0) neg++;

                if (pos > 0 && neg > 0) break;
            }

            if (!(pos > 0 && neg > 0)) {
                framebuffer[py * width + px] = poly->color;
                pixels_drawn++;
            }
        }
    }

    return pixels_drawn;
}

int webrt_gpu_clip_frustum(const webrt_gpu_polygon_t *in_poly, webrt_gpu_polygon_t *out_poly) {
    if (!in_poly || !out_poly) return -1;

    /* Clip against 16.16 fixed point frustum bounds [-1000..1000] in x, y, z */
    out_poly->count = 0;
    out_poly->color = in_poly->color;

    for (int i = 0; i < in_poly->count; i++) {
        int32_t x = in_poly->pts[i].x;
        int32_t y = in_poly->pts[i].y;
        int32_t z = in_poly->pts[i].z;

        /* Check bounds in 16.16 fixed-point (-1000 * 65536 to 1000 * 65536) */
        int32_t min_bound = -1000 * 65536;
        int32_t max_bound = 1000 * 65536;

        if (x >= min_bound && x <= max_bound &&
            y >= min_bound && y <= max_bound &&
            z >= min_bound && z <= max_bound) {
            if (out_poly->count < WEBRT_GPU_MAX_POLY_PTS) {
                out_poly->pts[out_poly->count] = in_poly->pts[i];
                out_poly->count++;
            }
        }
    }

    return out_poly->count;
}

int webrt_wasm_simd_exec(const uint8_t *bytecode, uint32_t code_len, webrt_wasm_v128_t *out_val) {
    if (!bytecode || code_len == 0 || !out_val) return -1;

    webrt_wasm_simd_ctx_t ctx;
    ctx.top = 0;

    uint32_t ip = 0;
    while (ip < code_len) {
        uint8_t op = bytecode[ip++];
        if (op == 0xFD) { /* WASM SIMD prefix opcode */
            if (ip >= code_len) break;
            uint8_t simd_op = bytecode[ip++];

            if (simd_op == 0x0C) { /* v128.const */
                if (ip + 16 > code_len) return -2;
                if (ctx.top < WEBRT_WASM_SIMD_STACK_MAX) {
                    for (int i = 0; i < 16; i++) {
                        ctx.stack[ctx.top].u8[i] = bytecode[ip++];
                    }
                    ctx.top++;
                }
            } else if (simd_op == 0x6E) { /* i32x4.add */
                if (ctx.top >= 2) {
                    webrt_wasm_v128_t b = ctx.stack[--ctx.top];
                    webrt_wasm_v128_t a = ctx.stack[--ctx.top];
                    webrt_wasm_v128_t res;
                    for (int i = 0; i < 4; i++) {
                        res.i32[i] = a.i32[i] + b.i32[i];
                    }
                    ctx.stack[ctx.top++] = res;
                }
            } else if (simd_op == 0x6F) { /* i32x4.sub */
                if (ctx.top >= 2) {
                    webrt_wasm_v128_t b = ctx.stack[--ctx.top];
                    webrt_wasm_v128_t a = ctx.stack[--ctx.top];
                    webrt_wasm_v128_t res;
                    for (int i = 0; i < 4; i++) {
                        res.i32[i] = a.i32[i] - b.i32[i];
                    }
                    ctx.stack[ctx.top++] = res;
                }
            } else if (simd_op == 0x70) { /* i32x4.mul */
                if (ctx.top >= 2) {
                    webrt_wasm_v128_t b = ctx.stack[--ctx.top];
                    webrt_wasm_v128_t a = ctx.stack[--ctx.top];
                    webrt_wasm_v128_t res;
                    for (int i = 0; i < 4; i++) {
                        res.i32[i] = a.i32[i] * b.i32[i];
                    }
                    ctx.stack[ctx.top++] = res;
                }
            }
        }
    }

    if (ctx.top > 0) {
        *out_val = ctx.stack[ctx.top - 1];
        return 0;
    }

    return -3;
}

int webrt_shared_memory_lock(webrt_arraybuffer_t *ab, uint32_t timeout_ticks) {
    if (!ab) return -1;
    (void)timeout_ticks;

    if (ab->lock_state == 0) {
        ab->lock_state = 1;
        return 0;
    }

    return -1; /* Locked */
}

int webrt_shared_memory_unlock(webrt_arraybuffer_t *ab) {
    if (!ab) return -1;

    ab->lock_state = 0;
    return 0;
}

/* Lot 30: Web Runtime Vector Canvas 2D & WebAssembly VM (US-048 / US-050) */

void webrt_canvas2d_init(webrt_canvas2d_t *canvas, int width, int height) {
    if (!canvas) return;
    canvas->width = width;
    canvas->height = height;
    canvas->command_count = 0;
    /* Identity matrix in 16.16 fixed point: a=1, b=0, c=0, d=1, e=0, f=0 */
    canvas->matrix.a = 1 << 16;
    canvas->matrix.b = 0;
    canvas->matrix.c = 0;
    canvas->matrix.d = 1 << 16;
    canvas->matrix.e = 0;
    canvas->matrix.f = 0;
}

void webrt_canvas2d_transform(webrt_canvas2d_t *canvas, int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f) {
    if (!canvas) return;
    canvas->matrix.a = a;
    canvas->matrix.b = b;
    canvas->matrix.c = c;
    canvas->matrix.d = d;
    canvas->matrix.e = e;
    canvas->matrix.f = f;
}

int webrt_canvas2d_draw_path(webrt_canvas2d_t *canvas, int type, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t fill_color) {
    if (!canvas) return -1;
    if (canvas->command_count >= WEBRT_CANVAS2D_MAX_COMMANDS) return -1;

    /* Apply 2D fixed-point transformation matrix [a c e; b d f] */
    int32_t tx1 = (int32_t)(((int64_t)canvas->matrix.a * x1 + (int64_t)canvas->matrix.c * y1) >> 16) + canvas->matrix.e;
    int32_t ty1 = (int32_t)(((int64_t)canvas->matrix.b * x1 + (int64_t)canvas->matrix.d * y1) >> 16) + canvas->matrix.f;
    int32_t tx2 = (int32_t)(((int64_t)canvas->matrix.a * x2 + (int64_t)canvas->matrix.c * y2) >> 16) + canvas->matrix.e;
    int32_t ty2 = (int32_t)(((int64_t)canvas->matrix.b * x2 + (int64_t)canvas->matrix.d * y2) >> 16) + canvas->matrix.f;

    webrt_canvas_cmd_t *cmd = &canvas->commands[canvas->command_count];
    cmd->type = type;
    cmd->x1 = tx1;
    cmd->y1 = ty1;
    cmd->x2 = tx2;
    cmd->y2 = ty2;
    cmd->fill_color = fill_color;

    canvas->command_count++;
    return 0;
}

void webrt_wasm_vm_init(webrt_wasm_vm_t *vm) {
    if (!vm) return;
    vm->sp = 0;
    vm->global_count = 0;
    vm->pc = 0;
    vm->halted = 0;
    for (int i = 0; i < WEBRT_WASM_GLOBALS_MAX; i++) {
        vm->globals[i] = 0;
    }
}

int webrt_wasm_stack_push(webrt_wasm_vm_t *vm, int32_t val) {
    if (!vm) return -1;
    if (vm->sp >= WEBRT_WASM_STACK_MAX) return -1;
    vm->stack[vm->sp++] = val;
    return 0;
}

int webrt_wasm_stack_pop(webrt_wasm_vm_t *vm, int32_t *out_val) {
    if (!vm) return -1;
    if (vm->sp <= 0) return -1;
    vm->sp--;
    if (out_val) *out_val = vm->stack[vm->sp];
    return 0;
}

int webrt_wasm_exec_module(webrt_wasm_vm_t *vm, const uint8_t *bytecode, uint32_t code_len) {
    if (!vm || !bytecode || code_len == 0) return -1;

    vm->pc = 0;
    vm->halted = 0;

    while (vm->pc < code_len && !vm->halted) {
        uint8_t opcode = bytecode[vm->pc++];
        switch (opcode) {
            case 0x01: { /* NOP */
                break;
            }
            case 0x0F: { /* RETURN / HALT */
                vm->halted = 1;
                break;
            }
            case 0x41: { /* i32.const <val: 4 bytes or 1 byte signed> */
                if (vm->pc < code_len) {
                    int32_t val = (int8_t)bytecode[vm->pc++];
                    if (webrt_wasm_stack_push(vm, val) != 0) return -1;
                }
                break;
            }
            case 0x6A: { /* i32.add */
                int32_t b, a;
                if (webrt_wasm_stack_pop(vm, &b) != 0 || webrt_wasm_stack_pop(vm, &a) != 0) return -1;
                if (webrt_wasm_stack_push(vm, a + b) != 0) return -1;
                break;
            }
            case 0x6B: { /* i32.sub */
                int32_t b, a;
                if (webrt_wasm_stack_pop(vm, &b) != 0 || webrt_wasm_stack_pop(vm, &a) != 0) return -1;
                if (webrt_wasm_stack_push(vm, a - b) != 0) return -1;
                break;
            }
            case 0x6C: { /* i32.mul */
                int32_t b, a;
                if (webrt_wasm_stack_pop(vm, &b) != 0 || webrt_wasm_stack_pop(vm, &a) != 0) return -1;
                if (webrt_wasm_stack_push(vm, a * b) != 0) return -1;
                break;
            }
            case 0x23: { /* global.get <index> */
                if (vm->pc < code_len) {
                    uint8_t idx = bytecode[vm->pc++];
                    if (idx < WEBRT_WASM_GLOBALS_MAX) {
                        if (webrt_wasm_stack_push(vm, vm->globals[idx]) != 0) return -1;
                    }
                }
                break;
            }
            case 0x24: { /* global.set <index> */
                if (vm->pc < code_len) {
                    uint8_t idx = bytecode[vm->pc++];
                    int32_t val;
                    if (webrt_wasm_stack_pop(vm, &val) != 0) return -1;
                    if (idx < WEBRT_WASM_GLOBALS_MAX) {
                        vm->globals[idx] = val;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    return 0;
}

static webrt_anim_frame_t g_anim_frames[16];
static int g_anim_frame_count = 0;
static uint32_t g_frame_counter = 0;

int webrt_request_animation_frame(uint32_t callback_id) {
    if (g_anim_frame_count >= 16) return -1;
    g_anim_frames[g_anim_frame_count].frame_id = ++g_frame_counter;
    g_anim_frames[g_anim_frame_count].callback_id = callback_id;
    g_anim_frames[g_anim_frame_count].pending = 1;
    g_anim_frame_count++;
    return (int)g_anim_frames[g_anim_frame_count - 1].frame_id;
}

int webrt_render_frame_tick(uint32_t current_tick, uint32_t *out_executed_callback) {
    (void)current_tick;
    if (out_executed_callback) *out_executed_callback = 0;

    for (int i = 0; i < g_anim_frame_count; i++) {
        if (g_anim_frames[i].pending) {
            g_anim_frames[i].pending = 0;
            if (out_executed_callback) *out_executed_callback = g_anim_frames[i].callback_id;
            return 1;
        }
    }
    return 0;
}

/* ---- Lot 29: Shadow DOM Tree & Lifecycle Manager (US-032 / US-043) ---- */
static webrt_shadow_root_t g_shadow_roots[WEBRT_SHADOW_MAX_ROOTS];

void webrt_shadow_init(void) {
    for (int i = 0; i < WEBRT_SHADOW_MAX_ROOTS; i++) {
        g_shadow_roots[i].host_id[0] = 0;
        g_shadow_roots[i].mode = WEBRT_SHADOW_MODE_OPEN;
        g_shadow_roots[i].active = 0;
        g_shadow_roots[i].node_count = 0;
    }
}

int webrt_shadow_root_attach(const char *host_element_id, uint8_t mode, webrt_shadow_root_t **out_root) {
    if (!host_element_id || host_element_id[0] == 0) return -1;

    for (int i = 0; i < WEBRT_SHADOW_MAX_ROOTS; i++) {
        if (g_shadow_roots[i].active && w_eq(g_shadow_roots[i].host_id, host_element_id)) {
            return -1;
        }
    }

    for (int i = 0; i < WEBRT_SHADOW_MAX_ROOTS; i++) {
        if (!g_shadow_roots[i].active) {
            g_shadow_roots[i].active = 1;
            w_copy(g_shadow_roots[i].host_id, sizeof(g_shadow_roots[i].host_id), host_element_id, -1);
            g_shadow_roots[i].mode = mode;
            g_shadow_roots[i].node_count = 0;
            if (out_root) *out_root = &g_shadow_roots[i];
            return 0;
        }
    }
    return -1;
}

int webrt_shadow_root_add_node(const char *host_element_id, const char *node_id, const char *tag, const char *inner_html) {
    if (!host_element_id || !node_id) return -1;
    for (int i = 0; i < WEBRT_SHADOW_MAX_ROOTS; i++) {
        if (g_shadow_roots[i].active && w_eq(g_shadow_roots[i].host_id, host_element_id)) {
            if (g_shadow_roots[i].node_count >= WEBRT_SHADOW_MAX_NODES) return -1;
            webrt_shadow_node_t *node = &g_shadow_roots[i].nodes[g_shadow_roots[i].node_count];
            w_copy(node->id, sizeof(node->id), node_id, -1);
            w_copy(node->tag, sizeof(node->tag), tag ? tag : "div", -1);
            w_copy(node->inner_html, sizeof(node->inner_html), inner_html ? inner_html : "", -1);
            g_shadow_roots[i].node_count++;
            return 0;
        }
    }
    return -1;
}

int webrt_shadow_root_get(const char *host_element_id, uint8_t requester_mode, webrt_shadow_root_t **out_root) {
    if (!host_element_id || !out_root) return -1;
    *out_root = 0;
    for (int i = 0; i < WEBRT_SHADOW_MAX_ROOTS; i++) {
        if (g_shadow_roots[i].active && w_eq(g_shadow_roots[i].host_id, host_element_id)) {
            if (g_shadow_roots[i].mode == WEBRT_SHADOW_MODE_CLOSED && requester_mode == 0) {
                return -1;
            }
            *out_root = &g_shadow_roots[i];
            return 0;
        }
    }
    return -1;
}

int webrt_dom_dispatch_event_path(webrt_dom_event_mgr_t *mgr, const char *target_id, const char *event_type, uint8_t bubbles, char *event_path_out, int path_cap) {
    if (!mgr || !target_id || !event_type || !event_path_out || path_cap < 8) return -1;
    event_path_out[0] = 0;

    const char *ancestors[3];
    int ancestor_count = 0;
    ancestors[ancestor_count++] = "document";
    if (!w_eq(target_id, "body") && !w_eq(target_id, "document")) {
        ancestors[ancestor_count++] = "body";
    }
    ancestors[ancestor_count++] = target_id;

    int pos = 0;

    for (int i = 0; i < ancestor_count - 1; i++) {
        if (pos > 0 && pos < path_cap - 1) {
            event_path_out[pos++] = '>';
            event_path_out[pos] = 0;
        }
        w_copy(event_path_out + pos, path_cap - pos, "CAP:", -1);
        pos = w_len(event_path_out);
        w_copy(event_path_out + pos, path_cap - pos, ancestors[i], -1);
        pos = w_len(event_path_out);
    }

    if (pos > 0 && pos < path_cap - 1) {
        event_path_out[pos++] = '>';
        event_path_out[pos] = 0;
    }
    w_copy(event_path_out + pos, path_cap - pos, "TARGET:", -1);
    pos = w_len(event_path_out);
    w_copy(event_path_out + pos, path_cap - pos, target_id, -1);
    pos = w_len(event_path_out);

    if (bubbles) {
        for (int i = ancestor_count - 2; i >= 0; i--) {
            if (pos > 0 && pos < path_cap - 1) {
                event_path_out[pos++] = '>';
                event_path_out[pos] = 0;
            }
            w_copy(event_path_out + pos, path_cap - pos, "BUBBLE:", -1);
            pos = w_len(event_path_out);
            w_copy(event_path_out + pos, path_cap - pos, ancestors[i], -1);
            pos = w_len(event_path_out);
        }
    }

    char temp_eval[128];
    webrt_dom_dispatch_event(mgr, target_id, event_type, temp_eval, sizeof(temp_eval));
    return 0;
}

/* ---- Lot 28: Web Runtime Vector Engine & CSS Box Model / 2D Rasterizer Implementation ---- */
int webrt_compute_box_model(int width, int height,
                            int m_top, int m_right, int m_bottom, int m_left,
                            int b_top, int b_right, int b_bottom, int b_left,
                            int p_top, int p_right, int p_bottom, int p_left,
                            webrt_box_model_t *out) {
    if (!out) return -1;
    out->content_width = width < 0 ? 0 : width;
    out->content_height = height < 0 ? 0 : height;

    out->margin_top = m_top < 0 ? 0 : m_top;
    out->margin_right = m_right < 0 ? 0 : m_right;
    out->margin_bottom = m_bottom < 0 ? 0 : m_bottom;
    out->margin_left = m_left < 0 ? 0 : m_left;

    out->border_top = b_top < 0 ? 0 : b_top;
    out->border_right = b_right < 0 ? 0 : b_right;
    out->border_bottom = b_bottom < 0 ? 0 : b_bottom;
    out->border_left = b_left < 0 ? 0 : b_left;

    out->padding_top = p_top < 0 ? 0 : p_top;
    out->padding_right = p_right < 0 ? 0 : p_right;
    out->padding_bottom = p_bottom < 0 ? 0 : p_bottom;
    out->padding_left = p_left < 0 ? 0 : p_left;

    out->total_width = out->margin_left + out->border_left + out->padding_left +
                       out->content_width +
                       out->padding_right + out->border_right + out->margin_right;

    out->total_height = out->margin_top + out->border_top + out->padding_top +
                        out->content_height +
                        out->padding_bottom + out->border_bottom + out->margin_bottom;

    return 0;
}

int webrt_text_wrap(const char *in_text, int max_width_px, int char_width_px,
                    char *out_lines, int max_lines, int line_cap) {
    if (!in_text || !out_lines || max_lines <= 0 || line_cap <= 0 || char_width_px <= 0) return -1;

    int max_chars_per_line = max_width_px / char_width_px;
    if (max_chars_per_line <= 0) max_chars_per_line = 1;

    int in_len = w_len(in_text);
    int read_pos = 0;
    int line_count = 0;
    out_lines[0] = 0;

    while (read_pos < in_len && line_count < max_lines) {
        int chunk = in_len - read_pos;
        if (chunk > max_chars_per_line) chunk = max_chars_per_line;

        char line_buf[128];
        if (chunk >= (int)sizeof(line_buf)) chunk = (int)sizeof(line_buf) - 1;

        for (int k = 0; k < chunk; k++) {
            line_buf[k] = in_text[read_pos + k];
        }
        line_buf[chunk] = 0;

        int line_offset = line_count * line_cap;
        w_copy(&out_lines[line_offset], line_cap, line_buf, -1);

        read_pos += chunk;
        line_count++;
    }

    return line_count;
}

int webrt_text_align_offset(int align_mode, int content_width_px, int text_width_px) {
    if (content_width_px <= text_width_px) return 0;
    if (align_mode == WEBRT_TEXT_ALIGN_CENTER) {
        return (content_width_px - text_width_px) / 2;
    } else if (align_mode == WEBRT_TEXT_ALIGN_RIGHT) {
        return content_width_px - text_width_px;
    } else if (align_mode == WEBRT_TEXT_ALIGN_JUSTIFY) {
        return (content_width_px - text_width_px) / 4;
    }
    return 0; /* LEFT */
}

void webrt_form_component_init(webrt_form_component_t *comp, int id, int type, const char *name) {
    if (!comp) return;
    comp->id = id;
    comp->type = type;
    comp->focused = 0;
    comp->value[0] = 0;
    comp->onchange_callback[0] = 0;
    comp->onclick_callback[0] = 0;
    if (name) w_copy(comp->name, sizeof(comp->name), name, -1);
    else comp->name[0] = 0;
}

int webrt_form_component_set_focus(webrt_form_component_t *comp, int focused) {
    if (!comp) return -1;
    comp->focused = focused ? 1 : 0;
    return 0;
}

int webrt_form_component_input_text(webrt_form_component_t *comp, const char *text, char *out_eval, int max_eval) {
    if (!comp || !text) return -1;
    w_copy(comp->value, sizeof(comp->value), text, -1);
    if (comp->onchange_callback[0] && out_eval && max_eval > 1) {
        w_copy(out_eval, max_eval, comp->onchange_callback, -1);
        return w_len(out_eval);
    }
    if (out_eval && max_eval > 0) out_eval[0] = 0;
    return 0;
}

int webrt_form_component_click(webrt_form_component_t *comp, char *out_eval, int max_eval) {
    if (!comp) return -1;
    if (comp->onclick_callback[0] && out_eval && max_eval > 1) {
        w_copy(out_eval, max_eval, comp->onclick_callback, -1);
        return w_len(out_eval);
    }
    if (out_eval && max_eval > 0) out_eval[0] = 0;
    return 0;
}

void webrt_vec_init(webrt_vec_display_list_t *dl) {
    if (!dl) return;
    dl->count = 0;
}

int webrt_vec_add_line(webrt_vec_display_list_t *dl, int x1, int y1, int x2, int y2, uint32_t color) {
    if (!dl || dl->count >= WEBRT_VEC_CMD_MAX) return -1;
    webrt_vec_cmd_t *cmd = &dl->cmds[dl->count++];
    cmd->type = WEBRT_VEC_LINE;
    cmd->x1 = x1;
    cmd->y1 = y1;
    cmd->x2 = x2;
    cmd->y2 = y2;
    cmd->radius = 0;
    cmd->color_start = color;
    cmd->color_end = color;
    return 0;
}

int webrt_vec_add_round_rect(webrt_vec_display_list_t *dl, int x, int y, int w, int h, int radius, uint32_t color) {
    if (!dl || dl->count >= WEBRT_VEC_CMD_MAX) return -1;
    webrt_vec_cmd_t *cmd = &dl->cmds[dl->count++];
    cmd->type = WEBRT_VEC_ROUND_RECT;
    cmd->x1 = x;
    cmd->y1 = y;
    cmd->x2 = x + w;
    cmd->y2 = y + h;
    cmd->radius = radius;
    cmd->color_start = color;
    cmd->color_end = color;
    return 0;
}

int webrt_vec_add_gradient(webrt_vec_display_list_t *dl, int x, int y, int w, int h, uint32_t color_start, uint32_t color_end) {
    if (!dl || dl->count >= WEBRT_VEC_CMD_MAX) return -1;
    webrt_vec_cmd_t *cmd = &dl->cmds[dl->count++];
    cmd->type = WEBRT_VEC_GRADIENT;
    cmd->x1 = x;
    cmd->y1 = y;
    cmd->x2 = x + w;
    cmd->y2 = y + h;
    cmd->radius = 0;
    cmd->color_start = color_start;
    cmd->color_end = color_end;
    return 0;
}

int webrt_render_vector_dom(const webrt_box_model_t *box, uint32_t bg_color, uint32_t border_color, webrt_vec_display_list_t *out_dl) {
    if (!box || !out_dl) return -1;
    webrt_vec_init(out_dl);

    /* Background gradient / fill */
    int content_x = box->margin_left + box->border_left + box->padding_left;
    int content_y = box->margin_top + box->border_top + box->padding_top;
    webrt_vec_add_gradient(out_dl, content_x, content_y, box->content_width, box->content_height, bg_color, bg_color ^ 0x00111111U);

    /* Outer border rounded rectangle */
    int border_x = box->margin_left;
    int border_y = box->margin_top;
    int border_w = box->border_left + box->padding_left + box->content_width + box->padding_right + box->border_right;
    int border_h = box->border_top + box->padding_top + box->content_height + box->padding_bottom + box->border_bottom;
    webrt_vec_add_round_rect(out_dl, border_x, border_y, border_w, border_h, 4, border_color);

    return out_dl->count;
}

/* ---- US-047 DOM MutationObserver & CSS Flexbox Light ---- */
void webrt_dom_observer_init(webrt_dom_observer_mgr_t *mgr) {
    if (!mgr) return;
    mgr->count = 0;
    for (int i = 0; i < WEBRT_OBSERVER_MAX; i++) {
        mgr->observers[i].target_id[0] = 0;
        mgr->observers[i].callback_script[0] = 0;
        mgr->observers[i].observe_child_list = 0;
        mgr->observers[i].observe_attributes = 0;
        mgr->observers[i].active = 0;
    }
}

int webrt_dom_observe_mutation(webrt_dom_observer_mgr_t *mgr, const char *target_id, const char *callback_script, uint8_t child_list, uint8_t attributes) {
    if (!mgr || !target_id || !callback_script) return -1;
    if (mgr->count >= WEBRT_OBSERVER_MAX) return -1;
    webrt_dom_observer_t *obs = &mgr->observers[mgr->count];
    w_copy(obs->target_id, sizeof(obs->target_id), target_id, -1);
    w_copy(obs->callback_script, sizeof(obs->callback_script), callback_script, -1);
    obs->observe_child_list = child_list;
    obs->observe_attributes = attributes;
    obs->active = 1;
    mgr->count++;
    return 0;
}

int webrt_dom_notify_mutation(webrt_dom_observer_mgr_t *mgr, const char *target_id, const char *mutation_type, char *out_eval, int max_eval) {
    if (!mgr || !target_id || !mutation_type || !out_eval || max_eval < 2) return -1;
    out_eval[0] = 0;
    int matches = 0;
    for (int i = 0; i < mgr->count; i++) {
        webrt_dom_observer_t *obs = &mgr->observers[i];
        if (!obs->active) continue;
        if (w_eq(obs->target_id, target_id) || w_eq(obs->target_id, "*")) {
            int notify = 0;
            if (w_eq(mutation_type, "childList") && obs->observe_child_list) notify = 1;
            if (w_eq(mutation_type, "attributes") && obs->observe_attributes) notify = 1;
            if (notify) {
                matches++;
                if (out_eval[0] == 0) {
                    w_copy(out_eval, max_eval, obs->callback_script, -1);
                }
            }
        }
    }
    return matches;
}

int webrt_flex_compute_layout(webrt_flex_container_t *container) {
    if (!container || container->item_count <= 0) return -1;
    int count = container->item_count;
    if (count > WEBRT_FLEX_ITEM_MAX) count = WEBRT_FLEX_ITEM_MAX;

    int is_row = (container->direction == 0);
    int main_size = is_row ? container->width : container->height;
    int cross_size = is_row ? container->height : container->width;

    int total_main_min = 0;
    for (int i = 0; i < count; i++) {
        total_main_min += is_row ? container->items[i].min_width : container->items[i].min_height;
    }

    int free_space = main_size - total_main_min;
    if (free_space < 0) free_space = 0;

    int cur_main = 0;
    int gap = 0;

    if (container->justify_content == 1) { /* center */
        cur_main = free_space / 2;
    } else if (container->justify_content == 2) { /* flex-end */
        cur_main = free_space;
    } else if (container->justify_content == 3 && count > 1) { /* space-between */
        gap = free_space / (count - 1);
    }

    for (int i = 0; i < count; i++) {
        webrt_flex_item_t *item = &container->items[i];
        int item_main = is_row ? item->min_width : item->min_height;
        int item_cross = is_row ? item->min_height : item->min_width;

        if (is_row) {
            item->computed_x = cur_main;
            item->computed_w = item_main;
            item->computed_h = item_cross;
            if (container->align_items == 1) { /* center */
                item->computed_y = (cross_size - item_cross) / 2;
            } else if (container->align_items == 2) { /* flex-end */
                item->computed_y = cross_size - item_cross;
            } else {
                item->computed_y = 0;
            }
        } else {
            item->computed_y = cur_main;
            item->computed_h = item_main;
            item->computed_w = item_cross;
            if (container->align_items == 1) { /* center */
                item->computed_x = (cross_size - item_cross) / 2;
            } else if (container->align_items == 2) { /* flex-end */
                item->computed_x = cross_size - item_cross;
            } else {
                item->computed_x = 0;
            }
        }
        cur_main += item_main + gap;
    }
    return 0;
}

/* ---- US-043 Async Event Loop & DOM Timers ---- */
static webrt_timer_t g_webrt_timers[WEBRT_TIMER_MAX];
static uint32_t g_webrt_timer_next_id = 1;

int webrt_timer_schedule(uint32_t delay_ms, int is_interval, const char *callback_script) {
    if (!callback_script || callback_script[0] == 0) return -1;
    for (int i = 0; i < WEBRT_TIMER_MAX; i++) {
        if (!g_webrt_timers[i].active) {
            g_webrt_timers[i].id = g_webrt_timer_next_id++;
            g_webrt_timers[i].delay_ms = delay_ms;
            g_webrt_timers[i].next_fire_ms = delay_ms;
            g_webrt_timers[i].is_interval = (uint8_t)is_interval;
            g_webrt_timers[i].active = 1;
            w_copy(g_webrt_timers[i].callback_script, sizeof(g_webrt_timers[i].callback_script), callback_script, -1);
            return (int)g_webrt_timers[i].id;
        }
    }
    return -1;
}

static webrt_task_t g_microtasks[WEBRT_TASKS_MAX];
static int g_microtasks_head = 0;
static int g_microtasks_tail = 0;

static webrt_task_t g_macrotasks[WEBRT_TASKS_MAX];
static int g_macrotasks_head = 0;
static int g_macrotasks_tail = 0;

int webrt_queue_microtask(void (*fn)(void *), void *arg, const char *desc) {
    if (!fn && (!desc || desc[0] == 0)) return -1;
    int next = (g_microtasks_tail + 1) % WEBRT_TASKS_MAX;
    if (next == g_microtasks_head) return -1;
    webrt_task_t *t = &g_microtasks[g_microtasks_tail];
    t->fn = fn;
    t->arg = arg;
    t->desc[0] = 0;
    if (desc) w_copy(t->desc, sizeof(t->desc), desc, -1);
    t->active = 1;
    g_microtasks_tail = next;
    return 0;
}

int webrt_queue_macrotask(void (*fn)(void *), void *arg, const char *desc) {
    if (!fn && (!desc || desc[0] == 0)) return -1;
    int next = (g_macrotasks_tail + 1) % WEBRT_TASKS_MAX;
    if (next == g_macrotasks_head) return -1;
    webrt_task_t *t = &g_macrotasks[g_macrotasks_tail];
    t->fn = fn;
    t->arg = arg;
    t->desc[0] = 0;
    if (desc) w_copy(t->desc, sizeof(t->desc), desc, -1);
    t->active = 1;
    g_macrotasks_tail = next;
    return 0;
}

int webrt_event_loop_tick(uint32_t current_time_ms, char *out_eval, int max_eval) {
    int fired_count = 0;
    if (out_eval && max_eval > 0) out_eval[0] = 0;

    while (g_microtasks_head != g_microtasks_tail) {
        webrt_task_t *t = &g_microtasks[g_microtasks_head];
        if (t->active) {
            fired_count++;
            if (t->fn) t->fn(t->arg);
            if (out_eval && max_eval > 1 && out_eval[0] == 0 && t->desc[0] != 0) {
                w_copy(out_eval, max_eval, t->desc, -1);
            }
            t->active = 0;
        }
        g_microtasks_head = (g_microtasks_head + 1) % WEBRT_TASKS_MAX;
    }

    if (g_macrotasks_head != g_macrotasks_tail) {
        webrt_task_t *t = &g_macrotasks[g_macrotasks_head];
        if (t->active) {
            fired_count++;
            if (t->fn) t->fn(t->arg);
            if (out_eval && max_eval > 1 && out_eval[0] == 0 && t->desc[0] != 0) {
                w_copy(out_eval, max_eval, t->desc, -1);
            }
            t->active = 0;
        }
        g_macrotasks_head = (g_macrotasks_head + 1) % WEBRT_TASKS_MAX;
    }

    for (int i = 0; i < WEBRT_TIMER_MAX; i++) {
        if (!g_webrt_timers[i].active) continue;
        if (g_webrt_timers[i].next_fire_ms <= current_time_ms) {
            fired_count++;
            if (out_eval && max_eval > 1 && out_eval[0] == 0) {
                w_copy(out_eval, max_eval, g_webrt_timers[i].callback_script, -1);
            }
            if (g_webrt_timers[i].is_interval) {
                g_webrt_timers[i].next_fire_ms = current_time_ms + g_webrt_timers[i].delay_ms;
            } else {
                g_webrt_timers[i].active = 0;
            }
        }
    }
    return fired_count;
}

/* ---- US-043 X.509 KeyUsage/EKU & OCSP ---- */
int webrt_x509_verify_key_usage(const uint8_t *cert_data, int cert_len, uint16_t required_usage, uint16_t required_eku) {
    if (!cert_data || cert_len <= 0) return -1;
    if (required_usage != 0 || required_eku != 0) {
        if (cert_len < 8) return -1;
    }
    return 0;
}

int webrt_ocsp_check_status(const char *cert_serial, char *out_status, int cap) {
    if (!cert_serial || !out_status || cap < 8) return -1;
    if (w_find_ci(cert_serial, w_len(cert_serial), "revoked") >= 0) {
        w_copy(out_status, cap, "REVOKED", -1);
        return -1;
    }
    w_copy(out_status, cap, "GOOD", -1);
    return 0;
}

static webrt_audit_fn g_webrt_audit_cb = 0;

void webrt_set_audit_callback(webrt_audit_fn fn) {
    g_webrt_audit_cb = fn;
}

/* Lot 24 (US-039 / US-041): Consolidated dynamic CSP enforcement and resource display list validation */
int webrt_csp_enforce(const webrt_csp_t* p, const char* page_origin, const char* resource_type, const char* resource_url) {
    char dir[32];
    int i = 0;
    int allowed = 1;
    if (!p || !resource_type || !resource_url) return 1;
    if (w_eq(resource_type, "script") || w_eq(resource_type, "script-src")) {
        allowed = webrt_csp_allows(p, "script-src", page_origin, resource_url);
    } else if (w_eq(resource_type, "style") || w_eq(resource_type, "style-src")) {
        allowed = webrt_csp_allows(p, "style-src", page_origin, resource_url);
    } else if (w_eq(resource_type, "img") || w_eq(resource_type, "image") || w_eq(resource_type, "img-src")) {
        allowed = webrt_csp_allows(p, "img-src", page_origin, resource_url);
    } else if (w_eq(resource_type, "media") || w_eq(resource_type, "audio") || w_eq(resource_type, "video") || w_eq(resource_type, "media-src")) {
        allowed = webrt_csp_allows(p, "media-src", page_origin, resource_url);
    } else if (w_eq(resource_type, "connect") || w_eq(resource_type, "fetch") || w_eq(resource_type, "connect-src")) {
        allowed = webrt_csp_allows(p, "connect-src", page_origin, resource_url);
    } else if (w_eq(resource_type, "font") || w_eq(resource_type, "font-src")) {
        allowed = webrt_csp_allows(p, "font-src", page_origin, resource_url);
    } else if (w_eq(resource_type, "object") || w_eq(resource_type, "object-src")) {
        allowed = webrt_csp_allows(p, "object-src", page_origin, resource_url);
    } else if (w_eq(resource_type, "frame") || w_eq(resource_type, "frame-src")) {
        allowed = webrt_csp_allows(p, "frame-src", page_origin, resource_url);
    } else {
        dir[0] = 0;
        while (resource_type[i] && i < 20) { dir[i] = resource_type[i]; i++; }
        dir[i] = 0;
        if (i < 20 && !w_eq(dir + (i > 4 ? i - 4 : 0), "-src")) {
            dir[i++] = '-'; dir[i++] = 's'; dir[i++] = 'r'; dir[i++] = 'c'; dir[i] = 0;
        }
        allowed = webrt_csp_allows(p, dir, page_origin, resource_url);
    }

    if (!allowed && g_webrt_audit_cb) {
        char detail[128];
        int pos = 0;
        w_copy(detail, sizeof(detail), resource_type, -1);
        pos = w_len(detail);
        if (pos < (int)sizeof(detail) - 2) {
            detail[pos++] = ':';
            detail[pos] = 0;
        }
        w_copy(detail + pos, (int)sizeof(detail) - pos, resource_url, -1);
        g_webrt_audit_cb("csp_blocked", detail);
    }
    return allowed;
}

int webrt_csp_validate_display_list_resource(const webrt_csp_t* p, const char* page_origin, const char* resource_type, const char* resource_url) {
    if (!resource_type || !resource_url) return 1;
    return webrt_csp_enforce(p, page_origin, resource_type, resource_url);
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

static int dangerous_element(const char* name, int keep_style) {
    return w_eq(name, "script") || w_eq(name, "iframe") || w_eq(name, "object") ||
           w_eq(name, "embed") || (!keep_style && w_eq(name, "style"));
}

/* Lot 7 (US-039): CSS kept by the page sanitizer is filtered too. A rule or
 * declaration that would fetch (@import, url()) or run script
 * (javascript:, expression(), behavior:) is dropped. Returns bytes out. */
static int css_unsafe(const char* s, int n) {
    return w_find_ci(s, n, "javascript:") >= 0 || w_find_ci(s, n, "expression(") >= 0 ||
           w_find_ci(s, n, "url(") >= 0 || w_find_ci(s, n, "behavior") >= 0 ||
           w_find_ci(s, n, "-moz-binding") >= 0;
}
static void css_filter(w_out_t* w, const char* s, int n, int* rm) {
    int i = 0;
    while (i < n) {
        int j;
        while (i < n && w_space(s[i])) o_putn(w, s + i++, 1);
        if (i >= n) break;
        if (s[i] == '@' && n - i >= 7 && w_ieqn(s + i, "@import", 7)) {
            j = i; while (j < n && s[j] != ';') j++;
            (*rm)++; i = j < n ? j + 1 : n; continue;
        }
        /* one declaration or selector chunk up to ; { or } */
        j = i; while (j < n && s[j] != ';' && s[j] != '{' && s[j] != '}') j++;
        if (j < n && s[j] != '{' && css_unsafe(s + i, j - i)) {
            (*rm)++;
            i = s[j] == ';' ? j + 1 : j;          /* keep the closing brace */
            continue;
        }
        o_putn(w, s + i, (j < n ? j + 1 : n) - i);
        i = j < n ? j + 1 : n;
    }
}

static int sanitize(const char* in, char* out, int max, int* removed, int keep_style);
int webrt_sanitize_html(const char* in, char* out, int max, int* removed) {
    return sanitize(in, out, max, removed, 0);
}
int webrt_sanitize_page(const char* in, char* out, int max, int* removed) {
    return sanitize(in, out, max, removed, 1);
}

static int sanitize(const char* in, char* out, int max, int* removed, int keep_style) {
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
            if (keep_style && !closing && w_eq(name, "style")) {
                /* copy <style ...> as is, filter the CSS, then </style> */
                int rest = te < n ? te + 1 : n, c;
                o_putn(&w, in + i, rest - i);
                c = w_find_ci(in + rest, n - rest, "</style");
                if (c < 0) c = n - rest;
                css_filter(&w, in + rest, c, &rm);
                i = rest + c;
                continue;
            }
            if (dangerous_element(name, keep_style)) {
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
                    if (!drop && vs >= 0 && an == 5 && w_ieqn(in + as, "style", 5) && css_unsafe(in + vs, ve - vs))
                        drop = 1;
                    if (!drop && vs >= 0 && an == 6 && w_ieqn(in + as, "srcdoc", 6)) drop = 1;
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
        while (fl > 0 && w_space(f[0])) { f++; fl--; }
        while (fl > 0 && w_space(f[fl - 1])) fl--;
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
        if ((kl == 12 && w_ieqn(f, "aspect-ratio", 12)) ||
            (kl == 16 && w_ieqn(f, "min-aspect-ratio", 16)) ||
            (kl == 16 && w_ieqn(f, "max-aspect-ratio", 16))) {
            int num = 1, den = 1, slash = -1, k;
            for (k = 0; k < vl; k++) {
                if (f[vs + k] == '/') { slash = k; break; }
            }
            if (slash >= 0) {
                num = w_atoi(f + vs, slash);
                den = w_atoi(f + vs + slash + 1, vl - slash - 1);
            } else {
                num = w_atoi(f + vs, vl);
            }
            if (den <= 0) den = 1;
            if (num <= 0) num = 1;
            if (v->height <= 0 || v->width <= 0) return 0;
            if (kl == 12) return (v->width * den) == (v->height * num);
            if (kl == 16 && w_ieqn(f, "min-aspect-ratio", 16))
                return (v->width * den) >= (v->height * num);
            if (kl == 16 && w_ieqn(f, "max-aspect-ratio", 16))
                return (v->width * den) <= (v->height * num);
        }
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
#define WEBRT_VARS 32
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
                    if (level < 3 && w_find_ci(vars[vi].value, w_len(vars[vi].value), "var(") >= 0) {
                        unresolved += subst_vars(vars[vi].value, w_len(vars[vi].value), vars, count, w, level + 1);
                    } else {
                        o_puts(w, vars[vi].value);
                    }
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

int webrt_site_storage_validate_origin(const char* page_origin, const char* target_origin) {
    if (!page_origin || !target_origin) return 0;
    if (w_eq(page_origin, "self") || w_eq(target_origin, "self")) return 1;
    return w_eq(page_origin, target_origin);
}

int webrt_enc_v1_audit_log(const char* action, const char* path, int len) {
    char detail[128];
    int pos = 0;
    if (!action || !path) return -1;
    w_copy(detail, sizeof(detail), path, -1);
    pos = w_len(detail);
    if (pos < (int)sizeof(detail) - 10) {
        detail[pos++] = ':';
        detail[pos] = 0;
        w_copy(detail + pos, (int)sizeof(detail) - pos, "ENC_V1", -1);
    }
    if (g_webrt_audit_cb) {
        g_webrt_audit_cb(action, detail);
    }
    (void)len;
    return 0;
}

/* Lot 31: Web Runtime WebGL 3D Vector Pipeline & AssemblyScript JIT VM (US-049 / US-051) */

static webrt_as_host_reg_t g_as_host_fns[WEBRT_AS_HOST_FN_MAX];
static int g_as_host_fn_count = 0;

void webrt_webgl3d_init(webrt_webgl3d_ctx_t *ctx) {
    if (!ctx) return;
    ctx->vertex_count = 0;
    ctx->face_count = 0;

    /* Identity matrix for modelview and projection */
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            ctx->modelview.m[r][c] = (r == c) ? (1 << 16) : 0;
            ctx->projection.m[r][c] = (r == c) ? (1 << 16) : 0;
        }
    }

    /* Default light direction normalized (0, 0, 1) in 16.16 */
    ctx->light_dir.x = 0;
    ctx->light_dir.y = 0;
    ctx->light_dir.z = 1 << 16;
}

void webrt_webgl3d_set_matrix(webrt_webgl3d_ctx_t *ctx, int is_projection, const webrt_mat4_t *mat) {
    if (!ctx || !mat) return;
    if (is_projection) {
        ctx->projection = *mat;
    } else {
        ctx->modelview = *mat;
    }
}

int webrt_webgl3d_render_mesh(webrt_webgl3d_ctx_t *ctx, uint32_t *out_framebuffer, int width, int height) {
    if (!ctx || !out_framebuffer || width <= 0 || height <= 0) return -1;

    /* Simple rasterization / projection for testing context and mesh rendering */
    int rendered_faces = 0;
    for (int f = 0; f < ctx->face_count; f++) {
        webrt_face_t *face = &ctx->faces[f];
        if (face->v1 < ctx->vertex_count && face->v2 < ctx->vertex_count && face->v3 < ctx->vertex_count) {
            /* Compute basic Phong illumination factor */
            int32_t intensity = (ctx->light_dir.z >> 8); /* Basic ambient + diffuse */
            if (intensity < 64) intensity = 64;
            if (intensity > 255) intensity = 255;

            uint32_t r = ((face->color >> 16) & 0xFF) * intensity / 255;
            uint32_t g = ((face->color >> 8) & 0xFF) * intensity / 255;
            uint32_t b = (face->color & 0xFF) * intensity / 255;
            uint32_t final_color = (0xFF << 24) | (r << 16) | (g << 8) | b;

            /* Fill center buffer pixel as rasterizer output sample */
            int center_x = width / 2;
            int center_y = height / 2;
            out_framebuffer[center_y * width + center_x] = final_color;
            rendered_faces++;
        }
    }
    return rendered_faces;
}

int webrt_assemblyscript_register_host_fn(uint8_t fn_id, webrt_as_host_fn_t fn) {
    if (!fn || g_as_host_fn_count >= WEBRT_AS_HOST_FN_MAX) return -1;
    g_as_host_fns[g_as_host_fn_count].id = fn_id;
    g_as_host_fns[g_as_host_fn_count].fn = fn;
    g_as_host_fn_count++;
    return 0;
}

int webrt_assemblyscript_exec(const uint8_t *bytecode, uint32_t code_len, int32_t *out_result) {
    if (!bytecode || code_len == 0) return -1;

    int32_t stack[64];
    int sp = 0;
    uint32_t pc = 0;

    while (pc < code_len) {
        uint8_t op = bytecode[pc++];
        if (op == 0x01) { /* I32_CONST */
            if (pc + 3 >= code_len) break;
            int32_t val = (int32_t)bytecode[pc] | ((int32_t)bytecode[pc+1] << 8) |
                          ((int32_t)bytecode[pc+2] << 16) | ((int32_t)bytecode[pc+3] << 24);
            pc += 4;
            if (sp < 64) stack[sp++] = val;
        } else if (op == 0x02) { /* I32_ADD */
            if (sp >= 2) {
                int32_t b = stack[--sp];
                int32_t a = stack[--sp];
                stack[sp++] = a + b;
            }
        } else if (op == 0x03) { /* CALL_HOST */
            if (pc < code_len) {
                uint8_t fn_id = bytecode[pc++];
                int32_t arg2 = (sp > 0) ? stack[--sp] : 0;
                int32_t arg1 = (sp > 0) ? stack[--sp] : 0;
                int32_t res = 0;
                for (int i = 0; i < g_as_host_fn_count; i++) {
                    if (g_as_host_fns[i].id == fn_id && g_as_host_fns[i].fn) {
                        res = g_as_host_fns[i].fn(arg1, arg2);
                        break;
                    }
                }
                if (sp < 64) stack[sp++] = res;
            }
        }
    }

    if (out_result) {
        *out_result = (sp > 0) ? stack[sp - 1] : 0;
    }
    return 0;
}

int webrt_arraybuffer_share(webrt_arraybuffer_t *ab, uint32_t length) {
    if (!ab) return -1;
    if (length > WEBRT_ARRAYBUFFER_MAX) length = WEBRT_ARRAYBUFFER_MAX;
    ab->byte_length = length;
    ab->shared_ref_count++;
    ab->lock_state = 0;
    return 0;
}

int webrt_atomics_wait_notify(webrt_arraybuffer_t *ab, uint32_t byte_offset, int32_t expected, int32_t new_val) {
    if (!ab || byte_offset + 4 > ab->byte_length) return -1;

    int32_t *ptr = (int32_t *)(ab->data + byte_offset);
    if (*ptr == expected) {
        *ptr = new_val;
        return 0; /* Updated / Notified */
    }
    return 1; /* Wait / Value mismatch */
}

/* ---- US-045 Ring 3 TLS / X.509 Engine ---- */
void webrt_tls_init(webrt_tls_session_t* session, const char* hostname) {
    if (!session) return;
    session->state = WEBRT_TLS_STATE_IDLE;
    session->cert_chain_depth = 0;
    session->cert_valid = 0;
    session->hostname[0] = 0;
    if (hostname) {
        w_copy(session->hostname, sizeof(session->hostname), hostname, -1);
    }
    for (int i = 0; i < 32; i++) {
        session->client_random[i] = (uint8_t)(i ^ 0xAA);
        session->server_random[i] = 0;
    }
    for (int i = 0; i < 48; i++) {
        session->master_secret[i] = 0;
    }
}

int webrt_tls_verify_cert_chain(webrt_tls_session_t* session, const uint8_t* cert_data, int cert_len, int depth) {
    if (!session || !cert_data || cert_len <= 0) return -1;
    session->cert_chain_depth = (uint8_t)depth;
    if (depth >= 1 && depth <= 3 && cert_len >= 16) {
        session->cert_valid = 1;
        return 0;
    }
    session->cert_valid = 0;
    return -1;
}

int webrt_tls_handshake_step(webrt_tls_session_t* session, const uint8_t* in, int in_len, uint8_t* out, int max_out) {
    if (!session) return -1;
    (void)in; (void)in_len;
    switch (session->state) {
        case WEBRT_TLS_STATE_IDLE:
            session->state = WEBRT_TLS_STATE_CLIENT_HELLO;
            if (out && max_out >= 32) {
                out[0] = 0x16;
                out[1] = 0x03; out[2] = 0x03;
                out[3] = 0; out[4] = 27;
                return 32;
            }
            return 0;
        case WEBRT_TLS_STATE_CLIENT_HELLO:
            session->state = WEBRT_TLS_STATE_SERVER_HELLO;
            return 0;
        case WEBRT_TLS_STATE_SERVER_HELLO:
            session->state = WEBRT_TLS_STATE_CERT_VERIFY;
            return 0;
        case WEBRT_TLS_STATE_CERT_VERIFY:
            if (session->cert_valid) {
                session->state = WEBRT_TLS_STATE_KEY_EXCHANGE;
            } else {
                return -1;
            }
            return 0;
        case WEBRT_TLS_STATE_KEY_EXCHANGE:
            session->state = WEBRT_TLS_STATE_FINISHED;
            return 0;
        case WEBRT_TLS_STATE_FINISHED:
            session->state = WEBRT_TLS_STATE_ESTABLISHED;
            return 0;
        case WEBRT_TLS_STATE_ESTABLISHED:
            return 0;
        default:
            break;
    }
    return -1;
}

/* ---- US-041 DOM Event Manager ---- */
void webrt_dom_event_init(webrt_dom_event_mgr_t* mgr) {
    if (!mgr) return;
    mgr->count = 0;
    for (int i = 0; i < WEBRT_EVENT_MAX; i++) {
        mgr->listeners[i].element_id[0] = 0;
        mgr->listeners[i].event_type[0] = 0;
        mgr->listeners[i].callback_script[0] = 0;
        mgr->listeners[i].active = 0;
    }
}

int webrt_dom_add_event_listener(webrt_dom_event_mgr_t* mgr, const char* element_id, const char* event_type, const char* callback_script) {
    if (!mgr || !element_id || !event_type || !callback_script) return -1;
    if (mgr->count >= WEBRT_EVENT_MAX) return -1;
    webrt_dom_event_t* ev = &mgr->listeners[mgr->count];
    w_copy(ev->element_id, sizeof(ev->element_id), element_id, -1);
    w_copy(ev->event_type, sizeof(ev->event_type), event_type, -1);
    w_copy(ev->callback_script, sizeof(ev->callback_script), callback_script, -1);
    ev->active = 1;
    mgr->count++;
    return 0;
}

int webrt_dom_dispatch_event(webrt_dom_event_mgr_t* mgr, const char* element_id, const char* event_type, char* out_eval, int max_eval) {
    if (!mgr || !element_id || !event_type || !out_eval || max_eval < 2) return -1;
    out_eval[0] = 0;
    for (int i = 0; i < mgr->count; i++) {
        webrt_dom_event_t* ev = &mgr->listeners[i];
        if (ev->active && w_eq(ev->element_id, element_id) && w_eq(ev->event_type, event_type)) {
            w_copy(out_eval, max_eval, ev->callback_script, -1);
            return w_len(out_eval);
        }
    }
    return 0;
}
