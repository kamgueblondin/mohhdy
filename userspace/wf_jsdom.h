/* wf_jsdom.h - Lot 113: the page script pass. A tolerant HTML parser
 * builds a bounded DOM, the inline scripts run against it in the
 * JavaScript subset (wf_js.h) with document and window host objects,
 * and the DOM is written back as HTML for the existing sanitizer and
 * renderer (the sanitizer still removes every script, on* attribute and
 * javascript: URL afterwards: US-039 is unchanged).
 *
 * Bounds: WF_DOM_NODES nodes, WF_DOM_ATTRS attributes, WF_DOM_BYTES of
 * text; external scripts (src=) are not fetched; no events after load
 * (no clicks reach scripts), timers run once in delay order after load
 * (intervals at most WF_DOM_INTERVAL_RUNS times), no fetch or
 * XMLHttpRequest, no layout values (sizes read as 0). */
#ifndef WF_JSDOM_H
#define WF_JSDOM_H

#include <stdint.h>

#define WF_DOM_NODES 6000
#define WF_DOM_ATTRS 9000
#define WF_DOM_BYTES (384 * 1024)
#define WF_DOM_TIMERS 64
#define WF_DOM_INTERVAL_RUNS 3
#define WF_DOM_SCRIPT_BUDGET 3000000u     /* steps per script or callback */
#define WF_DOM_PAGE_CALLS 256             /* scripts + callbacks per page */

typedef struct {
    int scripts;        /* inline scripts found */
    int ran;            /* ran to the end */
    int failed;         /* threw or hit a bound */
    int external;       /* src= scripts, not fetched */
    int other_type;     /* type= not JavaScript (JSON, templates, modules) */
    int writes;         /* document.write calls */
    int timers;         /* timer callbacks run */
    int events;         /* DOMContentLoaded and load listeners run */
    int nodes;          /* DOM nodes after the scripts */
    int truncated;      /* the parse or the output hit a bound */
    uint32_t mem;       /* arena bytes used */
    uint32_t steps;     /* interpreter steps, all runs */
    char first_err[160];
} wf_jspage_stats_t;

/* run the inline scripts of html[0..n) against its DOM; the page as
 * HTML after the scripts goes to out. Returns the output length, 0 when
 * the page has no inline script (out untouched), -1 when the pass could
 * not run (the caller keeps the original page). arena: at least 1 MiB. */
int wf_jspage_run(void *arena, uint32_t arena_size, const char *html, int n, const char *url, char *out, int out_max,
                  wf_jspage_stats_t *st);
/* the console log of the last page run */
const char *wf_jspage_log(int *len);

/* DOM only (host tests): parse and serialize without scripts */
int wf_jsdom_roundtrip(const char *html, int n, char *out, int out_max);

#endif
