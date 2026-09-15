#include "ecdsa_p256.h"
#include "bigint.h"

#define P256_BYTES 32U

typedef struct { uint32_t limb[ECDSA_P256_LIMBS]; } p256_num_t;
typedef struct { p256_num_t x; p256_num_t y; p256_num_t z; } p256_point_t;

/* Valeurs en little-endian par mots de 32 bits. */
static const p256_num_t p256_prime = {{0xffffffffU,0xffffffffU,0xffffffffU,0x00000000U,0x00000000U,0x00000000U,0x00000001U,0xffffffffU}};
static const p256_num_t p256_order = {{0xfc632551U,0xf3b9cac2U,0xa7179e84U,0xbce6faadU,0xffffffffU,0xffffffffU,0x00000000U,0xffffffffU}};
static const p256_num_t p256_b = {{0x27d2604bU,0x3bce3c3eU,0xcc53b0f6U,0x651d06b0U,0x769886bcU,0xb3ebbd55U,0xaa3a93e7U,0x5ac635d8U}};
static const p256_num_t p256_gx = {{0xd898c296U,0xf4a13945U,0x2deb33a0U,0x77037d81U,0x63a440f2U,0xf8bce6e5U,0xe12c4247U,0x6b17d1f2U}};
static const p256_num_t p256_gy = {{0x37bf51f5U,0xcbb64068U,0x6b315eceU,0x2bce3357U,0x7c0f9e16U,0x8ee7eb4aU,0xfe1a7f9bU,0x4fe342e2U}};
static const p256_num_t p256_p_minus_2 = {{0xfffffffdU,0xffffffffU,0xffffffffU,0x00000000U,0x00000000U,0x00000000U,0x00000001U,0xffffffffU}};
static const p256_num_t p256_n_minus_2 = {{0xfc63254fU,0xf3b9cac2U,0xa7179e84U,0xbce6faadU,0xffffffffU,0xffffffffU,0x00000000U,0xffffffffU}};

static void p256_zero(p256_num_t* value) { uint16_t i; for (i=0U;i<ECDSA_P256_LIMBS;++i) value->limb[i]=0U; }
static void p256_copy(p256_num_t* out,const p256_num_t* in) { uint16_t i; for (i=0U;i<ECDSA_P256_LIMBS;++i) out->limb[i]=in->limb[i]; }
static void p256_one(p256_num_t* value) { p256_zero(value); value->limb[0]=1U; }
static int p256_zero_p(const p256_num_t* value) { uint16_t i; uint32_t any=0U; for(i=0U;i<ECDSA_P256_LIMBS;++i) any|=value->limb[i]; return any==0U; }
static int p256_cmp(const p256_num_t* left,const p256_num_t* right) { int i; for(i=(int)ECDSA_P256_LIMBS-1;i>=0;--i){if(left->limb[i]!=right->limb[i])return left->limb[i]>right->limb[i]?1:-1;} return 0; }
static uint8_t p256_bit(const p256_num_t* value,uint16_t bit) { return (uint8_t)((value->limb[bit/32U]>>(bit%32U))&1U); }

/* Crée une vue bigint sur les limbs P-256 sans les initialiser : bigint_init()
 * remettrait les huit mots à zéro et détruirait les opérandes de courbe. */
static void p256_bigint(bigint_t* out,p256_num_t* value) {
    out->limbs=value->limb;
    out->capacity=ECDSA_P256_LIMBS;
    out->length=ECDSA_P256_LIMBS;
}
static int p256_add(p256_num_t* out,const p256_num_t* left,const p256_num_t* right,const p256_num_t* modulus) { bigint_t o,l,r,m; p256_bigint(&o,out);p256_bigint(&l,(p256_num_t*)left);p256_bigint(&r,(p256_num_t*)right);p256_bigint(&m,(p256_num_t*)modulus);return bigint_mod_add_ct(&o,&l,&r,&m); }
static int p256_sub(p256_num_t* out,const p256_num_t* left,const p256_num_t* right,const p256_num_t* modulus) { bigint_t o,l,r,m; p256_bigint(&o,out);p256_bigint(&l,(p256_num_t*)left);p256_bigint(&r,(p256_num_t*)right);p256_bigint(&m,(p256_num_t*)modulus);return bigint_mod_subtract_ct(&o,&l,&r,&m); }
static int p256_mul(p256_num_t* out,const p256_num_t* left,const p256_num_t* right,const p256_num_t* modulus) {
    /* bigint_mod_multiply_ct initialise sa sortie. Les formules Jacobiennes
     * réalisent légitimement des carrés et produits en place, d'où les copies. */
    bigint_t o,l,r,m,t;
    p256_num_t left_copy,right_copy,temporary;
    p256_copy(&left_copy,left);
    p256_copy(&right_copy,right);
    p256_bigint(&o,out);
    p256_bigint(&l,&left_copy);
    p256_bigint(&r,&right_copy);
    p256_bigint(&m,(p256_num_t*)modulus);
    p256_bigint(&t,&temporary);
    return bigint_mod_multiply_ct(&o,&l,&r,&m,&t);
}
static int p256_inv(p256_num_t* out,const p256_num_t* input,const p256_num_t* modulus,const p256_num_t* exponent,uint32_t* workspace) { bigint_t o,i,m,e; p256_bigint(&o,out);p256_bigint(&i,(p256_num_t*)input);p256_bigint(&m,(p256_num_t*)modulus);p256_bigint(&e,(p256_num_t*)exponent);return bigint_modexp(&o,&i,&e,&m,workspace,32U); }

static int p256_from_be(p256_num_t* value,const uint8_t bytes[P256_BYTES]) { bigint_t destination; p256_bigint(&destination,value); return bigint_from_be(&destination,bytes,P256_BYTES); }
static int p256_reduce(p256_num_t* out,const p256_num_t* input,const p256_num_t* modulus) {
    /* bigint_mod_reduce() initialise la sortie avant de parcourir l'entrée ;
     * une copie locale préserve donc le cas légitime `out == input`. */
    bigint_t o,i,m;
    p256_num_t copy;
    p256_copy(&copy,input);
    p256_bigint(&o,out);
    p256_bigint(&i,&copy);
    p256_bigint(&m,(p256_num_t*)modulus);
    return bigint_mod_reduce(&o,&i,&m);
}

static void p256_point_inf(p256_point_t* point) { p256_zero(&point->x);p256_zero(&point->y);p256_zero(&point->z); }
static int p256_point_is_inf(const p256_point_t* point) { return p256_zero_p(&point->z); }
static void p256_point_copy(p256_point_t* out,const p256_point_t* in) { p256_copy(&out->x,&in->x);p256_copy(&out->y,&in->y);p256_copy(&out->z,&in->z); }
static void p256_point_from_affine(p256_point_t* point,const p256_num_t* x,const p256_num_t* y) { p256_copy(&point->x,x);p256_copy(&point->y,y);p256_one(&point->z); }

static int p256_point_double(p256_point_t* out,const p256_point_t* in) {
    p256_num_t a,b,c,d,e,f,z2,z4,t0,t1,t2;
    p256_point_t result;
    if(p256_point_is_inf(in)||p256_zero_p(&in->y)){p256_point_inf(out);return 0;}
    if(p256_mul(&a,&in->x,&in->x,&p256_prime)||p256_mul(&b,&in->y,&in->y,&p256_prime)||p256_mul(&c,&b,&b,&p256_prime))return -1;
    if(p256_add(&t0,&in->x,&b,&p256_prime)||p256_mul(&t0,&t0,&t0,&p256_prime)||p256_sub(&t0,&t0,&a,&p256_prime)||p256_sub(&t0,&t0,&c,&p256_prime)||p256_add(&d,&t0,&t0,&p256_prime))return -2;
    if(p256_mul(&z2,&in->z,&in->z,&p256_prime)||p256_mul(&z4,&z2,&z2,&p256_prime)||p256_sub(&e,&a,&z4,&p256_prime)||p256_add(&t0,&e,&e,&p256_prime)||p256_add(&e,&t0,&e,&p256_prime)||p256_mul(&f,&e,&e,&p256_prime))return -3;
    if(p256_add(&t0,&d,&d,&p256_prime)||p256_sub(&result.x,&f,&t0,&p256_prime)||p256_sub(&t1,&d,&result.x,&p256_prime)||p256_mul(&t1,&e,&t1,&p256_prime))return -4;
    if(p256_add(&t0,&c,&c,&p256_prime)||p256_add(&t0,&t0,&t0,&p256_prime)||p256_add(&t0,&t0,&t0,&p256_prime)||p256_sub(&result.y,&t1,&t0,&p256_prime))return -5;
    if(p256_mul(&t2,&in->y,&in->z,&p256_prime)||p256_add(&result.z,&t2,&t2,&p256_prime))return -6;
    p256_point_copy(out,&result);return 0;
}

static int p256_point_add(p256_point_t* out,const p256_point_t* left,const p256_point_t* right) {
    p256_num_t z1z1,z2z2,u1,u2,s1,s2,h,r,h2,h3,u1h2,t0,t1;
    p256_point_t result;
    if(p256_point_is_inf(left)){p256_point_copy(out,right);return 0;}
    if(p256_point_is_inf(right)){p256_point_copy(out,left);return 0;}
    if(p256_mul(&z1z1,&left->z,&left->z,&p256_prime)||p256_mul(&z2z2,&right->z,&right->z,&p256_prime)||p256_mul(&u1,&left->x,&z2z2,&p256_prime)||p256_mul(&u2,&right->x,&z1z1,&p256_prime))return -1;
    if(p256_mul(&t0,&right->z,&z2z2,&p256_prime)||p256_mul(&s1,&left->y,&t0,&p256_prime)||p256_mul(&t0,&left->z,&z1z1,&p256_prime)||p256_mul(&s2,&right->y,&t0,&p256_prime))return -2;
    if(p256_cmp(&u1,&u2)==0){if(p256_cmp(&s1,&s2)!=0){p256_point_inf(out);return 0;}return p256_point_double(out,left);}
    if(p256_sub(&h,&u2,&u1,&p256_prime)||p256_sub(&r,&s2,&s1,&p256_prime)||p256_mul(&h2,&h,&h,&p256_prime)||p256_mul(&h3,&h,&h2,&p256_prime)||p256_mul(&u1h2,&u1,&h2,&p256_prime))return -3;
    if(p256_mul(&t0,&r,&r,&p256_prime)||p256_sub(&t0,&t0,&h3,&p256_prime)||p256_add(&t1,&u1h2,&u1h2,&p256_prime)||p256_sub(&result.x,&t0,&t1,&p256_prime))return -4;
    if(p256_sub(&t0,&u1h2,&result.x,&p256_prime)||p256_mul(&t0,&r,&t0,&p256_prime)||p256_mul(&t1,&s1,&h3,&p256_prime)||p256_sub(&result.y,&t0,&t1,&p256_prime))return -5;
    if(p256_mul(&t0,&left->z,&right->z,&p256_prime)||p256_mul(&result.z,&t0,&h,&p256_prime))return -6;
    p256_point_copy(out,&result);return 0;
}

static int p256_scalar_mul(p256_point_t* out,const p256_num_t* scalar,const p256_point_t* point) {
    int bit; p256_point_t accumulator, doubled;
    p256_point_inf(&accumulator);
    for(bit=255;bit>=0;--bit){if(p256_point_double(&doubled,&accumulator)!=0)return -1;p256_point_copy(&accumulator,&doubled);if(p256_bit(scalar,(uint16_t)bit)){if(p256_point_add(&doubled,&accumulator,point)!=0)return -2;p256_point_copy(&accumulator,&doubled);}}
    p256_point_copy(out,&accumulator);return 0;
}

static int p256_affine_x(p256_num_t* out,const p256_point_t* point,uint32_t* workspace) {
    p256_num_t inverse,z2;
    if(p256_point_is_inf(point))return -1;
    if(p256_inv(&inverse,&point->z,&p256_prime,&p256_p_minus_2,workspace)!=0)return -2;
    if(p256_mul(&z2,&inverse,&inverse,&p256_prime)!=0)return -3;
    return p256_mul(out,&point->x,&z2,&p256_prime);
}

static int p256_point_validate(const p256_num_t* x,const p256_num_t* y) {
    p256_num_t lhs,rhs,t0,t1;
    if(p256_cmp(x,&p256_prime)>=0||p256_cmp(y,&p256_prime)>=0)return -1;
    if(p256_mul(&lhs,y,y,&p256_prime)||p256_mul(&t0,x,x,&p256_prime)||p256_mul(&rhs,&t0,x,&p256_prime))return -2;
    if(p256_add(&t1,x,x,&p256_prime)||p256_add(&t1,&t1,x,&p256_prime)||p256_sub(&rhs,&rhs,&t1,&p256_prime)||p256_add(&rhs,&rhs,&p256_b,&p256_prime))return -3;
    return p256_cmp(&lhs,&rhs)==0?0:-4;
}

static int p256_parse_integer(p256_num_t* out,const uint8_t* input,uint16_t length) {
    const uint8_t* value; uint16_t value_length;
    if(!input||length<3U||input[0]!=0x02U||input[1]==0U||input[1]>=0x80U||(uint16_t)(input[1]+2U)>length)return -1;
    value=input+2U;value_length=input[1];
    if(value[0]&0x80U)return -2;
    if(value_length>1U&&value[0]==0U&&!(value[1]&0x80U))return -3;
    if(value_length==33U&&value[0]==0U){++value;--value_length;}
    if(value_length==0U||value_length>P256_BYTES)return -4;
    p256_zero(out);
    { bigint_t target; p256_bigint(&target,out); if(bigint_from_be(&target,value,value_length)!=0)return -5; }
    return (int)(input[1]+2U);
}

static int p256_parse_signature(p256_num_t* r,p256_num_t* s,const uint8_t* der,uint16_t length) {
    int consumed; uint16_t body;
    if(!der||length<8U||der[0]!=0x30U||der[1]>=0x80U)return -1;
    body=der[1];if((uint16_t)(body+2U)!=length)return -2;
    consumed=p256_parse_integer(r,der+2U,body);if(consumed<0)return -3;
    if((uint16_t)(consumed+2U)>=length)return -4;
    { int second=p256_parse_integer(s,der+2U+(uint16_t)consumed,(uint16_t)(body-(uint16_t)consumed)); if(second<0||(uint16_t)(consumed+second)!=(uint16_t)body)return -5; }
    if(p256_zero_p(r)||p256_zero_p(s)||p256_cmp(r,&p256_order)>=0||p256_cmp(s,&p256_order)>=0)return -6;
    return 0;
}

int ecdsa_p256_sha256_verify(const uint8_t public_key[ECDSA_P256_PUBLIC_KEY_LENGTH],const uint8_t hash[ECDSA_P256_SHA256_LENGTH],const uint8_t* der_signature,uint16_t der_signature_length,uint32_t* workspace,uint16_t workspace_words) {
    p256_num_t qx,qy,r,s,e,w,u1,u2,v; p256_point_t q,g,p1,p2,sum;
    if(!public_key||!hash||!der_signature||!workspace||workspace_words<ECDSA_P256_WORKSPACE_WORDS)return -1;
    if(public_key[0]!=0x04U)return -2;
    if(p256_from_be(&qx,public_key+1U)||p256_from_be(&qy,public_key+33U)||p256_point_validate(&qx,&qy)!=0)return -3;
    if(p256_parse_signature(&r,&s,der_signature,der_signature_length)!=0)return -4;
    if(p256_from_be(&e,hash)!=0)return -5;
    if(p256_inv(&w,&s,&p256_order,&p256_n_minus_2,workspace)!=0)return -6;
    if(p256_mul(&u1,&e,&w,&p256_order)||p256_mul(&u2,&r,&w,&p256_order))return -7;
    p256_point_from_affine(&q,&qx,&qy);p256_point_from_affine(&g,&p256_gx,&p256_gy);
    if(p256_scalar_mul(&p1,&u1,&g)||p256_scalar_mul(&p2,&u2,&q)||p256_point_add(&sum,&p1,&p2)||p256_affine_x(&v,&sum,workspace)!=0)return -8;
    if(p256_reduce(&v,&v,&p256_order)!=0)return -9;
    return p256_cmp(&v,&r)==0?0:-10;
}
