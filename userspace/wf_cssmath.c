/* wf_cssmath.c - Lot 89 (US-031): calc()/min()/max()/clamp() to one
 * length token, see wf_cssmath.h. Recursive descent over a bounded
 * string. */
#include "wf_cssmath.h"
#include "../include/os_ui_scale.h"

typedef struct { int px, em, len, mix; } mv_t;     /* thousandths; len 0: a plain number */

/* Lot 111: a value mixing px and % (box mode) as a tree: MX_LIN a
 * linear part (px and em fields), MX_MIN / MX_MAX / MX_CLAMP (3) of the
 * kids; mv_t.mix is its index + 1 */
enum { MX_LIN, MX_MIN, MX_MAX, MX_CLAMP };
#define MX_NODES 32      /* Lot 116: was 24 */
#define MX_KIDS 8
typedef struct { unsigned char op, n, kid[MX_KIDS]; int px, em; } mx_t;

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
    /* Lot 111: lh against this line height (px thousandths) when >= 0,
     * else line-height normal (10/16 em); Lot 112: rlh against the root's
     * (else 10/16 rem) */
    int lh_px, rlh_px;
    /* Lot 112: ex and ch against these (px thousandths) when >= 0 */
    int ex_px, ch_px;
    /* Lot 111: % of this basis (px thousandths) when >= 0 (box mode) */
    int basis;
    /* Lot 111: the trees of mixed values */
    int nmx;
    mx_t mx[MX_NODES];
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
    else if (p->box && pre(p->s + p->i, "rlh")) {
        /* Lot 112: the root element's line height */
        p->i += 3;
        if (p->rlh_px >= 0) v.px = mul_milli(n, p->rlh_px, &p->bad);
        else { int x = mul_milli(n, p->rem_px, &p->bad); v.px = x / 16 * 10 + x % 16 * 10 / 16; }
    }
    else if (p->box && (pre(p->s + p->i, "rcap") || pre(p->s + p->i, "rex") || pre(p->s + p->i, "rch") ||
                        pre(p->s + p->i, "ric") || pre(p->s + p->i, "cap") || pre(p->s + p->i, "ic"))) {
        /* Lot 116: cap (and the root's rcap, rex, rch, ric; CSS Values 4
         * 6.1.1): the inked rows of a capital in the engine's 8x8 font
         * are 7 (kernel/gfx_font8.h), so cap is os_ui_px(7, q) of the
         * font's scale when ex and ch are the engine's (ex_px >= 0), 7/16
         * of the size otherwise (as ex 5/16 and ch 8/16); the font has no
         * ideograph, so ic is 1em (CSS Values 4 6.1.1.1). The root ones
         * take the root's font size (rem_px) */
        int root = p->s[p->i] == 'r' || p->s[p->i] == 'R', u = p->s[p->i + root], k, f, x;
        k = u == 'c' || u == 'C' ? (p->s[p->i + root + 1] == 'a' || p->s[p->i + root + 1] == 'A' ? 7 : 8)
          : u == 'e' || u == 'E' ? 5 : 16;
        p->i += root + (k == 7 ? 3 : 2);
        f = root ? p->rem_px : p->em_px;
        if (k == 16) v.px = mul_milli(n, f, &p->bad);
        else if (p->ex_px >= 0) {
            /* the font's px rounded and its scale, as for ex and ch
             * (wf_sitecss.c font_metrics) */
            int px = f / 1000 + (f % 1000 >= 500), q = os_ui_q_clamp((px + 2) / 4);
            v.px = mul_milli(n, os_ui_px(k, q) * 1000, &p->bad);
        } else {
            x = mul_milli(n, f, &p->bad);
            v.px = x / 16 * k + x % 16 * k / 16;
        }
    }
    else if (p->box && (pre(p->s + p->i, "ex") || pre(p->s + p->i, "ch") || pre(p->s + p->i, "lh"))) {
        /* Lot 109: the font's metrics (wf_cssprop.h, webeng.h at 16 px):
         * ex 5/16 em (inked rows of "x"), ch 8/16 em (WE_CHAR_W), lh
         * 10/16 em (WE_LINE_H, line-height normal) */
        int k = p->s[p->i] == 'e' || p->s[p->i] == 'E' ? 5 : p->s[p->i] == 'c' || p->s[p->i] == 'C' ? 8 : 10;
        int x = mul_milli(n, p->em_px, &p->bad);
        p->i += 2;
        v.px = x / 16 * k + x % 16 * k / 16;
        /* Lot 111: lh of an element with a line height of its own */
        if (k == 10 && p->lh_px >= 0) v.px = mul_milli(n, p->lh_px, &p->bad);
        /* Lot 112: ex and ch of the element's actual font */
        if (k == 5 && p->ex_px >= 0) v.px = mul_milli(n, p->ex_px, &p->bad);
        if (k == 8 && p->ch_px >= 0) v.px = mul_milli(n, p->ch_px, &p->bad);
    }
    else if (p->s[p->i] == '%' && p->box) {
        p->i++;
        /* Lot 111: of a known basis, a px value (n is 1/1000 %) */
        if (p->basis >= 0) v.px = mul_milli(n, p->basis, &p->bad) / 100;
        else v.em = n;
    }
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

/* Lot 111: a new tree node, or -1 (bad) */
static int mx_new(mp_t *p, int op, int px, int em) {
    mx_t *x;
    if (p->nmx >= MX_NODES) { p->bad = 1; return -1; }
    x = &p->mx[p->nmx];
    x->op = (unsigned char)op; x->n = 0; x->px = px; x->em = em;
    return p->nmx++;
}

/* the tree of a value (a linear one gets a leaf) */
static int mx_of(mp_t *p, mv_t v) { return v.mix ? v.mix - 1 : mx_new(p, MX_LIN, v.px, v.em); }

static mv_t mx_val(mp_t *p, int x) {
    mv_t v;
    v.len = 1; v.mix = 0; v.px = v.em = 0;
    if (x < 0 || p->bad) { p->bad = 1; return v; }
    if (p->mx[x].op == MX_LIN) { v.px = p->mx[x].px; v.em = p->mx[x].em; }
    else v.mix = x + 1;
    if (!in_range(v)) p->bad = 1;
    return v;
}

/* Lot 112: leaf b is never below leaf a (op MX_MIN: a can stand for b
 * in a min()) when a's px and % parts are both at most b's, as a % is of
 * a size that is never negative; the first of equal ones stays */
static int mx_cover(const mx_t *a, const mx_t *b, int op) {
    if (op == MX_MIN) return a->px <= b->px && a->em <= b->em;
    return a->px >= b->px && a->em >= b->em;
}

/* Lot 111: min (op MX_MIN) or max of the trees kid[0..n): a kid of the
 * same op gives its own kids (min(a, min(b, c)) is min(a, b, c)); one kid
 * is the value itself. Lot 112: a leaf covered by another leaf goes
 * (mx_cover: min(10px, 20px, 5%) is min(10px, 5%), min(3px, calc(10% +
 * 5px)) is 3px), so more values fit the engine's bounds */
static int mx_op(mp_t *p, int op, const int *kid, int n) {
    int k, j, x;
    int flat[MX_KIDS], m = 0, keep = 0;
    if (p->bad) return -1;
    for (k = 0; k < n; k++) {
        const mx_t *t = &p->mx[kid[k] < 0 ? 0 : kid[k]];
        int c = t->op == op ? t->n : 1;
        if (kid[k] < 0 || m + c > MX_KIDS) { p->bad = 1; return -1; }
        if (t->op == op) for (j = 0; j < c; j++) flat[m++] = t->kid[j];
        else flat[m++] = kid[k];
    }
    for (k = 0; k < m; k++) {
        const mx_t *b = &p->mx[flat[k]];
        int gone = 0;
        for (j = 0; j < m && !gone && b->op == MX_LIN; j++) {
            const mx_t *a = &p->mx[flat[j]];
            if (j == k || a->op != MX_LIN || !mx_cover(a, b, op)) continue;
            /* of two equal leaves the later one goes */
            gone = j < k || !mx_cover(b, a, op);
        }
        if (!gone) flat[keep++] = flat[k];
    }
    m = keep;
    if (m == 1) return flat[0];
    x = mx_new(p, op, 0, 0);
    if (x < 0) return -1;
    for (k = 0; k < m; k++) p->mx[x].kid[k] = (unsigned char)flat[k];
    p->mx[x].n = (unsigned char)m;
    return x;
}

/* Lot 111: clamp(kid[0], kid[1], kid[2]) */
static int mx_clamp(mp_t *p, const int *kid) {
    int x, k;
    for (k = 0; k < 3; k++) if (kid[k] < 0) { p->bad = 1; return -1; }
    x = mx_new(p, MX_CLAMP, 0, 0);
    if (x < 0) return -1;
    for (k = 0; k < 3; k++) p->mx[x].kid[k] = (unsigned char)kid[k];
    p->mx[x].n = 3;
    return x;
}

/* Lot 111: tree x times k/1000 (div 0) or 1000/k (div 1): a negative
 * factor turns min into max (k * min(a, b) is max(k * a, k * b)) */
static int mx_scale(mp_t *p, int x, int k, int div) {
    int kid[MX_KIDS], j, n, op;
    if (p->bad || x < 0) return -1;
    if (p->mx[x].op == MX_LIN) {
        int px = div ? div_milli(p->mx[x].px, k, &p->bad) : mul_milli(p->mx[x].px, k, &p->bad);
        int em = div ? div_milli(p->mx[x].em, k, &p->bad) : mul_milli(p->mx[x].em, k, &p->bad);
        mv_t v;
        v.px = px; v.em = em;
        if (!in_range(v)) { p->bad = 1; return -1; }
        return mx_new(p, MX_LIN, px, em);
    }
    n = p->mx[x].n;
    op = p->mx[x].op;
    if (op == MX_CLAMP) {
        for (j = 0; j < 3; j++) kid[j] = mx_scale(p, p->mx[x].kid[j], k, div);
        if (k >= 0) return mx_clamp(p, kid);
        /* -clamp(a, b, c) is min(-a, max(-b, -c)) */
        kid[1] = mx_op(p, MX_MAX, kid + 1, 2);
        return mx_op(p, MX_MIN, kid, 2);
    }
    if (k < 0) op = op == MX_MIN ? MX_MAX : MX_MIN;
    for (j = 0; j < n; j++) kid[j] = mx_scale(p, p->mx[x].kid[j], k, div);
    return mx_op(p, op, kid, n);
}

/* Lot 111: tree x plus (sg 1) or minus (sg -1) tree y: the sum goes into
 * the kids (min(a, b) + c is min(a + c, b + c)) */
static int mx_add(mp_t *p, int x, int y, int sg) {
    int kid[MX_KIDS], j, n;
    if (p->bad || x < 0 || y < 0) return -1;
    if (p->mx[x].op == MX_LIN && p->mx[y].op == MX_LIN) {
        mv_t v;
        v.px = p->mx[x].px + sg * p->mx[y].px; v.em = p->mx[x].em + sg * p->mx[y].em;
        if (!in_range(v)) { p->bad = 1; return -1; }
        return mx_new(p, MX_LIN, v.px, v.em);
    }
    if (p->mx[x].op != MX_LIN) {
        n = p->mx[x].n;
        for (j = 0; j < n; j++) kid[j] = mx_add(p, p->mx[x].kid[j], y, sg);
        return p->mx[x].op == MX_CLAMP ? mx_clamp(p, kid) : mx_op(p, p->mx[x].op, kid, n);
    }
    if (sg < 0) return mx_add(p, x, mx_scale(p, y, -1000, 0), 1);
    n = p->mx[y].n;
    for (j = 0; j < n; j++) kid[j] = mx_add(p, x, p->mx[y].kid[j], 1);
    return p->mx[y].op == MX_CLAMP ? mx_clamp(p, kid) : mx_op(p, p->mx[y].op, kid, n);
}

/* Lot 111: a min()/max()/clamp() of lengths mixing px and % (box mode
 * only; Lot 109 took one per value, not nested, not in an arithmetic) */
static mv_t mixed(mp_t *p, const mv_t *a, int n, int op) {
    mv_t v;
    int k, kid[MX_KIDS];
    v.px = v.em = v.mix = 0;
    v.len = 1;
    if (!p->box) { p->bad = 1; return v; }
    for (k = 0; k < n; k++) if (!a[k].len) { p->bad = 1; return v; }
    for (k = 0; k < n; k++) kid[k] = mx_of(p, a[k]);
    if (op == MX_CLAMP) return mx_val(p, mx_clamp(p, kid));
    return mx_val(p, mx_op(p, op, kid, n));
}

/* Lot 111: the engine's nodes (webeng.c parse_math) and function depth of
 * tree x as put_mx writes it */
static int mx_cost(const mp_t *p, int x, int *depth) {
    const mx_t *t = &p->mx[x];
    int j, n, d, dm = 0;
    *depth = 0;     /* also past the argument bound (the cost refuses it) */
    if (t->op == MX_LIN) { *depth = t->em && t->px ? 1 : 0; return t->em && t->px ? 3 : 1; }
    if (t->n > WF_CSSMATH_ARGS) return WF_CSSMATH_NODES + 1;      /* the engine takes 8 arguments (Lot 116) */
    n = t->op == MX_CLAMP ? 1 : t->n - 1;
    for (j = 0; j < t->n; j++) {
        n += mx_cost(p, t->kid[j], &d);
        if (d > dm) dm = d;
    }
    *depth = dm + 1;
    return n;
}

static void put_mx(mp_t *p, int x) {
    const mx_t *t = &p->mx[x];
    int j;
    mv_t v;
    if (t->op == MX_LIN) { v.px = t->px; v.em = t->em; put_lin(p, v); return; }
    put(p, t->op == MX_MIN ? "min(" : t->op == MX_MAX ? "max(" : "clamp(");
    for (j = 0; j < t->n; j++) { if (j) put(p, ", "); put_mx(p, t->kid[j]); }
    put(p, ")");
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
            if (!part) { p->depth--; return mixed(p, a, n, is_min ? MX_MIN : MX_MAX); }
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
            if (!part) { p->depth--; return mixed(p, a, 3, MX_CLAMP); }
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
        if (v.mix || w.mix) {
            /* Lot 111: a mixed value times or over a plain number */
            int k = v.mix ? w.px : v.px;
            if ((v.mix && w.mix) || (v.mix ? w.len : v.len) || (op == '/' && !v.mix)) { p->bad = 1; return v; }
            v = mx_val(p, mx_scale(p, v.mix ? v.mix - 1 : w.mix - 1, k, op == '/'));
            if (p->bad) return v;
            continue;
        }
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
        if (w.len != v.len) { p->bad = 1; return v; }   /* a number plus a length */
        if (v.mix || w.mix) {           /* Lot 111: a mixed value in a sum */
            v = mx_val(p, mx_add(p, mx_of(p, v), mx_of(p, w), op == '+' ? 1 : -1));
            if (p->bad) return v;
            continue;
        }
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
    p.ctx = 0; p.em_px = p.rem_px = 0; p.box = 0; p.olen = 0; p.lh_px = p.rlh_px = p.ex_px = p.ch_px = -1; p.basis = -1; p.nmx = 0;
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
    p.ctx = 1; p.em_px = em_px; p.rem_px = rem_px; p.box = 0; p.olen = 0; p.lh_px = p.rlh_px = p.ex_px = p.ch_px = -1; p.basis = -1; p.nmx = 0;
    v = factor(&p);
    ws(&p);
    if (p.bad || p.s[p.i]) return -1;
    if (!v.len && v.px) return -1;              /* a plain number other than 0 */
    *px = v.px;
    return 1;
}

static int box_run(const char *s, const wf_css_box_ctx_t *c, int *px, char *out, int cap);

static wf_css_box_ctx_t box_ctx(int vw, int vh, int em_px, int rem_px, int lh_px, int basis) {
    wf_css_box_ctx_t c;
    c.vw = vw; c.vh = vh; c.em_px = em_px; c.rem_px = rem_px; c.lh_px = lh_px; c.rlh_px = -1; c.basis = basis; c.ex_px = c.ch_px = -1;
    return c;
}

/* Lot 109: a box scroller length, see wf_cssmath.h */
int wf_css_math_box(const char *s, int vw, int vh, int em_px, int rem_px, int *px, char *out, int cap) {
    wf_css_box_ctx_t c = box_ctx(vw, vh, em_px, rem_px, -1, -1);
    return box_run(s, &c, px, out, cap);
}

/* Lot 111: with the element's line height, see wf_cssmath.h */
int wf_css_math_box_lh(const char *s, int vw, int vh, int em_px, int rem_px, int lh_px, int *px, char *out, int cap) {
    wf_css_box_ctx_t c = box_ctx(vw, vh, em_px, rem_px, lh_px, -1);
    return box_run(s, &c, px, out, cap);
}

/* Lot 111: with percentages of a known basis, see wf_cssmath.h */
int wf_css_math_box_at(const char *s, int vw, int vh, int em_px, int rem_px, int lh_px, int basis, int *px) {
    wf_css_box_ctx_t c = box_ctx(vw, vh, em_px, rem_px, lh_px, basis);
    if (px) *px = 0;
    if (basis < 0) return -1;
    return wf_css_math_box_ctx(s, &c, px, 0, 0);
}

/* Lot 112: see wf_cssmath.h */
int wf_css_math_box_ctx(const char *s, const wf_css_box_ctx_t *c, int *px, char *out, int cap) {
    char o[4];
    if (px) *px = 0;
    if (!c) return -1;
    if (c->basis < 0) return box_run(s, c, px, out, cap);
    if (c->basis > WF_CSSMATH_MAX) return -1;
    return box_run(s, c, px, o, (int)sizeof(o)) == 1 ? 1 : -1;
}

static int box_run(const char *s, const wf_css_box_ctx_t *c, int *px, char *out, int cap) {
    mp_t p;
    mv_t v;
    if (!s || !px || !out || cap <= 0) return -1;
    *px = 0;
    out[0] = 0;
    if (!s[0]) return 0;
    if (c->em_px <= 0 || c->em_px > WF_CSSMATH_MAX || c->rem_px <= 0 || c->rem_px > WF_CSSMATH_MAX ||
        c->lh_px > WF_CSSMATH_MAX || c->rlh_px > WF_CSSMATH_MAX || c->ex_px > WF_CSSMATH_MAX || c->ch_px > WF_CSSMATH_MAX)
        return -1;
    p.s = s; p.i = 0; p.depth = 0; p.vw = c->vw; p.vh = c->vh; p.bad = 0;
    p.ctx = 1; p.em_px = c->em_px; p.rem_px = c->rem_px;
    p.box = 1; p.olen = 0; p.cap = cap; p.out = out; p.lh_px = c->lh_px < 0 ? -1 : c->lh_px;
    p.rlh_px = c->rlh_px < 0 ? -1 : c->rlh_px; p.basis = c->basis; p.nmx = 0;
    p.ex_px = c->ex_px < 0 ? -1 : c->ex_px; p.ch_px = c->ch_px < 0 ? -1 : c->ch_px;
    v = factor(&p);
    ws(&p);
    if (p.bad || p.s[p.i] || (!v.len && v.px)) { out[0] = 0; return -1; }
    if (v.mix) {
        int d;
        /* Lot 111: the tree as the engine's math reads it (at most
         * WE_MATH_NODES_MAX nodes, WE_MATH_DEPTH_MAX functions deep) */
        if (mx_cost(&p, v.mix - 1, &d) > WF_CSSMATH_NODES || d > WF_CSSMATH_FN_DEPTH) return -1;
        put_mx(&p, v.mix - 1);
        if (p.bad) { out[0] = 0; return -1; }
        return 2;
    }
    if (!v.em) { *px = v.px; return 1; }
    put_lin(&p, v);
    if (p.bad) { out[0] = 0; return -1; }
    return 2;
}
