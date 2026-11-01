/* wf_http.h - Lot 67: URL parsing and an incremental HTTP/1.1 response
 * parser (status, headers, Content-Length, chunked, close-delimited) for
 * the Ring 3 web stack. Bounded buffers, no allocation. */
#ifndef WF_HTTP_H
#define WF_HTTP_H
#include <stdint.h>

#define WF_URL_HOST_MAX 256
#define WF_URL_PATH_MAX 1024

typedef struct {
    int https;
    char host[WF_URL_HOST_MAX];
    int port;
    char path[WF_URL_PATH_MAX];   /* path + query, always starts with '/' */
} wf_url_t;

/* "http://h[:p]/path?q", "https://...", or a bare "host/path" (https
 * assumed). Fragments are dropped. Userinfo, IPv6 literals and other
 * schemes are refused. 0 or -1. */
int wf_url_parse(const char* s, wf_url_t* u);
/* Resolves a Location value (absolute, scheme-relative "//h/p",
 * absolute-path "/p" or relative "p") against base. 0 or -1. */
int wf_url_resolve(const wf_url_t* base, const char* loc, wf_url_t* out);
/* Writes "scheme://host[:port]path" into out (bounded). Returns length. */
int wf_url_format(const wf_url_t* u, char* out, int cap);

/* Builds the GET request. Returns length or -1. */
int wf_http_build_get(const wf_url_t* u, char* out, int cap);
/* Lot 71: request head for method ("GET" / "POST"); body_len >= 0 adds
 * Content-Type (ctype, default urlencoded) and Content-Length; extra is
 * 0 or header lines each ending with CRLF. -1 on overflow or CR/LF in a
 * header value. The body is sent after the head by the caller. */
int wf_http_build_request(const wf_url_t* u, const char* method, const char* ctype, int body_len, const char* extra,
                          char* out, int cap);

#define WF_HTTP_HDR_MAX (32 * 1024)   /* Lot 69: youtube.com sends ~10 KiB of headers */
#define WF_HTTP_LOC_MAX 1024

typedef struct {
    /* caller: body destination */
    uint8_t* body;
    int body_cap;
    /* results */
    int status;
    char reason[64];
    char location[WF_HTTP_LOC_MAX];
    char content_type[128];
    long content_length;     /* -1 unknown */
    int chunked;
    int content_encoding;    /* Lot 69: WF_ENC_* (0 identity, 1 gzip, 2 deflate) */
    int body_len;
    int truncated;           /* body larger than body_cap: rest discarded */
    int done;                /* full message received */
    int error;               /* < 0: protocol error */
    const char* error_text;
    /* parser state */
    int state;
    char hdr[WF_HTTP_HDR_MAX];
    int hdr_len;
    long remaining;          /* bytes left in body or chunk */
    char line[64];
    int line_len;
    long total_body;         /* bytes of body seen (incl. discarded) */
} wf_http_resp_t;

void wf_http_resp_init(wf_http_resp_t* r, uint8_t* body, int cap);
/* Feeds bytes. Returns 0 (continue), 1 (message complete) or < 0. */
int wf_http_feed(wf_http_resp_t* r, const uint8_t* d, int n);
/* The peer closed: completes a close-delimited body. Returns 1 if the
 * message is complete, < 0 if it was cut short. */
int wf_http_eof(wf_http_resp_t* r);
#endif
