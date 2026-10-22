/* userspace/prod.c - Phase 8 production toolkit. See prod.h. */
#include "prod.h"

static int s_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static void s_copy(char* d, const char* s, int cap) { int i = 0; while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; } d[i] = 0; }
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
int prod_alerts_eval(prod_alerts_t* a, const prod_sample_t* s) {
    int i, changes = 0;
    for (i = 0; i < PROD_RULES; i++) {
        prod_rule_t* r = &a->r[i];
        uint32_t v;
        int bad;
        if (!r->used) continue;
        v = s->v[r->metric];
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

int prod_perf_tune_eval(prod_perf_tune_t* pt, const prod_metrics_t* m, uint32_t alu, uint32_t mem,
                        uint32_t sys, uint32_t io, int requested_mode) {
    prod_stat_t st;
    if (!pt || requested_mode > 2) return -1;
    mzero(pt, (int)sizeof(*pt));
    pt->cpu_perf_score = alu; pt->mem_perf_score = mem; pt->sys_perf_score = sys; pt->io_perf_score = io;
    if (m && prod_metric_stat(m, PM_BUSY, &st) == 0 && st.n > 0) { pt->busy_avg = st.avg; pt->samples = (uint32_t)st.n; }
    if (requested_mode >= 0) { pt->tuning_mode = (uint32_t)requested_mode; return 0; }
    pt->auto_chosen = 1;
    if (pt->samples == 0) pt->tuning_mode = 0;
    else if (pt->busy_avg >= 70U) pt->tuning_mode = 1;      /* saturated: batch work */
    else if (pt->busy_avg <= 20U) pt->tuning_mode = 2;      /* mostly idle: favour latency */
    else pt->tuning_mode = 0;
    return 0;
}
const char* prod_perf_tune_mode_name(uint32_t mode) {
    return mode == 1 ? "high-throughput" : mode == 2 ? "low-latency" : "balanced";
}

int prod_maint_predict(const prod_metrics_t* m, const prod_alerts_t* a, prod_maint_t* pm) {
    int32_t slope;
    uint32_t left = 0xFFFFFFFFU, risk;
    int i;
    if (!pm) return -1;
    mzero(pm, (int)sizeof(*pm));
    if (a) for (i = 0; i < PROD_RULES; i++) if (a->r[i].used && a->r[i].firing) pm->firing_alerts++;
    if (m && prod_predict(m, PM_MEM_FREE, 0U, &slope, &left) != 0) left = 0xFFFFFFFFU;
    pm->exhaust_ticks = left;
    risk = pm->firing_alerts * 25U;
    if (left != 0xFFFFFFFFU) risk += left < 6000U ? 50U : left < 60000U ? 25U : 0U;  /* < 1 min, < 10 min at 100 Hz */
    pm->failure_risk = risk > 100U ? 100U : risk;
    pm->health_score = 100U - pm->failure_risk;
    return 0;
}

/* ------------------------------------------- US-115 learned predictor */
static const int32_t k_sig[33] = {
    1, 2, 4, 6, 10, 17, 27, 45, 74, 120, 194, 311, 488, 747, 1102, 1546, 2048,
    2550, 2994, 3349, 3608, 3785, 3902, 3976, 4022, 4051, 4069, 4079, 4086, 4090, 4092, 4094, 4095
};
int32_t prod_sigmoid_q12(int32_t z) {
    int32_t u, i, f;
    if (z <= -32768) return k_sig[0];
    if (z >= 32768) return k_sig[32];
    u = z + 32768;                 /* 0 .. 65536, step 0.5 = 2048 */
    i = u >> 11; f = u & 2047;
    if (i >= 32) return k_sig[32];
    return k_sig[i] + (((k_sig[i + 1] - k_sig[i]) * f) >> 11);
}
static int32_t mm_clamp(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }
/* v * 4096 / d for small values, without a division instruction issue:
 * plain 32-bit unsigned division (no libgcc needed). */
static int32_t mm_scale(uint32_t v, uint32_t d) {
    if (!d) return 0;
    if (v > 1000000U) v = 1000000U;
    return (int32_t)((v * 4096U) / d);
}
static int mm_failure(const prod_maint_model_t* mm, const prod_sample_t* s) {
    uint32_t tot = s->v[PM_MEM_USED] + s->v[PM_MEM_FREE];
    if (s->v[PM_CUSTOM] >= mm->fail_at) return 1;
    if (mm->fail_mem_pct && tot && s->v[PM_MEM_FREE] * 100U <= tot * mm->fail_mem_pct) return 1;
    return 0;
}
int prod_maint_features(const prod_metrics_t* m, const prod_maint_model_t* mm, int end, int32_t x[PROD_MM_FEAT]) {
    const prod_sample_t *a, *b;
    uint32_t tot;
    int32_t d;
    if (!m || !mm || end < PROD_MM_WIN - 1 || end >= m->n || !mm->fail_at) return -1;
    a = at(m, end - (PROD_MM_WIN - 1)); b = at(m, end);
    x[0] = mm_clamp(mm_scale(b->v[PM_CUSTOM], mm->fail_at), 0, 8192);
    d = b->v[PM_CUSTOM] >= a->v[PM_CUSTOM] ? mm_scale(b->v[PM_CUSTOM] - a->v[PM_CUSTOM], mm->fail_at)
                                           : -mm_scale(a->v[PM_CUSTOM] - b->v[PM_CUSTOM], mm->fail_at);
    x[1] = mm_clamp(d, -8192, 8192);
    tot = b->v[PM_MEM_USED] + b->v[PM_MEM_FREE];
    x[2] = tot ? mm_clamp(mm_scale(b->v[PM_MEM_USED] > 1000000U ? 1000000U : b->v[PM_MEM_USED],
                                   tot > 1000000U ? 1000000U : tot), 0, 4096) : 0;
    x[3] = mm_clamp(mm_scale(b->v[PM_BUSY], 100U), 0, 4096);
    x[4] = 4096;
    return 0;
}
static int32_t mm_prob(const int32_t* w, const int32_t* x) {
    int64_t z = 0;
    int j;
    for (j = 0; j < PROD_MM_FEAT; j++) z += (int64_t)w[j] * (int64_t)x[j];
    z >>= 12;
    if (z > 1000000) z = 1000000;
    if (z < -1000000) z = -1000000;
    return prod_sigmoid_q12((int32_t)z);
}
static int mm_label(const prod_metrics_t* m, const prod_maint_model_t* mm, int end) {
    int k;
    for (k = end + 1; k <= end + (int)mm->horizon && k < m->n; k++) if (mm_failure(mm, at(m, k))) return 1;
    return 0;
}
int prod_maint_train(prod_maint_model_t* mm, const prod_metrics_t* m, uint32_t fail_at, uint32_t fail_mem_pct,
                     uint32_t horizon, uint32_t epochs) {
    int32_t x[PROD_MM_FEAT];
    int i, j, last;
    uint32_t e, pos = 0, neg = 0, nw = 0;
    if (!mm || !m || !fail_at || !horizon || horizon > 16U || !epochs || epochs > 200U) return -1;
    mzero(mm, (int)sizeof(*mm));
    mm->fail_at = fail_at; mm->fail_mem_pct = fail_mem_pct; mm->horizon = horizon;
    /* a window is usable when its horizon is fully observed and the
     * window itself is not already failing */
    last = m->n - 1 - (int)horizon;
    for (i = PROD_MM_WIN - 1; i <= last; i++) {
        if (mm_failure(mm, at(m, i))) continue;
        nw++;
        if (mm_label(m, mm, i)) pos++; else neg++;
    }
    if (nw < 8U || !pos || !neg) return -1;
    for (e = 0; e < epochs; e++) {
        for (i = PROD_MM_WIN - 1; i <= last; i++) {
            int32_t p, err;
            if (mm_failure(mm, at(m, i))) continue;
            (void)prod_maint_features(m, mm, i, x);
            p = mm_prob(mm->w, x);
            err = (mm_label(m, mm, i) ? 4096 : 0) - p;
            for (j = 0; j < PROD_MM_FEAT; j++)
                mm->w[j] = mm_clamp(mm->w[j] + (int32_t)(((int64_t)err * x[j]) >> 13), -262144, 262144);
        }
    }
    for (i = PROD_MM_WIN - 1; i <= last; i++) {
        if (mm_failure(mm, at(m, i))) continue;
        (void)prod_maint_features(m, mm, i, x);
        if ((mm_prob(mm->w, x) >= 2048) == (mm_label(m, mm, i) != 0)) mm->correct++;
    }
    mm->windows = nw; mm->positives = pos; mm->epochs = epochs; mm->trained = 1;
    return (int)nw;
}
int prod_maint_model_risk(const prod_maint_model_t* mm, const prod_metrics_t* m) {
    int32_t x[PROD_MM_FEAT];
    int32_t p;
    uint32_t risk;
    if (!mm || !mm->trained || !m || m->n < PROD_MM_WIN || prod_maint_features(m, mm, m->n - 1, x) != 0) return -1;
    p = mm_prob(mm->w, x);
    if (p < 0) p = 0;
    if (p > 4096) p = 4096;
    risk = ((uint32_t)p * 100U + 2048U) >> 12;
    return risk > 100U ? 100 : (int)risk;
}

int prod_biz_metrics_get(const prod_usage_t* u, uint32_t uptime_ticks, prod_biz_metrics_t* bm) {
    if (!bm) return -1;
    mzero(bm, (int)sizeof(*bm));
    bm->uptime_seconds = uptime_ticks / 100U;
    bm->total_tasks_executed = u ? u->total : 0;
    bm->distinct_commands = u ? (uint32_t)u->n : 0;
    bm->business_kpi_score = uptime_ticks >= 100U && bm->uptime_seconds > 0 ? bm->total_tasks_executed * 60U / bm->uptime_seconds : 0;
    return 0;
}

int prod_biz_kpi_eval(const prod_usage_t* u, uint32_t uptime_ticks, uint32_t total_cmd_time_ms, prod_biz_metrics_t* bm) {
    if (!bm) return -1;
    prod_biz_metrics_get(u, uptime_ticks, bm);
    if (bm->total_tasks_executed > 0) {
        bm->avg_exec_time_ms = total_cmd_time_ms / bm->total_tasks_executed;
    } else {
        bm->avg_exec_time_ms = 0;
    }
    uint32_t eff = 100U;
    if (bm->avg_exec_time_ms > 1000U) {
        eff = (eff > 30U) ? eff - 30U : 0U;
    } else if (bm->avg_exec_time_ms > 200U) {
        eff = (eff > 15U) ? eff - 15U : 0U;
    }
    if (bm->business_kpi_score < 5U) {
        eff = (eff > 10U) ? eff - 10U : 0U;
    }
    bm->global_efficiency_score = eff;
    return 0;
}

static void s_cat(char* d, const char* s, int cap) { int n = s_len(d), i = 0; while (s && s[i] && n < cap - 1) d[n++] = s[i++]; d[n] = 0; }
static void cat_u(char* d, uint32_t v, int cap) {
    char t[11]; char r[12]; int n = 0, i = 0;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v);
    while (n) r[i++] = t[--n];
    r[i] = 0; s_cat(d, r, cap);
}

/* --------------------------------------------- US-110 / Lot 24 Autoscaling Workload */
int prod_autoscale_workload_eval(prod_scaler_t* s, prod_autoscale_workload_t* wl, uint32_t now_tick, char* out, int cap) {
    int old_workers, new_workers;
    uint32_t effective_load;
    if (!s || !wl || !out) return -1;
    old_workers = s->cur;
    effective_load = wl->queue_depth > 0 ? wl->queue_depth : (wl->cpu_load_pct / 10U);
    if (effective_load == 0 && wl->cpu_load_pct > 80U) effective_load = 10U;
    if (wl->queue_depth > 50U || wl->cpu_load_pct > 90U) {
        effective_load *= 2U;
    }
    new_workers = prod_scaler_step(s, effective_load, now_tick);
    if (new_workers != old_workers) wl->scale_events++;
    wl->active_workers = (uint32_t)new_workers;
    out[0] = 0;
    s_cat(out, "autoscale workload workers ", cap); cat_u(out, (uint32_t)new_workers, cap);
    s_cat(out, " queue ", cap); cat_u(out, wl->queue_depth, cap);
    s_cat(out, " cpu_load_pct ", cap); cat_u(out, wl->cpu_load_pct, cap);
    s_cat(out, " scale_events ", cap); cat_u(out, wl->scale_events, cap);
    s_cat(out, "\n", cap);
    return 0;
}

int prod_autofix_stalled_workers(prod_autoscale_workload_t* wl, char* out, int cap) {
    if (!wl || !out || cap <= 0) return -1;
    int fixes = 0;
    out[0] = 0;
    if (wl->queue_depth > 0 && wl->active_workers == 0) {
        wl->active_workers = 1;
        wl->scale_events++;
        fixes++;
    }
    if (wl->cpu_load_pct > 95U && wl->queue_depth > 50U) {
        wl->active_workers += 2;
        wl->scale_events++;
        fixes++;
    }
    s_cat(out, "autofix active_workers ", cap); cat_u(out, wl->active_workers, cap);
    s_cat(out, " fixes ", cap); cat_u(out, (uint32_t)fixes, cap);
    s_cat(out, "\n", cap);
    return fixes;
}

/* --------------------------------------------- US-112 Security Scanner */
static uint32_t le32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint32_t le16(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8); }

int prod_elf_audit(const uint8_t* b, int len, prod_elf_audit_t* r) {
    uint32_t phoff, phent, phnum, i;
    int k;
    for (k = 0; k < (int)sizeof(*r); k++) ((uint8_t*)r)[k] = 0;
    if (!b || len < 52 || b[0] != 0x7F || b[1] != 'E' || b[2] != 'L' || b[3] != 'F' || b[4] != 1 || b[5] != 1)
        return -1;
    if (le16(b + 18) != 3U) return -1;                    /* EM_386 */
    r->is_elf = 1;
    r->entry = le32(b + 24);
    phoff = le32(b + 28); phent = le16(b + 42); phnum = le16(b + 44);
    if (phent < 32U || phnum > 64U || phoff > (uint32_t)len || phoff + phent * phnum > (uint32_t)len) return -1;
    for (i = 0; i < phnum; i++) {
        const uint8_t* ph = b + phoff + i * phent;
        uint32_t type = le32(ph), vaddr = le32(ph + 8), memsz = le32(ph + 20), flags = le32(ph + 24);
        if (type == 1U) {
            r->load_segments++;
            if (flags & 1U) {
                r->exec_segments++;
                if (r->entry >= vaddr && r->entry < vaddr + memsz) r->entry_ok = 1;
            }
            if ((flags & 3U) == 3U) r->wx_segments++;
        } else if (type == 0x6474E551U && (flags & 1U)) {
            r->stack_exec = 1;
        }
    }
    return 0;
}

uint32_t prod_fnv1a(const uint8_t* b, int len) {
    uint32_t h = 2166136261U;
    int i;
    for (i = 0; i < len; i++) { h ^= b[i]; h *= 16777619U; }
    return h;
}

static void cat_hex8(char* d, uint32_t v, int cap) {
    char t[9];
    int i;
    for (i = 7; i >= 0; i--) { t[i] = "0123456789abcdef"[v & 15U]; v >>= 4; }
    t[8] = 0;
    s_cat(d, t, cap);
}

int prod_sec_baseline_line(char* out, int cap, const char* name, uint32_t size, uint32_t hash) {
    s_cat(out, name, cap); s_cat(out, " ", cap); cat_u(out, size, cap); s_cat(out, " ", cap);
    cat_hex8(out, hash, cap); s_cat(out, "\n", cap);
    return s_len(out) < cap - 1 ? 0 : -1;
}

static const char* bl_field(const char* p, char* w, int cap) {
    int n = 0;
    while (*p == ' ') p++;
    while (*p && *p != ' ' && *p != '\n') { if (n < cap - 1) w[n++] = *p; p++; }
    w[n] = 0;
    return p;
}

int prod_sec_baseline_find(const char* text, const char* name, uint32_t* size, uint32_t* hash) {
    const char* p = text;
    while (p && *p) {
        char n[64], sz[16], hx[16];
        const char* q = bl_field(p, n, 64);
        q = bl_field(q, sz, 16);
        q = bl_field(q, hx, 16);
        if (s_eq(n, name)) {
            uint32_t v = 0, h = 0;
            int i;
            for (i = 0; sz[i] >= '0' && sz[i] <= '9'; i++) v = v * 10U + (uint32_t)(sz[i] - '0');
            for (i = 0; hx[i]; i++) {
                char c = hx[i];
                h = (h << 4) | (uint32_t)(c >= 'a' ? c - 'a' + 10 : c - '0');
            }
            *size = v; *hash = h;
            return 1;
        }
        while (*q && *q != '\n') q++;
        p = *q ? q + 1 : q;
    }
    return 0;
}

int prod_sec_baseline_count(const char* text) {
    int n = 0;
    for (; text && *text; text++) if (*text == '\n') n++;
    return n;
}

void prod_sec_scan_init(prod_sec_scan_t* s) {
    int k;
    for (k = 0; k < (int)sizeof(*s); k++) ((uint8_t*)s)[k] = 0;
}

void prod_sec_scan_file(prod_sec_scan_t* s, int audited, const prod_elf_audit_t* a, int in_baseline, int same) {
    s->files++;
    if (audited && a && a->is_elf) {
        s->elf++;
        if (a->wx_segments) s->wx++;
        if (a->stack_exec) s->stack_exec++;
        if (!a->entry_ok) s->bad_entry++;
    } else {
        s->not_elf++;
    }
    if (s->baseline) {
        if (!in_baseline) s->added++;
        else if (!same) s->changed++;
    }
}

void prod_sec_scan_finish(prod_sec_scan_t* s, int baseline_entries) {
    uint32_t seen, r;
    if (s->baseline) {
        seen = s->files - s->added;
        s->missing = (uint32_t)baseline_entries > seen ? (uint32_t)baseline_entries - seen : 0U;
    }
    r = 30U * s->wx + 20U * s->stack_exec + 20U * s->bad_entry + 25U * s->changed + 10U * s->added + 10U * s->missing;
    s->risk_score = r > 100U ? 100U : r;
}

int prod_sec_scan_report(const prod_sec_scan_t* s, char* out, int cap) {
    out[0] = 0;
    s_cat(out, "sec-scan files ", cap); cat_u(out, s->files, cap);
    s_cat(out, " elf ", cap); cat_u(out, s->elf, cap);
    s_cat(out, " wx_segments ", cap); cat_u(out, s->wx, cap);
    s_cat(out, " exec_stack ", cap); cat_u(out, s->stack_exec, cap);
    s_cat(out, " bad_entry ", cap); cat_u(out, s->bad_entry, cap);
    if (s->baseline) {
        s_cat(out, " changed ", cap); cat_u(out, s->changed, cap);
        s_cat(out, " added ", cap); cat_u(out, s->added, cap);
        s_cat(out, " missing ", cap); cat_u(out, s->missing, cap);
    } else {
        s_cat(out, " baseline none", cap);
    }
    s_cat(out, " risk_score ", cap); cat_u(out, s->risk_score, cap);
    s_cat(out, s->risk_score ? " status findings" : " status clean", cap);
    s_cat(out, " (no CVE database)\n", cap);
    return 0;
}

/* ---------------------------------- US-114 Smart Alert Threshold Learning */
/* ------------------------------------------- US-112 signature database */
/* Built-in database (Lot 9): the four test-rogue programs of the image,
 * identified by a byte pattern of their own code, and a sample known-bad
 * hash. Patterns are hex so this text never matches the shell itself. */
const char prod_secdb_builtin[] =
    "# MOHHDY local signature database v1 (MSA = MOHHDY security advisory)\n"
    "MSA-2026-0001 high hex 646576726f67756520756e7365616c\n"      /* sealed-key unseal attempt */
    "MSA-2026-0002 high hex 706369726f67756520636c61696d\n"        /* PCI port claim by a non-driver */
    "MSA-2026-0003 medium hex 617461726f677565207265676973746572\n" /* ATA service name squatting */
    "MSA-2026-0004 medium hex 6169726f677565207265676973746572\n"  /* AI engine service squatting */
    "MSA-2026-0005 critical fnv deadbeef 4096\n"                    /* sample known-bad file */
    "MSA-2026-0006 high hex 7379735f7261775f696f5f636c61696d\n";    /* unauthorized raw IO claim */
const char* prod_sev_name(int sev) {
    return sev == PROD_SEV_LOW ? "low" : sev == PROD_SEV_MEDIUM ? "medium" : sev == PROD_SEV_HIGH ? "high"
         : sev == PROD_SEV_CRITICAL ? "critical" : "?";
}
uint32_t prod_sev_weight(int sev) {
    return sev == PROD_SEV_LOW ? 5U : sev == PROD_SEV_MEDIUM ? 15U : sev == PROD_SEV_HIGH ? 30U
         : sev == PROD_SEV_CRITICAL ? 60U : 0U;
}
static int db_hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
/* next blank-separated word; NULL when it does not fit in `cap` */
static const char* db_word(const char* p, const char* end, char* w, int cap) {
    int n = 0;
    if (!p) return 0;
    while (p < end && *p == ' ') p++;
    while (p < end && *p != ' ') { if (n >= cap - 1) return 0; w[n++] = *p++; }
    w[n] = 0;
    return p;
}
static int db_line(const char* p, const char* end, prod_secdb_rec_t* r) {
    char id[24], sev[12], kind[8], a[72], b[16];
    int i, n;
    uint32_t v;
    mzero(r, (int)sizeof(*r));
    p = db_word(p, end, id, (int)sizeof(id));
    p = db_word(p, end, sev, (int)sizeof(sev));
    p = db_word(p, end, kind, (int)sizeof(kind));
    p = db_word(p, end, a, (int)sizeof(a));
    p = db_word(p, end, b, (int)sizeof(b));
    if (!p) return -1;
    while (p < end && *p == ' ') p++;
    if (p != end || !id[0] || s_len(id) >= (int)sizeof(r->id)) return -1;
    if (s_eq(sev, "low")) r->severity = PROD_SEV_LOW;
    else if (s_eq(sev, "medium")) r->severity = PROD_SEV_MEDIUM;
    else if (s_eq(sev, "high")) r->severity = PROD_SEV_HIGH;
    else if (s_eq(sev, "critical")) r->severity = PROD_SEV_CRITICAL;
    else return -1;
    s_copy(r->id, id, (int)sizeof(r->id));
    if (s_eq(kind, "hex")) {
        n = s_len(a);
        if (b[0] || n < 8 || (n & 1) || n / 2 > PROD_SECDB_PAT) return -1;   /* 4 bytes at least */
        for (i = 0; i < n; i += 2) {
            int hi = db_hex(a[i]), lo = db_hex(a[i + 1]);
            if (hi < 0 || lo < 0) return -1;
            r->pat[i / 2] = (uint8_t)(hi * 16 + lo);
        }
        r->plen = n / 2;
        r->kind = PROD_SIG_HEX;
        return 0;
    }
    if (s_eq(kind, "fnv")) {
        if (s_len(a) != 8 || !b[0]) return -1;
        for (i = 0, v = 0; i < 8; i++) { int d = db_hex(a[i]); if (d < 0) return -1; v = (v << 4) | (uint32_t)d; }
        r->hash = v;
        for (i = 0, v = 0; b[i]; i++) { if (b[i] < '0' || b[i] > '9' || v > 100000000U) return -1; v = v * 10U + (uint32_t)(b[i] - '0'); }
        r->size = v;
        r->kind = PROD_SIG_FNV;
        return 0;
    }
    return -1;
}
int prod_secdb_parse(const char* text, prod_secdb_t* db) {
    const char* p = text;
    if (!text || !db) return -1;
    mzero(db, (int)sizeof(*db));
    while (*p) {
        const char* e = p;
        const char* q;
        while (*e && *e != '\n') e++;
        q = p;
        while (q < e && (*q == ' ' || *q == '\r')) q++;
        if (q < e && *q != '#') {
            const char* t = e;
            while (t > q && (t[-1] == '\r' || t[-1] == ' ')) t--;
            if (db->n >= PROD_SECDB_MAX) return -1;
            if (db_line(q, t, &db->r[db->n]) == 0) db->n++;
            else db->bad_lines++;
        }
        p = *e ? e + 1 : e;
    }
    return db->n;
}
int prod_secdb_match(const prod_secdb_t* db, const uint8_t* b, int len, uint32_t size, uint32_t hash,
                     int* hits, int max) {
    int i, j, k, n = 0;
    if (!db || (!b && len)) return 0;
    for (i = 0; i < db->n; i++) {
        const prod_secdb_rec_t* r = &db->r[i];
        int hit = 0;
        if (r->kind == PROD_SIG_FNV) hit = r->hash == hash && r->size == size;
        else if (r->kind == PROD_SIG_HEX) {
            for (j = 0; j + r->plen <= len && !hit; j++) {
                if (b[j] != r->pat[0]) continue;
                for (k = 1; k < r->plen && b[j + k] == r->pat[k]; k++) {}
                if (k == r->plen) hit = 1;
            }
        }
        if (hit) { if (hits && n < max) hits[n] = i; n++; }
    }
    return n;
}

int prod_alert_learn_update(prod_alert_learn_t* al, const prod_metrics_t* m, int metric_id) {
    prod_stat_t st;
    uint32_t var_sum = 0;
    int i;
    if (!al || !m || prod_metric_stat(m, metric_id, &st) != 0) return -1;
    al->sample_count = (uint32_t)st.n;
    al->mean = st.avg;
    for (i = 0; i < st.n; i++) {
        uint32_t v = at(m, i)->v[metric_id];
        uint32_t diff = v > st.avg ? (v - st.avg) : (st.avg - v);
        var_sum += diff * diff;
    }
    al->std_dev = st.n > 0 ? (var_sum / (uint32_t)st.n) / (st.avg > 0 ? st.avg : 1U) : 0U;
    return 0;
}

int prod_alert_learn_threshold(prod_alert_learn_t* al, uint32_t margin_pct) {
    if (!al) return -1;
    al->learned_threshold = al->mean + (al->mean * margin_pct / 100U) + (al->std_dev * 2U);
    return 0;
}

/* ------------------------ US-106 / US-107 Performance Monitor Consolidation */
int prod_perf_monitor_eval(const prod_metrics_t* m, const prod_alerts_t* a, prod_perf_monitor_t* pm, char* out, int cap) {
    prod_stat_t st_busy, st_mem;
    uint32_t active = 0;
    int i;
    if (!pm || !out || cap <= 0) return -1;
    mzero(pm, (int)sizeof(*pm));
    if (a) {
        for (i = 0; i < PROD_RULES; i++) {
            if (a->r[i].used && a->r[i].firing) active++;
        }
    }
    pm->active_alerts = active;
    if (m && m->n > 0) {
        pm->sample_count = (uint32_t)m->n;
        if (prod_metric_stat(m, PM_BUSY, &st_busy) == 0) pm->avg_busy_pct = st_busy.avg;
        if (prod_metric_stat(m, PM_MEM_FREE, &st_mem) == 0) pm->free_mem_kib = st_mem.last;
    }
    pm->overall_health_score = (active * 20U < 100U) ? (100U - active * 20U) : 0U;
    pm->system_status_ok = (pm->overall_health_score >= 60U) ? 1 : 0;

    if (active > 2 || pm->avg_busy_pct > 85U) {
        pm->auto_restarts++;
    }

    out[0] = 0;
    s_cat(out, "perf-monitor health ", cap); cat_u(out, pm->overall_health_score, cap);
    s_cat(out, " active_alerts ", cap); cat_u(out, pm->active_alerts, cap);
    s_cat(out, " samples ", cap); cat_u(out, pm->sample_count, cap);
    s_cat(out, " avg_busy_pct ", cap); cat_u(out, pm->avg_busy_pct, cap);
    s_cat(out, " free_mem_kib ", cap); cat_u(out, pm->free_mem_kib, cap);
    s_cat(out, " auto_restarts ", cap); cat_u(out, pm->auto_restarts, cap);
    s_cat(out, pm->system_status_ok ? " status ok_healthy\n" : " status warning_degraded\n", cap);
    return s_len(out);
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

/* ------------------------------------------------ US-116 support bundle */
int prod_support_bundle_text(const prod_metrics_t* m, const prod_alerts_t* a, const prod_usage_t* u,
                             const prod_feedback_t* f, uint32_t tick, char* out, int cap) {
    int i;
    if (!out || cap < 64) return -1;
    out[0] = 0;
    s_cat(out, "mohhdy support bundle v1 tick ", cap); cat_u(out, tick, cap); s_cat(out, "\n", cap);
    s_cat(out, "metrics samples ", cap); cat_u(out, m ? (uint32_t)m->n : 0, cap);
    s_cat(out, " total ", cap); cat_u(out, m ? m->total : 0, cap); s_cat(out, "\n", cap);
    for (i = 0; m && i < PM_COUNT; i++) {
        prod_stat_t st;
        if (prod_metric_stat(m, i, &st) != 0 || st.n == 0) continue;
        s_cat(out, "metric ", cap); s_cat(out, prod_metric_name(i), cap);
        s_cat(out, " min ", cap); cat_u(out, st.min, cap); s_cat(out, " max ", cap); cat_u(out, st.max, cap);
        s_cat(out, " avg ", cap); cat_u(out, st.avg, cap); s_cat(out, " last ", cap); cat_u(out, st.last, cap); s_cat(out, "\n", cap);
    }
    for (i = 0; a && i < PROD_RULES; i++) if (a->r[i].used) {
        s_cat(out, "alert ", cap); s_cat(out, a->r[i].name, cap); s_cat(out, " ", cap); s_cat(out, prod_metric_name(a->r[i].metric), cap);
        s_cat(out, a->r[i].above ? " > " : " < ", cap); cat_u(out, a->r[i].threshold, cap);
        s_cat(out, a->r[i].firing ? " firing\n" : " ok\n", cap);
    }
    if (a) {
        s_cat(out, "events fired ", cap); cat_u(out, a->fired_total, cap); s_cat(out, " resolved ", cap); cat_u(out, a->resolved_total, cap);
        s_cat(out, " suppressed ", cap); cat_u(out, a->suppressed, cap); s_cat(out, "\n", cap);
    }
    for (i = 0; u && i < u->n && i < 8; i++) {
        s_cat(out, "usage ", cap); s_cat(out, u->u[i].name, cap); s_cat(out, " ", cap); cat_u(out, u->u[i].count, cap); s_cat(out, "\n", cap);
    }
    s_cat(out, "feedback total ", cap); cat_u(out, f ? f->total : 0, cap); s_cat(out, "\n", cap);
    {
        prod_perf_monitor_t pm;
        char pm_buf[256];
        if (prod_perf_monitor_eval(m, a, &pm, pm_buf, (int)sizeof(pm_buf)) >= 0) {
            s_cat(out, "health_score ", cap); cat_u(out, pm.overall_health_score, cap);
            s_cat(out, " auto_restarts ", cap); cat_u(out, pm.auto_restarts, cap);
            s_cat(out, " status ", cap); s_cat(out, pm.system_status_ok ? "ok_healthy\n" : "warning_degraded\n", cap);
        }
    }
    {
        extern int sys_readfile(const char* path, char* buf, int max);
        static char rmbuf[2048];
        prod_roadmap_t rm;
        int rlen = sys_readfile("/roadmap.txt", rmbuf, (int)sizeof(rmbuf) - 1);
        if (rlen > 0) {
            rmbuf[rlen] = 0;
            if (prod_roadmap_parse(rmbuf, rlen, &rm) == 0) {
                s_cat(out, "roadmap fnv ", cap);
                cat_hex8(out, rm.checksum_fnv, cap);
                s_cat(out, "\n", cap);
            }
        }
    }
    {
        uint32_t bfnv = prod_fnv1a((const uint8_t*)out, s_len(out));
        s_cat(out, "checksum verified=true fnv ", cap);
        cat_hex8(out, bfnv, cap);
        s_cat(out, "\n", cap);
    }
    s_cat(out, "end\n", cap);
    return s_len(out) < cap - 1 ? s_len(out) : -1;
}

/* ------------------------------------------------ US-120 roadmap (Lot 8) */
static int rm_word(const char* t, int len, int* pos, char* w, int cap) {
    int n = 0;
    while (*pos < len && t[*pos] == ' ') (*pos)++;
    while (*pos < len && t[*pos] != ' ' && t[*pos] != '\n') { if (n < cap - 1) w[n++] = t[*pos]; (*pos)++; }
    w[n] = 0;
    return n;
}
int prod_roadmap_parse(const char* t, int len, prod_roadmap_t* rm) {
    int pos = 0;
    if (!t || !rm || len < 0) return -1;
    mzero(rm, (int)sizeof(*rm));
    rm->checksum_fnv = prod_fnv1a((const uint8_t*)t, len);
    while (pos < len) {
        char us[12], ph[4], st[12];
        int start = pos, phase;
        if (t[pos] == '#' || t[pos] == '\n') {
            while (pos < len && t[pos] != '\n') pos++;
            pos++;
            continue;
        }
        rm_word(t, len, &pos, us, 12); rm_word(t, len, &pos, ph, 4); rm_word(t, len, &pos, st, 12);
        while (pos < len && t[pos] != '\n') pos++;
        pos++;
        phase = (ph[0] >= '1' && ph[0] <= '8' && !ph[1]) ? ph[0] - '0' : -1;
        if (us[0] != 'U' || us[1] != 'S' || us[2] != '-' || phase < 0 || start == pos) { rm->bad++; continue; }
        if (s_eq(st, "done")) rm->done[phase]++;
        else if (s_eq(st, "partial")) rm->partial[phase]++;
        else if (s_eq(st, "simule")) rm->simule[phase]++;
        else if (s_eq(st, "open")) rm->open[phase]++;
        else { rm->bad++; continue; }
        rm->entries++;
        if (!s_eq(st, "done")) {
            s_cat(rm->next, rm->next[0] ? " " : "", (int)sizeof(rm->next));
            s_cat(rm->next, us, (int)sizeof(rm->next));
        }
    }
    return (int)rm->entries;
}

int prod_roadmap_eval(const prod_roadmap_t* rm, uint32_t* total_done, uint32_t* total_entries, uint32_t* pct) {
    uint32_t d = 0U, e = 0U;
    int p;
    if (!rm || !total_done || !total_entries || !pct) return -1;
    for (p = 0; p < PROD_RM_PHASES; p++) {
        d += rm->done[p];
        e += rm->done[p] + rm->partial[p] + rm->simule[p] + rm->open[p];
    }
    *total_done = d;
    *total_entries = e > 0U ? e : rm->entries;
    *pct = *total_entries > 0U ? (d * 100U) / *total_entries : 0U;
    return 0;
}

/* US-122: AI SLA Monitor & Predictive Fault Engine */
void prod_ai_sla_init(prod_ai_sla_t *sla, uint32_t target_latency_ms) {
    if (!sla) return;
    sla->total_requests = 0;
    sla->successful_requests = 0;
    sla->avg_token_latency_ms = 0;
    sla->max_token_latency_ms = 0;
    sla->sla_breaches = 0;
    sla->target_latency_ms = target_latency_ms ? target_latency_ms : 100;
}

void prod_ai_sla_record(prod_ai_sla_t *sla, uint32_t token_latency_ms, int success) {
    if (!sla) return;
    sla->total_requests++;
    if (success) sla->successful_requests++;
    if (token_latency_ms > sla->max_token_latency_ms) {
        sla->max_token_latency_ms = token_latency_ms;
    }
    if (token_latency_ms > sla->target_latency_ms) {
        sla->sla_breaches++;
    }
    sla->avg_token_latency_ms = (sla->avg_token_latency_ms * (sla->total_requests - 1) + token_latency_ms) / sla->total_requests;
}

int prod_ai_sla_eval(const prod_ai_sla_t *sla, uint32_t *success_rate_pct, uint32_t *sla_compliance_pct) {
    if (!sla || sla->total_requests == 0) {
        if (success_rate_pct) *success_rate_pct = 100;
        if (sla_compliance_pct) *sla_compliance_pct = 100;
        return 0;
    }
    uint32_t s_rate = (sla->successful_requests * 100U) / sla->total_requests;
    uint32_t compliance = ((sla->total_requests - sla->sla_breaches) * 100U) / sla->total_requests;
    if (success_rate_pct) *success_rate_pct = s_rate;
    if (sla_compliance_pct) *sla_compliance_pct = compliance;
    return 0;
}

void prod_fault_predict_init(prod_fault_predict_t *fp) {
    if (!fp) return;
    fp->io_errors = 0;
    fp->memory_corruptions = 0;
    fp->packet_losses = 0;
    fp->risk_score = 0;
}

void prod_fault_predict_record(prod_fault_predict_t *fp, uint32_t io_err, uint32_t mem_err, uint32_t pkt_loss) {
    if (!fp) return;
    fp->io_errors += io_err;
    fp->memory_corruptions += mem_err;
    fp->packet_losses += pkt_loss;

    uint32_t score = fp->io_errors * 15U + fp->memory_corruptions * 40U + fp->packet_losses * 5U;
    if (score > 100U) score = 100U;
    fp->risk_score = score;
}

int prod_fault_predict_eval(const prod_fault_predict_t *fp, char *out_verdict, int cap) {
    if (!fp || !out_verdict || cap < 16) return -1;
    out_verdict[0] = 0;
    if (fp->risk_score >= 75U) {
        s_copy(out_verdict, "CRITICAL_RISK", cap);
    } else if (fp->risk_score >= 40U) {
        s_copy(out_verdict, "ELEVATED_RISK", cap);
    } else if (fp->risk_score >= 15U) {
        s_copy(out_verdict, "MODERATE_RISK", cap);
    } else {
        s_copy(out_verdict, "HEALTHY", cap);
    }
    return (int)fp->risk_score;
}
