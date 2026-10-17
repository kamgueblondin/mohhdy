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
/* Lot 9 (US-103): Schnorr NIZK proof of knowledge in a 2048/256 group.
 * group_check returns 1 when q | p-1, g^q = 1, g != 1 and 2^(p-1) = 1. */
#define CSIG_ZK_SK 32
#define CSIG_ZK_PK 256
#define CSIG_ZK_PROOF 288
int csig_zk_group_check(void);
int csig_zk_keypair(const uint8_t seed[32], uint8_t sk[CSIG_ZK_SK], uint8_t pk[CSIG_ZK_PK]);
int csig_zk_prove(const uint8_t sk[CSIG_ZK_SK], const uint8_t* msg, int len, uint8_t proof[CSIG_ZK_PROOF]);
/* 1 valid, 0 invalid */
int csig_zk_verify(const uint8_t pk[CSIG_ZK_PK], const uint8_t* msg, int len, const uint8_t proof[CSIG_ZK_PROOF]);
#endif
