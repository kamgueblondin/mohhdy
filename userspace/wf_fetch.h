/* wf_fetch.h - Lot 67: one HTTP(S) GET with redirects, over a pluggable
 * transport (Mohhdy sockets in the guest, POSIX sockets in host tests).
 * Shared by the shell `fetch` command and the GUI /browser. */
#ifndef WF_FETCH_H
#define WF_FETCH_H
#include <stdint.h>
#include "wf_http.h"
#include "wf_tls.h"

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
} wf_fetch_env_t;

typedef struct {
    int status;               /* final HTTP status, 0 if none */
    char url[WF_URL_HOST_MAX + WF_URL_PATH_MAX + 16];   /* final URL */
    char content_type[128];
    int body_len;
    int truncated;
    int redirects;
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
#endif
