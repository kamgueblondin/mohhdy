/* wf_tls.c - Lot 67: TLS 1.2 client (see wf_tls.h). */
#include "wf_tls.h"
#include "../kernel/sha256.h"
#include "../kernel/aes_gcm.h"
#include "../kernel/x25519.h"
#include "wf_ec.h"

#define CT_CCS 20
#define CT_ALERT 21
#define CT_HS 22
#define CT_APP 23

#define HS_CLIENT_HELLO 1
#define HS_SERVER_HELLO 2
#define HS_CERTIFICATE 11
#define HS_SERVER_KEY_EXCHANGE 12
#define HS_CERTIFICATE_REQUEST 13
#define HS_SERVER_HELLO_DONE 14
#define HS_CLIENT_KEY_EXCHANGE 16
#define HS_FINISHED 20

static void mcpy(uint8_t* d, const uint8_t* s, int n) { int i; for (i = 0; i < n; i++) d[i] = s[i]; }
static void mset(uint8_t* d, uint8_t v, int n) { int i; for (i = 0; i < n; i++) d[i] = v; }
static int slen(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static int ct_eq(const uint8_t* a, const uint8_t* b, int n) {
    uint8_t d = 0; int i;
    for (i = 0; i < n; i++) d |= (uint8_t)(a[i] ^ b[i]);
    return d == 0;
}
static void put16(uint8_t* p, int v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void put24(uint8_t* p, int v) { p[0] = (uint8_t)(v >> 16); p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)v; }
static int get16(const uint8_t* p) { return (p[0] << 8) | p[1]; }
static int get24(const uint8_t* p) { return (p[0] << 16) | (p[1] << 8) | p[2]; }

static void seterr(wf_tls_t* t, const char* a, const char* b) {
    int n = 0, i;
    for (i = 0; a && a[i] && n < 126; i++) t->err[n++] = a[i];
    for (i = 0; b && b[i] && n < 126; i++) t->err[n++] = b[i];
    t->err[n] = 0;
}

const char* wf_tls_suite_name(uint16_t s) {
    if (s == 0xc02bU) return "ECDHE-ECDSA-AES128-GCM-SHA256";
    if (s == 0xc02fU) return "ECDHE-RSA-AES128-GCM-SHA256";
    if (s == 0x1301U) return "TLS13-AES128-GCM-SHA256";
    return "none";
}

/* ---------------------------------------------------------------- PRF */

void wf_tls_prf(const uint8_t* secret, int sl, const char* label, const uint8_t* seed, int seedlen,
                uint8_t* out, int outlen) {
    uint8_t ls[160], a[32 + 160], h[32];
    int ll = slen(label), lsl, done = 0, i;
    if (ll + seedlen > 160) return;
    mcpy(ls, (const uint8_t*)label, ll);
    mcpy(ls + ll, seed, seedlen);
    lsl = ll + seedlen;
    hmac_sha256(secret, (uint32_t)sl, ls, (uint32_t)lsl, a);        /* A(1) */
    while (done < outlen) {
        mcpy(a + 32, ls, lsl);
        hmac_sha256(secret, (uint32_t)sl, a, (uint32_t)(32 + lsl), h);
        for (i = 0; i < 32 && done < outlen; i++) out[done++] = h[i];
        hmac_sha256(secret, (uint32_t)sl, a, 32, a);                 /* A(i+1) */
    }
}

/* ---------------------------------------------------------------- HKDF (TLS 1.3) */

void wf_hkdf_extract(const uint8_t* salt, int sl, const uint8_t* ikm, int il, uint8_t out[32]) {
    static const uint8_t zero[32] = {0};
    if (!salt || sl == 0) { salt = zero; sl = 32; }
    hmac_sha256(salt, (uint32_t)sl, ikm, (uint32_t)il, out);
}

void wf_hkdf_expand_label(const uint8_t secret[32], const char* label, const uint8_t* ctx, int cl,
                          uint8_t* out, int outlen) {
    uint8_t info[2 + 1 + 6 + 64 + 1 + 64 + 1], t[32], m[32 + sizeof(info)];
    int ll = slen(label), il = 0, done = 0, tl = 0, i;
    uint8_t ctr = 1;
    if (ll > 64 || cl > 64 || outlen > 255 * 32) return;
    put16(info, outlen); il = 2;
    info[il++] = (uint8_t)(6 + ll);
    mcpy(info + il, (const uint8_t*)"tls13 ", 6); il += 6;
    mcpy(info + il, (const uint8_t*)label, ll); il += ll;
    info[il++] = (uint8_t)cl;
    if (cl) mcpy(info + il, ctx, cl);
    il += cl;
    while (done < outlen) {
        mcpy(m, t, tl);
        mcpy(m + tl, info, il);
        m[tl + il] = ctr++;
        hmac_sha256(secret, 32, m, (uint32_t)(tl + il + 1), t);
        tl = 32;
        for (i = 0; i < 32 && done < outlen; i++) out[done++] = t[i];
    }
}

static void derive_secret(const uint8_t secret[32], const char* label, const uint8_t hash[32], uint8_t out[32]) {
    wf_hkdf_expand_label(secret, label, hash, 32, out, 32);
}

static void tls13_keys(const uint8_t secret[32], uint8_t key[16], uint8_t iv[12]) {
    wf_hkdf_expand_label(secret, "key", 0, 0, key, 16);
    wf_hkdf_expand_label(secret, "iv", 0, 0, iv, 12);
}

static void nonce13(const uint8_t iv[12], uint64_t seq, uint8_t n[12]) {
    int i;
    for (i = 0; i < 12; i++) n[i] = iv[i];
    for (i = 11; i >= 4; i--) { n[i] ^= (uint8_t)seq; seq >>= 8; }
}

/* ---------------------------------------------------------------- I/O */

static int send_all(wf_tls_t* t, const uint8_t* d, int n) {
    int off = 0, r;
    while (off < n) {
        r = t->send(t->io, d + off, n - off);
        if (r <= 0) { seterr(t, "send failed", 0); return -1; }
        off += r;
    }
    return 0;
}

static void seq_bytes(uint64_t s, uint8_t o[8]) {
    int i;
    for (i = 7; i >= 0; i--) { o[i] = (uint8_t)s; s >>= 8; }
}


static int send_record(wf_tls_t* t, int type, const uint8_t* body, int n, int enc) {
    uint8_t* o = t->out;
    if (n > 16384) return -1;
    o[0] = (uint8_t)type; o[1] = 3; o[2] = 3;
    if (!enc) {
        if (type == CT_HS && !t->version) o[2] = 1;   /* ClientHello record: legacy 0x0301 like every browser */
        put16(o + 3, n);
        mcpy(o + 5, body, n);
        return send_all(t, o, 5 + n);
    } else if (t->version == 0x0304) {
        /* TLSInnerPlaintext: content || type, outer type application_data */
        uint8_t n12[12], aad[5], tag[16];
        static uint8_t inner[16384 + 1];
        mcpy(inner, body, n);
        inner[n] = (uint8_t)type;
        o[0] = CT_APP;
        put16(o + 3, n + 1 + 16);
        mcpy(aad, o, 5);
        nonce13(t->c_iv13, t->c_seq, n12);
        if (aes128_gcm_encrypt(t->c_key, n12, n12 + 4, aad, 5, inner, (uint16_t)(n + 1), o + 5, tag) != 0) return -1;
        mcpy(o + 5 + n + 1, tag, 16);
        t->c_seq++;
        return send_all(t, o, 5 + n + 1 + 16);
    } else {
        uint8_t nonce[8], aad[13], tag[16];
        seq_bytes(t->c_seq, nonce);
        mcpy(aad, nonce, 8);
        aad[8] = (uint8_t)type; aad[9] = 3; aad[10] = 3; put16(aad + 11, n);
        put16(o + 3, 8 + n + 16);
        mcpy(o + 5, nonce, 8);
        if (aes128_gcm_encrypt(t->c_key, t->c_iv, nonce, aad, 13, body, (uint16_t)n, o + 13, tag) != 0) return -1;
        mcpy(o + 13 + n, tag, 16);
        t->c_seq++;
        return send_all(t, o, 5 + 8 + n + 16);
    }
}

/* Reads one full record into t->plain (decrypted). Returns type or < 0. */
static int read_record(wf_tls_t* t) {
    int need, r, len, type;
    for (;;) {
        if (t->rec_len >= 5) {
            len = get16(t->rec + 3);
            if (t->rec[1] != 3 || len > 16384 + 2048) { seterr(t, "bad record header", 0); return -1; }
            need = 5 + len;
            if (t->rec_len >= need) break;
        }
        r = t->recv(t->io, t->rec + t->rec_len, WF_TLS_REC_MAX - t->rec_len);
        if (r == 0) { t->peer_closed = 1; seterr(t, "connection closed by peer", 0); return -2; }
        if (r < 0) { seterr(t, "receive failed (timeout or reset)", 0); return -1; }
        t->rec_len += r;
    }
    type = t->rec[0];
    len = get16(t->rec + 3);
    if (t->version == 0x0304 && t->s_encrypted && type == CT_APP) {
        uint8_t n12[12];
        int pl = len - 16;
        if (pl < 1 || pl > 16384 + 256) { seterr(t, "bad encrypted record", 0); return -1; }
        nonce13(t->s_iv13, t->s_seq, n12);
        if (aes128_gcm_decrypt(t->s_key, n12, n12 + 4, t->rec, 5, t->rec + 5, (uint16_t)pl, t->rec + 5 + pl, t->plain) != 0) {
            seterr(t, "record authentication failed (bad_record_mac)", 0);
            return -1;
        }
        t->s_seq++;
        while (pl > 0 && t->plain[pl - 1] == 0) pl--;    /* padding */
        if (pl < 1) { seterr(t, "record without content type", 0); return -1; }
        type = t->plain[pl - 1];
        t->plain_len = pl - 1;
    } else if (!t->s_encrypted || type == CT_CCS) {
        if (len > (int)sizeof(t->plain)) { seterr(t, "record too long", 0); return -1; }
        mcpy(t->plain, t->rec + 5, len);
        t->plain_len = len;
    } else {
        uint8_t aad[13];
        int pl = len - 8 - 16;
        if (pl < 0 || pl > 16384) { seterr(t, "bad encrypted record", 0); return -1; }
        seq_bytes(t->s_seq, aad);
        aad[8] = (uint8_t)type; aad[9] = 3; aad[10] = 3; put16(aad + 11, pl);
        if (aes128_gcm_decrypt(t->s_key, t->s_iv, t->rec + 5, aad, 13, t->rec + 13, (uint16_t)pl, t->rec + 13 + pl,
                               t->plain) != 0) {
            seterr(t, "record authentication failed (bad_record_mac)", 0);
            return -1;
        }
        t->s_seq++;
        t->plain_len = pl;
    }
    t->plain_off = 0;
    /* shift the remainder */
    {
        int i, rest = t->rec_len - (5 + len);
        for (i = 0; i < rest; i++) t->rec[i] = t->rec[5 + len + i];
        t->rec_len = rest;
    }
    if (type == CT_ALERT) {
        if (t->plain_len >= 2) {
            t->alert = t->plain[1];
            if (t->plain[1] == 0) { t->peer_closed = 1; return CT_ALERT; }
        }
        seterr(t, "alert from server", 0);
        {
            static const char hex[] = "0123456789abcdef";
            int n = slen(t->err);
            if (n < 120 && t->alert >= 0) {
                t->err[n++] = ' '; t->err[n++] = '#';
                t->err[n++] = hex[(t->alert >> 4) & 15]; t->err[n++] = hex[t->alert & 15]; t->err[n] = 0;
            }
        }
        return -3;
    }
    return type;
}

/* ---------------------------------------------------------------- handshake */

static void hs_hash(wf_tls_t* t, const uint8_t* d, int n) { sha256_update(&t->hs_hash, d, (uint32_t)n); }

static void hs_snapshot(wf_tls_t* t, uint8_t out[32]) {
    sha256_ctx_t copy = t->hs_hash;
    sha256_final(&copy, out);
}

static int send_hs(wf_tls_t* t, int type, const uint8_t* body, int n, int enc) {
    static uint8_t m[4 + 2048];
    if (n > 2048) return -1;
    m[0] = (uint8_t)type; put24(m + 1, n);
    mcpy(m + 4, body, n);
    hs_hash(t, m, 4 + n);
    return send_record(t, CT_HS, m, 4 + n, enc);
}

/* Returns the next handshake message (type, body in t->hs at *body). */
static int next_hs(wf_tls_t* t, int* body_off, int* body_len, int* consumed) {
    for (;;) {
        if (t->hs_len >= 4) {
            int l = get24(t->hs + 1);
            if (l > WF_TLS_HS_MAX - 4) { seterr(t, "handshake message too long", 0); return -1; }
            if (t->hs_len >= 4 + l) {
                *body_off = 4; *body_len = l; *consumed = 4 + l;
                hs_hash(t, t->hs, 4 + l);
                return t->hs[0];
            }
        }
        {
            int type = read_record(t);
            if (type < 0) return -1;
            if (type == CT_ALERT) { seterr(t, "close_notify during handshake", 0); return -1; }
            if (type == CT_CCS && t->version == 0x0304 && t->plain_len == 1 && t->plain[0] == 1) continue;   /* compat CCS */
            if (type != CT_HS) { seterr(t, "unexpected record during handshake", 0); return -1; }
            if (t->hs_len + t->plain_len > WF_TLS_HS_MAX) { seterr(t, "handshake too large", 0); return -1; }
            mcpy(t->hs + t->hs_len, t->plain, t->plain_len);
            t->hs_len += t->plain_len;
        }
    }
}

static void hs_consume(wf_tls_t* t, int n) {
    int i;
    for (i = 0; i + n < t->hs_len; i++) t->hs[i] = t->hs[i + n];
    t->hs_len -= n;
}

static int build_client_hello(wf_tls_t* t, uint8_t* b, const uint8_t my_pub[32], const uint8_t sid[32]) {
    static const uint8_t sigalgs[] = {0x04,0x03, 0x05,0x03, 0x08,0x04, 0x08,0x05, 0x08,0x06, 0x04,0x01, 0x05,0x01, 0x06,0x01};
    int p = 0, hl = slen(t->host), ext_start, i;
    b[p++] = 3; b[p++] = 3;
    mcpy(b + p, t->client_random, 32); p += 32;
    b[p++] = 32;                                /* legacy_session_id (RFC 8446 D.4 middlebox compatibility) */
    mcpy(b + p, sid, 32); p += 32;
    put16(b + p, 8); p += 2;
    b[p++] = 0x13; b[p++] = 0x01;               /* TLS_AES_128_GCM_SHA256 (TLS 1.3) */
    b[p++] = 0xc0; b[p++] = 0x2b; b[p++] = 0xc0; b[p++] = 0x2f;
    b[p++] = 0x00; b[p++] = 0xff;               /* renegotiation SCSV */
    b[p++] = 1; b[p++] = 0;                     /* null compression */
    ext_start = p; p += 2;
    if (hl > 0 && hl < 256) {                   /* server_name */
        put16(b + p, 0); put16(b + p + 2, hl + 5); put16(b + p + 4, hl + 3);
        b[p + 6] = 0; put16(b + p + 7, hl); p += 9;
        for (i = 0; i < hl; i++) b[p++] = (uint8_t)t->host[i];
    }
    /* supported_groups: X25519 is the only key exchange; P-256/P-384 are
     * listed because a TLS 1.2 server only uses an ECDSA certificate on a
     * curve the client names here (RFC 8422 5.1.1). A server that picks
     * them for ECDHE is refused with a clear error. */
    put16(b + p, 0x000a); put16(b + p + 2, 8); put16(b + p + 4, 6);
    put16(b + p + 6, 0x001d); put16(b + p + 8, 0x0017); put16(b + p + 10, 0x0018); p += 12;
    put16(b + p, 0x000b); put16(b + p + 2, 2); b[p + 4] = 1; b[p + 5] = 0; p += 6;                     /* uncompressed */
    put16(b + p, 0x000d); put16(b + p + 2, (int)sizeof(sigalgs) + 2); put16(b + p + 4, (int)sizeof(sigalgs)); p += 6;
    mcpy(b + p, sigalgs, (int)sizeof(sigalgs)); p += (int)sizeof(sigalgs);
    put16(b + p, 0x0017); put16(b + p + 2, 0); p += 4;                                                  /* EMS */
    put16(b + p, 0xff01); put16(b + p + 2, 1); b[p + 4] = 0; p += 5;                                    /* renegotiation_info */
    /* ALPN http/1.1: the stack speaks HTTP/1.1 only */
    put16(b + p, 0x0010); put16(b + p + 2, 11); put16(b + p + 4, 9); b[p + 6] = 8; p += 7;
    mcpy(b + p, (const uint8_t*)"http/1.1", 8); p += 8;
    /* TLS 1.3: supported_versions, psk_key_exchange_modes, key_share X25519 */
    put16(b + p, 0x002b); put16(b + p + 2, 5); b[p + 4] = 4; put16(b + p + 5, 0x0304); put16(b + p + 7, 0x0303); p += 9;
    put16(b + p, 0x002d); put16(b + p + 2, 2); b[p + 4] = 1; b[p + 5] = 1; p += 6;
    put16(b + p, 0x0033); put16(b + p + 2, 38); put16(b + p + 4, 36); put16(b + p + 6, 0x001d); put16(b + p + 8, 32); p += 10;
    mcpy(b + p, my_pub, 32); p += 32;
    put16(b + ext_start, p - ext_start - 2);
    return p;
}

static int scheme_to_sig(int s) {
    switch (s) {
    case 0x0403: return WF_SIG_ECDSA_SHA256;
    case 0x0503: return WF_SIG_ECDSA_SHA384;
    case 0x0603: return WF_SIG_ECDSA_SHA512;
    case 0x0401: return WF_SIG_RSA_SHA256;
    case 0x0501: return WF_SIG_RSA_SHA384;
    case 0x0601: return WF_SIG_RSA_SHA512;
    case 0x0804: return WF_SIG_RSA_PSS_SHA256;
    case 0x0805: return WF_SIG_RSA_PSS_SHA384;
    case 0x0806: return WF_SIG_RSA_PSS_SHA512;
    default: return WF_SIG_NONE;
    }
}

static int parse_server_hello(wf_tls_t* t, const uint8_t* b, int n) {
    int p = 0, sid, ems = 0, reneg = 0;
    if (n < 38 || b[0] != 3 || b[1] != 3) { seterr(t, "server is not TLS 1.2", 0); return -1; }
    mcpy(t->server_random, b + 2, 32);
    p = 34;
    sid = b[p++];
    if (sid > 32 || p + sid + 3 > n) return -1;
    p += sid;
    t->suite = (uint16_t)get16(b + p); p += 2;
    if (t->suite != 0xc02bU && t->suite != 0xc02fU && t->suite != 0x1301U) { seterr(t, "server chose a suite we did not offer", 0); return -1; }
    if (b[p++] != 0) { seterr(t, "compression refused", 0); return -1; }
    if (p + 2 <= n) {
        int el = get16(b + p), e;
        p += 2;
        if (p + el > n) return -1;
        e = p;
        while (e + 4 <= p + el) {
            int et = get16(b + e), elen = get16(b + e + 2);
            if (e + 4 + elen > p + el) return -1;
            if (et == 0x0017) ems = 1;
            if (et == 0x002b && elen == 2) t->version = get16(b + e + 4);
            if (et == 0x0033 && elen >= 4) {
                if (get16(b + e + 4) != 0x001d) { seterr(t, "server key share is not X25519", 0); return -1; }
                if (elen != 4 + 32 || get16(b + e + 6) != 32) { seterr(t, "bad key share", 0); return -1; }
                mcpy(t->ecdh_srv_pub, b + e + 8, 32);
                t->got_share = 1;
            }
            if (et == 0xff01) { if (elen != 1 || b[e + 4] != 0) { seterr(t, "bad renegotiation_info", 0); return -1; } reneg = 1; }
            e += 4 + elen;
        }
    }
    (void)reneg;
    {
        /* HelloRetryRequest: the special random (RFC 8446 4.1.3) */
        static const uint8_t hrr[8] = {0xcf, 0x21, 0xad, 0x74, 0xe5, 0x9a, 0x61, 0x11};
        if (ct_eq(t->server_random, hrr, 8)) { seterr(t, "server asked for a HelloRetryRequest (group other than X25519)", 0); return -1; }
    }
    if (t->version == 0x0304) {
        if (t->suite != 0x1301U) { seterr(t, "TLS 1.3 with a TLS 1.2 suite", 0); return -1; }
        if (!t->got_share) { seterr(t, "TLS 1.3 server sent no key share", 0); return -1; }
        return 0;
    }
    if (t->version != 0 && t->version != 0x0303) { seterr(t, "server picked an unknown version", 0); return -1; }
    t->version = 0x0303;
    if (t->suite == 0x1301U) { seterr(t, "TLS 1.3 suite without supported_versions", 0); return -1; }
    {
        /* downgrade sentinel: a TLS 1.3 server answering 1.2 to us is an attack */
        static const uint8_t dg[7] = {0x44, 0x4f, 0x57, 0x4e, 0x47, 0x52, 0x44};
        if (ct_eq(t->server_random + 24, dg, 7)) { seterr(t, "TLS downgrade detected", 0); return -1; }
    }
    /* RFC 7627 is preferred; a server without it (still common: badssl.com,
     * older load balancers) gets the RFC 5246 master secret, as every
     * mainstream browser does. The fetch report says which one was used. */
    t->ems = ems;
    return 0;
}

static int parse_certificates(wf_tls_t* t, const uint8_t* b, int n) {
    int total, p = 3, used = 0;
    if (n < 3) return -1;
    total = get24(b);
    if (total + 3 != n) return -1;
    t->cert_count = 0;
    while (p + 3 <= n) {
        int cl = get24(b + p);
        p += 3;
        if (p + cl > n) return -1;
        if (t->cert_count < WF_CHAIN_MAX && used + cl <= WF_TLS_CERT_STORE) {
            mcpy(t->cert_store + used, b + p, cl);
            if (wf_x509_parse(t->cert_store + used, cl, &t->certs[t->cert_count]) != 0) {
                if (t->cert_count == 0) { seterr(t, "cannot parse the server certificate", 0); return -1; }
                /* an unparsable extra certificate is skipped (not trusted either) */
            } else {
                t->cert_count++;
                used += cl;
            }
        }
        p += cl;
    }
    if (t->cert_count == 0) { seterr(t, "empty certificate list", 0); return -1; }
    return 0;
}

static int parse_ske(wf_tls_t* t, const uint8_t* b, int n) {
    static uint8_t signed_data[64 + 4 + 65];
    int p, scheme, sl, sig, group, pl;
    const wf_cert_t* leaf = &t->certs[0];
    if (n < 4 || b[0] != 3) { seterr(t, "bad key exchange", 0); return -1; }
    group = get16(b + 1); pl = b[3];
    if (group == 0x001d && pl == 32 && n >= 4 + 32 + 4) {
        t->group = 0x001d;
        mcpy(t->ecdh_srv_pub, b + 4, 32);
    } else if (group == 0x0017 && pl == 65 && n >= 4 + 65 + 4) {
        t->group = 0x0017;          /* secp256r1: servers without X25519 */
        mcpy(t->ecdh_srv_p256, b + 4, 65);
    } else {
        seterr(t, "server chose a key exchange group other than X25519 or P-256", 0);
        return -1;
    }
    p = 4 + pl;
    scheme = get16(b + p); p += 2;
    sl = get16(b + p); p += 2;
    if (p + sl != n) return -1;
    sig = scheme_to_sig(scheme);
    if (sig == WF_SIG_NONE) { seterr(t, "signature scheme not offered", 0); return -1; }
    if (t->suite == 0xc02bU && leaf->key_type == WF_KEY_RSA) { seterr(t, "ECDSA suite with an RSA certificate", 0); return -1; }
    if (t->suite == 0xc02fU && leaf->key_type != WF_KEY_RSA) { seterr(t, "RSA suite with an EC certificate", 0); return -1; }
    mcpy(signed_data, t->client_random, 32);
    mcpy(signed_data + 32, t->server_random, 32);
    mcpy(signed_data + 64, b, 4 + pl);
    if (wf_x509_verify_data(leaf, sig, signed_data, 64 + 4 + pl, b + p, sl) != 0) {
        seterr(t, "server key exchange signature is invalid", 0);
        return -1;
    }
    t->sig_scheme = (uint16_t)scheme;
    return 0;
}

/* ---------------------------------------------------------------- TLS 1.3 */

static int parse_certificates13(wf_tls_t* t, const uint8_t* b, int n) {
    int p, total, used = 0;
    if (n < 4) return -1;
    p = 1 + b[0];                         /* certificate_request_context */
    if (p + 3 > n) return -1;
    total = get24(b + p); p += 3;
    if (p + total != n) return -1;
    t->cert_count = 0;
    while (p + 3 <= n) {
        int cl = get24(b + p), el;
        p += 3;
        if (p + cl + 2 > n) return -1;
        if (t->cert_count < WF_CHAIN_MAX && used + cl <= WF_TLS_CERT_STORE) {
            mcpy(t->cert_store + used, b + p, cl);
            if (wf_x509_parse(t->cert_store + used, cl, &t->certs[t->cert_count]) != 0) {
                if (t->cert_count == 0) { seterr(t, "cannot parse the server certificate", 0); return -1; }
            } else {
                t->cert_count++;
                used += cl;
            }
        }
        p += cl;
        el = get16(b + p);
        p += 2 + el;                      /* OCSP / SCT extensions are not used */
        if (p > n) return -1;
    }
    if (t->cert_count == 0) { seterr(t, "empty certificate list", 0); return -1; }
    return 0;
}

static int verify_cv13(wf_tls_t* t, const uint8_t* b, int n, const uint8_t hash[32]) {
    static uint8_t content[64 + 34 + 32];
    static const char ctx[] = "TLS 1.3, server CertificateVerify";
    const wf_cert_t* leaf = &t->certs[0];
    int scheme, sl, sig, i;
    if (n < 4) return -1;
    scheme = get16(b); sl = get16(b + 2);
    if (4 + sl != n) return -1;
    switch (scheme) {
    case 0x0403: sig = WF_SIG_ECDSA_SHA256; if (leaf->key_type != WF_KEY_EC_P256) sig = WF_SIG_NONE; break;
    case 0x0503: sig = WF_SIG_ECDSA_SHA384; if (leaf->key_type != WF_KEY_EC_P384) sig = WF_SIG_NONE; break;
    case 0x0804: case 0x0805: case 0x0806:
        sig = scheme_to_sig(scheme); if (leaf->key_type != WF_KEY_RSA) sig = WF_SIG_NONE; break;
    default: sig = WF_SIG_NONE; break;   /* PKCS#1 v1.5 is not allowed in TLS 1.3 */
    }
    if (sig == WF_SIG_NONE) { seterr(t, "CertificateVerify scheme does not match the certificate", 0); return -1; }
    for (i = 0; i < 64; i++) content[i] = 0x20;
    mcpy(content + 64, (const uint8_t*)ctx, 33);
    content[97] = 0;
    mcpy(content + 98, hash, 32);
    if (wf_x509_verify_data(leaf, sig, content, 130, b + 4, sl) != 0) {
        seterr(t, "CertificateVerify signature is invalid", 0);
        return -1;
    }
    t->sig_scheme = (uint16_t)scheme;
    return 0;
}

static int tls13_handshake(wf_tls_t* t, uint32_t* ws) {
    static const uint8_t empty_hash[32] = {
        0xe3,0xb0,0xc4,0x42,0x98,0xfc,0x1c,0x14,0x9a,0xfb,0xf4,0xc8,0x99,0x6f,0xb9,0x24,
        0x27,0xae,0x41,0xe4,0x64,0x9b,0x93,0x4c,0xa4,0x95,0x99,0x1b,0x78,0x52,0xb8,0x55};
    static const uint8_t zero[32] = {0};
    uint8_t shared[32], secret[32], derived[32], hs_secret[32], c_hs[32], s_hs[32], hash[32], fk[32], fin[32];
    int type, off, len, consumed, got_cert = 0, got_cv = 0, i;
    uint8_t z = 0;
    if (x25519_shared_secret(shared, t->ecdh_priv, t->ecdh_srv_pub, ws, X25519_WORKSPACE_LIMBS) != 0) {
        seterr(t, "x25519 failed", 0);
        return -1;
    }
    for (i = 0; i < 32; i++) z |= shared[i];
    if (!z) { seterr(t, "x25519 low-order point", 0); return -1; }
    mset(t->ecdh_priv, 0, 32);
    wf_hkdf_extract(0, 0, zero, 32, secret);                       /* early secret */
    derive_secret(secret, "derived", empty_hash, derived);
    wf_hkdf_extract(derived, 32, shared, 32, hs_secret);
    mset(shared, 0, 32);
    hs_snapshot(t, hash);                                          /* CH..SH */
    derive_secret(hs_secret, "c hs traffic", hash, c_hs);
    derive_secret(hs_secret, "s hs traffic", hash, s_hs);
    tls13_keys(s_hs, t->s_key, t->s_iv13);
    tls13_keys(c_hs, t->c_key, t->c_iv13);
    t->s_seq = 0; t->c_seq = 0;
    t->s_encrypted = 1;
    for (;;) {
        if (got_cv) hs_snapshot(t, hash);   /* transcript before the server Finished */
        type = next_hs(t, &off, &len, &consumed);
        if (type < 0) return -1;
        if (type == 8) {                     /* EncryptedExtensions */
        } else if (type == HS_CERTIFICATE_REQUEST) {
            seterr(t, "server asks for a client certificate (not supported)", 0);
            return -1;
        } else if (type == HS_CERTIFICATE) {
            if (parse_certificates13(t, t->hs + off, len) != 0) { if (!t->err[0]) seterr(t, "bad certificate message", 0); return -1; }
            t->chain_rc = wf_chain_verify(t->certs, t->cert_count, t->trust, t->host, t->now, &t->chain);
            if (t->chain_rc != WF_CHAIN_OK) { seterr(t, "certificate rejected: ", wf_chain_error(t->chain_rc)); return -1; }
            got_cert = 1;
            hs_consume(t, consumed);
            hs_snapshot(t, hash);            /* CertificateVerify covers CH..Certificate */
            type = next_hs(t, &off, &len, &consumed);
            if (type != 15) { if (type >= 0) seterr(t, "expected CertificateVerify", 0); return -1; }
            if (verify_cv13(t, t->hs + off, len, hash) != 0) { if (!t->err[0]) seterr(t, "bad CertificateVerify", 0); return -1; }
            got_cv = 1;
        } else if (type == HS_FINISHED) {
            if (!got_cert || !got_cv) { seterr(t, "server Finished before its certificate", 0); return -1; }
            wf_hkdf_expand_label(s_hs, "finished", 0, 0, fk, 32);
            hmac_sha256(fk, 32, hash, 32, fin);
            if (len != 32 || !ct_eq(t->hs + off, fin, 32)) { seterr(t, "server Finished does not verify", 0); return -1; }
            hs_consume(t, consumed);
            break;
        } else {
            seterr(t, "unexpected handshake message", 0);
            return -1;
        }
        hs_consume(t, consumed);
    }
    if (t->hs_len != 0) { seterr(t, "data after the server Finished", 0); return -1; }
    hs_snapshot(t, hash);                                          /* CH..server Finished */
    derive_secret(hs_secret, "derived", empty_hash, derived);
    wf_hkdf_extract(derived, 32, zero, 32, secret);                /* master secret */
    derive_secret(secret, "c ap traffic", hash, t->c_secret);
    derive_secret(secret, "s ap traffic", hash, t->s_secret);
    /* compat CCS, then the client Finished under the handshake keys */
    {
        uint8_t one = 1;
        int v = t->version;
        t->version = 0x0303;                 /* plaintext record */
        if (send_record(t, CT_CCS, &one, 1, 0) != 0) return -1;
        t->version = v;
    }
    wf_hkdf_expand_label(c_hs, "finished", 0, 0, fk, 32);
    hmac_sha256(fk, 32, hash, 32, fin);
    if (send_hs(t, HS_FINISHED, fin, 32, 1) != 0) return -1;
    tls13_keys(t->s_secret, t->s_key, t->s_iv13);
    tls13_keys(t->c_secret, t->c_key, t->c_iv13);
    t->s_seq = 0; t->c_seq = 0;
    mset(hs_secret, 0, 32); mset(c_hs, 0, 32); mset(s_hs, 0, 32); mset(secret, 0, 32); mset(fk, 0, 32);
    t->plain_len = 0; t->plain_off = 0;
    t->ems = 1;                              /* TLS 1.3 binds the transcript by design */
    t->established = 1;
    return 0;
}

/* Post-handshake messages in TLS 1.3: NewSessionTicket is dropped,
 * KeyUpdate rolls the keys (RFC 8446 4.6.3). */
static int tls13_post_hs(wf_tls_t* t) {
    int p = 0;
    if (t->hs_len + t->plain_len > WF_TLS_HS_MAX) { seterr(t, "post-handshake message too large", 0); return -1; }
    mcpy(t->hs + t->hs_len, t->plain, t->plain_len);
    t->hs_len += t->plain_len;
    t->plain_len = 0;
    while (t->hs_len - p >= 4) {
        int l = get24(t->hs + p + 1), ty = t->hs[p];
        if (t->hs_len - p < 4 + l) break;
        if (ty == 24 && l == 1) {
            int req = t->hs[p + 4];
            uint8_t ns[32];
            wf_hkdf_expand_label(t->s_secret, "traffic upd", 0, 0, ns, 32);
            mcpy(t->s_secret, ns, 32);
            tls13_keys(t->s_secret, t->s_key, t->s_iv13);
            t->s_seq = 0;
            if (req == 1) {
                uint8_t m[5] = {24, 0, 0, 1, 0};
                if (send_record(t, CT_HS, m, 5, 1) != 0) return -1;
                wf_hkdf_expand_label(t->c_secret, "traffic upd", 0, 0, ns, 32);
                mcpy(t->c_secret, ns, 32);
                tls13_keys(t->c_secret, t->c_key, t->c_iv13);
                t->c_seq = 0;
            }
        } else if (ty != 4) {
            seterr(t, "unexpected post-handshake message", 0);
            return -1;
        }
        p += 4 + l;
    }
    hs_consume(t, p);
    return 0;
}

int wf_tls_handshake(wf_tls_t* t) {
    static uint8_t buf[1024];
    static uint32_t ws[X25519_WORKSPACE_LIMBS];
    uint8_t pms[32], my_pub[32], hash[32], keys[40], fin[12], sid[32];
    int type, off, len, consumed, n, got_cert = 0, got_ske = 0;
    t->err[0] = 0; t->alert = -1; t->suite = 0; t->sig_scheme = 0; t->chain_rc = -1;
    t->established = 0; t->peer_closed = 0; t->c_seq = 0; t->s_seq = 0; t->s_encrypted = 0;
    t->rec_len = 0; t->plain_len = 0; t->plain_off = 0; t->hs_len = 0; t->cert_count = 0;
    t->version = 0; t->got_share = 0; t->ems = 0; t->group = 0;
    if (!t->send || !t->recv || !t->rand || !t->trust) { seterr(t, "tls: missing callbacks or root store", 0); return -1; }
    sha256_init(&t->hs_hash);
    if (t->rand(t->io, t->client_random, 32) != 0 || t->rand(t->io, t->ecdh_priv, 32) != 0) {
        seterr(t, "no entropy", 0);
        return -1;
    }
    if (t->rand(t->io, sid, 32) != 0) { seterr(t, "no entropy", 0); return -1; }
    if (x25519_public_key(my_pub, t->ecdh_priv, ws, X25519_WORKSPACE_LIMBS) != 0) { seterr(t, "x25519 failed", 0); return -1; }
    n = build_client_hello(t, buf, my_pub, sid);
    if (send_hs(t, HS_CLIENT_HELLO, buf, n, 0) != 0) return -1;
    /* ServerHello .. ServerHelloDone */
    for (;;) {
        type = next_hs(t, &off, &len, &consumed);
        if (type < 0) return -1;
        if (type == HS_SERVER_HELLO) {
            if (parse_server_hello(t, t->hs + off, len) != 0) { if (!t->err[0]) seterr(t, "bad server hello", 0); return -1; }
            if (t->version == 0x0304) { hs_consume(t, consumed); return tls13_handshake(t, ws); }
        } else if (type == HS_CERTIFICATE) {
            if (parse_certificates(t, t->hs + off, len) != 0) { if (!t->err[0]) seterr(t, "bad certificate message", 0); return -1; }
            t->chain_rc = wf_chain_verify(t->certs, t->cert_count, t->trust, t->host, t->now, &t->chain);
            if (t->chain_rc != WF_CHAIN_OK) { seterr(t, "certificate rejected: ", wf_chain_error(t->chain_rc)); return -1; }
            got_cert = 1;
        } else if (type == HS_SERVER_KEY_EXCHANGE) {
            if (!got_cert) { seterr(t, "key exchange before certificate", 0); return -1; }
            if (parse_ske(t, t->hs + off, len) != 0) { if (!t->err[0]) seterr(t, "bad key exchange", 0); return -1; }
            got_ske = 1;
        } else if (type == HS_CERTIFICATE_REQUEST) {
            seterr(t, "server asks for a client certificate (not supported)", 0);
            return -1;
        } else if (type == HS_SERVER_HELLO_DONE) {
            hs_consume(t, consumed);
            break;
        } else {
            seterr(t, "unexpected handshake message", 0);
            return -1;
        }
        hs_consume(t, consumed);
    }
    if (!got_cert || !got_ske) { seterr(t, "incomplete server flight", 0); return -1; }
    /* ClientKeyExchange */
    if (t->group == 0x0017) {
        static uint8_t p256_pub[65];
        if (wf_ecdh_p256(t->ecdh_priv, t->ecdh_srv_p256, 65, p256_pub, pms) != 0) { seterr(t, "P-256 ECDH failed", 0); return -1; }
        buf[0] = 65;
        mcpy(buf + 1, p256_pub, 65);
    } else {
        if (x25519_shared_secret(pms, t->ecdh_priv, t->ecdh_srv_pub, ws, X25519_WORKSPACE_LIMBS) != 0) {
            seterr(t, "x25519 failed", 0);
            return -1;
        }
        buf[0] = 32;
        mcpy(buf + 1, my_pub, 32);
    }
    {
        uint8_t z = 0; int i;
        for (i = 0; i < 32; i++) z |= pms[i];
        if (!z) { seterr(t, "x25519 low-order point", 0); return -1; }
    }
    if (send_hs(t, HS_CLIENT_KEY_EXCHANGE, buf, 1 + buf[0], 0) != 0) return -1;
    /* extended master secret over the session hash */
    if (t->ems) {
        hs_snapshot(t, hash);
        wf_tls_prf(pms, 32, "extended master secret", hash, 32, t->master, 48);
    } else {
        mcpy(buf, t->client_random, 32);
        mcpy(buf + 32, t->server_random, 32);
        wf_tls_prf(pms, 32, "master secret", buf, 64, t->master, 48);
    }
    mset(pms, 0, 32);
    mset(t->ecdh_priv, 0, 32);
    mcpy(buf, t->server_random, 32);
    mcpy(buf + 32, t->client_random, 32);
    wf_tls_prf(t->master, 48, "key expansion", buf, 64, keys, 40);
    mcpy(t->c_key, keys, 16); mcpy(t->s_key, keys + 16, 16);
    mcpy(t->c_iv, keys + 32, 4); mcpy(t->s_iv, keys + 36, 4);
    mset(keys, 0, 40);
    buf[0] = 1;
    if (send_record(t, CT_CCS, buf, 1, 0) != 0) return -1;
    hs_snapshot(t, hash);
    wf_tls_prf(t->master, 48, "client finished", hash, 32, fin, 12);
    if (send_hs(t, HS_FINISHED, fin, 12, 1) != 0) return -1;
    /* server ChangeCipherSpec + Finished */
    if (t->hs_len != 0) { seterr(t, "data after ServerHelloDone", 0); return -1; }
    type = read_record(t);
    if (type != CT_CCS || t->plain_len != 1 || t->plain[0] != 1) {
        if (type >= 0) seterr(t, "expected ChangeCipherSpec", 0);
        return -1;
    }
    t->s_encrypted = 1;
    hs_snapshot(t, hash);
    wf_tls_prf(t->master, 48, "server finished", hash, 32, fin, 12);
    type = next_hs(t, &off, &len, &consumed);
    if (type != HS_FINISHED || len != 12) { if (type >= 0) seterr(t, "expected Finished", 0); return -1; }
    if (!ct_eq(t->hs + off, fin, 12)) { seterr(t, "server Finished does not verify", 0); return -1; }
    hs_consume(t, consumed);
    t->plain_len = 0; t->plain_off = 0;   /* the Finished record is not application data */
    t->established = 1;
    return 0;
}

int wf_tls_write(wf_tls_t* t, const uint8_t* d, int n) {
    int off = 0;
    if (!t->established) return -1;
    while (off < n) {
        int chunk = n - off > 16000 ? 16000 : n - off;
        if (send_record(t, CT_APP, d + off, chunk, 1) != 0) return -1;
        off += chunk;
    }
    return n;
}

int wf_tls_read(wf_tls_t* t, uint8_t* d, int cap) {
    int type, n, i;
    if (!t->established) return -1;
    while (t->plain_off >= t->plain_len) {
        if (t->peer_closed) return 0;
        type = read_record(t);
        if (type == -2) return 0;   /* EOF without close_notify: treated as end, caller checks lengths */
        if (type < 0) return -1;
        if (type == CT_ALERT) return 0;
        if (type == CT_HS) {
            if (t->version == 0x0304) { if (tls13_post_hs(t) != 0) return -1; continue; }
            t->plain_len = 0; continue;   /* HelloRequest / tickets ignored, renegotiation refused */
        }
        if (type != CT_APP) { seterr(t, "unexpected record", 0); return -1; }
    }
    n = t->plain_len - t->plain_off;
    if (n > cap) n = cap;
    for (i = 0; i < n; i++) d[i] = t->plain[t->plain_off + i];
    t->plain_off += n;
    return n;
}

void wf_tls_close(wf_tls_t* t) {
    uint8_t a[2] = {1, 0};
    if (t->established) send_record(t, CT_ALERT, a, 2, 1);
    t->established = 0;
}
