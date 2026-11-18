/* wf_jshist.h - Lot 114: history and structuredClone of the page realm
 * (wf_jshist.c). Internal to the page pass (wf_jsdom.c sets them up after
 * wf_jsapi.c).
 *
 * history: length, state, scrollRestoration, pushState, replaceState,
 * back, forward and go over the entries of the current page only (the
 * reader's own back/forward list is not touched). pushState and
 * replaceState change location (and the base URL of fetch) in place and
 * refuse a URL of another origin (SecurityError). A traversal (back,
 * forward, go(n)) is a task after the current script: location moves,
 * popstate runs on the window with the entry's state, then hashchange
 * when only the fragment changed. go(0) (a reload) does nothing.
 * At most WF_JSHIST_ENTRIES entries, the oldest dropped.
 *
 * structuredClone (and the state of pushState) copies undefined, null,
 * booleans, numbers, strings, plain objects (own enumerable properties),
 * arrays (their elements and holes), Date, RegExp (source and flags),
 * Map, Set, the Error kinds (name and message) and Boolean / Number /
 * String objects, keeping shared and cyclic references. Functions, DOM
 * nodes, promises, generators and the platform objects (URL, FormData,
 * ...) throw a DataCloneError. At most WF_JSHIST_CLONE_OBJS objects per
 * clone and WF_JSHIST_CLONE_DEPTH levels (a RangeError past them);
 * prototypes, getters (read once, copied as values) and extra properties
 * of arrays are not kept, as in browsers except the last one. */
#ifndef WF_JSHIST_H
#define WF_JSHIST_H
#include "wf_jsi.h"

#define WF_JSHIST_ENTRIES 50
#define WF_JSHIST_CLONE_OBJS 4096
#define WF_JSHIST_CLONE_DEPTH 64

/* history and structuredClone on the global object; page_url is the
 * first entry. 0 or -1. */
int js_hist_init(js_vm_t *vm, const char *page_url);

/* structuredClone(v): the copy, or undefined with the exception thrown */
jv js_hist_clone(js_vm_t *vm, jv v);

/* from wf_jsdom.c */
int wf_jsdom_set_location(js_vm_t *vm, const char *url);
void wf_jsdom_window_event(js_vm_t *vm, const char *type, const char *k1, jv v1, const char *k2, jv v2);
int wf_jsdom_task(js_vm_t *vm, jv fn, jv arg);
#endif
