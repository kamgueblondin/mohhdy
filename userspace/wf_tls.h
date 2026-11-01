/* wf_tls.h - Lot 67: TLS 1.2 client for the Ring 3 web stack; Lot 69:
 * TLS 1.3 (TLS_AES_128_GCM_SHA256, X25519, RSA-PSS/ECDSA CertificateVerify)
 * is offered first and TLS 1.2 is the fallback.
 * Suites: ECDHE-ECDSA-AES128-GCM-SHA256 (c02b), ECDHE-RSA-AES128-GCM-SHA256
 * (c02f); key exchange X25519; extended master secret (RFC 7627) preferred, RFC 5246 master secret otherwise;
 * SNI; server certificate chain checked against a wf_trust_t root store
 * with host name and (when the clock is trusted) validity dates.
 * Lot 72: TLS 1.3 TLS_AES_256_GCM_SHA384 (1302) and
 * TLS_CHACHA20_POLY1305_SHA256 (1303); TLS 1.2 ECDHE-{ECDSA,RSA}-AES256-
 * GCM-SHA384 (c02c/c030) and ECDHE-{ECDSA,RSA}-CHACHA20-POLY1305 (cca9/
 * cca8), with SHA-384 transcript / PRF / HKDF for the SHA-384 suites
 * (wf_aead.c ciphers). P-256 ECDHE runs on the constant-time wf_p256.c.
 * Renegotiation, session resumption and client certificates are refused.
 * The transport is two callbacks, so the same code runs over Mohhdy
 * sockets and over host sockets in the tests. Bounded: one wf_tls_t
 * holds every buffer (no allocation). */
#ifndef WF_TLS_H
#define WF_TLS_H
#include <stdint.h>
#include "wf_x509.h"
#include "../kernel/sha256.h"
#include "wf_sha512.h"
#include "wf_aead.h"

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
    uint16_t only_suite;   /* Lot 72: 0 = offer every suite, else offer only this one (tests, tls-suite) */
    /* results */
    char err[128];
    int alert;             /* last alert received, -1 none */
    uint16_t suite;
    uint16_t sig_scheme;
    int chain_rc;
    wf_chain_info_t chain;
    int established;
    int ems;               /* 1: extended master secret (RFC 7627) was negotiated */
    int peer_closed;
    /* state */
    uint8_t client_random[32], server_random[32];
    uint8_t master[48];
    wf_aead_t c_aead, s_aead;     /* Lot 72: record ciphers (AES-128/256-GCM, ChaCha20-Poly1305) */
    uint8_t c_iv[12], s_iv[12];   /* TLS 1.2 GCM: 4 implicit bytes; ChaCha20 and TLS 1.3: 12 */
    uint64_t c_seq, s_seq;
    int aead;                     /* WF_AEAD_* of the suite */
    int hlen;                     /* 32 (SHA-256 suites) or 48 (SHA-384 suites) */
    /* TLS 1.3 (RFC 8446) with X25519 */
    int got_share;
    int version;                  /* 0x0303 or 0x0304 once the ServerHello is read */
    uint8_t c_secret[48], s_secret[48];   /* current traffic secrets */
    int s_encrypted;
    sha256_ctx_t hs_hash;         /* running transcript hashes: both until the suite is known */
    wf_sha512_ctx_t hs_hash384;
    uint8_t ecdh_priv[32];
    uint8_t ecdh_srv_pub[32];
    uint8_t ecdh_srv_p256[65];
    int group;                    /* 0x001d X25519 or 0x0017 P-256 */
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
/* Lot 72: suite id from a name ("TLS13-CHACHA20-POLY1305-SHA256", "c030",
 * "0xc030"); 0 if unknown. Number of suites offered; their ids. */
uint16_t wf_tls_suite_by_name(const char* name);
int wf_tls_suite_count(void);
uint16_t wf_tls_suite_at(int i);
/* TLS 1.2 PRF with SHA-256 (hlen 32) or SHA-384 (hlen 48). */
void wf_tls_prf_h(int hlen, const uint8_t* secret, int slen, const char* label, const uint8_t* seed, int seedlen,
                  uint8_t* out, int outlen);
void wf_hkdf_extract_h(int hlen, const uint8_t* salt, int sl, const uint8_t* ikm, int il, uint8_t* out);
void wf_hkdf_expand_label_h(int hlen, const uint8_t* secret, const char* label, const uint8_t* ctx, int cl,
                            uint8_t* out, int outlen);
/* TLS 1.3 HKDF helpers (exported for the RFC 8448 known-answer test). */
void wf_hkdf_extract(const uint8_t* salt, int sl, const uint8_t* ikm, int il, uint8_t out[32]);
void wf_hkdf_expand_label(const uint8_t secret[32], const char* label, const uint8_t* ctx, int cl,
                          uint8_t* out, int outlen);
#endif
