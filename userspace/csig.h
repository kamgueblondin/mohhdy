/* userspace/csig.h - per-node Schnorr signatures (RFC 5114 1024/160 group). */
#ifndef MOHHDY_CSIG_H
#define MOHHDY_CSIG_H
#include <stdint.h>
#define CSIG_SK 20
#define CSIG_PK 128
#define CSIG_SIG 40
int csig_keypair(const uint8_t seed[32], uint8_t sk[CSIG_SK], uint8_t pk[CSIG_PK]);
int csig_sign(const uint8_t sk[CSIG_SK], const uint8_t* msg, int len, uint8_t sig[CSIG_SIG]);
/* 1 valid, 0 invalid */
int csig_verify(const uint8_t pk[CSIG_PK], const uint8_t* msg, int len, const uint8_t sig[CSIG_SIG]);
/* 1 if 1 < y < p and y^q = 1 (y in the prime-order subgroup) */
int csig_pk_valid(const uint8_t pk[CSIG_PK]);
#endif
