/* wf_autofill.c - Lot 86: see wf_autofill.h. */
#include "wf_autofill.h"

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
static void push_value(wf_af_ent_t *e, const char *value) {
    int k, at = -1;
    if (seq(e->value, value)) return;
    for (k = 0; k < e->nold; k++) if (seq(e->old[k], value)) { at = k; break; }
    if (at < 0) {
        at = e->nold < WF_AF_HIST - 1 ? e->nold++ : WF_AF_HIST - 2;
    }
    /* close the gap at 'at', then shift the rest down by one */
    for (k = at; k > 0; k--) acpy(e->old[k], WF_AF_VALUE, e->old[k - 1], 0);
    if (WF_AF_HIST > 1) acpy(e->old[0], WF_AF_VALUE, e->value, 0);
    acpy(e->value, WF_AF_VALUE, value, 0);
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
            push_value(&a->e[i], value);
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
    if (at == 0) acpy(e->value, WF_AF_VALUE, e->old[0], 0);
    for (k = (at == 0 ? 0 : at - 1); k < e->nold - 1; k++) acpy(e->old[k], WF_AF_VALUE, e->old[k + 1], 0);
    e->nold--;
    azero(e->old[e->nold], (uint32_t)WF_AF_VALUE);
    return 1 + e->nold;
}

static int app(char *o, int cap, int p, const char *s) {
    while (s && *s) { if (p >= cap - 1) return -1; o[p++] = *s++; }
    return p;
}

int wf_autofill_save(const wf_autofill_t *a, char *out, int cap) {
    int p = 0, i;
    if (!a || !out || cap < 16) return -1;
    p = app(out, cap, p, "MOHHDY-AUTOFILL 1\n");
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
            out[p++] = '\n';
        }
    }
    out[p] = 0;
    return p;
}

int wf_autofill_load(wf_autofill_t *a, const char *text, int len) {
    wf_autofill_t tmp;
    int i = 0, line = 0;
    if (!a || !text || len < 0) return -1;
    wf_autofill_init(&tmp);
    while (i < len) {
        char buf[WF_AF_HOST + WF_AF_NAME + WF_AF_VALUE + 4];
        int k = 0, t1 = -1, t2 = -1;
        while (i < len && text[i] != '\n' && k < (int)sizeof(buf) - 1) buf[k++] = text[i++];
        if (i < len && text[i] == '\n') i++;
        buf[k] = 0;
        line++;
        if (k == 0) continue;
        if (line == 1) {
            if (buf[0] != 'M') return -1;
            continue;
        }
        for (k = 0; buf[k]; k++) if (buf[k] == '\t') { if (t1 < 0) t1 = k; else if (t2 < 0) t2 = k; }
        if (t1 < 0 || t2 < 0) return -1;
        buf[t1] = 0; buf[t2] = 0;
        if (wf_autofill_put(&tmp, buf, buf + t1 + 1, buf + t2 + 1, "text") != 0) return -1;
    }
    {
        uint8_t *d = (uint8_t *)a;
        const uint8_t *s = (const uint8_t *)&tmp;
        uint32_t n = (uint32_t)sizeof(tmp);
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
