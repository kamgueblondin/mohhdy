/* wf_x509.c - Lot 67: DER reader, X.509 v3 parsing, signature checks,
 * host name matching, PEM trust store and chain validation. */
#include "wf_x509.h"
#include "wf_sha512.h"
#include "wf_ec.h"
#include "wf_rsa.h"

/* ------------------------------------------------------------ DER */

int wf_der_next(const uint8_t* d, int len, int* pos, wf_der_t* out) {
    int p = *pos, l, n, i;
    if (!d || p < 0 || p + 2 > len) return -1;
    out->raw = d + p;
    out->tag = d[p++];
    if ((out->tag & 0x1fU) == 0x1fU) return -1;   /* high tag numbers: not used in X.509 */
    l = d[p++];
    if (l & 0x80) {
        n = l & 0x7f;
        if (n == 0 || n > 3 || p + n > len) return -1;
        l = 0;
        for (i = 0; i < n; i++) l = (l << 8) | d[p++];
        if (l < 128) return -1;                    /* DER: shortest form */
    }
    if (l < 0 || p + l > len) return -1;
    out->val = d + p;
    out->len = l;
    out->raw_len = p + l - *pos;
    *pos = p + l;
    return 0;
}

int wf_der_expect(const uint8_t* d, int len, int* pos, uint8_t tag, wf_der_t* out) {
    int save = *pos;
    if (wf_der_next(d, len, pos, out) != 0 || out->tag != tag) { *pos = save; return -1; }
    return 0;
}

static int oid_eq(const wf_der_t* o, const uint8_t* oid, int n) {
    int i;
    if (o->tag != 0x06U || o->len != n) return 0;
    for (i = 0; i < n; i++)
        if (o->val[i] != oid[i]) return 0;
    return 1;
}

static const uint8_t OID_RSA[] = {0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x01};
static const uint8_t OID_RSA_SHA1[] = {0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x05};
static const uint8_t OID_RSA_SHA256[] = {0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x0b};
static const uint8_t OID_RSA_SHA384[] = {0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x0c};
static const uint8_t OID_RSA_SHA512[] = {0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x0d};
static const uint8_t OID_RSA_PSS[] = {0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x0a};
static const uint8_t OID_EC_KEY[] = {0x2a,0x86,0x48,0xce,0x3d,0x02,0x01};
static const uint8_t OID_P256[] = {0x2a,0x86,0x48,0xce,0x3d,0x03,0x01,0x07};
static const uint8_t OID_P384[] = {0x2b,0x81,0x04,0x00,0x22};
static const uint8_t OID_ECDSA_SHA1[] = {0x2a,0x86,0x48,0xce,0x3d,0x04,0x01};
static const uint8_t OID_ECDSA_SHA256[] = {0x2a,0x86,0x48,0xce,0x3d,0x04,0x03,0x02};
static const uint8_t OID_ECDSA_SHA384[] = {0x2a,0x86,0x48,0xce,0x3d,0x04,0x03,0x03};
static const uint8_t OID_ECDSA_SHA512[] = {0x2a,0x86,0x48,0xce,0x3d,0x04,0x03,0x04};
static const uint8_t OID_SHA256[] = {0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x01};
static const uint8_t OID_SHA384[] = {0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x02};
static const uint8_t OID_SHA512[] = {0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x03};
static const uint8_t OID_CN[] = {0x55,0x04,0x03};
static const uint8_t OID_BC[] = {0x55,0x1d,0x13};
static const uint8_t OID_KU[] = {0x55,0x1d,0x0f};
static const uint8_t OID_SAN[] = {0x55,0x1d,0x11};
/* Extensions understood well enough to accept when critical. */
static const uint8_t OID_EKU[] = {0x55,0x1d,0x25};
static const uint8_t OID_SKI[] = {0x55,0x1d,0x0e};
static const uint8_t OID_AKI[] = {0x55,0x1d,0x23};
static const uint8_t OID_POLICIES[] = {0x55,0x1d,0x20};

/* PSS parameters: SEQUENCE { [0] hashAlgorithm, [1] mgf, [2] saltLength ... } */
static int pss_hash(const wf_der_t* params) {
    int pos = 0;
    wf_der_t h, alg, oid;
    if (params->tag != 0x30U) return WF_SIG_NONE;
    if (wf_der_expect(params->val, params->len, &pos, 0xa0U, &h) != 0) return WF_SIG_NONE;  /* default SHA-1 */
    pos = 0;
    if (wf_der_expect(h.val, h.len, &pos, 0x30U, &alg) != 0) return WF_SIG_NONE;
    pos = 0;
    if (wf_der_expect(alg.val, alg.len, &pos, 0x06U, &oid) != 0) return WF_SIG_NONE;
    if (oid_eq(&oid, OID_SHA256, sizeof(OID_SHA256))) return WF_SIG_RSA_PSS_SHA256;
    if (oid_eq(&oid, OID_SHA384, sizeof(OID_SHA384))) return WF_SIG_RSA_PSS_SHA384;
    if (oid_eq(&oid, OID_SHA512, sizeof(OID_SHA512))) return WF_SIG_RSA_PSS_SHA512;
    return WF_SIG_NONE;
}

static int parse_sig_alg(const wf_der_t* algseq) {
    int pos = 0;
    wf_der_t oid, params;
    if (algseq->tag != 0x30U) return WF_SIG_NONE;
    if (wf_der_expect(algseq->val, algseq->len, &pos, 0x06U, &oid) != 0) return WF_SIG_NONE;
    if (oid_eq(&oid, OID_RSA_SHA256, sizeof(OID_RSA_SHA256))) return WF_SIG_RSA_SHA256;
    if (oid_eq(&oid, OID_RSA_SHA384, sizeof(OID_RSA_SHA384))) return WF_SIG_RSA_SHA384;
    if (oid_eq(&oid, OID_RSA_SHA512, sizeof(OID_RSA_SHA512))) return WF_SIG_RSA_SHA512;
    if (oid_eq(&oid, OID_ECDSA_SHA256, sizeof(OID_ECDSA_SHA256))) return WF_SIG_ECDSA_SHA256;
    if (oid_eq(&oid, OID_ECDSA_SHA384, sizeof(OID_ECDSA_SHA384))) return WF_SIG_ECDSA_SHA384;
    if (oid_eq(&oid, OID_ECDSA_SHA512, sizeof(OID_ECDSA_SHA512))) return WF_SIG_ECDSA_SHA512;
    if (oid_eq(&oid, OID_RSA_SHA1, sizeof(OID_RSA_SHA1)) || oid_eq(&oid, OID_ECDSA_SHA1, sizeof(OID_ECDSA_SHA1)))
        return WF_SIG_SHA1_REFUSED;
    if (oid_eq(&oid, OID_RSA_PSS, sizeof(OID_RSA_PSS))) {
        if (wf_der_next(algseq->val, algseq->len, &pos, &params) != 0) return WF_SIG_NONE;
        return pss_hash(&params);
    }
    return WF_SIG_NONE;
}

/* UTCTime / GeneralizedTime -> YYYYMMDDHHMMSS. */
static int parse_time(const wf_der_t* t, char out[15]) {
    int i, off = 0;
    const uint8_t* v = t->val;
    if (t->tag == 0x17U) {
        if (t->len != 13 || v[12] != 'Z') return -1;
        out[0] = (v[0] - '0') < 5 ? '2' : '1';
        out[1] = (v[0] - '0') < 5 ? '0' : '9';
        off = 2;
        for (i = 0; i < 12; i++) out[off + i] = (char)v[i];
    } else if (t->tag == 0x18U) {
        if (t->len != 15 || v[14] != 'Z') return -1;
        for (i = 0; i < 14; i++) out[i] = (char)v[i];
    } else {
        return -1;
    }
    for (i = 0; i < 14; i++)
        if (out[i] < '0' || out[i] > '9') return -1;
    out[14] = 0;
    return 0;
}

static void copy_cn(const wf_der_t* name, char* cn, int cap) {
    int pos = 0, n = 0;
    wf_der_t set, atv, oid, val;
    cn[0] = 0;
    while (pos < name->len && wf_der_expect(name->val, name->len, &pos, 0x31U, &set) == 0) {
        int sp = 0;
        while (sp < set.len && wf_der_expect(set.val, set.len, &sp, 0x30U, &atv) == 0) {
            int ap = 0;
            if (wf_der_expect(atv.val, atv.len, &ap, 0x06U, &oid) != 0) continue;
            if (wf_der_next(atv.val, atv.len, &ap, &val) != 0) continue;
            if (oid_eq(&oid, OID_CN, sizeof(OID_CN))) {
                int i;
                n = 0;
                for (i = 0; i < val.len && n < cap - 1; i++) {
                    uint8_t ch = val.val[i];
                    cn[n++] = (ch >= 0x20U && ch < 0x7fU) ? (char)ch : '?';
                }
                cn[n] = 0;
            }
        }
    }
}

static int parse_spki(const wf_der_t* spki, wf_cert_t* c) {
    int pos = 0, ap = 0;
    wf_der_t alg, key, oid, param;
    if (wf_der_expect(spki->val, spki->len, &pos, 0x30U, &alg) != 0) return -1;
    if (wf_der_expect(spki->val, spki->len, &pos, 0x03U, &key) != 0 || key.len < 2 || key.val[0] != 0U) return -1;
    if (wf_der_expect(alg.val, alg.len, &ap, 0x06U, &oid) != 0) return -1;
    if (oid_eq(&oid, OID_RSA, sizeof(OID_RSA))) {
        int kp = 0, rp = 0;
        wf_der_t seq, n, e;
        if (wf_der_expect(key.val + 1, key.len - 1, &kp, 0x30U, &seq) != 0) return -1;
        if (wf_der_expect(seq.val, seq.len, &rp, 0x02U, &n) != 0 || wf_der_expect(seq.val, seq.len, &rp, 0x02U, &e) != 0)
            return -1;
        c->rsa_n = n.val; c->rsa_n_len = n.len;
        c->rsa_e = e.val; c->rsa_e_len = e.len;
        c->key_type = WF_KEY_RSA;
        return 0;
    }
    if (oid_eq(&oid, OID_EC_KEY, sizeof(OID_EC_KEY))) {
        if (wf_der_expect(alg.val, alg.len, &ap, 0x06U, &param) != 0) return -1;
        if (oid_eq(&param, OID_P256, sizeof(OID_P256))) c->key_type = WF_KEY_EC_P256;
        else if (oid_eq(&param, OID_P384, sizeof(OID_P384))) c->key_type = WF_KEY_EC_P384;
        else return -1;
        c->ec_point = key.val + 1;
        c->ec_point_len = key.len - 1;
        return 0;
    }
    return -1;
}

static int parse_ext(const wf_der_t* ext, wf_cert_t* c) {
    int pos = 0, critical = 0;
    wf_der_t oid, crit, val;
    if (wf_der_expect(ext->val, ext->len, &pos, 0x06U, &oid) != 0) return -1;
    if (wf_der_expect(ext->val, ext->len, &pos, 0x01U, &crit) == 0) critical = crit.len == 1 && crit.val[0] != 0U;
    if (wf_der_expect(ext->val, ext->len, &pos, 0x04U, &val) != 0) return -1;
    if (oid_eq(&oid, OID_BC, sizeof(OID_BC))) {
        int bp = 0, ip = 0;
        wf_der_t seq, b, n;
        if (wf_der_expect(val.val, val.len, &bp, 0x30U, &seq) != 0) return -1;
        if (wf_der_expect(seq.val, seq.len, &ip, 0x01U, &b) == 0) c->is_ca = b.len == 1 && b.val[0] != 0U;
        if (wf_der_expect(seq.val, seq.len, &ip, 0x02U, &n) == 0 && n.len >= 1 && n.len <= 2) {
            int i, v = 0;
            for (i = 0; i < n.len; i++) v = (v << 8) | n.val[i];
            c->path_len = v;
        }
        return 0;
    }
    if (oid_eq(&oid, OID_KU, sizeof(OID_KU))) {
        int kp = 0;
        wf_der_t bits;
        if (wf_der_expect(val.val, val.len, &kp, 0x03U, &bits) != 0 || bits.len < 2) return -1;
        c->has_key_usage = 1;
        c->digital_signature = (bits.val[1] & 0x80U) != 0;
        c->key_cert_sign = (bits.val[1] & 0x04U) != 0;
        return 0;
    }
    if (oid_eq(&oid, OID_SAN, sizeof(OID_SAN))) {
        int sp = 0, np = 0;
        wf_der_t seq, gn;
        if (wf_der_expect(val.val, val.len, &sp, 0x30U, &seq) != 0) return -1;
        while (np < seq.len && wf_der_next(seq.val, seq.len, &np, &gn) == 0) {
            if (gn.tag == 0x82U && c->san_count < WF_X509_MAX_SAN) {
                c->san[c->san_count] = gn.val;
                c->san_len[c->san_count] = gn.len;
                c->san_count++;
            } else if (gn.tag == 0x87U && gn.len == 4 && c->san_ip_count < 4) {
                int i;
                for (i = 0; i < 4; i++) c->san_ip[c->san_ip_count][i] = gn.val[i];
                c->san_ip_count++;
            }
        }
        return 0;
    }
    if (critical && !oid_eq(&oid, OID_EKU, sizeof(OID_EKU)) && !oid_eq(&oid, OID_SKI, sizeof(OID_SKI)) &&
        !oid_eq(&oid, OID_AKI, sizeof(OID_AKI)) && !oid_eq(&oid, OID_POLICIES, sizeof(OID_POLICIES)))
        c->unknown_critical = 1;
    return 0;
}

int wf_x509_parse(const uint8_t* der, int len, wf_cert_t* c) {
    int pos = 0, cp = 0, tp = 0, i;
    wf_der_t cert, tbs, salg, sig, el, validity, nb, na;
    uint8_t* z = (uint8_t*)c;
    for (i = 0; i < (int)sizeof(*c); i++) z[i] = 0U;
    c->path_len = -1;
    if (wf_der_expect(der, len, &pos, 0x30U, &cert) != 0 || pos != len) return WF_X509_E_DER;
    c->der = der; c->der_len = len;
    if (wf_der_expect(cert.val, cert.len, &cp, 0x30U, &tbs) != 0) return WF_X509_E_DER;
    if (wf_der_expect(cert.val, cert.len, &cp, 0x30U, &salg) != 0) return WF_X509_E_DER;
    if (wf_der_expect(cert.val, cert.len, &cp, 0x03U, &sig) != 0 || sig.len < 2 || sig.val[0] != 0U) return WF_X509_E_DER;
    c->tbs = tbs.raw; c->tbs_len = tbs.raw_len;
    c->sig_alg = parse_sig_alg(&salg);
    if (c->sig_alg == WF_SIG_NONE) return WF_X509_E_ALG;
    c->sig = sig.val + 1; c->sig_len = sig.len - 1;
    /* tbs: [0] version, serial, signature, issuer, validity, subject, spki, ... [3] exts */
    c->version = 1;
    if (wf_der_expect(tbs.val, tbs.len, &tp, 0xa0U, &el) == 0) {
        int vp = 0;
        wf_der_t v;
        if (wf_der_expect(el.val, el.len, &vp, 0x02U, &v) != 0 || v.len != 1 || v.val[0] > 2U) return WF_X509_E_VERSION;
        c->version = v.val[0] + 1;
    }
    if (wf_der_expect(tbs.val, tbs.len, &tp, 0x02U, &el) != 0) return WF_X509_E_DER;  /* serial */
    if (wf_der_expect(tbs.val, tbs.len, &tp, 0x30U, &el) != 0) return WF_X509_E_DER;  /* inner signature */
    if (parse_sig_alg(&el) != c->sig_alg) return WF_X509_E_ALG;
    if (wf_der_expect(tbs.val, tbs.len, &tp, 0x30U, &el) != 0) return WF_X509_E_DER;
    c->issuer = el.raw; c->issuer_len = el.raw_len;
    if (wf_der_expect(tbs.val, tbs.len, &tp, 0x30U, &validity) != 0) return WF_X509_E_DER;
    {
        int vp = 0;
        if (wf_der_next(validity.val, validity.len, &vp, &nb) != 0 || wf_der_next(validity.val, validity.len, &vp, &na) != 0)
            return WF_X509_E_TIME;
        if (parse_time(&nb, c->not_before) != 0 || parse_time(&na, c->not_after) != 0) return WF_X509_E_TIME;
    }
    if (wf_der_expect(tbs.val, tbs.len, &tp, 0x30U, &el) != 0) return WF_X509_E_DER;
    c->subject = el.raw; c->subject_len = el.raw_len;
    copy_cn(&el, c->cn, (int)sizeof(c->cn));
    if (wf_der_expect(tbs.val, tbs.len, &tp, 0x30U, &el) != 0) return WF_X509_E_DER;
    if (parse_spki(&el, c) != 0) return WF_X509_E_KEY;
    while (tp < tbs.len) {
        if (wf_der_next(tbs.val, tbs.len, &tp, &el) != 0) return WF_X509_E_DER;
        if (el.tag == 0xa3U) {
            int ep = 0, xp = 0;
            wf_der_t seq, ext;
            if (wf_der_expect(el.val, el.len, &ep, 0x30U, &seq) != 0) return WF_X509_E_DER;
            while (xp < seq.len) {
                if (wf_der_expect(seq.val, seq.len, &xp, 0x30U, &ext) != 0) return WF_X509_E_DER;
                if (parse_ext(&ext, c) != 0) return WF_X509_E_DER;
            }
        }
    }
    return 0;
}

/* ------------------------------------------------------------ signatures */

static int sig_hash(int sig_alg) {
    switch (sig_alg) {
    case WF_SIG_RSA_SHA256: case WF_SIG_ECDSA_SHA256: case WF_SIG_RSA_PSS_SHA256: return WF_HASH_SHA256;
    case WF_SIG_RSA_SHA384: case WF_SIG_ECDSA_SHA384: case WF_SIG_RSA_PSS_SHA384: return WF_HASH_SHA384;
    case WF_SIG_RSA_SHA512: case WF_SIG_ECDSA_SHA512: case WF_SIG_RSA_PSS_SHA512: return WF_HASH_SHA512;
    default: return 0;
    }
}

int wf_x509_verify_data(const wf_cert_t* k, int sig_alg, const uint8_t* data, int data_len,
                        const uint8_t* sig, int sig_len) {
    uint8_t digest[64];
    int h = sig_hash(sig_alg), dl;
    if (!h) return -1;
    dl = wf_hash(h, data, (uint32_t)data_len, digest);
    if (dl <= 0) return -1;
    if (sig_alg >= WF_SIG_RSA_SHA256 && sig_alg <= WF_SIG_RSA_SHA512) {
        if (k->key_type != WF_KEY_RSA) return -2;
        return wf_rsa_pkcs1_verify(k->rsa_n, k->rsa_n_len, k->rsa_e, k->rsa_e_len, h, digest, dl, sig, sig_len) ? -3 : 0;
    }
    if (sig_alg >= WF_SIG_RSA_PSS_SHA256 && sig_alg <= WF_SIG_RSA_PSS_SHA512) {
        if (k->key_type != WF_KEY_RSA) return -2;
        return wf_rsa_pss_verify(k->rsa_n, k->rsa_n_len, k->rsa_e, k->rsa_e_len, h, digest, dl, sig, sig_len) ? -3 : 0;
    }
    if (k->key_type == WF_KEY_EC_P256)
        return wf_ecdsa_verify_der(WF_CURVE_P256, k->ec_point, k->ec_point_len, digest, dl, sig, sig_len) ? -3 : 0;
    if (k->key_type == WF_KEY_EC_P384)
        return wf_ecdsa_verify_der(WF_CURVE_P384, k->ec_point, k->ec_point_len, digest, dl, sig, sig_len) ? -3 : 0;
    return -2;
}

int wf_x509_verify_signed_by(const wf_cert_t* c, const wf_cert_t* issuer) {
    if (c->sig_alg == WF_SIG_SHA1_REFUSED) return -4;
    return wf_x509_verify_data(issuer, c->sig_alg, c->tbs, c->tbs_len, c->sig, c->sig_len);
}

/* ------------------------------------------------------------ names */

int wf_x509_name_eq(const uint8_t* a, int alen, const uint8_t* b, int blen) {
    int i;
    if (alen != blen) return 0;
    for (i = 0; i < alen; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

static char lower(char ch) { return (ch >= 'A' && ch <= 'Z') ? (char)(ch + 32) : ch; }

static int host_len(const char* h) {
    int n = 0;
    while (h[n]) n++;
    if (n > 0 && h[n - 1] == '.') n--;   /* absolute name */
    return n;
}

static int parse_ipv4(const char* s, uint8_t ip[4]) {
    int part = 0, val = -1, i;
    for (i = 0;; i++) {
        char ch = s[i];
        if (ch >= '0' && ch <= '9') {
            val = (val < 0 ? 0 : val) * 10 + (ch - '0');
            if (val > 255) return -1;
        } else if (ch == '.' || ch == 0) {
            if (val < 0 || part > 3) return -1;
            ip[part++] = (uint8_t)val;
            val = -1;
            if (ch == 0) break;
        } else {
            return -1;
        }
    }
    return part == 4 ? 0 : -1;
}

static int dns_match(const uint8_t* pat, int plen, const char* host, int hlen) {
    int i;
    if (plen <= 0 || hlen <= 0) return 0;
    if (plen > 2 && pat[0] == '*' && pat[1] == '.') {
        /* "*.example.com": exactly one label, and at least two labels after it */
        int dots = 0, first;
        for (i = 2; i < plen; i++) if (pat[i] == '.') dots++;
        if (dots < 1) return 0;
        for (first = 0; first < hlen && host[first] != '.'; first++) {}
        if (first == 0 || first >= hlen) return 0;
        if (hlen - first != plen - 1) return 0;
        for (i = 0; i < plen - 1; i++)
            if (lower((char)pat[1 + i]) != lower(host[first + i])) return 0;
        return 1;
    }
    if (plen != hlen) return 0;
    for (i = 0; i < plen; i++) {
        if (pat[i] == '*') return 0;   /* partial-label wildcards are refused */
        if (lower((char)pat[i]) != lower(host[i])) return 0;
    }
    return 1;
}

int wf_x509_host_match(const wf_cert_t* c, const char* host) {
    uint8_t ip[4];
    int i, hl;
    if (!c || !host || !host[0]) return -1;
    if (parse_ipv4(host, ip) == 0) {
        for (i = 0; i < c->san_ip_count; i++)
            if (c->san_ip[i][0] == ip[0] && c->san_ip[i][1] == ip[1] && c->san_ip[i][2] == ip[2] && c->san_ip[i][3] == ip[3])
                return 0;
        return -1;
    }
    hl = host_len(host);
    for (i = 0; i < c->san_count; i++)
        if (dns_match(c->san[i], c->san_len[i], host, hl)) return 0;
    return -1;
}

/* ------------------------------------------------------------ PEM */

static int b64v(char ch) {
    if (ch >= 'A' && ch <= 'Z') return ch - 'A';
    if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
    if (ch >= '0' && ch <= '9') return ch - '0' + 52;
    if (ch == '+') return 62;
    if (ch == '/') return 63;
    return -1;
}

int wf_base64_decode(const char* in, int in_len, uint8_t* out, int cap) {
    uint32_t acc = 0;
    int bits = 0, n = 0, i, pad = 0;
    for (i = 0; i < in_len; i++) {
        char ch = in[i];
        int v;
        if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') continue;
        if (ch == '=') { pad++; continue; }
        if (pad) return -1;
        v = b64v(ch);
        if (v < 0) return -1;
        acc = (acc << 6) | (uint32_t)v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (n >= cap) return -1;
            out[n++] = (uint8_t)(acc >> bits);
        }
    }
    if (pad > 2) return -1;
    return n;
}

static int find(const char* s, int len, int from, const char* pat) {
    int i, j;
    for (i = from; i < len; i++) {
        for (j = 0; pat[j] && i + j < len && s[i + j] == pat[j]; j++) {}
        if (!pat[j]) return i;
    }
    return -1;
}

int wf_trust_add_pem(wf_trust_t* t, const char* pem, int pem_len, uint8_t* der_out, int der_cap, int* der_used) {
    static const char BEGIN[] = "-----BEGIN CERTIFICATE-----";
    static const char END[] = "-----END CERTIFICATE-----";
    int pos = 0, added = 0;
    while (pos < pem_len && t->count < WF_TRUST_MAX) {
        int b = find(pem, pem_len, pos, BEGIN), e, n;
        if (b < 0) break;
        b += (int)sizeof(BEGIN) - 1;
        e = find(pem, pem_len, b, END);
        if (e < 0) break;
        pos = e + (int)sizeof(END) - 1;
        n = wf_base64_decode(pem + b, e - b, der_out + *der_used, der_cap - *der_used);
        if (n <= 0 || wf_x509_parse(der_out + *der_used, n, &t->roots[t->count]) != 0) {
            t->skipped++;
            continue;
        }
        *der_used += n;
        t->count++;
        added++;
    }
    return added;
}

/* ------------------------------------------------------------ chain */

static int time_cmp(const char* a, const char* b) {
    int i;
    for (i = 0; i < 14; i++) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    return 0;
}

static void cn_copy(char* d, const char* s) {
    int i;
    for (i = 0; i < 63 && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int check_time(const wf_cert_t* c, const char* now) {
    if (!now) return 0;
    if (time_cmp(now, c->not_before) < 0) return WF_CHAIN_E_NOT_YET;
    if (time_cmp(now, c->not_after) > 0) return WF_CHAIN_E_EXPIRED;
    return 0;
}

static int check_ca(const wf_cert_t* ca, int below) {
    if (ca->version == 3 && !ca->is_ca) return WF_CHAIN_E_NOT_CA;
    if (ca->has_key_usage && !ca->key_cert_sign) return WF_CHAIN_E_NOT_CA;
    if (ca->path_len >= 0 && below > ca->path_len) return WF_CHAIN_E_PATHLEN;
    return 0;
}

int wf_chain_verify(const wf_cert_t* certs, int count, const wf_trust_t* trust,
                    const char* host, const char* now, wf_chain_info_t* info) {
    const wf_cert_t* cur;
    int depth, used[WF_CHAIN_MAX], i, rc, intermediates = 0;
    if (info) {
        info->depth = 0; info->anchor = -1; info->time_checked = now ? 1 : 0;
        info->leaf_cn[0] = 0; info->anchor_cn[0] = 0; info->not_after[0] = 0;
    }
    if (!certs || count <= 0 || !trust) return WF_CHAIN_E_EMPTY;
    if (count > WF_CHAIN_MAX) count = WF_CHAIN_MAX;
    for (i = 0; i < count; i++) used[i] = 0;
    cur = &certs[0];
    used[0] = 1;
    if (info) { cn_copy(info->leaf_cn, cur->cn); cn_copy(info->not_after, cur->not_after); }
    if (cur->unknown_critical) return WF_CHAIN_E_CRITICAL;
    if (cur->sig_alg == WF_SIG_SHA1_REFUSED) return WF_CHAIN_E_WEAK;
    if (host && wf_x509_host_match(cur, host) != 0) return WF_CHAIN_E_HOST;
    if ((rc = check_time(cur, now)) != 0) return rc;
    for (depth = 1; depth <= WF_CHAIN_MAX; depth++) {
        const wf_cert_t* next = 0;
        /* 1. a trust anchor that signed cur */
        for (i = 0; i < trust->count; i++) {
            const wf_cert_t* a = &trust->roots[i];
            if (!wf_x509_name_eq(cur->issuer, cur->issuer_len, a->subject, a->subject_len)) continue;
            if (wf_x509_verify_signed_by(cur, a) != 0) continue;
            if ((rc = check_time(a, now)) != 0) return rc;
            if (info) { info->depth = depth + 1; info->anchor = i; cn_copy(info->anchor_cn, a->cn); }
            return WF_CHAIN_OK;
        }
        /* cur itself is a trust anchor (server sent the root only) */
        for (i = 0; i < trust->count; i++) {
            const wf_cert_t* a = &trust->roots[i];
            if (a->der_len == cur->der_len && wf_x509_name_eq(a->der, a->der_len, cur->der, cur->der_len)) {
                if (info) { info->depth = depth; info->anchor = i; cn_copy(info->anchor_cn, a->cn); }
                return WF_CHAIN_OK;
            }
        }
        /* 2. a presented intermediate that signed cur */
        for (i = 1; i < count; i++) {
            const wf_cert_t* c = &certs[i];
            if (used[i]) continue;
            if (!wf_x509_name_eq(cur->issuer, cur->issuer_len, c->subject, c->subject_len)) continue;
            if (c->unknown_critical) return WF_CHAIN_E_CRITICAL;
            if ((rc = check_ca(c, intermediates)) != 0) return rc;
            if (wf_x509_verify_signed_by(cur, c) != 0) return WF_CHAIN_E_SIGNATURE;
            if ((rc = check_time(c, now)) != 0) return rc;
            used[i] = 1;
            next = c;
            break;
        }
        if (!next) {
            /* a name match with a bad signature is a signature error */
            for (i = 0; i < trust->count; i++)
                if (wf_x509_name_eq(cur->issuer, cur->issuer_len, trust->roots[i].subject, trust->roots[i].subject_len))
                    return WF_CHAIN_E_SIGNATURE;
            return WF_CHAIN_E_UNTRUSTED;
        }
        intermediates++;
        cur = next;
    }
    return WF_CHAIN_E_DEPTH;
}

const char* wf_chain_error(int rc) {
    switch (rc) {
    case WF_CHAIN_OK: return "ok";
    case WF_CHAIN_E_EMPTY: return "no certificate";
    case WF_CHAIN_E_PARSE: return "certificate parse error";
    case WF_CHAIN_E_HOST: return "host name mismatch";
    case WF_CHAIN_E_EXPIRED: return "certificate expired";
    case WF_CHAIN_E_NOT_YET: return "certificate not yet valid";
    case WF_CHAIN_E_UNTRUSTED: return "unknown issuer (not in the root store)";
    case WF_CHAIN_E_SIGNATURE: return "bad signature in chain";
    case WF_CHAIN_E_NOT_CA: return "issuer is not a CA";
    case WF_CHAIN_E_PATHLEN: return "path length exceeded";
    case WF_CHAIN_E_DEPTH: return "chain too long";
    case WF_CHAIN_E_CRITICAL: return "unknown critical extension";
    case WF_CHAIN_E_WEAK: return "SHA-1 signature refused";
    default: return "error";
    }
}
