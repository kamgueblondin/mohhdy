/* wf_fetch.h - Lot 67: one HTTP(S) GET with redirects, over a pluggable
 * transport (Mohhdy sockets in the guest, POSIX sockets in host tests).
 * Shared by the shell `fetch` command and the GUI /browser. */
#ifndef WF_FETCH_H
#define WF_FETCH_H
#include <stdint.h>
#include "wf_http.h"
#include "wf_tls.h"
#include "wf_cookie.h"

#define WF_FETCH_MAX_REDIRECTS 5

typedef struct {
    void* net;
    /* resolves host to IPv4; 0 = ok */
    int (*resolve)(void* net, const char* host, uint8_t ip[4]);
    /* opens a TCP connection; returns a handle >= 0 or < 0 */
    int (*connect)(void* net, const uint8_t ip[4], int port);
    int (*send)(void* net, int h, const uint8_t* d, int n);
    int (*recv)(void* net, int h, uint8_t* d, int cap);   /* > 0, 0 = closed, < 0 error */
    void (*close)(void* net, int h);
    int (*rand)(void* net, uint8_t* out, int n);
    void (*log)(void* net, const char* line);              /* optional progress lines */
    const wf_trust_t* trust;
    const char* now;          /* YYYYMMDDHHMMSS or 0 (clock not trusted) */
    int max_redirects;        /* clamped to WF_FETCH_MAX_REDIRECTS */
    wf_cookie_jar_t* jar;     /* Lot 71: 0 = no cookies; else sent and stored on every hop */
} wf_fetch_env_t;

typedef struct {
    int status;               /* final HTTP status, 0 if none */
    char url[WF_URL_HOST_MAX + WF_URL_PATH_MAX + 16];   /* final URL */
    char content_type[128];
    int body_len;
    int truncated;
    int encoding;             /* Lot 69: Content-Encoding decoded (WF_ENC_*) */
    int wire_len;             /* body bytes on the wire (compressed) */
    int redirects;
    int cookies_set;          /* Lot 71: Set-Cookie stored over all hops */
    int cookies_sent;         /* cookies sent on the last hop */
    int https;
    uint8_t ip[4];
    /* TLS details of the final hop */
    char suite[40];
    char leaf_cn[64];
    char anchor_cn[64];
    char not_after[15];
    int time_checked;
    char err[160];
} wf_fetch_result_t;

/* 0 when a final response was received (any status), < 0 otherwise. */
int wf_fetch(const wf_fetch_env_t* env, const char* url, uint8_t* body, int cap, wf_fetch_result_t* res);

/* Lot 71: a request other than a plain GET. method "POST" sends body
 * (body_len bytes, content type ctype) on the first hop; a 301/302/303
 * answer is followed with a GET without body (as browsers do), 307/308
 * repeat the POST. extra: 0 or header lines each ending with CRLF (an
 * API key header for a configured search API); they are sent to the
 * first host only, never after a redirect to another host. */
typedef struct {
    const char* method;       /* 0 = "GET" */
    const char* body;
    int body_len;
    const char* ctype;
    const char* extra;
} wf_fetch_req_t;
int wf_fetch_ex(const wf_fetch_env_t* env, const char* url, const wf_fetch_req_t* req, uint8_t* body, int cap,
                wf_fetch_result_t* res);
#endif
