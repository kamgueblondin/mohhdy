/* userspace/prod.c - Phase 8 production toolkit. See prod.h. */
#include "prod.h"

static int s_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static void s_copy(char* d, const char* s, int cap) { int i = 0; while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; } d[i] = 0; }
static void s_cat(char* d, const char* s, int cap) { int n = s_len(d), i = 0; while (s && s[i] && n < cap - 1) d[n++] = s[i++]; d[n] = 0; }
static void cat_u(char* d, uint32_t v, int cap) { char t[11], r[12]; int n = 0, i = 0; do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v); while (n) r[i++] = t[--n]; r[i] = 0; s_cat(d, r, cap); }
static void mzero(void* p, int n) { int i; for (i = 0; i < n; i++) ((uint8_t*)p)[i] = 0; }

static const char* const k_metrics[PM_COUNT] = {"mem_used", "mem_free", "procs", "busy", "custom"};
const char* prod_metric_name(int id) { return (id >= 0 && id < PM_COUNT) ? k_metrics[id] : "?"; }
int prod_metric_id(const char* name) { int i; for (i = 0; i < PM_COUNT; i++) if (s_eq(name, k_metrics[i])) return i; return -1; }

void prod_metrics_init(prod_metrics_t* m) { mzero(m, (int)sizeof(*m)); }
void prod_metrics_push(prod_metrics_t* m, const prod_sample_t* s) {
    m->s[m->head] = *s;
    m->head = (m->head + 1) % PROD_SAMPLES;
    if (m->n < PROD_SAMPLES) m->n++;
    m->total++;
}
static const prod_sample_t* at(const prod_metrics_t* m, int i) { /* 0 = oldest */
    return &m->s[(m->head - m->n + i + PROD_SAMPLES) % PROD_SAMPLES];
}
int prod_metric_stat(const prod_metrics_t* m, int id, prod_stat_t* o) {
    int i; uint32_t sum = 0;
    if (id < 0 || id >= PM_COUNT || m->n == 0) return -1;
    o->min = 0xFFFFFFFFU; o->max = 0; o->n = m->n;
    for (i = 0; i < m->n; i++) {
        uint32_t v = at(m, i)->v[id];
        if (v < o->min) o->min = v;
        if (v > o->max) o->max = v;
        sum += v;
    }
    o->avg = sum / (uint32_t)m->n; o->last = at(m, m->n - 1)->v[id];
    return 0;
}
int prod_predict(const prod_metrics_t* m, int id, uint32_t limit, int32_t* slope_milli, uint32_t* ticks_left) {
    /* Two-halves trend (no 64-bit division in the freestanding guest):
     * slope = (mean value of the newer half - older half) / (mean tick gap). */
    int i, n = m->n, h;
    uint32_t t0, ta = 0, tb = 0, va = 0, vb = 0, dt, last;
    int32_t dv;
    if (id < 0 || id >= PM_COUNT || n < 4) return -1;
    h = n / 2; t0 = at(m, 0)->tick;
    for (i = 0; i < h; i++) { ta += at(m, i)->tick - t0; va += at(m, i)->v[id]; }
    for (i = n - h; i < n; i++) { tb += at(m, i)->tick - t0; vb += at(m, i)->v[id]; }
    ta /= (uint32_t)h; tb /= (uint32_t)h; va /= (uint32_t)h; vb /= (uint32_t)h;
    dt = tb - ta;
    if (dt == 0) return -1;
    dv = (int32_t)vb - (int32_t)va;
    *slope_milli = (int32_t)((dv * 1000) / (int32_t)dt);
    last = at(m, n - 1)->v[id];
    *ticks_left = 0xFFFFFFFFU;
    if (*slope_milli > 0 && limit > last) *ticks_left = (limit - last) * 1000U / (uint32_t)*slope_milli;
    else if (*slope_milli < 0 && limit < last) *ticks_left = (last - limit) * 1000U / (uint32_t)(-*slope_milli);
    else if (*slope_milli > 0 && limit <= last) *ticks_left = 0;
    return 0;
}

/* ------------------------------------------------------------- alerts */
void prod_alerts_init(prod_alerts_t* a) { mzero(a, (int)sizeof(*a)); }
void prod_event(prod_alerts_t* a, uint32_t tick, const char* text) {
    prod_event_t* e = &a->ev[a->ev_head];
    e->tick = tick; s_copy(e->text, text, (int)sizeof(e->text));
    a->ev_head = (a->ev_head + 1) % PROD_EVENTS;
    if (a->ev_n < PROD_EVENTS) a->ev_n++;
}
int prod_alert_add(prod_alerts_t* a, const char* name, int metric, int above, uint32_t threshold, int for_n) {
    int i, slot = -1;
    if (!name[0] || s_len(name) >= 16 || metric < 0 || metric >= PM_COUNT || for_n < 1 || for_n > 16) return -1;
    for (i = 0; i < PROD_RULES; i++) {
        if (a->r[i].used && s_eq(a->r[i].name, name)) return -2;
        if (!a->r[i].used && slot < 0) slot = i;
    }
    if (slot < 0) return -3;
    mzero(&a->r[slot], (int)sizeof(a->r[slot]));
    s_copy(a->r[slot].name, name, 16);
    a->r[slot].metric = metric; a->r[slot].above = above; a->r[slot].threshold = threshold; a->r[slot].for_n = for_n; a->r[slot].used = 1;
    return slot;
}
int prod_alert_remove(prod_alerts_t* a, const char* name) {
    int i;
    for (i = 0; i < PROD_RULES; i++) if (a->r[i].used && s_eq(a->r[i].name, name)) { a->r[i].used = 0; return 0; }
    return -1;
}
static void ev2(prod_alerts_t* a, uint32_t tick, const char* what, const char* name, uint32_t v) {
    char t[64], n[12]; int k = 0, i = 0;
    t[0] = 0;
    while (what[i] && k < 60) t[k++] = what[i++];
    i = 0; while (name[i] && k < 60) t[k++] = name[i++];
    if (k < 52) {
        int m = 0;
        t[k++] = ' '; t[k++] = 'v'; t[k++] = '=';
        do { n[m++] = (char)('0' + v % 10U); v /= 10U; } while (v && m < 11);
        while (m) t[k++] = n[--m];
    }
    t[k] = 0;
    prod_event(a, tick, t);
}
int prod_alert_add_adaptive(prod_alerts_t* a, const char* name, int metric, int k, int for_n) {
    int i;
    if (k < 1 || k > 20) return -1;
    i = prod_alert_add(a, name, metric, 1, 0xFFFFFFFFU, for_n);
    if (i < 0) return i;
    a->r[i].adaptive = 1; a->r[i].k = k; a->r[i].seen = 0; a->r[i].ewma16 = 0; a->r[i].dev16 = 0;
    return i;
}
static uint32_t adapt_limit(const prod_rule_t* r) {
    uint32_t floor16 = r->ewma16 / 20U + 16U;            /* 5% of the baseline, at least 1 */
    uint32_t d = r->dev16 > floor16 ? r->dev16 : floor16;
    return (r->ewma16 + (uint32_t)r->k * d) / 16U;
}
static void adapt_learn(prod_rule_t* r, uint32_t v) {
    uint32_t v16 = v > 0x0FFFFFFFU ? 0xFFFFFFF0U : v * 16U, diff;
    if (r->seen == 0) { r->ewma16 = v16; r->dev16 = 0; }
    else {
        if (v16 >= r->ewma16) r->ewma16 += (v16 - r->ewma16) / 8U; else r->ewma16 -= (r->ewma16 - v16) / 8U;
        diff = v16 > r->ewma16 ? v16 - r->ewma16 : r->ewma16 - v16;
        if (diff >= r->dev16) r->dev16 += (diff - r->dev16) / 8U; else r->dev16 -= (r->dev16 - diff) / 8U;
    }
    if (r->seen < 1000) r->seen++;
}
int prod_alerts_eval(prod_alerts_t* a, const prod_sample_t* s) {
    int i, changes = 0;
    for (i = 0; i < PROD_RULES; i++) {
        prod_rule_t* r = &a->r[i];
        uint32_t v;
        int bad;
        if (!r->used) continue;
        v = s->v[r->metric];
        if (r->adaptive) {
            /* anomalies are not learned, so a sustained spike keeps firing */
            if (r->seen < PROD_LEARN) { adapt_learn(r, v); r->threshold = adapt_limit(r); continue; }
            r->threshold = adapt_limit(r);
            bad = v > r->threshold;
            if (!bad) { adapt_learn(r, v); r->threshold = adapt_limit(r); }
        } else
        bad = r->above ? (v > r->threshold) : (v < r->threshold);
        if (bad) {
            r->clear = 0;
            if (r->breach < 1000) r->breach++;
            if (!r->firing && r->breach >= r->for_n) { r->firing = 1; a->fired_total++; changes++; ev2(a, s->tick, "FIRING ", r->name, v); }
            else if (r->firing) a->suppressed++; /* deduplicated: already firing */
        } else {
            r->breach = 0;
            if (r->firing && ++r->clear >= 2) { r->firing = 0; r->clear = 0; a->resolved_total++; changes++; ev2(a, s->tick, "RESOLVED ", r->name, v); }
        }
    }
    return changes;
}

/* ---------------------------------------------------------- log analysis */
static int has_word(const char* line, int len, const char* w) {
    int i, j, wl = s_len(w);
    for (i = 0; i + wl <= len; i++) {
        for (j = 0; j < wl; j++) {
            char c = line[i + j];
            if (c >= 'a' && c <= 'z') c = (char)(c - 32);
            if (c != w[j]) break;
        }
        if (j == wl) return 1;
    }
    return 0;
}
static void normalize(const char* line, int len, char* out, int cap) {
    int i, k = 0, in_num = 0;
    for (i = 0; i < len && k < cap - 1; i++) {
        char c = line[i];
        int digit = (c >= '0' && c <= '9');
        if (digit) { if (!in_num) out[k++] = '#'; in_num = 1; continue; }
        in_num = 0;
        out[k++] = c;
    }
    out[k] = 0;
}
void prod_log_analyze(const char* text, prod_log_report_t* r) {
    static uint8_t win[10];
    static prod_pattern_t pats[32];
    int npat = 0, i, wpos = 0;
    uint32_t wsum = 0;
    const char* p = text;
    mzero(r, (int)sizeof(*r)); mzero(win, 10);
    while (p && *p) {
        const char* e = p;
        int len, lvl, err;
        while (*e && *e != '\n') e++;
        len = (int)(e - p);
        if (len > 0) {
            char norm[48];
            r->lines++;
            err = has_word(p, len, "ERROR") || has_word(p, len, "FAIL") || has_word(p, len, "PANIC");
            lvl = err ? 2 : has_word(p, len, "WARN") ? 1 : has_word(p, len, "INFO") ? 0 : 3;
            if (lvl == 2) { r->errors++; if (!r->first_error[0]) { int n = len < 63 ? len : 63; for (i = 0; i < n; i++) r->first_error[i] = p[i]; r->first_error[n] = 0; } }
            else if (lvl == 1) r->warns++; else if (lvl == 0) r->infos++; else r->other++;
            wsum -= win[wpos]; win[wpos] = (uint8_t)(lvl == 2); wsum += win[wpos]; wpos = (wpos + 1) % 10;
            if (wsum > r->burst_max) { r->burst_max = wsum; r->burst_line = r->lines; }
            if (lvl >= 1 && lvl <= 2) {
                normalize(p, len, norm, (int)sizeof(norm));
                for (i = 0; i < npat && !s_eq(pats[i].pattern, norm); i++) {}
                if (i < npat) pats[i].count++;
                else if (npat < 32) { s_copy(pats[npat].pattern, norm, 48); pats[npat].count = 1; npat++; }
            }
        }
        p = *e ? e + 1 : e;
    }
    for (r->ntop = 0; r->ntop < 5; r->ntop++) { /* top 5 by count, first seen wins ties */
        int best = -1;
        for (i = 0; i < npat; i++) if (pats[i].count && (best < 0 || pats[i].count > pats[best].count)) best = i;
        if (best < 0) break;
        r->top[r->ntop] = pats[best]; pats[best].count = 0;
    }
}

/* ------------------------------------------------------------- archive */
uint32_t prod_fnv(const void* p, uint32_t n) {
    uint32_t h = 2166136261U, i;
    for (i = 0; i < n; i++) { h ^= ((const uint8_t*)p)[i]; h *= 16777619U; }
    return h;
}
void prod_archive_init(prod_archive_t* a, const char* label, uint32_t tick) {
    a->n = 0; a->used = 0; a->tick = tick; s_copy(a->label, label, 16);
}
int prod_archive_add(prod_archive_t* a, const char* path, const void* data, uint32_t len) {
    uint32_t i;
    if (a->n >= PROD_ARCH_FILES || s_len(path) >= 64) return -1;
    if (a->used + len > PROD_ARCH_BYTES) return -2;
    s_copy(a->e[a->n].path, path, 64);
    a->e[a->n].off = a->used; a->e[a->n].len = len; a->e[a->n].fnv = prod_fnv(data, len);
    for (i = 0; i < len; i++) a->data[a->used + i] = ((const uint8_t*)data)[i];
    a->used += len; a->n++;
    return a->n - 1;
}
int prod_archive_verify(const prod_archive_t* a, int* first_bad) {
    int i, bad = 0;
    if (first_bad) *first_bad = -1;
    for (i = 0; i < a->n; i++)
        if (prod_fnv(a->data + a->e[i].off, a->e[i].len) != a->e[i].fnv) { if (!bad && first_bad) *first_bad = i; bad++; }
    return bad;
}

/* ------------------------------------------------------------- manifest */
static int hexval(const char* s, uint32_t* v) {
    uint32_t r = 0; int n = 0;
    while ((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f')) { r = r * 16U + (uint32_t)(*s <= '9' ? *s - '0' : *s - 'a' + 10); s++; n++; }
    if (n == 0 || n > 8) return -1;
    *v = r; return n;
}
int prod_manifest_check(const char* m, prod_read_fn rd, void* ctx, char* why, int cap) {
    static char buf[4096];
    int files = 0;
    why[0] = 0;
    while (*m) {
        char path[64]; int k = 0, n; uint32_t want;
        while (*m == '\n' || *m == ' ') m++;
        if (!*m) break;
        while (*m && *m != ' ' && *m != '\n' && k < 63) path[k++] = *m++;
        path[k] = 0;
        while (*m == ' ') m++;
        if (hexval(m, &want) < 0) { s_copy(why, "bad manifest line", cap); return -1; }
        while (*m && *m != '\n') m++;
        n = rd(ctx, path, buf, (int)sizeof(buf));
        if (n < 0) { s_copy(why, "missing ", cap); { int l = s_len(why), i = 0; while (path[i] && l < cap - 1) why[l++] = path[i++]; why[l] = 0; } return -2; }
        if (prod_fnv(buf, (uint32_t)n) != want) { s_copy(why, "checksum ", cap); { int l = s_len(why), i = 0; while (path[i] && l < cap - 1) why[l++] = path[i++]; why[l] = 0; } return -3; }
        files++;
    }
    if (!files) { s_copy(why, "empty manifest", cap); return -1; }
    return files;
}

/* -------------------------------------------------------------- scaler */
void prod_scaler_init(prod_scaler_t* s, int min, int max, uint32_t hi, uint32_t lo, uint32_t cd) {
    mzero(s, (int)sizeof(*s));
    s->min = min; s->max = max; s->cur = min; s->high_milli = hi; s->low_milli = lo; s->cooldown = cd;
    s->last_change = 0;
}
int prod_scaler_step(prod_scaler_t* s, uint32_t queue, uint32_t now) {
    uint32_t per = queue * 1000U / (uint32_t)(s->cur > 0 ? s->cur : 1);
    if (s->ups + s->downs > 0 && now - s->last_change < s->cooldown) return s->cur;
    if (per > s->high_milli && s->cur < s->max) { s->cur++; s->ups++; s->last_change = now; }
    else if (per < s->low_milli && s->cur > s->min) { s->cur--; s->downs++; s->last_change = now; }
    return s->cur;
}

/* ------------------------------------------------------------ baseline */
int prod_baseline_add(prod_baseline_t* b, const char* path, uint32_t fnv) {
    if (b->n >= PROD_BASE || s_len(path) >= 64) return -1;
    s_copy(b->e[b->n].path, path, 64); b->e[b->n].fnv = fnv; b->n++;
    return 0;
}
int prod_baseline_cmp(const prod_baseline_t* b, const char* path, uint32_t fnv) {
    int i;
    for (i = 0; i < b->n; i++) if (s_eq(b->e[i].path, path)) return b->e[i].fnv == fnv ? 0 : 1;
    return 2;
}

/* ------------------------------------------------------ feedback, usage */
int prod_feedback_add(prod_feedback_t* f, int rating, const char* text) {
    int slot;
    if (rating < 1 || rating > 5) return -1;
    slot = f->n < PROD_FEEDBACK ? f->n++ : (int)(f->total % PROD_FEEDBACK);
    f->f[slot].rating = (uint8_t)rating; s_copy(f->f[slot].text, text, 48);
    f->total++; f->sum += (uint32_t)rating; f->hist[rating]++;
    return 0;
}
void prod_usage_hit(prod_usage_t* u, const char* name) {
    int i;
    u->total++;
    for (i = 0; i < u->n; i++) if (s_eq(u->u[i].name, name)) { u->u[i].count++; return; }
    if (u->n >= PROD_USAGE) { u->other++; return; }
    s_copy(u->u[u->n].name, name, 20); u->u[u->n].count = 1; u->n++;
}
void prod_usage_sort(prod_usage_t* u) {
    int i, j;
    for (i = 1; i < u->n; i++) {
        prod_use_t k = u->u[i];
        for (j = i - 1; j >= 0 && u->u[j].count < k.count; j--) u->u[j + 1] = u->u[j];
        u->u[j + 1] = k;
    }
}

/* ---------------------------------------------------------- persistence */
typedef struct { uint8_t* p; int n, cap, bad; } wr_t;
typedef struct { const uint8_t* p; int n, pos, bad; } rd_t;
static void w8(wr_t* w, uint32_t v) { if (w->n >= w->cap) { w->bad = 1; return; } w->p[w->n++] = (uint8_t)v; }
static void w32(wr_t* w, uint32_t v) { w8(w, v); w8(w, v >> 8); w8(w, v >> 16); w8(w, v >> 24); }
static void wstr(wr_t* w, const char* s) { int l = s_len(s), i; if (l > 255) l = 255; w8(w, (uint32_t)l); for (i = 0; i < l; i++) w8(w, (uint8_t)s[i]); }
static void wbytes(wr_t* w, const uint8_t* b, uint32_t n) { uint32_t i; for (i = 0; i < n; i++) w8(w, b[i]); }
static uint32_t r8(rd_t* r) { if (r->pos >= r->n) { r->bad = 1; return 0; } return r->p[r->pos++]; }
static uint32_t r32(rd_t* r) { uint32_t v = r8(r); v |= r8(r) << 8; v |= r8(r) << 16; v |= r8(r) << 24; return v; }
static void rstr(rd_t* r, char* d, int cap) { int l = (int)r8(r), i; for (i = 0; i < l; i++) { char c = (char)r8(r); if (i < cap - 1) d[i] = c; } d[l < cap - 1 ? l : cap - 1] = 0; }
#define PROD_MAGIC 0x31445250U /* "PRD1" */

int prod_state_save(const prod_state_ref_t* r, uint8_t* out, int cap) {
    wr_t w; int i, k, ns, ne;
    w.p = out; w.n = 0; w.cap = cap; w.bad = 0;
    w32(&w, PROD_MAGIC);
    ns = r->m->n < 16 ? r->m->n : 16;
    w32(&w, r->m->total); w8(&w, (uint32_t)ns);
    for (i = r->m->n - ns; i < r->m->n; i++) { const prod_sample_t* s = at(r->m, i); w32(&w, s->tick); for (k = 0; k < PM_COUNT; k++) w32(&w, s->v[k]); }
    k = 0; for (i = 0; i < PROD_RULES; i++) k += r->a->r[i].used;
    w8(&w, (uint32_t)k);
    for (i = 0; i < PROD_RULES; i++) if (r->a->r[i].used) {
        const prod_rule_t* q = &r->a->r[i];
        wstr(&w, q->name); w8(&w, (uint32_t)q->metric); w8(&w, (uint32_t)q->above); w32(&w, q->threshold);
        w8(&w, (uint32_t)q->for_n); w32(&w, (uint32_t)q->breach); w8(&w, (uint32_t)q->clear); w8(&w, (uint32_t)q->firing);
    }
    w32(&w, r->a->fired_total); w32(&w, r->a->resolved_total); w32(&w, r->a->suppressed);
    ne = r->a->ev_n < 8 ? r->a->ev_n : 8;
    w8(&w, (uint32_t)ne);
    for (i = r->a->ev_n - ne; i < r->a->ev_n; i++) {
        const prod_event_t* e = &r->a->ev[(r->a->ev_head - r->a->ev_n + i + PROD_EVENTS) % PROD_EVENTS];
        w32(&w, e->tick); wstr(&w, e->text);
    }
    w8(&w, (uint32_t)r->slots);
    for (i = 0; i < r->slots; i++) {
        const prod_archive_t* a = &r->arch[i];
        wstr(&w, a->label); wstr(&w, r->dir[i]); w32(&w, a->tick); w8(&w, (uint32_t)a->n);
        for (k = 0; k < a->n; k++) { wstr(&w, a->e[k].path); w32(&w, a->e[k].len); w32(&w, a->e[k].fnv); wbytes(&w, a->data + a->e[k].off, a->e[k].len); }
    }
    w32(&w, r->fb->total); w32(&w, r->fb->sum);
    for (i = 0; i < 6; i++) w32(&w, r->fb->hist[i]);
    w8(&w, (uint32_t)r->fb->n);
    for (i = 0; i < r->fb->n; i++) { w8(&w, r->fb->f[i].rating); wstr(&w, r->fb->f[i].text); }
    return w.bad ? -1 : w.n;
}

int prod_state_load(const prod_state_ref_t* r, const uint8_t* in, int len) {
    rd_t d; int i, k, ns, nr, ne, sl;
    d.p = in; d.n = len; d.pos = 0; d.bad = 0;
    prod_metrics_init(r->m); prod_alerts_init(r->a); mzero(r->fb, (int)sizeof(*r->fb));
    for (i = 0; i < r->slots; i++) { prod_archive_init(&r->arch[i], "", 0); r->dir[i][0] = 0; }
    if (r32(&d) != PROD_MAGIC) return -1;
    { uint32_t total = r32(&d); ns = (int)r8(&d);
      for (i = 0; i < ns && !d.bad; i++) { prod_sample_t s; s.tick = r32(&d); for (k = 0; k < PM_COUNT; k++) s.v[k] = r32(&d); prod_metrics_push(r->m, &s); }
      r->m->total = total; }
    nr = (int)r8(&d);
    for (i = 0; i < nr && i < PROD_RULES && !d.bad; i++) {
        prod_rule_t* q = &r->a->r[i];
        rstr(&d, q->name, 16); q->metric = (int)r8(&d); q->above = (int)r8(&d); q->threshold = r32(&d);
        q->for_n = (int)r8(&d); q->breach = (int)r32(&d); q->clear = (int)r8(&d); q->firing = (int)r8(&d); q->used = 1;
        if (q->metric >= PM_COUNT) d.bad = 1;
    }
    r->a->fired_total = r32(&d); r->a->resolved_total = r32(&d); r->a->suppressed = r32(&d);
    ne = (int)r8(&d);
    for (i = 0; i < ne && !d.bad; i++) { char t[64]; uint32_t tick = r32(&d); rstr(&d, t, 64); prod_event(r->a, tick, t); }
    sl = (int)r8(&d);
    for (i = 0; i < sl && !d.bad; i++) {
        char label[16], dir[64]; uint32_t tick; int n;
        rstr(&d, label, 16); rstr(&d, dir, 64); tick = r32(&d); n = (int)r8(&d);
        if (i < r->slots) { prod_archive_init(&r->arch[i], label, tick); s_copy(r->dir[i], dir, 64); }
        for (k = 0; k < n && !d.bad; k++) {
            char path[64]; uint32_t l, f; int idx;
            rstr(&d, path, 64); l = r32(&d); f = r32(&d);
            if (d.pos + (int)l > d.n) { d.bad = 1; break; }
            if (i < r->slots) { idx = prod_archive_add(&r->arch[i], path, d.p + d.pos, l); if (idx >= 0) r->arch[i].e[idx].fnv = f; else d.bad = 1; }
            d.pos += (int)l;
        }
    }
    r->fb->total = r32(&d); r->fb->sum = r32(&d);
    for (i = 0; i < 6; i++) r->fb->hist[i] = r32(&d);
    ne = (int)r8(&d);
    for (i = 0; i < ne && i < PROD_FEEDBACK && !d.bad; i++) { r->fb->f[i].rating = (uint8_t)r8(&d); rstr(&d, r->fb->f[i].text, 48); }
    r->fb->n = i;
    if (d.bad) {
        prod_metrics_init(r->m); prod_alerts_init(r->a); mzero(r->fb, (int)sizeof(*r->fb));
        for (i = 0; i < r->slots; i++) { prod_archive_init(&r->arch[i], "", 0); r->dir[i][0] = 0; }
        return -1;
    }
    return 0;
}

/* ---------------------------------------------------------- US-110 load */
int prod_runq(const prod_task_t* t, int n, const int* exclude, int nexclude, uint32_t min_ran) {
    int i, j, q = 0;
    for (i = 0; i < n; i++) {
        int skip = 0;
        if (!t[i].user || (t[i].state != 0 && t[i].state != 1) || t[i].ran < min_ran) continue; /* running / ready, busy */
        for (j = 0; j < nexclude; j++) if (exclude[j] == t[i].pid) skip = 1;
        if (!skip) q++;
    }
    return q;
}

/* ------------------------------------------------------ US-112 security */
static const char* const k_known_tasks[] = {
    "shell", "idle", "spin", "networker", "atadriver", "vfsserver", "vfsvirtual", "aiworker", "aiclient",
    "aistat", "ggufclient", "ipcserver", "ipcwait", "ok", "waitchild", "user_program", "fake_ai", "ai_assistant", 0
};
static const char* const k_test_tasks[] = { "atarogue", "airogue", "netclaim", "serviceclaim", "vfsclaim", "netwire", "ggufpause", 0 };
static int in_list(const char* const* l, const char* n) {
    int i; const char* b = n; int k;
    for (k = 0; n[k]; k++) if (n[k] == '/') b = n + k + 1;   /* basename */
    for (i = 0; l[i]; i++) if (s_eq(l[i], b)) return 1;
    return 0;
}
static void sec_line(char* out, int cap, const char* sev, const char* what, const char* detail, int* f, int* c, int* score, int cost) {
    s_cat(out, "prod-sec ", cap); s_cat(out, sev, cap); s_cat(out, " ", cap); s_cat(out, what, cap);
    if (detail && detail[0]) { s_cat(out, " ", cap); s_cat(out, detail, cap); }
    s_cat(out, "\n", cap);
    (*f)++; if (sev[0] == 'c') (*c)++;
    *score -= cost;
}
int prod_sec_scan(const prod_sec_input_t* in, char* out, int cap, int* findings, int* critical) {
    int i, f = 0, c = 0, score = 100;
    char t[48];
    for (i = 0; i < in->ntask; i++) {
        if (in_list(k_known_tasks, in->task[i])) continue;
        if (in_list(k_test_tasks, in->task[i])) sec_line(out, cap, "critical", "capability test/rogue binary running:", in->task[i], &f, &c, &score, 30);
        else sec_line(out, cap, "warn", "capability unknown task:", in->task[i], &f, &c, &score, 10);
    }
    for (i = 0; i < in->nport; i++) {
        t[0] = 0; s_cat(t, in->port[i].udp ? "udp/" : "tcp/", 48); cat_u(t, in->port[i].port, 48);
        s_cat(t, " ", 48); s_cat(t, in->port[i].what, 48);
        s_cat(out, "prod-sec info open port ", cap); s_cat(out, t, cap);
        s_cat(out, in->port[i].encrypted ? " encrypted\n" : " plaintext\n", cap);
        if (!in->port[i].encrypted) sec_line(out, cap, "warn", "port without encryption:", t, &f, &c, &score, 15);
    }
    if (in->netkey_len > 0) {
        if (in->netkey_default) sec_line(out, cap, "critical", "weak key: default P2P network key", "", &f, &c, &score, 30);
        else if (in->netkey_len < 12) sec_line(out, cap, "warn", "weak key: P2P network key under 12 chars", "", &f, &c, &score, 15);
    }
    if (in->signing_key && !in->key_at_rest_encrypted) sec_line(out, cap, "warn", "weak key: signing key not encrypted at rest (no passphrase)", "", &f, &c, &score, 15);
    if (in->pass_len > 0 && in->pass_len < 10) sec_line(out, cap, "warn", "weak key: passphrase under 10 chars", "", &f, &c, &score, 15);
    if (score < 0) score = 0;
    if (findings) *findings = f;
    if (critical) *critical = c;
    return score;
}
