/* wf_compat.h - Lot 86: a bounded reading of a fetched page, for the
 * compat table. It does not run JavaScript and it does not claim a
 * site works when the bytes are a script wall. No heap. */
#ifndef WF_COMPAT_H
#define WF_COMPAT_H
#include <stdint.h>

typedef struct {
    int bytes;
    int text_bytes;             /* outside script and style */
    int scripts;
    int links;
    int forms;
    int tables;
    int images;
    int title_len;
    char title[96];
    int js_wall;                /* enablejs, noscript-only, or "update your browser" */
    int has_article;
    int cookie_banner;
    int search_form;
    /* Lot 87 */
    int http_status;            /* 0 when unknown */
    int wall_kind;              /* WF_WALL_* */
    int inputs;                 /* visible input/textarea/select */
    int empty_root;             /* <div id=root|app|__next|__nuxt> with nothing inside */
    int meta_refresh;           /* outside noscript */
    char fallback[160];         /* noscript refresh or link target, as written */
} wf_compat_t;

/* Lot 87: why a page shows nothing useful without scripts. */
enum {
    WF_WALL_NONE = 0,
    WF_WALL_ENABLEJS,           /* "enable javascript", enablejs retry pages */
    WF_WALL_NOSCRIPT,           /* noscript refresh or link is the only content */
    WF_WALL_CHALLENGE,          /* bot challenge: cf-chl, "just a moment", captcha */
    WF_WALL_BLOCKED,            /* 403/429 with a block notice */
    WF_WALL_SPA,                /* empty app root filled by script */
    WF_WALL_BROWSER             /* "update your browser" */
};

void wf_compat_scan(const char *html, int n, wf_compat_t *c);
/* One line. verdict is "js-wall", "reader", "table", "short" or "empty". */
int wf_compat_line(const char *host, const wf_compat_t *c, char *out, int cap);
const char *wf_compat_verdict(const wf_compat_t *c);
/* Lot 87: the scan with the HTTP status, which a block page needs. */
void wf_compat_scan_ex(const char *html, int n, int http_status, wf_compat_t *c);
const char *wf_compat_wall_name(int kind);
/* One honest line on what the reader can do with the page: it never says
 * a script wall works. Returns the length or -1. */
int wf_compat_advice(const wf_compat_t *c, char *out, int cap);

/* Hand-checked expectations for the 2026-11-09 measurements. */
typedef struct {
    const char *host;
    const char *verdict;
    int min_bytes;
    int js_wall;
} wf_compat_expect_t;

int wf_compat_expect_count(void);
const wf_compat_expect_t *wf_compat_expect_at(int i);
/* Lot 87 measurements (2026-11-09, curl from the box as a stand-in for
 * the guest path); fixtures in tests/fixtures/compat are hand-reduced. */
int wf_compat_expect87_count(void);
const wf_compat_expect_t *wf_compat_expect87_at(int i);
const char *wf_compat_expect87_file(int i);
int wf_compat_expect87_status(int i);
int wf_compat_expect87_wall(int i);
#endif
