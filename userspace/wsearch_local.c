/* wsearch_local.c - Lot 85. See wsearch_local.h. No heap, no network. */
#include "wsearch_local.h"

typedef struct { const char *title; const char *url; const char *text; } loc_t;

/* Titles and blurbs a guest can open without the public Internet.
 * URLs are local pages or the command that shows the thing. */
static const loc_t k_cat[] = {
    { "fetch", "local:fetch", "fetch and http-get load an http or https page in the shell and the browser. TLS checks the certificate. The body is kept in a bounded buffer." },
    { "search", "local:search", "search WORDS asks Wikipedia first, then DuckDuckGo, then Google. search-engine local searches this catalogue. search-open N follows a result." },
    { "reader", "local:reader", "A fetched page is reduced to a reader view: the article or main element, headings, paragraphs and links. Navigation, the table of contents and cookie banners are left out." },
    { "vietnamese", "local:glyphs", "Vietnamese letters and O or U with a horn are drawn from the base glyph plus an accent. Fullwidth ASCII is folded. A few rare combinations stay the base letter." },
    { "masonry", "local:masonry", "grid-template-rows masonry packs items into the shortest column. Auto columns are sized from the column the item occupies after that packing." },
    { "form-factor", "local:form-factor", "form-factor reads the screen and the EDID size and plans phone, tablet, desktop or tv. display-info prints the physical size when the EDID block is valid." },
    { "power", "local:power", "power-apply sets performance, balanced or saver. The battery is simulated. After the profile's idle time the refresh is halved. power-sim advances the simulated battery." },
    { "ui-scale", "local:ui-scale", "ui-scale and text-size and Ctrl plus or minus share one scale for every window: web, chat, shell, pane, ide and the app windows." },
    { "windows", "local:windows", "Every window uses the same title buttons: minimize, maximize and close, and a resize grip. The full-screen web window uses that chrome too." },
    { "wikipedia", "local:wikipedia", "The Wikipedia search API needs no key and no JavaScript. Result links point at the language wiki. Google's HTML page still asks for a newer browser." },
    { "p2p", "local:p2p", "The bandwidth estimate samples a probe ack even when it arrives after the loss timeout. That late ack is not kept as a confirmed loss." },
    { "edid", "local:edid", "The EDID parser reads the preferred timing, the monitor name, the range limits, standard timings and the established VESA modes. Real monitors over DDC are not driven." },
};

static int llen(const char *s) { int n = 0; while (s && s[n]) n++; return n; }

static char low(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

static int fold_eq(const char *a, int an, const char *b, int bn) {
    int i;
    if (an != bn) return 0;
    for (i = 0; i < an; i++) if (low(a[i]) != low(b[i])) return 0;
    return 1;
}

/* Word i of the query, or 0. Words are split on space. */
static int word_at(const char *q, int which, const char **start, int *len) {
    int i = 0, n = 0;
    while (q[i]) {
        while (q[i] == ' ') i++;
        if (!q[i]) break;
        if (n == which) {
            int j = i;
            while (q[j] && q[j] != ' ') j++;
            *start = q + i;
            *len = j - i;
            return 1;
        }
        while (q[i] && q[i] != ' ') i++;
        n++;
    }
    return 0;
}

static int count_words(const char *q) {
    const char *s;
    int n = 0, len;
    while (word_at(q, n, &s, &len)) n++;
    return n;
}

/* How many times the folded word occurs in text. */
static int hits(const char *text, const char *word, int wn) {
    int i, n = llen(text), c = 0;
    for (i = 0; i + wn <= n; i++) {
        int boundary = (i == 0) || text[i - 1] == ' ';
        int end = (i + wn == n) || text[i + wn] == ' ' || text[i + wn] == '.' || text[i + wn] == ',';
        if (boundary && end && fold_eq(text + i, wn, word, wn)) c++;
    }
    return c;
}

static void copy_cap(char *d, int cap, const char *s) {
    int i = 0;
    if (cap <= 0) return;
    while (s[i] && i < cap - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}

int wsearch_local_query(const char *query, wsearch_page_t *page) {
    int docs = (int)(sizeof(k_cat) / sizeof(k_cat[0]));
    int words, d, w, nscore[32];
    if (!page) return 0;
    {
        int i;
        for (i = 0; i < (int)sizeof(*page); i++) ((char *)page)[i] = 0;
    }
    if (!query || !query[0]) return 0;
    words = count_words(query);
    if (words < 1) return 0;
    if (docs > 32) docs = 32;
    for (d = 0; d < docs; d++) {
        int score = 0;
        for (w = 0; w < words; w++) {
            const char *word;
            int wn, h;
            if (!word_at(query, w, &word, &wn) || wn < 2) continue;
            h = hits(k_cat[d].title, word, wn) * 5 + hits(k_cat[d].text, word, wn);
            if (!h) { score = 0; break; }   /* every word must occur */
            score += h;
        }
        nscore[d] = score;
    }
    while (page->n < WSEARCH_MAX) {
        int best = -1, bests = 0;
        for (d = 0; d < docs; d++) if (nscore[d] > bests) { bests = nscore[d]; best = d; }
        if (best < 0) break;
        copy_cap(page->r[page->n].title, WSEARCH_TITLE, k_cat[best].title);
        copy_cap(page->r[page->n].url, WSEARCH_URL, k_cat[best].url);
        copy_cap(page->r[page->n].snippet, WSEARCH_SNIPPET, k_cat[best].text);
        page->n++;
        nscore[best] = 0;
    }
    if (page->n) page->layout = WSEARCH_LAYOUT_LOCAL;
    return page->n;
}
