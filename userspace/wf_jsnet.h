/* wf_jsnet.h - Lot 114: fetch(), Headers, Response and XMLHttpRequest of
 * the page realm (wf_jsnet.c), over the network hook of the page pass
 * (wf_jspage_set_fetch in wf_jsdom.h). Internal to the page pass.
 *
 * Bounds and limits (said in docs/lot114_plan.md): same origin only (no
 * CORS: a cross-origin request fails as a network error, with a log
 * line), WF_DOM_NET_REQS requests per pass, WF_DOM_NET_BYTES per answer
 * (a longer one fails), GET and POST with a string body, only the
 * Content-Type request header is sent, the answer headers are
 * Content-Type alone, no streams, Blob, ArrayBuffer or FormData bodies.
 * The request is made when fetch() or send() is called; the answer is
 * delivered as a task (a 0 ms timer) after the current script, so
 * .then() handlers and onload run after it, in request order. A
 * synchronous XMLHttpRequest (open(m, u, false)) answers in send(). */
#ifndef WF_JSNET_H
#define WF_JSNET_H

#include "wf_jsi.h"
#include "wf_jsdom.h"

/* fetch, Headers, Response and XMLHttpRequest on the global object */
int js_net_init(js_vm_t *vm);

/* from wf_jsdom.c */
int wf_jsdom_resolve(const char *href, char *out, int cap);
int wf_jsdom_same_origin(const char *abs);
int wf_jsdom_request(wf_jsreq_t *r);
int wf_jsdom_task(js_vm_t *vm, jv fn, jv arg);
wf_jspage_stats_t *wf_jsdom_stats(void);

#endif
