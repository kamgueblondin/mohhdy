/* wf_nav.h - Lot 86: browser history, back/forward and bookmarks.
 *
 * One caller-owned store, no heap. The session stack is what Back and
 * Forward walk. The visit log is a ring of the same navigations, kept
 * even after the forward list is cut. Bookmarks are a separate list.
 * A URL is stored absolute, with the fragment dropped and the host in
 * lower case. javascript: and data: are refused. Nothing here opens a
 * socket: the caller fetches the URL the store returns.
 */
#ifndef WF_NAV_H
#define WF_NAV_H
#include <stdint.h>

#define WF_NAV_HIST 40
#define WF_NAV_STACK 24
#define WF_NAV_MARKS 24
#define WF_NAV_URL 180
#define WF_NAV_TITLE 72

typedef struct {
    char url[WF_NAV_URL];
    char title[WF_NAV_TITLE];
    char stamp[15];             /* YYYYMMDDHHMMSS or "" */
} wf_nav_ent_t;

typedef struct {
    wf_nav_ent_t hist[WF_NAV_HIST];
    int hist_n;                 /* how many of the ring are live, <= WF_NAV_HIST */
    int hist_next;              /* next write index */
    wf_nav_ent_t stack[WF_NAV_STACK];
    int stack_n;
    int cursor;                 /* -1 empty, else the current entry */
    wf_nav_ent_t mark[WF_NAV_MARKS];
    int nmark;
    uint32_t visits, backs, forwards, marked, replaced, refused, dropped;
} wf_nav_t;

void wf_nav_init(wf_nav_t *n);

/* Absolute URL of ref against base (base may be 0 when ref is absolute).
 * 0 and out holds the canonical URL, -1 refused or does not fit. */
int wf_nav_resolve(const char *base, const char *ref, char *out, int cap);

/* Push a navigation. Same URL as the current entry updates the title
 * and does not cut the forward list. A different URL drops the forward
 * list. Returns 0, or -1 refused. */
int wf_nav_visit(wf_nav_t *n, const char *url, const char *title, const char *stamp);

/* Move the cursor. 0 and *url receives the target (cap bytes), -1 when
 * there is nowhere to go or url does not fit. Does not itself record a
 * new visit: the caller loads the URL and must not call visit for it. */
int wf_nav_back(wf_nav_t *n, char *url, int cap);
int wf_nav_forward(wf_nav_t *n, char *url, int cap);
int wf_nav_current(const wf_nav_t *n, char *url, int cap);

/* Bookmarks. add replaces an existing URL. del removes by exact URL.
 * Returns 0, or -1. */
int wf_nav_mark_add(wf_nav_t *n, const char *url, const char *title);
int wf_nav_mark_del(wf_nav_t *n, const char *url);
/* index 0..count-1, or -1 */
int wf_nav_mark_count(const wf_nav_t *n);
const wf_nav_ent_t *wf_nav_mark_at(const wf_nav_t *n, int index);
/* first bookmark whose url or title contains needle (case sensitive), or -1 */
int wf_nav_mark_find(const wf_nav_t *n, const char *needle);

int wf_nav_hist_count(const wf_nav_t *n);
/* 0 is the oldest live visit, count-1 the newest. */
const wf_nav_ent_t *wf_nav_hist_at(const wf_nav_t *n, int index);

/* Text form, one record per line:
 *   H <url> <tab> <title> <tab> <stamp>
 *   B <url> <tab> <title>
 *   S <url> <tab> <title> <tab> <stamp>     (the session stack, in order)
 *   C <cursor>
 * Load replaces the store. Returns bytes written, or -1. */
int wf_nav_save(const wf_nav_t *n, char *out, int cap);
int wf_nav_load(wf_nav_t *n, const char *text, int len);

/* One status line. */
int wf_nav_text(const wf_nav_t *n, char *out, int cap);
#endif
