#include "x25519.h"
#include "bigint.h"

#define X25519_LIMBS 8U
#define X25519_ELEMENTS 17U

static const uint8_t x25519_prime_be[X25519_KEY_LENGTH]={
    0x7fU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,
    0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0xedU
};

static void x25519_copy(bigint_t* output,const bigint_t* input){
    uint16_t i;
    for(i=0U;i<output->capacity;i++)output->limbs[i]=input->limbs[i];
    output->length=output->capacity;
}

static void x25519_swap(bigint_t* left,bigint_t* right,uint32_t swap){
    uint16_t i;uint32_t mask=0U-swap;
    for(i=0U;i<left->capacity;i++){uint32_t difference=mask&(left->limbs[i]^right->limbs[i]);left->limbs[i]^=difference;right->limbs[i]^=difference;}
}

static int x25519_subtract(bigint_t* output,const bigint_t* left,const bigint_t* right,const bigint_t* modulus){return bigint_mod_subtract_ct(output,left,right,modulus);}

static int x25519_small_multiply(bigint_t* output,const bigint_t* input,uint32_t multiplier,const bigint_t* modulus,bigint_t* product,bigint_t* temporary){
    uint16_t bit;
    if(!output||!input||!product||!temporary)return -1;
    for(bit=0U;bit<product->capacity;bit++)product->limbs[bit]=0U;
    product->length=0U;
    if(bigint_mod_reduce(temporary,input,modulus)!=0)return -2;
    for(bit=0U;bit<17U;bit++){
        if((multiplier>>bit)&1U)if(bigint_mod_add_ct(product,product,temporary,modulus)!=0)return -3;
        if(bit<16U&&bigint_mod_add_ct(temporary,temporary,temporary,modulus)!=0)return -4;
    }
    x25519_copy(output,product);return 0;
}

static int x25519_invert(bigint_t* output,const bigint_t* input,const bigint_t* modulus,bigint_t* result,bigint_t* product,bigint_t* temporary){
    uint16_t bit;uint32_t limb;
    for(limb=0U;limb<X25519_LIMBS;limb++)result->limbs[limb]=0U;
    result->limbs[0]=1U;result->length=1U;
    /* p - 2 = 2^255 - 21 : parcours fixe de 255 bits. */
    for(bit=256U;bit>0U;bit--){
        uint16_t bit_index=(uint16_t)(bit-1U);
        if(bigint_mod_multiply_ct(product,result,result,modulus,temporary)!=0)return -1;
        x25519_copy(result,product);
        if((bit_index>=5U&&bit_index<=254U)||bit_index==3U||bit_index==1U||bit_index==0U){if(bigint_mod_multiply_ct(product,result,input,modulus,temporary)!=0)return -2;x25519_copy(result,product);}
    }
    x25519_copy(output,result);return 0;
}

int x25519_scalar_mult(uint8_t output[X25519_KEY_LENGTH],const uint8_t scalar[X25519_KEY_LENGTH],const uint8_t u[X25519_KEY_LENGTH],uint32_t* workspace,uint16_t workspace_length){
    uint8_t k[X25519_KEY_LENGTH],encoded_u[X25519_KEY_LENGTH],encoded_output[X25519_KEY_LENGTH];
    uint16_t i,bit;uint32_t swap=0U;
    bigint_t p,x1,x2,z2,x3,z3,a,aa,b,bb,e,c,d,da,cb,t0,t1,temporary,product;
    if(!output||!scalar||!u||!workspace||workspace_length<X25519_WORKSPACE_LIMBS)return -1;
    if(bigint_init(&p,workspace+0U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&x1,workspace+1U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&x2,workspace+2U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&z2,workspace+3U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&x3,workspace+4U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&z3,workspace+5U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&a,workspace+6U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&aa,workspace+7U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&b,workspace+8U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&bb,workspace+9U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&e,workspace+10U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&c,workspace+11U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&d,workspace+12U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&da,workspace+13U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&cb,workspace+14U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&t0,workspace+15U*X25519_LIMBS,X25519_LIMBS)!=0||bigint_init(&t1,workspace+16U*X25519_LIMBS,X25519_LIMBS)!=0)return -2;
    temporary=t1;product=da;
    if(bigint_from_be(&p,x25519_prime_be,sizeof(x25519_prime_be))!=0)return -3;
    for(i=0U;i<X25519_KEY_LENGTH;i++){k[i]=scalar[i];encoded_u[i]=u[X25519_KEY_LENGTH-1U-i];}
    k[0]&=248U;k[31]&=127U;k[31]|=64U;encoded_u[0]&=127U;
    if(bigint_from_be(&x1,encoded_u,sizeof(encoded_u))!=0||bigint_mod_reduce(&t0,&x1,&p)!=0)return -4;
    x25519_copy(&x1,&t0);
    x1.length=X25519_LIMBS;x2.limbs[0]=1U;x2.length=X25519_LIMBS;z2.length=X25519_LIMBS;x3.length=X25519_LIMBS;z3.limbs[0]=1U;z3.length=X25519_LIMBS;x25519_copy(&x3,&x1);
    for(bit=255U;bit>0U;bit--){
        uint32_t current=(uint32_t)((k[(bit-1U)/8U]>>((bit-1U)&7U))&1U);
        swap^=current;x25519_swap(&x2,&x3,swap);x25519_swap(&z2,&z3,swap);swap=current;
        if(bigint_mod_add_ct(&a,&x2,&z2,&p)!=0||bigint_mod_multiply_ct(&aa,&a,&a,&p,&temporary)!=0||x25519_subtract(&b,&x2,&z2,&p)!=0||bigint_mod_multiply_ct(&bb,&b,&b,&p,&temporary)!=0||x25519_subtract(&e,&aa,&bb,&p)!=0||bigint_mod_add_ct(&c,&x3,&z3,&p)!=0||x25519_subtract(&d,&x3,&z3,&p)!=0||bigint_mod_multiply_ct(&da,&d,&a,&p,&temporary)!=0||bigint_mod_multiply_ct(&cb,&c,&b,&p,&temporary)!=0)return -5;
        if(bigint_mod_add_ct(&t0,&da,&cb,&p)!=0||bigint_mod_multiply_ct(&x3,&t0,&t0,&p,&temporary)!=0||x25519_subtract(&t0,&da,&cb,&p)!=0||bigint_mod_multiply_ct(&da,&t0,&t0,&p,&temporary)!=0||bigint_mod_multiply_ct(&z3,&x1,&da,&p,&temporary)!=0||bigint_mod_multiply_ct(&x2,&aa,&bb,&p,&temporary)!=0||x25519_small_multiply(&t0,&e,121665U,&p,&product,&temporary)!=0||bigint_mod_add_ct(&t0,&aa,&t0,&p)!=0||bigint_mod_multiply_ct(&z2,&e,&t0,&p,&temporary)!=0)return -6;
    }
    x25519_swap(&x2,&x3,swap);x25519_swap(&z2,&z3,swap);
    if(x25519_invert(&z2,&z2,&p,&a,&t0,&t1)!=0||bigint_mod_multiply_ct(&t0,&x2,&z2,&p,&temporary)!=0)return -7;
    x25519_copy(&x2,&t0);
    if(bigint_to_be(&x2,encoded_output,sizeof(encoded_output))!=0)return -8;
    for(i=0U;i<X25519_KEY_LENGTH;i++)output[i]=encoded_output[X25519_KEY_LENGTH-1U-i];
    return 0;
}

int x25519_public_key(uint8_t output[X25519_KEY_LENGTH],const uint8_t private_key[X25519_KEY_LENGTH],uint32_t* workspace,uint16_t workspace_length){
    uint8_t base[X25519_KEY_LENGTH]={9U};
    return x25519_scalar_mult(output,private_key,base,workspace,workspace_length);
}

int x25519_shared_secret(uint8_t output[X25519_KEY_LENGTH],const uint8_t private_key[X25519_KEY_LENGTH],const uint8_t peer_public[X25519_KEY_LENGTH],uint32_t* workspace,uint16_t workspace_length){
    uint8_t difference=0U;uint16_t i;int status=x25519_scalar_mult(output,private_key,peer_public,workspace,workspace_length);
    if(status!=0)return status;
    for(i=0U;i<X25519_KEY_LENGTH;i++)difference|=output[i];
    return difference==0U?-9:0;
}
