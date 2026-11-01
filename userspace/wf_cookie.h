/* wf_cookie.h - Lot 71: a bounded HTTP cookie jar (RFC 6265 subset) for
 * the Ring 3 web stack, so a form login or a consent page keeps working
 * across requests and redirects.
 * Kept: name, value, Domain (host-only when absent), Path (default path
 * of the request when absent), Secure, HttpOnly (stored, there is no
 * script to hide it from), SameSite (stored, not enforced: every request
 * here is a top-level navigation), Max-Age and Expires as "delete now"
 * when zero, negative or in the past (the guest has no trusted calendar
 * without a clock, so a future date makes a session cookie).
 * Refused: a Domain that does not domain-match the host, a Domain with no
 * dot inside (a top-level domain), names or values with control bytes,
 * a Secure cookie set over http. The jar lives in memory for the shell
 * session only; it is never written to /persist. At most WF_COOKIE_MAX
 * cookies, the least recently set one is evicted. Host tested
 * (tests/unit/userspace/test_wf_cookie.c). */
#ifndef WF_COOKIE_H
#define WF_COOKIE_H
#include <stdint.h>

#define WF_COOKIE_MAX 24
#define WF_COOKIE_NAME 48
#define WF_COOKIE_VALUE 320
#define WF_COOKIE_DOMAIN 96
#define WF_COOKIE_PATH 96
#define WF_COOKIE_HDR 2048    /* the Cookie: header we send */

typedef struct {
    char name[WF_COOKIE_NAME];
    char value[WF_COOKIE_VALUE];
    char domain[WF_COOKIE_DOMAIN];   /* lower case, no leading dot */
    char path[WF_COOKIE_PATH];
    uint32_t order;                  /* set order, for eviction */
    uint8_t used, host_only, secure, http_only, same_site;   /* 0 none 1 lax 2 strict 3 none */
} wf_cookie_t;

typedef struct {
    wf_cookie_t c[WF_COOKIE_MAX];
    uint32_t clock;
    uint32_t set, replaced, deleted, refused, evicted, sent;
    int enabled;                      /* 0: neither stored nor sent */
} wf_cookie_jar_t;

void wf_cookie_jar_init(wf_cookie_jar_t* j);
/* one Set-Cookie value received from host for request path (the path
 * part of the URL, query allowed). now: YYYYMMDDHHMMSS or 0. Returns 1
 * stored, 2 deleted, 0 ignored, -1 refused. */
int wf_cookie_set(wf_cookie_jar_t* j, const char* host, const char* req_path, int https, const char* value,
                  const char* now);
/* every Set-Cookie line of a raw response head (hdr, n bytes); returns the
 * number stored */
int wf_cookie_from_head(wf_cookie_jar_t* j, const char* host, const char* req_path, int https, const char* hdr,
                        int n, const char* now);
/* "Cookie: a=1; b=2\r\n" for a request, longest paths first; "" when no
 * cookie matches. Returns the length, -1 when cap is too small. */
int wf_cookie_header(wf_cookie_jar_t* j, const char* host, const char* req_path, int https, char* out, int cap);
/* number of cookies kept */
int wf_cookie_count(const wf_cookie_jar_t* j);
/* removes the cookies of a domain (and its subdomains), all when 0 or "" */
int wf_cookie_clear(wf_cookie_jar_t* j, const char* domain);
/* one line per cookie, values masked: "domain path name len=N secure host-only" */
void wf_cookie_list(const wf_cookie_jar_t* j, char* out, int cap);
#endif
