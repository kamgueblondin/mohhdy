/* webeng_cssq.inc - Lot 73: CSS conditional rules, selector lists and
 * custom properties. Included by webeng.c only, the single engine.
 *
 * - @media (Media Queries 4 subset): a comma list of queries, each
 *   [not|only] type [and condition] or a condition alone; conditions are
 *   parenthesised features joined by and / or (not mixed without
 *   parentheses), or "not (...)". Features: width, height (min-, max-
 *   and the range forms "width >= 600px", "400px < width <= 700px"),
 *   orientation, aspect-ratio, prefers-color-scheme (light),
 *   prefers-reduced-motion (no-preference), hover and any-hover (hover),
 *   pointer and any-pointer (fine), color (8 bits), monochrome (0),
 *   resolution (1dppx = 96dpi). Lengths in px, em and rem (16 px). The
 *   media size is the layout viewport (we_render) or we_set_media; 640 x
 *   480 when unset. Types all and screen match; print, speech and the
 *   others do not.
 * - @supports: (property: value) is supported when the engine applies it
 *   with no diag (tried on a scratch node, every side effect undone;
 *   custom properties always); selector(...) when the selector parses
 *   (pseudo-elements included); not, and, or as in @media.
 * - @layer (lot 74): cascade layers ranked in post-order, unlayered last,
 *   !important reversed (webeng.c parse_css_range, rule_layer_key);
 *   other at-rules are skipped as before.
 * - :not(), :is(), :matches(), :-webkit-any() and :where() take selector
 *   lists (combinators allowed, nesting up to 3); :is and :not count the
 *   most specific argument, :where nothing. :has() takes relative
 *   selectors (descendant, > child, + next sibling, ~ later sibling; not
 *   nested in :has; lot 74: the chain of an argument is anchored at the
 *   element, no part of it matches outside), its specificity is its most
 *   specific argument. :placeholder-shown, :indeterminate (progress without
 *   value) and :open (details) read the form state. :link and :any-link (a or
 *   area with href), :checked, :disabled, :enabled, :first-of-type,
 *   :last-of-type, :only-of-type; :visited, :hover and :active
 *   never match (static page); lot 74: :focus, :focus-visible,
 *   :focus-within, :required, :optional, :read-only, :read-write,
 *   :valid, :invalid follow the live form state (webeng_finput.inc);
 *   :target and :target-within the URL fragment (we_set_target).
 * - Custom properties: "--name: value" declarations (case-sensitive
 *   names, inherited, up to WE_VARS_MAX per element) and var(--name) /
 *   var(--name, fallback) in any value, nested. Custom values are resolved
 *   where they are declared; a reference to itself makes the property
 *   invalid ("css variable cycle"); a missing variable without fallback
 *   drops the declaration ("css variable undefined"). */

#define WE_MEDIA_W_DEFAULT 640
#define WE_MEDIA_H_DEFAULT 480
#define WE_CSS_NEST_MAX 4
#define WE_SEL_LOGIC_DEPTH 3
#define WE_VAR_VALUE_MAX 512

static int cssq_media_w(const we_doc_t *d) { return d->media_w > 0 ? d->media_w : WE_MEDIA_W_DEFAULT; }
static int cssq_media_h(const we_doc_t *d) { return d->media_h > 0 ? d->media_h : WE_MEDIA_H_DEFAULT; }

void we_set_media(we_doc_t *d, int w, int h) {
    if (!d) return;
    d->media_w = (int16_t)(w > 0 && w <= WE_LAYOUT_MAX ? w : 0);
    d->media_h = (int16_t)(h > 0 && h <= WE_LAYOUT_MAX ? h : 0);
}

static int cssq_trim(const char *s, int *a, int *b) {
    while (*a < *b && w_space(s[*a])) (*a)++;
    while (*b > *a && w_space(s[*b - 1])) (*b)--;
    return *b - *a;
}

/* exact, case-insensitive */
static int cssq_is(const char *s, int n, const char *word) {
    return n == w_len(word) && w_ieq_n(s, n, word);
}

/* a length in px, em or rem (16 px), in hundredths of a px; 0 alone */
static int cssq_len(const char *v, int n, int *out) {
    int i = 0, val = 0, frac = 0, fd = 0, dig = 0;
    while (i < n && v[i] >= '0' && v[i] <= '9') { if (val < 100000) val = val * 10 + v[i] - '0'; i++; dig++; }
    if (i < n && v[i] == '.') {
        i++;
        while (i < n && v[i] >= '0' && v[i] <= '9') { if (fd < 2) { frac = frac * 10 + v[i] - '0'; fd++; } i++; dig++; }
    }
    if (!dig) return -1;
    if (fd == 1) frac *= 10;
    val = val * 100 + frac;
    if (i == n) { if (val) return -1; *out = 0; return 0; }
    if (cssq_is(v + i, n - i, "px")) { *out = val; return 0; }
    if (cssq_is(v + i, n - i, "em") || cssq_is(v + i, n - i, "rem")) { *out = val * WE_FONT_PX_DEFAULT; return 0; }
    return -1;
}

/* a resolution in dppx hundredths: dppx, x, dpi (96 = 1), dpcm */
static int cssq_res(const char *v, int n, int *out) {
    int i = 0, val = 0, frac = 0, fd = 0, dig = 0;
    while (i < n && v[i] >= '0' && v[i] <= '9') { if (val < 100000) val = val * 10 + v[i] - '0'; i++; dig++; }
    if (i < n && v[i] == '.') {
        i++;
        while (i < n && v[i] >= '0' && v[i] <= '9') { if (fd < 2) { frac = frac * 10 + v[i] - '0'; fd++; } i++; dig++; }
    }
    if (!dig) return -1;
    if (fd == 1) frac *= 10;
    val = val * 100 + frac;
    if (cssq_is(v + i, n - i, "dppx") || cssq_is(v + i, n - i, "x")) { *out = val; return 0; }
    if (cssq_is(v + i, n - i, "dpi")) { *out = val / 96; return 0; }
    if (cssq_is(v + i, n - i, "dpcm")) { *out = val * 254 / 9600; return 0; }
    return -1;
}

/* a ratio "a / b" or "a": returns a * 1000 / b */
static int cssq_ratio(const char *v, int n, int *out) {
    int i = 0, a = 0, b = 0, da = 0, db = 0;
    while (i < n && v[i] >= '0' && v[i] <= '9') { if (a < 100000) a = a * 10 + v[i] - '0'; i++; da++; }
    while (i < n && w_space(v[i])) i++;
    if (i == n) b = 1, db = 1;
    else if (v[i++] == '/') {
        while (i < n && w_space(v[i])) i++;
        while (i < n && v[i] >= '0' && v[i] <= '9') { if (b < 100000) b = b * 10 + v[i] - '0'; i++; db++; }
    }
    if (!da || !db || i != n || !b) return -1;
    *out = (int)((long)a * 1000 / b);
    return 0;
}

enum { CSSQ_LT = 1, CSSQ_LE, CSSQ_GT, CSSQ_GE, CSSQ_EQ };

static int cssq_compare(int have, int op, int want) {
    if (op == CSSQ_LT) return have < want;
    if (op == CSSQ_LE) return have <= want;
    if (op == CSSQ_GT) return have > want;
    if (op == CSSQ_GE) return have >= want;
    return have == want;
}

/* a range feature value of the page: hundredths of a px, dppx or a
 * ratio * 1000; kind 1 length, 2 resolution, 3 ratio, 4 integer, 0 none */
static int cssq_range_value(const we_doc_t *d, const char *name, int n, int *value) {
    if (cssq_is(name, n, "width")) { *value = cssq_media_w(d) * 100; return 1; }
    if (cssq_is(name, n, "height")) { *value = cssq_media_h(d) * 100; return 1; }
    if (cssq_is(name, n, "resolution")) { *value = 100; return 2; }
    if (cssq_is(name, n, "aspect-ratio")) { *value = (int)((long)cssq_media_w(d) * 1000 / cssq_media_h(d)); return 3; }
    if (cssq_is(name, n, "color")) { *value = 8; return 4; }
    if (cssq_is(name, n, "monochrome")) { *value = 0; return 4; }
    return 0;
}

static int cssq_parse_value(int kind, const char *v, int n, int *out) {
    if (kind == 1) return cssq_len(v, n, out);
    if (kind == 2) return cssq_res(v, n, out);
    if (kind == 3) return cssq_ratio(v, n, out);
    {
        int i = 0, val = 0;
        if (!n) return -1;
        for (; i < n; i++) { if (v[i] < '0' || v[i] > '9') return -1; if (val < 100000) val = val * 10 + v[i] - '0'; }
        *out = val;
        return 0;
    }
}

/* one comparison operator at s[*at]: returns CSSQ_* or 0 */
static int cssq_op(const char *s, int n, int *at) {
    int i = *at, op = 0;
    if (i < n && s[i] == '<') { op = CSSQ_LT; i++; if (i < n && s[i] == '=') { op = CSSQ_LE; i++; } }
    else if (i < n && s[i] == '>') { op = CSSQ_GT; i++; if (i < n && s[i] == '=') { op = CSSQ_GE; i++; } }
    else if (i < n && s[i] == '=') { op = CSSQ_EQ; i++; }
    *at = i;
    return op;
}

static int cssq_flip(int op) {
    return op == CSSQ_LT ? CSSQ_GT : op == CSSQ_LE ? CSSQ_GE : op == CSSQ_GT ? CSSQ_LT : op == CSSQ_GE ? CSSQ_LE : op;
}

/* the range form: "name op value", "value op name", "v1 op name op v2" */
static int cssq_feature_range(we_doc_t *d, const char *s, int n) {
    int i = 0, a, cuts[2], ops[2], count = 0, name = -1, k, kind = 0, have = 0, ok = 1;
    int parts_a[3], parts_b[3];
    a = 0;
    while (i < n) {
        if (s[i] == '<' || s[i] == '>' || s[i] == '=') {
            if (count == 2) return -1;
            cuts[count] = i;
            ops[count] = cssq_op(s, n, &i);
            parts_b[count] = cuts[count];
            parts_a[count] = a;
            a = i;
            count++;
        } else i++;
    }
    if (!count) return -1;
    parts_a[count] = a; parts_b[count] = n;
    for (k = 0; k <= count; k++) {
        int pa = parts_a[k], pb = parts_b[k], kk;
        if (!cssq_trim(s, &pa, &pb)) return -1;
        parts_a[k] = pa; parts_b[k] = pb;
        kk = cssq_range_value(d, s + pa, pb - pa, &have);
        if (kk) { if (name >= 0) return -1; name = k; kind = kk; }
    }
    if (name < 0) {
        diag(d, "media feature refused", -1);
        return 0;
    }
    if (count == 2) {
        int lo, hi;
        if (name != 1) return -1;
        /* both operators must point the same way */
        if ((ops[0] == CSSQ_LT || ops[0] == CSSQ_LE) != (ops[1] == CSSQ_LT || ops[1] == CSSQ_LE) || ops[0] == CSSQ_EQ || ops[1] == CSSQ_EQ) return -1;
        if (cssq_parse_value(kind, s + parts_a[0], parts_b[0] - parts_a[0], &lo) ||
            cssq_parse_value(kind, s + parts_a[2], parts_b[2] - parts_a[2], &hi)) return -1;
        ok = cssq_compare(have, cssq_flip(ops[0]), lo) && cssq_compare(have, ops[1], hi);
        return ok;
    }
    {
        int other = name ? 0 : 1, want, op = name ? cssq_flip(ops[0]) : ops[0];
        if (cssq_parse_value(kind, s + parts_a[other], parts_b[other] - parts_a[other], &want)) return -1;
        return cssq_compare(have, op, want);
    }
}

/* one media feature inside its parentheses: 1 true, 0 false, -1 bad */
static int cssq_feature(we_doc_t *d, const char *s, int n) {
    int colon = 0, a = 0, b = n, va, vb, kind, have, want, prefix = 0;
    while (colon < n && s[colon] != ':') colon++;
    if (colon == n) {
        int i;
        for (i = 0; i < n; i++) if (s[i] == '<' || s[i] == '>' || s[i] == '=') return cssq_feature_range(d, s, n);
        if (!cssq_trim(s, &a, &b)) return -1;
        /* boolean context: true unless the value would be 0 or none */
        if (cssq_is(s + a, b - a, "monochrome") || cssq_is(s + a, b - a, "grid")) return 0;
        if (cssq_is(s + a, b - a, "prefers-reduced-motion") || cssq_is(s + a, b - a, "prefers-reduced-transparency")) return 0;
        if (cssq_range_value(d, s + a, b - a, &have) || cssq_is(s + a, b - a, "orientation") ||
            cssq_is(s + a, b - a, "hover") || cssq_is(s + a, b - a, "any-hover") || cssq_is(s + a, b - a, "pointer") ||
            cssq_is(s + a, b - a, "any-pointer") || cssq_is(s + a, b - a, "prefers-color-scheme")) return 1;
        diag(d, "media feature refused", -1);
        return 0;
    }
    b = colon;
    if (!cssq_trim(s, &a, &b)) return -1;
    va = colon + 1; vb = n;
    if (!cssq_trim(s, &va, &vb)) return -1;
    if (b - a > 4 && w_ieq_n(s + a, b - a, "min-")) { prefix = CSSQ_GE; a += 4; }
    else if (b - a > 4 && w_ieq_n(s + a, b - a, "max-")) { prefix = CSSQ_LE; a += 4; }
    kind = cssq_range_value(d, s + a, b - a, &have);
    if (kind) {
        if (cssq_parse_value(kind, s + va, vb - va, &want)) return -1;
        return cssq_compare(have, prefix ? prefix : CSSQ_EQ, want);
    }
    if (prefix) { diag(d, "media feature refused", -1); return 0; }
    if (cssq_is(s + a, b - a, "orientation")) {
        int portrait = cssq_media_h(d) >= cssq_media_w(d);
        if (cssq_is(s + va, vb - va, "portrait")) return portrait;
        if (cssq_is(s + va, vb - va, "landscape")) return !portrait;
        return -1;
    }
    if (cssq_is(s + a, b - a, "prefers-color-scheme")) {
        if (cssq_is(s + va, vb - va, "light")) return 1;
        if (cssq_is(s + va, vb - va, "dark")) return 0;
        return -1;
    }
    if (cssq_is(s + a, b - a, "prefers-reduced-motion") || cssq_is(s + a, b - a, "prefers-reduced-transparency")) {
        if (cssq_is(s + va, vb - va, "no-preference")) return 1;
        if (cssq_is(s + va, vb - va, "reduce")) return 0;
        return -1;
    }
    if (cssq_is(s + a, b - a, "hover") || cssq_is(s + a, b - a, "any-hover")) {
        if (cssq_is(s + va, vb - va, "hover")) return 1;
        if (cssq_is(s + va, vb - va, "none")) return 0;
        return -1;
    }
    if (cssq_is(s + a, b - a, "pointer") || cssq_is(s + a, b - a, "any-pointer")) {
        if (cssq_is(s + va, vb - va, "fine")) return 1;
        if (cssq_is(s + va, vb - va, "coarse") || cssq_is(s + va, vb - va, "none")) return 0;
        return -1;
    }
    diag(d, "media feature refused", -1);
    return 0;
}

static int cssq_cond(we_doc_t *d, const char *s, int n, int supports, int depth);
static void apply_decl(we_doc_t *d, we_node_t *n, const char *p, int pl, const char *v, int vl);

/* @supports (property: value) */
static we_node_t g_cssq_scratch;
static int cssq_supports_decl(we_doc_t *d, const char *s, int n) {
    int colon = 0, pa = 0, pb, va, vb, ok;
    uint32_t ndiag = d->ndiag, dropped = d->diag_dropped, pool = d->pool_used;
    uint32_t applied = d->perf.decls_applied, unknown = d->perf.unknown_props;
    while (colon < n && s[colon] != ':') colon++;
    if (colon == n) return 0;
    pb = colon; va = colon + 1; vb = n;
    if (!cssq_trim(s, &pa, &pb) || !cssq_trim(s, &va, &vb)) return 0;
    w_zero(&g_cssq_scratch, sizeof(g_cssq_scratch));
    g_cssq_scratch.parent = -1; g_cssq_scratch.first = g_cssq_scratch.last = g_cssq_scratch.next = -1;
    g_cssq_scratch.font_px = g_cssq_scratch.font_parent_px = WE_FONT_PX_DEFAULT;
    apply_decl(d, &g_cssq_scratch, s + pa, pb - pa, s + va, vb - va);
    ok = d->ndiag == ndiag && d->diag_dropped == dropped && d->perf.unknown_props == unknown;
    d->ndiag = ndiag; d->diag_dropped = dropped; d->pool_used = pool;
    d->perf.decls_applied = applied; d->perf.unknown_props = unknown;
    return ok;
}

static int parse_sel(const char *s, int len, we_rule_t *r);
static we_rule_t g_cssq_rule[WE_SEL_LOGIC_DEPTH + 1];

/* one parenthesised term "( ... )" or a function selector(...) */
static int cssq_term(we_doc_t *d, const char *s, int n, int *at, int supports, int depth) {
    int i = *at, close, level = 0, a, b, sel = 0;
    if (supports && n - i > 9 && w_ieq_n(s + i, n - i, "selector(")) { sel = 1; i += 8; }
    if (i >= n || s[i] != '(') return -1;
    for (close = i; close < n; close++) {
        if (s[close] == '(') level++;
        else if (s[close] == ')' && --level == 0) break;
    }
    if (close == n) return -1;
    *at = close + 1;
    a = i + 1; b = close;
    if (!cssq_trim(s, &a, &b)) return -1;
    if (sel) {
        we_rule_t *rule = &g_cssq_rule[WE_SEL_LOGIC_DEPTH];
        return parse_sel(s + a, b - a, rule) == 0;   /* pseudo-elements allowed */
    }
    if (s[a] == '(' || (b - a > 4 && w_ieq_n(s + a, b - a, "not") && (w_space(s[a + 3]) || s[a + 3] == '(')))
        return cssq_cond(d, s + a, b - a, supports, depth + 1);
    return supports ? cssq_supports_decl(d, s + a, b - a) : cssq_feature(d, s + a, b - a);
}

/* not T | T (and T)* | T (or T)*: 1, 0 or -1 (bad syntax) */
static int cssq_cond(we_doc_t *d, const char *s, int n, int supports, int depth) {
    int i = 0, result, joiner = 0, r;
    if (depth > WE_CSS_NEST_MAX) return -1;
    while (i < n && w_space(s[i])) i++;
    if (n - i > 3 && w_ieq_n(s + i, n - i, "not") && (w_space(s[i + 3]) || s[i + 3] == '(')) {
        i += 3;
        while (i < n && w_space(s[i])) i++;
        r = cssq_term(d, s, n, &i, supports, depth);
        while (i < n && w_space(s[i])) i++;
        if (r < 0 || i != n) return -1;
        return !r;
    }
    result = cssq_term(d, s, n, &i, supports, depth);
    if (result < 0) return -1;
    for (;;) {
        int word;
        while (i < n && w_space(s[i])) i++;
        if (i == n) break;
        if (n - i > 3 && w_ieq_n(s + i, n - i, "and") && (w_space(s[i + 3]) || s[i + 3] == '(')) { word = 1; i += 3; }
        else if (n - i > 2 && w_ieq_n(s + i, n - i, "or") && (w_space(s[i + 2]) || s[i + 2] == '(')) { word = 2; i += 2; }
        else return -1;
        if (joiner && joiner != word) return -1;
        joiner = word;
        while (i < n && w_space(s[i])) i++;
        r = cssq_term(d, s, n, &i, supports, depth);
        if (r < 0) return -1;
        result = word == 1 ? result && r : result || r;
    }
    return result;
}

/* one media query */
static int cssq_query(we_doc_t *d, const char *s, int n) {
    int i = 0, neg = 0, type = 1, a;
    while (i < n && w_space(s[i])) i++;
    if (i == n) return -1;
    if (s[i] == '(') return cssq_cond(d, s + i, n - i, 0, 0);
    if (n - i > 3 && w_ieq_n(s + i, n - i, "not") && (w_space(s[i + 3]) || s[i + 3] == '(')) {
        int j = i + 3;
        while (j < n && w_space(s[j])) j++;
        if (j < n && s[j] == '(') { int r = cssq_cond(d, s + i, n - i, 0, 0); return r; }
        neg = 1; i = j;
    } else if (n - i > 4 && w_ieq_n(s + i, n - i, "only") && w_space(s[i + 4])) {
        i += 4;
        while (i < n && w_space(s[i])) i++;
    }
    a = i;
    while (i < n && w_namec(s[i])) i++;
    if (i == a) return -1;
    if (cssq_is(s + a, i - a, "all") || cssq_is(s + a, i - a, "screen")) type = 1;
    else type = 0;
    while (i < n && w_space(s[i])) i++;
    if (i < n) {
        int r;
        if (!(n - i > 3 && w_ieq_n(s + i, n - i, "and") && (w_space(s[i + 3]) || s[i + 3] == '('))) return -1;
        i += 3;
        r = cssq_cond(d, s + i, n - i, 0, 0);
        if (r < 0) return -1;
        /* "not" does not reach inside the condition: not all and (x) is not (all and x) */
        type = type && r;
    }
    return neg ? !type : type;
}

/* a media query list: true when one query is (an empty list matches) */
static int cssq_media(we_doc_t *d, const char *s, int n) {
    int i = 0, a = 0, level = 0, any = 0, empty = 1;
    for (i = 0; i < n; i++) if (!w_space(s[i])) { empty = 0; break; }
    if (empty) return 1;
    for (i = 0; i <= n; i++) {
        if (i < n && s[i] == '(') level++;
        else if (i < n && s[i] == ')') level--;
        else if (i == n || (s[i] == ',' && !level)) {
            int r = cssq_query(d, s + a, i - a);
            if (r < 0) diag(d, "media query refused", -1);
            else if (r) any = 1;
            a = i + 1;
        }
    }
    return any;
}

static int cssq_supports(we_doc_t *d, const char *s, int n) {
    int r = cssq_cond(d, s, n, 1, 0);
    if (r < 0) { diag(d, "supports condition refused", -1); return 0; }
    return r;
}

/* ---------- selector lists: :not() :is() :where() ---------- */

static int cssq_logic_kind(const char *pseudo) {
    if (w_eq(pseudo, "not")) return WE_LOGIC_NOT;
    if (w_eq(pseudo, "is") || w_eq(pseudo, "matches") || w_eq(pseudo, "-webkit-any")) return WE_LOGIC_IS;
    if (w_eq(pseudo, "where")) return WE_LOGIC_WHERE;
    if (w_eq(pseudo, "has")) return WE_LOGIC_HAS;
    return 0;
}

static int cssq_sel_depth;
static const char *cssq_sel_base;
static int cssq_in_has;

/* :has() arguments are relative: a leading >, + or ~ (else descendant) */
static int cssq_rel(const char *s, int *a, int end) {
    int i = *a, rel = 0;
    while (i < end && w_space(s[i])) i++;
    if (i < end && (s[i] == '>' || s[i] == '+' || s[i] == '~')) rel = s[i++];
    *a = i;
    return rel;
}

/* the arguments of a logical pseudo-class: the highest specificity of
 * the valid alternatives, -1 when :not has a bad one or none is valid */
static int cssq_list_spec(const char *s, int n, int kind) {
    int a = 0, i, level = 0, best = -1, quote = 0;
    we_rule_t *r;
    if (cssq_sel_depth >= WE_SEL_LOGIC_DEPTH) return -1;
    r = &g_cssq_rule[cssq_sel_depth];
    for (i = 0; i <= n; i++) {
        int ch = i < n ? s[i] : ',';
        if (quote) { if (ch == quote) quote = 0; continue; }
        if (ch == '\'' || ch == '"') quote = ch;
        else if (ch == '(' || ch == '[') level++;
        else if (ch == ')' || ch == ']') level--;
        else if (ch == ',' && !level) {
            int ok, sa = a;
            if (kind == WE_LOGIC_HAS) cssq_rel(s, &sa, i);
            cssq_sel_depth++;
            cssq_in_has += kind == WE_LOGIC_HAS;
            ok = parse_sel(s + sa, i - sa, r) == 0 && !r->pseudo_el;
            cssq_in_has -= kind == WE_LOGIC_HAS;
            cssq_sel_depth--;
            if (ok) { if (r->spec > best) best = r->spec; }
            else if (kind == WE_LOGIC_NOT || kind == WE_LOGIC_HAS) return -1;
            a = i + 1;
        }
    }
    return best;
}

/* parse_sel helper: ":name(" at s[*at] already read up to the name */
static int cssq_sel_logic(const char *s, int len, int *at, we_selector_part_t *part, const char *pseudo, uint16_t *spec) {
    int kind = cssq_logic_kind(pseudo), i = *at, begin, level = 1, slot, best;
    if (!kind) return 1;
    if (!cssq_sel_base || i == len || s[i] != '(' || (kind == WE_LOGIC_HAS && cssq_in_has)) return -1;
    begin = ++i;
    while (i < len && level) { if (s[i] == '(') level++; else if (s[i] == ')') level--; if (level) i++; }
    if (i == len) return -1;
    if (part->logic_kind[0] == 0) slot = 0;
    else if (part->logic_kind[1] == 0) slot = 1;
    else return -1;
    best = cssq_list_spec(s + begin, i - begin, kind);
    if (best < 0) return -1;
    part->logic_kind[slot] = (uint8_t)kind;
    part->logic_off[slot] = (uint16_t)(s + begin - cssq_sel_base);
    part->logic_len[slot] = (uint16_t)(i - begin);
    if (kind != WE_LOGIC_WHERE) *spec = (uint16_t)(*spec + best);
    *at = i + 1;
    return 0;
}

static int match_chain(const we_doc_t *d, int idx, const we_rule_t *r, int part, int *budget);
static int cssq_match_depth;

static int inside(const we_doc_t *d, int node, int root);

/* Lot 74: a :has() argument is anchored: the first compound of its chain
 * stands in the relation rel to the :has element (descendant, > child,
 * + next sibling, ~ later sibling), so no part matches outside it. */
static const we_rule_t *cssq_anchor_rule;
static int cssq_anchor, cssq_anchor_rel;
static int previous_element(const we_doc_t *d, int idx);
static int cssq_anchor_ok(const we_doc_t *d, int idx, const we_rule_t *r) {
    int e;
    if (r != cssq_anchor_rule) return 1;
    if (cssq_anchor_rel == '>') return d->nodes[idx].parent == cssq_anchor;
    if (cssq_anchor_rel == '+') return previous_element(d, idx) == cssq_anchor;
    if (cssq_anchor_rel == '~') {
        for (e = previous_element(d, idx); e > 0; e = previous_element(d, e)) if (e == cssq_anchor) return 1;
        return 0;
    }
    return idx != cssq_anchor && inside(d, idx, cssq_anchor);
}

/* :has(): an element after idx (in its subtree, or for + and ~ in the
 * subtree of its parent) that matches r anchored at idx */
static int cssq_has_match(const we_doc_t *d, int idx, const we_rule_t *r, int rel, int *budget) {
    const we_rule_t *save_rule = cssq_anchor_rule;
    int save = cssq_anchor, save_rel = cssq_anchor_rel, e, hit = 0;
    int scope = rel == '+' || rel == '~' ? d->nodes[idx].parent : idx;
    if (scope < 0) return 0;
    cssq_anchor_rule = r; cssq_anchor = idx; cssq_anchor_rel = rel;
    for (e = idx + 1; e < (int)d->nnodes && *budget > 0 && !hit; e++) {
        if (d->nodes[e].is_text || d->nodes[e].pseudo_kind) continue;
        if (!inside(d, e, scope)) continue;
        if (match_chain(d, e, r, r->ncomp - 1, budget)) hit = 1;
    }
    cssq_anchor_rule = save_rule; cssq_anchor = save; cssq_anchor_rel = save_rel;
    return hit;
}

static int cssq_match_list(const we_doc_t *d, int idx, int off, int n, int kind) {
    const char *s = d->css + off;
    int a = 0, i, level = 0, quote = 0, hit = 0;
    we_rule_t *r;
    if (cssq_match_depth >= WE_SEL_LOGIC_DEPTH) return 0;
    r = &g_cssq_rule[cssq_match_depth];
    for (i = 0; i <= n && !hit; i++) {
        int ch = i < n ? s[i] : ',';
        if (quote) { if (ch == quote) quote = 0; continue; }
        if (ch == '\'' || ch == '"') quote = ch;
        else if (ch == '(' || ch == '[') level++;
        else if (ch == ')' || ch == ']') level--;
        else if (ch == ',' && !level) {
            const char *base = cssq_sel_base;
            int ok, budget = WE_SELECTOR_WORK_MAX / 4, depth = cssq_sel_depth, sa = a, rel = 0;
            if (kind == WE_LOGIC_HAS) rel = cssq_rel(s, &sa, i);
            cssq_sel_base = d->css;
            cssq_match_depth++;
            cssq_sel_depth = cssq_match_depth;
            ok = parse_sel(s + sa, i - sa, r) == 0 && !r->pseudo_el;
            if (ok && r->ncomp)
                hit = kind == WE_LOGIC_HAS ? cssq_has_match(d, idx, r, rel, &budget) : match_chain(d, idx, r, r->ncomp - 1, &budget);
            cssq_match_depth--;
            cssq_sel_depth = depth;
            cssq_sel_base = base;
            a = i + 1;
        }
    }
    return hit;
}

static int cssq_match_logic(const we_doc_t *d, int idx, const we_selector_part_t *part) {
    int k;
    for (k = 0; k < 2; k++) {
        int hit;
        if (!part->logic_kind[k]) continue;
        hit = cssq_match_list(d, idx, part->logic_off[k], part->logic_len[k], part->logic_kind[k]);
        if (part->logic_kind[k] == WE_LOGIC_NOT ? hit : !hit) return 0;
    }
    return 1;
}

/* state pseudo-classes (flags in part->state) */
static int cssq_state_flag(const char *pseudo) {
    static const char *const never[] = { "visited", "hover", "active", 0 };
    int k;
    if (w_eq(pseudo, "link") || w_eq(pseudo, "any-link")) return WE_STATE_LINK;
    if (w_eq(pseudo, "checked")) return WE_STATE_CHECKED;
    if (w_eq(pseudo, "disabled")) return WE_STATE_DISABLED;
    if (w_eq(pseudo, "enabled")) return WE_STATE_ENABLED;
    if (w_eq(pseudo, "placeholder-shown")) return WE_STATE_PLACEHOLDER;
    if (w_eq(pseudo, "indeterminate")) return WE_STATE_INDETERMINATE;
    if (w_eq(pseudo, "open")) return WE_STATE_OPEN;
    if (w_eq(pseudo, "target")) return WE_STATE_TARGET;               /* lot 74 */
    if (w_eq(pseudo, "target-within")) return WE_STATE_TARGET_WITHIN;
    /* lot 74: the live form state */
    if (w_eq(pseudo, "focus") || w_eq(pseudo, "focus-visible")) return WE_STATE_FOCUS;
    if (w_eq(pseudo, "focus-within")) return WE_STATE_FOCUS_WITHIN;
    if (w_eq(pseudo, "required")) return WE_STATE_REQUIRED;
    if (w_eq(pseudo, "optional")) return WE_STATE_OPTIONAL;
    if (w_eq(pseudo, "read-only")) return WE_STATE_READ_ONLY;
    if (w_eq(pseudo, "read-write")) return WE_STATE_READ_WRITE;
    if (w_eq(pseudo, "invalid") || w_eq(pseudo, "user-invalid")) return WE_STATE_INVALID;
    if (w_eq(pseudo, "valid") || w_eq(pseudo, "user-valid")) return WE_STATE_VALID;
    for (k = 0; never[k]; k++) if (w_eq(pseudo, never[k])) return WE_STATE_NEVER;
    return 0;
}

static int cssq_form_element(const we_node_t *n) {
    return n->form_type || w_eq(n->tag, "button") || w_eq(n->tag, "select") || w_eq(n->tag, "textarea") ||
           w_eq(n->tag, "option") || w_eq(n->tag, "fieldset");
}

/* lot 74: the live form states (focus, validity, editability) */
static int cssq_match_live(const we_doc_t *d, const we_node_t *n, int state) {
    int idx = (int)(n - d->nodes), input = w_eq(n->tag, "input") || w_eq(n->tag, "select") || w_eq(n->tag, "textarea");
    int editable = finput_textlike(n) && !(n->form_flags2 & WE_FF2_READONLY) && !finput_disabled(d, idx);
    if ((state & WE_STATE_FOCUS) && !(n->form_flags2 & WE_FF2_FOCUS)) return 0;
    if (state & WE_STATE_FOCUS_WITHIN) {
        int f = d->forms ? finput_node(d, d->forms->focus) : -1;
        while (f >= 0 && f != idx) f = d->nodes[f].parent;
        if (f < 0) return 0;
    }
    if ((state & WE_STATE_REQUIRED) && !(input && (n->form_flags2 & WE_FF2_REQUIRED))) return 0;
    if ((state & WE_STATE_OPTIONAL) && !(input && !(n->form_flags2 & WE_FF2_REQUIRED))) return 0;
    if ((state & WE_STATE_READ_WRITE) && !editable) return 0;
    if ((state & WE_STATE_READ_ONLY) && editable) return 0;
    if ((state & WE_STATE_INVALID) && !finput_invalid(d, idx)) return 0;
    if ((state & WE_STATE_VALID) && !(input && n->form_ord && !finput_invalid(d, idx))) return 0;
    if (state & (WE_STATE_TARGET | WE_STATE_TARGET_WITHIN)) {
        int t = we_target_node(d);
        if ((state & WE_STATE_TARGET) && t != idx) return 0;
        while (t >= 0 && t != idx) t = d->nodes[t].parent;
        if (t < 0) return 0;
    }
    return 1;
}

static int cssq_match_state(const we_doc_t *d, const we_node_t *n, int state) {
    if ((state & WE_STATE_NEVER)) return 0;
    if ((state & WE_STATE_LINK) && !((w_eq(n->tag, "a") || w_eq(n->tag, "area")) && (n->attr_flags & 4))) return 0;
    if ((state & WE_STATE_CHECKED) && !(n->form_flags & (WE_FF_CHECKED | WE_FF_SELECTED))) return 0;
    if ((state & WE_STATE_DISABLED) && !(cssq_form_element(n) && (n->form_flags & WE_FF_DISABLED))) return 0;
    if ((state & WE_STATE_ENABLED) && !(cssq_form_element(n) && !(n->form_flags & WE_FF_DISABLED))) return 0;
    /* the placeholder is the grey text child forms_build made */
    if ((state & WE_STATE_PLACEHOLDER) && !(n->first >= 0 && (d->nodes[n->first].form_flags & WE_FF_PLACEHOLDER))) return 0;
    if ((state & WE_STATE_INDETERMINATE) && !(w_eq(n->tag, "progress") && !(n->form_flags & WE_FF_VALUE))) return 0;
    if ((state & WE_STATE_OPEN) && !(w_eq(n->tag, "details") && (n->form_flags & WE_FF_OPEN))) return 0;
    return state < WE_STATE_FOCUS || cssq_match_live(d, n, state);
}

/* ---------- custom properties ---------- */

static int cssq_pool_ref(we_doc_t *d, const char *s, int n, uint16_t *off) {
    if (s >= d->pool && s + n <= d->pool + d->pool_used) { *off = (uint16_t)(s - d->pool); return 0; }
    return pool_put(d, s, n, off);
}

static int cssq_var_find(const we_doc_t *d, const we_node_t *n, const char *name, int len) {
    int k;
    for (k = 0; k < n->nvars; k++)
        if (n->var_name_len[k] == len) {
            int j;
            const char *have = d->pool + n->var_name_off[k];
            for (j = 0; j < len && have[j] == name[j]; j++) {}
            if (j == len) return k;
        }
    return -1;
}

static const char *cssq_defining;
static int cssq_defining_len;
static int cssq_cycle;

/* copy v with every var() replaced; -1 when a variable is missing */
static int cssq_expand(const we_doc_t *d, const we_node_t *n, const char *v, int vl, char *out, int cap, int *used, int depth) {
    int i = 0;
    if (depth > 4) return -1;
    while (i < vl) {
        if (vl - i > 4 && w_ieq_n(v + i, vl - i, "var(") && (i == 0 || !w_namec(v[i - 1]))) {
            int a = i + 4, level = 1, end, na, nb, comma = -1, k;
            for (end = a; end < vl; end++) {
                if (v[end] == '(') level++;
                else if (v[end] == ')' && --level == 0) break;
                else if (v[end] == ',' && level == 1 && comma < 0) comma = end;
            }
            if (end == vl) return -1;
            na = a; nb = comma >= 0 ? comma : end;
            cssq_trim(v, &na, &nb);
            if (nb - na < 3 || v[na] != '-' || v[na + 1] != '-') return -1;
            if (cssq_defining && nb - na == cssq_defining_len) {
                int j;
                for (j = 0; j < cssq_defining_len && v[na + j] == cssq_defining[j]; j++) {}
                if (j == cssq_defining_len) { cssq_cycle = 1; return -1; }
            }
            k = cssq_var_find(d, n, v + na, nb - na);
            if (k >= 0) {
                int j;
                if (*used + n->var_len[k] > cap) return -1;
                for (j = 0; j < n->var_len[k]; j++) out[(*used)++] = d->pool[n->var_off[k] + j];
            } else if (comma >= 0) {
                int fa = comma + 1, fb = end;
                cssq_trim(v, &fa, &fb);
                if (cssq_expand(d, n, v + fa, fb - fa, out, cap, used, depth + 1)) return -1;
            } else return -1;
            i = end + 1;
        } else {
            if (*used >= cap) return -1;
            out[(*used)++] = v[i++];
        }
    }
    return 0;
}

static int cssq_has_var(const char *v, int vl) {
    int i;
    for (i = 0; i + 4 <= vl; i++) if (w_ieq_n(v + i, vl - i, "var(") && (i == 0 || !w_namec(v[i - 1]))) return 1;
    return 0;
}

static void cssq_var_drop(we_node_t *n, int k) {
    for (; k + 1 < n->nvars; k++) {
        n->var_name_off[k] = n->var_name_off[k + 1]; n->var_name_len[k] = n->var_name_len[k + 1];
        n->var_off[k] = n->var_off[k + 1]; n->var_len[k] = n->var_len[k + 1];
    }
    n->nvars--;
}

static char cssq_def_buf[WE_VAR_VALUE_MAX];

/* --name: value on element n (vars pass) */
static void cssq_set_var(we_doc_t *d, we_node_t *n, const char *p, int pl, const char *v, int vl) {
    int k = cssq_var_find(d, n, p, pl), a = 0, b = vl, used = 0, rc;
    uint16_t name_off, off;
    cssq_trim(v, &a, &b);
    v += a; vl = b - a;
    if (pl > 64) { diag(d, "css variable refused", -1); return; }
    if (cssq_is(v, vl, "initial") || cssq_is(v, vl, "unset")) { if (k >= 0) cssq_var_drop(n, k); return; }
    if (cssq_is(v, vl, "inherit")) {
        const we_node_t *parent = n->parent >= 0 ? &d->nodes[n->parent] : 0;
        int pk = parent ? cssq_var_find(d, parent, p, pl) : -1;
        if (k >= 0) cssq_var_drop(n, k);
        k = -1;
        if (pk < 0) return;
        v = d->pool + parent->var_off[pk]; vl = parent->var_len[pk];
    }
    if (cssq_has_var(v, vl)) {
        cssq_defining = p; cssq_defining_len = pl; cssq_cycle = 0;
        rc = cssq_expand(d, n, v, vl, cssq_def_buf, (int)sizeof(cssq_def_buf), &used, 0);
        cssq_defining = 0;
        if (rc) {
            diag(d, cssq_cycle ? "css variable cycle" : "css variable undefined", -1);
            if (k >= 0) cssq_var_drop(n, k);
            return;
        }
        if (pool_put(d, cssq_def_buf, used, &off)) { diag(d, "css variable refused", -1); return; }
        vl = used;
    } else if (cssq_pool_ref(d, v, vl, &off)) { diag(d, "css variable refused", -1); return; }
    if (k < 0) {
        if (n->nvars >= WE_VARS_MAX) { diag(d, "css variables full", -1); return; }
        if (cssq_pool_ref(d, p, pl, &name_off)) { diag(d, "css variable refused", -1); return; }
        k = n->nvars++;
        n->var_name_off[k] = name_off;
        n->var_name_len[k] = (uint8_t)pl;
    }
    n->var_off[k] = off;
    n->var_len[k] = (uint16_t)vl;
}

static void cssq_inherit_vars(const we_node_t *p, we_node_t *n) {
    int k;
    n->nvars = p ? p->nvars : 0;
    for (k = 0; k < n->nvars; k++) {
        n->var_name_off[k] = p->var_name_off[k]; n->var_name_len[k] = p->var_name_len[k];
        n->var_off[k] = p->var_off[k]; n->var_len[k] = p->var_len[k];
    }
}

static int vars_pass;   /* 1 while only custom properties apply */
static char cssq_use_buf[WE_VAR_VALUE_MAX];

/* apply_decl front: 1 when the declaration is done (custom property,
 * vars pass, or an undefined variable), else v / vl may be replaced */
static int cssq_decl_front(we_doc_t *d, we_node_t *n, const char *p, int pl, const char **v, int *vl) {
    int used = 0;
    if (pl > 2 && p[0] == '-' && p[1] == '-') {
        if (vars_pass) cssq_set_var(d, n, p, pl, *v, *vl);
        return 1;
    }
    if (vars_pass) return 1;
    if (!cssq_has_var(*v, *vl)) return 0;
    if (cssq_expand(d, n, *v, *vl, cssq_use_buf, (int)sizeof(cssq_use_buf), &used, 0)) {
        diag(d, "css variable undefined", -1);
        return 1;
    }
    d->perf.vars_used++;
    *v = cssq_use_buf; *vl = used;
    return 0;
}
