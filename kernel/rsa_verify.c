#include "rsa_verify.h"
#include "bigint.h"

#define RSA_SHA256_DIGEST_INFO_LENGTH 19U

static const uint8_t rsa_sha256_digest_info[RSA_SHA256_DIGEST_INFO_LENGTH]={
    0x30U,0x31U,0x30U,0x0dU,0x06U,0x09U,0x60U,0x86U,0x48U,0x01U,
    0x65U,0x03U,0x04U,0x02U,0x01U,0x05U,0x00U,0x04U,0x20U
};

static uint32_t rsa_exponent_u32(const uint8_t* exponent,uint16_t length,int* ok){
    uint16_t i;uint32_t value=0U;
    while(length>1U&&*exponent==0U){exponent++;length--;}
    if(length==0U||length>4U){*ok=0;return 0U;}
    for(i=0U;i<length;i++)value=(value<<8U)|exponent[i];
    *ok=value!=0U;return value;
}

int rsa_pkcs1_v15_sha256_verify(const uint8_t* modulus,uint16_t modulus_length,
                                const uint8_t* exponent,uint16_t exponent_length,
                                const uint8_t digest[32],const uint8_t* signature,
                                uint16_t signature_length,uint32_t* workspace,
                                uint16_t workspace_length){
    uint16_t limbs,encoded_offset,i,padding_length;uint8_t difference=0U,*recovered;
    uint32_t public_exponent;int exponent_ok;
    bigint_t n,s,encoded;
    if(!modulus||!exponent||!digest||!signature||!workspace||modulus_length==0U)return -1;
    while(modulus_length>1U&&*modulus==0U){modulus++;modulus_length--;}
    if(signature_length!=modulus_length)return -2;
    limbs=(uint16_t)((modulus_length+3U)/4U);
    if(limbs==0U||workspace_length<(uint16_t)(7U*limbs))return -3;
    public_exponent=rsa_exponent_u32(exponent,exponent_length,&exponent_ok);
    if(!exponent_ok)return -4;
    if(bigint_init(&n,workspace,limbs)!=0||bigint_init(&s,workspace+limbs,limbs)!=0||bigint_init(&encoded,workspace+(uint16_t)(2U*limbs),limbs)!=0)return -5;
    if(bigint_from_be(&n,modulus,modulus_length)!=0||n.length==0U)return -6;
    if(bigint_from_be(&s,signature,signature_length)!=0||bigint_compare(&s,&n)>=0)return -7;
    if(bigint_modexp_u32(&encoded,&s,public_exponent,&n,workspace+(uint16_t)(3U*limbs),(uint16_t)(4U*limbs))!=0)return -8;
    recovered=(uint8_t*)(workspace+(uint16_t)(3U*limbs));
    if(bigint_to_be(&encoded,recovered,signature_length)!=0)return -9;
    if(modulus_length<11U+RSA_SHA256_DIGEST_INFO_LENGTH+32U)return -10;
    encoded_offset=(uint16_t)(modulus_length-(RSA_SHA256_DIGEST_INFO_LENGTH+32U));
    padding_length=(uint16_t)(encoded_offset-3U);
    difference|=recovered[0];
    difference|=(uint8_t)(recovered[1]^0x01U);
    for(i=0U;i<padding_length;i++)difference|=(uint8_t)(recovered[2U+i]^0xffU);
    difference|=recovered[2U+padding_length];
    for(i=0U;i<RSA_SHA256_DIGEST_INFO_LENGTH;i++)difference|=(uint8_t)(recovered[encoded_offset+i]^rsa_sha256_digest_info[i]);
    for(i=0U;i<32U;i++)difference|=(uint8_t)(recovered[encoded_offset+RSA_SHA256_DIGEST_INFO_LENGTH+i]^digest[i]);
    return difference==0U?0:-11;
}
