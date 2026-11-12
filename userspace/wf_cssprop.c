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

int wf_cssprop_syntax(const char *s, int n) {
    static const char *const names[] = {"<length>", "<percentage>", "<length-percentage>", "<number>",
                                        "<integer>", "<color>", 0};
    static const int codes[] = {WF_CSSPROP_LENGTH, WF_CSSPROP_PERCENT, WF_CSSPROP_LENPCT, WF_CSSPROP_NUMBER,
                                WF_CSSPROP_INTEGER, WF_CSSPROP_COLOR};
    int a = 0, b = n, k;
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
    for (k = 0; names[k]; k++) if (cp_eqn(s + a, b - a, names[k])) return codes[k];
    return WF_CSSPROP_BAD;
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

/* calc( min( max( clamp( with balanced parentheses to the end */
static int cp_math(const char *v) {
    static const char *const fn[] = {"calc(", "min(", "max(", "clamp(", 0};
    int k, i, depth = 0;
    for (k = 0; fn[k]; k++) {
        int m = cp_len(fn[k]);
        if (cp_eqn(v, m, fn[k])) break;
    }
    if (!fn[k]) return 0;
    for (i = 0; v[i]; i++) {
        if (v[i] == '(') depth++;
        else if (v[i] == ')') {
            depth--;
            if (depth < 0) return 0;
            if (depth == 0 && v[i + 1]) return 0;
        }
    }
    return depth == 0 && i > 0 && v[i - 1] == ')';
}

/* one dimension: 1 length, 2 percentage, 3 plain number (*frac set), 0 other */
static int cp_dim(const char *v, int *frac, int *zero) {
    int i = 0, a, k;
    if (!cp_number(v, &i, frac)) return 0;
    *zero = 1;
    for (k = 0; k < i; k++) if (cp_digit(v[k]) && v[k] != '0') *zero = 0;
    if (!v[i]) return 3;
    if (v[i] == '%' && !v[i + 1]) return 2;
    a = i;
    while (v[i] && ((v[i] >= 'a' && v[i] <= 'z') || (v[i] >= 'A' && v[i] <= 'Z'))) i++;
    if (v[i] || !cp_unit(v + a, i - a)) return 0;
    return 1;
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
    int frac = 0, zero = 0, d;
    if (!v) return 0;
    if (syntax == WF_CSSPROP_ANY) return 1;
    if (!v[0]) return 0;
    if (syntax == WF_CSSPROP_COLOR) return cp_color(v);
    if (syntax < WF_CSSPROP_LENGTH || syntax > WF_CSSPROP_INTEGER) return 0;
    if (cp_math(v)) return 1;
    d = cp_dim(v, &frac, &zero);
    switch (syntax) {
    case WF_CSSPROP_LENGTH: return d == 1 || (d == 3 && zero);
    case WF_CSSPROP_PERCENT: return d == 2;
    case WF_CSSPROP_LENPCT: return d == 1 || d == 2 || (d == 3 && zero);
    case WF_CSSPROP_NUMBER: return d == 3;
    default: return d == 3 && !frac;   /* integer */
    }
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
            out->syntax = (unsigned char)wf_cssprop_syntax(body + va, vb - va);
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
        if (!out->has_initial || !wf_cssprop_match(out->syntax, out->initial) ||
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
