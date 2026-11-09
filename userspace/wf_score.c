/* wf_score.c - Lot 85. See wf_score.h. Freestanding, no heap. */
#include "wf_score.h"

#define WF_SCORE_STACK 16
#define WF_SCORE_MIN_TEXT 24
#define WF_SCORE_CLASS_TEXT 16

static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int spc(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }

static int eq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static int has_hyphen(const char *s) {
    while (*s) { if (*s == '-') return 1; s++; }
    return 0;
}

/* Whole token, or a hyphenated continuation when the wanted name itself
 * contains a hyphen (article-body matches article-body-wrap). A bare
 * word does not swallow a longer token. */
static int token(const char *s, const char *want) {
    int i = 0, wl = 0;
    if (!s || !s[0] || !want || !want[0]) return 0;
    while (want[wl]) wl++;
    while (s[i]) {
        int k = 0;
        while (s[i] == ' ') i++;
        while (s[i] && s[i] != ' ' && k < wl && s[i] == want[k]) { i++; k++; }
        if (k == wl && (s[i] == 0 || s[i] == ' ' || (s[i] == '-' && has_hyphen(want)))) return 1;
        while (s[i] && s[i] != ' ') i++;
    }
    return 0;
}

static int one_of(const char *id, const char *cls, const char *const *names) {
    int i;
    for (i = 0; names[i]; i++) {
        if (id && eq(id, names[i])) return 1;
        if (token(cls, names[i])) return 1;
    }
    return 0;
}

/* Classes and ids that name the article on sites that do not use
 * <article> or <main>. Each one is a whole token. */
static const char *const CONTENT[] = {
    "article-body", "article__body", "articlebody", "article-content",
    "story-body", "story-content", "post-content", "post-body",
    "entry-content", "entry-body", "markdown-body", "md-content",
    "md-content__inner", "rst-content", "devsite-article-body",
    "body-content", "content-body", "field-body", "node-content",
    "doc-content", "docs-content", "page-content", "main-column",
    "news-body", "hentry", "td-content", "doc-body", "documentation",
    "mw-parser-output", "vector-body", "caas-body", "article-text",
    "post-text", "entry-text", "story-text", "read-content",
    "reader-content", "c-blog-post", "blog-post", "news-article",
    0
};

/* Modest positives: a blog calls the post "post" or "story" or
 * "document". They win only when the text is denser than the links. */
static const char *const SOFT[] = {
    "post", "story", "entry", "document", "content", "article",
    "main-content", "maincontent", "bodycontent",
    0
};

/* Chrome around a news or docs page. The reader skips these even when
 * they sit inside a kept root. */
static const char *const CHROME[] = {
    "site-header", "page-header", "global-header", "masthead",
    "site-footer", "page-footer", "footer-nav", "global-footer",
    "sidebar", "side-bar", "rail", "complementary", "sidebar-wrap",
    "global-nav", "main-nav", "navbar", "nav-menu", "nav",
    "breadcrumb", "breadcrumbs", "breadcrumb-nav",
    "related-posts", "related-articles", "related", "related-content",
    "social-share", "share-buttons", "share-bar", "social",
    "newsletter", "subscribe-box", "subscribe",
    "pagination", "pager", "page-nav",
    "comments", "comment-list", "discussion", "comment-section",
    "ad-slot", "advertisement", "promo", "sponsored", "ad-container",
    "cookie-banner", "cookie-notice", "cookie", "consent", "gdpr",
    "skip-link", "visually-hidden", "sr-only",
    0
};

int wf_score_is_chrome(const char *id, const char *cls) {
    return one_of(id, cls, CHROME);
}

int wf_score_hint(const char *id, const char *cls) {
    if (wf_score_is_chrome(id, cls)) return -80;
    if (one_of(id, cls, CONTENT)) return 80;
    if (one_of(id, cls, SOFT)) return 30;
    return 0;
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
        if (i > s && !spc(in[i - 1])) continue;
        for (k = 0; k < al && lc(in[i + k]) == attr[k]; k++) {}
        if (k != al) continue;
        i += al;
        while (i < e && spc(in[i])) i++;
        if (i >= e || in[i] != '=') return 0;
        i++;
        while (i < e && spc(in[i])) i++;
        {
            char q = 0;
            int o = 0;
            if (i < e && (in[i] == '"' || in[i] == '\'')) q = in[i++];
            while (i < e && o < cap - 1) {
                if (q ? in[i] == q : (spc(in[i]) || in[i] == '>')) break;
                out[o++] = lc(in[i]);
                i++;
            }
            out[o] = 0;
            return o;
        }
    }
    return 0;
}

static int block_tag(const char *name) {
    return eq(name, "div") || eq(name, "section") || eq(name, "article") ||
           eq(name, "main") || eq(name, "td") || eq(name, "aside") ||
           eq(name, "nav") || eq(name, "header") || eq(name, "footer") ||
           eq(name, "table");
}

/* Letters outside tags. Letters inside <a> also count as links.
 * script and style are not text. */
static void measure(const char *in, int lo, int hi, int *text, int *links) {
    int i = lo, in_a = 0, skip = 0;
    *text = 0;
    *links = 0;
    if (lo < 0) lo = 0;
    if (hi < lo) hi = lo;
    i = lo;
    while (i < hi) {
        char nm[16];
        int cl = 0, end = 0;
        unsigned char c;
        if (in[i] != '<') {
            c = (unsigned char)in[i++];
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

typedef struct {
    int start;
    int hint;
    char name[12];
} wf_score_frame_t;

int wf_score_pick(const char *in, int n, int *lo, int *hi, int *text_out, int *link_out) {
    wf_score_frame_t st[WF_SCORE_STACK];
    int sp = 0, i = 0;
    int best_lo = 0, best_hi = 0, best = 0, best_text = 0, best_link = 0, found = 0;
    if (lo) *lo = 0;
    if (hi) *hi = n > 0 ? n : 0;
    if (text_out) *text_out = 0;
    if (link_out) *link_out = 0;
    if (!in || n < 32) return 0;
    while (i < n) {
        char nm[16], id[48], cls[96];
        int cl = 0, end = 0, hint, k;
        if (in[i] != '<') { i++; continue; }
        if (!tag_at(in, n, i, nm, (int)sizeof(nm), &cl, &end)) { i++; continue; }
        if (!block_tag(nm)) { i = end; continue; }
        if (!cl && in[end - 2] != '/') {
            id[0] = cls[0] = 0;
            attr_of(in, i + 1, end - 1, "id", id, (int)sizeof(id));
            attr_of(in, i + 1, end - 1, "class", cls, (int)sizeof(cls));
            hint = wf_score_hint(id, cls);
            if (sp < WF_SCORE_STACK) {
                wf_score_frame_t *f = &st[sp++];
                f->start = end;
                f->hint = hint;
                for (k = 0; k < 11 && nm[k]; k++) f->name[k] = nm[k];
                f->name[k] = 0;
            }
            i = end;
            continue;
        }
        if (cl) {
            for (k = sp - 1; k >= 0; k--) if (eq(st[k].name, nm)) {
                int text = 0, links = 0, score, span, need;
                measure(in, st[k].start, i, &text, &links);
                score = text - links * 2 + st[k].hint;
                /* Chrome (a negative hint) is never the content root. */
                if (st[k].hint < 0) score = 0;
                need = st[k].hint >= 60 ? WF_SCORE_CLASS_TEXT : WF_SCORE_MIN_TEXT;
                span = i - st[k].start;
                /* A density-only block that is almost the whole page does
                 * not change what the reader keeps. An explicit content
                 * class still wins, so the header outside it is dropped. */
                if (st[k].hint < 40 && span > (n * 85) / 100) score = 0;
                if (st[k].hint < 40 && !(text > links * 2)) score = 0;
                if (text >= need && text > links && score > best) {
                    best = score;
                    best_lo = st[k].start;
                    best_hi = i;
                    best_text = text;
                    best_link = links;
                    found = 1;
                }
                sp = k;
                break;
            }
        }
        i = end;
    }
    if (!found || best_hi <= best_lo) return 0;
    if (lo) *lo = best_lo;
    if (hi) *hi = best_hi;
    if (text_out) *text_out = best_text;
    if (link_out) *link_out = best_link;
    return 1;
}
