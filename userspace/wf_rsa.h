/* wf_rsa.h - Lot 67: RSA signature verification (PKCS#1 v1.5 and PSS)
 * for 1024..4096-bit public keys, over SHA-256/384/512 digests. */
#ifndef WF_RSA_H
#define WF_RSA_H
#include <stdint.h>

/* 0 = valid. hash_id is WF_HASH_SHA256/384/512 (wf_sha512.h). */
int wf_rsa_pkcs1_verify(const uint8_t* n, int n_len, const uint8_t* e, int e_len,
                        int hash_id, const uint8_t* digest, int digest_len,
                        const uint8_t* sig, int sig_len);
/* RSASSA-PSS with MGF1 over the same hash, salt length = digest length. */
int wf_rsa_pss_verify(const uint8_t* n, int n_len, const uint8_t* e, int e_len,
                      int hash_id, const uint8_t* digest, int digest_len,
                      const uint8_t* sig, int sig_len);
#endif
