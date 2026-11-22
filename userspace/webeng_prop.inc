/* Lot 118: registered custom properties, @property (CSS Properties and
 * Values API 1, 3), included by webeng.c only, the single engine.
 *
 * "@property --name { syntax: '<length>'; inherits: false;
 * initial-value: 0px }" registers --name (the last rule of a name wins):
 *   - syntax is '*' or up to WE_PSYN_COMPONENTS components joined by |
 *     (the first that matches wins, 5.1): a type among <length>,
 *     <number>, <integer>, <percentage>, <length-percentage>, <color>,
 *     <angle>, <time>, <custom-ident>, or an identifier (not a CSS-wide
 *     keyword), each with an optional multiplier: + a space-separated
 *     list, # a comma-separated list (lists are computed item by item and
 *     written back with one space or ", " up to WE_PSYN_LIST_MAX
 *     characters). Other types and forms drop the rule with a diagnostic,
 *     as an invalid rule;
 *   - inherits is true or false; a non-inherited property starts every
 *     element at its initial value;
 *   - initial-value is required unless the syntax is '*', and must parse
 *     with the syntax and be computationally independent (px, numbers,
 *     angles, colors; font-relative and viewport units are refused, 3.1);
 *   - a value of the property that does not parse with the syntax is
 *     invalid at computed-value time: the property is unset (inherited
 *     or initial);
 *   - the computed value: a <length> is px (em, rem, the other units and
 *     math resolved at the element that declares it, so its children
 *     inherit the px), a <number> a number, an <integer> the nearest
 *     integer, an <angle> deg, a <time> s; <percentage>, <color>,
 *     identifiers and '*' keep their text;
 *   - var() of a registered property that has no value uses the initial
 *     value (its fallback is not used).
 * Values are written in thousandths at most ("12.5px"); the consumers of
 * the engine read them as before. */

enum { WE_PSYN_ANY = 1, WE_PSYN_LENGTH, WE_PSYN_NUMBER, WE_PSYN_INTEGER, WE_PSYN_PERCENT,
       WE_PSYN_LENGTH_PERCENT, WE_PSYN_COLOR, WE_PSYN_ANGLE, WE_PSYN_TIME, WE_PSYN_CUSTOM_IDENT,
       WE_PSYN_IDENT };
#define WE_PSYN_COMPONENTS 8
#define WE_PSYN_LIST_MAX 160

/* the CSS-wide keywords and default: never an identifier (CSS Values 4
 * 6.2) */
static int prop_reserved(const char *v, int n) {
    static const char *const words[] = { "initial", "inherit", "unset", "revert", "revert-layer", "default", 0 };
    int k;
    for (k = 0; words[k]; k++) if (value_is(v, n, words[k])) return 1;
    return 0;
}

/* an identifier: a letter, '_' or '-' then name characters, not "--" or
 * "-" followed by a digit */
static int prop_ident(const char *v, int n) {
    int k;
    if (n <= 0 || (v[0] >= '0' && v[0] <= '9')) return 0;
    if (v[0] == '-' && (n == 1 || v[1] == '-' || (v[1] >= '0' && v[1] <= '9'))) return 0;
    for (k = 0; k < n; k++) if (!w_namec(v[k])) return 0;
    return 1;
}

/* one syntax component v[0..n): its type (WE_PSYN_IDENT for an
 * identifier) and multiplier (0, '+' or '#'); 0 when it is not one */
static int prop_component(const char *v, int n, int *mult) {
    static const char *const names[] = { "length", "number", "integer", "percentage", "length-percentage",
                                         "color", "angle", "time", "custom-ident", 0 };
    int k;
    *mult = 0;
    if (n > 1 && (v[n - 1] == '+' || v[n - 1] == '#')) { *mult = v[n - 1]; n--; }
    if (n > 2 && v[0] == '<' && v[n - 1] == '>') {
        for (k = 0; names[k]; k++)
            if (n - 2 == w_len(names[k]) && w_ieq_n(v + 1, n - 2, names[k])) return WE_PSYN_LENGTH + k;
        return 0;
    }
    return prop_ident(v, n) && !prop_reserved(v, n) ? WE_PSYN_IDENT : 0;
}

/* the next component of the syntax text s[*at..n): its span; -1 at the
 * end (components are separated by | and spaces around it) */
static int prop_next(const char *s, int n, int *at, int *a, int *b) {
    int i = *at;
    if (i > n) return -1;
    *a = i;
    while (i < n && s[i] != '|') i++;
    *b = i;
    *at = i + 1;
    cssq_trim(s, a, b);
    return 0;
}

/* the syntax descriptor's value: a quoted string. WE_PSYN_ANY for '*', 0
 * when it is refused, else -1 (components, checked); *a..*b the text */
static int prop_syntax(const char *v, int n, int *a, int *b) {
    int at, ca, cb, count = 0, mult, len;
    *a = 1; *b = n - 1;
    if (n < 3 || (v[0] != '"' && v[0] != '\'') || v[n - 1] != v[0]) return 0;
    cssq_trim(v, a, b);
    len = *b;
    if (*b - *a == 1 && v[*a] == '*') return WE_PSYN_ANY;
    at = *a;
    while (prop_next(v, len, &at, &ca, &cb) == 0) {
        if (++count > WE_PSYN_COMPONENTS || !prop_component(v + ca, cb - ca, &mult)) return 0;
    }
    return *b - *a > 255 ? 0 : -1;
}

/* the registration of a custom property name, -1 when none */
static int prop_find(const we_doc_t *d, const char *name, int len) {
    int k;
    for (k = 0; k < d->nprops; k++) {
        const we_prop_t *r = &d->props[k];
        int j;
        if (r->name_len != len) continue;
        for (j = 0; j < len && d->pool[r->name_off + j] == name[j]; j++) {}
        if (j == len) return k;
    }
    return -1;
}

/* a signed decimal (whole part at most 1e6, three decimals kept) in
 * thousandths; *point when it had a decimal point */
static int prop_decimal(const char *v, int n, int *milli, int *point) {
    int i = 0, whole = 0, frac = 0, scale = 1000, digits = 0, neg = 0;
    *point = 0;
    if (i < n && (v[i] == '-' || v[i] == '+')) { neg = v[i] == '-'; i++; }
    while (i < n && v[i] >= '0' && v[i] <= '9') {
        whole = whole * 10 + (v[i] - '0'); i++; digits++;
        if (whole > 1000000) return -1;
    }
    if (i < n && v[i] == '.') {
        int fd = 0;
        *point = 1; i++;
        while (i < n && v[i] >= '0' && v[i] <= '9') {
            if (scale > 1) { scale /= 10; frac += (v[i] - '0') * scale; }
            i++; fd++;
        }
        if (!fd) return -1;
        digits += fd;
    }
    if (!digits || i != n) return -1;
    *milli = neg ? -(whole * 1000 + frac) : whole * 1000 + frac;
    return 0;
}

/* thousandths as text: "-12.5" then unit; the length written */
static int prop_format(char *out, int milli, const char *unit) {
    char digits[12];
    int n = 0, whole, frac, k;
    if (milli < 0) { out[n++] = '-'; milli = -milli; }
    whole = milli / 1000; frac = milli % 1000;
    w_itoa(whole, digits);
    for (k = 0; digits[k]; k++) out[n++] = digits[k];
    if (frac) {
        int scale = 100;
        out[n++] = '.';
        while (frac) { out[n++] = (char)('0' + frac / scale); frac %= scale; scale /= 10; }
    }
    for (k = 0; unit[k]; k++) out[n++] = unit[k];
    out[n] = 0;
    return n;
}

/* a <length>: thousandths of a px; -1 when it is not one (a percentage
 * expression is not) */
static int prop_length(const char *v, int n, int *milli) {
    int h, point;
    if (n == 1 && v[0] == '0') { *milli = 0; return 0; }
    if (n > 2 && value_is(v + n - 2, 2, "px")) return prop_decimal(v, n - 2, milli, &point);
    if (unit_len100(v, n, &h) == 0) { *milli = h * 10; return 0; }
    if (math_function(v, n)) {
        we_length_t e;
        if (parse_math(v, n, &e) || e.percent || math_len_milli(&e, 0, milli)) return -1;
        return 0;
    }
    return -1;
}

/* v against the syntax: 0 when it matches, with its computed text in out
 * (*outl, 0 when the text stays); independent refuses the font-relative
 * and viewport units (an initial value) */
static int prop_value(int syntax, const char *v, int n, int independent, char *out, int *outl) {
    int milli, point, pct, rc = -1;
    uint32_t col;
    *outl = 0;
    if (n <= 0) return -1;
    u_rel_used = 0;
    switch (syntax) {
    case WE_PSYN_LENGTH:
        if (prop_length(v, n, &milli)) return -1;
        *outl = prop_format(out, milli, "px"); rc = 0;
        break;
    case WE_PSYN_LENGTH_PERCENT:
        if (v[n - 1] == '%' && prop_decimal(v, n - 1, &milli, &point) == 0) return 0;
        if (math_function(v, n)) {
            we_length_t e;
            if (parse_math(v, n, &e)) return -1;
            if (e.percent) { rc = 0; break; }
        }
        if (prop_length(v, n, &milli)) return -1;
        *outl = prop_format(out, milli, "px"); rc = 0;
        break;
    case WE_PSYN_PERCENT:
        return v[n - 1] == '%' && prop_decimal(v, n - 1, &milli, &point) == 0 ? 0 : -1;
    case WE_PSYN_NUMBER:
        if (prop_decimal(v, n, &milli, &point) && (!math_function(v, n) || math_number_milli(v, n, &milli))) return -1;
        *outl = prop_format(out, milli, ""); rc = 0;
        break;
    case WE_PSYN_INTEGER:
        if (prop_decimal(v, n, &milli, &point) == 0) {
            if (point) return -1;
            pct = milli / 1000;
        } else if (math_integer(v, n, -1000000, 1000000, &pct)) return -1;
        *outl = prop_format(out, pct * 1000, ""); rc = 0;
        break;
    case WE_PSYN_COLOR:
        return parse_color(v, n, &col) ? -1 : 0;
    case WE_PSYN_ANGLE:
        if (tf_angle_milli(v, n, &milli)) return -1;
        *outl = prop_format(out, milli, "deg"); rc = 0;
        break;
    case WE_PSYN_TIME:
        /* s or ms, written in seconds (thousandths kept) */
        if (n > 2 && w_ieq_n(v + n - 2, 2, "ms")) {
            if (prop_decimal(v, n - 2, &milli, &point) || milli > 1000000000 || milli < -1000000000) return -1;
            milli /= 1000;
        } else if (n > 1 && w_low(v[n - 1]) == 's') {
            if (prop_decimal(v, n - 1, &milli, &point)) return -1;
        } else return -1;
        *outl = prop_format(out, milli, "s"); rc = 0;
        break;
    case WE_PSYN_CUSTOM_IDENT:
        return prop_ident(v, n) && !prop_reserved(v, n) ? 0 : -1;
    }
    if (rc == 0 && independent && u_rel_used) return -1;
    return rc;
}

static char prop_item_buf[64];

/* an identifier component matches its exact text (case-sensitive) */
static int prop_same(const char *v, int n, const char *s, int sn) {
    int k;
    if (n != sn) return 0;
    for (k = 0; k < n && v[k] == s[k]; k++) {}
    return k == n;
}

/* v against the registration's syntax: 0 when one of its components
 * matches, with the computed text in out (*outl, 0 when the text stays) */
static int prop_match(const we_doc_t *d, const we_prop_t *r, const char *v, int n, int independent, char *out,
                      int *outl) {
    const char *syn = d->pool + r->syn_off;
    int at = 0, ca, cb, len = r->syn_len;
    *outl = 0;
    if (r->syntax == WE_PSYN_ANY) return 0;
    while (prop_next(syn, len, &at, &ca, &cb) == 0) {
        int mult, type = prop_component(syn + ca, cb - ca, &mult), i = 0, used = 0, ok = 1;
        int il = cb - ca - (mult ? 1 : 0);
        if (!mult) {
            if (type == WE_PSYN_IDENT) { if (prop_same(v, n, syn + ca, il)) return 0; continue; }
            if (prop_value(type, v, n, independent, out, outl) == 0) return 0;
            continue;
        }
        /* a list: items split at the top level by spaces (+) or commas (#) */
        while (i < n && ok) {
            int a, b, depth = 0, itl;
            while (i < n && w_space(v[i])) i++;
            a = i;
            while (i < n && (depth || (mult == '+' ? !w_space(v[i]) : v[i] != ','))) {
                if (v[i] == '(') depth++;
                else if (v[i] == ')' && depth) depth--;
                i++;
            }
            b = i;
            cssq_trim(v, &a, &b);
            if (mult == '#') {
                if (i < n) i++;                     /* the comma */
                if (a == b || (i == n && v[n - 1] == ',')) { ok = 0; break; }
            }
            if (a == b) break;
            if (type == WE_PSYN_IDENT) {
                if (!prop_same(v + a, b - a, syn + ca, il)) { ok = 0; break; }
                itl = 0;
            } else if (prop_value(type, v + a, b - a, independent, prop_item_buf, &itl)) { ok = 0; break; }
            if (!itl) { w_cpy(prop_item_buf, sizeof(prop_item_buf), v + a, b - a > 63 ? 63 : b - a); itl = b - a; }
            if (itl > 63 || used + itl + 2 > WE_PSYN_LIST_MAX) { ok = 0; break; }
            if (used) { out[used++] = mult == '+' ? ' ' : ','; if (mult == '#') out[used++] = ' '; }
            w_cpy(out + used, itl + 1, prop_item_buf, itl);
            used += itl;
        }
        if (ok && used) { out[used] = 0; *outl = used; return 0; }
        *outl = 0;
    }
    return -1;
}

/* the declarations of an @property block: name, value spans */
static int prop_descriptor(const char *s, int a, int b, const char *name, int *va, int *vb) {
    int i = a;
    while (i < b) {
        int na, nb, colon, end = i, quote = 0;
        while (end < b && (quote || s[end] != ';')) {
            if (quote) { if (s[end] == quote) quote = 0; }
            else if (s[end] == '"' || s[end] == '\'') quote = s[end];
            end++;
        }
        for (colon = i; colon < end && s[colon] != ':'; colon++) {}
        na = i; nb = colon;
        cssq_trim(s, &na, &nb);
        if (colon < end && cssq_is(s + na, nb - na, name)) {
            *va = colon + 1; *vb = end;
            cssq_trim(s, va, vb);
            return 0;
        }
        i = end + 1;
    }
    return -1;
}

static char prop_buf[WE_PSYN_LIST_MAX + 4];

/* @property <prelude> { <body> } */
static void prop_rule(we_doc_t *d, const char *pre, int pl, const char *body, int bl) {
    int a = 0, b = pl, sa, sb, ia, ib, va, vb, syntax, inherits, k, outl, ta, tb;
    we_prop_t r;
    cssq_trim(pre, &a, &b);
    w_zero(&r, sizeof(r));
    if (b - a < 3 || b - a > 64 || pre[a] != '-' || pre[a + 1] != '-') { diag(d, "css property rule refused", -1); return; }
    for (k = a; k < b; k++) if (!w_namec(pre[k])) { diag(d, "css property rule refused", -1); return; }
    if (prop_descriptor(body, 0, bl, "syntax", &sa, &sb) || !(syntax = prop_syntax(body + sa, sb - sa, &ta, &tb)) ||
        prop_descriptor(body, 0, bl, "inherits", &ia, &ib) ||
        !(cssq_is(body + ia, ib - ia, "true") || cssq_is(body + ia, ib - ia, "false"))) {
        diag(d, "css property rule refused", -1);
        return;
    }
    inherits = cssq_is(body + ia, ib - ia, "true");
    if (syntax < 0) {
        if (pool_put(d, body + sa + ta, tb - ta, &r.syn_off)) { diag(d, "css property rule refused", -1); return; }
        r.syn_len = (uint8_t)(tb - ta);
        syntax = 0;
    }
    r.syntax = (uint8_t)syntax;
    if (prop_descriptor(body, 0, bl, "initial-value", &va, &vb) == 0 && vb > va) {
        uint16_t off;
        if (prop_match(d, &r, body + va, vb - va, 1, prop_buf, &outl) ||
            (outl ? pool_put(d, prop_buf, outl, &off) : pool_put(d, body + va, vb - va, &off))) {
            diag(d, "css property rule refused", -1);
            return;
        }
        r.init_off = off; r.init_len = (uint16_t)(outl ? outl : vb - va); r.has_init = 1;
    } else if (syntax != WE_PSYN_ANY) { diag(d, "css property rule refused", -1); return; }
    k = prop_find(d, pre + a, b - a);
    if (k < 0) {
        if (d->nprops >= WE_PROPS_MAX) { diag(d, "css property table full", -1); return; }
        if (pool_put(d, pre + a, b - a, &r.name_off)) { diag(d, "css property rule refused", -1); return; }
        k = d->nprops++;
    } else r.name_off = d->props[k].name_off;
    r.name_len = (uint8_t)(b - a); r.inherits = (uint8_t)inherits;
    d->props[k] = r;
}

/* a registered property's value on element n (vars pass): 0 when it
 * stays, 1 when it is invalid at computed-value time (then unset) */
static int prop_check(const we_doc_t *d, const char *name, int len, const char *v, int vl) {
    int k = prop_find(d, name, len), outl;
    if (k < 0) return 0;
    return prop_match(d, &d->props[k], v, vl, 0, prop_buf, &outl) ? 1 : 0;
}

/* after the font pass: the computed values of the registered properties
 * n declares (inherited ones are already computed) */
static void prop_compute(we_doc_t *d, we_node_t *n) {
    const we_node_t *p = n->parent >= 0 ? &d->nodes[n->parent] : 0;
    int k;
    if (!d->nprops) return;
    for (k = 0; k < n->nvars; k++) {
        int r = prop_find(d, d->pool + n->var_name_off[k], n->var_name_len[k]), outl, pk;
        uint16_t off;
        if (r < 0) continue;
        pk = p ? cssq_var_find(d, p, d->pool + n->var_name_off[k], n->var_name_len[k]) : -1;
        if (pk >= 0 && p->var_off[pk] == n->var_off[k] && p->var_len[pk] == n->var_len[k]) continue;
        if (prop_match(d, &d->props[r], d->pool + n->var_off[k], n->var_len[k], 0, prop_buf, &outl) || !outl) continue;
        if (pool_put(d, prop_buf, outl, &off)) { diag(d, "css variable refused", -1); continue; }
        n->var_off[k] = off; n->var_len[k] = (uint16_t)outl;
    }
}
