#include "sha256.h"
static const uint32_t K[64]={0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U};
static uint32_t ror(uint32_t x,uint32_t n){return (x>>n)|(x<<(32U-n));}
static void transform(sha256_ctx_t*c,const uint8_t*b){uint32_t w[64],a,bb,cc,d,e,f,g,h,t1,t2;uint32_t i;for(i=0;i<16;i++)w[i]=((uint32_t)b[4*i]<<24)|((uint32_t)b[4*i+1]<<16)|((uint32_t)b[4*i+2]<<8)|b[4*i+3];for(i=16;i<64;i++){uint32_t s0=ror(w[i-15],7)^ror(w[i-15],18)^(w[i-15]>>3);uint32_t s1=ror(w[i-2],17)^ror(w[i-2],19)^(w[i-2]>>10);w[i]=w[i-16]+s0+w[i-7]+s1;}a=c->state[0];bb=c->state[1];cc=c->state[2];d=c->state[3];e=c->state[4];f=c->state[5];g=c->state[6];h=c->state[7];for(i=0;i<64;i++){uint32_t S1=ror(e,6)^ror(e,11)^ror(e,25);uint32_t ch=(e&f)^((~e)&g);t1=h+S1+ch+K[i]+w[i];uint32_t S0=ror(a,2)^ror(a,13)^ror(a,22);uint32_t maj=(a&bb)^(a&cc)^(bb&cc);t2=S0+maj;h=g;g=f;f=e;e=d+t1;d=cc;cc=bb;bb=a;a=t1+t2;}c->state[0]+=a;c->state[1]+=bb;c->state[2]+=cc;c->state[3]+=d;c->state[4]+=e;c->state[5]+=f;c->state[6]+=g;c->state[7]+=h;}
void sha256_init(sha256_ctx_t*c){static const uint32_t s[8]={0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U};uint32_t i;if(!c)return;for(i=0;i<8;i++)c->state[i]=s[i];c->bit_count=0;c->block_len=0;}
void sha256_update(sha256_ctx_t*c,const uint8_t*d,uint32_t n){uint32_t take;if(!c||(!d&&n))return;c->bit_count+=(uint64_t)n*8U;while(n){take=64U-c->block_len;if(take>n)take=n;{uint32_t i;for(i=0;i<take;i++)c->block[c->block_len+i]=d[i];}c->block_len+=take;d+=take;n-=take;if(c->block_len==64U){transform(c,c->block);c->block_len=0;}}}
void sha256_final(sha256_ctx_t*c,uint8_t digest[32]){uint32_t i;uint64_t bits;if(!c||!digest)return;bits=c->bit_count;c->block[c->block_len++]=0x80U;while(c->block_len!=56U){if(c->block_len==64U){transform(c,c->block);c->block_len=0;}else c->block[c->block_len++]=0U;}for(i=0;i<8;i++)c->block[56+i]=(uint8_t)(bits>>(56U-8U*i));transform(c,c->block);for(i=0;i<8;i++){digest[4*i]=(uint8_t)(c->state[i]>>24);digest[4*i+1]=(uint8_t)(c->state[i]>>16);digest[4*i+2]=(uint8_t)(c->state[i]>>8);digest[4*i+3]=(uint8_t)c->state[i];}}

void hmac_sha256(const uint8_t* key, uint32_t key_length,
                 const uint8_t* message, uint32_t message_length,
                 uint8_t digest[32]) {
    uint8_t block[64], inner[32];
    sha256_ctx_t ctx;
    uint32_t i;
    if ((!key && key_length) || (!message && message_length) || !digest) return;
    for (i = 0; i < 64U; ++i) block[i] = 0U;
    if (key_length > 64U) {
        sha256_init(&ctx); sha256_update(&ctx, key, key_length); sha256_final(&ctx, block);
    } else {
        for (i = 0; i < key_length; ++i) block[i] = key[i];
    }
    for (i = 0; i < 64U; ++i) block[i] ^= 0x36U;
    sha256_init(&ctx); sha256_update(&ctx, block, 64U); sha256_update(&ctx, message, message_length); sha256_final(&ctx, inner);
    for (i = 0; i < 64U; ++i) block[i] ^= (uint8_t)(0x36U ^ 0x5cU);
    sha256_init(&ctx); sha256_update(&ctx, block, 64U); sha256_update(&ctx, inner, 32U); sha256_final(&ctx, digest);
}
