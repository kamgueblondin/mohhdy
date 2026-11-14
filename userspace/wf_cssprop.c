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

typedef struct { long long c[3], a; } cp_rgba_t;
static int cp_rgba(const char *v, cp_rgba_t *o);

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
    /* Lot 100: hwb() and color-mix() are matched by their conversion */
    if ((n > 4 && cp_eqn(v, 4, "hwb(")) || (n > 10 && cp_eqn(v, 10, "color-mix("))) return cp_rgba(v, &t);
    for (k = 0; fns[k]; k++) {
        int m = cp_len(fns[k]), i, depth = 0;
        if (n <= m || !cp_eqn(v, m, fns[k])) continue;
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

/* Lot 100: color-mix(in srgb, C1 [P1], C2 [P2]) (CSS Color 5) at v[0..n):
 * the percentages default to 50% each, or to 100% minus the other; their
 * sum S (> 0) normalises them; the colours are mixed premultiplied and an
 * S under 100% scales the alpha. With P in 1/10000 %: A = a1 P1 + a2 P2,
 * each channel (c1 a1 P1 + c2 a2 P2) / A (0 when A is 0), the alpha A / S,
 * or A / 100% when S is under it; each rounded once */
static int cp_mix(const char *v, int n, cp_rgba_t *o, int depth) {
    int pa[3], pb[3], k, w0, w1;
    long long p[2];
    cp_rgba_t x[2];
    long long sum, an;
    if (depth > 3 || cp_split(v, 10, n - 1, ',', pa, pb, 3) != 3) return 0;
    /* "in", spaces, "srgb" (the part is followed by a comma, so
     * v[w0 + 2] is inside v) */
    w0 = pa[0];
    if (!cp_eqn(v + w0, 2, "in") || !cp_sp(v[w0 + 2])) return 0;
    for (w1 = w0 + 2; cp_sp(v[w1]); w1++) {}
    if (!cp_eqn(v + w1, pb[0] - w1, "srgb")) return 0;
    for (k = 0; k < 2; k++) if (!cp_mix_item(v, pa[k + 1], pb[k + 1], &x[k], &p[k], depth)) return 0;
    if (p[0] < 0 && p[1] < 0) p[0] = p[1] = 50 * CP_S;
    else if (p[0] < 0) p[0] = 100 * CP_S - p[1];
    else if (p[1] < 0) p[1] = 100 * CP_S - p[0];
    sum = p[0] + p[1];
    if (sum <= 0) return 0;
    an = x[0].a * p[0] + x[1].a * p[1];
    for (k = 0; k < 3; k++) o->c[k] = an ? cp_div(x[0].c[k] * x[0].a * p[0] + x[1].c[k] * x[1].a * p[1], an, 0) : 0;
    o->a = cp_div(an, sum >= 100 * CP_S ? sum : 100 * CP_S, 0);
    return 1;
}

static int cp_rgba(const char *v, cp_rgba_t *o) { return cp_rgba_d(v, o, 0); }

static int cp_rgba_d(const char *v, cp_rgba_t *o, int depth) {
    int n = cp_len(v), k;
    o->a = CP_S;
    if (cp_eqn(v, n, "transparent")) { o->c[0] = o->c[1] = o->c[2] = 0; o->a = 0; return 1; }
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
    if ((n > 5 && cp_eqn(v, 4, "rgb(")) || (n > 6 && cp_eqn(v, 5, "rgba("))) {
        int ta[4], tb[4], got = cp_color_toks(v, n, v[3] == '(' ? 4 : 5, ta, tb), pct;
        long long x;
        for (k = 0; k < got; k++) {
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
        if (!cp_hue(v, ta[0], tb[0], &q)) return 0;
        h = q.v - cp_div(q.v, 3600000, 1) * 3600000;   /* 0..359.9999deg */
        for (k = 1; k < 3; k++) {
            if (!cp_tok_num(v, ta[k], tb[k], &x, &pct) || (comma && !pct)) return 0;
            sl[k - 1] = cp_clampll(x, 0, 100 * CP_S);
        }
        if (got == 4) {
            if (!cp_tok_num(v, ta[3], tb[3], &x, &pct)) return 0;
            o->a = cp_clampll(pct ? cp_div(x, 100, 0) : x, 0, CP_S);
        }
        cp_hsl_rgb(h, sl[0], sl[1], o->c);
        return 1;
    }
    if (n > 5 && cp_eqn(v, 4, "hwb(")) {
        /* Lot 100: hwb(H W B [/ A]), spaces only; W and B percentages or
         * numbers (the same scale), clamped to 0..100% */
        int ta[4], tb[4], got = cp_color_toks(v, n, 4, ta, tb), pct;
        long long h, wb[2], x;
        cp_q_t q;
        if ((got != 3 && got != 4) || cp_comma_after(v, tb[0], n) || !cp_hue(v, ta[0], tb[0], &q)) return 0;
        h = q.v - cp_div(q.v, 3600000, 1) * 3600000;
        for (k = 1; k < 3; k++) {
            if (!cp_tok_num(v, ta[k], tb[k], &x, &pct)) return 0;
            wb[k - 1] = cp_clampll(x, 0, 100 * CP_S);
        }
        if (got == 4) {
            if (!cp_tok_num(v, ta[3], tb[3], &x, &pct)) return 0;
            o->a = cp_clampll(pct ? cp_div(x, 100, 0) : x, 0, CP_S);
        }
        cp_hwb_rgb(h, wb[0], wb[1], o->c);
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
static int cp_interp_color(const char *a, const char *b, int t, int add, char *out, int cap) {
    cp_rgba_t x, y;
    long long an, ch[3];
    int k, len = 0, alpha;
    if (!cp_rgba(a, &x) || !cp_rgba(b, &y)) return -1;
    /* alpha * 10^8: Aa 10^4 + (Ab - Aa) t, or (Aa + Ab) 10^4 */
    an = cp_clampll(add ? (x.a + y.a) * CP_S : x.a * CP_S + (y.a - x.a) * t, 0, CP_S * CP_S);
    for (k = 0; k < 3; k++) {
        long long pa = x.c[k] * x.a, pb = y.c[k] * y.a;
        long long pn = add ? (pa + pb) * CP_S : pa * CP_S + (pb - pa) * t;
        ch[k] = an ? cp_clampll(cp_div(pn, an * CP_S, 0), 0, 255) : 0;
    }
    alpha = (int)cp_div(an, CP_S, 0);
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

/* one item of animation longhand which into o: 1, 0 when it is not valid */
static int cp_anim_long_one(int which, const char *v, int a, int b, wf_cssanim_t *o) {
    int x, y, l, k;
    const char *w;
    long long ms;
    wf_cssease_t e;
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
    default:
        dst->timeline = src->timeline;
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
