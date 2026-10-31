/* wf_html.h - Lot 67: reduces a real web page to the HTML subset the
 * Mohhdy engine renders (title, headings, paragraphs, lists, links, pre,
 * emphasis), so a 100 KiB page fits the engine's bounded source and node
 * budget. Scripts, styles, SVG, forms, comments and attributes other than
 * href are dropped; links are made absolute against the page URL. This is
 * a reader view, not a full browser (no JavaScript, no CSS from the site). */
#ifndef WF_HTML_H
#define WF_HTML_H
#include "wf_http.h"

typedef struct {
    int in_bytes;
    int out_bytes;
    int tags_kept;
    int tags_dropped;
    int blocks_dropped;   /* script/style/svg/... elements removed */
    int links;
    int truncated;        /* output budget reached */
    char title[96];
} wf_html_stats_t;

/* Returns the output length (NUL-terminated, < cap). base may be 0. */
int wf_html_reduce(const char* in, int n, const wf_url_t* base, char* out, int cap, wf_html_stats_t* st);
/* Plain text of a page for the console (tags removed, blocks on new
 * lines, entities &amp; &lt; &gt; &quot; &#39; &nbsp; decoded). */
int wf_html_text(const char* in, int n, char* out, int cap);
#endif
