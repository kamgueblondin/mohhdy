/* wf_cssprop.c - Lot 90: see wf_cssprop.h. */
#include "wf_cssprop.h"

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
                                        "<integer>", "<color>", "<angle>", "<time>", "<custom-ident>", 0};
    static const int codes[] = {WF_CSSPROP_LENGTH, WF_CSSPROP_PERCENT, WF_CSSPROP_LENPCT, WF_CSSPROP_NUMBER,
                                WF_CSSPROP_INTEGER, WF_CSSPROP_COLOR, WF_CSSPROP_ANGLE, WF_CSSPROP_TIME,
                                WF_CSSPROP_CIDENT};
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

/* Lot 91: the type a unit gives: length, angle, time, 0 unknown */
static int cp_unit_type(const char *u, int n) {
    if (cp_unit(u, n)) return WF_CSSPROP_LENGTH;
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
/* t: the type (0 invalid); ok: the value is known (v) */
typedef struct { int t, ok; long long v; } cp_q_t;

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
    {"s", WF_CSSPROP_TIME, 1, 1}, {"ms", WF_CSSPROP_TIME, 1, 1000}, {0, 0, 0, 0}};

static void cp_ws(cp_mp_t *m) { while (cp_sp(m->s[m->i])) m->i++; }

static void cp_msum(cp_mp_t *m, cp_q_t *q);

/* a number with its unit at m->i; q->t 0 when it is not one */
static void cp_mleaf(cp_mp_t *m, cp_q_t *q) {
    const char *s = m->s;
    int i = m->i, frac, a, k, neg = 0, fd = 0;
    long long raw = 0;
    q->t = 0; q->ok = 1; q->v = 0;
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
        if (!cp_units[k].num) { q->ok = 0; return; }
        if (q->ok) q->v = cp_div(raw * cp_units[k].num, cp_units[k].den, 0);
        if (!cp_fits(q->v)) q->ok = 0;
        return;
    }
}

/* the value is known. A length mixed with a percentage is typed but
 * never known: no leaf is a length-percentage, and + - min() max() and
 * clamp() keep ok only when both sides have the same type. */
static int cp_known(const cp_q_t *q) { return q->ok; }

/* a + b (sub: a - b) into a */
static void cp_qadd(cp_q_t *a, const cp_q_t *b, int sub) {
    int t = cp_tadd(a->t, b->t);
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
/* pi/2, pi/4, tan(pi/8) and ln 2 in 1/2^62; _LO the next 62 bits */
#define CP_PIO2 7244019458077122842LL
#define CP_PIO2_LO 1772391103515558810LL
#define CP_PIO4 3622009729038561421LL
#define CP_TANPI8 1910222894239003202LL
#define CP_LN2 3196577161300663915LL
#define CP_LN2_LO (-243692211680628885LL)

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

/* a / d for 0 < d <= 2^63: mode 0 rounded half away from zero, 1 floor */
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
    if ((mode == 0 && r >= d - r) || (mode == 1 && neg && r)) {
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
        x = cp_wsub(x, cp_wshr(cp_wmul(cp_w(CP_PIO2_LO), (long long)k.lo), 62));
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
    r = cp_wadd(cp_wmul(cp_w(CP_LN2), k), cp_wshr(cp_wmul(cp_w(CP_LN2_LO), k), 62));
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
    t = cp_wsub(t, cp_wshr(cp_wmul(cp_w(CP_LN2_LO), k), 62));
    r = (long long)t.lo;
    e = CP_ONE; term = CP_ONE;
    for (j = 1; j <= 30 && term; j++) {
        term = cp_div(cp_xmul(term, r), j, 0);
        e += term;
    }
    return cp_wout(cp_wshr(cp_wmul(cp_w(e), 10000), 62 - k), out);
}

/* sqrt of n (1/10^8 units squared) into 1/10000, rounded half away */
static int cp_sqrtw(cp_w_t n, long long *out) {
    unsigned long long s = cp_isqrt(n);
    cp_w_t rem = cp_wsub(n, cp_umul(s, s));
    if (cp_wlt(cp_w(0), cp_wsub(rem, cp_umul(s, 1)))) s++;
    if (s > (unsigned long long)CP_LIM) return 0;
    *out = (long long)s;
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
            one.t = WF_CSSPROP_NUMBER; one.ok = 1; one.v = CP_S;
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
    q->t = 0; q->ok = 0; q->v = 0;
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
            if (q->ok) q->v = cp_div(q->v * u.v, CP_S, 0);
            q->t = t;
        } else {
            q->t = u.t == WF_CSSPROP_NUMBER ? q->t : 0;
            q->ok = q->ok && u.ok && u.v != 0;
            if (q->ok) q->v = cp_div(q->v * CP_S, u.v, 0);
        }
        if (!cp_fits(q->v)) q->ok = 0;
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
    q->t = 0; q->ok = 0; q->v = 0;
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

/* Lot 92: "N[.dddd]" and the unit for a value in 1/10000 */
static int cp_fmt(long long v, const char *unit, char *out, int cap) {
    char d[24];
    int n = 0, k = 0, j;
    unsigned long long u, r;
    long long whole;
    int frac;
    if (v < 0) { out[n++] = '-'; v = -v; }
    whole = (long long)cp_udiv((unsigned long long)v, (unsigned long long)CP_S, &r);
    frac = (int)r;
    u = (unsigned long long)whole;
    do { unsigned long long rr; u = cp_udiv(u, 10, &rr); d[k++] = (char)('0' + (int)rr); } while (u && k < 20);
    if (n + k + 6 + cp_len(unit) >= cap) return -1;
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

int wf_cssprop_math_eval(const char *v, int *type, char *out, int cap) {
    cp_q_t q;
    const char *unit;
    if (type) *type = 0;
    if (out && cap > 0) out[0] = 0;
    cp_mwhole(v, &q);
    if (!q.t) return -1;
    if (type) *type = q.t;
    if (!cp_known(&q) || !out) return 0;
    switch (q.t) {
    case WF_CSSPROP_LENGTH: unit = "px"; break;
    case WF_CSSPROP_PERCENT: unit = "%"; break;
    case WF_CSSPROP_ANGLE: unit = "deg"; break;
    case WF_CSSPROP_TIME: unit = "s"; break;
    default: unit = ""; break;
    }
    if (cp_fmt(q.v, unit, out, cap) < 0) { out[0] = 0; return 0; }
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

static int cp_color(const char *v) {
    static const char *const names[] = {
        "black", "white", "red", "green", "blue", "yellow", "gray", "grey", "silver", "maroon", "purple",
        "fuchsia", "lime", "olive", "navy", "teal", "aqua", "orange", "transparent", "currentcolor", 0
    };
    static const char *const fns[] = {"rgb(", "rgba(", "hsl(", "hsla(", 0};
    int k, n = cp_len(v);
    if (v[0] == '#') {
        for (k = 1; k < n; k++) if (!cp_hex(v[k])) return 0;
        return n == 4 || n == 5 || n == 7 || n == 9;
    }
    for (k = 0; names[k]; k++) if (cp_eqn(v, n, names[k])) return 1;
    for (k = 0; fns[k]; k++) {
        int m = cp_len(fns[k]), i, depth = 0;
        if (n <= m || !cp_eqn(v, m, fns[k])) continue;
        /* one level, digits, '.', '%', ',', '/', spaces and "deg" */
        for (i = m; i < n - 1; i++) {
            char c = cp_low(v[i]);
            if (c == '(' || c == ')') { depth++; break; }
            if (!(cp_digit(c) || c == '.' || c == '%' || c == ',' || c == '/' || cp_sp(c) || c == 'd' || c == 'e' ||
                  c == 'g' || c == '-' || c == '+'))
                return 0;
        }
        return !depth && v[n - 1] == ')' && n - 1 > m;
    }
    return 0;
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
    if (syntax < WF_CSSPROP_LENGTH || syntax > WF_CSSPROP_TIME) return 0;
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

/* Lot 92: the computed value of a registered value that is one math
 * function: the first alternative it matches must be one numeric type
 * without a multiplier; the value then has to be known (absolute
 * lengths, angles, times, numbers, or percentages alone). An <integer>
 * rounds to the nearest integer, half toward +infinity. */
int wf_cssprop_compute(const wf_cssprop_t *p, const char *v, char *out, int cap) {
    const wf_cssprop_alt_t *al = 0;
    wf_cssprop_alt_t one;
    cp_q_t q;
    const char *unit = "";
    int k, n;
    if (out && cap > 0) out[0] = 0;
    if (!p || !v || !out || cap < 2 || !cp_mfn(v)) return 0;
    if (!p->nalt) {
        one.type = p->syntax;
        one.mult = WF_CSSPROP_ONE;
        one.ident[0] = 0;
        if (wf_cssprop_match(p->syntax, v)) al = &one;
    } else {
        for (k = 0; k < p->nalt && k < WF_CSSPROP_ALTS && !al; k++) if (cp_alt(&p->alt[k], v)) al = &p->alt[k];
    }
    if (!al || al->mult != WF_CSSPROP_ONE || al->type < WF_CSSPROP_LENGTH || al->type > WF_CSSPROP_TIME ||
        al->type == WF_CSSPROP_COLOR)
        return 0;
    cp_mwhole(v, &q);
    if (!q.t || !cp_known(&q)) return 0;
    if (al->type == WF_CSSPROP_INTEGER) q.v = cp_div(2 * q.v + CP_S, 2 * CP_S, 1) * CP_S;
    switch (q.t) {
    case WF_CSSPROP_LENGTH: unit = "px"; break;
    case WF_CSSPROP_PERCENT: unit = "%"; break;
    case WF_CSSPROP_ANGLE: unit = "deg"; break;
    case WF_CSSPROP_TIME: unit = "s"; break;
    default: break;
    }
    n = cp_fmt(q.v, unit, out, cap);
    if (n < 0) { out[0] = 0; return 0; }
    return 1;
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
