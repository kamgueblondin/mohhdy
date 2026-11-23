/* Lot 118: the engine's math (CSS Values 4 10), included by webeng.c
 * only, the single engine.
 *
 * Values are kept in thousandths (px, numbers, percentages; angles in
 * thousandths of a degree) in 32-bit integers, within +-WE_MATH_VALUE_MAX
 * px (1e9 thousandths); three sentinels stand for +infinity, -infinity
 * and NaN (CSS Values 4 10.9). Literals keep three decimals (px, numbers,
 * %, angles), the font-relative and viewport units their hundredths of a
 * px (Lot 117 rounded each such leaf to px). The used value is truncated
 * toward zero once, at use (Lot 82 to 117 truncated every operation, so
 * most expressions keep their value); NaN is used as 0 and the
 * infinities as +-WE_MATH_VALUE_MAX px.
 *
 * Products and quotients go through 64-bit products and a shift-and-
 * subtract division (no 64-bit division helper in the guest). The
 * trigonometric functions use CORDIC in millionths of a degree, log and
 * exp a base-2 logarithm and power in 2^-28 steps; their results are
 * rounded to the nearest thousandth.
 *
 * A leaf keeps its 32-bit value in value (low 16 bits) and a, b (high 16
 * bits): leaves have no children. An operator node keeps its children in
 * a, b, c; round keeps its strategy in c, log and the trigonometric
 * functions a flag (base given, argument in radians). */

static int math_refused;

enum { M_NUMBER, M_PX, M_PERCENT, M_ADD, M_SUB, M_MUL, M_DIV, M_MIN, M_MAX, M_CLAMP,
       M_ROUND, M_MOD, M_REM, M_ABS, M_SIGN, M_POW, M_SQRT, M_HYPOT, M_LOG, M_EXP,
       M_SIN, M_COS, M_TAN, M_ASIN, M_ACOS, M_ATAN, M_ATAN2 };
/* the types of CSS Values 4 10.10 the engine keeps: numbers, lengths
 * (with percentages) and angles */
enum { MT_NUMBER, MT_LENGTH, MT_ANGLE };
enum { MR_NEAREST, MR_UP, MR_DOWN, MR_TO_ZERO };

#define MV_MAX 1000000000
#define MV_INF 2000000000
#define MV_NAN (-2147483647 - 1)
#define MV_DEG_PER_RAD_U 57295780       /* millionths of a degree per radian */

typedef struct {
    const char *text;
    int length, at, depth;
    we_length_t *out;
    uint8_t type[WE_MATH_NODES_MAX];
} we_math_parser_t;

static int math_leaf_get(const we_math_node_t *n) {
    return (int)((uint32_t)(uint16_t)n->value | (uint32_t)n->a << 16 | (uint32_t)n->b << 24);
}

static void math_leaf_put(we_math_node_t *n, int v) {
    uint32_t u = (uint32_t)v;
    n->value = (int16_t)(uint16_t)(u & 0xffffu);
    n->a = (uint8_t)(u >> 16);
    n->b = (uint8_t)(u >> 24);
}

/* a one-leaf length (px or % in thousandths) */
static void math_single(we_length_t *e, int op, int value) {
    w_zero(e, sizeof(*e));
    e->count = 1;
    e->percent = (uint8_t)(op == M_PERCENT);
    e->node[0].op = (uint8_t)op;
    e->node[0].dimension = 1;
    math_leaf_put(&e->node[0], value);
}

/* ---------- 64-bit helpers without a division helper ----------
 * the callers never divide by zero (each checks its divisor first) */
static uint64_t m_udiv(uint64_t n, uint64_t d) {
    uint64_t q = 0, r = 0;
    int i;
    for (i = 63; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1u);
        if (r >= d) { r -= d; q |= (uint64_t)1 << i; }
    }
    return q;
}

/* n / d truncated toward zero */
static int64_t m_sdiv(int64_t n, int64_t d) {
    int neg = (n < 0) != (d < 0);
    uint64_t q = m_udiv(n < 0 ? (uint64_t)(-n) : (uint64_t)n, d < 0 ? (uint64_t)(-d) : (uint64_t)d);
    return neg ? -(int64_t)q : (int64_t)q;
}

static uint64_t m_isqrt(uint64_t n) {
    uint64_t res = 0, bit = (uint64_t)1 << 62;
    while (bit > n) bit >>= 2;
    while (bit) {
        if (n >= res + bit) { n -= res + bit; res = (res >> 1) + bit; }
        else res >>= 1;
        bit >>= 2;
    }
    return res;
}

static int m_finite(int v) { return v != MV_NAN && v != MV_INF && v != -MV_INF; }

/* a 64-bit result as a value: beyond the range it is an infinity */
static int m_sat(int64_t v) {
    if (v > MV_MAX) return MV_INF;
    if (v < -MV_MAX) return -MV_INF;
    return (int)v;
}

/* ---------- CORDIC (millionths of a degree, Q29) ---------- */
static const int32_t m_atan_udeg[28] = {
    45000000, 26565051, 14036243, 7125016, 3576334, 1789911, 895174, 447614, 223811, 111906,
    55953, 27976, 13988, 6994, 3497, 1749, 874, 437, 219, 109, 55, 27, 14, 7, 3, 2, 1, 0
};
#define M_CORDIC_K 326016437            /* the CORDIC gain 0.6072529 in Q29 */

/* cos and sin of z (millionths of a degree, -90..90 degrees), Q29 */
static void m_rotate(int32_t z, int32_t *c, int32_t *s) {
    int32_t x = M_CORDIC_K, y = 0, t;
    int i;
    for (i = 0; i < 28; i++) {
        if (z >= 0) { t = x - (y >> i); y = y + (x >> i); x = t; z -= m_atan_udeg[i]; }
        else { t = x + (y >> i); y = y - (x >> i); x = t; z += m_atan_udeg[i]; }
    }
    *c = x; *s = y;
}

/* atan2(y, x) in millionths of a degree, -180..180 degrees */
static int32_t m_atan2_udeg(int64_t y, int64_t x) {
    int32_t z = 0, xi, yi, t;
    int i, shift = 0;
    int64_t m;
    if (!x && !y) return 0;
    if (x < 0) {
        int64_t k = x;
        if (y >= 0) { x = y; y = -k; z = 90000000; }
        else { x = -y; y = k; z = -90000000; }
    }
    m = x > (y < 0 ? -y : y) ? x : (y < 0 ? -y : y);
    while ((m >> shift) >= ((int64_t)1 << 28)) shift++;
    x >>= shift; y >>= shift;
    m >>= shift;
    while (m && m < ((int64_t)1 << 27)) { m *= 2; x *= 2; y *= 2; }
    xi = (int32_t)x; yi = (int32_t)y;
    for (i = 0; i < 28; i++) {
        if (yi > 0) { t = xi + (yi >> i); yi = yi - (xi >> i); xi = t; z += m_atan_udeg[i]; }
        else if (yi < 0) { t = xi - (yi >> i); yi = yi + (xi >> i); xi = t; z -= m_atan_udeg[i]; }
    }
    return z;
}

/* a Q29 fraction as thousandths, rounded */
static int m_q29_milli(int32_t v) { return (int)(((int64_t)v * 1000 + ((int64_t)1 << 28)) >> 29); }

/* millionths of a degree as thousandths, rounded half away from zero */
static int m_udeg_milli(int32_t z) { return z < 0 ? -((-z + 500) / 1000) : (z + 500) / 1000; }

/* sin (which 0) or cos (which 1) of an angle in thousandths of a degree */
static int m_sincos(int deg_milli, int which) {
    int32_t c, s, z;
    int a = deg_milli % 360000, flip = 0;
    if (a > 180000) a -= 360000;
    if (a <= -180000) a += 360000;
    if (a > 90000) { a -= 180000; flip = 1; }
    else if (a < -90000) { a += 180000; flip = 1; }
    z = (int32_t)a * 1000;
    m_rotate(z, &c, &s);
    if (flip) { c = -c; s = -s; }
    return m_q29_milli(which ? c : s);
}

/* ---------- base-2 logarithm and power, 2^-28 steps ---------- */
static const uint32_t m_exp2_q30[28] = {
    1518500250u, 1276901417u, 1170923762u, 1121280436u, 1097253708u, 1085434106u, 1079572136u,
    1076653033u, 1075196443u, 1074468888u, 1074105294u, 1073923544u, 1073832680u, 1073787251u,
    1073764537u, 1073753181u, 1073747502u, 1073744663u, 1073743244u, 1073742534u, 1073742179u,
    1073742001u, 1073741913u, 1073741868u, 1073741846u, 1073741835u, 1073741830u, 1073741827u
};
#define M_LOG2_1000_Q28 2675169848LL    /* m_log2_q28(1000): log2(1000) at 1000 is 0 */
#define M_LN2_Q30 744261118LL           /* ln 2, Q30 */
#define M_LOG2E_2X 774541               /* 2 x log2(e) x 2^28 / 1000 */

/* log2 of a positive integer, Q28 */
static int64_t m_log2_q28(uint32_t n) {
    int k = 31, j;
    uint64_t y;
    int64_t r;
    while (!(n >> k)) k--;
    y = k <= 30 ? (uint64_t)n << (30 - k) : (uint64_t)n >> (k - 30);
    r = (int64_t)k << 28;
    for (j = 1; j <= 28; j++) {
        y = (y * y) >> 30;
        if (y >= ((uint64_t)1 << 31)) { y >>= 1; r += (int64_t)1 << (28 - j); }
    }
    return r;
}

/* 1000 x 2^t (t in Q28), rounded; an infinity past the range */
static int m_exp2_milli(int64_t t) {
    int64_t k = t >> 28, f = t - k * ((int64_t)1 << 28);
    uint64_t v = (uint64_t)1 << 30, m;
    int j;
    if (k >= 20) return MV_INF;          /* 1000 x 2^20 is past the range */
    if (k <= -12) return 0;               /* below 2^-42 x 2^41: rounds to 0 */
    for (j = 1; j <= 28; j++) if ((f >> (28 - j)) & 1) v = (v * m_exp2_q30[j - 1]) >> 30;
    m = v * 1000;
    if (k >= 0) {
        m <<= k;
        m = (m + ((uint64_t)1 << 29)) >> 30;
    } else {
        int sh = 30 + (int)(-k);
        m = (m + ((uint64_t)1 << (sh - 1))) >> sh;
    }
    return m > MV_MAX ? MV_INF : (int)m;
}

/* the natural logarithm of a positive value in thousandths, Q28 of the
 * base-2 logarithm of the value itself */
static int64_t m_log2_value(int v) { return m_log2_q28((uint32_t)v) - M_LOG2_1000_Q28; }

static int m_ln_milli(int v) {
    int64_t q = (m_log2_value(v) * M_LN2_Q30) >> 30;          /* ln, Q28 */
    return (int)((q * 1000 + ((int64_t)1 << 27)) >> 28);
}

/* ---------- evaluation ---------- */
static int math_parse_check;   /* 1 while parse_math checks the constants */

static int m_round(int strategy, int a, int b) {
    int64_t lower, upper;
    int step;
    if (a == MV_NAN || b == MV_NAN || b == 0) return MV_NAN;
    if (!m_finite(a)) return m_finite(b) ? a : MV_NAN;
    if (!m_finite(b)) {
        if (strategy == MR_UP) return a > 0 ? MV_INF : 0;
        if (strategy == MR_DOWN) return a < 0 ? -MV_INF : 0;
        return 0;
    }
    step = b < 0 ? -b : b;
    lower = (int64_t)(a / step) * step;
    if (lower > a) lower -= step;
    upper = lower == a ? lower : lower + step;
    if (strategy == MR_UP) return m_sat(upper);
    if (strategy == MR_DOWN) return m_sat(lower);
    if (strategy == MR_TO_ZERO) return m_sat(a >= 0 ? lower : upper);
    return m_sat(a - lower < upper - a ? lower : upper);
}

static int m_pow(int a, int b) {
    int64_t t;
    if (a == MV_NAN || b == MV_NAN) return MV_NAN;
    if (b == 0) return 1000;
    if (!m_finite(b)) {
        int big = !m_finite(a) || a > 1000 || a < -1000;
        if (a == 1000 || a == -1000) return 1000;
        return (b > 0) == big ? MV_INF : 0;
    }
    if (!m_finite(a)) {
        if (b < 0) return 0;
        if (a > 0) return MV_INF;
        return b % 2000 == 1000 ? -MV_INF : MV_INF;
    }
    if (b % 1000 == 0 && b <= 64000 && b >= -64000) {
        int n = (b < 0 ? -b : b) / 1000, k;
        int64_t r = 1000;
        for (k = 0; k < n; k++) {
            r = m_sdiv(r * a, 1000);
            if (r > MV_MAX || r < -MV_MAX) { r = r > 0 ? MV_INF : -MV_INF; break; }
        }
        if (b > 0) return (int)r;
        if (!r) return MV_INF;
        if (!m_finite((int)r)) return 0;
        return m_sat(m_sdiv((int64_t)1000000, r));
    }
    if (a < 0) return MV_NAN;
    if (a == 0) return b > 0 ? 0 : MV_INF;
    if (b > (1 << 29) || b < -(1 << 29)) return (b > 0) == (a > 1000) ? MV_INF : 0;
    t = m_sdiv(m_log2_value(a) * b, 1000);
    return m_exp2_milli(t);
}

static int m_log(int a, int base, int has_base) {
    int64_t la, lb;
    if (a == MV_NAN || base == MV_NAN) return MV_NAN;
    if (a < 0 || (has_base && base < 0)) return MV_NAN;
    if (!has_base) {
        if (a == 0) return -MV_INF;
        if (a == MV_INF) return MV_INF;
        return m_ln_milli(a);
    }
    if (!m_finite(base) || base == 0) return MV_NAN;
    if (a == MV_INF) return base > 1000 ? MV_INF : -MV_INF;
    if (a == 0) return base > 1000 ? -MV_INF : MV_INF;
    la = m_log2_value(a); lb = m_log2_value(base);
    if (!lb) return la ? (la > 0 ? MV_INF : -MV_INF) : MV_NAN;
    return m_sat(m_sdiv(la * 1000, lb));
}

static int m_exp(int a) {
    if (a == MV_NAN) return MV_NAN;
    if (a == MV_INF) return MV_INF;
    if (a == -MV_INF) return 0;
    return m_exp2_milli(((int64_t)a * M_LOG2E_2X) >> 1);
}

/* value of node i from its children; -2 when it overflows or divides a
 * constant by zero (parse time) */
static int m_apply(const we_math_node_t *node, const int *values, const uint8_t *pct_dep, int *out) {
    int a = values[node->a], b = values[node->b], r;
    switch (node->op) {
    case M_ADD: case M_SUB:
        if (a == MV_NAN || b == MV_NAN) { *out = MV_NAN; return 0; }
        if (node->op == M_SUB) b = b == MV_NAN ? b : -b;
        if (!m_finite(a) || !m_finite(b)) {
            *out = !m_finite(a) && !m_finite(b) && a != b ? MV_NAN : m_finite(a) ? b : a;
            return 0;
        }
        r = a + b;
        if (r > MV_MAX || r < -MV_MAX) return -2;
        *out = r; return 0;
    case M_MUL: {
        int64_t p;
        if (a == MV_NAN || b == MV_NAN) { *out = MV_NAN; return 0; }
        if (!m_finite(a) || !m_finite(b)) {
            if (!a || !b) { *out = MV_NAN; return 0; }
            *out = (a < 0) != (b < 0) ? -MV_INF : MV_INF; return 0;
        }
        p = m_sdiv((int64_t)a * b, 1000);
        if (p > MV_MAX || p < -MV_MAX) return -2;
        *out = (int)p; return 0;
    }
    case M_DIV: {
        int64_t q;
        if (a == MV_NAN || b == MV_NAN) { *out = MV_NAN; return 0; }
        if (!b) {
            if (math_parse_check && !pct_dep[node->b]) return -2;
            *out = !a ? MV_NAN : a > 0 ? MV_INF : -MV_INF;
            return 0;
        }
        if (!m_finite(a) && !m_finite(b)) { *out = MV_NAN; return 0; }
        if (!m_finite(a)) { *out = (a < 0) != (b < 0) ? -MV_INF : MV_INF; return 0; }
        if (!m_finite(b)) { *out = 0; return 0; }
        q = m_sdiv((int64_t)a * 1000, b);
        if (q > MV_MAX || q < -MV_MAX) return -2;
        *out = (int)q; return 0;
    }
    case M_MIN: case M_MAX:
        if (a == MV_NAN || b == MV_NAN) { *out = MV_NAN; return 0; }
        *out = node->op == M_MIN ? (a < b ? a : b) : (a > b ? a : b); return 0;
    case M_CLAMP: {
        int c = values[node->c];
        if (a == MV_NAN || b == MV_NAN || c == MV_NAN) { *out = MV_NAN; return 0; }
        r = b < c ? b : c;
        *out = r < a ? a : r; return 0;
    }
    case M_ROUND: *out = m_round(node->c, a, b); return 0;
    case M_MOD: case M_REM:
        if (a == MV_NAN || b == MV_NAN || !b || !m_finite(a)) { *out = MV_NAN; return 0; }
        if (!m_finite(b)) {
            *out = node->op == M_MOD && a && (a < 0) != (b < 0) ? MV_NAN : a; return 0;
        }
        r = a % b;
        if (node->op == M_MOD && r && (r < 0) != (b < 0)) r += b;
        *out = r; return 0;
    case M_ABS: *out = a == MV_NAN ? a : a < 0 ? -a : a; return 0;
    case M_SIGN: *out = a == MV_NAN ? a : a > 0 ? 1000 : a < 0 ? -1000 : 0; return 0;
    case M_POW: *out = m_pow(a, b); return 0;
    case M_SQRT:
        if (a == MV_NAN || a < 0) *out = MV_NAN;
        else if (a == MV_INF) *out = MV_INF;
        else *out = (int)m_isqrt((uint64_t)a * 1000);
        return 0;
    case M_HYPOT: {
        uint64_t s;
        if (a == MV_NAN || b == MV_NAN) { *out = MV_NAN; return 0; }
        if (!m_finite(a) || !m_finite(b)) { *out = MV_INF; return 0; }
        s = (uint64_t)((int64_t)a * a) + (uint64_t)((int64_t)b * b);
        *out = m_sat((int64_t)m_isqrt(s));
        return 0;
    }
    case M_LOG: *out = m_log(a, b, node->c); return 0;
    case M_EXP: *out = m_exp(a); return 0;
    case M_SIN: case M_COS: case M_TAN: {
        int64_t d64;
        int deg;
        if (!m_finite(a)) { *out = MV_NAN; return 0; }
        d64 = node->c ? m_sdiv((int64_t)a * MV_DEG_PER_RAD_U, 1000000) : a;
        deg = (int)(d64 - m_sdiv(d64, 360000) * 360000);
        if (node->op != M_TAN) { *out = m_sincos(deg, node->op == M_COS); return 0; }
        {
            int s = m_sincos(deg, 0), c = m_sincos(deg, 1);
            if (!c) { *out = s < 0 ? -MV_INF : MV_INF; return 0; }
            *out = m_sat(m_sdiv((int64_t)s * 1000, c));
        }
        return 0;
    }
    case M_ASIN: case M_ACOS: {
        int64_t rest;
        if (a == MV_NAN || a > 1000 || a < -1000) { *out = MV_NAN; return 0; }
        /* sqrt(1 - a^2) in millionths, a in millionths */
        rest = (int64_t)m_isqrt((uint64_t)(1000000 - (int64_t)a * a) * 1000000);
        *out = m_udeg_milli(node->op == M_ASIN ? m_atan2_udeg((int64_t)a * 1000, rest) :
                                                  m_atan2_udeg(rest, (int64_t)a * 1000));
        return 0;
    }
    case M_ATAN:
        if (a == MV_NAN) { *out = MV_NAN; return 0; }
        if (!m_finite(a)) { *out = a > 0 ? 90000 : -90000; return 0; }
        *out = m_udeg_milli(m_atan2_udeg(a, 1000)); return 0;
    case M_ATAN2:
        if (a == MV_NAN || b == MV_NAN) { *out = MV_NAN; return 0; }
        *out = m_udeg_milli(m_atan2_udeg(m_finite(a) ? a : (a > 0 ? MV_MAX : -MV_MAX),
                                         m_finite(b) ? b : (b > 0 ? MV_MAX : -MV_MAX)));
        return 0;
    default: return -2;
    }
}

/* the value of an expression in thousandths, percentages of basis px;
 * -1 when it needs a basis that is not known, -2 when invalid */
static int math_eval_milli(const we_length_t *expr, int basis, int *out) {
    int values[WE_MATH_NODES_MAX], i;
    uint8_t dep[WE_MATH_NODES_MAX];
    if (!expr->count || expr->count > WE_MATH_NODES_MAX) return -2;
    if (basis < 0 && expr->percent) return -1;
    for (i = 0; i < expr->count; i++) {
        const we_math_node_t *node = &expr->node[i];
        if (node->op <= M_PERCENT) {
            int v = math_leaf_get(node);
            dep[i] = node->op == M_PERCENT;
            if (node->op == M_PERCENT) {
                int64_t p = m_sdiv((int64_t)basis * v, 100);
                if (p > MV_MAX || p < -MV_MAX) return -2;
                v = (int)p;
            }
            values[i] = v;
            continue;
        }
        if (node->a >= i || node->b >= i || (node->op == M_CLAMP && node->c >= i)) return -2;
        dep[i] = (uint8_t)(dep[node->a] | dep[node->b] | (node->op == M_CLAMP ? dep[node->c] : 0));
        if (m_apply(node, values, dep, &values[i])) return -2;
    }
    *out = values[expr->count - 1];
    return 0;
}

/* thousandths to the used whole px: NaN 0, infinities the largest */
static int math_px_of(int v) {
    if (v == MV_NAN) return 0;
    if (v == MV_INF) return WE_MATH_VALUE_MAX;
    if (v == -MV_INF) return -WE_MATH_VALUE_MAX;
    return v / 1000;
}

static int math_eval(const we_length_t *expr, int basis, int *out) {
    int v, r = math_eval_milli(expr, basis, &v);
    if (r) return r;
    *out = math_px_of(v);
    return 0;
}

static int math_used(const we_node_t *n, int slot, int basis, int fallback, int nonnegative) {
    int result;
    if (!n->length[slot].count) return fallback;
    if (math_eval(&n->length[slot], basis, &result)) { math_refused = 1; return fallback; }
    if (nonnegative && result < 0) return 0;
    if (result > WE_LAYOUT_MAX) result = WE_LAYOUT_MAX;
    if (result < -WE_LAYOUT_MAX) result = -WE_LAYOUT_MAX;
    return result;
}

static int math_value(const we_node_t *n, int slot, int basis, int fallback) {
    return math_used(n, slot, basis, fallback, 0);
}

static int math_nonnegative(const we_node_t *n, int slot, int basis, int fallback) {
    return math_used(n, slot, basis, fallback, 1);
}

/* ---------- parsing ---------- */
static void math_space(we_math_parser_t *p) {
    while (p->at < p->length && w_space(p->text[p->at])) p->at++;
}

static int math_node(we_math_parser_t *p, int op, int a, int b, int c, int value, int type) {
    we_math_node_t *node;
    int index = p->out->count;
    if (index >= WE_MATH_NODES_MAX) return -1;
    node = &p->out->node[index];
    node->op = (uint8_t)op; node->a = (uint8_t)a; node->b = (uint8_t)b; node->c = (uint8_t)c;
    node->value = 0; node->dimension = (uint8_t)(type == MT_LENGTH);
    if (op <= M_PERCENT) math_leaf_put(node, value);
    p->type[index] = (uint8_t)type;
    p->out->count++;
    if (op == M_PERCENT) p->out->percent = 1;
    return index;
}

static int math_sum(we_math_parser_t *p);

/* a number with up to three decimals (whole part at most 2000), in
 * thousandths; the end of the digits in *end */
static int math_digits(const char *s, int len, int *end, int *milli) {
    int i = 0, whole = 0, frac = 0, scale = 1000, digits = 0;
    while (i < len && s[i] >= '0' && s[i] <= '9') {
        whole = whole * 10 + (s[i] - '0'); i++; digits++;
        if (whole > WE_MATH_LITERAL_MAX) return -1;
    }
    if (i < len && s[i] == '.') {
        int fd = 0;
        i++;
        while (i < len && s[i] >= '0' && s[i] <= '9') {
            if (scale > 1) { scale /= 10; frac += (s[i] - '0') * scale; }
            i++; fd++;
        }
        if (!fd) return -1;
        digits += fd;
    }
    if (!digits) return -1;
    *end = i;
    *milli = whole * 1000 + frac;
    return 0;
}

/* an angle unit: thousandths of a degree per thousandth */
static int math_angle_unit(const char *u, int n, int milli, int *out) {
    int64_t v;
    if (value_is(u, n, "deg")) v = milli;
    else if (value_is(u, n, "grad")) v = m_sdiv((int64_t)milli * 9, 10);
    else if (value_is(u, n, "turn")) v = (int64_t)milli * 360;
    else if (value_is(u, n, "rad")) v = m_sdiv((int64_t)milli * MV_DEG_PER_RAD_U, 1000000);
    else return -1;
    if (v > MV_MAX) return -2;
    *out = (int)v;
    return 0;
}

/* Lot 118: the exponent of a number in scientific notation (CSS Syntax 3
 * 4.3.12: 1e3, 2.5E-2, 1e+1), the length of "e[+-]digits" or 0 when there
 * is none (1em is a unit); *exp in -99..99 */
static int math_exponent(const char *s, int len, int *exp) {
    int i = 1, neg = 0, v = 0, digits = 0;
    if (len < 2 || w_low(s[0]) != 'e') return 0;
    if (s[1] == '+' || s[1] == '-') { neg = s[1] == '-'; i = 2; }
    while (i < len && s[i] >= '0' && s[i] <= '9') {
        if (digits++ == 2) return -1;
        v = v * 10 + (s[i++] - '0');
    }
    if (!digits) return 0;
    *exp = neg ? -v : v;
    return i;
}

/* the mantissa s[0..end) (digits and at most one point, a digit after
 * it) times 10^exp in thousandths, cut toward zero; -1 when its whole
 * part is above the literal bound. Seven significant digits are kept:
 * all that a thousandth below 2001 shows (the others are cut anyway) */
static int math_scientific(const char *s, int end, int exp, int *milli) {
    uint32_t mant = 0;
    int i, scale = exp + 3, significant = 0, point = 0, digits = 0;
    for (i = 0; i < end; i++) {
        if (s[i] == '.') {
            if (point) return -1;
            point = 1;
            continue;
        }
        digits++;
        if (significant < 7) {
            mant = mant * 10 + (uint32_t)(s[i] - '0');
            if (mant) significant++;
            if (point) scale--;
        } else if (!point) scale++;
    }
    if (!digits || s[end - 1] == '.') return -1;
    while (scale < 0 && mant) { mant /= 10; scale++; }
    while (scale > 0 && mant) {
        if (mant > (uint32_t)WE_MATH_LITERAL_MAX * 1000) return -1;
        mant *= 10; scale--;
    }
    if (mant >= (uint32_t)(WE_MATH_LITERAL_MAX + 1) * 1000) return -1;
    *milli = (int)mant;
    return 0;
}

static int math_literal(we_math_parser_t *p, int sign) {
    int first = p->at, end, milli, unit_end, h, r, exp = 0, elen;
    const char *s = p->text;
    end = first;
    while (end < p->length && ((s[end] >= '0' && s[end] <= '9') || s[end] == '.')) end++;
    elen = math_exponent(s + end, p->length - end, &exp);
    if (elen < 0) return -1;
    if (elen) {
        /* a number, a px length or a percentage: the other units read
         * their text (unit_len100) and take no exponent */
        if (math_scientific(s + first, end - first, exp, &milli)) return -1;
        p->at = end + elen;
        if (p->at < p->length && s[p->at] == '%') {
            p->at++;
            return math_node(p, M_PERCENT, 0, 0, 0, milli * sign, MT_LENGTH);
        }
        unit_end = p->at;
        while (unit_end < p->length && w_low(s[unit_end]) >= 'a' && w_low(s[unit_end]) <= 'z') unit_end++;
        if (unit_end == p->at) return math_node(p, M_NUMBER, 0, 0, 0, milli * sign, MT_NUMBER);
        if (!value_is(s + p->at, unit_end - p->at, "px")) return -1;
        p->at = unit_end;
        return math_node(p, M_PX, 0, 0, 0, milli * sign, MT_LENGTH);
    }
    if (math_digits(s + first, p->length - first, &end, &milli)) return -1;
    p->at = first + end;
    unit_end = p->at;
    while (unit_end < p->length && w_low(s[unit_end]) >= 'a' && w_low(s[unit_end]) <= 'z') unit_end++;
    if (p->at < p->length && s[p->at] == '%') {
        p->at++;
        return math_node(p, M_PERCENT, 0, 0, 0, milli * sign, MT_LENGTH);
    }
    if (unit_end == p->at) return math_node(p, M_NUMBER, 0, 0, 0, milli * sign, MT_NUMBER);
    if (value_is(s + p->at, unit_end - p->at, "px")) {
        p->at = unit_end;
        return math_node(p, M_PX, 0, 0, 0, milli * sign, MT_LENGTH);
    }
    /* Lot 117 font-relative and Lot 118 viewport units: hundredths of a px */
    if (unit_code(s + p->at, unit_end - p->at) >= 0) {
        if (unit_len100(s + first, unit_end - first, &h)) return -1;
        if (h > WE_MATH_LITERAL_MAX * 100) return -1;
        p->at = unit_end;
        return math_node(p, M_PX, 0, 0, 0, h * 10 * sign, MT_LENGTH);
    }
    r = math_angle_unit(s + p->at, unit_end - p->at, milli, &h);
    if (r) return -1;
    p->at = unit_end;
    return math_node(p, M_NUMBER, 0, 0, 0, h * sign, MT_ANGLE);
}

/* the CSS Values 4 constants: e, pi, infinity, -infinity, NaN; sign is
 * -1 or 1 after a sign character, 0 without one (only -infinity) */
static int math_constant(we_math_parser_t *p, const char *name, int n, int sign) {
    int v;
    if (value_is(name, n, "infinity") && sign <= 0) v = sign < 0 ? -MV_INF : MV_INF;
    else if (sign) return -1;
    else if (value_is(name, n, "e")) v = 2718;
    else if (value_is(name, n, "pi")) v = 3142;
    else if (value_is(name, n, "nan")) v = MV_NAN;
    else {
        /* Lot 119: the channel keywords of a relative color (r, g, b,
         * alpha, h, s, l, w) are numbers while it is parsed */
        int k;
        for (k = 0; k < math_chan_n && !value_is(name, n, math_chan_names[k]); k++) {}
        if (k == math_chan_n) return -1;
        v = math_chan_vals[k];
    }
    return math_node(p, M_NUMBER, 0, 0, 0, v, MT_NUMBER);
}

static const char *const math_names[] = {
    "calc", "min", "max", "clamp", "round", "mod", "rem", "abs", "sign", "pow", "sqrt", "hypot",
    "log", "exp", "sin", "cos", "tan", "asin", "acos", "atan", "atan2", 0
};
static const uint8_t math_ops[] = {
    M_ADD, M_MIN, M_MAX, M_CLAMP, M_ROUND, M_MOD, M_REM, M_ABS, M_SIGN, M_POW, M_SQRT, M_HYPOT,
    M_LOG, M_EXP, M_SIN, M_COS, M_TAN, M_ASIN, M_ACOS, M_ATAN, M_ATAN2
};

static int math_kind(const char *name, int n) {
    int k;
    for (k = 0; math_names[k]; k++) if (value_is(name, n, math_names[k])) return math_ops[k];
    return -1;
}

/* the node of a function of count arguments, checked against the types of
 * CSS Values 4 10.10 */
static int math_function_node(we_math_parser_t *p, int kind, const int *args, int count, int strategy) {
    int t0 = p->type[args[0]], i, result;
    for (i = 1; i < count; i++) {
        int same = p->type[args[i]] == t0;
        if (kind == M_LOG || kind == M_POW) same = p->type[args[i]] == MT_NUMBER;
        if (!same) return -1;
    }
    switch (kind) {
    case M_ADD: return count == 1 ? args[0] : -1;
    case M_MIN: case M_MAX: case M_HYPOT:
        result = args[0];
        for (i = 1; i < count; i++) {
            result = math_node(p, kind, result, args[i], 0, 0, t0);
            if (result < 0) return -1;
        }
        if (kind == M_HYPOT && count == 1) result = math_node(p, M_ABS, args[0], args[0], 0, 0, t0);
        return result;
    case M_CLAMP:
        return count == 3 ? math_node(p, kind, args[0], args[1], args[2], 0, t0) : -1;
    case M_ROUND: {
        int step;
        if (count == 1) {
            if (t0 != MT_NUMBER) return -1;
            step = math_node(p, M_NUMBER, 0, 0, 0, 1000, MT_NUMBER);
        } else if (count == 2) step = args[1];
        else return -1;
        if (step < 0) return -1;
        return math_node(p, kind, args[0], step, strategy, 0, t0);
    }
    case M_MOD: case M_REM:
        return count == 2 ? math_node(p, kind, args[0], args[1], 0, 0, t0) : -1;
    case M_ABS:
        return count == 1 ? math_node(p, kind, args[0], args[0], 0, 0, t0) : -1;
    case M_SIGN:
        return count == 1 ? math_node(p, kind, args[0], args[0], 0, 0, MT_NUMBER) : -1;
    case M_POW:
        return count == 2 && t0 == MT_NUMBER ? math_node(p, kind, args[0], args[1], 0, 0, MT_NUMBER) : -1;
    case M_SQRT: case M_EXP:
        return count == 1 && t0 == MT_NUMBER ? math_node(p, kind, args[0], args[0], 0, 0, MT_NUMBER) : -1;
    case M_LOG:
        if (t0 != MT_NUMBER || count > 2) return -1;
        return math_node(p, kind, args[0], count == 2 ? args[1] : args[0], count == 2, 0, MT_NUMBER);
    case M_SIN: case M_COS: case M_TAN:
        if (count != 1 || t0 == MT_LENGTH) return -1;
        return math_node(p, kind, args[0], args[0], t0 == MT_NUMBER, 0, MT_NUMBER);
    case M_ASIN: case M_ACOS: case M_ATAN:
        if (count != 1 || t0 != MT_NUMBER) return -1;
        return math_node(p, kind, args[0], args[0], 0, 0, MT_ANGLE);
    case M_ATAN2:
        return count == 2 ? math_node(p, kind, args[0], args[1], 0, 0, MT_ANGLE) : -1;
    default: return -1;
    }
}

/* the rounding strategy that may start round(), -1 when none */
static int math_strategy(we_math_parser_t *p) {
    static const char *const names[] = { "nearest", "up", "down", "to-zero" };
    int start, end, k, save = p->at;
    math_space(p);
    start = end = p->at;
    while (end < p->length && ((w_low(p->text[end]) >= 'a' && w_low(p->text[end]) <= 'z') ||
                               p->text[end] == '-')) end++;
    for (k = 0; k < 4; k++) if (value_is(p->text + start, end - start, names[k])) {
        p->at = end; math_space(p);
        if (p->at < p->length && p->text[p->at] == ',') { p->at++; return k; }
        break;
    }
    p->at = save;
    return -1;
}

static int math_atom(we_math_parser_t *p) {
    int first, sign = 1, result, had_sign = 0;
    math_space(p);
    if (p->at == p->length) return -1;
    if (p->text[p->at] == '+' || p->text[p->at] == '-') {
        sign = p->text[p->at++] == '-' ? -1 : 1;
        had_sign = 1;
    }
    first = p->at;
    if (first < p->length && ((p->text[first] >= '0' && p->text[first] <= '9') || p->text[first] == '.'))
        return math_literal(p, sign);
    if (first >= p->length || p->depth >= WE_MATH_DEPTH_MAX) return -1;
    if (p->text[p->at] == '(') {
        if (sign < 0) return -1;
        p->depth++;
        p->at++; result = math_sum(p); math_space(p);
        if (result < 0 || p->at == p->length || p->text[p->at++] != ')') return -1;
    } else {
        int name = p->at, kind, args[WE_MATH_ARGS_MAX], count = 0, strategy = MR_NEAREST;
        while (p->at < p->length && ((w_low(p->text[p->at]) >= 'a' && w_low(p->text[p->at]) <= 'z') ||
                                     (p->at > name && p->text[p->at] >= '0' && p->text[p->at] <= '9'))) p->at++;
        if (p->at == name) return -1;
        if (p->at == p->length || p->text[p->at] != '(') return math_constant(p, p->text + name, p->at - name, had_sign ? sign : 0);
        if (sign < 0) return -1;
        kind = math_kind(p->text + name, p->at - name);
        if (kind < 0) return -1;
        p->at++;
        p->depth++;
        if (kind == M_ROUND) {
            int s = math_strategy(p);
            if (s >= 0) strategy = s;
        }
        for (;;) {
            if (count == WE_MATH_ARGS_MAX) return -1;     /* Lot 116: was 4 */
            args[count] = math_sum(p);
            if (args[count] < 0) return -1;
            count++; math_space(p);
            if (p->at == p->length) return -1;
            if (p->text[p->at] != ',') break;
            p->at++;
        }
        if (p->text[p->at++] != ')') return -1;
        result = math_function_node(p, kind, args, count, strategy);
        if (result < 0) return -1;
    }
    p->depth--;
    return result;
}

static int math_product(we_math_parser_t *p) {
    int left = math_atom(p);
    if (left < 0) return -1;
    for (;;) {
        int op, right, lt, rt, type, saved = p->at;
        math_space(p);
        if (p->at == p->length || (p->text[p->at] != '*' && p->text[p->at] != '/')) { p->at = saved; break; }
        op = p->text[p->at++] == '*' ? M_MUL : M_DIV;
        right = math_atom(p);
        if (right < 0) return -1;
        lt = p->type[left]; rt = p->type[right];
        /* a product takes one number; a quotient a number or the same type
         * (typed arithmetic, CSS Values 4 10.10.1: a number) */
        if (op == M_MUL) {
            if (lt != MT_NUMBER && rt != MT_NUMBER) return -1;
            type = lt == MT_NUMBER ? rt : lt;
        } else {
            if (rt != MT_NUMBER && rt != lt) return -1;
            type = rt == MT_NUMBER ? lt : MT_NUMBER;
        }
        left = math_node(p, op, left, right, 0, 0, type);
        if (left < 0) return -1;
    }
    return left;
}

static int math_sum(we_math_parser_t *p) {
    int left = math_product(p);
    if (left < 0) return -1;
    for (;;) {
        int op, right, saved = p->at;
        math_space(p);
        if (p->at == p->length || (p->text[p->at] != '+' && p->text[p->at] != '-')) { p->at = saved; break; }
        /* Binary +/- require whitespace on each side; unary signs do not. */
        if (p->at == saved || p->at + 1 == p->length || !w_space(p->text[p->at + 1])) return -1;
        op = p->text[p->at++] == '+' ? M_ADD : M_SUB;
        right = math_product(p);
        if (right < 0 || p->type[left] != p->type[right]) return -1;
        left = math_node(p, op, left, right, 0, 0, p->type[left]);
        if (left < 0) return -1;
    }
    return left;
}

/* an expression of the given type (MT_LENGTH: percentages allowed) */
static int parse_math_typed(const char *v, int len, we_length_t *out, int type) {
    static we_math_parser_t parser;
    int root, result, bad;
    w_zero(out, sizeof(*out)); w_zero(&parser, sizeof(parser));
    parser.text = v; parser.length = len; parser.out = out;
    root = math_sum(&parser); math_space(&parser);
    if (root < 0 || parser.at != len || root != out->count - 1 || parser.type[root] != type) return -1;
    /* A zero denominator and constant overflow are rejected before cascade. */
    math_parse_check = 1;
    bad = math_eval_milli(out, 0, &result);
    math_parse_check = 0;
    return bad ? -1 : 0;
}

static int parse_math(const char *v, int len, we_length_t *out) {
    return parse_math_typed(v, len, out, MT_LENGTH);
}

/* A top-level value in thousandths as a property uses it: NaN is 0 and
 * the infinities are the largest finite value (CSS Values 4 10.9). */
static int m_used_milli(int v) {
    if (v == MV_NAN) return 0;
    if (v == MV_INF) return MV_MAX;
    if (v == -MV_INF) return -MV_MAX;
    return v;
}

static int math_typed_milli(const char *v, int len, int type, int *milli) {
    we_length_t e;
    if (parse_math_typed(v, len, &e, type) || e.percent || math_eval_milli(&e, 0, milli)) return -1;
    *milli = m_used_milli(*milli);
    return 0;
}

/* a number-valued expression (no percentage) in thousandths */
static int math_number_milli(const char *v, int len, int *milli) {
    return math_typed_milli(v, len, MT_NUMBER, milli);
}

/* an angle-valued expression in thousandths of a degree */
static int math_angle_milli(const char *v, int len, int *milli) {
    return math_typed_milli(v, len, MT_ANGLE, milli);
}

/* A number expression rounded to the nearest integer (halves up, CSS
 * Values 4 10.9) and clamped to lo..hi; -1 when it is not one. */
static int math_integer(const char *v, int len, int lo, int hi, int *out) {
    int milli, r;
    if (!math_function(v, len) || math_number_milli(v, len, &milli)) return -1;
    r = milli >= 0 ? (milli + 500) / 1000 : -((-milli - 500 + 999) / 1000);
    *out = r < lo ? lo : r > hi ? hi : r;
    return 0;
}

/* a length expression against basis in thousandths of a px, used value */
static int math_len_milli(const we_length_t *e, int basis, int *milli) {
    if (math_eval_milli(e, basis, milli)) return -1;
    *milli = m_used_milli(*milli);
    return 0;
}

/* Lot 118: a math length of a property that reads whole px (parse_len):
 * no percentage, the used px cut toward zero, within +-2000 as unit_len */
static int math_fixed_px(const char *v, int len, int *px) {
    we_length_t e;
    int milli, r;
    if (parse_math(v, len, &e) || e.percent || math_len_milli(&e, 0, &milli)) return -1;
    r = milli / 1000;
    *px = r > 2000 ? 2000 : r < -2000 ? -2000 : r;
    return 0;
}

static int math_function(const char *v, int len) {
    int k, n;
    for (k = 0; math_names[k]; k++) {
        n = w_len(math_names[k]);
        if (len > n && w_ieq_n(v, len, math_names[k]) && v[n] == '(') return 1;
    }
    return 0;
}

int we_math_milli(const char *v, int len, const we_unit_ctx_t *c, int basis, int *type, int *milli) {
    static const we_unit_ctx_t defaults;
    we_length_t e;
    int t;
    if (!v || len < 0 || !type || !milli) return -1;
    u_ctx_set(c ? c : &defaults);
    for (t = MT_NUMBER; t <= MT_ANGLE; t++) {
        if (parse_math_typed(v, len, &e, t)) continue;
        if (math_len_milli(&e, basis, milli)) return -1;
        *type = t;
        return 0;
    }
    return -1;
}
