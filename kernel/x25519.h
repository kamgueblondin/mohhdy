#ifndef MOHHDY_X25519_H
#define MOHHDY_X25519_H
#include <stdint.h>

#define X25519_KEY_LENGTH 32U
#define X25519_WORKSPACE_LIMBS 136U

/* Calcule X25519(scalar, u) sur Curve25519. Le workspace contient au moins
 * X25519_WORKSPACE_LIMBS uint32_t alignés et appartient à l’appelant. */
int x25519_scalar_mult(uint8_t output[X25519_KEY_LENGTH],
                       const uint8_t scalar[X25519_KEY_LENGTH],
                       const uint8_t u[X25519_KEY_LENGTH],
                       uint32_t* workspace,uint16_t workspace_length);

int x25519_public_key(uint8_t output[X25519_KEY_LENGTH],
                      const uint8_t private_key[X25519_KEY_LENGTH],
                      uint32_t* workspace,uint16_t workspace_length);

/* Rejette les secrets tout-zéro, interdits par le contrat TLS de ce projet. */
int x25519_shared_secret(uint8_t output[X25519_KEY_LENGTH],
                         const uint8_t private_key[X25519_KEY_LENGTH],
                         const uint8_t peer_public[X25519_KEY_LENGTH],
                         uint32_t* workspace,uint16_t workspace_length);
/* Step-wise X25519 (P2P key agreement without stalling the caller's loop).
 * The job owns its workspace; x25519_job_step runs at most `budget` ladder
 * or inversion iterations (511 in total) and returns 1 when the result is
 * ready, 0 if more steps are needed, < 0 on error. Same result as
 * x25519_scalar_mult. The job must not move in memory once started. */
#include "bigint.h"
typedef struct {
    bigint_t v[17];
    uint32_t ws[X25519_WORKSPACE_LIMBS];
    uint8_t k[X25519_KEY_LENGTH];
    uint32_t swap;
    uint16_t bit, inv_bit;
    int phase;                 /* 1 ladder, 2 invert, 3 done */
} x25519_job_t;
int x25519_job_start(x25519_job_t* job, const uint8_t scalar[X25519_KEY_LENGTH], const uint8_t u[X25519_KEY_LENGTH]);
int x25519_job_step(x25519_job_t* job, int budget);
/* rejects the all-zero shared secret like x25519_shared_secret */
int x25519_job_result(x25519_job_t* job, uint8_t output[X25519_KEY_LENGTH]);
#define X25519_JOB_STEPS 511
#endif
