/* wf_store.h - Lot 86: text snapshots of the cookie jar and the HTTP cache.
 *
 * The guest keeps both in memory. save writes a bounded text the caller
 * can put in /persist; load replaces the memory copy. Cookie values are
 * in the file (it is the operator's disk, not a log). Cache bodies over
 * WF_STORE_BODY bytes, or that would not fit in cap, are left out: the
 * memory cache still holds them until the process ends. No heap. */
#ifndef WF_STORE_H
#define WF_STORE_H
#include "wf_cookie.h"
#include "wf_cache.h"

#define WF_STORE_BODY 1536

/* Bytes written, or -1 when cap is too small for the header. */
int wf_store_cookies_save(const wf_cookie_jar_t *j, char *out, int cap);
/* 0 replaces *j. -1 leaves *j unchanged. */
int wf_store_cookies_load(wf_cookie_jar_t *j, const char *text, int len);

int wf_store_cache_save(const wf_cache_t *c, char *out, int cap);
int wf_store_cache_load(wf_cache_t *c, const char *text, int len);
#endif
