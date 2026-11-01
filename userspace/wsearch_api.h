/* wsearch_api.h - Lot 71: search engines that answer the real guest.
 *
 * DuckDuckGo answers the Mohhdy guest with a bot check (Lot 69 real
 * campaign), so `search` can use, in order:
 *   searx       a SearXNG instance chosen by the user (HTML results page,
 *               or JSON when the instance enables format=json),
 *   brave-api   the Brave Search API with the user's key (JSON, key in a
 *               request header),
 *   google-api  the Google Programmable Search JSON API (key and cx),
 *   google      the Google results page of the basic HTML form (gbv=1),
 *   ddg / ddg-lite  as in Lot 69.
 * The configuration lives in /persist/search.cfg (never in the repo, the
 * keys are never printed back in full). Pure code, host tested in
 * tests/unit/userspace/test_wsearch_api.c; the requests go through the
 * Lot 67 fetch stack (userspace/shell_search.c). */
#ifndef WSEARCH_API_H
#define WSEARCH_API_H
#include "wsearch.h"

#define WSEARCH_KEY_MAX 96
#define WSEARCH_CFG_MAX 1024

enum {
    WSEARCH_KIND_DDG = 0,
    WSEARCH_KIND_SEARX = 1,
    WSEARCH_KIND_BRAVE = 2,
    WSEARCH_KIND_GOOGLE_API = 3,
    WSEARCH_KIND_GOOGLE = 4,
    WSEARCH_KIND_URL = 5,          /* a user template (Lot 69) */
    WSEARCH_KIND_WIKI = 6,         /* Wikipedia full text search API (no key, no JavaScript) */
};
enum { WSEARCH_LAYOUT_SEARX = 10, WSEARCH_LAYOUT_JSON = 11, WSEARCH_LAYOUT_GOOGLE = 12, WSEARCH_LAYOUT_WIKI = 13 };

typedef struct {
    char engine[WSEARCH_ENGINE_URL];   /* name or template */
    char searx[WSEARCH_ENGINE_URL];    /* https://instance (no path needed) */
    int searx_json;                    /* ask format=json */
    char brave_key[WSEARCH_KEY_MAX];
    char google_key[WSEARCH_KEY_MAX];
    char google_cx[WSEARCH_KEY_MAX];
    char wiki[8];                      /* Wikipedia language ("fr") */
} wsearch_cfg_t;

void wsearch_cfg_default(wsearch_cfg_t *c);
/* "key=value" lines (engine, searx, searx_json, brave_key, google_key,
 * google_cx); unknown keys and comments (#) ignored. Returns the number
 * of keys read. */
int wsearch_cfg_parse(wsearch_cfg_t *c, const char *text, int n);
int wsearch_cfg_format(const wsearch_cfg_t *c, char *out, int cap);
/* sets one key (same names); 0 or -1 (unknown key, bad value) */
int wsearch_cfg_set(wsearch_cfg_t *c, const char *key, const char *value);
/* "abcd...wxyz (40 chars)" or "(none)": a key for a status line */
void wsearch_key_mask(const char *key, char *out, int cap);

/* kind of an engine name (searx, brave-api, google-api, google, ddg,
 * ddg-html, ddg-lite) or WSEARCH_KIND_URL for a template URL, -1 unknown */
int wsearch_kind(const char *engine);
/* Request for engine kind: URL (and extra header lines for brave-api).
 * -1 when the engine is not configured (no instance, no key). */
int wsearch_api_request(const wsearch_cfg_t *c, int kind, const char *query, char *url, int url_cap, char *extra,
                        int extra_cap);
/* Parses an answer of engine kind (JSON or HTML, sniffed). Returns the
 * result count (page->layout says which parser matched). */
int wsearch_api_parse(int kind, const char *body, int len, wsearch_page_t *page);

/* Building blocks, exported for the tests. */
int wsearch_parse_json(const char *json, int len, wsearch_page_t *page);
int wsearch_parse_searx_html(const char *html, int len, wsearch_page_t *page);
int wsearch_parse_google_html(const char *html, int len, wsearch_page_t *page);
/* {"query":{"search":[{"title":...,"snippet":...}]}} of the MediaWiki
 * API; result URLs are built on https://LANG.wikipedia.org/wiki/. */
int wsearch_parse_wiki(const char *json, int len, const char *lang, wsearch_page_t *page);
/* JSON string at s[0] == '"' decoded to UTF-8 (\\uXXXX and surrogate
 * pairs); returns the bytes consumed or -1. */
int wsearch_json_string(const char *s, int n, char *out, int cap);
/* text of an HTML fragment: tags removed, character references decoded,
 * spaces collapsed, written as UTF-8 */
int wsearch_html_text(const char *s, int n, char *out, int cap);
#endif
