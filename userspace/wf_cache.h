/* wf_cache.h - Lot 73: a bounded in-memory HTTP cache for the guest
 * browser (RFC 9111, the parts a private single-user cache needs).
 *
 * Only plain GET answers with status 200 are stored, under their request
 * URL, when the response allows it:
 *  - Cache-Control: no-store (or Pragma: no-cache with no Cache-Control)
 *    is never stored; no-cache is stored but always revalidated;
 *    max-age=N (s-maxage is for shared caches and ignored) gives the
 *    freshness lifetime; must-revalidate / proxy-revalidate change
 *    nothing for a cache that never serves stale answers.
 *  - Without max-age the answer is stale at once (no heuristic
 *    freshness): it is kept only when it has a validator.
 *  - Validators: ETag (sent back as If-None-Match) and Last-Modified
 *    (sent back as If-Modified-Since). An answer with neither validator
 *    nor max-age > 0 is not stored.
 * A fresh entry is served without a request. A stale one is revalidated:
 * a 304 answer refreshes its lifetime and validators (headers of the
 * 304), and the stored body is served; a 200 replaces it.
 * Bounds: WF_CACHE_ENTRIES entries, WF_CACHE_POOL body bytes in all, one
 * body at most WF_CACHE_POOL / 4; the least recently used entries are
 * evicted. Time is in seconds of a caller clock (system ticks / 100).
 * Freestanding, no allocation; the state is caller owned. */
#ifndef WF_CACHE_H
#define WF_CACHE_H
#include <stdint.h>

#define WF_CACHE_ENTRIES 16
#define WF_CACHE_POOL (512 * 1024)
#define WF_CACHE_URL 300
#define WF_CACHE_TAG 96
#define WF_CACHE_DATE 40
#define WF_CACHE_CTYPE 128

enum { WF_CACHE_MISS = 0, WF_CACHE_FRESH = 1, WF_CACHE_STALE = 2 };

typedef struct {
    int store;                /* 0: no-store (or nothing to keep) */
    int no_cache;             /* stored, revalidated before every use */
    long max_age;             /* seconds, -1 none */
    char etag[WF_CACHE_TAG];
    char last_modified[WF_CACHE_DATE];
} wf_cache_meta_t;

typedef struct {
    int used;
    char url[WF_CACHE_URL];
    char ctype[WF_CACHE_CTYPE];
    wf_cache_meta_t meta;
    uint32_t stored_at;       /* seconds */
    uint32_t last_use;        /* LRU stamp */
    uint32_t off, len;        /* body bytes in the pool */
} wf_cache_entry_t;

typedef struct {
    wf_cache_entry_t e[WF_CACHE_ENTRIES];
    uint8_t pool[WF_CACHE_POOL];
    uint32_t pool_used;
    uint32_t stamp;
    uint32_t hits, revalidated, misses, stored, evicted, refused;
} wf_cache_t;

void wf_cache_init(wf_cache_t* c);
/* the cache policy of a response head (status line and header lines) */
void wf_cache_meta(const char* head, int len, wf_cache_meta_t* m);
/* entry index for url into *idx; WF_CACHE_FRESH, _STALE or _MISS */
int wf_cache_lookup(wf_cache_t* c, const char* url, uint32_t now, int* idx);
/* If-None-Match / If-Modified-Since lines (CRLF each) for entry idx into
 * out; returns the length (0: no validator), -1 when it does not fit */
int wf_cache_validators(const wf_cache_t* c, int idx, char* out, int cap);
/* stores a 200 answer; returns the entry index or -1 (refused) */
int wf_cache_store(wf_cache_t* c, const char* url, const wf_cache_meta_t* m, const char* ctype, const uint8_t* body,
                   int len, uint32_t now);
/* a 304 for entry idx: its new lifetime and validators from m */
void wf_cache_refresh(wf_cache_t* c, int idx, const wf_cache_meta_t* m, uint32_t now);
/* the body of entry idx copied into out (cap bytes); returns its length or -1 */
int wf_cache_body(const wf_cache_t* c, int idx, uint8_t* out, int cap);
/* drops url (a POST to it, or the user); 1 when it was there */
int wf_cache_drop(wf_cache_t* c, const char* url);
/* one line: entries, bytes, hits, revalidated, misses, stored, evicted, refused */
int wf_cache_text(const wf_cache_t* c, char* out, int cap);
#endif
