/* wf_jsapi.h - Lot 114: the web APIs of the page realm beside the DOM
 * and the network (wf_jsapi.c): URL and URLSearchParams, FormData,
 * localStorage and sessionStorage, AbortController and AbortSignal,
 * DOMException. Internal to the page pass (wf_jsdom.c sets them up).
 *
 * Bounds and limits (said in docs/lot114_plan.md):
 * - URL follows the WHATWG parser for the common cases: special schemes
 *   (http, https, ws, wss, ftp, file) with an authority, other schemes
 *   with an authority or an opaque path, relative references against a
 *   base, dot segments, default ports, percent-encoding of the bytes the
 *   standard encodes. No IDNA (hosts are lowercased ASCII), no IPv6 or
 *   IPv4 number forms beyond copying them, at most WF_JSAPI_URL_MAX bytes.
 * - localStorage and sessionStorage live in the shell process (a static
 *   store of WF_JSAPI_STORE_BYTES for keys and values, WF_JSAPI_STORE_ITEMS
 *   items, WF_JSAPI_STORE_ORIGINS origins, the oldest origin dropped for
 *   a new one): they last across pages until the shell ends, never on
 *   disk. Items are read with getItem/setItem/removeItem/key/clear and
 *   length (no named properties). A full store throws QuotaExceededError.
 * - FormData holds strings (no Blob or File); a FormData body is sent as
 *   multipart/form-data, a URLSearchParams body as urlencoded.
 * - entries(), keys() and values() return arrays (the guest has no
 *   iterator objects), which for-of and spread take. */
#ifndef WF_JSAPI_H
#define WF_JSAPI_H

#include "wf_jsi.h"

#define WF_JSAPI_URL_MAX 2048
#define WF_JSAPI_STORE_BYTES 32768
#define WF_JSAPI_STORE_ITEMS 256
#define WF_JSAPI_STORE_ORIGINS 8
#define WF_JSAPI_BOUNDARY "----mohhdyFormBoundary114"

/* the constructors and localStorage/sessionStorage on the global object;
 * page_url gives the storage origin. 0 or -1. */
int js_api_init(js_vm_t *vm, const char *page_url);

/* a fetch or XMLHttpRequest body: a URLSearchParams (urlencoded) or a
 * FormData (multipart) as its string and content type; 1 when body was
 * one of them, 0 otherwise (the caller converts it to a string) */
int js_api_body(js_vm_t *vm, jv body, uint32_t *str, const char **ctype);

/* an AbortSignal: 1 when it is aborted (*reason set), 0 when not or not
 * a signal */
int js_api_aborted(js_vm_t *vm, jv signal, jv *reason);

/* the storage store, for tests: forget every item */
void js_api_store_reset(void);

/* from wf_jsdom.c: the named controls of a form element as [name, value]
 * pairs pushed onto list (submit buttons, files and disabled controls
 * left out, boxes only when checked); 0, or -1 when form is not a form */
int wf_jsdom_form_pairs(js_vm_t *vm, jv form, uint32_t list);

#endif
