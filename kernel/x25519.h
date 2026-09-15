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
#endif
