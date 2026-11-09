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
} wf_compat_t;

void wf_compat_scan(const char *html, int n, wf_compat_t *c);
/* One line. verdict is "js-wall", "reader", "table", "short" or "empty". */
int wf_compat_line(const char *host, const wf_compat_t *c, char *out, int cap);
const char *wf_compat_verdict(const wf_compat_t *c);

/* Hand-checked expectations for the 2026-11-09 measurements. */
typedef struct {
    const char *host;
    const char *verdict;
    int min_bytes;
    int js_wall;
} wf_compat_expect_t;

int wf_compat_expect_count(void);
const wf_compat_expect_t *wf_compat_expect_at(int i);
#endif
