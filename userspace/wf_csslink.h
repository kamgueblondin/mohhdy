/* wf_csslink.h - Lot 87: external stylesheets for the reader.
 * Finds <link rel=stylesheet> (not "alternate") and the leading @import
 * rules of <style> blocks, resolves them against the page URL, skips a
 * sheet whose media list does not apply, fetches at most WF_CSSL_SHEETS
 * through a caller callback, follows one level of @import inside a
 * fetched sheet, and writes one text for wf_site_prepare_ext: imports
 * first, each sheet wrapped in @media when it had a media list. No heap,
 * no network of its own.
 * Lot 87: "@import url(x) layer(name) supports(cond) media": the sheet goes
 * into "@layer name { }" (anonymous "@layer { }" for a bare "layer"), and
 * is not fetched when supports() is false. "@layer a, b;" statements may
 * come before the imports; they are kept in front so the order holds. */
#ifndef WF_CSSLINK_H
#define WF_CSSLINK_H

#define WF_CSSL_SHEETS 4          /* fetch attempts, imports included */
#define WF_CSSL_FOUND 8           /* candidates kept from the page */
#define WF_CSSL_SHEET_MAX 12288   /* bytes kept from one sheet */
#define WF_CSSL_URL 256

/* Fetch one URL into buf (cap bytes). Returns the length, or -1. */
typedef int (*wf_css_get_t)(void *ctx, const char *url, char *buf, int cap);

typedef struct {
    char url[WF_CSSL_URL];
    char media[32];
    int from_import;
    int layered;        /* Lot 87: @import ... layer / layer(name) */
    char layer[32];     /* "" for an anonymous layer */
} wf_cssl_ref_t;

typedef struct {
    int found;          /* candidates in the page */
    int fetched;        /* sheets fetched */
    int failed;         /* fetch errors */
    int skipped_media;  /* media list did not apply: not fetched */
    int refused;        /* not http(s), or no base */
    int dropped;        /* over WF_CSSL_SHEETS or the output buffer */
    int truncated;      /* a sheet longer than WF_CSSL_SHEET_MAX */
    int bytes;          /* bytes written to out */
    int unsupported;    /* Lot 87: @import supports() false: not fetched */
    int layered;        /* Lot 87: sheets placed in a cascade layer */
} wf_cssl_stats_t;

/* The candidates of a page, in document order (links and leading
 * @import), resolved against base_url. Returns the count. */
int wf_csslink_scan(const char *html, int n, const char *base_url, wf_cssl_ref_t *out, int cap,
                    wf_cssl_stats_t *st);
/* Leading @import rules of one stylesheet text (they must come before
 * any rule). Resolved against sheet_url. Returns the count. */
int wf_csslink_imports(const char *css, int n, const char *sheet_url, wf_cssl_ref_t *out, int cap);
/* Scan, fetch and join. Returns bytes written to out (0 when no sheet),
 * or -1 on bad arguments. */
int wf_csslink_collect(const char *html, int n, const char *base_url, int vw, int vh,
                       wf_css_get_t get, void *ctx, char *out, int cap, wf_cssl_stats_t *st);
/* "css-links found F fetched N failed E media M refused R dropped D bytes B"
 * Lot 87: then " layered L" and " unsupported U" when not zero. */
int wf_csslink_stats_text(const wf_cssl_stats_t *st, char *out, int cap);
#endif
