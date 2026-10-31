/* wf_ec.h - Lot 67: ECDSA signature verification on NIST P-256 and P-384
 * (short Weierstrass, a = -3), Jacobian coordinates in Montgomery form.
 * Public data only (verification), so not constant time. */
#ifndef WF_EC_H
#define WF_EC_H
#include <stdint.h>
#include "wf_bn.h"

#define WF_CURVE_P256 1
#define WF_CURVE_P384 2

/* Byte length of a field element / scalar for the curve, 0 if unknown. */
int wf_ec_size(int curve);
/* Checks that the uncompressed point 04||X||Y is on the curve. 0 = ok. */
int wf_ec_point_check(int curve, const uint8_t* pub, int pub_len);
/* Verifies an ECDSA signature (r, s big-endian, any length up to the
 * scalar size) over a message digest. pub is 04||X||Y. 0 = valid. */
int wf_ecdsa_verify(int curve, const uint8_t* pub, int pub_len,
                    const uint8_t* digest, int digest_len,
                    const uint8_t* r, int r_len, const uint8_t* s, int s_len);
/* DER Ecdsa-Sig-Value (SEQUENCE { INTEGER r, INTEGER s }) variant. */
int wf_ecdsa_verify_der(int curve, const uint8_t* pub, int pub_len,
                        const uint8_t* digest, int digest_len,
                        const uint8_t* sig, int sig_len);
#endif
