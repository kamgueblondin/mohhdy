/* wsearch.h - Lot 69: Internet search without JavaScript.
 * Pure code (host tested in tests/unit/userspace/test_wsearch.c): query
 * URL encoding, search engine table, result parsing of the HTML pages of
 * no-JS engines (DuckDuckGo html and lite layouts, with a generic link
 * fallback when the markup changes), DuckDuckGo redirect decoding (uddg=)
 * and the result page shown in /browser. The network GET is the Lot 67
 * fetch stack (userspace/shell_search.c). Bounded: WSEARCH_MAX results of
 * fixed size, no heap. */
#ifndef WSEARCH_H
#define WSEARCH_H

#define WSEARCH_MAX 10
#define WSEARCH_TITLE 96
#define WSEARCH_URL 384
#define WSEARCH_SNIPPET 240
#define WSEARCH_QUERY 200
#define WSEARCH_ENGINE_URL 200

typedef struct {
    char title[WSEARCH_TITLE];
    char url[WSEARCH_URL];
    char snippet[WSEARCH_SNIPPET];
} wsearch_result_t;

typedef struct {
    wsearch_result_t r[WSEARCH_MAX];
    int n;
    int layout;          /* WSEARCH_LAYOUT_* that matched */
    int blocked;         /* the engine answered a bot check / captcha page */
    int skipped;         /* links dropped (engine internal, ads, duplicates) */
} wsearch_page_t;

enum { WSEARCH_LAYOUT_NONE = 0, WSEARCH_LAYOUT_DDG_HTML = 1, WSEARCH_LAYOUT_DDG_LITE = 2, WSEARCH_LAYOUT_GENERIC = 3 };

/* application/x-www-form-urlencoded: unreserved kept, space '+', others
 * %XX (UTF-8 bytes as they are). Returns the length or -1 if cap is short. */
int wsearch_urlencode(const char *in, char *out, int cap);
/* %XX and '+' decoding, in place allowed (out may equal in); returns length */
int wsearch_urldecode(const char *in, int len, char *out, int cap);
/* engine template: "ddg" / "ddg-html", "ddg-lite", or a URL containing
 * "%s" (replaced by the encoded query) or ending with '=' (appended).
 * Returns 0, or -1 for an unusable template / overflow. */
int wsearch_build_url(const char *engine, const char *query, char *out, int cap);
/* canonical template of a known engine name, 0 when not a known name */
const char *wsearch_engine_template(const char *name);
/* result href -> target URL: "//duckduckgo.com/l/?uddg=ENC&..." decoded,
 * "//host/x" gets https:, relative and javascript: links refused (-1). */
int wsearch_decode_href(const char *href, char *out, int cap);
/* parses a result page; returns the result count */
int wsearch_parse(const char *html, int len, wsearch_page_t *page);
/* numbered console lines "search N TITLE\n  URL\n  SNIPPET\n" */
int wsearch_format(const wsearch_page_t *page, char *out, int cap);
/* small HTML page listing the results with links, for /browser */
int wsearch_results_html(const wsearch_page_t *page, const char *query, const char *engine_url,
                         char *out, int cap);
/* Lot 72: same with a note line under the header (0: none) */
int wsearch_results_html_note(const wsearch_page_t *page, const char *query, const char *engine_url, const char *note,
                              char *out, int cap);
/* text that is not a URL or a local path: the address bar searches it */
int wsearch_is_query(const char *text);
#endif
