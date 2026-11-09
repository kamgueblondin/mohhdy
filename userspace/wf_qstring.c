/* wf_qstring.c - Lot 86: see wf_qstring.h. */
#include "wf_qstring.h"

static int slen(const char *s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}
static int eq(const char *a, const char *b) {
    int i = 0;
    if (!a || !b) return 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}
static void clear(void *p, int n) {
    unsigned char *b = (unsigned char *)p;
    int i;
    for (i = 0; i < n; i++) b[i] = 0;
}
static int hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Decode one component into out. Returns the length, or -1. */
static int decode(const char *s, int n, char *out, int cap) {
    int i = 0, k = 0;
    if (cap <= 0) return -1;
    while (i < n) {
        unsigned char c;
        if (s[i] == '+') { c = ' '; i++; }
        else if (s[i] == '%') {
            int h, l;
            if (i + 2 >= n) return -1;
            h = hex(s[i + 1]);
            l = hex(s[i + 2]);
            if (h < 0 || l < 0) return -1;
            c = (unsigned char)((h << 4) | l);
            i += 3;
        } else c = (unsigned char)s[i++];
        if (k >= cap - 1) return -1;
        out[k++] = (char)c;
    }
    out[k] = 0;
    return k;
}

static int encode_byte(char *out, int cap, int n, unsigned char c) {
    const char *h = "0123456789ABCDEF";
    int plain = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                c == '-' || c == '_' || c == '.' || c == '*';
    if (c == ' ') {
        if (n + 1 >= cap) return -1;
        out[n++] = '+';
        return n;
    }
    if (plain) {
        if (n + 1 >= cap) return -1;
        out[n++] = (char)c;
        return n;
    }
    if (n + 3 >= cap) return -1;
    out[n++] = '%';
    out[n++] = h[c >> 4];
    out[n++] = h[c & 15];
    return n;
}

int wf_qs_parse(const char *s, int n, wf_qs_t *out) {
    int i = 0;
    if (!out) return -1;
    clear(out, (int)sizeof(*out));
    if (!s || n < 0) return -1;
    if (n == 0) return 0;
    while (i <= n) {
        int a = i, eqat = -1, b;
        while (i < n && s[i] != '&') {
            if (s[i] == '=' && eqat < 0) eqat = i;
            i++;
        }
        b = i;
        if (b > a || (i < n && s[i] == '&')) {
            wf_qs_pair_t *p;
            int rc;
            if (out->n >= WF_QS_PAIRS) out->dropped++;
            else {
                p = &out->pair[out->n];
                if (eqat < 0) {
                    rc = decode(s + a, b - a, p->key, (int)sizeof(p->key));
                    p->val[0] = 0;
                } else {
                    rc = decode(s + a, eqat - a, p->key, (int)sizeof(p->key));
                    if (rc >= 0) rc = decode(s + eqat + 1, b - (eqat + 1), p->val, (int)sizeof(p->val));
                }
                if (rc < 0) { out->bad++; p->key[0] = 0; p->val[0] = 0; }
                else if (p->key[0]) out->n++;
            }
        }
        if (i >= n) break;
        i++;
    }
    return out->bad ? -1 : 0;
}

int wf_qs_get(const wf_qs_t *q, const char *key, char *out, int cap) {
    int i;
    if (!q || !key || !out || cap <= 0) return -1;
    out[0] = 0;
    for (i = 0; i < q->n; i++) if (eq(q->pair[i].key, key)) {
        int k = 0;
        while (q->pair[i].val[k] && k < cap - 1) { out[k] = q->pair[i].val[k]; k++; }
        out[k] = 0;
        return 0;
    }
    return -1;
}

int wf_qs_format(const wf_qs_t *q, char *out, int cap) {
    int i, n = 0, k;
    if (!q || !out || cap <= 0) return -1;
    out[0] = 0;
    for (i = 0; i < q->n; i++) {
        if (i && n + 1 < cap) out[n++] = '&';
        else if (i) return -1;
        for (k = 0; q->pair[i].key[k]; k++) {
            n = encode_byte(out, cap, n, (unsigned char)q->pair[i].key[k]);
            if (n < 0) return -1;
        }
        if (n + 1 >= cap) return -1;
        out[n++] = '=';
        for (k = 0; q->pair[i].val[k]; k++) {
            n = encode_byte(out, cap, n, (unsigned char)q->pair[i].val[k]);
            if (n < 0) return -1;
        }
        out[n] = 0;
    }
    return n;
}

int wf_qs_same(const char *a, int an, const char *b, int bn) {
    static wf_qs_t qa, qb;
    int i, j, used[WF_QS_PAIRS];
    if (wf_qs_parse(a, an, &qa) != 0 || wf_qs_parse(b, bn, &qb) != 0) return 0;
    if (qa.n != qb.n) return 0;
    for (i = 0; i < WF_QS_PAIRS; i++) used[i] = 0;
    for (i = 0; i < qa.n; i++) {
        int hit = 0;
        for (j = 0; j < qb.n; j++) {
            if (used[j]) continue;
            if (eq(qa.pair[i].key, qb.pair[j].key) && eq(qa.pair[i].val, qb.pair[j].val)) {
                used[j] = 1;
                hit = 1;
                break;
            }
        }
        if (!hit) return 0;
    }
    return 1;
}

/* silence a length helper kept for callers that pass a C string */
int wf_qs_parse_str(const char *s, wf_qs_t *out) {
    return wf_qs_parse(s, slen(s), out);
}
