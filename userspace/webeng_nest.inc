/* Lot 74: CSS Nesting (subset). A style block may contain nested rules.
 * Inside a declaration body, a nested selector starts a nested block; the
 * parent selector is prefixed (an initial '&' is replaced by the parent,
 * else the parent is prepended with a descendant combinator). Nesting
 * deeper than WE_NEST_DEPTH is refused. The expander rewrites d->css in
 * place into a flat list of rules before parse_css_range. */
#define WE_NEST_DEPTH 4
#define WE_NEST_OUT (WE_CSS_MAX)

static int nest_is_sel_start(const char *s, int i, int n) {
    char c;
    while (i < n && w_space(s[i])) i++;
    if (i >= n) return 0;
    c = s[i];
    /* a property is ident ':' value ';' — a nested rule has '{' before ';' */
    if (c == '&' || c == '.' || c == '#' || c == '*' || c == '[' || c == '>' || c == '+' || c == '~') {
        int j = i;
        while (j < n && s[j] != '{' && s[j] != ';' && s[j] != '}') j++;
        return j < n && s[j] == '{';
    }
    if (c == ':') {
        /* :hover { } is a nested rule; : in a property is not */
        int j = i + 1;
        if (j < n && (w_namec(s[j]) || s[j] == ':')) {
            while (j < n && s[j] != '{' && s[j] != ';' && s[j] != '}') j++;
            return j < n && s[j] == '{';
        }
        return 0;
    }
    if (w_namec(c)) {
        int j = i;
        while (j < n && s[j] != '{' && s[j] != ';' && s[j] != '}') j++;
        return j < n && s[j] == '{';
    }
    return 0;
}

static int nest_compose(char *out, int omax, int *o, const char *parent, int plen, const char *sel, int slen) {
    int i = 0, used = 0;
    while (i < slen && w_space(sel[i])) i++;
    while (i < slen) {
        int a = i, amp = 0, k, quote = 0, brackets = 0, parens = 0;
        while (i < slen) {
            char ch = sel[i];
            if (quote) { if (ch == quote) quote = 0; i++; continue; }
            if (ch == 39 || ch == 34) { quote = ch; i++; continue; }
            if (ch == '[') brackets++;
            else if (ch == ']') brackets--;
            else if (ch == '(') parens++;
            else if (ch == ')') parens--;
            else if (ch == ',' && !brackets && !parens) break;
            i++;
        }
        /* one selector in the list */
        k = a;
        while (k < i && w_space(sel[k])) k++;
        if (k < i && sel[k] == '&') { amp = 1; k++; /* keep a following space as a combinator */ }
        if (*o && *o + 1 < omax) out[(*o)++] = ',';
        if (amp) {
            if (*o + plen + (i - k) >= omax) return -1;
            for (a = 0; a < plen; a++) out[(*o)++] = parent[a];
            while (k < i) out[(*o)++] = sel[k++];
        } else {
            if (*o + plen + 1 + (i - k) >= omax) return -1;
            for (a = 0; a < plen; a++) out[(*o)++] = parent[a];
            out[(*o)++] = ' ';
            while (k < i) out[(*o)++] = sel[k++];
        }
        used = 1;
        if (i < slen) i++;
    }
    return used ? 0 : -1;
}

/* expand nested rules from [from,to) under parent selector into out */
static int nest_expand(const char *s, int from, int to, const char *parent, int plen, int depth,
                       char *out, int omax, int *o, we_doc_t *d) {
    int i = from;
    while (i < to) {
        int sel_a, sel_b, body_a, body_b, level, j;
        while (i < to && w_space(s[i])) i++;
        if (i >= to) break;
        if (s[i] == '@') {
            /* keep at-rules flat: copy through their block or statement */
            int start = i, body;
            i++; while (i < to && w_namec(s[i])) i++;
            body = decl_end(s, to, i, '{');
            { int semi = decl_end(s, to, i, ';'); if (semi < body) { i = semi + 1; goto copy_at; } }
            if (body >= to) { i = to; goto copy_at; }
            level = 0;
            for (j = body; j < to; j++) {
                if (s[j] == '"' || s[j] == '\'') { char q = s[j++]; while (j < to && s[j] != q) j++; continue; }
                if (s[j] == '{') level++;
                else if (s[j] == '}' && --level == 0) { j++; break; }
            }
            i = j;
        copy_at:
            if (*o + (i - start) >= omax) return -1;
            while (start < i) out[(*o)++] = s[start++];
            continue;
        }
        if (!nest_is_sel_start(s, i, to)) {
            /* plain declarations until the next nested rule or end */
            int start = i;
            while (i < to) {
                if (s[i] == '"' || s[i] == '\'') { char q = s[i++]; while (i < to && s[i] != q) i++; if (i < to) i++; continue; }
                if (s[i] == ';') { i++; continue; }
                if (nest_is_sel_start(s, i, to)) break;
                if (s[i] == '}') break;
                i++;
            }
            /* emit parent { decls } when we have a parent and decls */
            if (plen && i > start) {
                int k;
                while (start < i && w_space(s[start])) start++;
                while (i > start && w_space(s[i - 1])) i--;
                if (i > start) {
                    if (*o + plen + 3 + (i - start) >= omax) return -1;
                    for (k = 0; k < plen; k++) out[(*o)++] = parent[k];
                    out[(*o)++] = '{';
                    while (start < i) out[(*o)++] = s[start++];
                    out[(*o)++] = '}';
                }
            } else {
                while (start < i) {
                    if (*o + 1 >= omax) return -1;
                    out[(*o)++] = s[start++];
                }
            }
            continue;
        }
        /* nested rule */
        sel_a = i;
        i = decl_end(s, to, i, '{');
        sel_b = i;
        if (i >= to) break;
        body_a = ++i;
        level = 1;
        for (body_b = i; body_b < to; body_b++) {
            if (s[body_b] == '"' || s[body_b] == '\'') { char q = s[body_b++]; while (body_b < to && s[body_b] != q) body_b++; continue; }
            if (s[body_b] == '{') level++;
            else if (s[body_b] == '}' && --level == 0) break;
        }
        i = body_b + 1;
        if (depth >= WE_NEST_DEPTH) { diag(d, "css nest depth refused", sel_a); continue; }
        {
            char composed[256];
            int clen = 0;
            if (nest_compose(composed, (int)sizeof(composed), &clen, parent, plen, s + sel_a, sel_b - sel_a) != 0) {
                diag(d, "css nest selector refused", sel_a);
                continue;
            }
            if (nest_expand(s, body_a, body_b, composed, clen, depth + 1, out, omax, o, d) != 0) return -1;
        }
    }
    return 0;
}

static int nest_needed(const char *s, int n) {
    int i, level = 0;
    for (i = 0; i < n; i++) {
        if (s[i] == '"' || s[i] == 39) { char q = s[i++]; while (i < n && s[i] != q) i++; continue; }
        if (s[i] == '{') {
            level++;
            if (level >= 2) return 1;   /* a block inside a block */
        } else if (s[i] == '}') { if (level > 0) level--; }
        else if (s[i] == '&' && level >= 1) return 1;
    }
    return 0;
}

static void nest_rewrite(we_doc_t *d) {
    static char out[WE_NEST_OUT];
    int o = 0;
    if (!d->css_len || !nest_needed(d->css, (int)d->css_len)) return;
    if (nest_expand(d->css, 0, (int)d->css_len, "", 0, 0, out, WE_NEST_OUT, &o, d) != 0) {
        diag(d, "css nest budget", 0);
        return;
    }
    if (o < 1) return;
    if (o >= WE_CSS_MAX) { diag(d, "css source exceeds limit", 0); o = WE_CSS_MAX - 1; }
    { int k; for (k = 0; k < o; k++) d->css[k] = out[k]; d->css[o] = 0; d->css_len = (uint16_t)o; }
}
