/* wf_cssvar.c - Lot 89: see wf_cssvar.h. */
#include "wf_cssvar.h"

typedef struct {
    char *out;
    int cap;
    int len;
    wf_cssvar_get_t get;
    const void *ctx;
} cv_t;

static int cv_ident(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
}
static int cv_sp(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
static char cv_low(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

static int cv_put(cv_t *c, char ch) {
    if (c->len >= c->cap - 1) return -1;
    c->out[c->len++] = ch;
    return 0;
}

/* "var(" at i, not the tail of a longer name (e.g. "somevar(") */
static int cv_at_var(const char *s, int i, int n) {
    if (i + 4 > n) return 0;
    if (cv_low(s[i]) != 'v' || cv_low(s[i + 1]) != 'a' || cv_low(s[i + 2]) != 'r' || s[i + 3] != '(') return 0;
    return i == 0 || !cv_ident(s[i - 1]);
}

/* One var() starting at s[i] ("var("). Parses the name and the fallback
 * span; *end gets the index after its ')'. Returns 0, or -1 malformed. */
static int cv_parse(const char *s, int i, int n, char *name, int *fb_a, int *fb_b, int *end) {
    int k = 0, depth = 0;
    i += 4;
    while (i < n && cv_sp(s[i])) i++;
    if (i + 2 > n || s[i] != '-' || s[i + 1] != '-') return -1;
    while (i < n && cv_ident(s[i])) {
        if (k >= WF_CSSVAR_NAME - 1) return -1;
        name[k++] = cv_low(s[i++]);
    }
    name[k] = 0;
    if (k < 3) return -1;   /* "--" alone is not a name */
    while (i < n && cv_sp(s[i])) i++;
    *fb_a = *fb_b = -1;
    if (i < n && s[i] == ',') {
        i++;
        *fb_a = i;
        while (i < n && (depth || s[i] != ')')) {
            if (s[i] == '(') depth++;
            else if (s[i] == ')') depth--;
            i++;
        }
        if (i >= n) return -1;
        *fb_b = i;
    }
    if (i >= n || s[i] != ')') return -1;
    *end = i + 1;
    return 0;
}

static int cv_run(cv_t *c, const char *s, int n, int depth);

/* the substitution of one var(): the property's value, else the fallback */
static int cv_one(cv_t *c, const char *name, const char *s, int fb_a, int fb_b, int depth) {
    const char *v = c->get ? c->get(c->ctx, name) : 0;
    if (depth >= WF_CSSVAR_DEPTH) return -1;
    if (v) {
        int n = 0;
        while (v[n]) n++;
        return cv_run(c, v, n, depth + 1);
    }
    if (fb_a < 0) return -1;
    /* the fallback without its outer spaces; it may itself hold var() */
    while (fb_a < fb_b && cv_sp(s[fb_a])) fb_a++;
    while (fb_b > fb_a && cv_sp(s[fb_b - 1])) fb_b--;
    return cv_run(c, s + fb_a, fb_b - fb_a, depth + 1);
}

static int cv_run(cv_t *c, const char *s, int n, int depth) {
    int i = 0, hits = 0;
    while (i < n) {
        if (cv_at_var(s, i, n)) {
            char name[WF_CSSVAR_NAME];
            int fa, fb, end, r;
            if (cv_parse(s, i, n, name, &fa, &fb, &end) != 0) return -1;
            r = cv_one(c, name, s, fa, fb, depth);
            if (r < 0) return -1;
            hits++;
            i = end;
            continue;
        }
        if (cv_put(c, s[i]) != 0) return -1;
        i++;
    }
    return hits;
}

int wf_cssvar_subst(const char *in, char *out, int cap, wf_cssvar_get_t get, const void *ctx) {
    cv_t c;
    int n = 0, r;
    if (!out || cap < 1) return -1;
    out[0] = 0;
    if (!in) return -1;
    while (in[n]) n++;
    c.out = out;
    c.cap = cap;
    c.len = 0;
    c.get = get;
    c.ctx = ctx;
    r = cv_run(&c, in, n, 0);
    if (r < 0) { out[0] = 0; return -1; }
    /* trim what the substitutions left at the ends */
    while (c.len > 0 && cv_sp(out[c.len - 1])) c.len--;
    out[c.len] = 0;
    if (r > 0) {
        int a = 0, k;
        while (a < c.len && cv_sp(out[a])) a++;
        if (a) { for (k = 0; a + k <= c.len; k++) out[k] = out[a + k]; }
    }
    return r > 0 ? 1 : 0;
}

int wf_cssvar_count(const char *in) {
    int n = 0, i = 0, hits = 0;
    if (!in) return 0;
    while (in[n]) n++;
    while (i < n) {
        if (cv_at_var(in, i, n)) {
            char name[WF_CSSVAR_NAME];
            int fa, fb, end;
            if (cv_parse(in, i, n, name, &fa, &fb, &end) != 0) return -1;
            hits++;
            i = end;
            continue;
        }
        i++;
    }
    return hits;
}
