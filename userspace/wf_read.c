/* wf_read.c - Lot 85. See wf_read.h. Freestanding, no heap. */
#include "wf_read.h"
#include "wf_score.h"

static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int sp(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }

static int eq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static int starts(const char *s, const char *p) {
    while (*p && *s == *p) { s++; p++; }
    return *p == 0;
}

static int tag_at(const char *in, int n, int i, char *name, int cap, int *close, int *end) {
    int k = 0, q = 0;
    if (i >= n || in[i] != '<') return 0;
    i++;
    if (i < n && (in[i] == '!' || in[i] == '?')) return 0;
    *close = (i < n && in[i] == '/');
    if (*close) i++;
    while (i < n && k < cap - 1) {
        char c = lc(in[i]);
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) break;
        name[k++] = c;
        i++;
    }
    name[k] = 0;
    if (!k) return 0;
    while (i < n && (q || in[i] != '>')) {
        if (in[i] == '"' || in[i] == '\'') q = (q == in[i]) ? 0 : (q ? q : in[i]);
        i++;
    }
    *end = i < n ? i + 1 : n;
    return 1;
}

static int attr_of(const char *in, int s, int e, const char *attr, char *out, int cap) {
    int i, al = 0, k;
    while (attr[al]) al++;
    out[0] = 0;
    for (i = s; i + al < e; i++) {
        if (i > s && !sp(in[i - 1])) continue;
        for (k = 0; k < al && lc(in[i + k]) == attr[k]; k++) {}
        if (k != al) continue;
        i += al;
        while (i < e && sp(in[i])) i++;
        if (i >= e || in[i] != '=') return 0;
        i++;
        while (i < e && sp(in[i])) i++;
        {
            char q = 0;
            int o = 0;
            if (i < e && (in[i] == '"' || in[i] == '\'')) q = in[i++];
            while (i < e && o < cap - 1) {
                if (q ? in[i] == q : (sp(in[i]) || in[i] == '>')) break;
                out[o++] = lc(in[i]);
                i++;
            }
            out[o] = 0;
            return o;
        }
    }
    return 0;
}

/* Matching close tag, or n when the element runs off the buffer. */
static int close_of(const char *in, int n, int from, const char *name) {
    int depth = 1, i = from;
    while (i < n) {
        char nm[16];
        int cl = 0, end = 0;
        if (in[i] != '<') { i++; continue; }
        if (!tag_at(in, n, i, nm, (int)sizeof(nm), &cl, &end)) { i++; continue; }
        if (eq(nm, name)) {
            if (cl) { if (--depth == 0) return i; }
            else if (in[end - 2] != '/') depth++;
        }
        i = end;
    }
    return n;
}

/* Letters, not tags. Letters inside <a> also count as link text. */
static void measure(const char *in, int lo, int hi, int *text, int *links) {
    int i = lo, in_a = 0, skip = 0;
    *text = 0;
    *links = 0;
    while (i < hi) {
        char nm[16];
        int cl = 0, end = 0;
        if (in[i] != '<') {
            unsigned char c = (unsigned char)in[i++];
            if (!skip && ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c >= 0x80u)) {
                (*text)++;
                if (in_a) (*links)++;
            }
            continue;
        }
        if (!tag_at(in, hi, i, nm, (int)sizeof(nm), &cl, &end)) { i++; continue; }
        if (eq(nm, "script") || eq(nm, "style") || eq(nm, "noscript")) {
            if (!cl) skip++;
            else if (skip) skip--;
        } else if (eq(nm, "a") && !skip) {
            if (!cl && in[end - 2] != '/') in_a = 1;
            else in_a = 0;
        }
        i = end;
    }
}

static int content_id(const char *id) {
    return eq(id, "content") || eq(id, "mw-content-text") || eq(id, "bodycontent") ||
           eq(id, "article") || eq(id, "main-content") || eq(id, "maincontent") ||
           starts(id, "mw-content-text");
}

static int kind_of(const char *name, const char *role, const char *id) {
    if (eq(name, "article")) return 1;
    if (eq(name, "main")) return 2;
    if (eq(role, "main")) return 3;
    if (content_id(id)) return 4;
    return 0;
}

int wf_read_root(const char *in, int n, int *lo, int *hi, wf_read_info_t *info) {
    int i = 0, best_lo = 0, best_hi = n, best_score = 0, best_kind = 0;
    int best_text = 0, best_link = 0, cands = 0;
    if (lo) *lo = 0;
    if (hi) *hi = n > 0 ? n : 0;
    if (info) {
        info->candidates = 0; info->chosen = 0; info->kind = 0;
        info->text_chars = 0; info->link_chars = 0; info->boilerplate = 0;
    }
    if (!in || n < 16) return 0;
    while (i < n && cands < 12) {
        char nm[16], role[24], id[48];
        int cl = 0, end = 0, kind, inner, stop, text, links, score;
        if (in[i] != '<') { i++; continue; }
        if (!tag_at(in, n, i, nm, (int)sizeof(nm), &cl, &end) || cl) { i++; continue; }
        role[0] = id[0] = 0;
        attr_of(in, i + 1, end - 1, "role", role, (int)sizeof(role));
        attr_of(in, i + 1, end - 1, "id", id, (int)sizeof(id));
        kind = kind_of(nm, role, id);
        if (!kind) { i = end; continue; }
        cands++;
        stop = close_of(in, n, end, nm);
        inner = end;
        measure(in, inner, stop, &text, &links);
        /* A link list is navigation even when it is marked main. The
         * article has more letters outside links than inside them. */
        score = text - links * 2;
        if (kind == 1) score += 40;
        if (kind == 2) score += 30;
        if (kind == 4) score += 20;
        if (text >= 16 && text > links && score > best_score) {
            best_score = score;
            best_lo = inner;
            best_hi = stop;
            best_kind = kind;
            best_text = text;
            best_link = links;
        }
        i = end;
    }
    if (info) {
        info->candidates = cands;
        info->chosen = best_kind ? 1 : 0;
        info->kind = best_kind;
        info->text_chars = best_text;
        info->link_chars = best_link;
    }
    if (!best_kind) {
        int slo = 0, shi = 0, text = 0, links = 0;
        /* Lot 85: news and docs pages that use a class instead of
         * <article> or <main>. A page with no such block stays whole. */
        if (wf_score_pick(in, n, &slo, &shi, &text, &links)) {
            best_lo = slo;
            best_hi = shi;
            best_kind = 5;
            best_text = text;
            best_link = links;
        }
    }
    if (!best_kind) return 0;
    if (lo) *lo = best_lo;
    if (hi) *hi = best_hi;
    if (info) {
        info->chosen = 1;
        info->kind = best_kind;
        info->text_chars = best_text;
        info->link_chars = best_link;
    }
    return 1;
}

/* A class token, or the start of a hyphenated token (vector-toc matches
 * vector-toc-contents). Short tokens must match a whole token so a word
 * like "reference-implementation" is not treated as a reference list. */
static int strchr_hyphen(const char *s) {
    while (*s) { if (*s == '-') return 1; s++; }
    return 0;
}

static int token(const char *s, const char *want) {
    int i = 0, wl = 0;
    if (!s || !s[0]) return 0;
    while (want[wl]) wl++;
    while (s[i]) {
        int k = 0;
        while (s[i] == ' ') i++;
        while (s[i] && s[i] != ' ' && k < wl && s[i] == want[k]) { i++; k++; }
        /* A hyphen continues the token only when the wanted name already
         * has one (vector-toc matches vector-toc-contents). A bare word
         * must be the whole token, so "reference" does not swallow
         * "reference-implementation". */
        if (k == wl && (s[i] == 0 || s[i] == ' ' || (s[i] == '-' && strchr_hyphen(want)))) return 1;
        while (s[i] && s[i] != ' ') i++;
    }
    return 0;
}

int wf_read_chrome(const char *id, const char *cls) {
    static const char *const ids[] = {
        "toc", "siteSub", "contentSub", "jump-to-nav", "mw-navigation",
        "mw-panel", "mw-head", "footer", "mw-data-after-content", "catlinks",
        "cookie", "cookie-banner", "onetrust", 0
    };
    static const char *const classes[] = {
        "toc", "mw-jump-link", "vector-toc", "vector-menu", "vector-sidebar",
        "navbox", "vertical-navbox", "sistersitebox", "catlinks", "mw-editsection",
        "mw-indicators", "noprint", "nomobile", "hatnote", "shortdescription",
        "metadata", "side-box", "mw-empty-elt", "reference", "reflist",
        "cookie", "cookie-banner", "cookie-notice", "consent", "gdpr",
        "interlanguage", "language-list", "skip-link", "visually-hidden", "sr-only",
        "nopopups", "mw-hidden-catlinks", 0
    };
    int i;
    if (id && content_id(id)) return 0;
    for (i = 0; ids[i]; i++) if (id && eq(id, ids[i])) return 1;
    for (i = 0; classes[i]; i++) if (token(cls, classes[i])) return 1;
    /* Lot 85: news and docs chrome (sidebar, breadcrumb, comments). */
    if (wf_score_is_chrome(id, cls)) return 1;
    return 0;
}

static int heading_level(const char *name) {
    if (name[0] == 'h' && name[1] >= '1' && name[1] <= '3' && name[2] == 0) return name[1] - '0';
    return 0;
}

int wf_read_outline(const char *in, int lo, int hi, char *out, int cap, int max_heads) {
    int i, n = 0, heads = 0;
    if (!out || cap < 2) return 0;
    out[0] = 0;
    if (!in || lo < 0 || hi < lo || max_heads < 1) return 0;
    if (hi > lo + 512 * 1024) hi = lo + 512 * 1024;
    i = lo;
    while (i < hi && heads < max_heads && n < cap - 8) {
        char nm[16];
        int cl = 0, end = 0, level, j, wrote = 0;
        if (in[i] != '<') { i++; continue; }
        if (!tag_at(in, hi, i, nm, (int)sizeof(nm), &cl, &end) || cl) { i++; continue; }
        level = heading_level(nm);
        if (!level) { i = end; continue; }
        out[n++] = (char)('0' + level);
        out[n++] = ' ';
        for (j = end; j < hi && in[j] != '<' && n < cap - 2; j++) {
            unsigned char c = (unsigned char)in[j];
            if (c < 32) c = ' ';
            if (c == ' ' && wrote && out[n - 1] == ' ') continue;
            out[n++] = (char)c;
            wrote = 1;
        }
        while (n > 0 && out[n - 1] == ' ') n--;
        out[n++] = '\n';
        out[n] = 0;
        heads++;
        i = end;
    }
    return n;
}
