/* userspace/webrt.h - Lot 5 Phase 3 web runtime building blocks (pure C, no
 * libc, host unit tested). They feed the OS-UI text renderer
 * (osui_runtime.c); none of this is a browser engine: no network fetch of
 * real pages here, no JavaScript, no layout.
 *   US-038 HTTP cache: Cache-Control/ETag freshness, LRU, revalidation.
 *   US-039 web security: CSP parsing and source checks, HTML sanitizer.
 *   US-040 responsive: media query evaluation, @media flattening.
 *   US-041 themes: CSS custom properties (var()) and colour scheme.
 *   US-042 Web Components: <template> custom elements with <slot>.
 *   US-044 accessibility: static audit of an HTML fragment. */
#ifndef MOHHDY_WEBRT_H
#define MOHHDY_WEBRT_H
#include <stdint.h>

/* ---- US-038 cache ---- */
#define WEBRT_CACHE_SLOTS 8
#define WEBRT_CACHE_MISS 0
#define WEBRT_CACHE_FRESH 1
#define WEBRT_CACHE_STALE 2    /* revalidate with If-None-Match: etag */
typedef struct {
    char url[96];
    char etag[32];
    uint32_t stored, max_age, last_use, bytes;
    uint8_t used, no_cache;
} webrt_cache_entry_t;
typedef struct {
    webrt_cache_entry_t e[WEBRT_CACHE_SLOTS];
    uint32_t hits, misses, stale, revalidated, evictions, stores, refused;
    uint32_t clock;   /* LRU use counter */
} webrt_cache_t;
void webrt_cache_init(webrt_cache_t* c);
/* Store a response. Returns the slot, or -1 when Cache-Control forbids it. */
int webrt_cache_store(webrt_cache_t* c, const char* url, const char* cache_control,
                      const char* etag, uint32_t bytes, uint32_t now);
int webrt_cache_lookup(webrt_cache_t* c, const char* url, uint32_t now, const char** etag);
/* Answer to a conditional request: 304 refreshes the entry, 200 replaces it
 * (with the new headers), anything else drops it. Returns 1 if kept. */
int webrt_cache_revalidated(webrt_cache_t* c, const char* url, int status, const char* cache_control,
                            const char* etag, uint32_t bytes, uint32_t now);

/* ---- US-039 security ---- */
#define WEBRT_CSP_MAX 8
typedef struct {
    char name[16];
    char sources[112];
} webrt_csp_directive_t;
typedef struct {
    webrt_csp_directive_t d[WEBRT_CSP_MAX];
    int count;
    int errors;
} webrt_csp_t;
int webrt_csp_parse(webrt_csp_t* p, const char* header);
/* url "inline" asks about an inline script/style ('unsafe-inline'). */
int webrt_csp_allows(const webrt_csp_t* p, const char* directive, const char* page_origin, const char* url);
int webrt_csp_enforce(const webrt_csp_t* p, const char* page_origin, const char* resource_type, const char* resource_url);
/* Origin "scheme://host[:port]" of an absolute URL into out (0 if relative). */
int webrt_url_origin(const char* url, char* out, int max);
/* Drops <script>, <iframe>, <object>, <embed>, <style> elements, on*
 * attributes and javascript: URLs. Returns the output length, *removed the
 * number of things removed. */
int webrt_sanitize_html(const char* in, char* out, int max, int* removed);
/* Lot 7 (US-039): same filter but <style> is kept, its CSS filtered
 * (@import, url(), javascript:, expression(), behavior dropped), and unsafe
 * inline style attributes removed. Used by the guest web engine pipeline. */
int webrt_sanitize_page(const char* in, char* out, int max, int* removed);

/* ---- US-040 responsive / US-041 themes ---- */
typedef struct {
    int width, height;
    int dark;      /* prefers-color-scheme: dark */
} webrt_viewport_t;
int webrt_mq_match(const char* query, const webrt_viewport_t* v);
/* Keeps the rules of matching @media blocks, drops the others. Returns the
 * output length; *kept / *dropped count the blocks. */
int webrt_css_flatten_media(const char* css, const webrt_viewport_t* v, char* out, int max,
                            int* kept, int* dropped);
/* Substitutes var(--name[, fallback]) with the custom property values
 * (declared anywhere, last wins) and drops the --name declarations. */
int webrt_css_resolve_vars(const char* css, char* out, int max, int* defined, int* unresolved);

/* ---- US-042 Web Components ---- */
int webrt_components_expand(const char* html, char* out, int max, int* defined, int* expanded);

/* ---- US-044 accessibility ---- */
typedef struct {
    int images, img_no_alt;
    int inputs, input_no_label;
    int buttons, button_no_text;
    int links, link_no_text;
    int headings, heading_skips;
    int lang_missing;
    int issues;
} webrt_a11y_t;
int webrt_a11y_audit(const char* html, webrt_a11y_t* r);

#endif
