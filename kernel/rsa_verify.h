#ifndef MOHHDY_RSA_VERIFY_H
#define MOHHDY_RSA_VERIFY_H
#include <stdint.h>

/* Vérifie une signature RSA PKCS#1 v1.5 SHA-256.
 * `workspace` appartient à l'appelant, est aligné sur uint32_t et contient au
 * au moins 7 * ceil(modulus_length / 4) limbs : module, signature, résultat et quatre temporaires bigint. */
int rsa_pkcs1_v15_sha256_verify(const uint8_t* modulus,uint16_t modulus_length,
                                const uint8_t* exponent,uint16_t exponent_length,
                                const uint8_t digest[32],const uint8_t* signature,
                                uint16_t signature_length,uint32_t* workspace,
                                uint16_t workspace_length);
#endif
