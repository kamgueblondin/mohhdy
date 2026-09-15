#ifndef MOHHDY_ECDSA_P256_H
#define MOHHDY_ECDSA_P256_H

#include <stdint.h>

#define ECDSA_P256_LIMBS 8U
#define ECDSA_P256_PUBLIC_KEY_LENGTH 65U
#define ECDSA_P256_SHA256_LENGTH 32U
#define ECDSA_P256_DER_SIGNATURE_MAX 72U
#define ECDSA_P256_WORKSPACE_WORDS 2048U

/*
 * Vérifie une signature ECDSA SHA-256 de P-256.
 *
 * La clé est un point SEC1 non compressé de 65 octets (`0x04 || X || Y`).
 * La signature est une SEQUENCE DER canonique de deux INTEGER positifs r,s.
 * `workspace` est exclusivement détenu par l'appelant et doit contenir au
 * moins ECDSA_P256_WORKSPACE_WORDS mots. Aucune allocation dynamique ni état
 * cryptographique global n'est utilisé.
 *
 * Retourne 0 si la signature est valide, une valeur négative sinon.
 */
int ecdsa_p256_sha256_verify(const uint8_t public_key[ECDSA_P256_PUBLIC_KEY_LENGTH],
                             const uint8_t hash[ECDSA_P256_SHA256_LENGTH],
                             const uint8_t* der_signature, uint16_t der_signature_length,
                             uint32_t* workspace, uint16_t workspace_words);

#endif
