/* wf_rsa.c - Lot 67: RSA public-key signature verification. */
#include "wf_rsa.h"
#include "wf_bn.h"
#include "wf_sha512.h"

static wf_mont_t g_rsa;
static uint8_t g_em[512];

/* DigestInfo prefixes (RFC 8017, 9.2 note 1). */
static const uint8_t DI_SHA256[19] = {0x30,0x31,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x01,0x05,0x00,0x04,0x20};
static const uint8_t DI_SHA384[19] = {0x30,0x41,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x02,0x05,0x00,0x04,0x30};
static const uint8_t DI_SHA512[19] = {0x30,0x51,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x03,0x05,0x00,0x04,0x40};

/* s^e mod n into g_em (k bytes). Returns k or < 0. */
static int rsa_public(const uint8_t* n, int n_len, const uint8_t* e, int e_len,
                      const uint8_t* sig, int sig_len) {
    wf_bn_t N, E, S, M;
    int k;
    while (n_len > 0 && n[0] == 0U) { n++; n_len--; }
    k = n_len;
    if (k < 128 || k > 512 || sig_len != k || e_len < 1 || e_len > 8) return -1;
    if (wf_bn_from_bytes(&N, n, n_len) || wf_bn_from_bytes(&E, e, e_len) || wf_bn_from_bytes(&S, sig, sig_len)) return -1;
    if (wf_bn_cmp(&S, &N) >= 0 || wf_bn_is_zero(&E) || !(E.v[0] & 1U)) return -2;
    if (wf_mont_init(&g_rsa, &N) != 0) return -1;
    wf_mont_exp(&g_rsa, &M, &S, &E);
    if (wf_bn_to_bytes(&M, g_em, k) != 0) return -2;
    return k;
}

int wf_rsa_pkcs1_verify(const uint8_t* n, int n_len, const uint8_t* e, int e_len,
                        int hash_id, const uint8_t* digest, int digest_len,
                        const uint8_t* sig, int sig_len) {
    const uint8_t* di;
    int k, i, t, ps;
    if (hash_id == WF_HASH_SHA256 && digest_len == 32) di = DI_SHA256;
    else if (hash_id == WF_HASH_SHA384 && digest_len == 48) di = DI_SHA384;
    else if (hash_id == WF_HASH_SHA512 && digest_len == 64) di = DI_SHA512;
    else return -1;
    k = rsa_public(n, n_len, e, e_len, sig, sig_len);
    if (k < 0) return k;
    t = 19 + digest_len;
    ps = k - 3 - t;
    if (ps < 8) return -3;
    if (g_em[0] != 0x00U || g_em[1] != 0x01U) return -4;
    for (i = 0; i < ps; i++)
        if (g_em[2 + i] != 0xffU) return -4;
    if (g_em[2 + ps] != 0x00U) return -4;
    for (i = 0; i < 19; i++)
        if (g_em[3 + ps + i] != di[i]) return -5;
    for (i = 0; i < digest_len; i++)
        if (g_em[3 + ps + 19 + i] != digest[i]) return -6;
    return 0;
}

/* MGF1 mask XORed into db (len bytes). */
static int mgf1_xor(int hash_id, const uint8_t* seed, int seed_len, uint8_t* db, int len) {
    uint8_t buf[64 + 4], out[64];
    uint32_t counter = 0;
    int done = 0, i, h;
    for (i = 0; i < seed_len; i++) buf[i] = seed[i];
    while (done < len) {
        buf[seed_len] = (uint8_t)(counter >> 24);
        buf[seed_len + 1] = (uint8_t)(counter >> 16);
        buf[seed_len + 2] = (uint8_t)(counter >> 8);
        buf[seed_len + 3] = (uint8_t)counter;
        h = wf_hash(hash_id, buf, (uint32_t)(seed_len + 4), out);
        if (h <= 0) return -1;
        for (i = 0; i < h && done < len; i++) db[done++] ^= out[i];
        counter++;
    }
    return 0;
}

int wf_rsa_pss_verify(const uint8_t* n, int n_len, const uint8_t* e, int e_len,
                      int hash_id, const uint8_t* digest, int digest_len,
                      const uint8_t* sig, int sig_len) {
    static uint8_t mprime[8 + 64 + 64];
    uint8_t hh[64];
    int k, em_len, db_len, i, sl = digest_len, bits, top_bits, h;
    wf_bn_t N;
    k = rsa_public(n, n_len, e, e_len, sig, sig_len);
    if (k < 0) return k;
    while (n_len > 0 && n[0] == 0U) { n++; n_len--; }
    (void)wf_bn_from_bytes(&N, n, n_len);
    bits = wf_bn_bits(&N) - 1;           /* emBits */
    em_len = (bits + 7) / 8;
    /* when emBits is a multiple of 8, EM is k - 1 bytes and g_em[0] is 0 */
    {
        const uint8_t* em = g_em + (k - em_len);
        uint8_t* db;
        if (k - em_len == 1 && g_em[0] != 0U) return -4;
        if (em_len < digest_len + sl + 2) return -3;
        if (em[em_len - 1] != 0xbcU) return -4;
        db_len = em_len - digest_len - 1;
        db = (uint8_t*)em;
        for (i = 0; i < digest_len; i++) hh[i] = em[db_len + i];
        top_bits = 8 * em_len - bits;
        if (top_bits && (db[0] & (uint8_t)(0xffU << (8 - top_bits)))) return -4;
        if (mgf1_xor(hash_id, hh, digest_len, db, db_len) != 0) return -1;
        if (top_bits) db[0] &= (uint8_t)(0xffU >> top_bits);
        for (i = 0; i < db_len - sl - 1; i++)
            if (db[i] != 0U) return -5;
        if (db[db_len - sl - 1] != 0x01U) return -5;
        for (i = 0; i < 8; i++) mprime[i] = 0U;
        for (i = 0; i < digest_len; i++) mprime[8 + i] = digest[i];
        for (i = 0; i < sl; i++) mprime[8 + digest_len + i] = db[db_len - sl + i];
        {
            uint8_t chk[64];
            h = wf_hash(hash_id, mprime, (uint32_t)(8 + digest_len + sl), chk);
            if (h != digest_len) return -1;
            for (i = 0; i < h; i++)
                if (chk[i] != hh[i]) return -6;
        }
    }
    return 0;
}
