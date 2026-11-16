/* wf_cssprop.c - Lot 90: see wf_cssprop.h. */
#include "wf_cssprop.h"
#include "wf_cssbg.h"

static int cp_sp(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
static char cp_low(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int cp_digit(char c) { return c >= '0' && c <= '9'; }
static int cp_ident(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || cp_digit(c) || c == '-' || c == '_';
}
static int cp_len(const char *s) { int n = 0; while (s[n]) n++; return n; }

/* s[0..n) equals w (lower case), ASCII case-insensitive */
static int cp_eqn(const char *s, int n, const char *w) {
    int k;
    for (k = 0; k < n; k++) if (!w[k] || cp_low(s[k]) != w[k]) return 0;
    return w[n] == 0;
}

/* Lot 91: the type names a syntax may use */
static int cp_type_name(const char *s, int n) {
    static const char *const names[] = {"<length>", "<percentage>", "<length-percentage>", "<number>",
                                        "<integer>", "<color>", "<angle>", "<time>", "<custom-ident>",
                                        /* Lot 95 */
                                        "<url>", "<image>", "<resolution>", "<transform-function>",
                                        "<transform-list>", "<string>", 0};
    static const int codes[] = {WF_CSSPROP_LENGTH, WF_CSSPROP_PERCENT, WF_CSSPROP_LENPCT, WF_CSSPROP_NUMBER,
                                WF_CSSPROP_INTEGER, WF_CSSPROP_COLOR, WF_CSSPROP_ANGLE, WF_CSSPROP_TIME,
                                WF_CSSPROP_CIDENT, WF_CSSPROP_URL, WF_CSSPROP_IMAGE, WF_CSSPROP_RESOLUTION,
                                WF_CSSPROP_TFUNC, WF_CSSPROP_TLIST, WF_CSSPROP_STRING};
    int k;
    for (k = 0; names[k]; k++) if (cp_eqn(s, n, names[k])) return codes[k];
    return WF_CSSPROP_BAD;
}

/* Lot 91: CSS-wide keywords and "default" are never a custom ident */
static int cp_reserved(const char *s, int n) {
    static const char *const w[] = {"initial", "inherit", "unset", "revert", "revert-layer", "default", 0};
    int k;
    for (k = 0; w[k]; k++) if (cp_eqn(s, n, w[k])) return 1;
    return 0;
}

/* Lot 91: an identifier: [a-z_] or '-' then [a-z_-], then [a-z0-9_-] */
static int cp_is_ident(const char *s, int n) {
    int k;
    char c0, c1;
    if (n < 1) return 0;
    c0 = cp_low(s[0]);
    if (c0 == '-') {
        if (n < 2) return 0;
        c1 = cp_low(s[1]);
        if (!((c1 >= 'a' && c1 <= 'z') || c1 == '_' || c1 == '-')) return 0;
    } else if (!((c0 >= 'a' && c0 <= 'z') || c0 == '_')) return 0;
    for (k = 1; k < n; k++) if (!cp_ident(s[k])) return 0;
    return 1;
}

int wf_cssprop_syntax_parse(const char *s, int n, wf_cssprop_t *out) {
    int a = 0, b = n, k, nalt = 0;
    wf_cssprop_alt_t alt[WF_CSSPROP_ALTS];
    if (out) out->nalt = 0;
    if (!s || n < 3) return WF_CSSPROP_BAD;
    while (a < b && cp_sp(s[a])) a++;
    while (b > a && cp_sp(s[b - 1])) b--;
    /* a string: same quote at both ends */
    if (b - a < 3 || (s[a] != '\'' && s[a] != '"') || s[b - 1] != s[a]) return WF_CSSPROP_BAD;
    a++;
    b--;
    while (a < b && cp_sp(s[a])) a++;
    while (b > a && cp_sp(s[b - 1])) b--;
    if (b - a == 1 && s[a] == '*') return WF_CSSPROP_ANY;
    while (a <= b) {
        int e = a, x, y, m = WF_CSSPROP_ONE, t;
        while (e < b && s[e] != '|') e++;
        x = a; y = e;
        while (x < y && cp_sp(s[x])) x++;
        while (y > x && cp_sp(s[y - 1])) y--;
        if (y <= x || nalt >= WF_CSSPROP_ALTS) return WF_CSSPROP_BAD;
        if (s[y - 1] == '+' || s[y - 1] == '#') {
            m = s[y - 1] == '+' ? WF_CSSPROP_PLUS : WF_CSSPROP_HASH;
            y--;
            if (y <= x) return WF_CSSPROP_BAD;
        }
        /* "<length> +" and "a b" fail below: a type name is compared whole
         * and an ident has no space */
        alt[nalt].mult = (unsigned char)m;
        alt[nalt].ident[0] = 0;
        if (s[x] == '<') {
            t = cp_type_name(s + x, y - x);
            if (t == WF_CSSPROP_BAD) return WF_CSSPROP_BAD;
            /* Lot 95: <transform-list> is already a list */
            if (t == WF_CSSPROP_TLIST && m != WF_CSSPROP_ONE) return WF_CSSPROP_BAD;
            alt[nalt].type = (unsigned char)t;
        } else {
            if (y - x >= WF_CSSPROP_IDENT || !cp_is_ident(s + x, y - x) || cp_reserved(s + x, y - x))
                return WF_CSSPROP_BAD;
            alt[nalt].type = WF_CSSPROP_KEYWORD;
            for (k = 0; k < y - x; k++) alt[nalt].ident[k] = cp_low(s[x + k]);
            alt[nalt].ident[k] = 0;
        }
        nalt++;
        if (e >= b) break;
        a = e + 1;
        if (a > b) return WF_CSSPROP_BAD;
        if (a == b) return WF_CSSPROP_BAD;                       /* a trailing '|' */
    }
    if (out) {
        out->nalt = (unsigned char)nalt;
        for (k = 0; k < nalt; k++) out->alt[k] = alt[k];
    }
    if (nalt == 1 && alt[0].mult == WF_CSSPROP_ONE && alt[0].type != WF_CSSPROP_KEYWORD) return alt[0].type;
    return WF_CSSPROP_COMPLEX;
}

int wf_cssprop_syntax(const char *s, int n) {
    wf_cssprop_t t;
    return wf_cssprop_syntax_parse(s, n, &t);
}

/* a number at s[*i]: sign, digits, one optional fraction; *frac 1 when it
 * had a '.'; 0 when there is none */
static int cp_number(const char *s, int *i, int *frac) {
    int k = *i, digits = 0;
    *frac = 0;
    if (s[k] == '+' || s[k] == '-') k++;
    while (cp_digit(s[k])) { k++; digits++; }
    if (s[k] == '.') {
        int f = 0;
        k++;
        while (cp_digit(s[k])) { k++; f++; }
        if (!f) return 0;
        digits += f;
        *frac = 1;
    }
    if (!digits) return 0;
    *i = k;
    return 1;
}

static int cp_unit(const char *u, int n) {
    static const char *const units[] = {"px", "em", "rem", "ex", "ch", "vw", "vh", "vmin", "vmax",
                                        "pt", "pc", "cm", "mm", "in", "q", 0};
    int k;
    for (k = 0; units[k]; k++) if (cp_eqn(u, n, units[k])) return 1;
    return 0;
}

/* Lot 91: the type a unit gives: length, angle, time, Lot 95 resolution,
 * 0 unknown */
static int cp_unit_type(const char *u, int n) {
    if (cp_unit(u, n)) return WF_CSSPROP_LENGTH;
    if (cp_eqn(u, n, "dpi") || cp_eqn(u, n, "dpcm") || cp_eqn(u, n, "dppx") || cp_eqn(u, n, "x"))
        return WF_CSSPROP_RESOLUTION;
    if (cp_eqn(u, n, "deg") || cp_eqn(u, n, "grad") || cp_eqn(u, n, "rad") || cp_eqn(u, n, "turn"))
        return WF_CSSPROP_ANGLE;
    if (cp_eqn(u, n, "s") || cp_eqn(u, n, "ms")) return WF_CSSPROP_TIME;
    return 0;
}

/* ---- Lot 91/92: math functions with their types and values ----------- */

/* Lot 92: a value is kept in 1/10000 of its canonical unit (px, deg, s,
 * %, or a plain number); every operation rounds half away from zero and
 * any value past CP_LIM (100000 units) is not simplified. 64-bit
 * products, and a shift-subtract division (no __divdi3 in the guest). */
#define CP_S 10000LL
#define CP_LIM 1000000000LL

enum {
    CP_F_CALC = 1, CP_F_MIN, CP_F_MAX, CP_F_CLAMP, CP_F_ABS, CP_F_SIGN, CP_F_ROUND, CP_F_MOD, CP_F_REM,
    CP_F_SIN, CP_F_COS, CP_F_TAN, CP_F_ASIN, CP_F_ACOS, CP_F_ATAN, CP_F_ATAN2,
    /* Lot 93 */
    CP_F_POW, CP_F_SQRT, CP_F_HYPOT, CP_F_LOG, CP_F_EXP
};

typedef struct { const char *s; int i, depth; } cp_mp_t;
/* t: the type (0 invalid); ok: the value is known (v). Lot 95: a
 * length mixed with a percentage (t WF_CSSPROP_LENPCT) is known as v px
 * plus p % */
typedef struct { int t, ok; long long v, p; } cp_q_t;

/* Lot 95: the viewport of the value being computed (vw, vh, vmin, vmax);
 * 0 outside wf_cssprop_compute_ctx */
static const wf_cssprop_ctx_t *cp_ctx;
/* Lot 97: the font context (em, rem, ex, ch); 0 outside wf_cssprop_compute_font */
static const wf_cssprop_fctx_t *cp_fctx;

static const char *const cp_fn_names[] = {"calc", "min", "max", "clamp", "abs", "sign", "round", "mod", "rem",
                                          "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
                                          "pow", "sqrt", "hypot", "log", "exp", 0};

/* the math function starting at s (its code), *len the bytes of its name
 * and '('; 0 when s does not start one. Lot 91 had calc, min, max and
 * clamp; Lot 92 adds abs, sign, round, mod, rem and the trigonometric
 * functions; Lot 93 pow, sqrt, hypot, log and exp. */
static int cp_mfn_len(const char *s, int *len) {
    int k, j;
    for (k = 0; cp_fn_names[k]; k++) {
        const char *w = cp_fn_names[k];
        for (j = 0; w[j] && cp_low(s[j]) == w[j]; j++) {}
        if (!w[j] && s[j] == '(') { *len = j + 1; return k + 1; }
    }
    return 0;
}

static int cp_mfn(const char *s) { int l; return cp_mfn_len(s, &l); }

/* + and -: the same type, or a length with a percentage */
static int cp_tadd(int a, int b) {
    int la, lb;
    if (!a || !b) return 0;
    if (a == b) return a;
    la = a == WF_CSSPROP_LENGTH || a == WF_CSSPROP_PERCENT || a == WF_CSSPROP_LENPCT;
    lb = b == WF_CSSPROP_LENGTH || b == WF_CSSPROP_PERCENT || b == WF_CSSPROP_LENPCT;
    return la && lb ? WF_CSSPROP_LENPCT : 0;
}

static unsigned long long cp_udiv(unsigned long long n, unsigned long long d, unsigned long long *rem) {
    unsigned long long q = 0, r = 0;
    int b;
    for (b = 63; b >= 0; b--) {
        r = (r << 1) | ((n >> b) & 1ULL);
        if (r >= d) { r -= d; q |= 1ULL << b; }
    }
    *rem = r;
    return q;
}

/* n / d: mode 0 rounded half away from zero, 1 floor, 2 toward zero */
static long long cp_div(long long n, long long d, int mode) {
    int neg = (n < 0) != (d < 0);
    unsigned long long un = (unsigned long long)(n < 0 ? -n : n), ud = (unsigned long long)(d < 0 ? -d : d), r, q;
    q = cp_udiv(un, ud, &r);
    if (mode == 0 && 2 * r >= ud) q++;
    else if (mode == 1 && neg && r) q++;
    return neg ? -(long long)q : (long long)q;
}

static int cp_fits(long long v) { return v <= CP_LIM && v >= -CP_LIM; }

/* the value a math leaf "number[unit]" stands for: in canonical units */
static const struct { const char *u; int t; long long num, den; } cp_units[] = {
    {"px", WF_CSSPROP_LENGTH, 1, 1}, {"in", WF_CSSPROP_LENGTH, 96, 1}, {"cm", WF_CSSPROP_LENGTH, 4800, 127},
    {"mm", WF_CSSPROP_LENGTH, 480, 127}, {"q", WF_CSSPROP_LENGTH, 120, 127}, {"pt", WF_CSSPROP_LENGTH, 4, 3},
    {"pc", WF_CSSPROP_LENGTH, 16, 1},
    /* relative lengths: typed, not simplified (no font or viewport here) */
    {"em", WF_CSSPROP_LENGTH, 0, 1}, {"rem", WF_CSSPROP_LENGTH, 0, 1}, {"ex", WF_CSSPROP_LENGTH, 0, 1},
    {"ch", WF_CSSPROP_LENGTH, 0, 1}, {"vw", WF_CSSPROP_LENGTH, 0, 1}, {"vh", WF_CSSPROP_LENGTH, 0, 1},
    {"vmin", WF_CSSPROP_LENGTH, 0, 1}, {"vmax", WF_CSSPROP_LENGTH, 0, 1},
    {"deg", WF_CSSPROP_ANGLE, 1, 1}, {"grad", WF_CSSPROP_ANGLE, 9, 10}, {"turn", WF_CSSPROP_ANGLE, 360, 1},
    /* 180 / pi with pi to eight significant digits (3.1415927) */
    {"rad", WF_CSSPROP_ANGLE, 1800000000LL, 31415927LL},
    {"s", WF_CSSPROP_TIME, 1, 1}, {"ms", WF_CSSPROP_TIME, 1, 1000},
    /* Lot 95: resolutions in dppx (96dpi = 1dppx, 1dpcm = 2.54dpi) */
    {"dppx", WF_CSSPROP_RESOLUTION, 1, 1}, {"x", WF_CSSPROP_RESOLUTION, 1, 1},
    {"dpi", WF_CSSPROP_RESOLUTION, 1, 96}, {"dpcm", WF_CSSPROP_RESOLUTION, 254, 9600}, {0, 0, 0, 0}};

/* Lot 95: the px of 1vw, 1vh, 1vmin or 1vmax times 100 (0 not known) */
static long long cp_vp(const char *u, int n) {
    int w, h;
    if (!cp_ctx || cp_ctx->vw <= 0 || cp_ctx->vh <= 0) return 0;
    w = cp_ctx->vw; h = cp_ctx->vh;
    if (cp_eqn(u, n, "vw")) return w;
    if (cp_eqn(u, n, "vh")) return h;
    if (cp_eqn(u, n, "vmin")) return w < h ? w : h;
    if (cp_eqn(u, n, "vmax")) return w > h ? w : h;
    return 0;
}

static void cp_ws(cp_mp_t *m) { while (cp_sp(m->s[m->i])) m->i++; }

static void cp_msum(cp_mp_t *m, cp_q_t *q);

/* a number with its unit at m->i; q->t 0 when it is not one */
static void cp_mleaf(cp_mp_t *m, cp_q_t *q) {
    const char *s = m->s;
    int i = m->i, frac, a, k, neg = 0, fd = 0;
    long long raw = 0;
    q->t = 0; q->ok = 1; q->v = 0; q->p = 0;
    if (!cp_number(s, &i, &frac)) return;
    k = m->i;
    if (s[k] == '+' || s[k] == '-') { neg = s[k] == '-'; k++; }
    for (; k < i && s[k] != '.'; k++) {
        if (raw > CP_LIM) q->ok = 0;
        else raw = raw * 10 + (s[k] - '0');
    }
    raw *= CP_S;
    if (k < i) {
        int place = 1000;
        for (k++; k < i; k++, fd++) {
            if (fd < 4) { raw += (s[k] - '0') * place; place /= 10; }
            else if (fd == 4 && s[k] >= '5') raw++;   /* half away from zero at the fifth digit */
        }
    }
    if (raw > CP_LIM) q->ok = 0;
    if (neg) raw = -raw;
    if (s[i] == '%') { m->i = i + 1; q->t = WF_CSSPROP_PERCENT; q->v = raw; return; }
    a = i;
    while ((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z')) i++;
    m->i = i;
    if (i == a) { q->t = WF_CSSPROP_NUMBER; q->v = raw; return; }
    for (k = 0; cp_units[k].u; k++) {
        if (!cp_eqn(s + a, i - a, cp_units[k].u)) continue;
        q->t = cp_units[k].t;
        if (!cp_units[k].num) {
            /* Lot 95: viewport units with the context's viewport */
            long long base = cp_vp(s + a, i - a);
            /* Lot 97: em, rem, ex and ch with the font context (px
             * thousandths). Lot 98: ex and ch from the engine's font
             * metrics (WF_CSSPROP_EX_*, WF_CSSPROP_CH_*) */
            if (cp_fctx && (cp_eqn(s + a, i - a, "em") || cp_eqn(s + a, i - a, "rem") ||
                            cp_eqn(s + a, i - a, "ex") || cp_eqn(s + a, i - a, "ch"))) {
                long long fs = cp_eqn(s + a, i - a, "rem") ? cp_fctx->rem : cp_fctx->em;
                long long num = 1, den = 1000;
                if (cp_eqn(s + a, i - a, "ex")) { num = WF_CSSPROP_EX_NUM; den = 1000 * WF_CSSPROP_EX_DEN; }
                else if (cp_eqn(s + a, i - a, "ch")) { num = WF_CSSPROP_CH_NUM; den = 1000 * WF_CSSPROP_CH_DEN; }
                /* fs > 0: the reader keeps a font size of at least 1px */
                if (q->ok) q->v = cp_div(raw * fs * num, den, 0);
                if (!cp_fits(q->v)) q->ok = 0;
                return;
            }
            if (!base) { q->ok = 0; return; }
            if (q->ok) q->v = cp_div(raw * base, 100, 0);
            if (!cp_fits(q->v)) q->ok = 0;
            return;
        }
        if (q->ok) q->v = cp_div(raw * cp_units[k].num, cp_units[k].den, 0);
        if (!cp_fits(q->v)) q->ok = 0;
        return;
    }
}

/* the value is known. A length mixed with a percentage is typed but
 * never known: no leaf is a length-percentage, and + - min() max() and
 * clamp() keep ok only when both sides have the same type. */
static int cp_known(const cp_q_t *q) { return q->ok; }

/* Lot 95: the px and % parts of a length, a percentage or a mix */
static void cp_parts(const cp_q_t *q, long long *px, long long *pc) {
    *px = q->t == WF_CSSPROP_PERCENT ? 0 : q->v;
    *pc = q->t == WF_CSSPROP_PERCENT ? q->v : q->t == WF_CSSPROP_LENPCT ? q->p : 0;
}

/* a + b (sub: a - b) into a. Lot 95: a length and a percentage give a
 * known mix (Lot 92: not known) */
static void cp_qadd(cp_q_t *a, const cp_q_t *b, int sub) {
    int t = cp_tadd(a->t, b->t);
    if (t == WF_CSSPROP_LENPCT) {
        long long ax, ap, bx, bp;
        cp_parts(a, &ax, &ap);
        cp_parts(b, &bx, &bp);
        a->ok = a->ok && b->ok;
        a->t = t;
        a->v = sub ? ax - bx : ax + bx;
        a->p = sub ? ap - bp : ap + bp;
        if (!cp_fits(a->v) || !cp_fits(a->p)) a->ok = 0;
        return;
    }
    a->ok = a->ok && b->ok && t == a->t && t == b->t;
    a->t = t;
    if (a->ok) a->v = sub ? a->v - b->v : a->v + b->v;
    if (!cp_fits(a->v)) a->ok = 0;
}

/* min (want -1), max (want 1) of two values into a */
static void cp_qpick(cp_q_t *a, const cp_q_t *b, int want) {
    int t = cp_tadd(a->t, b->t);
    a->ok = a->ok && b->ok && t == a->t && t == b->t;
    a->t = t;
    if (a->ok && ((want < 0 && b->v < a->v) || (want > 0 && b->v > a->v))) a->v = b->v;
}

/* the round() strategy word at m->i: 1 nearest, 2 up, 3 down, 4 to-zero,
 * 0 none (m->i not moved) */
static int cp_strategy(cp_mp_t *m) {
    static const char *const w[] = {"nearest", "up", "down", "to-zero", 0};
    int k, j = m->i, e;
    while (cp_ident(m->s[j])) j++;
    for (e = j; cp_sp(m->s[e]); e++) {}
    if (j == m->i || m->s[e] != ',') return 0;
    for (k = 0; w[k]; k++) if (cp_eqn(m->s + m->i, j - m->i, w[k])) { m->i = e + 1; return k + 1; }
    return 0;
}

/* round(): A rounded to a multiple of |B| */
static void cp_round(cp_q_t *a, const cp_q_t *b, int how) {
    long long step, k;
    if (!cp_known(a) || !b->ok || a->t != b->t || b->v == 0) { a->ok = 0; return; }
    step = b->v < 0 ? -b->v : b->v;
    if (how == 2) k = -cp_div(-a->v, step, 1);
    else if (how == 3) k = cp_div(a->v, step, 1);
    else if (how == 4) k = cp_div(a->v, step, 2);
    else k = cp_div(2 * a->v + step, 2 * step, 1);   /* half toward +infinity */
    a->v = k * step;
    if (!cp_fits(a->v)) a->ok = 0;
}

/* ---- Lot 93: 128-bit integers and 62-bit fixed point ------------------
 * The trigonometric functions, exp(), log() and pow() with an exponent
 * that is not a small integer work on values in 1/2^62 (CP_ONE), with
 * 128-bit intermediates made of two 64-bit words, and round the result
 * half away from zero to 1/10000 at the end. No libm, no 64-bit division
 * operator (shift-subtract, as cp_udiv). sqrt(), hypot() and pow() with
 * a small integer exponent are exact integer computations. */
typedef struct { unsigned long long hi, lo; } cp_w_t;   /* two's complement */

#define CP_ONE (1LL << 62)
/* pi/2, pi/4, tan(pi/8) and ln 2 in 1/2^62, rounded to nearest */
#define CP_PIO2 7244019458077122842LL
#define CP_PIO4 3622009729038561421LL
#define CP_TANPI8 1910222894239003202LL
#define CP_LN2 3196577161300663915LL

static cp_w_t cp_w(long long v) {
    cp_w_t r;
    r.lo = (unsigned long long)v;
    r.hi = v < 0 ? ~0ULL : 0;
    return r;
}

static int cp_wneg_p(cp_w_t a) { return (int)(a.hi >> 63); }

static cp_w_t cp_wneg(cp_w_t a) {
    a.lo = ~a.lo + 1;
    a.hi = ~a.hi + (a.lo == 0);
    return a;
}

static cp_w_t cp_wadd(cp_w_t a, cp_w_t b) {
    cp_w_t r;
    r.lo = a.lo + b.lo;
    r.hi = a.hi + b.hi + (r.lo < a.lo);
    return r;
}

static cp_w_t cp_wsub(cp_w_t a, cp_w_t b) { return cp_wadd(a, cp_wneg(b)); }

/* the value fits a signed 64-bit word */
static int cp_wsmall(cp_w_t a) { return a.hi == (a.lo >> 63 ? ~0ULL : 0); }

/* a < b */
static int cp_wlt(cp_w_t a, cp_w_t b) { return cp_wneg_p(cp_wsub(a, b)); }

/* unsigned 64 x 64 -> 128 bits, from four 32 x 32 products */
static cp_w_t cp_umul(unsigned long long a, unsigned long long b) {
    unsigned long long a0 = a & 0xffffffffULL, a1 = a >> 32, b0 = b & 0xffffffffULL, b1 = b >> 32;
    unsigned long long p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0, p11 = a1 * b1, mid;
    cp_w_t r;
    mid = (p00 >> 32) + (p01 & 0xffffffffULL) + (p10 & 0xffffffffULL);
    r.lo = (p00 & 0xffffffffULL) | (mid << 32);
    r.hi = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
    return r;
}

/* a * m, the low 128 bits (callers keep the product within 127 bits) */
static cp_w_t cp_wmul(cp_w_t a, long long m) {
    int neg = cp_wneg_p(a) != (m < 0);
    unsigned long long um = (unsigned long long)(m < 0 ? -m : m);
    cp_w_t r;
    if (cp_wneg_p(a)) a = cp_wneg(a);
    r = cp_umul(a.lo, um);
    r.hi += a.hi * um;
    return neg ? cp_wneg(r) : r;
}

/* a / d for 0 < d <= 2^63: mode 0 rounded half away from zero, 1 truncated */
static cp_w_t cp_wdivu(cp_w_t a, unsigned long long d, int mode) {
    int neg = cp_wneg_p(a), b;
    unsigned long long r = 0;
    cp_w_t q;
    q.hi = 0; q.lo = 0;
    if (neg) a = cp_wneg(a);
    for (b = 127; b >= 0; b--) {
        unsigned long long bit = b >= 64 ? (a.hi >> (b - 64)) & 1ULL : (a.lo >> b) & 1ULL;
        r = (r << 1) | bit;
        if (r >= d) {
            r -= d;
            if (b >= 64) q.hi |= 1ULL << (b - 64);
            else q.lo |= 1ULL << b;
        }
    }
    if (mode == 0 && r >= d - r) {
        q.lo++;
        if (!q.lo) q.hi++;
    }
    return neg ? cp_wneg(q) : q;
}

/* a / d, d a signed 64-bit word other than 0 (mode as cp_wdivu) */
static cp_w_t cp_wdiv(cp_w_t a, long long d, int mode) {
    if (d < 0) { a = cp_wneg(a); d = -d; }
    return cp_wdivu(a, (unsigned long long)d, mode);
}

/* a / 2^sh rounded half away from zero, 0 < sh < 128 */
static cp_w_t cp_wshr(cp_w_t a, int sh) {
    int neg = cp_wneg_p(a);
    cp_w_t h;
    if (neg) a = cp_wneg(a);
    h.hi = sh - 1 >= 64 ? 1ULL << (sh - 1 - 64) : 0;
    h.lo = sh - 1 < 64 ? 1ULL << (sh - 1) : 0;
    a = cp_wadd(a, h);
    if (sh >= 64) { a.lo = a.hi >> (sh - 64); a.hi = 0; }
    else { a.lo = (a.lo >> sh) | (a.hi << (64 - sh)); a.hi >>= sh; }
    return neg ? cp_wneg(a) : a;
}

/* a * 2^sh for a >= 0, 0 < sh < 128 (no bit lost) */
static cp_w_t cp_wshl(cp_w_t a, int sh) {
    if (sh >= 64) { a.hi = a.lo << (sh - 64); a.lo = 0; }
    else { a.hi = (a.hi << sh) | (a.lo >> (64 - sh)); a.lo <<= sh; }
    return a;
}

/* fixed point: a * b and a / b in 1/2^62 */
static long long cp_xmul(long long a, long long b) { return (long long)cp_wshr(cp_wmul(cp_w(a), b), 62).lo; }

static long long cp_xdiv(long long a, unsigned long long b) {
    return (long long)cp_wdivu(cp_wmul(cp_w(a), CP_ONE), b, 0).lo;
}

/* a / 2 for a >= 0 */
static cp_w_t cp_whalf(cp_w_t a) {
    a.lo = (a.lo >> 1) | (a.hi << 63);
    a.hi >>= 1;
    return a;
}

/* n / d for n >= 0 and d > 0, both 128-bit: rounded half away from zero */
static cp_w_t cp_wdivw(cp_w_t n, cp_w_t d) {
    cp_w_t q, r;
    int b;
    q.hi = 0; q.lo = 0; r.hi = 0; r.lo = 0;
    for (b = 127; b >= 0; b--) {
        unsigned long long bit = b >= 64 ? (n.hi >> (b - 64)) & 1ULL : (n.lo >> b) & 1ULL;
        r.hi = (r.hi << 1) | (r.lo >> 63);
        r.lo = (r.lo << 1) | bit;
        if (!cp_wlt(r, d)) {
            r = cp_wsub(r, d);
            if (b >= 64) q.hi |= 1ULL << (b - 64);
            else q.lo |= 1ULL << b;
        }
    }
    if (!cp_wlt(r, cp_wsub(d, r))) { q.lo++; if (!q.lo) q.hi++; }
    return q;
}

/* 1 and *out when a fits a 64-bit word and the 100000-unit limit */
static int cp_wout(cp_w_t a, long long *out) {
    if (!cp_wsmall(a) || !cp_fits((long long)a.lo)) return 0;
    *out = (long long)a.lo;
    return 1;
}

/* the floor of the square root of a >= 0 */
static unsigned long long cp_isqrt(cp_w_t n) {
    cp_w_t res, bit, t;
    res.hi = 0; res.lo = 0;
    bit.hi = 1ULL << 62; bit.lo = 0;
    while (bit.hi || bit.lo) {
        t = cp_wadd(res, bit);
        res = cp_whalf(res);
        if (!cp_wlt(n, t)) { n = cp_wsub(n, t); res = cp_wadd(res, bit); }
        bit.lo = (bit.lo >> 2) | (bit.hi << 62);
        bit.hi >>= 2;
    }
    return res.lo;
}

/* sin r and cos r for |r| <= pi/4 (and a little more) */
static void cp_sincos(long long r, long long *s, long long *c) {
    long long r2 = cp_xmul(r, r), ts = r, tc = CP_ONE;
    int k;
    *s = r; *c = CP_ONE;
    for (k = 1; k <= 14; k++) {
        ts = -cp_div(cp_xmul(ts, r2), (long long)(2 * k) * (2 * k + 1), 0);
        tc = -cp_div(cp_xmul(tc, r2), (long long)(2 * k - 1) * (2 * k), 0);
        *s += ts;
        *c += tc;
    }
}

/* the odd series sum z^(2k+1) / (2k+1), with the sign alternating when alt
 * (atan) or not (atanh), |z| <= 0.42 */
static long long cp_oddser(long long z, int alt) {
    long long z2 = cp_xmul(z, z), p = z, sum = z;
    int k;
    for (k = 1; k <= 40 && p; k++) {
        p = cp_xmul(p, z2);
        if (alt) p = -p;
        sum += cp_div(p, 2 * k + 1, 0);
    }
    return sum;
}

/* atan z for 0 <= z <= 1 */
static long long cp_atan01(long long z) {
    if (z <= CP_TANPI8) return cp_oddser(z, 1);
    /* atan z = pi/4 + atan((z - 1) / (z + 1)) */
    return CP_PIO4 + cp_oddser((long long)cp_wdivu(cp_wmul(cp_w(z - CP_ONE), CP_ONE),
                                                   (unsigned long long)z + (unsigned long long)CP_ONE, 0).lo, 1);
}

/* atan2(y, x) in 1/10000 deg */
static long long cp_atan2(long long y, long long x) {
    unsigned long long ax = (unsigned long long)(x < 0 ? -x : x), ay = (unsigned long long)(y < 0 ? -y : y);
    long long a;
    cp_w_t w;
    if (!ax && !ay) return 0;
    if (ay <= ax) a = cp_atan01(cp_xdiv((long long)ay, ax));
    else a = CP_PIO2 - cp_atan01(cp_xdiv((long long)ax, ay));
    w = cp_w(a);
    if (x < 0) w = cp_wsub(cp_wadd(cp_w(CP_PIO2), cp_w(CP_PIO2)), w);
    if (y < 0) w = cp_wneg(w);
    return (long long)cp_wdiv(cp_wmul(w, 900000), CP_PIO2, 0).lo;
}

/* sin, cos or tan of v (1/10000 deg when ang, else 1/10000 rad, at
 * most 100 rad) into *out (1/10000); 0 when not known (a pole of tan) */
static int cp_trig(int fn, int ang, long long v, long long *out) {
    long long r, s, c, num, den;
    int q;
    if (ang) {
        long long t = v - 3600000LL * cp_div(v, 3600000LL, 1), d;
        long long k = cp_div(t + 450000, 900000, 1);
        d = t - k * 900000;
        q = (int)(k & 3);
        r = (long long)cp_wdiv(cp_wmul(cp_w(d), CP_PIO2), 900000, 0).lo;
    } else {
        cp_w_t x, k;
        if (v > 1000000 || v < -1000000) return 0;
        x = cp_wdiv(cp_wmul(cp_w(v), CP_ONE), 10000, 0);
        k = cp_wdiv(x, CP_PIO2, 0);
        x = cp_wsub(x, cp_wmul(cp_w(CP_PIO2), (long long)k.lo));
        r = (long long)x.lo;
        q = (int)(k.lo & 3);
    }
    cp_sincos(r, &s, &c);
    if (fn == CP_F_TAN) {
        if (q & 1) { num = -c; den = s; }
        else { num = s; den = c; }
        if (!den) return 0;
        return cp_wout(cp_wdiv(cp_wmul(cp_w(num), 10000), den, 0), out);
    }
    if (fn == CP_F_COS) q = (q + 1) & 3;
    num = q == 0 ? s : q == 1 ? c : q == 2 ? -s : -c;
    *out = (long long)cp_wshr(cp_wmul(cp_w(num), 10000), 62).lo;
    return 1;
}

/* ln(v / 10000) for v > 0, in 1/2^62 */
static cp_w_t cp_ln(long long v) {
    long long m, s;
    int k = 0;
    cp_w_t r;
    /* v / 10000 = m 2^k with m in (1/2, 1] */
    while (v > (10000LL << k)) k++;
    while (k <= 0 && (v << (1 - k)) <= 10000) k--;
    m = (long long)cp_wdivu(cp_wshl(cp_w(v), 62 - k), 10000, 0).lo;
    /* ln m = 2 atanh((m - 1) / (m + 1)) */
    s = (long long)cp_wdivu(cp_wmul(cp_w(m - CP_ONE), CP_ONE), (unsigned long long)m + (unsigned long long)CP_ONE, 0).lo;
    r = cp_wmul(cp_w(CP_LN2), k);
    return cp_wadd(r, cp_w(2 * cp_oddser(s, 0)));
}

/* exp(t), t in 1/2^62, into *out (1/10000); 0 when past the limit */
static int cp_exp(cp_w_t t, long long *out) {
    long long r, e, term;
    int k, j;
    if (cp_wlt(cp_wmul(cp_w(CP_ONE), 12), t)) return 0;
    if (cp_wlt(t, cp_wmul(cp_w(CP_ONE), -12))) { *out = 0; return 1; }
    /* t = k ln 2 + r, |r| <= ln 2 / 2 */
    k = (int)(long long)cp_wdiv(t, CP_LN2, 0).lo;
    t = cp_wsub(t, cp_wmul(cp_w(CP_LN2), k));
    r = (long long)t.lo;
    e = CP_ONE; term = CP_ONE;
    for (j = 1; j <= 30 && term; j++) {
        term = cp_div(cp_xmul(term, r), j, 0);
        e += term;
    }
    /* at most e^12 = 162755 (1.7e9): cp_margs refuses it past the limit */
    *out = (long long)cp_wshr(cp_wmul(cp_w(e), 10000), 62 - k).lo;
    return 1;
}

/* sqrt of n (1/10^8 units squared) into 1/10000, rounded half away */
static int cp_sqrtw(cp_w_t n, long long *out) {
    unsigned long long s = cp_isqrt(n);
    cp_w_t rem = cp_wsub(n, cp_umul(s, s));
    if (cp_wlt(cp_w(0), cp_wsub(rem, cp_umul(s, 1)))) s++;
    *out = (long long)s;            /* past the limit: cp_margs refuses it */
    return 1;
}

/* pow(a, b), both in 1/10000, into *out; 0 when not known */
static int cp_pow(long long a, long long b, long long *out) {
    unsigned long long r;
    long long n, mag;
    int neg = 0, k;
    unsigned long long ua = (unsigned long long)(a < 0 ? -a : a);
    cp_w_t p;
    n = (long long)cp_udiv((unsigned long long)(b < 0 ? -b : b), 10000, &r);
    if (b < 0) n = -n;
    if (!r) {
        /* an integer exponent */
        if (!n) { *out = CP_S; return 1; }
        if (!ua) {
            if (n < 0) return 0;          /* infinity */
            *out = 0;
            return 1;
        }
        neg = a < 0 && (n & 1);
        if (n > 0 && n <= 127) {
            /* a^n / 10000^(n - 1), exact while a^n fits 127 bits */
            p = cp_w(1);
            for (k = 0; k < n && !(p.hi >> 32); k++) p = cp_wmul(p, (long long)ua);
            if (k == n) {
                int d = 4 * ((int)n - 1);
                if (d) {
                    for (k = 0; k < d - 1; k++) p = cp_wdivu(p, 10, 1);
                    p = cp_wdivu(cp_wadd(p, cp_w(5)), 10, 1);
                }
                if (!cp_wout(p, &mag)) return 0;
                *out = neg ? -mag : mag;
                return 1;
            }
        } else if (n < 0 && n >= -8) {
            /* 10000^(1 - n) / a^-n, exact while a^-n fits 127 bits */
            cp_w_t num = cp_w(1);
            p = cp_w(1);
            for (k = 0; k < -n && !(p.hi >> 32); k++) p = cp_wmul(p, (long long)ua);
            if (k == -n) {
                for (k = 0; k < 4 * (1 - (int)n); k++) num = cp_wmul(num, 10);
                if (!cp_wout(cp_wdivw(num, p), &mag)) return 0;
                *out = neg ? -mag : mag;
                return 1;
            }
        }
    } else if (a < 0) {
        return 0;                         /* NaN */
    } else if (!ua) {
        if (b < 0) return 0;
        *out = 0;
        return 1;
    }
    /* exp(b ln |a|) */
    if (!cp_exp(cp_wdiv(cp_wmul(cp_ln((long long)ua), b), 10000, 0), &mag)) return 0;
    *out = neg ? -mag : mag;
    return 1;
}

/* log(a) or log(a, b) (hasb), in 1/10000, into *out */
static int cp_log(long long a, int hasb, long long b, long long *out) {
    cp_w_t la, lb;
    if (a <= 0 || (hasb && b <= 0)) return 0;      /* -infinity or NaN */
    la = cp_ln(a);
    if (!hasb) return cp_wout(cp_wshr(cp_wmul(la, 10000), 62), out);
    lb = cp_ln(b);                                 /* exactly 0 for b = 1 */
    if (!lb.hi && !lb.lo) return 0;                /* a division by zero */
    la = cp_wmul(la, 10000);
    if (!cp_wsmall(lb)) { lb = cp_wshr(lb, 4); la = cp_wshr(la, 4); }
    return cp_wout(cp_wdiv(la, (long long)lb.lo, 0), out);
}

/* the arguments of math function fn after its '(' into q */
static void cp_margs(cp_mp_t *m, int fn, cp_q_t *q) {
    cp_q_t arg[3], u;
    int n = 0, how = 0, t = 0, hok = 1;
    cp_w_t hsum = cp_w(0);
    long long r = 0;
    cp_ws(m);
    if (fn == CP_F_ROUND) how = cp_strategy(m);
    for (;;) {
        cp_msum(m, &u);
        cp_ws(m);
        if (!u.t) { q->t = 0; return; }
        /* Lot 95: only calc() passes a mix on; the other functions of a
         * mix are not known */
        if (u.t == WF_CSSPROP_LENPCT && fn != CP_F_CALC) u.ok = 0;
        if (n < 3) arg[n] = u;
        /* Lot 93: hypot() sums the squares of values of one type */
        if (!u.ok || (n && u.t != arg[0].t)) hok = 0;
        else hsum = cp_wadd(hsum, cp_umul((unsigned long long)(u.v < 0 ? -u.v : u.v),
                                          (unsigned long long)(u.v < 0 ? -u.v : u.v)));
        n++;
        if (n == 1) { *q = u; t = u.t; }
        else {
            t = cp_tadd(t, u.t);
            if (!t) { q->t = 0; return; }
            if (fn == CP_F_MIN || fn == CP_F_MAX) cp_qpick(q, &u, fn == CP_F_MIN ? -1 : 1);
        }
        if (m->s[m->i] != ',' || fn == CP_F_CALC) break;
        m->i++;
        cp_ws(m);
    }
    if (m->s[m->i] != ')') { q->t = 0; return; }
    m->i++;
    switch (fn) {
    case CP_F_CLAMP:
        if (n != 3) { q->t = 0; return; }
        /* clamp(MIN, VAL, MAX) = max(MIN, min(VAL, MAX)) */
        *q = arg[1];
        cp_qpick(q, &arg[2], -1);
        cp_qpick(q, &arg[0], 1);
        break;
    case CP_F_ABS:
        if (n != 1) { q->t = 0; return; }
        if (q->v < 0) q->v = -q->v;
        break;
    case CP_F_SIGN:
        if (n != 1) { q->t = 0; return; }
        q->ok = cp_known(q);
        q->v = q->v > 0 ? CP_S : q->v < 0 ? -CP_S : 0;
        t = WF_CSSPROP_NUMBER;
        break;
    case CP_F_ROUND:
        if (n > 2 || (n == 1 && t != WF_CSSPROP_NUMBER)) { q->t = 0; return; }
        if (n == 1) {
            cp_q_t one;
            one.t = WF_CSSPROP_NUMBER; one.ok = 1; one.v = CP_S; one.p = 0;
            cp_round(q, &one, how);
        } else {
            *q = arg[0];
            cp_round(q, &arg[1], how);
        }
        break;
    case CP_F_MOD: case CP_F_REM:
        if (n != 2) { q->t = 0; return; }
        *q = arg[0];
        /* mod takes the sign of B (floor), rem the sign of A (truncation) */
        if (cp_known(q) && arg[1].ok && q->t == arg[1].t && arg[1].v != 0)
            q->v -= arg[1].v * cp_div(q->v, arg[1].v, fn == CP_F_MOD ? 1 : 2);
        else q->ok = 0;
        break;
    case CP_F_SIN: case CP_F_COS: case CP_F_TAN:
        /* Lot 93: simplified (Lot 92 typed them only) */
        if (n != 1 || (t != WF_CSSPROP_NUMBER && t != WF_CSSPROP_ANGLE)) { q->t = 0; return; }
        q->ok = q->ok && cp_trig(fn, t == WF_CSSPROP_ANGLE, q->v, &r);
        q->v = r;
        t = WF_CSSPROP_NUMBER;
        break;
    case CP_F_ASIN: case CP_F_ACOS: case CP_F_ATAN:
        if (n != 1 || t != WF_CSSPROP_NUMBER) { q->t = 0; return; }
        t = WF_CSSPROP_ANGLE;
        if (fn == CP_F_ATAN) { if (q->ok) q->v = cp_atan2(q->v, CP_S); break; }
        if (q->ok && q->v <= CP_S && q->v >= -CP_S) {
            /* asin x = atan2(x, sqrt(1 - x^2)), acos x = atan2(sqrt(1 - x^2), x);
             * both scaled by 10000 * 2^48 */
            long long y = q->v * (1LL << 48), c = (long long)cp_isqrt(cp_wshl(cp_w(CP_S * CP_S - q->v * q->v), 96));
            q->v = fn == CP_F_ASIN ? cp_atan2(y, c) : cp_atan2(c, y);
        } else q->ok = 0;                 /* NaN past [-1, 1] */
        break;
    case CP_F_ATAN2:
        if (n != 2) { q->t = 0; return; }
        q->ok = q->ok && arg[1].ok && arg[0].t == arg[1].t;
        if (q->ok) q->v = cp_atan2(q->v, arg[1].v);
        t = WF_CSSPROP_ANGLE;
        break;
    /* Lot 93: pow(), sqrt(), log() and exp() take numbers; hypot() values
     * of one type (its result) */
    case CP_F_POW:
        if (n != 2 || t != WF_CSSPROP_NUMBER) { q->t = 0; return; }
        q->ok = q->ok && arg[1].ok && cp_pow(q->v, arg[1].v, &r);
        q->v = r;
        break;
    case CP_F_SQRT:
        if (n != 1 || t != WF_CSSPROP_NUMBER) { q->t = 0; return; }
        q->ok = q->ok && q->v >= 0 && cp_sqrtw(cp_wmul(cp_w(q->v), CP_S), &r);
        q->v = r;
        break;
    case CP_F_HYPOT:
        q->ok = hok && cp_sqrtw(hsum, &r);
        q->v = r;
        break;
    case CP_F_LOG:
        if (n > 2 || t != WF_CSSPROP_NUMBER) { q->t = 0; return; }
        q->ok = q->ok && (n == 1 || arg[1].ok) && cp_log(q->v, n == 2, n == 2 ? arg[1].v : 0, &r);
        q->v = r;
        break;
    case CP_F_EXP:
        if (n != 1 || t != WF_CSSPROP_NUMBER) { q->t = 0; return; }
        q->ok = q->ok && cp_exp(cp_wdiv(cp_wmul(cp_w(q->v), CP_ONE), 10000, 0), &r);
        q->v = r;
        break;
    default: break;
    }
    q->t = t;
    if (!cp_fits(q->v)) q->ok = 0;
}

/* Lot 93: the constants e and pi (in 1/10000, as every value here),
 * infinity, -infinity and nan (numbers, never known); 0 when m->i does not
 * start one (an identifier such as -pi or pi-1 is not one) */
static int cp_mconst(cp_mp_t *m, cp_q_t *q) {
    static const char *const w[] = {"e", "pi", "infinity", "-infinity", "nan", 0};
    int j = m->i, k;
    while (cp_ident(m->s[j])) j++;
    for (k = 0; w[k]; k++) {
        if (!cp_eqn(m->s + m->i, j - m->i, w[k])) continue;
        m->i = j;
        q->t = WF_CSSPROP_NUMBER;
        q->ok = k < 2;
        q->v = k == 0 ? 27183 : k == 1 ? 31416 : 0;
        return 1;
    }
    return 0;
}

static void cp_mvalue(cp_mp_t *m, cp_q_t *q) {
    int fn, len;
    q->t = 0; q->ok = 0; q->v = 0; q->p = 0;
    if (++m->depth > 16) return;
    cp_ws(m);
    if (m->s[m->i] == '(') {
        m->i++;
        cp_ws(m);
        cp_msum(m, q);
        cp_ws(m);
        if (!q->t || m->s[m->i] != ')') { q->t = 0; return; }
        m->i++;
        m->depth--;
        return;
    }
    fn = cp_mfn_len(m->s + m->i, &len);
    if (fn) {
        m->i += len;
        cp_margs(m, fn, q);
        m->depth--;
        return;
    }
    if (!cp_mconst(m, q)) cp_mleaf(m, q);
    m->depth--;
}

static void cp_mproduct(cp_mp_t *m, cp_q_t *q) {
    cp_mvalue(m, q);
    while (q->t) {
        int j = m->i;
        char op;
        cp_q_t u;
        while (cp_sp(m->s[j])) j++;
        op = m->s[j];
        if (op != '*' && op != '/') break;
        m->i = j + 1;
        cp_mvalue(m, &u);
        if (!u.t) { q->t = 0; return; }
        if (op == '*') {
            int t = q->t == WF_CSSPROP_NUMBER ? u.t : u.t == WF_CSSPROP_NUMBER ? q->t : 0;
            q->ok = q->ok && u.ok && t;
            if (q->ok) {
                /* Lot 95: a mix scales both parts */
                long long f = q->t == WF_CSSPROP_NUMBER ? q->v : u.v;
                const cp_q_t *x = q->t == WF_CSSPROP_NUMBER ? &u : q;
                long long xp = x->p;
                q->v = cp_div(x->v * f, CP_S, 0);
                q->p = t == WF_CSSPROP_LENPCT ? cp_div(xp * f, CP_S, 0) : 0;
            }
            q->t = t;
        } else {
            q->t = u.t == WF_CSSPROP_NUMBER ? q->t : 0;
            q->ok = q->ok && u.ok && u.v != 0;
            if (q->ok) {
                q->v = cp_div(q->v * CP_S, u.v, 0);
                if (q->t == WF_CSSPROP_LENPCT) q->p = cp_div(q->p * CP_S, u.v, 0);
            }
        }
        if (!cp_fits(q->v) || !cp_fits(q->p)) q->ok = 0;
    }
}

static void cp_msum(cp_mp_t *m, cp_q_t *q) {
    cp_mproduct(m, q);
    while (q->t) {
        int j = m->i, sub;
        cp_q_t u;
        if (!cp_sp(m->s[j])) break;
        while (cp_sp(m->s[j])) j++;
        if ((m->s[j] != '+' && m->s[j] != '-') || !cp_sp(m->s[j + 1])) break;
        sub = m->s[j] == '-';
        m->i = j + 1;
        cp_mproduct(m, &u);
        cp_qadd(q, &u, sub);
    }
}

/* the whole value as one math function: q (q.t 0 when it is not one) */
static void cp_mwhole(const char *v, cp_q_t *q) {
    cp_mp_t m;
    q->t = 0; q->ok = 0; q->v = 0; q->p = 0;
    if (!v || !cp_mfn(v)) return;
    m.s = v;
    m.i = 0;
    m.depth = 0;
    cp_mvalue(&m, q);
    if (q->t && v[m.i]) q->t = 0;
}

int wf_cssprop_math_type(const char *v) {
    cp_q_t q;
    cp_mwhole(v, &q);
    return q.t;
}

/* Lot 95: append w[0..n) at out[*len] when it fits with the NUL (cap
 * bytes); 0 when it does not (nothing written) */
static int cp_put(char *out, int *len, int cap, const char *w, int n) {
    int k;
    if (*len + n >= cap) return 0;
    for (k = 0; k < n; k++) out[(*len)++] = w[k];
    out[*len] = 0;
    return 1;
}

/* Lot 92: "N[.dddd]" and the unit for a value in 1/10000 */
static int cp_fmt(long long v, const char *unit, char *out, int cap) {
    char d[24];
    int n = 0, k = 0, j, fd = 0, f;
    unsigned long long u, r;
    long long whole;
    int frac;
    if (v < 0) { n = 1; v = -v; }
    whole = (long long)cp_udiv((unsigned long long)v, (unsigned long long)CP_S, &r);
    frac = (int)r;
    u = (unsigned long long)whole;
    do { unsigned long long rr; u = cp_udiv(u, 10, &rr); d[k++] = (char)('0' + (int)rr); } while (u && k < 20);
    /* Lot 95: the exact length (Lot 92 kept 6 spare bytes), so a value
     * that fits is written and one that does not is refused, never cut */
    for (f = frac; f; f = (f % 1000) * 10) fd++;
    if (n + k + (fd ? fd + 1 : 0) + cp_len(unit) >= cap) return -1;
    if (n) out[0] = '-';
    while (k) out[n++] = d[--k];
    if (frac) {
        int p = 1000;
        out[n++] = '.';
        while (frac && p) { out[n++] = (char)('0' + frac / p); frac %= p; p /= 10; }
    }
    for (j = 0; unit[j]; j++) out[n++] = unit[j];
    out[n] = 0;
    return n;
}

/* Lot 95: a known value in its canonical unit; a mix as
 * "calc(P% + Lpx)" (or " - " and the absolute px); -1 when cap is short */
static int cp_fmt_q(const cp_q_t *q, char *out, int cap) {
    const char *unit;
    int n, k;
    if (q->t == WF_CSSPROP_LENPCT) {
        n = 0;
        if (!cp_put(out, &n, cap, "calc(", 5)) return -1;
        k = cp_fmt(q->p, "%", out + n, cap - n);
        if (k < 0) return -1;
        n += k;
        if (!cp_put(out, &n, cap, q->v < 0 ? " - " : " + ", 3)) return -1;
        k = cp_fmt(q->v < 0 ? -q->v : q->v, "px", out + n, cap - n);
        if (k < 0) return -1;
        n += k;
        if (!cp_put(out, &n, cap, ")", 1)) return -1;
        return n;
    }
    switch (q->t) {
    case WF_CSSPROP_LENGTH: unit = "px"; break;
    case WF_CSSPROP_PERCENT: unit = "%"; break;
    case WF_CSSPROP_ANGLE: unit = "deg"; break;
    case WF_CSSPROP_TIME: unit = "s"; break;
    case WF_CSSPROP_RESOLUTION: unit = "dppx"; break;
    default: unit = ""; break;
    }
    return cp_fmt(q->v, unit, out, cap);
}

int wf_cssprop_math_eval(const char *v, int *type, char *out, int cap) {
    cp_q_t q;
    if (type) *type = 0;
    if (out && cap > 0) out[0] = 0;
    cp_mwhole(v, &q);
    if (!q.t) return -1;
    if (type) *type = q.t;
    if (!cp_known(&q) || !out) return 0;
    if (cp_fmt_q(&q, out, cap) < 0) { out[0] = 0; return 0; }
    return 1;
}

/* one dimension: the type of a number with its unit (WF_CSSPROP_LENGTH,
 * _PERCENT, _NUMBER with *frac and *zero set, _ANGLE, _TIME), 0 other */
static int cp_dim(const char *v, int *frac, int *zero) {
    int i = 0, a, k;
    if (!cp_number(v, &i, frac)) return 0;
    *zero = 1;
    for (k = 0; k < i; k++) if (cp_digit(v[k]) && v[k] != '0') *zero = 0;
    if (!v[i]) return WF_CSSPROP_NUMBER;
    if (v[i] == '%' && !v[i + 1]) return WF_CSSPROP_PERCENT;
    a = i;
    while (v[i] && ((v[i] >= 'a' && v[i] <= 'z') || (v[i] >= 'A' && v[i] <= 'Z'))) i++;
    if (v[i]) return 0;
    return cp_unit_type(v + a, i - a);
}

static int cp_hex(char c) { return cp_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }

/* Lot 100: the 148 named colours of CSS Color 4 (0xRRGGBB) */
static const struct { const char *name; unsigned int rgb; } cp_named[] = {
    {"aliceblue", 0xf0f8ffU}, {"antiquewhite", 0xfaebd7U}, {"aqua", 0x00ffffU}, {"aquamarine", 0x7fffd4U},
    {"azure", 0xf0ffffU}, {"beige", 0xf5f5dcU}, {"bisque", 0xffe4c4U}, {"black", 0x000000U},
    {"blanchedalmond", 0xffebcdU}, {"blue", 0x0000ffU}, {"blueviolet", 0x8a2be2U}, {"brown", 0xa52a2aU},
    {"burlywood", 0xdeb887U}, {"cadetblue", 0x5f9ea0U}, {"chartreuse", 0x7fff00U}, {"chocolate", 0xd2691eU},
    {"coral", 0xff7f50U}, {"cornflowerblue", 0x6495edU}, {"cornsilk", 0xfff8dcU}, {"crimson", 0xdc143cU},
    {"cyan", 0x00ffffU}, {"darkblue", 0x00008bU}, {"darkcyan", 0x008b8bU}, {"darkgoldenrod", 0xb8860bU},
    {"darkgray", 0xa9a9a9U}, {"darkgreen", 0x006400U}, {"darkgrey", 0xa9a9a9U}, {"darkkhaki", 0xbdb76bU},
    {"darkmagenta", 0x8b008bU}, {"darkolivegreen", 0x556b2fU}, {"darkorange", 0xff8c00U}, {"darkorchid", 0x9932ccU},
    {"darkred", 0x8b0000U}, {"darksalmon", 0xe9967aU}, {"darkseagreen", 0x8fbc8fU}, {"darkslateblue", 0x483d8bU},
    {"darkslategray", 0x2f4f4fU}, {"darkslategrey", 0x2f4f4fU}, {"darkturquoise", 0x00ced1U},
    {"darkviolet", 0x9400d3U}, {"deeppink", 0xff1493U}, {"deepskyblue", 0x00bfffU}, {"dimgray", 0x696969U},
    {"dimgrey", 0x696969U}, {"dodgerblue", 0x1e90ffU}, {"firebrick", 0xb22222U}, {"floralwhite", 0xfffaf0U},
    {"forestgreen", 0x228b22U}, {"fuchsia", 0xff00ffU}, {"gainsboro", 0xdcdcdcU}, {"ghostwhite", 0xf8f8ffU},
    {"gold", 0xffd700U}, {"goldenrod", 0xdaa520U}, {"gray", 0x808080U}, {"green", 0x008000U},
    {"greenyellow", 0xadff2fU}, {"grey", 0x808080U}, {"honeydew", 0xf0fff0U}, {"hotpink", 0xff69b4U},
    {"indianred", 0xcd5c5cU}, {"indigo", 0x4b0082U}, {"ivory", 0xfffff0U}, {"khaki", 0xf0e68cU},
    {"lavender", 0xe6e6faU}, {"lavenderblush", 0xfff0f5U}, {"lawngreen", 0x7cfc00U}, {"lemonchiffon", 0xfffacdU},
    {"lightblue", 0xadd8e6U}, {"lightcoral", 0xf08080U}, {"lightcyan", 0xe0ffffU},
    {"lightgoldenrodyellow", 0xfafad2U}, {"lightgray", 0xd3d3d3U}, {"lightgreen", 0x90ee90U},
    {"lightgrey", 0xd3d3d3U}, {"lightpink", 0xffb6c1U}, {"lightsalmon", 0xffa07aU}, {"lightseagreen", 0x20b2aaU},
    {"lightskyblue", 0x87cefaU}, {"lightslategray", 0x778899U}, {"lightslategrey", 0x778899U},
    {"lightsteelblue", 0xb0c4deU}, {"lightyellow", 0xffffe0U}, {"lime", 0x00ff00U}, {"limegreen", 0x32cd32U},
    {"linen", 0xfaf0e6U}, {"magenta", 0xff00ffU}, {"maroon", 0x800000U}, {"mediumaquamarine", 0x66cdaaU},
    {"mediumblue", 0x0000cdU}, {"mediumorchid", 0xba55d3U}, {"mediumpurple", 0x9370dbU},
    {"mediumseagreen", 0x3cb371U}, {"mediumslateblue", 0x7b68eeU}, {"mediumspringgreen", 0x00fa9aU},
    {"mediumturquoise", 0x48d1ccU}, {"mediumvioletred", 0xc71585U}, {"midnightblue", 0x191970U},
    {"mintcream", 0xf5fffaU}, {"mistyrose", 0xffe4e1U}, {"moccasin", 0xffe4b5U}, {"navajowhite", 0xffdeadU},
    {"navy", 0x000080U}, {"oldlace", 0xfdf5e6U}, {"olive", 0x808000U}, {"olivedrab", 0x6b8e23U},
    {"orange", 0xffa500U}, {"orangered", 0xff4500U}, {"orchid", 0xda70d6U}, {"palegoldenrod", 0xeee8aaU},
    {"palegreen", 0x98fb98U}, {"paleturquoise", 0xafeeeeU}, {"palevioletred", 0xdb7093U}, {"papayawhip", 0xffefd5U},
    {"peachpuff", 0xffdab9U}, {"peru", 0xcd853fU}, {"pink", 0xffc0cbU}, {"plum", 0xdda0ddU},
    {"powderblue", 0xb0e0e6U}, {"purple", 0x800080U}, {"rebeccapurple", 0x663399U}, {"red", 0xff0000U},
    {"rosybrown", 0xbc8f8fU}, {"royalblue", 0x4169e1U}, {"saddlebrown", 0x8b4513U}, {"salmon", 0xfa8072U},
    {"sandybrown", 0xf4a460U}, {"seagreen", 0x2e8b57U}, {"seashell", 0xfff5eeU}, {"sienna", 0xa0522dU},
    {"silver", 0xc0c0c0U}, {"skyblue", 0x87ceebU}, {"slateblue", 0x6a5acdU}, {"slategray", 0x708090U},
    {"slategrey", 0x708090U}, {"snow", 0xfffafaU}, {"springgreen", 0x00ff7fU}, {"steelblue", 0x4682b4U},
    {"tan", 0xd2b48cU}, {"teal", 0x008080U}, {"thistle", 0xd8bfd8U}, {"tomato", 0xff6347U}, {"turquoise", 0x40e0d0U},
    {"violet", 0xee82eeU}, {"wheat", 0xf5deb3U}, {"white", 0xffffffU}, {"whitesmoke", 0xf5f5f5U},
    {"yellow", 0xffff00U}, {"yellowgreen", 0x9acd32U}, {0, 0}
};

/* the named colour v[0..n): its index in cp_named, -1 */
static int cp_named_at(const char *v, int n) {
    int k;
    for (k = 0; cp_named[k].name; k++) if (cp_eqn(v, n, cp_named[k].name)) return k;
    return -1;
}

/* Lot 102: miss, the missing components (the none keyword): bits 0..2
 * the channels (red, green, blue), bit 3 the alpha. Lot 103: the other
 * analogous sets of CSS Color 4 (12.2): bit 4 lightness, 5 colourfulness
 * (chroma, saturation), 6 hue, 7 opponent a, 8 opponent b; lin, when
 * lin_ok, the linear sRGB values before gamut mapping (1/2^40). Lot 106:
 * ok, when ok_ok, the exact OKLab values (1/2^40) of an oklab() or
 * oklch() colour or of a mix in oklab or oklch: the mix in Oklab takes
 * them as they are, the way back from linear sRGB losing precision near
 * a cone response of 0 (an OKLab L near 0) */
typedef struct { long long c[3], a; int miss; long long lin[3]; int lin_ok; long long ok[3]; int ok_ok; } cp_rgba_t;
static int cp_rgba(const char *v, cp_rgba_t *o);

/* Lot 108: relative colours (CSS Color 5): rgb(), rgba(), hsl(), hsla(),
 * oklab() and oklch() with "from" and an origin colour first; the
 * function (0 rgb, 1 hsl, 2 oklab, 3 oklch) and in *at the index after
 * "from" and its space, -1 when v[0..n) does not start one */
static const char *const cp_relf[] = {"rgb", "hsl", "oklab", "oklch", 0};

static int cp_rel_head(const char *v, int n, int *at) {
    int k;
    for (k = 0; cp_relf[k]; k++) {
        int l = cp_len(cp_relf[k]), i;
        if (n <= l + 1 || !cp_eqn(v, l, cp_relf[k])) continue;
        i = l + (k < 2 && cp_low(v[l]) == 'a');      /* rgba( and hsla( too */
        if (v[i] != '(') continue;
        for (i++; i < n && cp_sp(v[i]); i++) {}
        if (i + 5 < n && cp_eqn(v + i, 4, "from") && cp_sp(v[i + 4])) { *at = i + 5; return k; }
        return -1;
    }
    return -1;
}

static int cp_color(const char *v) {
    static const char *const fns[] = {"rgb(", "rgba(", "hsl(", "hsla(", 0};
    cp_rgba_t t;
    int k, n = cp_len(v);
    if (v[0] == '#') {
        for (k = 1; k < n; k++) if (!cp_hex(v[k])) return 0;
        return n == 4 || n == 5 || n == 7 || n == 9;
    }
    /* Lot 100: every named colour */
    if (cp_eqn(v, n, "transparent") || cp_eqn(v, n, "currentcolor") || cp_named_at(v, n) >= 0) return 1;
    /* Lot 100: hwb() and color-mix() are matched by their conversion;
     * Lot 101: lab(), lch(), oklab(), oklch() and color() too */
    if ((n > 4 && (cp_eqn(v, 4, "hwb(") || cp_eqn(v, 4, "lab(") || cp_eqn(v, 4, "lch("))) ||
        (n > 6 && (cp_eqn(v, 6, "oklab(") || cp_eqn(v, 6, "oklch(") || cp_eqn(v, 6, "color("))) ||
        (n > 10 && cp_eqn(v, 10, "color-mix(")))
        return cp_rgba(v, &t);
    if (cp_rel_head(v, n, &k) >= 0) return cp_rgba(v, &t);      /* Lot 108 */
    for (k = 0; fns[k]; k++) {
        int m = cp_len(fns[k]), i, depth = 0;
        if (n <= m || !cp_eqn(v, m, fns[k])) continue;
        /* Lot 102: a none component is matched by the conversion */
        for (i = m; i + 4 <= n; i++) if (cp_eqn(v + i, 4, "none")) return cp_rgba(v, &t);
        /* one level, digits, '.', '%', ',', '/', spaces and "deg"; Lot
         * 99: the letters of grad, rad and turn in hsl() and hsla() */
        for (i = m; i < n - 1; i++) {
            char c = cp_low(v[i]);
            if (c == '(' || c == ')') { depth++; break; }
            if (!(cp_digit(c) || c == '.' || c == '%' || c == ',' || c == '/' || cp_sp(c) || c == 'd' || c == 'e' ||
                  c == 'g' || c == '-' || c == '+' ||
                  (k >= 2 && (c == 'r' || c == 'a' || c == 't' || c == 'u' || c == 'n'))))
                return 0;
        }
        return !depth && v[n - 1] == ')' && n - 1 > m;
    }
    return 0;
}

/* ---- Lot 95: <url>, <string>, <image>, <transform-function>,
 * <transform-list> ------------------------------------------------------ */

/* a quoted string at s[a..b) exactly: the same quote at both ends, no
 * unescaped quote of that kind nor line break inside */
static int cp_string(const char *s, int a, int b) {
    char q;
    int i;
    if (b - a < 2 || (s[a] != '\'' && s[a] != '"') || s[b - 1] != s[a]) return 0;
    q = s[a];
    for (i = a + 1; i < b - 1; i++) {
        if (s[i] == '\\') { if (i + 1 >= b - 1 || s[i + 1] == '\n') return 0; i++; continue; }
        if (s[i] == q || s[i] == '\n' || s[i] == '\r') return 0;
    }
    return 1;
}

/* url(...) at s[a..b) exactly: a quoted string or an unquoted url (no
 * quote, parenthesis or white space inside; escapes taken), white space
 * allowed around it */
static int cp_url(const char *s, int a, int b) {
    int i, e;
    if (b - a < 5 || !cp_eqn(s + a, 4, "url(") || s[b - 1] != ')') return 0;
    i = a + 4;
    e = b - 1;
    while (i < e && cp_sp(s[i])) i++;
    while (e > i && cp_sp(s[e - 1])) e--;
    if (i < e && (s[i] == '\'' || s[i] == '"')) return cp_string(s, i, e);
    for (; i < e; i++) {
        char c = s[i];
        if (c == '\\') { if (i + 1 >= e) return 0; i++; continue; }
        if (c == '"' || c == '\'' || c == '(' || c == ')' || cp_sp(c) || (unsigned char)c < 32) return 0;
    }
    return 1;
}

/* the end of the parenthesis group that opens at s[i] == '(' within b
 * (the index after its ')'), -1 when it is not closed; quotes are skipped */
static int cp_group_end(const char *s, int i, int b) {
    int depth = 0;
    char q = 0;
    for (; i < b; i++) {
        char c = s[i];
        if (q) { if (c == '\\') i++; else if (c == q) q = 0; continue; }
        if (c == '\'' || c == '"') q = c;
        else if (c == '(') depth++;
        else if (c == ')') { if (--depth == 0) return i + 1; }
    }
    return -1;
}

/* <image>: a url, or a gradient the background reader takes (Lot 96: its
 * stops are checked by wf_cssbg_gradient; Lot 95: only balanced
 * parentheses closing at the end) */
static int cp_image(const char *s, int a, int b) {
    char t[WF_CSSBG_VAL];
    if (cp_url(s, a, b)) return 1;
    return wf_cssbg_gradient(s + a, b - a, t, (int)sizeof t) > 0;
}

/* the transform functions and their arguments: P length-percentage, L
 * length, N number, S number or percentage, A angle or a zero */
static const struct { const char *name; unsigned char lo, hi; const char *args; } cp_tfns[] = {
    {"translate", 1, 2, "PP"}, {"translatex", 1, 1, "P"}, {"translatey", 1, 1, "P"}, {"translatez", 1, 1, "L"},
    {"translate3d", 3, 3, "PPL"}, {"scale", 1, 2, "SS"}, {"scalex", 1, 1, "S"}, {"scaley", 1, 1, "S"},
    {"scalez", 1, 1, "S"}, {"scale3d", 3, 3, "SSS"}, {"rotate", 1, 1, "A"}, {"rotatex", 1, 1, "A"},
    {"rotatey", 1, 1, "A"}, {"rotatez", 1, 1, "A"}, {"rotate3d", 4, 4, "NNNA"}, {"skew", 1, 2, "AA"},
    {"skewx", 1, 1, "A"}, {"skewy", 1, 1, "A"}, {"matrix", 6, 6, "NNNNNN"},
    {"perspective", 1, 1, "L"}, {0, 0, 0, 0}};

/* the transform function whose name is s[a..a + l) (its index), -1 */
static int cp_tfn(const char *s, int a, int l) {
    int k;
    for (k = 0; cp_tfns[k].name; k++) if (cp_eqn(s + a, l, cp_tfns[k].name)) return k;
    return -1;
}

static int cp_copy_item(char *dst, int cap, const char *s, int a, int b) {
    int k;
    while (a < b && cp_sp(s[a])) a++;
    while (b > a && cp_sp(s[b - 1])) b--;
    if (b <= a || b - a >= cap) return -1;
    for (k = 0; k < b - a; k++) dst[k] = s[a + k];
    dst[k] = 0;
    return k;
}

static int cp_targ_ok(char kind, const char *item) {
    int frac, zero;
    switch (kind) {
    case 'P': return wf_cssprop_match(WF_CSSPROP_LENPCT, item);
    case 'L': return wf_cssprop_match(WF_CSSPROP_LENGTH, item);
    case 'N': return wf_cssprop_match(WF_CSSPROP_NUMBER, item);
    case 'S': return wf_cssprop_match(WF_CSSPROP_NUMBER, item) || wf_cssprop_match(WF_CSSPROP_PERCENT, item);
    default: return wf_cssprop_match(WF_CSSPROP_ANGLE, item) || (cp_dim(item, &frac, &zero) == WF_CSSPROP_NUMBER && zero);
    }
}

/* one transform function at s[a..b) exactly: its index, -1 */
static int cp_tfunc(const char *s, int a, int b) {
    char item[WF_CSSPROP_VAL];
    int i = a, k, n = 0, start;
    while (i < b && ((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z') || cp_digit(s[i]))) i++;
    if (i >= b || s[i] != '(' || (k = cp_tfn(s, a, i - a)) < 0) return -1;
    if (cp_group_end(s, i, b) != b) return -1;
    start = i + 1;
    for (i = start; i <= b - 1; i++) {
        int depth = 0, e = i;
        while (e < b - 1 && !(depth == 0 && s[e] == ',')) {
            if (s[e] == '(') depth++;
            else if (s[e] == ')') depth--;
            e++;
        }
        if (n >= cp_tfns[k].hi || cp_copy_item(item, (int)sizeof(item), s, i, e) < 0 ||
            !cp_targ_ok(cp_tfns[k].args[n], item))
            return -1;
        n++;
        i = e;
    }
    return n >= cp_tfns[k].lo ? k : -1;
}

/* the end of the transform function that starts at s[a] (index after it),
 * -1 when none starts there */
static int cp_tfunc_end(const char *s, int a, int b) {
    int i = a, e;
    while (i < b && s[i] != '(') i++;
    if (i >= b) return -1;
    e = cp_group_end(s, i, b);
    if (e < 0 || cp_tfunc(s, a, e) < 0) return -1;
    return e;
}

/* <transform-list>: one or more transform functions, white space between
 * them optional */
static int cp_tlist(const char *s, int a, int b) {
    int i = a, n = 0;
    while (i < b) {
        while (i < b && cp_sp(s[i])) i++;
        if (i >= b) break;
        i = cp_tfunc_end(s, i, b);
        if (i < 0) return 0;
        n++;
    }
    return n > 0;
}

int wf_cssprop_match(int syntax, const char *v) {
    int frac = 0, zero = 0, d, n;
    if (!v) return 0;
    if (syntax == WF_CSSPROP_ANY) return 1;
    if (!v[0]) return 0;
    if (syntax == WF_CSSPROP_COLOR) return cp_color(v);
    if (syntax == WF_CSSPROP_CIDENT) {
        n = cp_len(v);
        return cp_is_ident(v, n) && !cp_reserved(v, n);
    }
    n = cp_len(v);
    switch (syntax) {   /* Lot 95 */
    case WF_CSSPROP_URL: return cp_url(v, 0, n);
    case WF_CSSPROP_STRING: return cp_string(v, 0, n);
    case WF_CSSPROP_IMAGE: return cp_image(v, 0, n);
    case WF_CSSPROP_TFUNC: return cp_tfunc(v, 0, n) >= 0;
    case WF_CSSPROP_TLIST: return cp_tlist(v, 0, n);
    default: break;
    }
    if ((syntax < WF_CSSPROP_LENGTH || syntax > WF_CSSPROP_TIME) && syntax != WF_CSSPROP_RESOLUTION) return 0;
    if (cp_mfn(v)) {
        /* Lot 91: the type of the expression (Lot 90: any balanced one) */
        d = wf_cssprop_math_type(v);
        switch (syntax) {
        case WF_CSSPROP_LENGTH: return d == WF_CSSPROP_LENGTH;
        case WF_CSSPROP_PERCENT: return d == WF_CSSPROP_PERCENT;
        case WF_CSSPROP_LENPCT: return d == WF_CSSPROP_LENGTH || d == WF_CSSPROP_PERCENT || d == WF_CSSPROP_LENPCT;
        case WF_CSSPROP_NUMBER: case WF_CSSPROP_INTEGER: return d == WF_CSSPROP_NUMBER;
        default: return d == syntax;      /* angle, time */
        }
    }
    d = cp_dim(v, &frac, &zero);
    switch (syntax) {
    case WF_CSSPROP_LENGTH: return d == WF_CSSPROP_LENGTH || (d == WF_CSSPROP_NUMBER && zero);
    case WF_CSSPROP_PERCENT: return d == WF_CSSPROP_PERCENT;
    case WF_CSSPROP_LENPCT: return d == WF_CSSPROP_LENGTH || d == WF_CSSPROP_PERCENT || (d == WF_CSSPROP_NUMBER && zero);
    case WF_CSSPROP_NUMBER: return d == WF_CSSPROP_NUMBER;
    case WF_CSSPROP_INTEGER: return d == WF_CSSPROP_NUMBER && !frac;
    default: return d == syntax;          /* angle, time: a unit is required */
    }
}

/* Lot 91: one alternative against one item s[a..b) */
static int cp_alt_one(const wf_cssprop_alt_t *al, const char *s, int a, int b) {
    char item[WF_CSSPROP_VAL];
    int k;
    if (b - a < 1 || b - a >= (int)sizeof(item)) return 0;
    for (k = 0; k < b - a; k++) item[k] = s[a + k];
    item[k] = 0;
    if (al->type == WF_CSSPROP_KEYWORD) return cp_eqn(item, k, al->ident) && al->ident[0];
    return wf_cssprop_match(al->type, item);
}

/* Lot 91: an alternative against the whole value, with its multiplier */
static int cp_alt(const wf_cssprop_alt_t *al, const char *v) {
    int n = cp_len(v), i = 0, depth = 0, start, items = 0;
    if (al->mult == WF_CSSPROP_ONE) return cp_alt_one(al, v, 0, n);
    while (i <= n) {
        int x, y;
        start = i;
        depth = 0;
        while (i < n) {
            char c = v[i];
            if (c == '(') depth++;
            else if (c == ')') depth--;
            else if (depth == 0 && ((al->mult == WF_CSSPROP_HASH && c == ',') ||
                                    (al->mult == WF_CSSPROP_PLUS && cp_sp(c)))) break;
            i++;
        }
        x = start; y = i;
        while (x < y && cp_sp(v[x])) x++;
        while (y > x && cp_sp(v[y - 1])) y--;
        if (al->mult == WF_CSSPROP_PLUS) {
            if (y > x) { if (!cp_alt_one(al, v, x, y)) return 0; items++; }
        } else {
            if (y <= x || !cp_alt_one(al, v, x, y)) return 0;
            items++;
        }
        if (i >= n) break;
        i++;
    }
    return items > 0;
}

int wf_cssprop_match_prop(const wf_cssprop_t *p, const char *v) {
    int k;
    if (!p || !v) return 0;
    if (p->syntax == WF_CSSPROP_ANY) return 1;
    if (!p->nalt) return wf_cssprop_match(p->syntax, v);
    if (!v[0]) return 0;
    for (k = 0; k < p->nalt && k < WF_CSSPROP_ALTS; k++) if (cp_alt(&p->alt[k], v)) return 1;
    return 0;
}

/* ---- Lot 92/95: computed values ---------------------------------------- */

/* one numeric item s[a..b) of type t (it matches t): its computed text
 * into out (length, -1 when cap is short); an item whose value is not
 * known keeps its text. Lot 92 computed math functions only; Lot 95 also
 * plain dimensions, mixes and viewport units. */
static int cp_num_item(int t, const char *s, int a, int b, char *out, int cap) {
    char item[WF_CSSPROP_VAL];
    cp_q_t q;
    int n = cp_copy_item(item, (int)sizeof(item), s, a, b);
    if (n < 0) return -1;
    q.t = 0; q.ok = 0; q.v = 0; q.p = 0;
    if (cp_mfn(item)) {
        cp_mwhole(item, &q);
        if (q.t && cp_known(&q) && t == WF_CSSPROP_INTEGER)
            q.v = cp_div(2 * q.v + CP_S, 2 * CP_S, 1) * CP_S;   /* half toward +infinity */
    } else {
        cp_mp_t m;
        m.s = item; m.i = 0; m.depth = 0;
        cp_mleaf(&m, &q);   /* the item matches t, so the leaf is all of it */
        /* a unitless zero length is 0px */
        if (q.t == WF_CSSPROP_NUMBER && (t == WF_CSSPROP_LENGTH || t == WF_CSSPROP_LENPCT)) q.t = WF_CSSPROP_LENGTH;
    }
    if (q.t && cp_known(&q)) return cp_fmt_q(&q, out, cap);
    if (n >= cap) return -1;
    for (a = 0; a <= n; a++) out[a] = item[a];
    return n;
}

/* one item s[a..b) of an alternative of type t: computed text into out */
static int cp_item(int t, const char *s, int a, int b, char *out, int cap) {
    char item[WF_CSSPROP_VAL];
    int n, k, i, start, len;
    if ((t >= WF_CSSPROP_LENGTH && t <= WF_CSSPROP_TIME && t != WF_CSSPROP_COLOR) || t == WF_CSSPROP_RESOLUTION)
        return cp_num_item(t, s, a, b, out, cap);
    if (t == WF_CSSPROP_TFUNC || t == WF_CSSPROP_TLIST) {
        /* each function: its name, then each argument computed, ", " */
        len = 0;
        i = a;
        while (i < b) {
            int e, fn, arg = 0;
            while (i < b && cp_sp(s[i])) i++;
            if (i >= b) break;
            e = cp_tfunc_end(s, i, b);
            if (e < 0) return -1;
            k = i;
            while (s[k] != '(') k++;
            fn = cp_tfn(s, i, k - i);
            if (len && !cp_put(out, &len, cap, " ", 1)) return -1;
            if (!cp_put(out, &len, cap, cp_tfns[fn].name, cp_len(cp_tfns[fn].name)) || !cp_put(out, &len, cap, "(", 1))
                return -1;
            for (start = k + 1; start < e; start++) {
                int depth = 0, x = start, typ;
                char kind;
                while (x < e - 1 && !(depth == 0 && s[x] == ',')) {
                    if (s[x] == '(') depth++;
                    else if (s[x] == ')') depth--;
                    x++;
                }
                kind = cp_tfns[fn].args[arg++];
                if (cp_copy_item(item, (int)sizeof(item), s, start, x) < 0) return -1;
                /* only a length takes a unitless zero as 0px; the other
                 * kinds keep the unit of their item */
                typ = kind == 'P' ? WF_CSSPROP_LENPCT : kind == 'L' ? WF_CSSPROP_LENGTH : WF_CSSPROP_NUMBER;
                if (kind == 'A' && !wf_cssprop_match(WF_CSSPROP_ANGLE, item)) {
                    /* a zero angle */
                    if (!cp_put(out, &len, cap, "0deg", 4)) return -1;
                } else {
                    k = cp_num_item(typ, s, start, x, out + len, cap - len);
                    if (k < 0) return -1;
                    len += k;
                }
                if (x < e - 1 && !cp_put(out, &len, cap, ", ", 2)) return -1;
                start = x;
            }
            if (!cp_put(out, &len, cap, ")", 1)) return -1;
            i = e;
        }
        return len;
    }
    /* colours, idents, keywords, urls, strings, images: as written */
    n = cp_copy_item(item, (int)sizeof(item), s, a, b);
    if (n < 0 || n >= cap) return -1;
    for (k = 0; k <= n; k++) out[k] = item[k];
    return n;
}

/* the first alternative of p that value matches (one built from the
 * syntax code when p has none), 0 */
static const wf_cssprop_alt_t *cp_first_alt(const wf_cssprop_t *p, const char *v, wf_cssprop_alt_t *one) {
    int k;
    if (!p->nalt) {
        one->type = p->syntax;
        one->mult = WF_CSSPROP_ONE;
        one->ident[0] = 0;
        return wf_cssprop_match(p->syntax, v) ? one : 0;
    }
    for (k = 0; k < p->nalt && k < WF_CSSPROP_ALTS; k++) if (cp_alt(&p->alt[k], v)) return &p->alt[k];
    return 0;
}

int wf_cssprop_compute_ctx(const wf_cssprop_t *p, const char *v, const wf_cssprop_ctx_t *c, char *out, int cap) {
    const wf_cssprop_alt_t *al;
    wf_cssprop_alt_t one;
    int n, len = 0, i = 0, k;
    if (out && cap > 0) out[0] = 0;
    if (!p || !v || !out || cap < 2 || p->syntax == WF_CSSPROP_ANY) return 0;
    al = cp_first_alt(p, v, &one);
    if (!al || al->type == WF_CSSPROP_KEYWORD) return 0;
    cp_ctx = c;
    n = cp_len(v);
    if (al->mult == WF_CSSPROP_ONE) len = cp_item(al->type, v, 0, n, out, cap);
    else {
        /* each item of the list, joined by one space or ", " */
        while (i <= n && len >= 0) {
            int depth = 0, start = i, x, y;
            while (i < n) {
                char ch = v[i];
                if (ch == '(') depth++;
                else if (ch == ')') depth--;
                else if (depth == 0 && ((al->mult == WF_CSSPROP_HASH && ch == ',') ||
                                        (al->mult == WF_CSSPROP_PLUS && cp_sp(ch)))) break;
                i++;
            }
            x = start; y = i;
            while (x < y && cp_sp(v[x])) x++;
            while (y > x && cp_sp(v[y - 1])) y--;
            if (y > x) {
                if (len && !cp_put(out, &len, cap, al->mult == WF_CSSPROP_HASH ? ", " : " ",
                                   al->mult == WF_CSSPROP_HASH ? 2 : 1)) { len = -1; break; }
                k = cp_item(al->type, v, x, y, out + len, cap - len);
                len = k < 0 ? -1 : len + k;
            }
            if (i >= n) break;
            i++;
        }
    }
    cp_ctx = 0;
    if (len < 0) { out[0] = 0; return 0; }
    out[len] = 0;
    for (k = 0; k <= n && out[k] == v[k]; k++) {}
    if (k > n) { out[0] = 0; return 0; }
    return 1;
}

/* Lot 92: wf_cssprop_compute_ctx without a viewport */
int wf_cssprop_compute(const wf_cssprop_t *p, const char *v, char *out, int cap) {
    return wf_cssprop_compute_ctx(p, v, 0, out, cap);
}

int wf_cssprop_independent(const char *v) {
    int i;
    if (!v) return 0;
    for (i = 0; v[i]; i++) {
        int a, k;
        if (cp_low(v[i]) == 'v' && cp_low(v[i + 1]) == 'a' && cp_low(v[i + 2]) == 'r' && v[i + 3] == '(' &&
            (i == 0 || !cp_ident(v[i - 1])))
            return 0;
        if (!cp_digit(v[i]) && v[i] != '.') continue;
        if (i > 0 && (cp_ident(v[i - 1]) && !cp_digit(v[i - 1]) && v[i - 1] != '-')) continue;   /* inside a name */
        k = i;
        while (cp_digit(v[k]) || v[k] == '.') k++;
        a = k;
        while ((v[k] >= 'a' && v[k] <= 'z') || (v[k] >= 'A' && v[k] <= 'Z')) k++;
        if (cp_eqn(v + a, k - a, "em") || cp_eqn(v + a, k - a, "rem") || cp_eqn(v + a, k - a, "ex") ||
            cp_eqn(v + a, k - a, "ch"))
            return 0;
        i = k - 1;
    }
    return 1;
}

/* the value of one descriptor, lower case, outer spaces trimmed */
static int cp_copy(char *dst, int cap, const char *s, int n) {
    int a = 0, b = n, k;
    while (a < b && cp_sp(s[a])) a++;
    while (b > a && cp_sp(s[b - 1])) b--;
    if (b - a >= cap) return -1;
    for (k = 0; a + k < b; k++) dst[k] = cp_low(s[a + k]);
    dst[k] = 0;
    return k;
}

static void cp_clear(wf_cssprop_t *o) {
    unsigned char *p = (unsigned char *)o;
    int k;
    for (k = 0; k < (int)sizeof(*o); k++) p[k] = 0;
}

int wf_cssprop_parse(const char *pre, int pn, const char *body, int bn, wf_cssprop_t *out) {
    int a = 0, b = pn, k, i = 0, have_syntax = 0, have_inherits = 0;
    char word[24];
    if (!out) return 0;
    cp_clear(out);
    if (!pre || !body) return 0;
    while (a < b && cp_sp(pre[a])) a++;
    while (b > a && cp_sp(pre[b - 1])) b--;
    if (b - a < 3 || b - a >= WF_CSSPROP_NAME || pre[a] != '-' || pre[a + 1] != '-') return 0;
    for (k = a; k < b; k++) if (!cp_ident(pre[k])) return 0;
    (void)cp_copy(out->name, WF_CSSPROP_NAME, pre + a, b - a);
    while (i < bn) {
        int na, nb, va, vb, depth = 0;
        char q = 0;
        while (i < bn && (cp_sp(body[i]) || body[i] == ';')) i++;
        if (i >= bn) break;
        na = i;
        while (i < bn && body[i] != ':' && body[i] != ';') i++;
        if (i >= bn || body[i] != ':') { cp_clear(out); return 0; }
        nb = i;
        while (nb > na && cp_sp(body[nb - 1])) nb--;
        va = ++i;
        /* ';' inside a string or parentheses does not end the value */
        while (i < bn) {
            char c = body[i];
            if (q) { if (c == q) q = 0; }
            else if (c == '\'' || c == '"') q = c;
            else if (c == '(') depth++;
            else if (c == ')') depth--;
            else if (c == ';' && depth <= 0) break;
            i++;
        }
        vb = i;
        if (q) { cp_clear(out); return 0; }
        if (cp_eqn(body + na, nb - na, "syntax")) {
            out->syntax = (unsigned char)wf_cssprop_syntax_parse(body + va, vb - va, out);
            have_syntax = 1;
        } else if (cp_eqn(body + na, nb - na, "inherits")) {
            if (cp_copy(word, (int)sizeof(word), body + va, vb - va) < 0) { cp_clear(out); return 0; }
            if (cp_eqn(word, cp_len(word), "true")) out->inherits = 1;
            else if (cp_eqn(word, cp_len(word), "false")) out->inherits = 0;
            else { cp_clear(out); return 0; }
            have_inherits = 1;
        } else if (cp_eqn(body + na, nb - na, "initial-value")) {
            if (cp_copy(out->initial, WF_CSSPROP_VAL, body + va, vb - va) < 0) { cp_clear(out); return 0; }
            out->has_initial = 1;
        }
        /* other descriptors are ignored, as CSS says */
    }
    if (!have_syntax || out->syntax == WF_CSSPROP_BAD || !have_inherits) { cp_clear(out); return 0; }
    if (out->syntax != WF_CSSPROP_ANY) {
        if (!out->has_initial || !wf_cssprop_match_prop(out, out->initial) ||
            !wf_cssprop_independent(out->initial)) {
            cp_clear(out);
            return 0;
        }
    } else if (out->has_initial && !wf_cssprop_independent(out->initial)) {
        cp_clear(out);
        return 0;
    }
    return 1;
}

/* ---- Lot 97: font-relative lengths ------------------------------------ */

int wf_cssprop_compute_font(const wf_cssprop_t *p, const char *v, const wf_cssprop_fctx_t *c, char *out, int cap) {
    wf_cssprop_ctx_t vc;
    int r;
    if (!c) return wf_cssprop_compute_ctx(p, v, 0, out, cap);
    vc.vw = c->vw; vc.vh = c->vh;
    cp_fctx = c;
    r = wf_cssprop_compute_ctx(p, v, &vc, out, cap);
    cp_fctx = 0;
    return r;
}

/* ---- Lot 97: interpolation of computed values ------------------------- */

/* the index of the first alternative of p that v matches (0 for a syntax
 * of one type), -1 */
static int cp_alt_index(const wf_cssprop_t *p, const char *v) {
    int k;
    if (!p->nalt) return wf_cssprop_match(p->syntax, v) ? 0 : -1;
    for (k = 0; k < p->nalt && k < WF_CSSPROP_ALTS; k++) if (cp_alt(&p->alt[k], v)) return k;
    return -1;
}

/* the next item of a '+' or '#' list from *i: [*a, *b) trimmed; 0 at the
 * end */
static int cp_next_item(const char *v, int n, int mult, int *i, int *a, int *b) {
    int depth = 0, start;
    while (*i <= n) {
        start = *i;
        depth = 0;
        while (*i < n) {
            char ch = v[*i];
            if (ch == '(') depth++;
            else if (ch == ')') depth--;
            else if (depth == 0 && ((mult == WF_CSSPROP_HASH && ch == ',') || (mult == WF_CSSPROP_PLUS && cp_sp(ch)))) break;
            (*i)++;
        }
        *a = start; *b = *i;
        while (*a < *b && cp_sp(v[*a])) (*a)++;
        while (*b > *a && cp_sp(v[*b - 1])) (*b)--;
        (*i)++;
        if (*b > *a) return 1;
        if (*i > n) return 0;
    }
    return 0;
}

/* one numeric item as a known quantity (a unitless zero of a length is
 * 0px): 1, 0 when it is not known */
static int cp_item_q(int t, const char *s, int a, int b, cp_q_t *q) {
    char item[WF_CSSPROP_VAL];
    if (cp_copy_item(item, (int)sizeof(item), s, a, b) < 0) return 0;
    q->t = 0; q->ok = 0; q->v = 0; q->p = 0;
    if (cp_mfn(item)) cp_mwhole(item, q);
    else {
        cp_mp_t m;
        m.s = item; m.i = 0; m.depth = 0;
        cp_mleaf(&m, q);
        if (item[m.i]) return 0;
        if (q->t == WF_CSSPROP_NUMBER && (t == WF_CSSPROP_LENGTH || t == WF_CSSPROP_LENPCT)) q->t = WF_CSSPROP_LENGTH;
    }
    return q->t && cp_known(q);
}

/* a + round((b - a) t / 10000), half away from zero */
static long long cp_lerp(long long a, long long b, int t) { return a + cp_div((b - a) * t, CP_S, 0); }

/* one numeric item of type t: its interpolated text, -1 when it is not
 * known or does not fit (the caller goes discrete), -2 when cap is short */
static int cp_interp_num(int t, const char *s, int a0, int b0, const char *u, int a1, int b1, int tt, int add, char *out,
                         int cap) {
    cp_q_t qa, qb, r;
    long long pa, ca, pb, cb;
    int n;
    if (!cp_item_q(t, s, a0, b0, &qa) || !cp_item_q(t, u, a1, b1, &qb)) return -1;
    cp_parts(&qa, &pa, &ca);
    cp_parts(&qb, &pb, &cb);
    r.ok = 1;
    /* Lot 98: add sums the two values (an integer stays whole) */
    r.v = add ? pa + pb : cp_lerp(pa, pb, tt);
    r.p = add ? ca + cb : cp_lerp(ca, cb, tt);
    if (qa.t == qb.t && qa.t != WF_CSSPROP_LENPCT) {
        r.t = qa.t;
        if (r.t == WF_CSSPROP_PERCENT) r.v = r.p;
    } else if ((qa.t == WF_CSSPROP_LENGTH || qa.t == WF_CSSPROP_PERCENT || qa.t == WF_CSSPROP_LENPCT) &&
               (qb.t == WF_CSSPROP_LENGTH || qb.t == WF_CSSPROP_PERCENT || qb.t == WF_CSSPROP_LENPCT))
        r.t = WF_CSSPROP_LENPCT;
    else return -1;
    if (t == WF_CSSPROP_INTEGER) r.v = cp_div(2 * r.v + CP_S, 2 * CP_S, 1) * CP_S;
    if (!cp_fits(r.v) || !cp_fits(r.p)) return -1;
    n = cp_fmt_q(&r, out, cap);
    return n < 0 ? -2 : n;
}

/* a colour in sRGB (cp_rgba_t): channels in 1/10000 of 0..255, alpha in
 * 1/10000 of 0..1; 0 when it is not one this reader interpolates */

static int cp_hexv(char c) { return cp_digit(c) ? c - '0' : (cp_low(c) - 'a' + 10); }

/* one number or percentage token [a, b): *v in 1/10000, *pct 1 for % */
static int cp_tok_num(const char *s, int a, int b, long long *v, int *pct) {
    char item[32];
    cp_q_t q;
    cp_mp_t m;
    if (cp_copy_item(item, (int)sizeof(item), s, a, b) < 0) return 0;
    m.s = item; m.i = 0; m.depth = 0;
    cp_mleaf(&m, &q);
    if (item[m.i] || !q.ok || (q.t != WF_CSSPROP_NUMBER && q.t != WF_CSSPROP_PERCENT)) return 0;
    *v = q.v;
    *pct = q.t == WF_CSSPROP_PERCENT;
    return 1;
}

static long long cp_clampll(long long v, long long lo, long long hi) { return v < lo ? lo : v > hi ? hi : v; }

/* Lot 102: the none keyword at s[a..b) (CSS Color 4, missing components) */
static int cp_none(const char *s, int a, int b) { return cp_eqn(s + a, b - a, "none"); }

/* Lot 102: the alpha token s[a..b) into o: none (missing, 0), else a
 * number or percentage clamped to 0..1; 0 when it is neither */
static int cp_alpha_tok(const char *s, int a, int b, cp_rgba_t *o) {
    long long y;
    int pct;
    if (cp_none(s, a, b)) { o->a = 0; o->miss |= 8; return 1; }
    if (!cp_tok_num(s, a, b, &y, &pct)) return 0;
    o->a = cp_clampll(pct ? cp_div(y, 100, 0) : y, 0, CP_S);
    return 1;
}

/* Lot 98: the 3 or 4 arguments of a colour function from v[i] to the
 * closing parenthesis (v[n - 1], checked by cp_color before any
 * interpolation): by commas, or by spaces with "/ alpha" before the
 * fourth; their ranges and the count, 0 when the form is not one of
 * these */
static int cp_color_toks(const char *v, int n, int i, int *ta, int *tb) {
    int got = 0, slash = 0, commas = -1;
    while (i < n - 1) {
        int a;
        while (i < n - 1 && cp_sp(v[i])) i++;
        if (i >= n - 1) break;
        if (got > 0) {
            int sep = 0;
            if (v[i] == ',') { sep = 1; i++; }
            else if (v[i] == '/') { sep = 2; i++; }
            if (sep == 1) { if (commas == 0) return 0; commas = 1; }
            else {
                if (commas == 1) return 0;
                commas = 0;
                if (sep == 2) { if (got != 3) return 0; slash = 1; }
            }
            if (commas == 0 && got == 3 && !slash) return 0;
            while (i < n - 1 && cp_sp(v[i])) i++;
        }
        a = i;
        while (i < n - 1 && !cp_sp(v[i]) && v[i] != ',' && v[i] != '/') i++;
        if (got >= 4) return 0;       /* the bound of ta[] and tb[] (an empty token fails its parse) */
        ta[got] = a; tb[got] = i;
        got++;
    }
    return got;
}

/* the first separator after the first argument (at i) is a comma */
static int cp_comma_after(const char *v, int i, int n) {
    while (i < n && cp_sp(v[i])) i++;
    return i < n && v[i] == ',';
}

/* Lot 98: a hue token: a number of degrees or an angle; q in 1/10000 deg */
static int cp_hue(const char *s, int a, int b, cp_q_t *q) {
    char item[32];
    cp_mp_t m;
    int k;
    if (b - a >= (int)sizeof(item)) return 0;      /* over 31 bytes (the token has no space) */
    for (k = a; k < b; k++) item[k - a] = s[k];
    item[b - a] = 0;
    m.s = item; m.i = 0; m.depth = 0;
    cp_mleaf(&m, q);
    return !item[m.i] && q->ok && (q->t == WF_CSSPROP_NUMBER || q->t == WF_CSSPROP_ANGLE);
}

/* Lot 98: hsl to sRGB (CSS Color 4): h in 1/10000 deg (0..3599999), s
 * and l in 1/10000 % (0..1000000); c[] in 1/10000 of 0..255, rounded
 * half away from zero. f(n) = l - a max(-1, min(k - 3, 9 - k, 1)) with
 * k = (n + h / 30) mod 12 and a = s min(l, 1 - l), for n = 0, 8, 4; k is
 * kept in 1/300000, l and a as exact integers, so f is exact in
 * 1/(3 10^17) and the channel 255 f is f 17 / (2 10^12) in 1/10000 */
static void cp_hsl_rgb(long long h, long long s, long long l, long long *c) {
    static const int nn[3] = {0, 8, 4};
    long long a = s * (l < 1000000 - l ? l : 1000000 - l), m, f;
    int k, kk;
    for (k = 0; k < 3; k++) {
        kk = (nn[k] * 300000 + (int)h) % 3600000;    /* 32-bit: below 6e6 */
        m = kk - 900000;
        if (2700000 - kk < m) m = 2700000 - kk;
        m = cp_clampll(m, -300000, 300000);
        f = l * 1000000LL * 300000LL - a * m;
        c[k] = cp_div(f * 17, 2000000000000LL, 0);
    }
}

/* Lot 100: hwb() (CSS Color 4) from the hue (1/10000 deg, 0..3599999)
 * and the whiteness and blackness (1/10000 %, 0..1000000): when W + B
 * reaches 100% the grey W / (W + B); else the channel of hsl(H 100% 50%),
 * 255 g / 600000 with g = 300000 - m (m of cp_hsl_rgb), scaled by
 * 1 - W - B, plus W. Exact, then rounded half away from zero to 1/10000:
 * 255 (g (10^6 - W - B) + 600000 W) / (6 10^11) = 17 (...) / (4 10^6) */
static void cp_hwb_rgb(long long h, long long w, long long b, long long *c) {
    static const int nn[3] = {0, 8, 4};
    long long m;
    int k, kk;
    for (k = 0; k < 3; k++) {
        if (w + b >= 1000000) { c[k] = cp_div(w * 2550000, w + b, 0); continue; }
        kk = (nn[k] * 300000 + (int)h) % 3600000;
        m = kk - 900000;
        if (2700000 - kk < m) m = 2700000 - kk;
        m = cp_clampll(m, -300000, 300000);
        c[k] = cp_div(((300000 - m) * (1000000 - w - b) + 600000 * w) * 17, 4000000, 0);
    }
}

/* ---- Lot 101: lab(), lch(), oklab(), oklch() and color() (CSS Color 4)
 * The components are taken to 1/2^40 (CP_Q), converted to linear sRGB
 * by the CSS Color 4 matrices (products of them, rounded to 1/2^40 once),
 * clipped to 0..1 (no gamut mapping) and gamma-encoded with cp_ln and
 * cp_exp style series at 1/2^62; each channel is then rounded half away
 * from zero to 1/10000 like every other colour of this reader. */
#define CP_Q (1LL << 40)
enum { CP_SP_LAB, CP_SP_OKLAB, CP_SP_D65, CP_SP_D50, CP_SP_P3, CP_SP_A98, CP_SP_PRO, CP_SP_2020, CP_SP_LIN };
/* to linear sRGB, row by row, in 1/2^40: lab (XYZ D50 / white D50),
 * oklab (LMS), XYZ D65, XYZ D50, display-p3, a98-rgb, prophoto-rgb and
 * rec2020 (each from its linear values) */
static const long long cp_mat[8][9] = {
    {3322981042554LL, -1778334711423LL, -445134703355LL, -1037772121500LL, 2106943969724LL, 30339779552LL,
     76291015004LL, -251762609907LL, 1274983222679LL},
    {4482424832304LL, -3636867298743LL, 253954094215LL, -1394662300740LL, 2869458551216LL, -375284622700LL,
     -4613634506LL, -773416949615LL, 1877542211896LL},
    {3563484136397LL, -1690370680086LL, -548228328676LL, -1065694648239LL, 2062648081238LL, 45690268812LL,
     61165919481LL, -224275038097LL, 1162152470138LL},
    {3446018813293LL, -1778334711423LL, -539488813904LL, -1076197007688LL, 2106943969724LL, 36770828159LL,
     79115790799LL, -251762609907LL, 1545238287121LL},
    {1346835967150LL, -247324339374LL, 0LL, -46242110732LL, 1145753738508LL, 0LL, -21591719613LL, -86461246445LL,
     1207564593834LL},
    {1537508400252LL, -437996772476LL, 0LL, 0LL, 1099511627776LL, 0LL, 0LL, -47200922898LL, 1146712550674LL},
    {2236825399369LL, -800044011819LL, -337269759774LL, -251596552665LL, 1354315246465LL, -3207066024LL,
     -9410531768LL, -168518521152LL, 1277440680696LL},
    {1825729164636LL, -646118265058LL, -80099271802LL, -136944694982LL, 1245636609996LL, -9180287239LL,
     -19956975362LL, -110587667869LL, 1230056271006LL}};
/* OKLab to LMS' (CSS Color 4), in 1/2^40 */
static const long long cp_okl[9] = {1099511627776LL, 435777994752LL,   237278740480LL,
                                    1099511627776LL, -116065927168LL,  -70208405504LL,
                                    1099511627776LL, -98388893696LL,   -1420003377152LL};

/* v (1/10000) in 1/2^40; a percentage of the reference rn / rd */
static long long cp_q40(long long v, int pct, long long rn, long long rd) {
    if (pct) return (long long)cp_wdiv(cp_wmul(cp_wmul(cp_w(v), rn), CP_Q), 1000000LL * rd, 0).lo;
    return (long long)cp_wdiv(cp_wmul(cp_w(v), CP_Q), 10000, 0).lo;
}

/* a * b in 1/2^40 */
static long long cp_qmul(long long a, long long b) { return (long long)cp_wshr(cp_wmul(cp_w(a), b), 40).lo; }

/* ln(x / 2^40) for x > 0, in 1/2^62: x / 2^40 = m 2^k, m in [1/2, 1) */
static cp_w_t cp_lnq(long long x) {
    int bl = 0;
    long long m, s;
    while (bl < 63 && (x >> bl)) bl++;
    m = x << (62 - bl);
    s = (long long)cp_wdivu(cp_wmul(cp_w(m - CP_ONE), CP_ONE), (unsigned long long)m + (unsigned long long)CP_ONE, 0).lo;
    return cp_wadd(cp_wmul(cp_w(CP_LN2), bl - 40), cp_w(2 * cp_oddser(s, 0)));
}

/* exp(t) for t in 1/2^62 (at most 12), in 1/2^40 */
static long long cp_expq(cp_w_t t) {
    long long r, e = CP_ONE, term = CP_ONE;
    int k, j;
    if (cp_wlt(t, cp_wmul(cp_w(CP_ONE), -60))) return 0;
    k = (int)(long long)cp_wdiv(t, CP_LN2, 0).lo;
    r = (long long)cp_wsub(t, cp_wmul(cp_w(CP_LN2), k)).lo;
    for (j = 1; j <= 30 && term; j++) {
        term = cp_div(cp_xmul(term, r), j, 0);
        e += term;
    }
    return (long long)cp_wshr(cp_w(e), 22 - k).lo;
}

/* x^(num / den) for x > 0 in 1/2^40 */
static long long cp_powq(long long x, long long num, long long den) {
    return cp_expq(cp_wdiv(cp_wmul(cp_lnq(x), num), den, 0));
}

/* the linear value of a gamma-encoded component v (1/2^40) of space sp,
 * odd around 0 (CSS Color 4) */
static long long cp_decode(int sp, long long v) {
    long long a = v < 0 ? -v : v, r;
    if (sp == CP_SP_P3) {
        /* sRGB curve: v / 12.92 up to 0.04045, else ((v + 0.055) / 1.055)^2.4 */
        if (!cp_wlt(cp_wmul(cp_w(CP_Q), 4045), cp_wmul(cp_w(a), 100000))) r = cp_div(a * 25, 323, 0);
        else r = cp_powq(cp_div((a + cp_div(CP_Q * 11, 200, 0)) * 200, 211, 0), 12, 5);
    } else if (sp == CP_SP_A98) r = a ? cp_powq(a, 563, 256) : 0;
    else if (sp == CP_SP_PRO) {
        /* v / 16 up to 16 / 512, else v^1.8 */
        if (a * 32 <= CP_Q) r = cp_div(a, 16, 0);
        else r = cp_powq(a, 9, 5);
    } else if (sp == CP_SP_2020) {
        /* alpha 1.09929682680944, beta 0.018053968510807: v / 4.5 under
         * 4.5 beta, else ((v + alpha - 1) / alpha)^(1 / 0.45) */
        if (a < 89327467373LL) r = cp_div(a * 2, 9, 0);
        else r = cp_powq((long long)cp_wdiv(cp_wmul(cp_w(a + 109178015678LL), CP_Q), 1208689643454LL, 0).lo, 20, 9);
    } else r = a;
    return v < 0 ? -r : r;
}

/* a linear sRGB value (1/2^40) clipped to 0..1 and encoded, as a channel
 * in 1/10000 of 0..255: 12.92 v up to 0.0031308, else 1.055 v^(1/2.4) -
 * 0.055 */
static long long cp_encode(long long v) {
    if (v <= 0) return 0;
    if (v >= CP_Q) return 255 * CP_S;
    if (!cp_wlt(cp_wmul(cp_w(31308), CP_Q), cp_wmul(cp_w(v), 10000000)))
        return (long long)cp_wshr(cp_wmul(cp_w(v), 32946000), 40).lo;
    return (long long)cp_wshr(cp_wmul(cp_w(211 * cp_powq(v, 5, 12) - 11 * CP_Q), 12750), 40).lo;
}

/* x (three values in 1/2^40 of space sp, overwritten) as linear sRGB
 * l[] (1/2^40, not clipped) */
static void cp_space_lin(int sp, long long *x, long long *l) {
    int i, k;
    if (sp == CP_SP_LAB) {
        /* L a b to the D50 XYZ / white ratios: fy = (L + 16) / 116, fx =
         * fy + a / 500, fz = fy - b / 200; f^3 above 216 / 24389, else
         * (116 f - 16) / (24389 / 27); for y, L / (24389 / 27) up to L 8 */
        long long f[3], y = x[0];
        f[1] = cp_div(x[0] + 16 * CP_Q, 116, 0);
        f[0] = f[1] + cp_div(x[1], 500, 0);
        f[2] = f[1] - cp_div(x[2], 200, 0);
        for (k = 0; k < 3; k++) {
            long long cube = cp_qmul(cp_qmul(f[k], f[k]), f[k]);
            if (k == 1) x[k] = y > 8 * CP_Q ? cube : cp_div(y * 27, 24389, 0);
            else x[k] = cp_wlt(cp_wmul(cp_w(CP_Q), 216), cp_wmul(cp_w(cube), 24389)) ? cube
                        : cp_div((116 * f[k] - 16 * CP_Q) * 27, 24389, 0);
        }
    } else if (sp == CP_SP_OKLAB) {
        long long m[3];
        for (k = 0; k < 3; k++) {
            m[k] = 0;
            for (i = 0; i < 3; i++) m[k] += cp_qmul(cp_okl[3 * k + i], x[i]);
        }
        for (k = 0; k < 3; k++) x[k] = cp_qmul(cp_qmul(m[k], m[k]), m[k]);
    } else
        for (k = 0; k < 3; k++) x[k] = cp_decode(sp, x[k]);
    for (k = 0; k < 3; k++) {
        if (sp == CP_SP_LIN) { l[k] = x[k]; continue; }
        l[k] = 0;
        for (i = 0; i < 3; i++) l[k] += cp_qmul(cp_mat[sp][3 * k + i], x[i]);
    }
}

/* ---- Lot 103: gamut mapping (CSS Color 4 section 13.2) and the
 * conversions from linear sRGB to the other spaces (for color-mix() in
 * them). Linear sRGB to each space of cp_mat, in 1/2^40 (products of the
 * CSS Color 4 matrices, rounded once): lab gives XYZ D50 / white D50,
 * oklab LMS, the rgb spaces their linear values */
static const long long cp_inv[8][9] = {
    {497211971836LL, 439158371015LL, 163141273125LL, 244633851437LL, 788225651481LL, 66652164568LL, 18554610962LL,
     129367842700LL, 951589102230LL},
    {453242298902LL, 589703861074LL, 56565467800LL, 232990357618LL, 748437070956LL, 118084199202LL, 97089580641LL,
     309753139394LL, 692668907742LL},
    {453428478981LL, 393168139063LL, 198440725438LL, 233799059474LL, 786336278126LL, 79376290175LL, 21254459952LL,
     131056046354LL, 1045121153973LL},
    {479459354711LL, 423478518438LL, 157316424322LL, 244633851437LL, 788225651481LL, 66652164568LL, 15309494903LL,
     106742002429LL, 785160547949LL},
    {904306498005LL, 195205129771LL, 0LL, 36497407611LL, 1063014220165LL, 0LL, 18782551111LL, 79601827831LL,
     1001127248834LL},
    {875892664319LL, 614586943001LL, 30421049688LL, 0LL, 1099511627776LL, 0LL, 0LL, 45258040943LL, 1054253586833LL},
    {581946182056LL, 363008644302LL, 154556778988LL, 108154432670LL, 960391296442LL, 30965963536LL, 18554610962LL,
     129367842700LL, 951589102230LL},
    {689837878892LL, 362050529526LL, 47623219358LL, 75973273097LL, 1011045356595LL, 12492998084LL, 18022577639LL,
     96771655410LL, 984717394727LL}};
/* LMS' to OKLab (CSS Color 4), in 1/2^40 */
static const long long cp_lmsok[9] = {231396915121LL, 872591971295LL, -4477258640LL, 2174832386132LL,
                                      -2670265409259LL, 495433023127LL, 28481795897LL, 860666599741LL,
                                      -889148395638LL};

/* the cube root of v (1/2^40), odd */
static long long cp_cbrtq(long long v) {
    long long a = v < 0 ? -v : v, r;
    if (!a) return 0;
    r = cp_powq(a, 1, 3);
    return v < 0 ? -r : r;
}

/* the gamma-encoded value of a linear component v (1/2^40) of space sp,
 * odd around 0 (the inverse of cp_decode) */
static long long cp_encq(int sp, long long v) {
    long long a = v < 0 ? -v : v, r;
    if (!a) return 0;
    if (sp == CP_SP_P3) {
        /* sRGB curve: 12.92 v up to 0.0031308, else 1.055 v^(1/2.4) - 0.055 */
        if (!cp_wlt(cp_wmul(cp_w(31308), CP_Q), cp_wmul(cp_w(a), 10000000))) r = cp_div(a * 323, 25, 0);
        else r = cp_div(211 * cp_powq(a, 5, 12) - 11 * CP_Q, 200, 0);
    } else if (sp == CP_SP_A98) r = cp_powq(a, 256, 563);
    else if (sp == CP_SP_PRO) r = a * 512 >= CP_Q ? cp_powq(a, 5, 9) : 16 * a;     /* 1 / 512 */
    else if (sp == CP_SP_2020) {
        /* alpha v^0.45 - (alpha - 1) from beta, else 4.5 v */
        if (a >= 19850548305LL) r = cp_qmul(1208689643454LL, cp_powq(a, 9, 20)) - 109178015678LL;
        else r = cp_div(a * 9, 2, 0);
    } else r = a;
    return v < 0 ? -r : r;
}

/* linear sRGB l (1/2^40) as the three values x of space sp (inverse of
 * cp_space_lin) */
static void cp_lin_space(int sp, const long long *l, long long *x) {
    long long m[3];
    int i, k;
    for (k = 0; k < 3; k++) {
        if (sp == CP_SP_LIN) { m[k] = l[k]; continue; }
        m[k] = 0;
        for (i = 0; i < 3; i++) m[k] += cp_qmul(cp_inv[sp][3 * k + i], l[i]);
    }
    if (sp == CP_SP_OKLAB) {
        for (k = 0; k < 3; k++) m[k] = cp_cbrtq(m[k]);
        for (k = 0; k < 3; k++) {
            x[k] = 0;
            for (i = 0; i < 3; i++) x[k] += cp_qmul(cp_lmsok[3 * k + i], m[i]);
        }
    } else if (sp == CP_SP_LAB) {
        /* f(t) = t^(1/3) above 216 / 24389, else (24389 / 27 t + 16) / 116;
         * L = 116 fy - 16, a = 500 (fx - fy), b = 200 (fy - fz) */
        long long f[3];
        for (k = 0; k < 3; k++)
            f[k] = cp_wlt(cp_wmul(cp_w(CP_Q), 216), cp_wmul(cp_w(m[k]), 24389)) ? cp_cbrtq(m[k])
                   : cp_div((long long)cp_wdiv(cp_wmul(cp_w(m[k]), 24389), 27, 0).lo + 16 * CP_Q, 116, 0);
        x[0] = 116 * f[1] - 16 * CP_Q;
        x[1] = 500 * (f[0] - f[1]);
        x[2] = 200 * (f[1] - f[2]);
    } else
        for (k = 0; k < 3; k++) x[k] = cp_encq(sp, m[k]);
}

static int cp_ingamut(const long long *l) {
    int k;
    for (k = 0; k < 3; k++) if (l[k] < 0 || l[k] > CP_Q) return 0;
    return 1;
}

/* (deltaE OK)^2 between two OKLab colours, in 1/2^80 */
static cp_w_t cp_de2(const long long *p, const long long *q) {
    cp_w_t s = cp_w(0);
    int k;
    for (k = 0; k < 3; k++) s = cp_wadd(s, cp_wmul(cp_w(p[k] - q[k]), p[k] - q[k]));
    return s;
}

/* the OKLab of clip(l) into lc, clip(l) into cl */
static void cp_clip_ok(const long long *l, long long *cl, long long *lc) {
    int k;
    for (k = 0; k < 3; k++) cl[k] = cp_clampll(l[k], 0, CP_Q);
    cp_lin_space(CP_SP_OKLAB, cl, lc);
}

/* CSS Color 4 13.2.2 (binary search of the OKLCh chroma, JND 0.02,
 * epsilon 0.0001): l (linear sRGB, 1/2^40) into the sRGB gamut; ok, the
 * OKLab of l when it is known exactly (oklab(), oklch()), else 0. The
 * in-gamut test comes first: an sRGB colour has 0 < L < 1 but black and
 * white, which map to themselves. The chroma is scaled by t (0..1, hue
 * kept); the search stops once (tmax - tmin) C <= 0.0001, compared
 * squared. E < JND as E^2 2500 < 2^80, JND - E < epsilon as E^2 10^8 >
 * 39601 2^80 */
static void cp_gamut(long long *l, const long long *ok) {
    long long o[3], cur[3], x[3], lc[3], cl[3], lcur[3], t, tmin = 0, tmax = CP_Q;
    int k, mig = 1, it;
    cp_w_t c2, e, j2 = cp_wshl(cp_w(1), 80);
    if (cp_ingamut(l)) return;
    if (ok) for (k = 0; k < 3; k++) o[k] = ok[k];
    else cp_lin_space(CP_SP_OKLAB, l, o);
    if (o[0] >= CP_Q || o[0] <= 0) {
        for (k = 0; k < 3; k++) l[k] = o[0] > 0 ? CP_Q : 0;
        return;
    }
    cp_clip_ok(l, cl, lc);
    if (cp_wlt(cp_wmul(cp_de2(lc, o), 2500), j2)) {
        for (k = 0; k < 3; k++) l[k] = cl[k];
        return;
    }
    c2 = cp_wadd(cp_wmul(cp_w(o[1]), o[1]), cp_wmul(cp_w(o[2]), o[2]));
    for (it = 0; cp_wlt(cp_wshl(cp_w(1), 80 + 2 * it), cp_wmul(c2, 100000000)); it++) {
        t = (tmin + tmax) / 2;
        cur[0] = o[0];
        cur[1] = cp_qmul(o[1], t);
        cur[2] = cp_qmul(o[2], t);
        for (k = 0; k < 3; k++) x[k] = cur[k];
        cp_space_lin(CP_SP_OKLAB, x, lcur);
        if (mig && cp_ingamut(lcur)) { tmin = t; continue; }
        cp_clip_ok(lcur, cl, lc);
        e = cp_de2(lc, cur);
        if (cp_wlt(cp_wmul(e, 2500), j2)) {
            if (cp_wlt(cp_wmul(j2, 39601), cp_wmul(e, 100000000))) break;
            mig = 0;
            tmin = t;
        } else tmax = t;
    }
    for (k = 0; k < 3; k++) l[k] = cl[k];
}

/* x (three values in 1/2^40 of space sp, overwritten) as sRGB channels
 * c[]; Lot 103: gamut-mapped (ok: x is OKLab), the linear values before
 * the mapping into lin when it is not 0 */
static void cp_space_rgb(int sp, long long *x, long long *c, long long *lin) {
    long long l[3], ok[3];
    int k;
    for (k = 0; k < 3; k++) ok[k] = x[k];
    cp_space_lin(sp, x, l);
    if (lin) for (k = 0; k < 3; k++) lin[k] = l[k];
    cp_gamut(l, sp == CP_SP_OKLAB ? ok : 0);
    for (k = 0; k < 3; k++) c[k] = cp_encode(l[k]);
}

/* cos and sin of h (1/10000 deg, any value: the quadrant is k mod 4) in
 * 1/2^40 */
static void cp_cossin(long long h, long long *c, long long *s) {
    long long k = cp_div(h + 450000, 900000, 1), r, sn, cs, t;
    int q = (int)(k & 3);
    r = (long long)cp_wdiv(cp_wmul(cp_w(h - k * 900000), CP_PIO2), 900000, 0).lo;
    cp_sincos(r, &sn, &cs);
    for (; q > 0; q--) { t = sn; sn = cs; cs = -t; }
    *c = (long long)cp_wshr(cp_w(cs), 22).lo;
    *s = (long long)cp_wshr(cp_w(sn), 22).lo;
}

static int cp_rgba_d(const char *v, cp_rgba_t *o, int depth);

/* s[*a..*b) without the spaces around it */
static void cp_trim(const char *s, int *a, int *b) {
    while (*a < *b && cp_sp(s[*a])) (*a)++;
    while (*b > *a && cp_sp(s[*b - 1])) (*b)--;
}

/* the top-level parts of s[a..b) split at sep (',' or a space run):
 * their ranges, trimmed; the count, -1 over max. An unbalanced
 * parenthesis leaves a stray '(' or ')' in a part, which no colour form
 * takes, so it is not refused here */
static int cp_split(const char *s, int a, int b, char sep, int *pa, int *pb, int max) {
    int n = 0, depth = 0, i, st = a;
    for (i = a; i <= b; i++) {
        int cut = i == b;
        if (!cut) {
            if (s[i] == '(') depth++;
            else if (s[i] == ')') depth--;
            else if (!depth && (sep == ',' ? s[i] == ',' : cp_sp(s[i]))) cut = 1;
        }
        if (!cut) continue;
        if (sep != ',' && i < b && i == st) { st = i + 1; continue; }     /* a run of spaces */
        if (n >= max) return -1;
        pa[n] = st; pb[n] = i;
        cp_trim(s, &pa[n], &pb[n]);
        n++;
        st = i + 1;
    }
    return n;
}

/* Lot 100: one color-mix() item s[a..b): a colour and an optional
 * percentage before or after it; *p -1 when there is none */
static int cp_mix_item(const char *s, int a, int b, cp_rgba_t *o, long long *p, int depth) {
    char item[WF_CSSPROP_VAL];
    int pa[3], pb[3], n = cp_split(s, a, b, ' ', pa, pb, 3), c = 0, k, pct = 0;
    if (n < 1 || n > 2) return 0;
    *p = -1;
    if (n == 2) {
        int q = s[pb[0] - 1] == '%' ? 0 : 1;      /* the percentage: a top-level token ends with '%' */
        /* a token that is not a number leaves *p at -1 (and pct 0) */
        (void)cp_tok_num(s, pa[q], pb[q], p, &pct);
        if (!pct || *p < 0 || *p > 100 * CP_S) return 0;
        c = 1 - q;
    }
    if (pb[c] - pa[c] >= (int)sizeof(item)) return 0;
    for (k = pa[c]; k < pb[c]; k++) item[k - pa[c]] = s[k];
    item[pb[c] - pa[c]] = 0;
    return cp_rgba_d(item, o, depth + 1);
}

/* Lot 102: the missing components of two colours before they are mixed
 * or interpolated (CSS Color 4, interpolating with missing components):
 * one missing in one colour takes the other's value, one missing in both
 * is 0 in both */
static void cp_carry(cp_rgba_t *x, cp_rgba_t *y) {
    int k;
    for (k = 0; k < 4; k++) {
        long long *p = k < 3 ? &x->c[k] : &x->a, *q = k < 3 ? &y->c[k] : &y->a;
        int mx = (x->miss >> k) & 1, my = (y->miss >> k) & 1;
        if (mx && my) *p = *q = 0;
        else if (mx) *p = *q;
        else if (my) *q = *p;
    }
}

/* Lot 103: the spaces of color-mix() (CSS Color 5): their cp_mat index
 * (-1 srgb, the Lot 100 path; -2 hsl and hwb, from the channels), the
 * analogous set of each component (CSS Color 4 12.2; -1 none) and the
 * hue component of the polar ones (-1) */
static const char *const cp_mixn[] = {"srgb", "srgb-linear", "display-p3", "a98-rgb", "prophoto-rgb", "rec2020",
                                      "lab", "oklab", "xyz", "xyz-d50", "xyz-d65", "hsl", "hwb", "lch", "oklch", 0};
static const signed char cp_mixs[] = {-1, CP_SP_LIN, CP_SP_P3, CP_SP_A98, CP_SP_PRO, CP_SP_2020, CP_SP_LAB,
                                      CP_SP_OKLAB, CP_SP_D65, CP_SP_D50, CP_SP_D65, -2, -2, CP_SP_LAB, CP_SP_OKLAB};
static const signed char cp_mixc[][3] = {{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
                                         {4, 7, 8}, {4, 7, 8}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {6, 5, 4},
                                         {6, -1, -1}, {4, 5, 6}, {4, 5, 6}};
static const signed char cp_mixh[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 2, 2};
static const char *const cp_hues[] = {"shorter", "longer", "increasing", "decreasing", 0};

/* the linear sRGB values of x (1/2^40): its own, else its channels decoded */
static void cp_lin_of(const cp_rgba_t *x, long long *l) {
    int k;
    for (k = 0; k < 3; k++) l[k] = x->lin_ok ? x->lin[k] : cp_decode(CP_SP_P3, cp_div(x->c[k] * CP_Q, 255 * CP_S, 0));
}

/* the components of x in mix space m into v (hue in 1/10000 deg: hsl and
 * hwb 0..3600000, lch and oklch -1800000..1800000, any turn being the same
 * for the hue methods, cp_hsl_rgb, cp_hwb_rgb and cp_cossin; hsl and hwb
 * in 1/10000 %, from the channels, exact; the others in 1/2^40); a
 * powerless hue (CSS Color 4: hsl and hwb of a grey,
 * an lch chroma at most 0.0015, an oklch one at most 0.000004) is missing
 * (bit 6 into *miss) */
static void cp_mix_comps(int m, const cp_rgba_t *x, long long *v, int *miss) {
    long long l[3];
    int k;
    if (cp_mixs[m] == -2) {
        long long r = x->c[0], g = x->c[1], b = x->c[2], u = 255 * CP_S;
        long long mx = r > g ? (r > b ? r : b) : (g > b ? g : b), mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
        long long d = mx - mn, den = mx + mn < 2 * u - mx - mn ? mx + mn : 2 * u - mx - mn;
        if (!d) { v[0] = 0; *miss |= 64; }
        else if (mx == r) v[0] = cp_div(600000 * (g - b) + (g < b ? 3600000 : 0) * d, d, 0);
        else if (mx == g) v[0] = cp_div(600000 * (b - r) + 1200000 * d, d, 0);
        else v[0] = cp_div(600000 * (r - g) + 2400000 * d, d, 0);
        if (m == 11) {
            v[1] = den > 0 ? cp_div(d * 1000000, den, 0) : 0;
            v[2] = cp_div((mx + mn) * 1000000, 2 * u, 0);
        } else {
            v[1] = cp_div(mn * 1000000, u, 0);
            v[2] = cp_div((u - mx) * 1000000, u, 0);
        }
        return;
    }
    if (cp_mixs[m] == CP_SP_OKLAB && x->ok_ok) for (k = 0; k < 3; k++) v[k] = x->ok[k];   /* Lot 106 */
    else {
        cp_lin_of(x, l);
        cp_lin_space(cp_mixs[m], l, v);
    }
    if (cp_mixh[m] == 2) {
        long long a = v[1], b = v[2];
        v[1] = (long long)cp_isqrt(cp_wadd(cp_wmul(cp_w(a), a), cp_wmul(cp_w(b), b)));
        v[2] = cp_atan2(b, a);
        if (m == 13 ? v[1] * 10000 <= 15 * CP_Q : v[1] * 1000000 <= 4 * CP_Q) *miss |= 64;
    }
}

/* Lot 100: color-mix(in srgb, C1 [P1], C2 [P2]) (CSS Color 5) at v[0..n):
 * the percentages default to 50% each, or to 100% minus the other; their
 * sum S (> 0) normalises them; the colours are mixed premultiplied and an
 * S under 100% scales the alpha. With P in 1/10000 %: A = a1 P1 + a2 P2,
 * each channel (c1 a1 P1 + c2 a2 P2) / A (0 when A is 0), the alpha A / S,
 * or A / 100% when S is under it; each rounded once. Lot 103: in any of
 * cp_mixn, the polar ones with a hue method (shorter by default); each
 * colour is converted from its linear sRGB values (hsl and hwb from its
 * channels), a component missing in one colour (its analogous set, or a
 * powerless hue) takes the other's value, the hue is not premultiplied,
 * and the result is converted back (gamut-mapped); the missing
 * components of both stay missing */
static int cp_mix_core(int m, int hm, cp_rgba_t *x, const long long *p, cp_rgba_t *o);

/* Lot 108: the channel keyword s[a..b) of relative function fn (0..2 its
 * channels in order: r g b, h s l, l a b, l c h; 3 alpha), else -1 */
static int cp_rel_kw(int fn, const char *s, int a, int b) {
    static const char kw[4][4] = {"rgb", "hsl", "lab", "lch"};
    int k;
    if (b - a == 1)
        for (k = 0; k < 3; k++) if (cp_low(s[a]) == kw[fn][k]) return k;
    return cp_eqn(s + a, b - a, "alpha") ? 3 : -1;
}

/* Lot 108: the values of the keywords of function fn for the origin x,
 * in 1/10000: r g b of 0..255 (its channels, gamut-mapped as painted);
 * h in deg (0..360, 0 when powerless), s and l of 0..100 (from the
 * channels, exact); oklab l a b and oklch l c (from its OKLab values,
 * exact for an oklab() or oklch() origin, else from its linear sRGB
 * values) and h in deg (0..360, 0 when powerless); alpha of 0..1. A
 * missing component of the origin is 0 */
static void cp_rel_kv(int fn, const cp_rgba_t *x, long long *kv) {
    long long c[3];
    int k, miss = 0;
    kv[3] = x->a;
    if (!fn) {
        for (k = 0; k < 3; k++) kv[k] = x->c[k];
        return;
    }
    cp_mix_comps(fn == 1 ? 11 : fn == 2 ? 7 : 14, x, c, &miss);
    if (fn == 1) {
        for (k = 0; k < 3; k++) kv[k] = c[k];
        return;
    }
    kv[0] = cp_div(c[0] * CP_S, CP_Q, 0);
    kv[1] = cp_div(c[1] * CP_S, CP_Q, 0);
    if (fn == 2) kv[2] = cp_div(c[2] * CP_S, CP_Q, 0);
    else kv[2] = (miss & 64) ? 0 : c[2] < 0 ? c[2] + 3600000 : c[2];
}

/* Lot 108: one argument s[a..b) of a relative colour into out (cap
 * bytes): a keyword as its value; a math function with the keywords
 * replaced by their values (a negative one in parentheses) as its result
 * ("N[.dddd]" and the unit); any other token as it is. -1 when it does
 * not fit or the math function has no known value */
static int cp_rel_arg(int fn, const char *s, int a, int b, const long long *kv, char *out, int cap) {
    char t[WF_CSSPROP_VAL], f[24];
    int len = 0, i = a, kw = cp_rel_kw(fn, s, a, b), l;
    cp_q_t q;
    out[0] = 0;
    if (kw >= 0) return cp_fmt(kv[kw], "", out, cap);
    for (l = a; l < b && s[l] != '('; l++) {}
    if (l == b) return cp_put(out, &len, cap, s + a, b - a) ? len : -1;
    t[0] = 0;
    while (i < b) {
        int j = i;
        if (cp_ident(s[i]) && !cp_digit(s[i]) && (i == a || !(cp_ident(s[i - 1]) || s[i - 1] == '.'))) {
            while (j < b && cp_ident(s[j])) j++;
            kw = cp_rel_kw(fn, s, i, j);
            if (kw >= 0) {
                if (cp_fmt(kv[kw], "", f, (int)sizeof(f)) < 0) return -1;
                if ((kv[kw] < 0 && !cp_put(t, &len, (int)sizeof(t), "(", 1)) ||
                    !cp_put(t, &len, (int)sizeof(t), f, cp_len(f)) ||
                    (kv[kw] < 0 && !cp_put(t, &len, (int)sizeof(t), ")", 1)))
                    return -1;
            } else if (!cp_put(t, &len, (int)sizeof(t), s + i, j - i)) return -1;
            i = j;
            continue;
        }
        if (!cp_put(t, &len, (int)sizeof(t), s + i, 1)) return -1;
        i++;
    }
    cp_mwhole(t, &q);
    if (!q.t || !cp_known(&q)) return -1;
    return cp_fmt_q(&q, out, cap);
}

/* Lot 108: the relative colour v[0..n) of function fn, its origin from
 * v[i] (after "from"): the origin, then three arguments and an optional
 * "/ alpha" (spaces only; a comma fails), each a number, percentage, angle,
 * none, a keyword of fn or a math function of them; the alpha is the
 * origin's when left out. The colour is the absolute one written with
 * those values (so it is clamped and converted as that one is) */
static int cp_rel(const char *v, int n, int fn, int i, cp_rgba_t *o, int depth) {
    static const char *const names[] = {"rgb(", "hsl(", "oklab(", "oklch("};
    char org[WF_CSSPROP_VAL], buf[WF_CSSPROP_VAL], arg[WF_CSSPROP_VAL];
    int ta[5], tb[5], nt = 0, slash = -1, len = 0, k, d, a;
    long long kv[4];
    cp_rgba_t x;
    if (depth > 3) return 0;
    for (;;) {
        while (i < n - 1 && cp_sp(v[i])) i++;
        if (i >= n - 1) break;
        if (v[i] == '/') {
            if (slash >= 0) return 0;
            slash = nt;
            i++;
            continue;
        }
        if (nt >= 5) return 0;
        a = i;
        d = 0;
        while (i < n - 1 && (d || (!cp_sp(v[i]) && v[i] != '/'))) {
            /* a comma or a stray ')' at the top level makes the absolute
             * colour below invalid (a legacy comma form takes no "/"; a
             * negative depth runs to the end and fails below) */
            if (v[i] == '(') d++;
            else if (v[i] == ')') d--;
            i++;
        }
        if (d) return 0;
        ta[nt] = a;
        tb[nt] = i;
        nt++;
    }
    if (!(nt == 4 && slash < 0) && !(nt == 5 && slash == 4)) return 0;
    if (tb[0] - ta[0] >= (int)sizeof(org)) return 0;
    for (k = ta[0]; k < tb[0]; k++) org[k - ta[0]] = v[k];
    org[tb[0] - ta[0]] = 0;
    if (!cp_rgba_d(org, &x, depth + 1)) return 0;
    cp_rel_kv(fn, &x, kv);
    if (!cp_put(buf, &len, (int)sizeof(buf), names[fn], cp_len(names[fn]))) return 0;
    for (k = 1; k < 5; k++) {
        if (k == 4 && nt == 4) {
            if (cp_fmt(kv[3], "", arg, (int)sizeof(arg)) < 0) return 0;
        } else if (cp_rel_arg(fn, v, ta[k], tb[k], kv, arg, (int)sizeof(arg)) < 0) return 0;
        if (!cp_put(buf, &len, (int)sizeof(buf), k == 4 ? " / " : k > 1 ? " " : "", k == 4 ? 3 : k > 1) ||
            !cp_put(buf, &len, (int)sizeof(buf), arg, cp_len(arg)))
            return 0;
    }
    if (!cp_put(buf, &len, (int)sizeof(buf), ")", 1)) return 0;
    return cp_rgba_d(buf, o, depth + 1);
}

static int cp_mix(const char *v, int n, cp_rgba_t *o, int depth) {
    int pa[3], pb[3], ha[4], hb[4], k, m = -1, hm = 0, nh;
    long long p[2], sum;
    cp_rgba_t x[2];
    if (depth > 3 || cp_split(v, 10, n - 1, ',', pa, pb, 3) != 3) return 0;
    /* "in", the space, then a hue method and "hue" for a polar space */
    nh = cp_split(v, pa[0], pb[0], ' ', ha, hb, 4);
    if (nh != 2 && nh != 4) return 0;
    if (!cp_eqn(v + ha[0], hb[0] - ha[0], "in")) return 0;
    for (k = 0; cp_mixn[k] && m < 0; k++) if (cp_eqn(v + ha[1], hb[1] - ha[1], cp_mixn[k])) m = k;
    if (m < 0) return 0;
    if (nh == 4) {
        for (hm = 0; cp_hues[hm] && !cp_eqn(v + ha[2], hb[2] - ha[2], cp_hues[hm]); hm++) {}
        if (cp_mixh[m] < 0 || !cp_hues[hm] || !cp_eqn(v + ha[3], hb[3] - ha[3], "hue")) return 0;
    }
    for (k = 0; k < 2; k++) if (!cp_mix_item(v, pa[k + 1], pb[k + 1], &x[k], &p[k], depth)) return 0;
    if (p[0] < 0 && p[1] < 0) p[0] = p[1] = 50 * CP_S;
    else if (p[0] < 0) p[0] = 100 * CP_S - p[1];
    else if (p[1] < 0) p[1] = 100 * CP_S - p[0];
    sum = p[0] + p[1];
    if (sum <= 0) return 0;
    return cp_mix_core(m, hm, x, p, o);
}

/* Lot 106: the mix of x[0] and x[1] with weights p (1/10000 %, sum S
 * over 0) in mix space m with hue method hm: cp_mix after its parsing
 * (Lot 100 and Lot 103). A weight may be negative (an interpolation past
 * its ends, wf_cssprop_interp): the premultiplied components are divided
 * by the weighted alpha sum as it is (CSS Color 4 12.3), and the alpha is
 * clamped to 0..1 afterwards; with a negative weight a component is
 * kept within +-32 (an alpha sum near 0 far past the ends of an Oklab
 * interpolation: no overflow, the gamut mapping takes such a colour to
 * black, white or the gamut edge) */
static int cp_mix_core(int m, int hm, cp_rgba_t *x, const long long *p, cp_rgba_t *o) {
    int k, mi[2];
    long long cv[2][3], an, sum = p[0] + p[1];
    o->ok_ok = 0;
    if (!m) {     /* o->lin_ok is 0 (cp_rgba_d) */
        cp_carry(&x[0], &x[1]);     /* Lot 102: a component missing in both stays missing */
        o->miss = x[0].miss & x[1].miss & 15;
        an = x[0].a * p[0] + x[1].a * p[1];
        for (k = 0; k < 3; k++)
            o->c[k] = an ? cp_div(x[0].c[k] * x[0].a * p[0] + x[1].c[k] * x[1].a * p[1], an, 0) : 0;
        o->a = cp_div(an, sum >= 100 * CP_S ? sum : 100 * CP_S, 0);
        return 1;
    }
    for (k = 0; k < 2; k++) {
        int j, cat;
        mi[k] = 0;
        cp_mix_comps(m, &x[k], cv[k], &mi[k]);
        mi[k] = cp_mixh[m] >= 0 && (mi[k] & 64) ? 1 << cp_mixh[m] : 0;
        for (j = 0; j < 3; j++) {
            cat = cp_mixc[m][j];
            if (cat >= 0 && ((x[k].miss >> cat) & 1)) mi[k] |= 1 << j;
        }
        if ((x[k].miss >> 3) & 1) mi[k] |= 8;
    }
    /* carry (CSS Color 4 12.3), the alpha as component 3 */
    o->miss = 0;
    for (k = 0; k < 4; k++) {
        long long *q0 = k < 3 ? &cv[0][k] : &x[0].a, *q1 = k < 3 ? &cv[1][k] : &x[1].a;
        int m0 = (mi[0] >> k) & 1, m1 = (mi[1] >> k) & 1;
        if (m0 && m1) {
            *q0 = *q1 = 0;
            if (k == 3) o->miss |= 8;
            else if (cp_mixc[m][k] >= 0) o->miss |= 1 << cp_mixc[m][k];
        } else if (m0) *q0 = *q1;
        else if (m1) *q1 = *q0;
    }
    an = x[0].a * p[0] + x[1].a * p[1];
    for (k = 0; k < 3; k++) {
        cp_w_t q, lim = cp_w(32 * CP_Q);
        if (k == cp_mixh[m]) {
            /* CSS Color 4 12.4: the hue method, then the weighted mean
             * (not reduced modulo 360: the conversions back take any
             * turn) */
            long long h0 = cv[0][k], h1 = cv[1][k], dh = h1 - h0;
            if (hm == 0) { if (dh > 1800000) h0 += 3600000; else if (dh < -1800000) h1 += 3600000; }
            else if (hm == 1) { if (dh > 0 && dh < 1800000) h0 += 3600000; else if (dh > -1800000 && dh <= 0) h1 += 3600000; }
            else if (hm == 2) { if (h1 < h0) h1 += 3600000; }
            else if (h0 < h1) h0 += 3600000;
            cv[0][k] = cp_div(h0 * p[0] + h1 * p[1], sum, 0);
            continue;
        }
        if (!an) { cv[0][k] = 0; continue; }
        q = cp_wdiv(cp_wadd(cp_wmul(cp_w(cv[0][k]), x[0].a * p[0]), cp_wmul(cp_w(cv[1][k]), x[1].a * p[1])), an, 0);
        if (p[0] < 0 || p[1] < 0) {     /* only past the ends (Oklab: wf_cssprop_interp) */
            if (cp_wlt(lim, q)) q = lim;
            else if (cp_wlt(q, cp_wneg(lim))) q = cp_wneg(lim);
        }
        cv[0][k] = (long long)q.lo;
    }
    o->a = cp_clampll(cp_div(an, sum >= 100 * CP_S ? sum : 100 * CP_S, 0), 0, CP_S);
    if (cp_mixs[m] == -2) {
        long long c1 = cp_clampll(cv[0][1], 0, 1000000), c2 = cp_clampll(cv[0][2], 0, 1000000);
        if (m == 11) cp_hsl_rgb(cv[0][0], c1, c2, o->c);
        else cp_hwb_rgb(cv[0][0], c1, c2, o->c);
        return 1;
    }
    if (cp_mixh[m] == 2) {
        long long cs, sn, c = cv[0][1];
        cp_cossin(cv[0][2], &cs, &sn);
        cv[0][1] = cp_qmul(c, cs);
        cv[0][2] = cp_qmul(c, sn);
    }
    if (cp_mixs[m] == CP_SP_OKLAB) {
        for (k = 0; k < 3; k++) o->ok[k] = cv[0][k];
        o->ok_ok = 1;
    }
    cp_space_rgb(cp_mixs[m], cv[0], o->c, o->lin);
    o->lin_ok = 1;
    return 1;
}

static int cp_rgba(const char *v, cp_rgba_t *o) { return cp_rgba_d(v, o, 0); }

/* Lot 101: the colour currentcolor stands for, when one is set */
static int cp_cur_on;
static cp_rgba_t cp_cur;

void wf_cssprop_set_current(int on, unsigned int rgb, int alpha) {
    int k;
    cp_cur_on = on;
    for (k = 0; k < 3; k++) cp_cur.c[k] = (long long)((rgb >> (16 - 8 * k)) & 255u) * CP_S;
    cp_cur.a = cp_div((long long)alpha * CP_S, 255, 0);
}

static int cp_rgba_d(const char *v, cp_rgba_t *o, int depth) {
    int n = cp_len(v), k, i;
    o->a = CP_S;
    o->miss = 0;
    o->lin_ok = 0;
    o->ok_ok = 0;
    if (cp_eqn(v, n, "transparent")) { o->c[0] = o->c[1] = o->c[2] = 0; o->a = 0; return 1; }
    if (cp_eqn(v, n, "currentcolor")) {
        /* Lot 101: the colour set by wf_cssprop_set_current, else none */
        *o = cp_cur;
        return cp_cur_on;
    }
    k = cp_named_at(v, n);      /* Lot 100: every named colour */
    if (k >= 0) {
        for (n = 0; n < 3; n++) o->c[n] = (long long)((cp_named[k].rgb >> (16 - 8 * n)) & 255u) * CP_S;
        return 1;
    }
    if (v[0] == '#') {
        int d = n == 4 || n == 5 ? 1 : 2, i;
        if (n != 4 && n != 5 && n != 7 && n != 9) return 0;
        for (k = 1; k < n; k++) if (!cp_hex(v[k])) return 0;
        for (i = 0; i < 4; i++) {
            int x;
            if (1 + i * d >= n) break;
            x = d == 1 ? cp_hexv(v[1 + i]) * 17 : cp_hexv(v[1 + 2 * i]) * 16 + cp_hexv(v[2 + 2 * i]);
            if (i < 3) o->c[i] = x * CP_S;
            else o->a = cp_div((long long)x * CP_S, 255, 0);
        }
        return 1;
    }
    if (!n || v[n - 1] != ')') return 0;     /* Lot 100: a function ends at its ')' */
    k = cp_rel_head(v, n, &i);      /* Lot 108: relative colours */
    if (k >= 0) return cp_rel(v, n, k, i, o, depth);
    if ((n > 5 && cp_eqn(v, 4, "rgb(")) || (n > 6 && cp_eqn(v, 5, "rgba("))) {
        int ta[4], tb[4], got = cp_color_toks(v, n, v[3] == '(' ? 4 : 5, ta, tb), pct;
        long long x;
        for (k = 0; k < got; k++) {
            if (cp_none(v, ta[k], tb[k])) {
                /* Lot 102: none, in the space form only: a missing
                 * component, 0 */
                if (cp_comma_after(v, tb[0], n)) return 0;
                if (k < 3) o->c[k] = 0;
                else o->a = 0;
                o->miss |= 1 << k;
                continue;
            }
            if (!cp_tok_num(v, ta[k], tb[k], &x, &pct)) return 0;
            if (k < 3) o->c[k] = cp_clampll(pct ? cp_div(x * 255, 100, 0) : x, 0, 255 * CP_S);
            else o->a = cp_clampll(pct ? cp_div(x, 100, 0) : x, 0, CP_S);
        }
        return got >= 3;
    }
    if ((n > 5 && cp_eqn(v, 4, "hsl(")) || (n > 6 && cp_eqn(v, 5, "hsla("))) {
        /* Lot 98: hsl() and hsla(): the hue, saturation and lightness
         * (percentages; numbers too in the space form), then the alpha */
        int ta[4], tb[4], got = cp_color_toks(v, n, v[3] == '(' ? 4 : 5, ta, tb), pct, comma;
        long long h, sl[2], x;
        cp_q_t q;
        if (got != 3 && got != 4) return 0;
        comma = cp_comma_after(v, tb[0], n);    /* the legacy form: percentages only */
        /* Lot 102: none (the space form only) is 0; Lot 103: a missing
         * hue (bit 6), saturation (5) or lightness (4) */
        for (k = 0; k < got; k++) if (comma && cp_none(v, ta[k], tb[k])) return 0;
        if (cp_none(v, ta[0], tb[0])) { q.v = 0; o->miss |= 64; }
        else if (!cp_hue(v, ta[0], tb[0], &q)) return 0;
        h = q.v - cp_div(q.v, 3600000, 1) * 3600000;   /* 0..359.9999deg */
        for (k = 1; k < 3; k++) {
            if (cp_none(v, ta[k], tb[k])) { x = 0; pct = 1; o->miss |= k == 1 ? 32 : 16; }
            else if (!cp_tok_num(v, ta[k], tb[k], &x, &pct) || (comma && !pct)) return 0;
            sl[k - 1] = cp_clampll(x, 0, 100 * CP_S);
        }
        if (got == 4 && !cp_alpha_tok(v, ta[3], tb[3], o)) return 0;
        cp_hsl_rgb(h, sl[0], sl[1], o->c);
        return 1;
    }
    if (n > 5 && cp_eqn(v, 4, "hwb(")) {
        /* Lot 100: hwb(H W B [/ A]), spaces only; W and B percentages or
         * numbers (the same scale), clamped to 0..100% */
        int ta[4], tb[4], got = cp_color_toks(v, n, 4, ta, tb), pct;
        long long h, wb[2], x;
        cp_q_t q;
        if ((got != 3 && got != 4) || cp_comma_after(v, tb[0], n)) return 0;
        if (cp_none(v, ta[0], tb[0])) { q.v = 0; o->miss |= 64; }     /* Lot 102: none is 0; Lot 103: hue */
        else if (!cp_hue(v, ta[0], tb[0], &q)) return 0;
        h = q.v - cp_div(q.v, 3600000, 1) * 3600000;
        for (k = 1; k < 3; k++) {
            if (cp_none(v, ta[k], tb[k])) x = 0;
            else if (!cp_tok_num(v, ta[k], tb[k], &x, &pct)) return 0;
            wb[k - 1] = cp_clampll(x, 0, 100 * CP_S);
        }
        if (got == 4 && !cp_alpha_tok(v, ta[3], tb[3], o)) return 0;
        cp_hwb_rgb(h, wb[0], wb[1], o->c);
        return 1;
    }
    if ((n > 5 && (cp_eqn(v, 4, "lab(") || cp_eqn(v, 4, "lch("))) ||
        (n > 7 && (cp_eqn(v, 6, "oklab(") || cp_eqn(v, 6, "oklch(")))) {
        /* Lot 101: lab(L a b [/ A]), lch(L C H [/ A]) and their OK forms,
         * spaces only; L clamped to 0..100 (0..1), C to 0 and over; a
         * percentage of L is of 100 (1), of a and b of 125 (0.4), of C of
         * 150 (0.4); a, b and C are kept within 10000 (10) */
        int ok = cp_low(v[0]) == 'o', lch = cp_low(v[ok ? 4 : 2]) == 'h', pct;
        int ta[4], tb[4], got = cp_color_toks(v, n, ok ? 6 : 4, ta, tb);
        long long x[3], y, cs, sn, lim = ok ? 10 * CP_Q : 10000 * CP_Q;
        cp_q_t q;
        if ((got != 3 && got != 4) || cp_comma_after(v, tb[0], n)) return 0;
        for (k = 0; k < 3; k++) {
            if (k == 2 && lch) {
                if (cp_none(v, ta[2], tb[2])) { q.v = 0; o->miss |= 64; }     /* Lot 102: 0; Lot 103: hue */
                else if (!cp_hue(v, ta[2], tb[2], &q)) return 0;
                cp_cossin(q.v, &cs, &sn);
                x[2] = cp_qmul(x[1], sn);
                x[1] = cp_qmul(x[1], cs);
                continue;
            }
            if (cp_none(v, ta[k], tb[k])) {
                /* Lot 103: L lightness (bit 4), C colourfulness (5), a and
                 * b the opponent pairs (7, 8) */
                y = 0; pct = 0;
                o->miss |= !k ? 16 : lch ? 32 : k == 1 ? 128 : 256;
            } else if (!cp_tok_num(v, ta[k], tb[k], &y, &pct)) return 0;
            if (!k) x[0] = cp_clampll(cp_q40(y, pct, ok ? 1 : 100, 1), 0, ok ? CP_Q : 100 * CP_Q);
            else if (k == 1 && lch) x[1] = cp_clampll(cp_q40(y, pct, ok ? 2 : 150, ok ? 5 : 1), 0, lim);
            else x[k] = cp_clampll(cp_q40(y, pct, ok ? 2 : 125, ok ? 5 : 1), -lim, lim);
        }
        if (got == 4 && !cp_alpha_tok(v, ta[3], tb[3], o)) return 0;
        for (k = 0; k < 3; k++) o->ok[k] = x[k];
        o->ok_ok = ok;
        cp_space_rgb(ok ? CP_SP_OKLAB : CP_SP_LAB, x, o->c, o->lin);
        o->lin_ok = 1;
        return 1;
    }
    if (n > 7 && cp_eqn(v, 6, "color(")) {
        /* Lot 101: color(space c1 c2 c3 [/ A]); a percentage is of 1;
         * the components are kept within 100 */
        static const char *const sn[] = {"srgb", "srgb-linear", "display-p3", "a98-rgb", "prophoto-rgb",
                                          "rec2020", "xyz", "xyz-d65", "xyz-d50", 0};
        static const int sc[] = {-1, CP_SP_LIN, CP_SP_P3, CP_SP_A98, CP_SP_PRO, CP_SP_2020, CP_SP_D65, CP_SP_D65,
                                 CP_SP_D50};
        int i = 6, a0, ta[4], tb[4], got, pct, sp = -2;
        long long x[3], y;
        while (i < n - 1 && cp_sp(v[i])) i++;
        a0 = i;
        while (i < n - 1 && !cp_sp(v[i])) i++;
        for (k = 0; sn[k]; k++) if (cp_eqn(v + a0, i - a0, sn[k])) sp = sc[k];
        if (sp == -2) return 0;
        got = cp_color_toks(v, n, i, ta, tb);
        if ((got != 3 && got != 4) || cp_comma_after(v, tb[0], n)) return 0;
        for (k = 0; k < 3; k++) {
            /* Lot 102: none is 0 and a missing channel (each component of
             * an rgb or xyz space is the analogue of red, green, blue) */
            if (cp_none(v, ta[k], tb[k])) { y = 0; pct = 0; o->miss |= 1 << k; }
            else if (!cp_tok_num(v, ta[k], tb[k], &y, &pct)) return 0;
            if (sp < 0) o->c[k] = cp_clampll(pct ? cp_div(y * 255, 100, 0) : y * 255, 0, 255 * CP_S);
            else x[k] = cp_clampll(cp_q40(y, pct, 1, 1), -100 * CP_Q, 100 * CP_Q);
        }
        if (got == 4 && !cp_alpha_tok(v, ta[3], tb[3], o)) return 0;
        if (sp >= 0) {
            cp_space_rgb(sp, x, o->c, o->lin);
            o->lin_ok = 1;
        }
        return 1;
    }
    if (n > 11 && cp_eqn(v, 10, "color-mix(")) return cp_mix(v, n, o, depth);
    return 0;
}

static int cp_put_u(char *out, int *len, int cap, long long v) {
    char d[24];
    int k = 0;
    unsigned long long u = (unsigned long long)v, r;
    do { u = cp_udiv(u, 10, &r); d[k++] = (char)('0' + (int)r); } while (u && k < 20);
    if (*len + k >= cap) return 0;
    while (k) out[(*len)++] = d[--k];
    out[*len] = 0;
    return 1;
}

/* the interpolated colour text, -1 when a value is not a colour taken
 * here, -2 when cap is short. Lot 98: add sums the premultiplied colours
 * (the alpha clamped to 1, then the channels to 0..255) */
/* Lot 106: one of the legacy sRGB colour formats of CSS Color 4 12.1
 * (hex, named colours with transparent, rgb(), rgba(), hsl(), hsla() and
 * hwb(), in either syntax); currentcolor is one too here, as the reader
 * keeps it as an sRGB colour (wf_cssprop_set_current) */
static int cp_legacy(const char *v) {
    int n = cp_len(v);
    static const char *const f[] = {"rgb(", "rgba(", "hsl(", "hsla(", "hwb(", 0};
    int k;
    if (v[0] == '#' || cp_named_at(v, n) >= 0 || cp_eqn(v, n, "transparent") || cp_eqn(v, n, "currentcolor")) return 1;
    for (k = 0; f[k]; k++) if (n > cp_len(f[k]) && cp_eqn(v, cp_len(f[k]), f[k])) return 1;
    return 0;
}

static int cp_interp_color(const char *a, const char *b, int t, int add, char *out, int cap) {
    cp_rgba_t x, y;
    long long an, ch[3];
    int k, len = 0, alpha;
    if (!cp_rgba(a, &x) || !cp_rgba(b, &y)) return -1;
    if (!add && (!cp_legacy(a) || !cp_legacy(b))) {
        /* Lot 106: CSS Color 4 12.1: Oklab unless both colours are
         * legacy sRGB ones: color-mix(in oklab, a (100 - t)%, b t%)
         * (premultiplied, missing components carried by analogous set,
         * gamut-mapped back to sRGB), each channel rounded half away from
         * zero to 0..255 */
        cp_rgba_t xy[2], o;
        long long p[2];
        xy[0] = x; xy[1] = y;
        p[0] = (CP_S - t) * 100; p[1] = (long long)t * 100;
        (void)cp_mix_core(7, 0, xy, p, &o);
        for (k = 0; k < 3; k++) ch[k] = cp_clampll(cp_div(o.c[k], CP_S, 0), 0, 255);
        alpha = (int)o.a;
    } else {
        /* Lot 102: missing components, when interpolating (add takes the
         * values as they convert: CSS Color 4 4.4, a missing component is
         * 0 outside interpolation) */
        if (!add) cp_carry(&x, &y);
        /* alpha * 10^8: Aa 10^4 + (Ab - Aa) t, or (Aa + Ab) 10^4 clamped
         * to 1. Lot 107: an interpolation past its ends divides by the
         * alpha as it is and clamps it afterwards (CSS Color 4 12.3, as
         * the Oklab path since Lot 106) */
        an = add ? cp_clampll((x.a + y.a) * CP_S, 0, CP_S * CP_S) : x.a * CP_S + (y.a - x.a) * t;
        for (k = 0; k < 3; k++) {
            long long pa = x.c[k] * x.a, pb = y.c[k] * y.a;
            long long pn = add ? (pa + pb) * CP_S : pa * CP_S + (pb - pa) * t;
            ch[k] = an ? cp_clampll(cp_div(pn, an * CP_S, 0), 0, 255) : 0;
        }
        alpha = (int)cp_clampll(cp_div(an, CP_S, 0), 0, CP_S);
    }
    if (!cp_put(out, &len, cap, alpha == CP_S ? "rgb(" : "rgba(", alpha == CP_S ? 4 : 5)) return -2;
    for (k = 0; k < 3; k++) {
        if (k && !cp_put(out, &len, cap, ", ", 2)) return -2;
        if (!cp_put_u(out, &len, cap, ch[k])) return -2;
    }
    if (alpha != CP_S) {
        int m;
        if (!cp_put(out, &len, cap, ", ", 2)) return -2;
        m = cp_fmt(alpha, "", out + len, cap - len);
        if (m < 0) return -2;
        len += m;
    }
    if (!cp_put(out, &len, cap, ")", 1)) return -2;
    return len;
}

/* ---- Lot 99: transform lists added and accumulated -------------------- */


/* the transform function s[a..b), one exactly and already matched (by
 * cp_alt_index, so its name is known and it has 1 to 6 arguments): its
 * index in cp_tfns and its argument ranges without their spaces; the
 * count */
static int cp_tf_args(const char *s, int a, int b, int *fn, int *xa, int *xb) {
    int i = a, n = 0, depth = 0, st;
    while (s[i] != '(') i++;
    *fn = cp_tfn(s, a, i - a);
    st = ++i;
    for (; i < b; i++) {
        if (s[i] == '(') depth++;
        else if (s[i] == ')' && i < b - 1) depth--;
        else if ((s[i] == ',' && !depth) || i == b - 1) {
            xa[n] = st; xb[n] = i;
            cp_trim(s, &xa[n], &xb[n]);
            n++;
            st = i + 1;
        }
    }
    return n;
}

/* the same text at s[a0..b0) and u[a1..b1) */
static int cp_same(const char *s, int a0, int b0, const char *u, int a1, int b1) {
    int k;
    if (b0 - a0 != b1 - a1) return 0;
    for (k = 0; k < b0 - a0; k++) if (s[a0 + k] != u[a1 + k]) return 0;
    return 1;
}

/* two transform functions accumulated (CSS Transforms 2 section 15): the
 * same function with as many arguments, summed argument by argument (the
 * one-based scale arguments as a + b - 1); -1 when they do not combine
 * this way (matrix(), perspective(), another function, rotate3d() about
 * another axis), -2 when cap is short, else the length */
static int cp_tf_acc(const char *s, int a0, int b0, const char *u, int a1, int b1, char *out, int cap) {
    int fa, fb, xa[6], xb[6], ya[6], yb[6], na, nb, k, len = 0, r;
    const char *nm;
    na = cp_tf_args(s, a0, b0, &fa, xa, xb);
    nb = cp_tf_args(u, a1, b1, &fb, ya, yb);
    if (na != nb || fa != fb) return -1;
    nm = cp_tfns[fa].name;
    if (cp_eqn(nm, cp_len(nm), "matrix") || cp_eqn(nm, cp_len(nm), "perspective")) return -1;
    if (!cp_put(out, &len, cap, nm, cp_len(nm)) || !cp_put(out, &len, cap, "(", 1)) return -2;
    for (k = 0; k < na; k++) {
        char kind = cp_tfns[fa].args[k];
        /* each piece is checked as it is written: the first failure, the
         * room (-2) or the sum (-1), decides */
        if (k && !cp_put(out, &len, cap, ", ", 2)) return -2;
        if (kind == 'N') {
            /* the axis of rotate3d(): kept when both write it the same */
            if (!cp_same(s, xa[k], xb[k], u, ya[k], yb[k])) return -1;
            if (!cp_put(out, &len, cap, s + xa[k], xb[k] - xa[k])) return -2;
        } else if (kind == 'S') {
            long long x, y, v;
            int px, py;
            if (!cp_tok_num(s, xa[k], xb[k], &x, &px) || !cp_tok_num(u, ya[k], yb[k], &y, &py)) return -1;
            if (px) x = cp_div(x, 100, 0);
            if (py) y = cp_div(y, 100, 0);
            v = x + y - CP_S;
            if (!cp_fits(v)) return -1;
            r = cp_fmt(v, "", out + len, cap - len);
            if (r < 0) return -2;
            len += r;
        } else {
            /* a length or length-percentage (a bare number is a length
             * there), or an angle */
            int typ = kind == 'A' ? WF_CSSPROP_ANGLE : WF_CSSPROP_LENPCT;
            r = cp_interp_num(typ, s, xa[k], xb[k], u, ya[k], yb[k], (int)CP_S, 1, out + len, cap - len);
            if (r < 0) return r;
            len += r;
        }
    }
    return cp_put(out, &len, cap, ")", 1) ? len : -2;
}

/* two transform lists: add 1 concatenates them (under, then v), add 2
 * accumulates their functions pair by pair and keeps the tail of the
 * longer one; -1 / -2 / the length as cp_tf_acc */
static int cp_tl_combine(const char *s, int a0, int b0, const char *u, int a1, int b1, int add, char *out, int cap) {
    int len = 0, i = a0, j = a1, r;
    if (add == 1) {
        cp_trim(s, &a0, &b0);
        cp_trim(u, &a1, &b1);
        if (!cp_put(out, &len, cap, s + a0, b0 - a0) || !cp_put(out, &len, cap, " ", 1) ||
            !cp_put(out, &len, cap, u + a1, b1 - a1))
            return -2;
        return len;
    }
    for (;;) {
        int ei, ej;
        while (i < b0 && cp_sp(s[i])) i++;
        while (j < b1 && cp_sp(u[j])) j++;
        if (i >= b0 && j >= b1) break;
        if (len && !cp_put(out, &len, cap, " ", 1)) return -2;
        /* both lists are matched already: each function ends */
        ei = i < b0 ? cp_tfunc_end(s, i, b0) : i;
        ej = j < b1 ? cp_tfunc_end(u, j, b1) : j;
        if (i >= b0 || j >= b1) {
            /* the tail of the longer list: the identity functions it is
             * paired with change nothing */
            if (i < b0 ? !cp_put(out, &len, cap, s + i, ei - i) : !cp_put(out, &len, cap, u + j, ej - j)) return -2;
        } else {
            r = cp_tf_acc(s, i, ei, u, j, ej, out + len, cap - len);
            if (r < 0) return r;
            len += r;
        }
        i = ei;
        j = ej;
    }
    return len;
}

/* one item of type t: -1 discrete, -2 short, else its length */
static int cp_interp_item(int t, const char *s, int a0, int b0, const char *u, int a1, int b1, int tt, int add, char *out,
                          int cap) {
    if ((t >= WF_CSSPROP_LENGTH && t <= WF_CSSPROP_TIME && t != WF_CSSPROP_COLOR) || t == WF_CSSPROP_RESOLUTION)
        return cp_interp_num(t, s, a0, b0, u, a1, b1, tt, add, out, cap);
    if (t == WF_CSSPROP_COLOR) {
        char x[WF_CSSPROP_VAL], y[WF_CSSPROP_VAL];
        if (cp_copy_item(x, (int)sizeof(x), s, a0, b0) < 0 || cp_copy_item(y, (int)sizeof(y), u, a1, b1) < 0) return -1;
        return cp_interp_color(x, y, tt, add, out, cap);
    }
    if (add && t == WF_CSSPROP_TLIST) return cp_tl_combine(s, a0, b0, u, a1, b1, add, out, cap);
    /* Lot 99: one function has no sum that is one function: only
     * accumulate combines it (the value or list item is trimmed already) */
    if (add == 2 && t == WF_CSSPROP_TFUNC) return cp_tf_acc(s, a0, b0, u, a1, b1, out, cap);
    return -1;
}

/* wf_cssprop_interp (add 0), wf_cssprop_add (add 1) and Lot 99
 * wf_cssprop_accumulate (add 2); t 10000 for both: the fallback is b */
static int cp_combine(const wf_cssprop_t *p, const char *a, const char *b, int t, int add, char *out, int cap) {
    const wf_cssprop_alt_t *al;
    wf_cssprop_alt_t one;
    int ia, ib, na, nb, len = 0, r;
    if (!out || cap < 1) return 0;
    out[0] = 0;
    if (!a || !b) return 0;
    na = cp_len(a); nb = cp_len(b);
    if (!add && (t == 0 || t == CP_S)) {
        const char *w = t == 0 ? a : b;
        int n = t == 0 ? na : nb;
        if (!cp_put(out, &len, cap, w, n)) { out[0] = 0; return 0; }
        return 2;
    }
    if (!p) goto discrete;      /* '*' matches as one item that is never numeric: discrete */
    ia = cp_alt_index(p, a);
    ib = cp_alt_index(p, b);
    if (ia < 0 || ia != ib) goto discrete;
    if (p->nalt) al = &p->alt[ia];
    else { one.type = p->syntax; one.mult = WF_CSSPROP_ONE; one.ident[0] = 0; al = &one; }
    if (al->mult == WF_CSSPROP_ONE || (add && al->type == WF_CSSPROP_TFUNC && al->mult == WF_CSSPROP_PLUS)) {
        /* Lot 99: <transform-function>+ adds and accumulates as a
         * transform list (CSS Properties and Values 1) */
        r = cp_interp_item(al->mult == WF_CSSPROP_ONE ? al->type : WF_CSSPROP_TLIST, a, 0, na, b, 0, nb, t, add, out, cap);
        if (r == -2) { out[0] = 0; return 0; }
        if (r < 0) goto discrete;
        return 2;
    }
    {
        int i = 0, j = 0, x0, x1, y0, y1, ga, gb;
        for (;;) {
            ga = cp_next_item(a, na, al->mult, &i, &x0, &x1);
            gb = cp_next_item(b, nb, al->mult, &j, &y0, &y1);
            if (ga != gb) goto discrete;
            if (!ga) break;
            if (len && !cp_put(out, &len, cap, al->mult == WF_CSSPROP_HASH ? ", " : " ", al->mult == WF_CSSPROP_HASH ? 2 : 1)) {
                out[0] = 0;
                return 0;
            }
            r = cp_interp_item(al->type, a, x0, x1, b, y0, y1, t, add, out + len, cap - len);
            if (r == -2) { out[0] = 0; return 0; }
            if (r < 0) goto discrete;
            len += r;
        }
        return len ? 2 : 0;
    }
discrete:
    len = 0;
    out[0] = 0;
    if (!cp_put(out, &len, cap, t < 5000 ? a : b, t < 5000 ? na : nb)) { out[0] = 0; return 0; }
    return 1;
}

int wf_cssprop_interp(const wf_cssprop_t *p, const char *a, const char *b, int t, char *out, int cap) {
    return cp_combine(p, a, b, t, 0, out, cap);
}

int wf_cssprop_add(const wf_cssprop_t *p, const char *under, const char *v, char *out, int cap) {
    return cp_combine(p, under, v, (int)CP_S, 1, out, cap);
}

int wf_cssprop_accumulate(const wf_cssprop_t *p, const char *under, const char *v, char *out, int cap) {
    return cp_combine(p, under, v, (int)CP_S, 2, out, cap);
}

int wf_cssprop_rgb24(const char *v, int n, unsigned int *rgb) {
    char buf[WF_CSSPROP_VAL];
    int a = 0, b, k;
    cp_rgba_t c;
    if (!v || !rgb) return 0;
    while (a < n && cp_sp(v[a])) a++;
    for (b = a; b < n && v[b] != ')'; b++) {}
    if (b >= n || b + 1 - a >= (int)sizeof(buf)) return 0;
    for (k = b + 1; k < n && cp_sp(v[k]); k++) {}
    if (k < n && v[k] != ';' && v[k] != '!') return 0;
    for (k = a; k <= b; k++) buf[k - a] = v[k];
    buf[b + 1 - a] = 0;
    /* hsl() only: the reader takes its other colours itself; cp_rgba
     * checks the arguments (letters in any case; buf ends at its only
     * ')') */
    if ((!cp_eqn(buf, 4, "hsl(") && !cp_eqn(buf, 5, "hsla(")) || !cp_rgba(buf, &c) || c.a != CP_S) return 0;
    *rgb = (unsigned int)((cp_div(c.c[0], CP_S, 0) << 16) | (cp_div(c.c[1], CP_S, 0) << 8) | cp_div(c.c[2], CP_S, 0));
    return 1;
}

/* Lot 101: a colour once currentcolor has a value (see wf_cssprop.h) */
int wf_cssprop_color_cur(const char *v, int n) {
    int on = cp_cur_on, ok, alpha;
    unsigned int rgb;
    cp_cur_on = 1;
    ok = wf_cssprop_color(v, n, &rgb, &alpha);
    cp_cur_on = on;
    return ok;
}

/* Lot 100: any colour the reader paints (see wf_cssprop.h) */
int wf_cssprop_color(const char *v, int n, unsigned int *rgb, int *alpha) {
    char buf[WF_CSSPROP_VAL];
    int a = 0, b, k, depth = 0;
    cp_rgba_t c;
    if (!v || !rgb || !alpha) return 0;
    while (a < n && cp_sp(v[a])) a++;
    /* to a space, ';' or '!' outside parentheses (a text glued after the
     * closing ')', an empty text or an unclosed '(' fails every colour
     * form below) */
    for (b = a; b < n; b++) {
        if (v[b] == '(') depth++;
        else if (v[b] == ')') depth--;
        else if (!depth && (cp_sp(v[b]) || v[b] == ';' || v[b] == '!')) break;
    }
    if (b - a >= (int)sizeof(buf)) return 0;
    for (k = b; k < n && cp_sp(v[k]); k++) {}
    if (k < n && v[k] != ';' && v[k] != '!') return 0;
    for (k = a; k < b; k++) buf[k - a] = v[k];     /* cp_rgba takes letters in any case */
    buf[b - a] = 0;
    if (!cp_rgba(buf, &c)) return 0;
    *rgb = (unsigned int)((cp_div(c.c[0], CP_S, 0) << 16) | (cp_div(c.c[1], CP_S, 0) << 8) | cp_div(c.c[2], CP_S, 0));
    *alpha = (int)cp_div(c.a * 255, CP_S, 0);
    return 1;
}

/* ---- Lot 97: easing functions ------------------------------------------ */

/* a number token [a, b) in 1/10000 (four decimals, the fifth rounds half
 * away from zero), no unit: 1 */
static int cp_tok_plain(const char *s, int a, int b, long long *v) {
    int pct;
    return cp_tok_num(s, a, b, v, &pct) && !pct;
}

/* the arguments of fn(...) at s[0..n): up to max token ranges split at
 * commas; the count, -1 when it is not "name(...)" */
static int cp_fn_args(const char *s, int n, const char *name, int *ta, int *tb, int max) {
    int m = cp_len(name), i, c = 0, a;
    if (n <= m + 1 || !cp_eqn(s, m, name) || s[m] != '(' || s[n - 1] != ')') return -1;
    i = m + 1;
    for (;;) {
        int x, y;
        a = i;
        while (i < n - 1 && s[i] != ',') { if (s[i] == '(' || s[i] == ')') return -1; i++; }
        x = a; y = i;
        while (x < y && cp_sp(s[x])) x++;
        while (y > x && cp_sp(s[y - 1])) y--;
        if (y == x || c >= max) return -1;
        ta[c] = x; tb[c] = y; c++;
        if (i >= n - 1) break;
        i++;
    }
    return c;
}

int wf_cssease_parse(const char *s, int n, wf_cssease_t *e) {
    static const struct { const char *w; int x1, y1, x2, y2; } kw[] = {
        {"ease", 2500, 1000, 2500, 10000}, {"ease-in", 4200, 0, 10000, 10000},
        {"ease-out", 0, 0, 5800, 10000}, {"ease-in-out", 4200, 0, 5800, 10000}, {0, 0, 0, 0, 0}};
    int ta[4], tb[4], c, k;
    long long v[4];
    if (!s || !e || n <= 0) return 0;
    e->kind = WF_CSSEASE_LINEAR; e->pos = WF_CSSEASE_END; e->n = 1;
    e->x1 = e->y1 = e->x2 = e->y2 = 0;
    if (cp_eqn(s, n, "linear")) return 1;
    for (k = 0; kw[k].w; k++)
        if (cp_eqn(s, n, kw[k].w)) {
            e->kind = WF_CSSEASE_BEZIER;
            e->x1 = kw[k].x1; e->y1 = kw[k].y1; e->x2 = kw[k].x2; e->y2 = kw[k].y2;
            return 1;
        }
    if (cp_eqn(s, n, "step-start")) { e->kind = WF_CSSEASE_STEPS; e->pos = WF_CSSEASE_START; return 1; }
    if (cp_eqn(s, n, "step-end")) { e->kind = WF_CSSEASE_STEPS; e->pos = WF_CSSEASE_END; return 1; }
    c = cp_fn_args(s, n, "cubic-bezier", ta, tb, 4);
    if (c >= 0) {
        if (c != 4) return 0;
        for (k = 0; k < 4; k++) if (!cp_tok_plain(s, ta[k], tb[k], &v[k])) return 0;
        if (v[0] < 0 || v[0] > CP_S || v[2] < 0 || v[2] > CP_S) return 0;
        if (v[1] < -100 * CP_S || v[1] > 100 * CP_S || v[3] < -100 * CP_S || v[3] > 100 * CP_S) return 0;
        e->kind = WF_CSSEASE_BEZIER;
        e->x1 = (int)v[0]; e->y1 = (int)v[1]; e->x2 = (int)v[2]; e->y2 = (int)v[3];
        return 1;
    }
    c = cp_fn_args(s, n, "steps", ta, tb, 2);
    if (c >= 1) {
        int steps = 0;
        for (k = ta[0]; k < tb[0]; k++) {
            if (!cp_digit(s[k]) || steps > 10000) return 0;
            steps = steps * 10 + (s[k] - '0');
        }
        if (steps < 1 || steps > 10000) return 0;
        e->pos = WF_CSSEASE_END;
        if (c == 2) {
            const char *w = s + ta[1];
            int l = tb[1] - ta[1];
            if (cp_eqn(w, l, "jump-start") || cp_eqn(w, l, "start")) e->pos = WF_CSSEASE_START;
            else if (cp_eqn(w, l, "jump-end") || cp_eqn(w, l, "end")) e->pos = WF_CSSEASE_END;
            else if (cp_eqn(w, l, "jump-none")) e->pos = WF_CSSEASE_NONE;
            else if (cp_eqn(w, l, "jump-both")) e->pos = WF_CSSEASE_BOTH;
            else return 0;
        }
        if (e->pos == WF_CSSEASE_NONE && steps < 2) return 0;
        e->kind = WF_CSSEASE_STEPS;
        e->n = steps;
        return 1;
    }
    return 0;
}

/* 3 c1 (1-u)^2 u + 3 c2 (1-u) u^2 + 10000 u^3, u in 1/2^30, as a 128-bit
 * value in 1/10000 * 2^-90 */
static cp_w_t cp_bez(long long c1, long long c2, long long u) {
    long long q = (1LL << 30) - u;
    cp_w_t t1 = cp_wmul(cp_wmul(cp_w(q * q), u), 3 * c1);
    cp_w_t t2 = cp_wmul(cp_wmul(cp_w(u * u), q), 3 * c2);
    cp_w_t t3 = cp_wmul(cp_wmul(cp_w(u * u), u), CP_S);
    return cp_wadd(cp_wadd(t1, t2), t3);
}

int wf_cssease_at(const wf_cssease_t *e, int x, int before) {
    if (!e) return x;
    if (e->kind == WF_CSSEASE_STEPS) {
        long long cur = cp_div((long long)x * e->n, CP_S, 1), jumps = e->n;
        if (e->pos == WF_CSSEASE_START || e->pos == WF_CSSEASE_BOTH) cur++;
        if (before && (x * e->n) % CP_S == 0) cur--;     /* x, n <= 10000: fits an int */
        if (x >= 0 && cur < 0) cur = 0;
        if (e->pos == WF_CSSEASE_NONE) jumps = e->n - 1;
        else if (e->pos == WF_CSSEASE_BOTH) jumps = e->n + 1;
        if (x <= CP_S && cur > jumps) cur = jumps;
        return (int)cp_div(cur * CP_S, jumps, 0);
    }
    if (e->kind == WF_CSSEASE_BEZIER) {
        long long lo = 0, hi = 1LL << 30;
        cp_w_t want;
        if (x <= 0) return x;     /* x = 10000 gives u = 2^30, y exactly 10000 */
        /* the smallest u with x(u) >= x: x(u) grows with u (x1, x2 in 0..1) */
        want = cp_wshl(cp_w(x), 90);
        while (hi - lo > 1) {
            long long mid = (lo + hi) / 2;
            if (cp_wlt(cp_bez(e->x1, e->x2, mid), want)) lo = mid;
            else hi = mid;
        }
        return (int)cp_wshr(cp_bez(e->y1, e->y2, hi), 90).lo;
    }
    return x;
}

/* ---- Lot 97: the animation and transition shorthands ------------------ */

/* a time token: 1 and *ms (rounded half away from zero to 1 ms) */
static int cp_time_ms(const char *s, int a, int b, long long *ms) {
    char item[32];
    int i = 0, frac, k, neg = 0, fd = 0;
    long long raw = 0;
    if (cp_copy_item(item, (int)sizeof(item), s, a, b) < 0 || !cp_number(item, &i, &frac)) return 0;
    k = 0;
    if (item[k] == '+' || item[k] == '-') { neg = item[k] == '-'; k++; }
    for (; k < i && item[k] != '.'; k++) {
        if (raw > CP_LIM) return 0;
        raw = raw * 10 + (item[k] - '0');
    }
    raw *= CP_S;
    if (k < i) {
        int place = 1000;
        for (k++; k < i; k++, fd++) {
            if (fd < 4) { raw += (item[k] - '0') * place; place /= 10; }
            else if (fd == 4 && item[k] >= '5') raw++;
        }
    }
    if (raw > CP_LIM * 10) return 0;
    if (neg) raw = -raw;
    if (cp_eqn(item + i, cp_len(item + i), "s")) *ms = cp_div(raw * 1000, CP_S, 0);
    else if (cp_eqn(item + i, cp_len(item + i), "ms")) *ms = cp_div(raw, CP_S, 0);
    else return 0;
    return *ms <= WF_CSSANIM_MS_MAX && *ms >= -WF_CSSANIM_MS_MAX;
}

/* the next token of one list item from *i (a function with its
 * parentheses is one token): [*a, *b); 0 at the end */
static int cp_next_tok(const char *s, int n, int *i, int *a, int *b) {
    int depth = 0;
    while (*i < n && cp_sp(s[*i])) (*i)++;
    if (*i >= n) return 0;
    *a = *i;
    while (*i < n && (depth || !cp_sp(s[*i]))) {
        if (s[*i] == '(') depth++;
        else if (s[*i] == ')') { if (!depth) return 0; depth--; }
        (*i)++;
    }
    *b = *i;
    return depth == 0;
}

/* the comma separated items of v: their ranges, the count, -1 when there
 * are more than max or one is empty */
static int cp_list_items(const char *v, int *ia, int *ib, int max) {
    int n = cp_len(v), i = 0, c = 0, a, b, commas = 0, depth = 0, k;
    if (!n) return -1;
    /* the top-level commas (split as cp_next_item does): at most max - 1 */
    for (k = 0; k < n; k++) {
        if (v[k] == '(') depth++;
        else if (v[k] == ')') depth--;
        else if (v[k] == ',' && !depth) commas++;
    }
    if (commas + 1 > max) return -1;
    while (cp_next_item(v, n, WF_CSSPROP_HASH, &i, &a, &b)) {
        ia[c] = a; ib[c] = b; c++;      /* c <= commas + 1 <= max */
        if (i > n) break;
    }
    /* an empty item ("a, , b" or a trailing comma) */
    if (commas + 1 != c) return -1;
    return c;
}

/* the initial values: no name, 0s, ease, 0s, 1, normal, none, running,
 * replace */
static void cp_anim_init(wf_cssanim_t *o) {
    int k;
    for (k = 0; k < (int)sizeof(*o); k++) ((unsigned char *)o)[k] = 0;
    o->iters = (int)CP_S;
    o->ease.kind = WF_CSSEASE_BEZIER;
    o->ease.x1 = 2500; o->ease.y1 = 1000; o->ease.x2 = 2500; o->ease.y2 = 10000; o->ease.n = 1;
}

static int cp_anim_one(const char *v, int a, int b, wf_cssanim_t *o) {
    int i = a, x, y, times = 0, have_ease = 0, have_count = 0, have_dir = 0, have_fill = 0, have_play = 0, have_name = 0;
    long long ms;
    int k;
    cp_anim_init(o);
    while (cp_next_tok(v, b, &i, &x, &y)) {
        const char *w = v + x;
        int l = y - x;
        wf_cssease_t e;
        long long cnt;
        if (cp_time_ms(v, x, y, &ms)) {
            if (times == 0) { if (ms < 0) return 0; o->dur = (int)ms; }
            else if (times == 1) o->delay = (int)ms;
            else return 0;
            times++;
            continue;
        }
        if (!have_ease && wf_cssease_parse(w, l, &e)) { o->ease = e; have_ease = 1; continue; }
        if (!have_count && cp_eqn(w, l, "infinite")) { o->iters = -1; have_count = 1; continue; }
        if (!have_count && cp_tok_plain(v, x, y, &cnt)) {
            if (cnt < 0 || cnt > 100000 * CP_S) return 0;
            o->iters = (int)cnt;
            have_count = 1;
            continue;
        }
        if (!have_dir) {
            int d = cp_eqn(w, l, "normal") ? 1 : cp_eqn(w, l, "reverse") ? 2 : cp_eqn(w, l, "alternate") ? 3 :
                    cp_eqn(w, l, "alternate-reverse") ? 4 : 0;
            if (d) { o->dir = (unsigned char)(d - 1); have_dir = 1; continue; }
        }
        if (!have_fill) {
            int f = cp_eqn(w, l, "none") ? 1 : cp_eqn(w, l, "forwards") ? 2 : cp_eqn(w, l, "backwards") ? 3 :
                    cp_eqn(w, l, "both") ? 4 : 0;
            if (f) { o->fill = (unsigned char)(f - 1); have_fill = 1; continue; }
        }
        if (!have_play) {
            if (cp_eqn(w, l, "running")) { have_play = 1; continue; }
            if (cp_eqn(w, l, "paused")) { o->paused = 1; have_play = 1; continue; }
        }
        if (have_name) return 0;
        if (cp_eqn(w, l, "none")) { have_name = 1; continue; }
        if (!cp_is_ident(w, l) || cp_reserved(w, l) || l >= WF_CSSPROP_NAME) return 0;
        for (k = 0; k < l; k++) o->name[k] = w[k];
        o->name[l] = 0;
        have_name = 1;
    }
    return 1;
}

int wf_cssanim_parse(const char *v, wf_cssanim_t *out, int max) {
    int ia[WF_CSSANIM_MAX], ib[WF_CSSANIM_MAX], c, k;
    if (!v || !out || max < 1) return -1;
    if (max > WF_CSSANIM_MAX) max = WF_CSSANIM_MAX;
    c = cp_list_items(v, ia, ib, max);
    if (c < 1) return -1;
    for (k = 0; k < c; k++) if (!cp_anim_one(v, ia[k], ib[k], &out[k])) return -1;
    return c;
}

/* the initial values: all, 0s, ease, 0s, normal */
static void cp_trans_init(wf_csstrans_t *o) {
    int k;
    for (k = 0; k < (int)sizeof(*o); k++) ((unsigned char *)o)[k] = 0;
    o->ease.kind = WF_CSSEASE_BEZIER;
    o->ease.x1 = 2500; o->ease.y1 = 1000; o->ease.x2 = 2500; o->ease.y2 = 10000; o->ease.n = 1;
    o->prop[0] = 'a'; o->prop[1] = 'l'; o->prop[2] = 'l'; o->prop[3] = 0;
}

static int cp_trans_one(const char *v, int a, int b, wf_csstrans_t *o, int *none) {
    int i = a, x, y, times = 0, have_ease = 0, have_prop = 0, have_beh = 0, k;
    long long ms;
    cp_trans_init(o);
    *none = 0;
    while (cp_next_tok(v, b, &i, &x, &y)) {
        const char *w = v + x;
        int l = y - x;
        wf_cssease_t e;
        if (cp_time_ms(v, x, y, &ms)) {
            if (times == 0) { if (ms < 0) return 0; o->dur = (int)ms; }
            else if (times == 1) o->delay = (int)ms;
            else return 0;
            times++;
            continue;
        }
        if (!have_ease && wf_cssease_parse(w, l, &e)) { o->ease = e; have_ease = 1; continue; }
        if (!have_beh && (cp_eqn(w, l, "allow-discrete") || cp_eqn(w, l, "normal"))) {
            o->discrete = (unsigned char)cp_eqn(w, l, "allow-discrete");
            have_beh = 1;
            continue;
        }
        if (have_prop) return 0;
        if (cp_eqn(w, l, "none")) { *none = 1; o->prop[0] = 0; have_prop = 1; continue; }
        if (!cp_is_ident(w, l) || l >= WF_CSSPROP_NAME ||
            (cp_reserved(w, l) && !cp_eqn(w, l, "all")))
            return 0;
        for (k = 0; k < l; k++) o->prop[k] = w[k];
        o->prop[l] = 0;
        have_prop = 1;
    }
    return 1;
}

int wf_csstrans_parse(const char *v, wf_csstrans_t *out, int max) {
    int ia[WF_CSSANIM_MAX], ib[WF_CSSANIM_MAX], c, k, none;
    if (!v || !out || max < 1) return -1;
    if (max > WF_CSSANIM_MAX) max = WF_CSSANIM_MAX;
    c = cp_list_items(v, ia, ib, max);
    if (c < 1) return -1;
    for (k = 0; k < c; k++) {
        if (!cp_trans_one(v, ia[k], ib[k], &out[k], &none)) return -1;
        if (none && c > 1) return -1;
    }
    return c;
}

/* ---- Lot 98: the longhands and their coordinated lists -------------- */

/* the one token of a longhand item [a, b): its range, 0 when there are
 * none or more */
static int cp_one_tok(const char *v, int a, int b, int *x, int *y) {
    int i = a, x2, y2;
    if (!cp_next_tok(v, b, &i, x, y)) return 0;
    return !cp_next_tok(v, b, &i, &x2, &y2);
}

/* a keyword of list w[] (0-terminated): its index + 1, 0 */
static int cp_kw(const char *w, int l, const char *const *list) {
    int k;
    for (k = 0; list[k]; k++) if (cp_eqn(w, l, list[k])) return k + 1;
    return 0;
}

static const char *const cp_dirs[] = {"normal", "reverse", "alternate", "alternate-reverse", 0};
static const char *const cp_fills[] = {"none", "forwards", "backwards", "both", 0};
static const char *const cp_plays[] = {"running", "paused", 0};
static const char *const cp_comps[] = {"replace", "add", "accumulate", 0};

static const char *const cp_tl_axes[] = { "block", "inline", "x", "y", 0 };
static const char *const cp_scrollers[] = {"nearest", "root", "self", 0};
static const char *const cp_ranges[] = {"cover", "contain", "entry", "exit", "entry-crossing", "exit-crossing",
                                        "scroll", 0};

/* Lot 102: an inset of view(): auto, a length, a percentage or 0. Lot
 * 104: its kind (WF_CSSTL_I_*) and value (1/10000 px or %) into *k, *v;
 * a length is absolute (the leaf has no font or viewport context here),
 * within +-100000px (CP_LIM) */
static int cp_inset_tok(const char *s, int a, int b, unsigned char *k, int *v) {
    char item[32];
    cp_q_t q;
    cp_mp_t m;
    if (cp_eqn(s + a, b - a, "auto")) { *k = WF_CSSTL_I_AUTO; *v = 0; return 1; }
    if (cp_copy_item(item, (int)sizeof(item), s, a, b) < 0) return 0;
    m.s = item; m.i = 0; m.depth = 0;
    cp_mleaf(&m, &q);
    if (item[m.i] || !q.ok) return 0;
    *v = (int)q.v;
    if (q.t == WF_CSSPROP_PERCENT) { *k = WF_CSSTL_I_PCT; return 1; }
    *k = WF_CSSTL_I_LEN;
    return q.t == WF_CSSPROP_LENGTH || (q.t == WF_CSSPROP_NUMBER && !q.v);
}

/* Lot 105: v is one absolute length (or a unitless 0): its value in
 * 1/10000 px into *out (within +-100000px) */
int wf_csslen_abs(const char *v, int *out) {
    unsigned char k;
    int x, y, q;
    if (!v || !out || !cp_one_tok(v, 0, cp_len(v), &x, &y) || !cp_inset_tok(v, x, y, &k, &q) || k != WF_CSSTL_I_LEN)
        return 0;
    *out = q;
    return 1;
}

/* Lot 102: scroll(...) or view(...) at w[0..l) into o: 1, 0 when it is
 * not valid, -1 when w is neither function */
static int cp_anon_tl(const char *w, int l, wf_cssanim_t *o) {
    int view, i, x, y, k, src = -1, axis = -1, inset = 0, closed = 0, iv[2] = {0, 0};
    unsigned char ik[2] = {WF_CSSTL_I_AUTO, WF_CSSTL_I_AUTO};
    if (l >= 8 && cp_eqn(w, 7, "scroll(")) { view = 0; i = 7; }
    else if (l >= 6 && cp_eqn(w, 5, "view(")) { view = 1; i = 5; }
    else return -1;
    if (w[l - 1] != ')') return 0;
    while (cp_next_tok(w, l - 1, &i, &x, &y)) {
        k = cp_kw(w + x, y - x, cp_tl_axes);
        if (k && axis < 0) {
            /* an axis ends the insets before it */
            axis = k - 1;
            closed = inset > 0;
            continue;
        }
        if (!view) {
            k = cp_kw(w + x, y - x, cp_scrollers);
            if (!k || src >= 0) return 0;
            src = k - 1;
            continue;
        }
        if (closed || inset >= 2 || !cp_inset_tok(w, x, y, &ik[inset], &iv[inset])) return 0;
        inset++;
    }
    /* a stray ')' stops the tokens early */
    while (i < l - 1 && cp_sp(w[i])) i++;
    if (i < l - 1) return 0;
    o->timeline = (unsigned char)(view ? WF_CSSANIM_TL_VIEW : WF_CSSANIM_TL_SCROLL);
    o->tlsrc = (unsigned char)(src < 0 ? WF_CSSANIM_SC_NEAREST : src);
    o->tlaxis = (unsigned char)(axis < 0 ? WF_CSSTL_BLOCK : axis);
    o->tlname[0] = 0;
    /* Lot 104: one inset is both */
    for (k = 0; k < 2; k++) {
        o->vik[k] = ik[inset == 1 ? 0 : k];
        o->viv[k] = iv[inset == 1 ? 0 : k];
    }
    return 1;
}

/* Lot 102: a percentage or a unitless 0 at [x, y): 1 and *p (1/10000 %) */
static int cp_range_pct(const char *v, int x, int y, long long *p) {
    int pct;
    if (!cp_tok_num(v, x, y, p, &pct)) return 0;
    return pct || !*p;
}

/* Lot 104: an absolute length at [x, y) (+-100000px): 1 and *p (1/10000
 * px); auto and percentages are other kinds */
static int cp_range_len(const char *v, int x, int y, long long *p) {
    unsigned char k;
    int iv;
    if (!cp_inset_tok(v, x, y, &k, &iv) || k != WF_CSSTL_I_LEN) return 0;
    *p = iv;
    return 1;
}

/* Lot 102: one animation-range edge from the tokens of v at *i (before
 * b): 1 into *kind and *p, 0 when there is none or it is not valid; a
 * range name alone is at 0% for a start, 100% for an end. Lot 104: *len
 * 1 when *p is a length */
static int cp_range_edge(const char *v, int b, int *i, int end, unsigned char *kind, int *p, unsigned char *len) {
    int x, y, j, k;
    long long q;
    *len = 0;
    if (!cp_next_tok(v, b, i, &x, &y)) return 0;
    if (cp_eqn(v + x, y - x, "normal")) { *kind = WF_CSSANIM_R_NORMAL; *p = 0; return 1; }
    if (cp_range_pct(v, x, y, &q)) { *kind = WF_CSSANIM_R_PCT; *p = (int)q; return 1; }
    if (cp_range_len(v, x, y, &q)) { *kind = WF_CSSANIM_R_PCT; *p = (int)q; *len = 1; return 1; }
    k = cp_kw(v + x, y - x, cp_ranges);
    if (!k) return 0;
    *kind = (unsigned char)(WF_CSSANIM_R_COVER + k - 1);
    *p = end ? (int)(100 * CP_S) : 0;
    j = *i;
    if (cp_next_tok(v, b, &j, &x, &y)) {
        if (cp_range_pct(v, x, y, &q)) { *p = (int)q; *i = j; }
        else if (cp_range_len(v, x, y, &q)) { *p = (int)q; *len = 1; *i = j; }
    }
    return 1;
}

/* Lot 102: one item of animation-range-start (end 0) or -end (1) */
static int cp_range_long(const char *v, int a, int b, int end, wf_cssanim_t *o) {
    int i = a, x, y;
    if (!cp_range_edge(v, b, &i, end, end ? &o->re : &o->rs, end ? &o->rep : &o->rsp, end ? &o->rel : &o->rsl)) return 0;
    return !cp_next_tok(v, b, &i, &x, &y);
}

/* Lot 102: one item of the animation-range shorthand */
static int cp_range_one(const char *v, int a, int b, wf_cssanim_t *o) {
    int i = a, j, x, y;
    if (!cp_range_edge(v, b, &i, 0, &o->rs, &o->rsp, &o->rsl)) return 0;
    j = i;
    if (!cp_next_tok(v, b, &j, &x, &y)) {
        /* no end: the start's range name at 100%, else normal */
        int named = o->rs >= WF_CSSANIM_R_COVER;
        o->re = named ? o->rs : (unsigned char)WF_CSSANIM_R_NORMAL;
        o->rep = named ? (int)(100 * CP_S) : 0;
        o->rel = 0;
        return 1;
    }
    if (!cp_range_edge(v, b, &i, 1, &o->re, &o->rep, &o->rel)) return 0;
    return !cp_next_tok(v, b, &i, &x, &y);
}

int wf_cssanim_range(const char *v, wf_cssanim_t *out, int max) {
    int ia[WF_CSSANIM_MAX], ib[WF_CSSANIM_MAX], c, k;
    if (!v || !out) return -1;
    if (max > WF_CSSANIM_MAX) max = WF_CSSANIM_MAX;
    c = cp_list_items(v, ia, ib, max);     /* max < 1: no list fits */
    for (k = 0; k < c; k++) if (!cp_range_one(v, ia[k], ib[k], &out[k])) return -1;
    return c;
}

int wf_cssanim_range_text(const wf_cssanim_t *a, int n, int end, char *out, int cap) {
    int k, m = 0, r;
    if (!out || cap <= 0) return 0;
    out[0] = 0;
    if (!a) return 0;
    for (k = 0; k < n; k++) {
        int kind = end ? a[k].re : a[k].rs;
        if (kind > WF_CSSANIM_R_SCROLL || (k && !cp_put(out, &m, cap, ", ", 2))) { out[0] = 0; return 0; }
        if (kind == WF_CSSANIM_R_NORMAL) {
            if (!cp_put(out, &m, cap, "normal", 6)) { out[0] = 0; return 0; }
            continue;
        }
        if (kind >= WF_CSSANIM_R_COVER) {
            const char *nm = cp_ranges[kind - WF_CSSANIM_R_COVER];
            if (!cp_put(out, &m, cap, nm, cp_len(nm)) || !cp_put(out, &m, cap, " ", 1)) { out[0] = 0; return 0; }
        }
        r = cp_fmt(end ? a[k].rep : a[k].rsp, (end ? a[k].rel : a[k].rsl) ? "px" : "%", out + m, cap - m);
        if (r < 0) { out[0] = 0; return 0; }
        m += r;
    }
    return m;
}

/* one item of animation longhand which into o: 1, 0 when it is not valid */
static int cp_anim_long_one(int which, const char *v, int a, int b, wf_cssanim_t *o) {
    int x, y, l, k;
    const char *w;
    long long ms;
    wf_cssease_t e;
    if (which == WF_CSSANIM_L_RANGE_START || which == WF_CSSANIM_L_RANGE_END)
        return cp_range_long(v, a, b, which == WF_CSSANIM_L_RANGE_END, o);
    if (!cp_one_tok(v, a, b, &x, &y)) return 0;
    w = v + x; l = y - x;
    switch (which) {
    case WF_CSSANIM_L_NAME:
        if (cp_eqn(w, l, "none")) { o->name[0] = 0; return 1; }
        if (!cp_is_ident(w, l) || cp_reserved(w, l) || l >= WF_CSSPROP_NAME) return 0;
        for (k = 0; k < l; k++) o->name[k] = w[k];
        o->name[l] = 0;
        return 1;
    case WF_CSSANIM_L_DURATION:
        if (cp_eqn(w, l, "auto")) { o->dur = 0; return 1; }     /* CSS Animations 2: auto is 0s for a time */
        if (!cp_time_ms(v, x, y, &ms) || ms < 0) return 0;
        o->dur = (int)ms;
        return 1;
    case WF_CSSANIM_L_TIMING:
        if (!wf_cssease_parse(w, l, &e)) return 0;
        o->ease = e;
        return 1;
    case WF_CSSANIM_L_DELAY:
        if (!cp_time_ms(v, x, y, &ms)) return 0;
        o->delay = (int)ms;
        return 1;
    case WF_CSSANIM_L_COUNT:
        if (cp_eqn(w, l, "infinite")) { o->iters = -1; return 1; }
        if (!cp_tok_plain(v, x, y, &ms) || ms < 0) return 0;     /* at most CP_LIM: 100000 */
        o->iters = (int)ms;
        return 1;
    case WF_CSSANIM_L_DIRECTION:
        k = cp_kw(w, l, cp_dirs);
        if (k) o->dir = (unsigned char)(k - 1);
        return k > 0;
    case WF_CSSANIM_L_FILL:
        k = cp_kw(w, l, cp_fills);
        if (k) o->fill = (unsigned char)(k - 1);
        return k > 0;
    case WF_CSSANIM_L_PLAY:
        k = cp_kw(w, l, cp_plays);
        if (k) o->paused = (unsigned char)(k - 1);
        return k > 0;
    case WF_CSSANIM_L_COMPOSITION:
        k = cp_kw(w, l, cp_comps);
        if (k) o->comp = (unsigned char)(k - 1);
        return k > 0;
    default:
        /* Lot 99: animation-timeline */
        /* tlname is meaningful for WF_CSSANIM_TL_NAMED only */
        k = cp_anon_tl(w, l, o);     /* Lot 102 */
        if (k >= 0) return k;
        if (cp_eqn(w, l, "auto")) { o->timeline = WF_CSSANIM_TL_AUTO; return 1; }
        if (cp_eqn(w, l, "none")) { o->timeline = WF_CSSANIM_TL_NONE; return 1; }
        if (l < 3 || w[0] != '-' || w[1] != '-' || !cp_is_ident(w, l) || l >= WF_CSSPROP_NAME) return 0;
        o->timeline = WF_CSSANIM_TL_NAMED;
        for (k = 0; k < l; k++) o->tlname[k] = w[k];
        o->tlname[l] = 0;
        return 1;
    }
}

int wf_cssanim_long(int which, const char *v, wf_cssanim_t *out, int max) {
    int ia[WF_CSSANIM_MAX], ib[WF_CSSANIM_MAX], c, k;
    if (!v || !out || which < 0 || which >= WF_CSSANIM_LONGS) return -1;     /* max < 1: no list fits */
    if (max > WF_CSSANIM_MAX) max = WF_CSSANIM_MAX;
    c = cp_list_items(v, ia, ib, max);     /* -1 or at least 1 */
    for (k = 0; k < c; k++) if (!cp_anim_long_one(which, v, ia[k], ib[k], &out[k])) return -1;
    return c;
}

static int cp_trans_long_one(int which, const char *v, int a, int b, wf_csstrans_t *o) {
    int x, y, l, k;
    const char *w;
    long long ms;
    wf_cssease_t e;
    if (!cp_one_tok(v, a, b, &x, &y)) return 0;
    w = v + x; l = y - x;
    switch (which) {
    case WF_CSSTRANS_L_PROPERTY:
        if (cp_eqn(w, l, "none")) { o->prop[0] = 0; return 2; }
        if (!cp_is_ident(w, l) || l >= WF_CSSPROP_NAME || (cp_reserved(w, l) && !cp_eqn(w, l, "all"))) return 0;
        for (k = 0; k < l; k++) o->prop[k] = w[k];
        o->prop[l] = 0;
        return 1;
    case WF_CSSTRANS_L_DURATION:
        if (!cp_time_ms(v, x, y, &ms) || ms < 0) return 0;
        o->dur = (int)ms;
        return 1;
    case WF_CSSTRANS_L_TIMING:
        if (!wf_cssease_parse(w, l, &e)) return 0;
        o->ease = e;
        return 1;
    case WF_CSSTRANS_L_DELAY:
        if (!cp_time_ms(v, x, y, &ms)) return 0;
        o->delay = (int)ms;
        return 1;
    default:
        if (cp_eqn(w, l, "allow-discrete")) { o->discrete = 1; return 1; }
        if (cp_eqn(w, l, "normal")) { o->discrete = 0; return 1; }
        return 0;
    }
}

int wf_csstrans_long(int which, const char *v, wf_csstrans_t *out, int max) {
    int ia[WF_CSSANIM_MAX], ib[WF_CSSANIM_MAX], c, k, r;
    if (!v || !out || which < 0 || which >= WF_CSSTRANS_LONGS) return -1;    /* max < 1: no list fits */
    if (max > WF_CSSANIM_MAX) max = WF_CSSANIM_MAX;
    c = cp_list_items(v, ia, ib, max);     /* -1 or at least 1 */
    for (k = 0; k < c; k++) {
        r = cp_trans_long_one(which, v, ia[k], ib[k], &out[k]);
        if (!r || (r == 2 && c > 1)) return -1;      /* none only alone */
    }
    return c;
}

/* field k of an animation (transition) from src to dst */
static void cp_anim_field(int k, wf_cssanim_t *dst, const wf_cssanim_t *src) {
    int j;
    switch (k) {
    case WF_CSSANIM_L_NAME: for (j = 0; j < WF_CSSPROP_NAME; j++) dst->name[j] = src->name[j]; break;
    case WF_CSSANIM_L_DURATION: dst->dur = src->dur; break;
    case WF_CSSANIM_L_TIMING: dst->ease = src->ease; break;
    case WF_CSSANIM_L_DELAY: dst->delay = src->delay; break;
    case WF_CSSANIM_L_COUNT: dst->iters = src->iters; break;
    case WF_CSSANIM_L_DIRECTION: dst->dir = src->dir; break;
    case WF_CSSANIM_L_FILL: dst->fill = src->fill; break;
    case WF_CSSANIM_L_PLAY: dst->paused = src->paused; break;
    case WF_CSSANIM_L_COMPOSITION: dst->comp = src->comp; break;
    case WF_CSSANIM_L_RANGE_START: dst->rs = src->rs; dst->rsp = src->rsp; dst->rsl = src->rsl; break;  /* Lot 102, 104 */
    case WF_CSSANIM_L_RANGE_END: dst->re = src->re; dst->rep = src->rep; dst->rel = src->rel; break;
    default:
        dst->timeline = src->timeline;
        dst->tlsrc = src->tlsrc;
        dst->tlaxis = src->tlaxis;
        for (j = 0; j < 2; j++) { dst->vik[j] = src->vik[j]; dst->viv[j] = src->viv[j]; }     /* Lot 104 */
        for (j = 0; j < WF_CSSPROP_NAME; j++) dst->tlname[j] = src->tlname[j];
        break;
    }
}

static void cp_trans_field(int k, wf_csstrans_t *dst, const wf_csstrans_t *src) {
    int j;
    switch (k) {
    case WF_CSSTRANS_L_PROPERTY: for (j = 0; j < WF_CSSPROP_NAME; j++) dst->prop[j] = src->prop[j]; break;
    case WF_CSSTRANS_L_DURATION: dst->dur = src->dur; break;
    case WF_CSSTRANS_L_TIMING: dst->ease = src->ease; break;
    case WF_CSSTRANS_L_DELAY: dst->delay = src->delay; break;
    default: dst->discrete = src->discrete; break;
    }
}

int wf_cssanim_coord(const char *const *v, const unsigned char *sh, wf_cssanim_t *out, int max) {
    static wf_cssanim_t lists[WF_CSSANIM_LONGS][WF_CSSANIM_MAX];
    int cnt[WF_CSSANIM_LONGS], k, j, n;
    if (!v || !sh || !out || max < 1) return 0;
    for (k = 0; k < WF_CSSANIM_LONGS; k++) {
        /* a longhand list writes field k of each item, the shorthand all */
        cnt[k] = !v[k] ? 0 : sh[k] ? wf_cssanim_parse(v[k], lists[k], max) : wf_cssanim_long(k, v[k], lists[k], max);
        if (cnt[k] < 1) { cp_anim_init(&lists[k][0]); cnt[k] = 1; }
    }
    n = cnt[WF_CSSANIM_L_NAME];
    for (j = 0; j < n; j++) {
        /* every field is copied */
        for (k = 0; k < WF_CSSANIM_LONGS; k++) cp_anim_field(k, &out[j], &lists[k][j % cnt[k]]);
    }
    return n;
}

int wf_csstrans_coord(const char *const *v, const unsigned char *sh, wf_csstrans_t *out, int max) {
    static wf_csstrans_t lists[WF_CSSTRANS_LONGS][WF_CSSANIM_MAX];
    int cnt[WF_CSSTRANS_LONGS], k, j, n;
    if (!v || !sh || !out || max < 1) return 0;
    for (k = 0; k < WF_CSSTRANS_LONGS; k++) {
        /* a longhand list writes field k of each item, the shorthand all */
        cnt[k] = !v[k] ? 0 : sh[k] ? wf_csstrans_parse(v[k], lists[k], max) : wf_csstrans_long(k, v[k], lists[k], max);
        if (cnt[k] < 1) { cp_trans_init(&lists[k][0]); cnt[k] = 1; }
    }
    n = cnt[WF_CSSTRANS_L_PROPERTY];
    for (j = 0; j < n; j++) {
        /* every field is copied */
        for (k = 0; k < WF_CSSTRANS_LONGS; k++) cp_trans_field(k, &out[j], &lists[k][j % cnt[k]]);
    }
    return n;
}

int wf_cssanim_progress(const wf_cssanim_t *a, int ms, int *p, int *before) {
    long long t, e, it = 0, simple = 0;
    int dir;
    if (!a || !p || !before) return 0;
    *p = 0; *before = 0;
    if (a->dur == 0 && a->iters < 0) return 0;
    t = a->paused ? 0 : ms;
    e = t - a->delay;
    if (e < 0) {
        if (!(a->fill & WF_CSSANIM_BACKWARDS)) return 0;
        *before = 1;
    } else if (a->dur > 0 && (a->iters < 0 || e * CP_S < (long long)a->dur * a->iters)) {
        long long r;
        it = cp_div(e, a->dur, 1);
        r = e - it * a->dur;
        simple = cp_div(r * CP_S, a->dur, 1);
    } else {
        if (!(a->fill & WF_CSSANIM_FORWARDS)) return 0;
        if (a->iters == 0) { it = 0; simple = 0; }
        else if (a->iters % CP_S == 0) { it = a->iters / CP_S - 1; simple = CP_S; }
        else { it = a->iters / CP_S; simple = a->iters % CP_S; }
    }
    dir = a->dir;
    if (dir == WF_CSSANIM_REVERSE || (dir == WF_CSSANIM_ALTERNATE && (it & 1)) ||
        (dir == WF_CSSANIM_ALT_REVERSE && !(it & 1)))
        simple = CP_S - simple;
    *p = (int)simple;
    return 1;
}

/* ---- Lot 101: scroll and view timeline names (CSS Scroll-driven
 * Animations 1) ---- */

/* one name: none (an empty name) or a <dashed-ident> as animation-timeline
 * takes it */
static int cp_tl_name(const char *w, int l, wf_csstl_t *o) {
    int k;
    if (cp_eqn(w, l, "none")) { o->name[0] = 0; return 1; }
    if (l < 3 || w[0] != '-' || w[1] != '-' || !cp_is_ident(w, l) || l >= WF_CSSPROP_NAME) return 0;
    for (k = 0; k < l; k++) o->name[k] = w[k];
    o->name[l] = 0;
    return 1;
}

/* Lot 104: the insets of one item (or of the view-timeline shorthand
 * after its name, with an axis before or after them: vshort) from the
 * tokens of v at *i before b into o: 1, 0 when they are not valid */
static int cp_tl_insets(const char *v, int b, int *i, int vshort, wf_csstl_t *o) {
    int x, y, k, n = 0, axis = 0, closed = 0, iv[2] = {0, 0};
    unsigned char ik[2] = {WF_CSSTL_I_AUTO, WF_CSSTL_I_AUTO};
    while (cp_next_tok(v, b, i, &x, &y)) {
        k = vshort ? cp_kw(v + x, y - x, cp_tl_axes) : 0;
        if (k && !axis) {
            axis = 1;
            closed = n > 0;
            o->axis = (unsigned char)(k - 1);
            continue;
        }
        if (closed || n >= 2 || !cp_inset_tok(v, x, y, &ik[n], &iv[n])) return 0;
        n++;
    }
    /* an item has a token (cp_list_items), so a list of insets has n >= 1 */
    for (k = 0; k < 2; k++) {
        o->ik[k] = ik[n == 1 ? 0 : k];
        o->iv[k] = iv[n == 1 ? 0 : k];
    }
    return 1;
}

static int cp_tl_one(int which, const char *v, int a, int b, wf_csstl_t *o) {
    int i = a, x, y, k;
    o->name[0] = 0;
    o->axis = WF_CSSTL_BLOCK;
    o->ik[0] = o->ik[1] = WF_CSSTL_I_AUTO;
    o->iv[0] = o->iv[1] = 0;
    if (which == WF_CSSTL_INSETS) return cp_tl_insets(v, b, &i, 0, o);
    if (which == WF_CSSTL_VSHORT) {
        if (!cp_next_tok(v, b, &i, &x, &y) || !cp_tl_name(v + x, y - x, o)) return 0;
        return cp_tl_insets(v, b, &i, 1, o);
    }
    if (which == WF_CSSTL_SHORT) {
        if (!cp_next_tok(v, b, &i, &x, &y) || !cp_tl_name(v + x, y - x, o)) return 0;
        if (!cp_next_tok(v, b, &i, &x, &y)) return 1;
        k = cp_kw(v + x, y - x, cp_tl_axes);
        if (!k) return 0;
        o->axis = (unsigned char)(k - 1);
        return !cp_next_tok(v, b, &i, &x, &y);
    }
    if (!cp_one_tok(v, a, b, &x, &y)) return 0;
    if (which == WF_CSSTL_SCOPE) {
        /* Lot 102: none and all are keywords (alone), else a name */
        if (cp_eqn(v + x, y - x, "all")) { o->axis = WF_CSSTL_ALL; return 2; }
        if (cp_eqn(v + x, y - x, "none")) return 2;
        return cp_tl_name(v + x, y - x, o);
    }
    if (which == WF_CSSTL_NAMES) return cp_tl_name(v + x, y - x, o);
    k = cp_kw(v + x, y - x, cp_tl_axes);
    if (!k) return 0;
    o->axis = (unsigned char)(k - 1);
    return 1;
}

int wf_csstl_parse(int which, const char *v, wf_csstl_t *out, int max) {
    int ia[WF_CSSANIM_MAX], ib[WF_CSSANIM_MAX], c, k, r;
    if (!v || !out || which < WF_CSSTL_NAMES || which > WF_CSSTL_VSHORT) return -1;
    if (max > WF_CSSANIM_MAX) max = WF_CSSANIM_MAX;
    c = cp_list_items(v, ia, ib, max);     /* max < 1: no list fits */
    for (k = 0; k < c; k++) {
        r = cp_tl_one(which, v, ia[k], ib[k], &out[k]);
        if (!r || (r == 2 && c > 1)) return -1;      /* Lot 102: none and all alone */
    }
    return c;
}

int wf_csstl_scoped(const char *v, const char *name) {
    static wf_csstl_t t[WF_CSSANIM_MAX];
    int n, k;
    if (!name || !name[0]) return 0;
    n = wf_csstl_parse(WF_CSSTL_SCOPE, v, t, WF_CSSANIM_MAX);    /* -1 for a null v */
    if (n == 1 && t[0].axis == WF_CSSTL_ALL) return 2;
    for (k = 0; k < n; k++) if (cp_eqn(t[k].name, cp_len(t[k].name), name)) return 1;
    return 0;
}

int wf_csstl_coord(const char *names, const char *axes, wf_csstl_t *out, int max) {
    return wf_csstl_coord_ex(names, axes, 0, out, max);
}

int wf_csstl_coord_ex(const char *names, const char *axes, const char *insets, wf_csstl_t *out, int max) {
    static wf_csstl_t ax[WF_CSSANIM_MAX], in[WF_CSSANIM_MAX];
    int n, a = 0, c = 0, k, j;
    n = wf_csstl_parse(WF_CSSTL_NAMES, names, out, max);     /* -1 for a null names or out */
    if (n < 1) return 0;
    if (axes) a = wf_csstl_parse(WF_CSSTL_AXES, axes, ax, WF_CSSANIM_MAX);
    if (insets) c = wf_csstl_parse(WF_CSSTL_INSETS, insets, in, WF_CSSANIM_MAX);   /* Lot 104 */
    for (k = 0; k < n; k++) {
        out[k].axis = a > 0 ? ax[k % a].axis : (unsigned char)WF_CSSTL_BLOCK;
        /* the names list left them auto */
        if (c > 0) for (j = 0; j < 2; j++) { out[k].ik[j] = in[k % c].ik[j]; out[k].iv[j] = in[k % c].iv[j]; }
    }
    return n;
}

/* Lot 104: one inset as text into out at *m: 1, 0 when it does not fit */
static int cp_inset_text(int k, int v, char *out, int *m, int cap) {
    int r;
    if (k == WF_CSSTL_I_AUTO) return cp_put(out, m, cap, "auto", 4);
    r = cp_fmt(v, k == WF_CSSTL_I_PCT ? "%" : "px", out + *m, cap - *m);
    if (r < 0) return 0;
    *m += r;
    return 1;
}

int wf_csstl_text(const wf_csstl_t *t, int n, int axes, char *out, int cap) {
    int k, m = 0, j;
    const char *s;
    if (!out || cap <= 0) return 0;
    out[0] = 0;
    if (!t) return 0;
    for (k = 0; k < n; k++) {
        if (axes == 2) {
            /* Lot 104: the insets, two equal ones once */
            int two = t[k].ik[0] != t[k].ik[1] || t[k].iv[0] != t[k].iv[1];
            if ((k && !cp_put(out, &m, cap, ", ", 2)) || !cp_inset_text(t[k].ik[0], t[k].iv[0], out, &m, cap) ||
                (two && (!cp_put(out, &m, cap, " ", 1) || !cp_inset_text(t[k].ik[1], t[k].iv[1], out, &m, cap)))) {
                out[0] = 0;
                return 0;
            }
            continue;
        }
        s = axes ? cp_tl_axes[t[k].axis & 3] : t[k].name[0] ? t[k].name : "none";
        if (k) { if (m + 2 >= cap) { out[0] = 0; return 0; } out[m++] = ','; out[m++] = ' '; }
        for (j = 0; s[j]; j++) { if (m + 1 >= cap) { out[0] = 0; return 0; } out[m++] = s[j]; }
    }
    out[m] = 0;
    return m;
}

int wf_cssanim_scroll_ms(const wf_cssanim_t *a, int prog, int *ms) {
    long long s;
    if (!a || !ms) return 0;
    *ms = 0;
    if (a->iters < 0 || prog < 0 || prog > (int)CP_S) return 0;
    /* the start delay and the active duration in 1/10000 ms */
    s = (long long)a->delay * CP_S + (long long)a->dur * a->iters;
    if (s <= 0 || s > (long long)WF_CSSANIM_MS_MAX * CP_S) return 0;
    *ms = (int)cp_div(s * prog, CP_S * CP_S, 1);
    return 1;
}

int wf_cssanim_range_ms(const wf_cssanim_t *a, int prog, int *ms) {
    long long s, st, en, den, t, lim = 2LL * WF_CSSANIM_MS_MAX;
    cp_w_t num, q;
    if (!wf_cssanim_scroll_ms(a, prog, ms)) return 0;     /* the same checks */
    *ms = 0;
    if (a->rsl || a->rel) return 0;     /* Lot 104: a length needs the scroll range (wf_cssanim_scroll_px_ms) */
    st = a->rs == WF_CSSANIM_R_PCT ? a->rsp : 0;
    en = a->re == WF_CSSANIM_R_PCT ? a->rep : 100 * CP_S;
    *ms = 0;
    if (en <= st) return 0;
    s = (long long)a->delay * CP_S + (long long)a->dur * a->iters;
    /* s (100 prog - S) / (10000 (E - S)), at most about 10^21 / 1 */
    num = cp_wmul(cp_w(s), (long long)prog * 100 - st);
    den = (en - st) * CP_S;
    q = cp_wdiv(num, den, 2);      /* toward zero */
    if (cp_wlt(q, cp_w(-lim))) t = -lim;
    else if (cp_wlt(cp_w(lim), q)) t = lim;
    else {
        t = (long long)q.lo;
        if (cp_wneg_p(cp_wsub(num, cp_wmul(cp_w(t), den)))) t--;     /* rounded down */
    }
    *ms = (int)t;
    return 1;
}

/* ---- Lot 104: view progress timelines (CSS Scroll-driven Animations 1,
 * 3.2 and 4.1) ---- */

/* floor(num / den), den > 0, the result within a 64-bit word */
static long long cp_wfloor(cp_w_t num, long long den) {
    long long t = (long long)cp_wdiv(num, den, 2).lo;      /* toward zero */
    if (cp_wneg_p(cp_wsub(num, cp_wmul(cp_w(t), den)))) t--;
    return t;
}

/* inset k (0 start, 1 end) of v in 1/1000000 px: auto 0 (the reader has
 * no scroll-padding), a length, a percentage of the view (rounded down) */
static long long cp_view_inset(const wf_cssview_t *v, int k) {
    if (v->ik[k] == WF_CSSTL_I_LEN) return (long long)v->iv[k] * 100;
    if (v->ik[k] == WF_CSSTL_I_PCT) return cp_wfloor(cp_wmul(cp_w(v->view), v->iv[k]), 1000000);
    return 0;
}

int wf_cssview_range(const wf_cssview_t *v, int range, long long *lo, long long *hi) {
    long long s, e, vw, is, ie, a, b;
    if (!v || !lo || !hi || range < WF_CSSANIM_R_COVER || range > WF_CSSANIM_R_SCROLL) return 0;
    s = v->start; e = v->start + v->size; vw = v->view;
    is = cp_view_inset(v, 0); ie = cp_view_inset(v, 1);
    /* contain: the subject inside the view, or covering it */
    a = e - vw + ie; b = s - is;
    if (a > b) { long long t = a; a = b; b = t; }
    switch (range) {
    case WF_CSSANIM_R_COVER: *lo = s - vw + ie; *hi = e - is; break;
    case WF_CSSANIM_R_CONTAIN: *lo = a; *hi = b; break;
    case WF_CSSANIM_R_ENTRY: *lo = s - vw + ie; *hi = a; break;
    case WF_CSSANIM_R_EXIT: *lo = b; *hi = e - is; break;
    case WF_CSSANIM_R_ENTRY_CROSSING: *lo = s - vw + ie; *hi = e - vw + ie; break;
    case WF_CSSANIM_R_EXIT_CROSSING: *lo = s - is; *hi = e - is; break;
    default: *lo = 0; *hi = v->range; break;
    }
    return 1;
}

int wf_cssview_edge(const wf_cssview_t *v, int kind, int val, int len, int end, long long *out) {
    long long lo, hi;
    if (!v || !out) return 0;
    if (!wf_cssview_range(v, kind >= WF_CSSANIM_R_COVER ? kind : WF_CSSANIM_R_COVER, &lo, &hi)) return 0;
    if (kind == WF_CSSANIM_R_NORMAL) { *out = end ? hi : lo; return 1; }
    if (len) { *out = lo + (long long)val * 100; return 1; }
    *out = lo + cp_wfloor(cp_wmul(cp_w(hi - lo), val), 1000000);
    return 1;
}

/* the time of a at pos within [st, en] (a: finite count and T checked) */
static int cp_span_ms(const wf_cssanim_t *a, long long st, long long en, long long pos, int *ms) {
    long long s, t, den, lim = 2LL * WF_CSSANIM_MS_MAX;
    cp_w_t num, q;
    *ms = 0;
    if (a->iters < 0) return 0;
    s = (long long)a->delay * CP_S + (long long)a->dur * a->iters;
    if (s <= 0 || s > (long long)WF_CSSANIM_MS_MAX * CP_S || en <= st) return 0;
    /* s (pos - st) / (CP_S (en - st)): s within 2^50, the positions
     * within about 2^40, so den within 2^54 */
    num = cp_wmul(cp_w(s), pos - st);
    den = (en - st) * CP_S;
    q = cp_wdiv(num, den, 2);      /* toward zero */
    if (cp_wlt(q, cp_w(-lim))) t = -lim;
    else if (cp_wlt(cp_w(lim), q)) t = lim;
    else {
        t = cp_wfloor(num, den);     /* rounded down: -2e8 - 0.5 is still -2e8 */
        if (t < -lim) t = -lim;
    }
    *ms = (int)t;
    return 1;
}

int wf_cssanim_view_ms(const wf_cssanim_t *a, const wf_cssview_t *v, int *ms) {
    long long st, en;
    if (!ms) return 0;
    *ms = 0;
    if (!a || !v) return 0;
    if (!wf_cssview_edge(v, a->rs, a->rsp, a->rsl, 0, &st) || !wf_cssview_edge(v, a->re, a->rep, a->rel, 1, &en)) return 0;
    return cp_span_ms(a, st, en, v->pos, ms);
}

int wf_cssanim_scroll_px_ms(const wf_cssanim_t *a, long long range, long long pos, int *ms) {
    wf_cssview_t v;
    wf_cssanim_t b;
    long long st, en;
    if (!ms) return 0;
    *ms = 0;
    if (!a || range <= 0) return 0;
    /* a view timeline whose cover range is [0, range]: no view, a subject
     * of the range's size at 0, no insets */
    v.view = 0; v.start = 0; v.size = range; v.range = range; v.pos = pos;
    v.ik[0] = v.ik[1] = WF_CSSTL_I_AUTO; v.iv[0] = v.iv[1] = 0;
    b = *a;
    if (b.rs >= WF_CSSANIM_R_COVER) { b.rs = WF_CSSANIM_R_NORMAL; b.rsl = 0; }
    if (b.re >= WF_CSSANIM_R_COVER) { b.re = WF_CSSANIM_R_NORMAL; b.rel = 0; }
    if (!wf_cssview_edge(&v, b.rs, b.rsp, b.rsl, 0, &st) || !wf_cssview_edge(&v, b.re, b.rep, b.rel, 1, &en)) return 0;
    return cp_span_ms(a, st, en, pos, ms);
}
