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

/* ---- Lot 91: math functions with their types (CSS Values 4) ---------- */

typedef struct { const char *s; int i, depth; } cp_mp_t;

static int cp_mfn(const char *s) {
    if (cp_low(s[0]) == 'c' && cp_low(s[1]) == 'a' && cp_low(s[2]) == 'l' && cp_low(s[3]) == 'c' && s[4] == '(') return 1;
    if (cp_low(s[0]) == 'm' && cp_low(s[1]) == 'i' && cp_low(s[2]) == 'n' && s[3] == '(') return 2;
    if (cp_low(s[0]) == 'm' && cp_low(s[1]) == 'a' && cp_low(s[2]) == 'x' && s[3] == '(') return 3;
    if (cp_low(s[0]) == 'c' && cp_low(s[1]) == 'l' && cp_low(s[2]) == 'a' && cp_low(s[3]) == 'm' &&
        cp_low(s[4]) == 'p' && s[5] == '(') return 4;
    return 0;
}

/* + and -: the same type, or a length with a percentage */
static int cp_tadd(int a, int b) {
    int la, lb;
    if (!a || !b) return 0;
    if (a == b) return a;
    la = a == WF_CSSPROP_LENGTH || a == WF_CSSPROP_PERCENT || a == WF_CSSPROP_LENPCT;
    lb = b == WF_CSSPROP_LENGTH || b == WF_CSSPROP_PERCENT || b == WF_CSSPROP_LENPCT;
    return la && lb ? WF_CSSPROP_LENPCT : 0;
}

static void cp_ws(cp_mp_t *m) { while (cp_sp(m->s[m->i])) m->i++; }

static int cp_msum(cp_mp_t *m);

/* the arguments of a math function after its '(' */
static int cp_margs(cp_mp_t *m, int fn) {
    int t, n = 1, u;
    cp_ws(m);
    t = cp_msum(m);
    cp_ws(m);
    while (t && m->s[m->i] == ',' && fn != 1) {
        m->i++;
        cp_ws(m);
        u = cp_msum(m);
        cp_ws(m);
        t = cp_tadd(t, u);
        n++;
    }
    if (!t || m->s[m->i] != ')') return 0;
    m->i++;
    if (fn == 4 && n != 3) return 0;
    return t;
}

static int cp_mvalue(cp_mp_t *m) {
    int fn, t, i, frac, a;
    if (++m->depth > 16) return 0;
    cp_ws(m);
    if (m->s[m->i] == '(') {
        m->i++;
        cp_ws(m);
        t = cp_msum(m);
        cp_ws(m);
        if (!t || m->s[m->i] != ')') return 0;
        m->i++;
        m->depth--;
        return t;
    }
    fn = cp_mfn(m->s + m->i);
    if (fn) {
        m->i += fn == 1 || fn == 4 ? (fn == 1 ? 5 : 6) : 4;
        t = cp_margs(m, fn);
        m->depth--;
        return t;
    }
    i = m->i;
    if (!cp_number(m->s, &i, &frac)) return 0;
    if (m->s[i] == '%') { m->i = i + 1; m->depth--; return WF_CSSPROP_PERCENT; }
    a = i;
    while ((m->s[i] >= 'a' && m->s[i] <= 'z') || (m->s[i] >= 'A' && m->s[i] <= 'Z')) i++;
    m->i = i;
    m->depth--;
    if (i == a) return WF_CSSPROP_NUMBER;
    return cp_unit_type(m->s + a, i - a);
}

static int cp_mproduct(cp_mp_t *m) {
    int t = cp_mvalue(m);
    while (t) {
        int j = m->i, u;
        char op;
        while (cp_sp(m->s[j])) j++;
        op = m->s[j];
        if (op != '*' && op != '/') break;
        m->i = j + 1;
        u = cp_mvalue(m);
        if (!u) return 0;
        if (op == '*') t = t == WF_CSSPROP_NUMBER ? u : u == WF_CSSPROP_NUMBER ? t : 0;
        else t = u == WF_CSSPROP_NUMBER ? t : 0;
    }
    return t;
}

static int cp_msum(cp_mp_t *m) {
    int t = cp_mproduct(m);
    while (t) {
        int j = m->i, u;
        if (!cp_sp(m->s[j])) break;
        while (cp_sp(m->s[j])) j++;
        if ((m->s[j] != '+' && m->s[j] != '-') || !cp_sp(m->s[j + 1])) break;
        m->i = j + 1;
        u = cp_mproduct(m);
        t = cp_tadd(t, u);
    }
    return t;
}

int wf_cssprop_math_type(const char *v) {
    cp_mp_t m;
    int t;
    if (!v || !cp_mfn(v)) return WF_CSSPROP_BAD;
    m.s = v;
    m.i = 0;
    m.depth = 0;
    t = cp_mvalue(&m);
    if (!t || v[m.i]) return WF_CSSPROP_BAD;
    return t;
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
