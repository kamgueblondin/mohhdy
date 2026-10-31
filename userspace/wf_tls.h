/* wf_tls.h - Lot 67: TLS 1.2 client for the Ring 3 web stack.
 * Suites: ECDHE-ECDSA-AES128-GCM-SHA256 (c02b), ECDHE-RSA-AES128-GCM-SHA256
 * (c02f); key exchange X25519; extended master secret (RFC 7627) required;
 * SNI; server certificate chain checked against a wf_trust_t root store
 * with host name and (when the clock is trusted) validity dates.
 * Renegotiation, session resumption and client certificates are refused.
 * The transport is two callbacks, so the same code runs over Mohhdy
 * sockets and over host sockets in the tests. Bounded: one wf_tls_t
 * holds every buffer (no allocation). */
#ifndef WF_TLS_H
#define WF_TLS_H
#include <stdint.h>
#include "wf_x509.h"
#include "../kernel/sha256.h"

/* send: returns bytes accepted (> 0) or < 0. recv: > 0 bytes, 0 = peer
 * closed, < 0 = error/timeout. */
typedef int (*wf_send_fn)(void* io, const uint8_t* data, int len);
typedef int (*wf_recv_fn)(void* io, uint8_t* data, int cap);
/* Fills out[len] with random bytes; returns 0. */
typedef int (*wf_rand_fn)(void* io, uint8_t* out, int len);

#define WF_TLS_REC_MAX (16384 + 2048)
#define WF_TLS_HS_MAX (40 * 1024)
#define WF_TLS_CERT_STORE (24 * 1024)

typedef struct {
    /* caller fills */
    void* io;
    wf_send_fn send;
    wf_recv_fn recv;
    wf_rand_fn rand;
    const wf_trust_t* trust;
    const char* host;      /* SNI and certificate host name */
    const char* now;       /* YYYYMMDDHHMMSS UTC, or 0 if the clock is not trusted */
    /* results */
    char err[128];
    int alert;             /* last alert received, -1 none */
    uint16_t suite;
    uint16_t sig_scheme;
    int chain_rc;
    wf_chain_info_t chain;
    int established;
    int peer_closed;
    /* state */
    uint8_t client_random[32], server_random[32];
    uint8_t master[48];
    uint8_t c_key[16], s_key[16], c_iv[4], s_iv[4];
    uint64_t c_seq, s_seq;
    int s_encrypted;
    sha256_ctx_t hs_hash;         /* running transcript hash */
    uint8_t ecdh_priv[32];
    uint8_t ecdh_srv_pub[32];
    int cert_count;
    wf_cert_t certs[WF_CHAIN_MAX];
    uint8_t cert_store[WF_TLS_CERT_STORE];
    /* record layer */
    uint8_t rec[WF_TLS_REC_MAX];
    int rec_len;                  /* bytes in rec (raw, maybe partial) */
    uint8_t plain[16384 + 256];
    int plain_off, plain_len;
    uint8_t hs[WF_TLS_HS_MAX];
    int hs_len;
    uint8_t out[16384 + 64];
} wf_tls_t;

/* Fields set by the caller survive; the state is reset. 0 = established. */
int wf_tls_handshake(wf_tls_t* t);
/* Returns len or < 0. */
int wf_tls_write(wf_tls_t* t, const uint8_t* data, int len);
/* > 0 bytes, 0 = clean close (close_notify or EOF after data), < 0 error. */
int wf_tls_read(wf_tls_t* t, uint8_t* data, int cap);
/* Sends close_notify (best effort). */
void wf_tls_close(wf_tls_t* t);

/* TLS 1.2 PRF with SHA-256 (exported for the known-answer test). */
void wf_tls_prf(const uint8_t* secret, int slen, const char* label, const uint8_t* seed, int seedlen,
                uint8_t* out, int outlen);
const char* wf_tls_suite_name(uint16_t suite);
#endif
