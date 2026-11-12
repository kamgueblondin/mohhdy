/* wf_autofill.c - Lot 86: see wf_autofill.h. */
#include "wf_autofill.h"
#include "wf_regdom.h"

static int alen(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static char alow(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int aeq(const char *a, const char *b) {
    int i = 0;
    while (a[i] || b[i]) { if (alow(a[i]) != alow(b[i])) return 0; if (!a[i]) break; i++; }
    return 1;
}
static void acpy(char *d, int cap, const char *s, int lower) {
    int i = 0;
    if (cap <= 0) return;
    while (s && s[i] && i < cap - 1) { d[i] = lower ? alow(s[i]) : s[i]; i++; }
    d[i] = 0;
}
static void azero(void *p, uint32_t n) { uint8_t *b = (uint8_t *)p; while (n--) *b++ = 0; }

void wf_autofill_init(wf_autofill_t *a) {
    if (!a) return;
    azero(a, (uint32_t)sizeof(*a));
}

int wf_autofill_secret(const char *type) {
    if (!type || !type[0]) return 0;
    return aeq(type, "password") || aeq(type, "hidden");
}

static int host_ok(const char *h) {
    int i, dots = 0;
    if (!h || !h[0] || alen(h) >= WF_AF_HOST) return 0;
    for (i = 0; h[i]; i++) {
        char c = alow(h[i]);
        if (c == '.') dots++;
        else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return 0;
    }
    return dots >= 1;
}

static int seq(const char *a, const char *b) {
    int i = 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}

/* Lot 87: the new value goes first; the one it replaces moves into the
 * older list (newest first, the oldest falls off). A value already in the
 * list moves to the front instead of being kept twice. */
/* Lot 90: each value keeps its date as it moves; the value put first is
 * dated day (0, the clock unknown: a value already there keeps its own
 * date, a new one is undated) */
static void push_value(wf_af_ent_t *e, const char *value, uint32_t day) {
    int k, at = -1;
    uint32_t keep = 0;
    if (seq(e->value, value)) { if (day) e->day = day; return; }
    for (k = 0; k < e->nold; k++) if (seq(e->old[k], value)) { at = k; break; }
    if (at < 0) {
        at = e->nold < WF_AF_HIST - 1 ? e->nold++ : WF_AF_HIST - 2;
    } else keep = e->old_day[at];
    /* close the gap at 'at', then shift the rest down by one */
    for (k = at; k > 0; k--) {
        acpy(e->old[k], WF_AF_VALUE, e->old[k - 1], 0);
        e->old_day[k] = e->old_day[k - 1];
    }
    if (WF_AF_HIST > 1) {
        acpy(e->old[0], WF_AF_VALUE, e->value, 0);
        e->old_day[0] = e->day;
    }
    acpy(e->value, WF_AF_VALUE, value, 0);
    e->day = day ? day : keep;
}

int wf_autofill_put(wf_autofill_t *a, const char *host, const char *name, const char *value, const char *type) {
    int i, free_i = -1;
    if (!a || !host_ok(host) || !name || !name[0] || alen(name) >= WF_AF_NAME) { if (a) a->refused++; return -1; }
    if (!value || wf_autofill_secret(type)) { a->refused++; return -1; }
    if (alen(value) >= WF_AF_VALUE) { a->refused++; return -1; }
    for (i = 0; value[i]; i++) if ((unsigned char)value[i] < 0x20u) { a->refused++; return -1; }
    for (i = 0; i < WF_AF_MAX; i++) {
        if (!a->e[i].used) { if (free_i < 0) free_i = i; continue; }
        if (aeq(a->e[i].host, host) && aeq(a->e[i].name, name)) {
            push_value(&a->e[i], value, a->today);
            a->stored++;
            return 0;
        }
    }
    if (free_i < 0) { a->refused++; return -1; }
    azero(&a->e[free_i], (uint32_t)sizeof(a->e[free_i]));
    a->e[free_i].used = 1;
    acpy(a->e[free_i].host, WF_AF_HOST, host, 1);
    acpy(a->e[free_i].name, WF_AF_NAME, name, 0);
    acpy(a->e[free_i].value, WF_AF_VALUE, value, 0);
    a->e[free_i].day = a->today;
    a->n++;
    a->stored++;
    return 0;
}

int wf_autofill_get(wf_autofill_t *a, const char *host, const char *name, char *out, int cap) {
    int i;
    if (!a || !host || !name || !out || cap < 2) return -1;
    for (i = 0; i < WF_AF_MAX; i++) {
        if (!a->e[i].used) continue;
        if (aeq(a->e[i].host, host) && aeq(a->e[i].name, name)) {
            acpy(out, cap, a->e[i].value, 0);
            a->hits++;
            return 0;
        }
    }
    return -1;
}

static const wf_af_ent_t *find_ent(const wf_autofill_t *a, const char *host, const char *name) {
    int i;
    if (!a || !host || !name) return 0;
    for (i = 0; i < WF_AF_MAX; i++)
        if (a->e[i].used && aeq(a->e[i].host, host) && aeq(a->e[i].name, name)) return &a->e[i];
    return 0;
}

int wf_autofill_get_at(wf_autofill_t *a, const char *host, const char *name, int idx, char *out, int cap) {
    const wf_af_ent_t *e;
    if (!out || cap < 2) return -1;
    out[0] = 0;
    e = find_ent(a, host, name);
    if (!e || idx < 0 || idx > e->nold) return -1;
    acpy(out, cap, idx == 0 ? e->value : e->old[idx - 1], 0);
    a->hits++;
    return 0;
}

int wf_autofill_count(const wf_autofill_t *a, const char *host, const char *name) {
    const wf_af_ent_t *e = find_ent(a, host, name);
    return e ? 1 + e->nold : 0;
}

int wf_autofill_forget(wf_autofill_t *a, const char *host) {
    int i, n = 0;
    if (!a) return 0;
    for (i = 0; i < WF_AF_MAX; i++) {
        if (!a->e[i].used) continue;
        if (!host || !host[0] || aeq(a->e[i].host, host)) {
            a->e[i].used = 0;
            a->n--;
            n++;
        }
    }
    return n;
}

static wf_af_ent_t *find_ent_rw(wf_autofill_t *a, const char *host, const char *name) {
    const wf_af_ent_t *e = find_ent(a, host, name);
    return e ? &a->e[e - a->e] : 0;
}

int wf_autofill_forget_key(wf_autofill_t *a, const char *host, const char *name) {
    wf_af_ent_t *e = find_ent_rw(a, host, name);
    if (!e) return 0;
    azero(e, (uint32_t)sizeof(*e));
    a->n--;
    return 1;
}

int wf_autofill_drop(wf_autofill_t *a, const char *host, const char *name, const char *value) {
    wf_af_ent_t *e = find_ent_rw(a, host, name);
    int k, at;
    if (!e || !value) return -1;
    if (seq(e->value, value)) at = 0;
    else {
        for (at = -1, k = 0; k < e->nold; k++) if (seq(e->old[k], value)) { at = k + 1; break; }
        if (at < 0) return -1;
    }
    if (e->nold == 0) {
        /* the last value: the key goes */
        azero(e, (uint32_t)sizeof(*e));
        a->n--;
        return 0;
    }
    /* value list: [value, old0, old1...]; remove index 'at' and close up */
    if (at == 0) { acpy(e->value, WF_AF_VALUE, e->old[0], 0); e->day = e->old_day[0]; }
    for (k = (at == 0 ? 0 : at - 1); k < e->nold - 1; k++) {
        acpy(e->old[k], WF_AF_VALUE, e->old[k + 1], 0);
        e->old_day[k] = e->old_day[k + 1];
    }
    e->nold--;
    azero(e->old[e->nold], (uint32_t)WF_AF_VALUE);
    e->old_day[e->nold] = 0;
    return 1 + e->nold;
}

static int app(char *o, int cap, int p, const char *s) {
    while (s && *s) { if (p >= cap - 1) return -1; o[p++] = *s++; }
    return p;
}

static int app_u(char *o, int cap, int p, uint32_t v) {
    char b[12];
    int n = 0;
    if (p < 0) return -1;
    do { b[n++] = (char)('0' + v % 10U); v /= 10U; } while (v && n < 11);
    while (n) { if (p >= cap - 1) return -1; o[p++] = b[--n]; }
    return p;
}

int wf_autofill_save(const wf_autofill_t *a, char *out, int cap) {
    int p = 0, i;
    if (!a || !out || cap < 16) return -1;
    /* Lot 90: version 3, the settings on the header line */
    p = app(out, cap, p, "MOHHDY-AUTOFILL 3");
    if (p >= 0 && a->expire_days) { p = app(out, cap, p, " expire "); p = app_u(out, cap, p, a->expire_days); }
    if (p >= 0 && a->tz_min) {
        p = app(out, cap, p, a->tz_min < 0 ? " tz -" : " tz ");
        p = app_u(out, cap, p, (uint32_t)(a->tz_min < 0 ? -a->tz_min : a->tz_min));
    }
    /* Lot 91: " dst eu|us|au" when a rule is set */
    if (p >= 0 && a->dst) { p = app(out, cap, p, " dst "); p = p < 0 ? -1 : app(out, cap, p, wf_autofill_dst_name(a->dst)); }
    p = p < 0 ? -1 : app(out, cap, p, "\n");
    if (p < 0) return -1;
    /* Lot 87: the older values first, oldest first, so a load that puts
     * the lines in order rebuilds the same list */
    for (i = 0; i < WF_AF_MAX; i++) if (a->e[i].used) {
        int k;
        for (k = a->e[i].nold; k >= 0; k--) {
            if (p >= cap - 1) return -1;
            p = app(out, cap, p, a->e[i].host);
            if (p < 0 || p >= cap - 1) return -1;
            out[p++] = '\t';
            p = app(out, cap, p, a->e[i].name);
            if (p < 0 || p >= cap - 1) return -1;
            out[p++] = '\t';
            p = app(out, cap, p, k == 0 ? a->e[i].value : a->e[i].old[k - 1]);
            if (p < 0 || p >= cap - 1) return -1;
            {
                /* Lot 89: the day as a fourth field; Lot 90: each value's */
                uint32_t day = k == 0 ? a->e[i].day : a->e[i].old_day[k - 1];
                if (day) {
                    out[p++] = '\t';
                    p = app_u(out, cap, p, day);
                    if (p < 0 || p >= cap - 1) return -1;
                }
            }
            out[p++] = '\n';
        }
    }
    out[p] = 0;
    return p;
}

static int pref(const char *s, const char *p) {
    int i = 0;
    while (p[i]) { if (s[i] != p[i]) return 0; i++; }
    return 1;
}

/* the settings after "MOHHDY-AUTOFILL 3": " expire N" (1..36500),
 * " tz M" (-720..840) and Lot 91 " dst R" (eu, us, au), each at most
 * once, in any order */
static int load_header(wf_autofill_t *t, const char *s) {
    int seen_e = 0, seen_t = 0, seen_d = 0;
    while (*s) {
        int neg = 0, k = 0;
        uint32_t v = 0;
        if (*s != ' ') return -1;
        s++;
        if (pref(s, "dst ") && !seen_d) {
            /* Lot 91: " dst eu|us|au" */
            char w[4];
            int r;
            s += 4;
            while (k < 3 && s[k] >= 'a' && s[k] <= 'z') { w[k] = s[k]; k++; }
            w[k] = 0;
            r = wf_autofill_dst_parse(w);
            if (r <= 0 || (s[k] && s[k] != ' ')) return -1;
            (void)wf_autofill_set_dst(t, r);
            s += k;
            seen_d = 1;
            continue;
        }
        if (pref(s, "expire ") && !seen_e) { s += 7; seen_e = 1; }
        else if (pref(s, "tz ") && !seen_t) { s += 3; seen_t = 2; if (*s == '-') { neg = 1; s++; } }
        else return -1;
        while (s[k] >= '0' && s[k] <= '9') {
            if (k >= 5) return -1;
            v = v * 10U + (uint32_t)(s[k] - '0');
            k++;
        }
        if (!k) return -1;
        s += k;
        if (seen_t == 2) {
            int m = neg ? -(int)v : (int)v;
            if (wf_autofill_set_tz(t, m) != 0 || m == 0) return -1;
            seen_t = 1;
        } else if (wf_autofill_set_expire(t, v) != 0 || v == 0) return -1;
    }
    return 0;
}

int wf_autofill_load(wf_autofill_t *a, const char *text, int len) {
    wf_autofill_t tmp;
    int i = 0, line = 0;
    if (!a || !text || len < 0) return -1;
    wf_autofill_init(&tmp);
    while (i < len) {
        char buf[WF_AF_HOST + WF_AF_NAME + WF_AF_VALUE + 4];
        int k = 0, t1 = -1, t2 = -1, t3 = -1;
        uint32_t day = 0;
        while (i < len && text[i] != '\n' && k < (int)sizeof(buf) - 1) buf[k++] = text[i++];
        if (i < len && text[i] == '\n') i++;
        buf[k] = 0;
        line++;
        if (k == 0) continue;
        if (line == 1) {
            if (buf[0] != 'M') return -1;
            if (!pref(buf, "MOHHDY-AUTOFILL 3")) continue;
            /* Lot 90: " expire N" and " tz M" after the version */
            if (load_header(&tmp, buf + 17) != 0) return -1;
            continue;
        }
        for (k = 0; buf[k]; k++) if (buf[k] == '\t') {
            if (t1 < 0) t1 = k;
            else if (t2 < 0) t2 = k;
            else if (t3 < 0) t3 = k;
            else return -1;
        }
        if (t1 < 0 || t2 < 0) return -1;
        buf[t1] = 0; buf[t2] = 0;
        if (t3 >= 0) {
            /* Lot 89: a fourth field is the day, digits only, 1..84006
             * (2199-12-31, the last day wf_autofill_day gives) */
            buf[t3] = 0;
            if (!buf[t3 + 1]) return -1;
            for (k = t3 + 1; buf[k]; k++) {
                if (buf[k] < '0' || buf[k] > '9' || day > 10000000U) return -1;
                day = day * 10U + (uint32_t)(buf[k] - '0');
            }
            if (day == 0 || day > WF_AF_DAY_MAX) return -1;
        }
        if (wf_autofill_put(&tmp, buf, buf + t1 + 1, buf + t2 + 1, "text") != 0) return -1;
        if (day) {
            wf_af_ent_t *e = find_ent_rw(&tmp, buf, buf + t1 + 1);
            if (e) e->day = day;
        }
    }
    {
        uint8_t *d = (uint8_t *)a;
        const uint8_t *s = (const uint8_t *)&tmp;
        uint32_t n = (uint32_t)sizeof(tmp);
        tmp.today = a->today;   /* Lot 89: the clock is not in the file */
        tmp.touched = a->touched;
        tmp.expired = a->expired;
        tmp.expire_ran = 0;     /* Lot 90: the next tick runs the expiry */
        while (n--) *d++ = *s++;
    }
    return 0;
}

int wf_autofill_text(const wf_autofill_t *a, char *out, int cap) {
    int p = 0, i;
    if (!a || !out || cap < 8) return -1;
    p = app(out, cap, p, "autofill ");
    {
        char b[8]; int n = 0, v = a->n;
        do { b[n++] = (char)('0' + v % 10); v /= 10; } while (v && n < 7);
        while (n) { if (p >= cap - 1) return -1; out[p++] = b[--n]; }
    }
    if (p < cap - 1) out[p++] = '\n';
    for (i = 0; i < WF_AF_MAX; i++) if (a->e[i].used) {
        p = app(out, cap, p, a->e[i].host);
        if (p < 0 || p >= cap - 1) return -1;
        out[p++] = ' ';
        p = app(out, cap, p, a->e[i].name);
        if (p < 0 || p >= cap - 1) return -1;
        if (a->e[i].nold > 0) {
            p = app(out, cap, p, " values ");
            if (p < 0 || p >= cap - 2) return -1;
            out[p++] = (char)('1' + a->e[i].nold);
        }
        out[p++] = '\n';
    }
    out[p] = 0;
    return p;
}

/* ---- Lot 88: sharing inside a site ------------------------------------ */

static const char *ent_value(const wf_af_ent_t *e, int k) { return k == 0 ? e->value : e->old[k - 1]; }

/* The site list of (host, name): the host's key first, then the other
 * keys of the site in slot order. Fills up to WF_AF_SITE_MAX pointers
 * into the store; other[i] is 1 for another host. Returns the count. */
static int site_list(const wf_autofill_t *a, const char *host, const char *name, const char **val, int *other) {
    int n = 0, pass, i, k, j;
    if (!a || !host || !name || !host[0] || !name[0]) return 0;
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < WF_AF_MAX && n < WF_AF_SITE_MAX; i++) {
            const wf_af_ent_t *e = &a->e[i];
            int mine;
            if (!e->used || !aeq(e->name, name)) continue;
            mine = aeq(e->host, host);
            if (pass == 0 ? !mine : (mine || !wf_regdom_same_site(e->host, host))) continue;
            for (k = 0; k <= e->nold && n < WF_AF_SITE_MAX; k++) {
                const char *v = ent_value(e, k);
                int dup = 0;
                for (j = 0; j < n; j++) if (seq(val[j], v)) dup = 1;
                if (dup) continue;
                val[n] = v;
                other[n] = pass;
                n++;
            }
        }
    }
    return n;
}

int wf_autofill_get_site(wf_autofill_t *a, const char *host, const char *name, int idx, char *out, int cap,
                         int *other) {
    const char *val[WF_AF_SITE_MAX];
    int oth[WF_AF_SITE_MAX], n;
    if (other) *other = 0;
    if (!out || cap < 2) return -1;
    out[0] = 0;
    n = site_list(a, host, name, val, oth);
    if (idx < 0 || idx >= n) return -1;
    acpy(out, cap, val[idx], 0);
    if (other) *other = oth[idx];
    a->hits++;
    return 0;
}

int wf_autofill_count_site(const wf_autofill_t *a, const char *host, const char *name) {
    const char *val[WF_AF_SITE_MAX];
    int oth[WF_AF_SITE_MAX];
    return site_list(a, host, name, val, oth);
}

int wf_autofill_drop_site(wf_autofill_t *a, const char *host, const char *name, const char *value) {
    int i, r;
    if (!a || !host || !name || !value) return -1;
    r = wf_autofill_drop(a, host, name, value);
    if (r >= 0) return r;
    for (i = 0; i < WF_AF_MAX; i++) {
        wf_af_ent_t *e = &a->e[i];
        if (!e->used || !aeq(e->name, name) || aeq(e->host, host)) continue;
        if (!wf_regdom_same_site(e->host, host)) continue;
        r = wf_autofill_drop(a, e->host, name, value);
        if (r >= 0) return r;
    }
    return -1;
}

/* ---- Lot 89: dates and expiry -------------------------------- */

static int leap(uint32_t y) { return (y % 4U == 0U && y % 100U != 0U) || y % 400U == 0U; }

uint32_t wf_autofill_day(const char *utc) {
    static const uint8_t mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint32_t y = 0, m = 0, d = 0, days = 0, i;
    int k;
    if (!utc) return 0;
    for (k = 0; k < 8; k++) if (utc[k] < '0' || utc[k] > '9') return 0;
    for (k = 0; k < 4; k++) y = y * 10U + (uint32_t)(utc[k] - '0');
    m = (uint32_t)(utc[4] - '0') * 10U + (uint32_t)(utc[5] - '0');
    d = (uint32_t)(utc[6] - '0') * 10U + (uint32_t)(utc[7] - '0');
    if (y < 1970U || y > 2199U || m < 1U || m > 12U || d < 1U) return 0;
    if (d > (uint32_t)mdays[m - 1U] + (m == 2U && leap(y) ? 1U : 0U)) return 0;
    /* whole years, then whole months, then the day (1970-01-01 is 1) */
    for (i = 1970U; i < y; i++) days += leap(i) ? 366U : 365U;
    for (i = 1U; i < m; i++) days += (uint32_t)mdays[i - 1U] + (i == 2U && leap(y) ? 1U : 0U);
    return days + d;
}

void wf_autofill_set_today(wf_autofill_t *a, uint32_t day) {
    if (a) a->today = day;
}

int wf_autofill_age(const wf_autofill_t *a, const char *host, const char *name) {
    const wf_af_ent_t *e = find_ent(a, host, name);
    if (!e || !e->day || !a->today) return -1;
    return e->day >= a->today ? 0 : (int)(a->today - e->day);
}

int wf_autofill_forget_older(wf_autofill_t *a, uint32_t days, int *undated) {
    int v;
    return wf_autofill_forget_older_values(a, days, undated, &v);
}

/* Lot 90: a value older than the limit goes; the others keep their order */
static int too_old(const wf_autofill_t *a, uint32_t day, uint32_t days) {
    return day && day < a->today && a->today - day > days;
}

int wf_autofill_forget_older_values(wf_autofill_t *a, uint32_t days, int *undated, int *values) {
    int i, n = 0, u = 0, gone = 0;
    if (undated) *undated = 0;
    if (values) *values = 0;
    if (!a || !a->today || days > 36500U) return -1;
    for (i = 0; i < WF_AF_MAX; i++) {
        wf_af_ent_t *e = &a->e[i];
        int k, had0 = 0;
        if (!e->used) continue;
        /* no date: dated now, it expires days days from today */
        if (!e->day) { e->day = a->today; had0 = 1; }
        for (k = 0; k < e->nold; k++) if (!e->old_day[k]) { e->old_day[k] = a->today; had0 = 1; }
        if (had0) u++;
        /* a date in the future (a clock set back) is kept; the oldest
         * values first so the key's own list closes up as it goes */
        for (k = e->nold - 1; k >= 0; k--) {
            if (!too_old(a, e->old_day[k], days)) continue;
            (void)wf_autofill_drop(a, e->host, e->name, e->old[k]);
            gone++;
        }
        if (too_old(a, e->day, days)) {
            gone++;
            if (wf_autofill_drop(a, e->host, e->name, e->value) == 0 && !e->used) n++;
        }
    }
    if (undated) *undated = u;
    if (values) *values = gone;
    return n;
}

static const wf_af_ent_t *find_ent(const wf_autofill_t *a, const char *host, const char *name);

int wf_autofill_value_age(const wf_autofill_t *a, const char *host, const char *name, int idx) {
    const wf_af_ent_t *e = find_ent(a, host, name);
    uint32_t day;
    if (!e || !a->today || idx < 0 || idx > e->nold) return -1;
    day = idx == 0 ? e->day : e->old_day[idx - 1];
    if (!day) return -1;
    return day >= a->today ? 0 : (int)(a->today - day);
}

static int ent_has(const wf_af_ent_t *e, const char *value) {
    int k;
    if (seq(e->value, value)) return 1;
    for (k = 0; k < e->nold; k++) if (seq(e->old[k], value)) return 1;
    return 0;
}

int wf_autofill_touch(wf_autofill_t *a, const char *host, const char *name, const char *value) {
    wf_af_ent_t *e;
    if (!a || !value) return -1;
    e = find_ent_rw(a, host, name);
    if (!e || !ent_has(e, value)) return -1;
    push_value(e, value, a->today);
    a->touched++;
    return 0;
}

int wf_autofill_touch_site(wf_autofill_t *a, const char *host, const char *name, const char *value) {
    int i;
    if (!a || !host || !name || !value) return -1;
    if (wf_autofill_touch(a, host, name, value) == 0) return 0;
    for (i = 0; i < WF_AF_MAX; i++) {
        wf_af_ent_t *e = &a->e[i];
        if (!e->used || !aeq(e->name, name) || aeq(e->host, host)) continue;
        if (!wf_regdom_same_site(e->host, host) || !ent_has(e, value)) continue;
        push_value(e, value, a->today);
        a->touched++;
        return 1;
    }
    return -1;
}

int wf_autofill_set_expire(wf_autofill_t *a, uint32_t days) {
    if (!a || days > 36500U) return -1;
    a->expire_days = days;
    a->expire_ran = 0;
    return 0;
}

int wf_autofill_tick(wf_autofill_t *a, uint32_t day) {
    int v = 0;
    if (!a) return -1;
    a->today = day;
    if (!day) return -1;
    if (!a->expire_days || a->expire_ran == day) return 0;
    if (wf_autofill_forget_older_values(a, a->expire_days, 0, &v) < 0) return 0;
    a->expire_ran = day;
    a->expired += (uint32_t)v;
    return v;
}

int wf_autofill_set_tz(wf_autofill_t *a, int minutes) {
    if (!a || minutes < -720 || minutes > 840) return -1;
    a->tz_min = minutes;
    return 0;
}

uint32_t wf_autofill_day_tz(const char *utc, int tz_min) {
    uint32_t day;
    int k, hh, mm, m;
    if (tz_min < -720 || tz_min > 840) return 0;
    day = wf_autofill_day(utc);
    if (!day || tz_min == 0) return day;
    for (k = 8; k < 12; k++) if (utc[k] < '0' || utc[k] > '9') return 0;
    hh = (utc[8] - '0') * 10 + (utc[9] - '0');
    mm = (utc[10] - '0') * 10 + (utc[11] - '0');
    if (hh > 23 || mm > 59) return 0;
    m = hh * 60 + mm + tz_min;
    if (m < 0) day--;
    else if (m >= 1440) day++;
    if (day == 0 || day > WF_AF_DAY_MAX) return 0;
    return day;
}

/* ---- Lot 91: daylight saving rules --------------------------------- */

static const uint8_t g_mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int mlen(uint32_t y, uint32_t m) { return g_mdays[m - 1U] + (m == 2U && leap(y) ? 1 : 0); }

/* day number (1970-01-01 = 1) of y-m-d; the date is valid */
static int32_t ymd_day(uint32_t y, uint32_t m, uint32_t d) {
    int32_t days = 0;
    uint32_t i;
    for (i = 1970U; i < y; i++) days += leap(i) ? 366 : 365;
    for (i = 1U; i < m; i++) days += mlen(y, i);
    return days + (int32_t)d;
}

/* 0 Sunday .. 6 Saturday; day 1 (1970-01-01) was a Thursday */
static int wday(int32_t day) { return (int)((day - 1 + 4) % 7); }

/* the year of a minute count from 1970-01-01 00:00 (>= 0) */
static uint32_t year_of(int32_t min) {
    int32_t day = min / 1440 + 1;
    uint32_t y = 1970U;
    while (day > (leap(y) ? 366 : 365)) { day -= leap(y) ? 366 : 365; y++; }
    return y;
}

/* the n-th Sunday (1..5) of a month; n 9 the last one */
static int32_t sunday(uint32_t y, uint32_t m, int n) {
    if (n == 9) {
        int32_t last = ymd_day(y, m, (uint32_t)mlen(y, m));
        return last - wday(last);
    } else {
        int32_t first = ymd_day(y, m, 1U);
        return first + (7 - wday(first)) % 7 + 7 * (n - 1);
    }
}

/* minutes from 1970-01-01 00:00 UTC of "YYYYMMDDhhmm"; -1 invalid */
static int32_t utc_minutes(const char *utc) {
    uint32_t day = wf_autofill_day(utc);
    int k, hh, mm;
    if (!day) return -1;
    for (k = 8; k < 12; k++) if (utc[k] < '0' || utc[k] > '9') return -1;
    hh = (utc[8] - '0') * 10 + (utc[9] - '0');
    mm = (utc[10] - '0') * 10 + (utc[11] - '0');
    if (hh > 23 || mm > 59) return -1;
    return (int32_t)(day - 1U) * 1440 + hh * 60 + mm;
}

/* 60 when the rule puts the instant t (UTC minutes) in summer time for
 * a zone tz_min minutes from UTC (its standard time), else 0 */
static int dst_shift(int32_t t, int tz_min, int rule) {
    int32_t l = t + tz_min, a, b;
    uint32_t y;
    if (rule == WF_AF_DST_EU) {
        /* last Sunday of March 01:00 UTC to last Sunday of October 01:00 UTC */
        y = year_of(t);
        a = (sunday(y, 3U, 9) - 1) * 1440 + 60;
        b = (sunday(y, 10U, 9) - 1) * 1440 + 60;
        return t >= a && t < b ? 60 : 0;
    }
    if (l < 0) return 0;
    y = year_of(l);
    if (rule == WF_AF_DST_US) {
        /* second Sunday of March 02:00 standard to first Sunday of
         * November 02:00 summer time (01:00 standard) */
        a = (sunday(y, 3U, 2) - 1) * 1440 + 120;
        b = (sunday(y, 11U, 1) - 1) * 1440 + 60;
        return l >= a && l < b ? 60 : 0;
    }
    if (rule == WF_AF_DST_AU) {
        /* southern: first Sunday of October 02:00 standard to first
         * Sunday of April 03:00 summer time (02:00 standard) */
        a = (sunday(y, 10U, 1) - 1) * 1440 + 120;
        b = (sunday(y, 4U, 1) - 1) * 1440 + 120;
        return l >= a || l < b ? 60 : 0;
    }
    return 0;
}

int wf_autofill_set_dst(wf_autofill_t *a, int rule) {
    if (!a || rule < WF_AF_DST_NONE || rule > WF_AF_DST_AU) return -1;
    a->dst = rule;
    return 0;
}

const char *wf_autofill_dst_name(int rule) {
    switch (rule) {
    case WF_AF_DST_EU: return "eu";
    case WF_AF_DST_US: return "us";
    case WF_AF_DST_AU: return "au";
    case WF_AF_DST_NONE: return "off";
    default: return "";
    }
}

int wf_autofill_dst_parse(const char *s) {
    int k;
    for (k = WF_AF_DST_NONE; k <= WF_AF_DST_AU; k++) {
        const char *n = wf_autofill_dst_name(k);
        int i = 0;
        while (n[i] && s && s[i] == n[i]) i++;
        if (s && !n[i] && !s[i]) return k;
    }
    if (s && s[0] == 'n' && s[1] == 'o' && s[2] == 'n' && s[3] == 'e' && !s[4]) return WF_AF_DST_NONE;
    return -1;
}

int wf_autofill_dst_offset(const char *utc, int tz_min, int rule) {
    int32_t t;
    if (tz_min < -720 || tz_min > 840 || rule < WF_AF_DST_NONE || rule > WF_AF_DST_AU) return -1;
    t = utc_minutes(utc);
    if (t < 0) return -1;
    return dst_shift(t, tz_min, rule);
}

uint32_t wf_autofill_day_rule(const char *utc, int tz_min, int rule) {
    int32_t t, l;
    /* rule 0: exactly wf_autofill_day_tz (which takes a bare date at
     * offset 0) */
    if (rule == WF_AF_DST_NONE) return wf_autofill_day_tz(utc, tz_min);
    if (tz_min < -720 || tz_min > 840 || rule < WF_AF_DST_NONE || rule > WF_AF_DST_AU) return 0;
    t = utc_minutes(utc);
    if (t < 0) return 0;
    l = t + tz_min + dst_shift(t, tz_min, rule);
    if (l < 0 || l / 1440 + 1 > (int32_t)WF_AF_DAY_MAX) return 0;
    return (uint32_t)(l / 1440 + 1);
}

int wf_autofill_status(const wf_autofill_t *a, char *out, int cap) {
    int p = 0, i, vals = 0;
    if (!a || !out || cap < 2) return -1;
    for (i = 0; i < WF_AF_MAX; i++) if (a->e[i].used) vals += 1 + a->e[i].nold;
    p = app(out, cap, p, "autofill status expire ");
    p = app_u(out, cap, p, a->expire_days);
    p = p < 0 ? -1 : app(out, cap, p, a->tz_min < 0 ? " tz -" : " tz ");
    p = app_u(out, cap, p, (uint32_t)(a->tz_min < 0 ? -a->tz_min : a->tz_min));
    p = p < 0 ? -1 : app(out, cap, p, " today ");
    p = app_u(out, cap, p, a->today);
    p = p < 0 ? -1 : app(out, cap, p, " touched ");
    p = app_u(out, cap, p, a->touched);
    p = p < 0 ? -1 : app(out, cap, p, " expired ");
    p = app_u(out, cap, p, a->expired);
    p = p < 0 ? -1 : app(out, cap, p, " keys ");
    p = app_u(out, cap, p, (uint32_t)(a->n < 0 ? 0 : a->n));
    p = p < 0 ? -1 : app(out, cap, p, " values ");
    p = app_u(out, cap, p, (uint32_t)vals);
    /* Lot 91: the summer time rule, only when one is set (the Lot 90
     * line is unchanged otherwise) */
    if (p >= 0 && a->dst) { p = app(out, cap, p, " dst "); p = p < 0 ? -1 : app(out, cap, p, wf_autofill_dst_name(a->dst)); }
    p = p < 0 ? -1 : app(out, cap, p, "\n");
    if (p < 0) { out[0] = 0; return -1; }
    out[p] = 0;
    return p;
}

int wf_autofill_value_ages(const wf_autofill_t *a, char *out, int cap) {
    int p = 0, i, k;
    if (!a || !out || cap < 2) return -1;
    out[0] = 0;
    for (i = 0; i < WF_AF_MAX; i++) {
        const wf_af_ent_t *e = &a->e[i];
        if (!e->used) continue;
        for (k = 0; k <= e->nold; k++) {
            uint32_t day = k == 0 ? e->day : e->old_day[k - 1];
            p = app(out, cap, p, "autofill value ");
            p = p < 0 ? -1 : app(out, cap, p, e->host);
            p = p < 0 ? -1 : app(out, cap, p, " ");
            p = p < 0 ? -1 : app(out, cap, p, e->name);
            p = p < 0 ? -1 : app(out, cap, p, " ");
            p = app_u(out, cap, p, (uint32_t)k);
            if (p >= 0 && day && a->today) {
                p = app(out, cap, p, " ");
                p = app_u(out, cap, p, day >= a->today ? 0U : a->today - day);
                p = p < 0 ? -1 : app(out, cap, p, "d\n");
            } else if (p >= 0) p = app(out, cap, p, " undated\n");
            if (p < 0) { out[0] = 0; return -1; }
        }
    }
    out[p] = 0;
    return p;
}

int wf_autofill_ages(const wf_autofill_t *a, char *out, int cap) {
    int p = 0, i, n = 0, dated = 0;
    if (!a || !out || cap < 2) return -1;
    out[0] = 0;
    for (i = 0; i < WF_AF_MAX; i++) {
        const wf_af_ent_t *e = &a->e[i];
        if (!e->used) continue;
        n++;
        p = app(out, cap, p, "autofill age ");
        p = p < 0 ? -1 : app(out, cap, p, e->host);
        p = p < 0 ? -1 : app(out, cap, p, " ");
        p = p < 0 ? -1 : app(out, cap, p, e->name);
        if (p < 0) { out[0] = 0; return -1; }
        if (e->day && a->today) {
            dated++;
            p = app(out, cap, p, " ");
            p = app_u(out, cap, p, e->day >= a->today ? 0U : a->today - e->day);
            p = p < 0 ? -1 : app(out, cap, p, "d\n");
        } else {
            if (e->day) dated++;
            p = app(out, cap, p, " undated\n");
        }
        if (p < 0) { out[0] = 0; return -1; }
    }
    p = app(out, cap, p, "autofill ages ");
    p = app_u(out, cap, p, (uint32_t)n);
    p = p < 0 ? -1 : app(out, cap, p, " dated ");
    p = app_u(out, cap, p, (uint32_t)dated);
    p = p < 0 ? -1 : app(out, cap, p, "\n");
    if (p < 0) { out[0] = 0; return -1; }
    out[p] = 0;
    return p;
}
