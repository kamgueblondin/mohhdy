/* wf_cssmath.c - Lot 89 (US-031): calc()/min()/max()/clamp() to one
 * length token, see wf_cssmath.h. Recursive descent over a bounded
 * string. */
#include "wf_cssmath.h"

typedef struct { int px, em, len, mix; } mv_t;     /* thousandths; len 0: a plain number */

typedef struct {
    const char *s;
    int i, depth, vw, vh, bad;
    /* Lot 90: ctx set: em and % are em_px thousandths of a px each, rem
     * is rem_px; every value is then a px value */
    int ctx, em_px, rem_px;
    /* Lot 109: box set (with ctx): ex, ch and lh are em_px fractions, %
     * is the em part (1/1000 % of the containing block), a min()/max()/
     * clamp() mixing parts is written to out (mix 1 then) */
    int box, olen, cap;
    char *out;
} mp_t;

static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

static int pre(const char *s, const char *w) {
    int i;
    for (i = 0; w[i]; i++) if (lc(s[i]) != w[i]) return 0;
    return 1;
}

static void ws(mp_t *p) { while (p->s[p->i] == ' ' || p->s[p->i] == '\t' || p->s[p->i] == '\n') p->i++; }

static int abs_i(int v) { return v < 0 ? -v : v; }

static int in_range(mv_t v) { return abs_i(v.px) <= WF_CSSMATH_MAX && abs_i(v.em) <= WF_CSSMATH_MAX; }

/* a * n / 1000 with |a|, |n| <= 10^7 in 32 bits */
static int mul_milli(int a, int n, int *bad) {
    int neg = (a < 0) != (n < 0), r;
    unsigned ua = (unsigned)abs_i(a), un = (unsigned)abs_i(n), hi, lo;
    hi = (ua / 1000U) * un;                   /* <= 10^4 * 10^7: too big, checked */
    if (ua / 1000U && un > 2000000000U / (ua / 1000U)) { *bad = 1; return 0; }
    lo = (ua % 1000U) * (un / 1000U) + ((ua % 1000U) * (un % 1000U)) / 1000U;
    if (hi > 2000000000U - lo) { *bad = 1; return 0; }
    r = (int)(hi + lo);
    return neg ? -r : r;
}

/* a * 1000 / n */
static int div_milli(int a, int n, int *bad) {
    int neg = (a < 0) != (n < 0);
    unsigned ua = (unsigned)abs_i(a), un = (unsigned)abs_i(n), q, r;
    if (!un) { *bad = 1; return 0; }
    q = ua / un;
    r = ua % un;
    if (q > 2000000U) { *bad = 1; return 0; }
    /* r < un <= 10^7: r * 1000 may pass 2^32 / 2; split the scale */
    q = q * 1000U + (r * 100U / un) * 10U + ((r * 100U % un) * 10U) / un;
    return neg ? -(int)q : (int)q;
}

static mv_t expr(mp_t *p);

/* a number in thousandths (up to three decimals, more are cut), or bad */
static int number(mp_t *p, int *out) {
    int v = 0, frac = 0, fd = 0, digits = 0, neg = 0;
    if (p->s[p->i] == '+' || p->s[p->i] == '-') { neg = p->s[p->i] == '-'; p->i++; }
    while (p->s[p->i] >= '0' && p->s[p->i] <= '9') {
        if (v > 100000) { p->bad = 1; return 0; }
        v = v * 10 + (p->s[p->i++] - '0');
        digits++;
    }
    if (p->s[p->i] == '.') {
        p->i++;
        while (p->s[p->i] >= '0' && p->s[p->i] <= '9') {
            if (fd < 3) { frac = frac * 10 + (p->s[p->i] - '0'); fd++; }
            p->i++;
            digits++;
        }
    }
    if (!digits) return 0;
    while (fd < 3) { frac *= 10; fd++; }
    *out = (neg ? -1 : 1) * (v * 1000 + frac);
    return 1;
}

/* a unit after a number: the value; len 0 when there was no unit */
static mv_t with_unit(mp_t *p, int n) {
    mv_t v;
    int vw = p->vw > 0 ? p->vw : 1024, vh = p->vh > 0 ? p->vh : 768;
    v.px = v.em = v.mix = 0;
    v.len = 1;
    if (pre(p->s + p->i, "px")) { p->i += 2; v.px = n; }
    else if (pre(p->s + p->i, "pt")) { p->i += 2; v.px = div_milli(mul_milli(n, 4000, &p->bad), 3000, &p->bad); }
    else if (pre(p->s + p->i, "pc")) { p->i += 2; v.px = mul_milli(n, 16000, &p->bad); }
    else if (pre(p->s + p->i, "rem")) { p->i += 3; v.px = mul_milli(n, p->ctx ? p->rem_px : 16000, &p->bad); }
    else if (pre(p->s + p->i, "em")) { p->i += 2; if (p->ctx) v.px = mul_milli(n, p->em_px, &p->bad); else v.em = n; }
    else if (pre(p->s + p->i, "vmin")) { p->i += 4; v.px = mul_milli(n, (vw < vh ? vw : vh) * 10, &p->bad); }
    else if (pre(p->s + p->i, "vmax")) { p->i += 4; v.px = mul_milli(n, (vw > vh ? vw : vh) * 10, &p->bad); }
    else if (pre(p->s + p->i, "vw")) { p->i += 2; v.px = mul_milli(n, vw * 10, &p->bad); }
    else if (pre(p->s + p->i, "vh")) { p->i += 2; v.px = mul_milli(n, vh * 10, &p->bad); }
    else if (p->box && (pre(p->s + p->i, "ex") || pre(p->s + p->i, "ch") || pre(p->s + p->i, "lh"))) {
        /* Lot 109: the font's metrics (wf_cssprop.h, webeng.h at 16 px):
         * ex 5/16 em (inked rows of "x"), ch 8/16 em (WE_CHAR_W), lh
         * 10/16 em (WE_LINE_H, line-height normal) */
        int k = p->s[p->i] == 'e' || p->s[p->i] == 'E' ? 5 : p->s[p->i] == 'c' || p->s[p->i] == 'C' ? 8 : 10;
        int x = mul_milli(n, p->em_px, &p->bad);
        p->i += 2;
        v.px = x / 16 * k + x % 16 * k / 16;
    }
    else if (p->s[p->i] == '%' && p->box) { p->i++; v.em = n; }
    else if (p->s[p->i] == '%') { p->i++; if (p->ctx) v.px = mul_milli(n / 100, p->em_px, &p->bad); else v.em = n / 100; }
    else if ((p->s[p->i] >= 'a' && p->s[p->i] <= 'z') || (p->s[p->i] >= 'A' && p->s[p->i] <= 'Z')) p->bad = 1;
    else { v.len = 0; v.px = n; }
    return v;
}

/* the arguments of min/max/clamp: comparable values with one part */
static int args(mp_t *p, mv_t *a, int cap) {
    int n = 0;
    for (;;) {
        ws(p);
        if (n >= cap) { p->bad = 1; return 0; }
        a[n++] = expr(p);
        if (p->bad) return 0;
        ws(p);
        if (p->s[p->i] == ',') { p->i++; continue; }
        if (p->s[p->i] == ')') { p->i++; return n; }
        p->bad = 1;
        return 0;
    }
}

/* the part a set of values share: 1 px, 2 em, 0 mixed (zero counts as either) */
static int one_part(const mv_t *a, int n) {
    int i, px = 0, em = 0;
    for (i = 0; i < n; i++) {
        if (a[i].len != a[0].len || a[i].mix) return 0;   /* a number against a length; Lot 109: a mixed one */
        if (a[i].px) px = 1;
        if (a[i].em) em = 1;
    }
    if (px && em) return 0;
    return em ? 2 : 1;
}

/* Lot 109: "N%", "Npx" or "calc(N% + Mpx)" for a value with both parts
 * (integers, rounded half away from zero, at most WF_CSSMATH_LIT: the
 * engine's math literals) at p->out */
static void put(mp_t *p, const char *t) {
    while (*t) {
        if (p->olen + 1 >= p->cap) { p->bad = 1; return; }
        p->out[p->olen++] = *t++;
    }
    p->out[p->olen] = 0;
}

static void put_int(mp_t *p, int milli, const char *unit) {
    char t[16];
    int r = (abs_i(milli) + 500) / 1000, k = 15;
    if (r > WF_CSSMATH_LIT) { p->bad = 1; return; }
    t[k] = 0;
    do { t[--k] = (char)('0' + r % 10); r /= 10; } while (r);
    put(p, t + k);
    put(p, unit);
}

static void put_lin(mp_t *p, mv_t v) {
    if (!v.em) { if (v.px < 0) put(p, "-"); put_int(p, v.px, "px"); return; }
    if (v.px) put(p, "calc(");
    if (v.em < 0) put(p, "-");
    put_int(p, v.em, "%");
    if (!v.px) return;
    put(p, v.px < 0 ? " - " : " + ");
    put_int(p, v.px, "px)");
}

/* Lot 109: a min()/max()/clamp() of lengths mixing px and % (box mode
 * only, one per value, not nested, not in an arithmetic): written as
 * the engine reads it */
static mv_t mixed(mp_t *p, const mv_t *a, int n, const char *fn) {
    mv_t v;
    int k;
    v.px = v.em = 0;
    v.len = v.mix = 1;
    if (!p->box || p->olen) { p->bad = 1; return v; }
    /* a mixed argument wrote out already (p->olen) */
    for (k = 0; k < n; k++) if (!a[k].len) { p->bad = 1; return v; }
    put(p, fn);
    for (k = 0; k < n; k++) { if (k) put(p, ", "); put_lin(p, a[k]); }
    put(p, ")");
    return v;
}

static int key(mv_t v, int part) { return part == 2 ? v.em : v.px; }

/* a factor: number[unit], ( expr ), calc( expr ), min/max/clamp( ... ).
 * len 0 when the factor is a plain number (for * and /) */
static mv_t factor(mp_t *p) {
    mv_t v, a[8];
    int n, k, part;
    v.px = v.em = v.len = v.mix = 0;
    ws(p);
    if (++p->depth > WF_CSSMATH_DEPTH) { p->bad = 1; return v; }
    if (p->s[p->i] == '(' || pre(p->s + p->i, "calc(")) {
        p->i += p->s[p->i] == '(' ? 1 : 5;
        v = expr(p);
        ws(p);
        if (p->s[p->i] != ')') p->bad = 1; else p->i++;
    } else if (pre(p->s + p->i, "min(") || pre(p->s + p->i, "max(")) {
        int is_min = pre(p->s + p->i, "min(");
        p->i += 4;
        n = args(p, a, 8);
        if (!p->bad) {
            part = one_part(a, n);
            if (!part) { p->depth--; return mixed(p, a, n, is_min ? "min(" : "max("); }
            v = a[0];
            for (k = 1; k < n && !p->bad; k++)
                if (is_min ? key(a[k], part) < key(v, part) : key(a[k], part) > key(v, part)) v = a[k];
        }
    } else if (pre(p->s + p->i, "clamp(")) {
        p->i += 6;
        n = args(p, a, 3);
        if (!p->bad && n != 3) p->bad = 1;
        if (!p->bad) {
            part = one_part(a, 3);
            if (!part) { p->depth--; return mixed(p, a, 3, "clamp("); }
            /* max(MIN, min(VAL, MAX)): MIN wins over MAX */
            v = a[1];
            if (key(a[2], part) < key(v, part)) v = a[2];
            if (key(a[0], part) > key(v, part)) v = a[0];
        }
    } else {
        int num;
        if (!number(p, &num)) { p->bad = 1; p->depth--; return v; }
        v = with_unit(p, num);
    }
    p->depth--;
    if (!in_range(v)) p->bad = 1;
    return v;
}

/* term: factor (('*' | '/') factor)*: one side of * is a plain number,
 * the right side of / is a plain number */
static mv_t term(mp_t *p) {
    mv_t v = factor(p), w;
    for (;;) {
        int at = p->i;
        char op;
        ws(p);
        op = p->s[p->i];
        if (p->bad || (op != '*' && op != '/')) { p->i = at; return v; }
        p->i++;
        w = factor(p);
        if (p->bad) return v;
        if (v.mix || w.mix) { p->bad = 1; return v; }
        if (op == '*') {
            if (!w.len) { v.px = mul_milli(v.px, w.px, &p->bad); v.em = mul_milli(v.em, w.px, &p->bad); }
            else if (!v.len) { int k = v.px; v.px = mul_milli(w.px, k, &p->bad); v.em = mul_milli(w.em, k, &p->bad); v.len = 1; }
            else { p->bad = 1; return v; }
        } else {
            if (w.len) { p->bad = 1; return v; }
            v.px = div_milli(v.px, w.px, &p->bad);
            v.em = div_milli(v.em, w.px, &p->bad);
        }
        if (!in_range(v)) { p->bad = 1; return v; }
    }
}

/* expr: term (('+' | '-') term)*; CSS wants spaces around + and -, a
 * sign glued to a number belongs to the number */
static mv_t expr(mp_t *p) {
    mv_t v = term(p), w;
    for (;;) {
        char op;
        int sp;
        sp = p->s[p->i] == ' ' || p->s[p->i] == '\t' || p->s[p->i] == '\n';
        ws(p);
        op = p->s[p->i];
        if (p->bad || (op != '+' && op != '-')) return v;
        if (!sp || !(p->s[p->i + 1] == ' ' || p->s[p->i + 1] == '\t' || p->s[p->i + 1] == '\n')) {
            p->bad = 1;
            return v;
        }
        p->i++;
        w = term(p);
        if (w.len != v.len || v.mix || w.mix) { p->bad = 1; return v; }   /* a number plus a length */
        if (op == '+') { v.px += w.px; v.em += w.em; }
        else { v.px -= w.px; v.em -= w.em; }
        if (!in_range(v)) { p->bad = 1; return v; }
    }
}

int wf_css_is_math(const char *s) {
    if (!s) return 0;
    return pre(s, "calc(") || pre(s, "min(") || pre(s, "max(") || pre(s, "clamp(");
}

/* "N.NNunit" with at most two decimals, rounded half away from zero */
int wf_css_token(int milli, const char *unit, char *out, int cap) {
    int n = 0, neg = milli < 0, v, whole, frac, k;
    char t[16];
    v = (abs_i(milli) + 5) / 10;              /* hundredths */
    whole = v / 100;
    frac = v % 100;
    if (neg && v) t[n++] = '-';
    {
        char d[12];
        int m = 0;
        do { d[m++] = (char)('0' + whole % 10); whole /= 10; } while (whole && m < 11);
        while (m) t[n++] = d[--m];
    }
    if (frac) {
        t[n++] = '.';
        t[n++] = (char)('0' + frac / 10);
        if (frac % 10) t[n++] = (char)('0' + frac % 10);
    }
    for (k = 0; unit[k]; k++) t[n++] = unit[k];
    if (n >= cap) return -1;
    for (k = 0; k < n; k++) out[k] = t[k];
    out[n] = 0;
    return n;
}

int wf_css_math(const char *s, int vw, int vh, char *out, int cap) {
    mp_t p;
    mv_t v;
    if (!s || !out || cap <= 0) return -1;
    out[0] = 0;
    if (!wf_css_is_math(s)) return 0;
    p.s = s; p.i = 0; p.depth = 0; p.vw = vw; p.vh = vh; p.bad = 0;
    p.ctx = 0; p.em_px = p.rem_px = 0; p.box = 0; p.olen = 0;
    v = factor(&p);
    ws(&p);
    if (p.bad || p.s[p.i] || !v.len) return -1;
    if (v.px && v.em) return -1;
    if (v.em) return wf_css_token(v.em, "em", out, cap) < 0 ? -1 : 1;
    return wf_css_token(v.px, "px", out, cap) < 0 ? -1 : 1;
}

/* Lot 90: the value in px against a known em and root size */
int wf_css_math_px(const char *s, int vw, int vh, int em_px, int rem_px, int *px) {
    mp_t p;
    mv_t v;
    if (!s || !px) return -1;
    *px = 0;
    if (!s[0]) return 0;
    if (em_px <= 0 || em_px > WF_CSSMATH_MAX || rem_px <= 0 || rem_px > WF_CSSMATH_MAX) return -1;
    p.s = s; p.i = 0; p.depth = 0; p.vw = vw; p.vh = vh; p.bad = 0;
    p.ctx = 1; p.em_px = em_px; p.rem_px = rem_px; p.box = 0; p.olen = 0;
    v = factor(&p);
    ws(&p);
    if (p.bad || p.s[p.i]) return -1;
    if (!v.len && v.px) return -1;              /* a plain number other than 0 */
    *px = v.px;
    return 1;
}

/* Lot 109: a box scroller length, see wf_cssmath.h */
int wf_css_math_box(const char *s, int vw, int vh, int em_px, int rem_px, int *px, char *out, int cap) {
    mp_t p;
    mv_t v;
    if (!s || !px || !out || cap <= 0) return -1;
    *px = 0;
    out[0] = 0;
    if (!s[0]) return 0;
    if (em_px <= 0 || em_px > WF_CSSMATH_MAX || rem_px <= 0 || rem_px > WF_CSSMATH_MAX) return -1;
    p.s = s; p.i = 0; p.depth = 0; p.vw = vw; p.vh = vh; p.bad = 0;
    p.ctx = 1; p.em_px = em_px; p.rem_px = rem_px;
    p.box = 1; p.olen = 0; p.cap = cap; p.out = out;
    v = factor(&p);
    ws(&p);
    if (p.bad || p.s[p.i] || (!v.len && v.px)) { out[0] = 0; return -1; }
    if (v.mix) return 2;
    if (!v.em) { *px = v.px; return 1; }
    put_lin(&p, v);
    if (p.bad) { out[0] = 0; return -1; }
    return 2;
}
