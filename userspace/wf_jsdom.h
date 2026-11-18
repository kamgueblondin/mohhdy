/* wf_jsdom.h - Lot 113: the page script pass. A tolerant HTML parser
 * builds a bounded DOM, the inline scripts run against it in the
 * JavaScript subset (wf_js.h) with document and window host objects,
 * and the DOM is written back as HTML for the existing sanitizer and
 * renderer (the sanitizer still removes every script, on* attribute and
 * javascript: URL afterwards: US-039 is unchanged).
 *
 * Bounds: WF_DOM_NODES nodes, WF_DOM_ATTRS attributes, WF_DOM_BYTES of
 * text; timers run in delay order on a virtual clock after load
 * (intervals at most WF_DOM_INTERVAL_RUNS times per pass).
 *
 * Lot 114: with a fetch hook (wf_jspage_set_fetch) external scripts
 * (src=) are fetched and run in document order (defer and async ones
 * after parsing, at most WF_DOM_EXT_SCRIPTS of WF_DOM_EXT_BYTES each),
 * fetch() and XMLHttpRequest reach the same origin through the same hook
 * (WF_DOM_NET_REQS requests per pass, WF_DOM_NET_BYTES each, answers
 * arrive as tasks after the current script). Promise jobs (microtasks)
 * run after every script, callback and event. The realm stays alive
 * after the pass: wf_jspage_event dispatches a click, key, input or other
 * event to an element and writes the page again, so the shell and the
 * GUI reach page scripts after load through one path. Elements with a
 * click handler are written inside a link "?__wfjs=N" (N: the node) that
 * the GUI turns into "web-js click @N". Layout values come from an
 * approximate block model (8x16 px characters, a 1024x768 viewport, see
 * wf_jsdom.c "layout"), not from the renderer. */
#ifndef WF_JSDOM_H
#define WF_JSDOM_H

#include <stdint.h>

#define WF_DOM_NODES 6000
#define WF_DOM_ATTRS 9000
#define WF_DOM_BYTES (384 * 1024)
#define WF_DOM_TIMERS 64
#define WF_DOM_INTERVAL_RUNS 3
#define WF_DOM_SCRIPT_BUDGET 3000000u     /* steps per script or callback */
#define WF_DOM_PAGE_CALLS 256             /* scripts + callbacks per page (and per live event) */
#define WF_DOM_EXT_SCRIPTS 16             /* Lot 114: external scripts per pass */
#define WF_DOM_EXT_BYTES (256 * 1024)     /* one external script */
#define WF_DOM_NET_REQS 16                /* fetch() and XMLHttpRequest per pass */
#define WF_DOM_NET_BYTES (256 * 1024)     /* one answer body */
#define WF_DOM_DYN_SCRIPTS 16             /* scripts inserted by scripts, waiting to run */
#define WF_DOM_MARKS 128                  /* clickable elements marked in the output */
#define WF_DOM_VIEW_W 1024
#define WF_DOM_VIEW_H 768

typedef struct {
    int scripts;        /* inline scripts found */
    int ran;            /* ran to the end */
    int failed;         /* threw or hit a bound */
    int external;       /* src= scripts found */
    int ext_ran;        /* Lot 114: external scripts fetched and run to the end */
    int ext_failed;     /* fetch failed, not 200, over a bound, or threw */
    int ext_skipped;    /* not fetched (no fetch hook, or the bound was reached) */
    uint32_t ext_bytes; /* bytes of external script fetched */
    int dynamic;        /* scripts inserted by scripts that ran */
    int microtasks;     /* Promise jobs run */
    int rejections;     /* unhandled Promise rejections reported */
    int net_requests;   /* fetch() and XMLHttpRequest sent */
    int net_failed;     /* refused (bound, origin) or failed */
    int handlers;       /* on* attributes compiled into handlers */
    int layout_reads;   /* layout values read (offsetWidth, getBoundingClientRect...) */
    int live_events;    /* events dispatched after the pass (wf_jspage_event) */
    int fields;         /* reader field events among them (field-*) */
    int marks;          /* clickable elements marked in the output */
    int other_type;     /* type= not JavaScript (JSON, templates, modules) */
    int writes;         /* document.write calls */
    int timers;         /* timer callbacks run */
    int events;         /* DOMContentLoaded and load listeners run */
    int nodes;          /* DOM nodes after the scripts */
    int truncated;      /* the parse or the output hit a bound */
    uint32_t mem;       /* arena bytes used */
    uint32_t steps;     /* interpreter steps, all runs */
    char first_err[160];
    int layout_engine;  /* Lot 115: elements whose layout values came from the renderer (last layout) */
} wf_jspage_stats_t;

/* run the inline scripts of html[0..n) against its DOM; the page as
 * HTML after the scripts goes to out. Returns the output length, 0 when
 * the page has no inline script (out untouched), -1 when the pass could
 * not run (the caller keeps the original page). arena: at least 1 MiB. */
int wf_jspage_run(void *arena, uint32_t arena_size, const char *html, int n, const char *url, char *out, int out_max,
                  wf_jspage_stats_t *st);
/* the console log of the last page run */
const char *wf_jspage_log(int *len);

/* Lot 114: the network hook of the pass (shell_fetch.c gives one built on
 * wf_guest_fetch_ex; host tests give a table). The hook fills status,
 * len (the body is cut at out_max, truncated set), ctype_out and
 * final_url; it returns 0 when an answer came, -1 on a network error. */
typedef struct {
    const char *url;            /* absolute http(s) URL */
    const char *method;         /* "GET" or "POST" */
    const char *body;           /* POST body or 0 */
    int body_len;
    const char *ctype;          /* POST content type or 0 */
    char *out;
    int out_max;
    int status;
    int len;
    int truncated;
    char ctype_out[96];
    char final_url[512];
} wf_jsreq_t;
typedef int (*wf_jsfetch_fn)(void *ctx, wf_jsreq_t *r);
void wf_jspage_set_fetch(wf_jsfetch_fn fn, void *ctx);

/* Lot 115: layout values from the renderer. The pass hands the hook the
 * document serialized without scripts (at most WF_JSLAY_NODES element and
 * text nodes: the rest is left out, closed tags kept) and the viewport;
 * the hook lays it out (wf_jslayout.c: webeng, the engine the GUI paints
 * with) and writes the border box of each element it laid out, in
 * document order, into out (at most max). It returns the boxes written,
 * or -1. The pass matches them to its elements by tag in document order
 * and keeps its block model for the elements the engine did not lay out
 * (past the node budget, or refused). No hook: the block model only. */
#define WF_JSLAY_NODES 150
typedef struct {
    char tag[12];               /* lowercased, cut at 11 characters */
    int x, y, w, h;             /* border box in page px */
    int cw, ch;                 /* the padding box (clientWidth / clientHeight) */
    unsigned char ok;           /* 1: a box, 0: no box (display:none), 2: the engine has no box
                                 * for it (an inline element without painted text): keep the model */
} wf_jslay_box_t;
typedef int (*wf_jslayout_fn)(void *ctx, const char *html, int n, int vw, int vh, wf_jslay_box_t *out, int max);
void wf_jspage_set_layout(wf_jslayout_fn fn, void *ctx);

/* 1 while the realm of the last pass that ran scripts is alive (until the
 * next wf_jspage_run or wf_jspage_close) */
int wf_jspage_alive(void);
void wf_jspage_close(void);
/* an event after load: type "click", "key" (arg: a key name such as "a"
 * or "Enter"), "input" (arg: the new value), "change", "focus", "blur",
 * "submit" or any other name (a plain bubbling Event). target: "@N" (a
 * node, as in the ?__wfjs=N links), a CSS selector, or empty (the focused
 * element, else the body). Promise jobs, timers and inserted scripts run
 * after it; the page is written to out again. Returns the output length,
 * -1 when no realm is alive, -2 when the target is not found, -3 when
 * the run could not start (arena full). st gets the stats of the pass
 * updated (pass the same struct as the run, or 0). */
/* Lot 114: reader form fields. A "field-ACTION" type (input, check, change,
 * focus, blur, submit) with the target "=ORD,NTH,NAME" (control NTH of
 * those named NAME, or control ORD in document order when NAME is empty,
 * as webeng_finput.inc counts them) tells the scripts what the reader's
 * control did. After wf_jspage_event, wf_jspage_event_flags says what the
 * reader must do: WF_JSEV_DOM write the page again, WF_JSEV_PREVENTED do
 * not send the form (or keep the old check), WF_JSEV_VALUE take the value
 * a script gave the field (wf_jspage_field_value, newlines as spaces,
 * "1"/"0" for a check, at most WF_DOM_FIELD_VAL - 1 bytes). */
#define WF_DOM_FIELD_VAL 256
#define WF_JSEV_DOM 1
#define WF_JSEV_PREVENTED 2
#define WF_JSEV_VALUE 4
int wf_jspage_event_flags(void);
const char *wf_jspage_field_value(void);
int wf_jspage_event(const char *type, const char *target, const char *arg, char *out, int out_max,
                    wf_jspage_stats_t *st);
/* one line per clickable element of the live page: "@N tag #id text" */
int wf_jspage_clickables(char *out, int out_max);
/* 1 when the live page listens to key events (window, document, body,
 * the focused element) */
int wf_jspage_wants_keys(void);
/* the node of a "?__wfjs=N" link (any URL holding it), or -1 */
int wf_jspage_mark_node(const char *href);

/* DOM only (host tests): parse and serialize without scripts */
int wf_jsdom_roundtrip(const char *html, int n, char *out, int out_max);

#endif
