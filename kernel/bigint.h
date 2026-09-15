#ifndef MOHHDY_BIGINT_H
#define MOHHDY_BIGINT_H
#include <stdint.h>
typedef struct { uint32_t* limbs; uint16_t capacity; uint16_t length; } bigint_t;
int bigint_init(bigint_t* value,uint32_t* limbs,uint16_t capacity);
int bigint_from_be(bigint_t* value,const uint8_t* input,uint16_t length);
int bigint_to_be(const bigint_t* value,uint8_t* output,uint16_t length);
int bigint_compare(const bigint_t* left,const bigint_t* right);
int bigint_add(bigint_t* output,const bigint_t* left,const bigint_t* right);
int bigint_subtract(bigint_t* output,const bigint_t* left,const bigint_t* right);
int bigint_multiply(bigint_t* output,const bigint_t* left,const bigint_t* right);
int bigint_mod_reduce(bigint_t* output,const bigint_t* input,const bigint_t* modulus);
int bigint_mod_add(bigint_t* output,const bigint_t* left,const bigint_t* right,const bigint_t* modulus);
int bigint_mod_multiply(bigint_t* output,const bigint_t* left,const bigint_t* right,const bigint_t* modulus,bigint_t* temporary);
/* Variantes à largeur fixe : les limbs jusqu’à modulus->length doivent être initialisés par l’appelant. */
int bigint_mod_add_ct(bigint_t* output,const bigint_t* left,const bigint_t* right,const bigint_t* modulus);
int bigint_mod_subtract_ct(bigint_t* output,const bigint_t* left,const bigint_t* right,const bigint_t* modulus);
int bigint_mod_multiply_ct(bigint_t* output,const bigint_t* left,const bigint_t* right,const bigint_t* modulus,bigint_t* temporary);
/* workspace contient au moins 4 * modulus->capacity limbs : résultat, base réduite, produit et quotient. */
int bigint_modexp(bigint_t* output,const bigint_t* base,const bigint_t* exponent,const bigint_t* modulus,uint32_t* workspace,uint16_t workspace_length);
/* Variante pour les exposants courts conservée pour les appelants existants. */
int bigint_modexp_u32(bigint_t* output,const bigint_t* base,uint32_t exponent,const bigint_t* modulus,uint32_t* workspace,uint16_t workspace_length);
#endif
