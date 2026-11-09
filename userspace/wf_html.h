/* wf_html.h - Lot 67: reduces a real web page to the HTML subset the
 * Mohhdy engine renders (title, headings, paragraphs, lists, links, pre,
 * emphasis), so a 100 KiB page fits the engine's bounded source and node
 * budget. Scripts, styles, SVG, forms, comments and attributes other than
 * href are dropped; links are made absolute against the page URL. This is
 * a reader view, not a full browser (no JavaScript, no CSS from the site). */
#ifndef WF_HTML_H
#define WF_HTML_H
#include "wf_http.h"
#include "wf_form.h"

typedef struct {
    int in_bytes;
    int out_bytes;
    int tags_kept;
    int tags_dropped;
    int blocks_dropped;   /* script/style/svg/... elements removed */
    int links;
    int truncated;        /* output budget reached */
    int charset;          /* Lot 71: WF_CS_* used for the text */
    char title[96];
    int images;           /* Lot 72: <img> kept (wf_html_set_images) */
    int images_dropped;   /* no usable src, data: URL, tracking pixel, over WF_HTML_IMG_MAX */
    int images_srcset;    /* Lot 73: URL chosen from srcset or a <picture> <source> */
    int sources_skipped;  /* Lot 73: <source> of a type not decoded or a media not matching */
    char icon[WF_URL_HOST_MAX + WF_URL_PATH_MAX + 16];   /* Lot 72: <link rel=icon>, absolute, or "" */
} wf_html_stats_t;
#define WF_HTML_IMG_MAX 48

/* Returns the output length (NUL-terminated, < cap). base may be 0.
 * Stops at 140 nodes (console and tests). Text is display bytes
 * (include/os_charset.h): UTF-8 / Latin-1 and character references
 * decoded, '<' '>' '&' re-escaped. */
int wf_html_reduce(const char* in, int n, const wf_url_t* base, char* out, int cap, wf_html_stats_t* st);
/* Lot 71: same with a node budget (the GUI keeps whole pages and shows
 * them in sections, wf_html_section) and, when forms is not 0, the forms
 * of the page recorded there and their controls written as placeholders
 * (see wf_form.h); with forms = 0 forms are dropped as before. */
int wf_html_reduce_ex(const char* in, int n, const wf_url_t* base, char* out, int cap, wf_html_stats_t* st,
                      int node_budget, wf_form_page_t* forms);
/* Lot 72: when on, <img> elements are kept for the GUI as
 * <img src="ABSOLUTE" width="W" height="H" alt="ALT"> (src, else
 * data-src, else the first srcset URL; data: URLs and 1x1 / 2x2 tracking
 * pixels are dropped) and the favicon link is reported in stats.icon.
 * Off by default (console text and earlier callers unchanged). */
void wf_html_set_images(int on);
/* Lot 73: responsive images (wf_srcset.h). The URL of an <img> is, in
 * order: the srcset of the first <source> of its <picture> whose type is
 * decoded and whose media matches, its own srcset (with sizes), then src,
 * data-src, data-lazy-src. vw is the viewport in CSS px (default 640),
 * dpr100 the device pixel ratio times 100 (default 100). */
void wf_html_set_viewport(int vw, int dpr100);
/* Charset announced by the server (Content-Type), WF_CS_*, or
 * WF_CS_NONE: then a <meta> charset in the page decides (HTML rule: the
 * transport charset wins). Applies to the next reduce. */
void wf_html_set_charset(int cs);
/* Lot 86: when on, the next reduce applies the page's <style> (display:none
 * skips a subtree; color, weight and align are copied onto the kept tag).
 * Off by default so earlier callers keep the same bytes. */
void wf_html_set_sitecss(int on);
int wf_html_sitecss_on(void);
/* Lot 87: external sheets (wf_csslink) applied before the page <style>
 * on the next reduce with sitecss on. 0 clears. The text is not copied. */
void wf_html_set_extcss(const char *css, int n);
/* Report of the sheet built by the last reduce that had sitecss on. */
int wf_html_sitecss_report(char *out, int cap);
/* Plain text of a page for the console (tags removed, blocks on new
 * lines). Lot 71: every character reference decoded and the text written
 * as UTF-8 (the console decodes UTF-8), Latin-1 pages converted. */
int wf_html_text(const char* in, int n, char* out, int cap);
/* Lot 71: same for a reduced page (display bytes in, UTF-8 out). */
int wf_html_text_display(const char* in, int n, char* out, int cap);
#endif
