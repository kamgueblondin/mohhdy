/* wf_x509.h - Lot 67: DER / X.509 v3 certificate parsing and chain
 * validation for the Ring 3 TLS client. Bounded, no allocation: a parsed
 * certificate points into the caller's DER buffer. */
#ifndef WF_X509_H
#define WF_X509_H
#include <stdint.h>

/* ---- DER reader ---- */
typedef struct {
    uint8_t tag;
    const uint8_t* val;   /* content */
    int len;              /* content length */
    const uint8_t* raw;   /* tag byte, for whole-element spans */
    int raw_len;          /* header + content */
} wf_der_t;

/* Reads one element at d[*pos]; advances *pos. 0 or -1. */
int wf_der_next(const uint8_t* d, int len, int* pos, wf_der_t* out);
/* Same, but fails unless the tag matches. */
int wf_der_expect(const uint8_t* d, int len, int* pos, uint8_t tag, wf_der_t* out);

/* ---- certificate ---- */
#define WF_KEY_NONE 0
#define WF_KEY_RSA 1
#define WF_KEY_EC_P256 2
#define WF_KEY_EC_P384 3

#define WF_SIG_NONE 0
#define WF_SIG_RSA_SHA256 1
#define WF_SIG_RSA_SHA384 2
#define WF_SIG_RSA_SHA512 3
#define WF_SIG_ECDSA_SHA256 4
#define WF_SIG_ECDSA_SHA384 5
#define WF_SIG_ECDSA_SHA512 6
#define WF_SIG_RSA_PSS_SHA256 7
#define WF_SIG_RSA_PSS_SHA384 8
#define WF_SIG_RSA_PSS_SHA512 9
#define WF_SIG_SHA1_REFUSED 10   /* recognised, never trusted */

#define WF_X509_MAX_SAN 24

typedef struct {
    const uint8_t* der; int der_len;            /* whole certificate */
    const uint8_t* tbs; int tbs_len;            /* signed part */
    const uint8_t* issuer; int issuer_len;      /* raw Name SEQUENCE */
    const uint8_t* subject; int subject_len;
    char not_before[15];                        /* YYYYMMDDHHMMSS */
    char not_after[15];
    int key_type;
    const uint8_t* rsa_n; int rsa_n_len;
    const uint8_t* rsa_e; int rsa_e_len;
    const uint8_t* ec_point; int ec_point_len;  /* 04||X||Y */
    int sig_alg;                                /* outer signatureAlgorithm */
    const uint8_t* sig; int sig_len;            /* BIT STRING content (no unused byte) */
    int version;                                /* 1..3 */
    int is_ca;                                  /* basicConstraints cA */
    int path_len;                               /* -1 = no limit */
    int has_key_usage;
    int key_cert_sign;                          /* keyUsage bit 5 */
    int digital_signature;                      /* keyUsage bit 0 */
    int san_count;
    const uint8_t* san[WF_X509_MAX_SAN]; int san_len[WF_X509_MAX_SAN];   /* dNSName */
    int san_ip_count;
    uint8_t san_ip[4][4];
    char cn[64];                                /* subject CN (display only) */
    int unknown_critical;                       /* a critical extension we do not know */
} wf_cert_t;

/* 0 or < 0 (WF_X509_E_*). */
#define WF_X509_E_DER (-1)
#define WF_X509_E_VERSION (-2)
#define WF_X509_E_ALG (-3)
#define WF_X509_E_KEY (-4)
#define WF_X509_E_TIME (-5)
int wf_x509_parse(const uint8_t* der, int len, wf_cert_t* c);

/* Verifies that `issuer` signed `c` (signature algorithm and key). 0 = ok. */
int wf_x509_verify_signed_by(const wf_cert_t* c, const wf_cert_t* issuer);
/* Verifies a TLS ServerKeyExchange-style signature made with c's key.
 * sig_alg is a WF_SIG_* value, data is the signed message. 0 = ok. */
int wf_x509_verify_data(const wf_cert_t* c, int sig_alg, const uint8_t* data, int data_len,
                        const uint8_t* sig, int sig_len);

/* RFC 6125 host check against the dNSName SANs (one leftmost-label
 * wildcard, never on a public suffix-like single label); IPv4 literals
 * against iPAddress SANs. No CN fallback. 0 = match. */
int wf_x509_host_match(const wf_cert_t* c, const char* host);
/* Name equality on the raw DER (what the issuer field must equal). */
int wf_x509_name_eq(const uint8_t* a, int alen, const uint8_t* b, int blen);

/* ---- trust store and chain ---- */
#define WF_TRUST_MAX 48
typedef struct {
    int count;
    wf_cert_t roots[WF_TRUST_MAX];
    int skipped;   /* PEM blocks that did not parse */
} wf_trust_t;

/* Decodes every -----BEGIN CERTIFICATE----- block of pem into der_out
 * (bounded) and adds the roots. The store points into der_out, which must
 * outlive it. Returns the number added (>= 0). */
int wf_trust_add_pem(wf_trust_t* t, const char* pem, int pem_len, uint8_t* der_out, int der_cap, int* der_used);
/* Base64 decode of one block (whitespace skipped). Returns bytes or -1. */
int wf_base64_decode(const char* in, int in_len, uint8_t* out, int cap);

/* Chain result codes. */
#define WF_CHAIN_OK 0
#define WF_CHAIN_E_EMPTY (-10)
#define WF_CHAIN_E_PARSE (-11)
#define WF_CHAIN_E_HOST (-12)
#define WF_CHAIN_E_EXPIRED (-13)
#define WF_CHAIN_E_NOT_YET (-14)
#define WF_CHAIN_E_UNTRUSTED (-15)
#define WF_CHAIN_E_SIGNATURE (-16)
#define WF_CHAIN_E_NOT_CA (-17)
#define WF_CHAIN_E_PATHLEN (-18)
#define WF_CHAIN_E_DEPTH (-19)
#define WF_CHAIN_E_CRITICAL (-20)
#define WF_CHAIN_E_WEAK (-21)

#define WF_CHAIN_MAX 8
typedef struct {
    int depth;                 /* certificates used, anchor included */
    int anchor;                /* index in the trust store */
    int time_checked;          /* 1: validity checked against now */
    char leaf_cn[64];
    char anchor_cn[64];
    char not_after[15];
} wf_chain_info_t;

/* certs[0] is the leaf. now is YYYYMMDDHHMMSS UTC or 0 when the guest
 * clock is not trusted (then dates are not checked and time_checked = 0:
 * the caller must say so). host may be 0 (no host check). */
int wf_chain_verify(const wf_cert_t* certs, int count, const wf_trust_t* trust,
                    const char* host, const char* now, wf_chain_info_t* info);
const char* wf_chain_error(int rc);
#endif
