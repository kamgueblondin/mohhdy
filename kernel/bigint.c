#include "bigint.h"

static void bigint_normalize(bigint_t* value){
    while(value->length>0U&&value->limbs[value->length-1U]==0U)value->length--;
}

static void bigint_zero(bigint_t* value){
    uint16_t i;
    for(i=0U;i<value->capacity;i++)value->limbs[i]=0U;
    value->length=0U;
}

static void bigint_copy(bigint_t* output,const bigint_t* input){
    uint16_t i;
    for(i=0U;i<output->capacity;i++)output->limbs[i]=i<input->length?input->limbs[i]:0U;
    output->length=input->length;
}

int bigint_init(bigint_t* value,uint32_t* limbs,uint16_t capacity){
    if(!value||!limbs||capacity==0U)return -1;
    value->limbs=limbs;value->capacity=capacity;bigint_zero(value);return 0;
}

int bigint_from_be(bigint_t* value,const uint8_t* input,uint16_t length){
    uint16_t bytes,offset;
    if(!value||!input||length==0U)return -1;
    bytes=(uint16_t)((length+3U)/4U);
    if(bytes>value->capacity)return -2;
    bigint_zero(value);
    for(offset=0U;offset<length;offset++){
        uint16_t reverse=(uint16_t)(length-1U-offset);
        value->limbs[reverse/4U]|=(uint32_t)input[offset]<<((reverse&3U)*8U);
    }
    value->length=bytes;bigint_normalize(value);return 0;
}

int bigint_to_be(const bigint_t* value,uint8_t* output,uint16_t length){
    uint16_t i;
    if(!value||!output)return -1;
    for(i=0U;i<length;i++){
        uint16_t reverse=(uint16_t)(length-1U-i),limb=reverse/4U;
        output[i]=limb<value->length?(uint8_t)(value->limbs[limb]>>((reverse&3U)*8U)):0U;
    }
    return 0;
}

int bigint_compare(const bigint_t* left,const bigint_t* right){
    uint16_t i;
    if(!left||!right)return -2;
    if(left->length!=right->length)return left->length>right->length?1:-1;
    for(i=left->length;i>0U;i--){
        if(left->limbs[i-1U]!=right->limbs[i-1U])return left->limbs[i-1U]>right->limbs[i-1U]?1:-1;
    }
    return 0;
}

int bigint_add(bigint_t* output,const bigint_t* left,const bigint_t* right){
    uint16_t i,max;uint64_t carry=0U;
    if(!output||!left||!right||!output->limbs||!left->limbs||!right->limbs)return -1;
    max=left->length>right->length?left->length:right->length;
    if(output->capacity<max)return -2;
    for(i=0U;i<max;i++){
        uint64_t sum=carry+(i<left->length?left->limbs[i]:0U)+(i<right->length?right->limbs[i]:0U);
        output->limbs[i]=(uint32_t)sum;carry=sum>>32U;
    }
    if(carry){if(max>=output->capacity)return -3;output->limbs[max++]=(uint32_t)carry;}
    output->length=max;bigint_normalize(output);return 0;
}

int bigint_subtract(bigint_t* output,const bigint_t* left,const bigint_t* right){
    uint16_t i;uint64_t borrow=0U;
    if(!output||!left||!right||!output->limbs||!left->limbs||!right->limbs||bigint_compare(left,right)<0||output->capacity<left->length)return -1;
    for(i=0U;i<left->length;i++){
        uint64_t lv=left->limbs[i],rv=(i<right->length?right->limbs[i]:0U)+borrow;
        output->limbs[i]=(uint32_t)(lv-rv);borrow=lv<rv?1U:0U;
    }
    output->length=left->length;bigint_normalize(output);return 0;
}

int bigint_multiply(bigint_t* output,const bigint_t* left,const bigint_t* right){
    uint16_t i,j;
    if(!output||!left||!right||!output->limbs||!left->limbs||!right->limbs||output==left||output==right)return -1;
    if((uint32_t)left->length+right->length>output->capacity)return -2;
    bigint_zero(output);
    for(i=0U;i<left->length;i++){
        uint64_t carry=0U;
        for(j=0U;j<right->length;j++){
            uint64_t product=(uint64_t)left->limbs[i]*right->limbs[j]+output->limbs[i+j]+carry;
            output->limbs[i+j]=(uint32_t)product;carry=product>>32U;
        }
        output->limbs[i+right->length]=(uint32_t)carry;
    }
    output->length=(uint16_t)(left->length+right->length);bigint_normalize(output);return 0;
}

static int bigint_raw_compare(const uint32_t* left,const uint32_t* right,uint16_t length){
    uint16_t i;
    for(i=length;i>0U;i--){
        if(left[i-1U]!=right[i-1U])return left[i-1U]>right[i-1U]?1:-1;
    }
    return 0;
}

static void bigint_raw_subtract(uint32_t* left,const uint32_t* right,uint16_t length){
    uint16_t i;uint64_t borrow=0U;
    for(i=0U;i<length;i++){
        uint64_t value=(uint64_t)right[i]+borrow,current=left[i];
        left[i]=(uint32_t)(current-value);borrow=current<value?1U:0U;
    }
}

/* Ajoute un bit à un reste inférieur au module. Le dépassement 2^N est éliminé
 * par une soustraction du module; le reste est toujours strictement inférieur au module. */
static void bigint_mod_shift_add(bigint_t* remainder,const bigint_t* modulus,uint8_t bit){
    uint16_t i;uint32_t carry=bit;
    for(i=0U;i<modulus->length;i++){
        uint32_t next=remainder->limbs[i]>>31U;
        remainder->limbs[i]=(remainder->limbs[i]<<1U)|carry;carry=next;
    }
    if(carry||bigint_raw_compare(remainder->limbs,modulus->limbs,modulus->length)>=0)bigint_raw_subtract(remainder->limbs,modulus->limbs,modulus->length);
    remainder->length=modulus->length;bigint_normalize(remainder);
}

int bigint_mod_reduce(bigint_t* output,const bigint_t* input,const bigint_t* modulus){
    uint16_t i,bit;
    if(!output||!input||!modulus||modulus->length==0U||output->capacity<modulus->length)return -1;
    bigint_zero(output);
    for(i=input->length;i>0U;i--)for(bit=32U;bit>0U;bit--)bigint_mod_shift_add(output,modulus,(uint8_t)((input->limbs[i-1U]>>(bit-1U))&1U));
    return 0;
}

int bigint_mod_add(bigint_t* output,const bigint_t* left,const bigint_t* right,const bigint_t* modulus){
    uint16_t i;uint64_t carry=0U;
    if(output->capacity<modulus->length)return -1;
    for(i=0U;i<modulus->length;i++){
        uint64_t sum=carry+(i<left->length?left->limbs[i]:0U)+(i<right->length?right->limbs[i]:0U);
        output->limbs[i]=(uint32_t)sum;carry=sum>>32U;
    }
    if(carry||bigint_raw_compare(output->limbs,modulus->limbs,modulus->length)>=0)bigint_raw_subtract(output->limbs,modulus->limbs,modulus->length);
    output->length=modulus->length;bigint_normalize(output);return 0;
}

static int bigint_mod_add_ct_masked(bigint_t* output,const bigint_t* left,const bigint_t* right,const bigint_t* modulus,uint32_t mask){uint16_t i;uint64_t carry=0U,borrow=0U;uint32_t keep_sub,restore_mask;if(!output||!left||!right||!modulus||modulus->length==0U||output->capacity<modulus->length||left->capacity<modulus->length||right->capacity<modulus->length)return -1;for(i=0U;i<modulus->length;i++){uint64_t sum=(uint64_t)left->limbs[i]+(right->limbs[i]&mask)+carry;output->limbs[i]=(uint32_t)sum;carry=sum>>32U;}for(i=0U;i<modulus->length;i++){uint64_t value=(uint64_t)modulus->limbs[i]+borrow,current=output->limbs[i];output->limbs[i]=(uint32_t)(current-value);borrow=current<value?1U:0U;}keep_sub=(uint32_t)carry|((uint32_t)borrow^1U);restore_mask=0U-(keep_sub^1U);carry=0U;for(i=0U;i<modulus->length;i++){uint64_t sum=(uint64_t)output->limbs[i]+(modulus->limbs[i]&restore_mask)+carry;output->limbs[i]=(uint32_t)sum;carry=sum>>32U;}output->length=modulus->length;return 0;}
int bigint_mod_add_ct(bigint_t* output,const bigint_t* left,const bigint_t* right,const bigint_t* modulus){return bigint_mod_add_ct_masked(output,left,right,modulus,0xffffffffU);}
int bigint_mod_subtract_ct(bigint_t* output,const bigint_t* left,const bigint_t* right,const bigint_t* modulus){uint16_t i;uint64_t borrow=0U,carry=0U;uint32_t mask;if(!output||!left||!right||!modulus||modulus->length==0U||output->capacity<modulus->length||left->capacity<modulus->length||right->capacity<modulus->length)return -1;for(i=0U;i<modulus->length;i++){uint64_t value=(uint64_t)right->limbs[i]+borrow,current=left->limbs[i];output->limbs[i]=(uint32_t)(current-value);borrow=current<value?1U:0U;}mask=0U-(uint32_t)borrow;for(i=0U;i<modulus->length;i++){uint64_t sum=(uint64_t)output->limbs[i]+(modulus->limbs[i]&mask)+carry;output->limbs[i]=(uint32_t)sum;carry=sum>>32U;}output->length=modulus->length;return 0;}
int bigint_mod_multiply_ct(bigint_t* output,const bigint_t* left,const bigint_t* right,const bigint_t* modulus,bigint_t* temporary){uint16_t i,bit;if(!output||!left||!right||!modulus||!temporary||modulus->length==0U||output->capacity<modulus->length||left->capacity<modulus->length||right->capacity<modulus->length||temporary->capacity<modulus->length)return -1;for(i=0U;i<modulus->length;i++){output->limbs[i]=0U;temporary->limbs[i]=left->limbs[i];}output->length=modulus->length;temporary->length=modulus->length;for(i=0U;i<modulus->length;i++)for(bit=0U;bit<32U;bit++){uint32_t mask=0U-((right->limbs[i]>>bit)&1U);if(bigint_mod_add_ct_masked(output,output,temporary,modulus,mask)!=0)return -2;if(bigint_mod_add_ct(temporary,temporary,temporary,modulus)!=0)return -3;}return 0;}

/* Produit modulaire par doublement et addition : lent mais borné, portable i386 et sans heap. */
int bigint_mod_multiply(bigint_t* output,const bigint_t* left,const bigint_t* right,const bigint_t* modulus,bigint_t* temporary){
    uint16_t i,bit;
    if(!output||!left||!right||!modulus||!temporary||output->capacity<modulus->length||temporary->capacity<modulus->length)return -1;
    bigint_zero(output);
    if(bigint_mod_reduce(temporary,left,modulus)!=0)return -2;
    for(i=0U;i<right->length;i++)for(bit=0U;bit<32U;bit++){
        if((right->limbs[i]>>bit)&1U)if(bigint_mod_add(output,output,temporary,modulus)!=0)return -3;
        if(bigint_mod_add(temporary,temporary,temporary,modulus)!=0)return -4;
    }
    return 0;
}

int bigint_modexp(bigint_t* output,const bigint_t* base,const bigint_t* exponent,const bigint_t* modulus,uint32_t* workspace,uint16_t workspace_length){
    uint16_t capacity,index,bit; bigint_t result,current,product,temporary;
    if(!output||!base||!exponent||!modulus||!workspace||modulus->length==0U)return -1;
    capacity=modulus->length;
    if(output->capacity<capacity||workspace_length<(uint16_t)(4U*capacity))return -2;
    if(bigint_init(&result,workspace,capacity)!=0||bigint_init(&current,workspace+capacity,capacity)!=0||bigint_init(&product,workspace+(uint16_t)(2U*capacity),capacity)!=0||bigint_init(&temporary,workspace+(uint16_t)(3U*capacity),capacity)!=0)return -3;
    if(modulus->length==1U&&modulus->limbs[0]==1U)result.length=0U;else{result.limbs[0]=1U;result.length=1U;}
    if(bigint_mod_reduce(&current,base,modulus)!=0)return -4;
    for(index=exponent->length;index>0U;index--){
        uint32_t limb=exponent->limbs[index-1U];
        for(bit=32U;bit>0U;bit--){
            if(bigint_mod_multiply(&product,&result,&result,modulus,&temporary)!=0)return -5;
            bigint_copy(&result,&product);
            if((limb>>(bit-1U))&1U){
                if(bigint_mod_multiply(&product,&result,&current,modulus,&temporary)!=0)return -6;
                bigint_copy(&result,&product);
            }
        }
    }
    bigint_zero(output);
    for(index=0U;index<result.length;index++)output->limbs[index]=result.limbs[index];
    output->length=result.length;
    return 0;
}

int bigint_modexp_u32(bigint_t* output,const bigint_t* base,uint32_t exponent,const bigint_t* modulus,uint32_t* workspace,uint16_t workspace_length){
    uint16_t capacity,i;bigint_t result,current,product,temporary;
    if(!output||!base||!modulus||!workspace||modulus->length==0U)return -1;
    capacity=modulus->length;
    if(output->capacity<capacity||workspace_length<(uint16_t)(4U*capacity))return -2;
    if(bigint_init(&result,workspace,capacity)!=0||bigint_init(&current,workspace+capacity,capacity)!=0||bigint_init(&product,workspace+(uint16_t)(2U*capacity),capacity)!=0||bigint_init(&temporary,workspace+(uint16_t)(3U*capacity),capacity)!=0)return -3;
    if(modulus->length==1U&&modulus->limbs[0]==1U)result.length=0U;else{result.limbs[0]=1U;result.length=1U;}
    if(bigint_mod_reduce(&current,base,modulus)!=0)return -4;
    while(exponent){
        if(exponent&1U){if(bigint_mod_multiply(&product,&result,&current,modulus,&temporary)!=0)return -5;bigint_copy(&result,&product);}
        exponent>>=1U;
        if(exponent){if(bigint_mod_multiply(&product,&current,&current,modulus,&temporary)!=0)return -6;bigint_copy(&current,&product);}
    }
    bigint_zero(output);
    for(i=0U;i<result.length;i++)output->limbs[i]=result.limbs[i];
    output->length=result.length;return 0;
}
