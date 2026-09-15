#include "x509_der.h"
#include "sha256.h"
#include "rsa_verify.h"
#include "ecdsa_p256.h"

static int x509_bytes_equal(const uint8_t* left,uint32_t left_length,const uint8_t* right,uint32_t right_length);
static int x509_dns_name_valid(const uint8_t* name,uint32_t length);

int der_tlv_parse(const uint8_t* input,uint32_t length,der_tlv_view_t* out){
    uint32_t value_length=0U,offset=2U,i;uint8_t count;
    if(!input||!out||length<2U)return -1;
    if((input[1]&128U)==0U)value_length=input[1];
    else{count=(uint8_t)(input[1]&127U);if(count==0U||count>4U||length<(uint32_t)2U+count)return -2;for(i=0U;i<count;i++){if(value_length>0x00ffffffU)return -3;value_length=(value_length<<8U)|input[2U+i];}offset+=(uint32_t)count;if(value_length<128U)return -4;}
    if(value_length>length-offset)return -5;
    out->tag=input[0];out->value=input+offset;out->length=value_length;out->total_length=offset+value_length;return 0;
}

static int next_tlv(const uint8_t** cursor,uint32_t* remaining,der_tlv_view_t* out){if(!cursor||!*cursor||!remaining||der_tlv_parse(*cursor,*remaining,out)!=0)return -1;*cursor+=out->total_length;*remaining-=out->total_length;return 0;}
static int require_sequence(const uint8_t** cursor,uint32_t* remaining,der_tlv_view_t* out){if(next_tlv(cursor,remaining,out)!=0||out->tag!=0x30U)return -1;return 0;}
static int oid_equal(const der_tlv_view_t* oid,const uint8_t* value,uint32_t length){uint32_t index;if(!oid||oid->tag!=0x06U||oid->length!=length)return 0;for(index=0U;index<length;index++)if(oid->value[index]!=value[index])return 0;return 1;}

/* AlgorithmIdentifier ::= id-ecPublicKey, namedCurve secp256r1.
 * Les deux OID sont encodés en DER dans le contenu de la SEQUENCE SPKI. */
static int x509_spki_is_ec_p256_algorithm(const uint8_t* algorithm,uint32_t algorithm_length){
    static const uint8_t ec_p256_algorithm[]={
        0x06U,0x07U,0x2aU,0x86U,0x48U,0xceU,0x3dU,0x02U,0x01U,
        0x06U,0x08U,0x2aU,0x86U,0x48U,0xceU,0x3dU,0x03U,0x01U,0x07U
    };
    return x509_bytes_equal(algorithm,algorithm_length,ec_p256_algorithm,sizeof(ec_p256_algorithm));
}

static int x509_extract_common_name(const uint8_t* subject,uint32_t subject_length,x509_certificate_view_t* out){
    static const uint8_t common_name_oid[]={0x55U,0x04U,0x03U};
    const uint8_t* cursor=subject;uint32_t remaining=subject_length;
    der_tlv_view_t set,attribute,oid,value;
    while(remaining){
        const uint8_t* set_cursor;uint32_t set_remaining;
        if(next_tlv(&cursor,&remaining,&set)!=0||set.tag!=0x31U)return -1;
        set_cursor=set.value;set_remaining=set.length;
        while(set_remaining){
            const uint8_t* attribute_cursor;uint32_t attribute_remaining;
            if(next_tlv(&set_cursor,&set_remaining,&attribute)!=0||attribute.tag!=0x30U)return -2;
            attribute_cursor=attribute.value;attribute_remaining=attribute.length;
            if(next_tlv(&attribute_cursor,&attribute_remaining,&oid)!=0||next_tlv(&attribute_cursor,&attribute_remaining,&value)!=0||attribute_remaining!=0U)return -3;
            if(oid_equal(&oid,common_name_oid,sizeof(common_name_oid))){
                if(out->common_name)return -4;
                if(value.tag!=0x0cU&&value.tag!=0x13U&&value.tag!=0x16U)return -5;
                out->common_name=value.value;out->common_name_length=value.length;
            }
        }
    }
    return 0;
}

static int x509_name_constraints_store_dns(const der_tlv_view_t* base,const uint8_t** names,uint32_t* lengths,uint8_t* count){
    const uint8_t* name;uint32_t name_length;
    if(!base||!names||!lengths||!count||base->tag!=0x82U||base->length==0U||*count>=X509_NAME_CONSTRAINTS_MAX_DNS)return -1;
    name=base->value;name_length=base->length;
    if(name_length>1U&&name[0]=='.'){name++;name_length--;}
    if(!x509_dns_name_valid(name,name_length))return -2;
    names[*count]=base->value;lengths[*count]=base->length;(*count)++;
    return 0;
}

static int x509_parse_name_constraint_subtrees(const uint8_t* input,uint32_t length,const uint8_t** names,uint32_t* lengths,uint8_t* count){
    der_tlv_view_t subtrees,subtree,base,distance;const uint8_t* cursor;uint32_t remaining;
    if(der_tlv_parse(input,length,&subtrees)!=0||subtrees.tag!=0x30U||subtrees.total_length!=length||subtrees.length==0U)return -1;
    cursor=subtrees.value;remaining=subtrees.length;
    while(remaining){
        const uint8_t* subtree_cursor;uint32_t subtree_remaining;
        if(next_tlv(&cursor,&remaining,&subtree)!=0||subtree.tag!=0x30U)return -2;
        subtree_cursor=subtree.value;subtree_remaining=subtree.length;
        if(next_tlv(&subtree_cursor,&subtree_remaining,&base)!=0||x509_name_constraints_store_dns(&base,names,lengths,count)!=0)return -3;
        if(subtree_remaining){if(next_tlv(&subtree_cursor,&subtree_remaining,&distance)!=0||distance.tag!=0x80U||distance.length!=1U||distance.value[0]!=0U||subtree_remaining!=0U)return -4;}
    }
    return 0;
}

static int x509_parse_name_constraints(const uint8_t* input,uint32_t length,x509_certificate_view_t* out){
    der_tlv_view_t constraints,field;const uint8_t* cursor;uint32_t remaining;uint8_t permitted_seen=0U,excluded_seen=0U;
    if(!out||out->name_constraints_present||der_tlv_parse(input,length,&constraints)!=0||constraints.tag!=0x30U||constraints.total_length!=length||constraints.length==0U)return -1;
    cursor=constraints.value;remaining=constraints.length;out->name_constraints_present=1U;
    while(remaining){
        if(next_tlv(&cursor,&remaining,&field)!=0)return -2;
        if(field.tag==0xa0U){if(permitted_seen||x509_parse_name_constraint_subtrees(field.value,field.length,out->name_constraints_dns_permitted,out->name_constraints_dns_permitted_length,&out->name_constraints_dns_permitted_count)!=0)return -3;permitted_seen=1U;}
        else if(field.tag==0xa1U){if(excluded_seen||x509_parse_name_constraint_subtrees(field.value,field.length,out->name_constraints_dns_excluded,out->name_constraints_dns_excluded_length,&out->name_constraints_dns_excluded_count)!=0)return -4;excluded_seen=1U;}
        else return -5;
    }
    return (permitted_seen||excluded_seen)?0:-6;
}

static int x509_parse_extensions(const uint8_t* input,uint32_t length,x509_certificate_view_t* out){
    static const uint8_t subject_alt_name_oid[]={0x55U,0x1dU,0x11U};
    static const uint8_t basic_constraints_oid[]={0x55U,0x1dU,0x13U};
    static const uint8_t key_usage_oid[]={0x55U,0x1dU,0x0fU};
    static const uint8_t subject_key_identifier_oid[]={0x55U,0x1dU,0x0eU};
    static const uint8_t authority_key_identifier_oid[]={0x55U,0x1dU,0x23U};
    static const uint8_t name_constraints_oid[]={0x55U,0x1dU,0x1eU};
    static const uint8_t extended_key_usage_oid[]={0x55U,0x1dU,0x25U};
    static const uint8_t server_auth_oid[]={0x2bU,0x06U,0x01U,0x05U,0x05U,0x07U,0x03U,0x01U};
    der_tlv_view_t extensions,extension,oid,field,names;
    const uint8_t* cursor;uint32_t remaining;
    if(der_tlv_parse(input,length,&extensions)!=0||extensions.tag!=0x30U||extensions.total_length!=length)return -1;
    cursor=extensions.value;remaining=extensions.length;
    while(remaining){
        const uint8_t* extension_cursor;uint32_t extension_remaining;uint8_t critical=0U;
        if(next_tlv(&cursor,&remaining,&extension)!=0||extension.tag!=0x30U)return -2;
        extension_cursor=extension.value;extension_remaining=extension.length;
        if(next_tlv(&extension_cursor,&extension_remaining,&oid)!=0)return -3;
        if(extension_remaining&&extension_cursor[0]==0x01U){if(next_tlv(&extension_cursor,&extension_remaining,&field)!=0||field.length!=1U||(field.value[0]!=0U&&field.value[0]!=0xffU))return -4;critical=field.value[0]==0xffU?1U:0U;}
        if(next_tlv(&extension_cursor,&extension_remaining,&field)!=0||field.tag!=0x04U||extension_remaining!=0U)return -5;
        if(oid_equal(&oid,subject_alt_name_oid,sizeof(subject_alt_name_oid))){
            if(out->subject_alt_names)return -6;
            if(der_tlv_parse(field.value,field.length,&names)!=0||names.tag!=0x30U||names.total_length!=field.length)return -7;
            out->subject_alt_names=names.value;out->subject_alt_names_length=names.length;
        }else if(oid_equal(&oid,basic_constraints_oid,sizeof(basic_constraints_oid))){
            const uint8_t* basic_cursor;uint32_t basic_remaining;
            if(out->basic_constraints_present||der_tlv_parse(field.value,field.length,&names)!=0||names.tag!=0x30U||names.total_length!=field.length)return -8;
            basic_cursor=names.value;basic_remaining=names.length;out->basic_constraints_present=1U;out->basic_constraints_ca=0U;
            if(basic_remaining&&basic_cursor[0]==0x01U){if(next_tlv(&basic_cursor,&basic_remaining,&field)!=0||field.length!=1U||(field.value[0]!=0U&&field.value[0]!=0xffU))return -9;out->basic_constraints_ca=field.value[0]==0xffU?1U:0U;}
            if(basic_remaining){uint32_t path=0U,index=0U;if(next_tlv(&basic_cursor,&basic_remaining,&field)!=0||field.tag!=0x02U||field.length==0U||field.length>5U||basic_remaining!=0U)return -10;if(field.length>1U&&field.value[0]==0U){if((field.value[1]&0x80U)==0U)return -10;index=1U;}if((field.value[index]&0x80U)!=0U)return -10;for(;index<field.length;index++){if(path>0x00ffffffU)return -10;path=(path<<8U)|field.value[index];}out->path_len_present=1U;out->path_len_constraint=path;}if(out->path_len_present&&!out->basic_constraints_ca)return -10;
        }else if(oid_equal(&oid,key_usage_oid,sizeof(key_usage_oid))){
            if(out->key_usage_present||der_tlv_parse(field.value,field.length,&names)!=0||names.tag!=0x03U||names.total_length!=field.length||names.length<2U||names.value[0]>7U)return -11;
            out->key_usage_present=1U;out->key_usage_key_cert_sign=(names.value[1]&0x04U)!=0U?1U:0U;
        }else if(oid_equal(&oid,subject_key_identifier_oid,sizeof(subject_key_identifier_oid))){
            if(out->subject_key_identifier||der_tlv_parse(field.value,field.length,&names)!=0||names.tag!=0x04U||names.total_length!=field.length||names.length==0U)return -12;out->subject_key_identifier=names.value;out->subject_key_identifier_length=names.length;
        }else if(oid_equal(&oid,authority_key_identifier_oid,sizeof(authority_key_identifier_oid))){
            const uint8_t* aki_cursor;uint32_t aki_remaining;if(out->authority_key_identifier||der_tlv_parse(field.value,field.length,&names)!=0||names.tag!=0x30U||names.total_length!=field.length)return -13;aki_cursor=names.value;aki_remaining=names.length;while(aki_remaining){if(next_tlv(&aki_cursor,&aki_remaining,&field)!=0)return -13;if(field.tag==0x80U){if(out->authority_key_identifier||field.length==0U)return -13;out->authority_key_identifier=field.value;out->authority_key_identifier_length=field.length;}}
        }else if(oid_equal(&oid,name_constraints_oid,sizeof(name_constraints_oid))){
            if(x509_parse_name_constraints(field.value,field.length,out)!=0)return -14;
        }else if(oid_equal(&oid,extended_key_usage_oid,sizeof(extended_key_usage_oid))){
            const uint8_t* eku_cursor;uint32_t eku_remaining;if(out->extended_key_usage_present||der_tlv_parse(field.value,field.length,&names)!=0||names.tag!=0x30U||names.total_length!=field.length||names.length==0U)return -14;out->extended_key_usage_present=1U;eku_cursor=names.value;eku_remaining=names.length;while(eku_remaining){if(next_tlv(&eku_cursor,&eku_remaining,&field)!=0||field.tag!=0x06U)return -14;if(field.length==sizeof(server_auth_oid)&&x509_bytes_equal(field.value,field.length,server_auth_oid,sizeof(server_auth_oid)))out->extended_key_usage_server_auth=1U;}
        }else if(critical)return -15;
    }
    return 0;
}

int x509_certificate_parse(const uint8_t* certificate,uint32_t length,x509_certificate_view_t* out){
    der_tlv_view_t outer,tbs,field,validity,spki,algorithm,bit_string,rsa,modulus,exponent;
    const uint8_t *p,*sp,*q,*tbs_der;uint32_t remaining,sp_remaining,left,i;
    if(!certificate||!out)return -1;
    if(der_tlv_parse(certificate,length,&outer)!=0||outer.tag!=0x30U||outer.total_length!=length)return -2;
    out->common_name=0;out->common_name_length=0U;out->subject_alt_names=0;out->subject_alt_names_length=0U;out->basic_constraints_present=0U;out->basic_constraints_ca=0U;out->path_len_present=0U;out->path_len_constraint=0U;out->subject_key_identifier=0;out->subject_key_identifier_length=0U;out->authority_key_identifier=0;out->authority_key_identifier_length=0U;out->key_usage_present=0U;out->extended_key_usage_present=0U;out->extended_key_usage_server_auth=0U;out->name_constraints_present=0U;out->name_constraints_dns_permitted_count=0U;out->name_constraints_dns_excluded_count=0U;for(i=0U;i<X509_NAME_CONSTRAINTS_MAX_DNS;i++){out->name_constraints_dns_permitted[i]=0;out->name_constraints_dns_permitted_length[i]=0U;out->name_constraints_dns_excluded[i]=0;out->name_constraints_dns_excluded_length[i]=0U;}out->key_usage_key_cert_sign=0U;
    out->rsa_modulus=0;out->rsa_modulus_length=0U;out->rsa_exponent=0;out->rsa_exponent_length=0U;
    out->ec_p256_public_key=0;out->ec_p256_public_key_length=0U;
    out->signature_algorithm=0;out->signature_algorithm_length=0U;out->signature=0;out->signature_length=0U;
    out->certificate=certificate;out->certificate_length=length;p=outer.value;remaining=outer.length;tbs_der=p;
    if(require_sequence(&p,&remaining,&tbs)!=0)return -3;
    out->tbs_certificate=tbs.value;out->tbs_certificate_length=tbs.length;out->tbs_certificate_der=tbs_der;out->tbs_certificate_der_length=tbs.total_length;
    q=tbs.value;left=tbs.length;
    if(left&&q[0]==0xa0U){if(next_tlv(&q,&left,&field)!=0)return -4;}
    if(next_tlv(&q,&left,&field)!=0||field.tag!=0x02U)return -5;
    out->serial=field.value;out->serial_length=field.length;
    if(require_sequence(&q,&left,&field)!=0)return -6;
    if(require_sequence(&q,&left,&field)!=0)return -7;
    out->issuer=field.value;out->issuer_length=field.length;
    if(require_sequence(&q,&left,&validity)!=0)return -8;
    {const uint8_t* validity_cursor=validity.value;uint32_t validity_remaining=validity.length;
        if(next_tlv(&validity_cursor,&validity_remaining,&field)!=0||(field.tag!=0x17U&&field.tag!=0x18U))return -9;
        out->not_before=field.value;out->not_before_length=field.length;
        if(next_tlv(&validity_cursor,&validity_remaining,&field)!=0||(field.tag!=0x17U&&field.tag!=0x18U)||validity_remaining!=0U)return -10;
        out->not_after=field.value;out->not_after_length=field.length;
    }
    if(require_sequence(&q,&left,&field)!=0)return -11;
    out->subject=field.value;out->subject_length=field.length;
    if(x509_extract_common_name(out->subject,out->subject_length,out)!=0)return -12;
    if(require_sequence(&q,&left,&spki)!=0)return -13;
    out->subject_public_key_info=spki.value;out->subject_public_key_info_length=spki.length;
    sp=spki.value;sp_remaining=spki.length;
    if(require_sequence(&sp,&sp_remaining,&algorithm)!=0)return -14;
    out->subject_public_key_algorithm=algorithm.value;out->subject_public_key_algorithm_length=algorithm.length;
    if(next_tlv(&sp,&sp_remaining,&bit_string)!=0||bit_string.tag!=0x03U||bit_string.length<1U||bit_string.value[0]!=0U||sp_remaining!=0U)return -15;
    if(x509_spki_is_ec_p256_algorithm(out->subject_public_key_algorithm,out->subject_public_key_algorithm_length)){
        if(bit_string.length-1U!=ECDSA_P256_PUBLIC_KEY_LENGTH||bit_string.value[1U]!=0x04U)return -16;
        out->ec_p256_public_key=bit_string.value+1U;
        out->ec_p256_public_key_length=ECDSA_P256_PUBLIC_KEY_LENGTH;
    }else{
        if(der_tlv_parse(bit_string.value+1U,bit_string.length-1U,&rsa)!=0||rsa.tag!=0x30U||rsa.total_length!=bit_string.length-1U)return -16;
        sp=rsa.value;sp_remaining=rsa.length;
        if(next_tlv(&sp,&sp_remaining,&modulus)!=0||modulus.tag!=0x02U||modulus.length==0U)return -17;
        if(next_tlv(&sp,&sp_remaining,&exponent)!=0||exponent.tag!=0x02U||exponent.length==0U||sp_remaining!=0U)return -18;
        out->rsa_modulus=modulus.value;out->rsa_modulus_length=modulus.length;out->rsa_exponent=exponent.value;out->rsa_exponent_length=exponent.length;
    }
    while(left){
        if(next_tlv(&q,&left,&field)!=0)return -19;
        if(field.tag==0xa3U){if(x509_parse_extensions(field.value,field.length,out)!=0)return -20;}
        else if(field.tag!=0xa1U&&field.tag!=0xa2U)return -21;
    }
    if(require_sequence(&p,&remaining,&field)!=0)return -22;
    out->signature_algorithm=field.value;out->signature_algorithm_length=field.length;
    if(next_tlv(&p,&remaining,&field)!=0||field.tag!=0x03U||field.length<1U||field.value[0]!=0U||remaining!=0U)return -23;
    out->signature=field.value+1U;out->signature_length=field.length-1U;
    if(out->signature_length==0U)return -24;
    return 0;
}

static uint8_t x509_ascii_lower(uint8_t value){return (value>='A'&&value<='Z')?(uint8_t)(value+('a'-'A')):value;}

static int x509_dns_name_valid(const uint8_t* name,uint32_t length){
    uint32_t index,label_start=0U;
    if(!name||length==0U||length>253U)return 0;
    for(index=0U;index<length;index++){
        uint8_t value=name[index];
        if(value=='.'){
            if(index==label_start||index-label_start>63U||name[label_start]=='-'||name[index-1U]=='-')return 0;
            label_start=index+1U;
        }else if(!((value>='a'&&value<='z')||(value>='A'&&value<='Z')||(value>='0'&&value<='9')||value=='-'))return 0;
    }
    if(label_start==length||length-label_start>63U||name[label_start]=='-'||name[length-1U]=='-')return 0;
    return 1;
}

static int x509_hostname_length(const char* hostname,uint32_t* length){
    uint32_t index=0U;
    if(!hostname||!length)return -1;
    while(index<=253U&&hostname[index])index++;
    if(index==0U||index>253U)return -2;
    *length=index;return x509_dns_name_valid((const uint8_t*)hostname,index)?0:-3;
}

static int x509_dns_name_match(const uint8_t* pattern,uint32_t pattern_length,const char* hostname,uint32_t hostname_length){
    uint32_t index,suffix_length,separator;
    if(!pattern||!x509_dns_name_valid((const uint8_t*)hostname,hostname_length))return 0;
    if(pattern_length>=3U&&pattern[0]=='*'&&pattern[1]=='.'){
        suffix_length=pattern_length-2U;
        if(!x509_dns_name_valid(pattern+2U,suffix_length)||hostname_length<=suffix_length+1U)return 0;
        separator=hostname_length-suffix_length-1U;
        if(hostname[separator]!='.')return 0;
        for(index=0U;index<separator;index++)if(hostname[index]=='.')return 0;
        for(index=0U;index<suffix_length;index++)if(x509_ascii_lower(pattern[2U+index])!=x509_ascii_lower((uint8_t)hostname[separator+1U+index]))return 0;
        return 1;
    }
    if(!x509_dns_name_valid(pattern,pattern_length)||pattern_length!=hostname_length)return 0;
    for(index=0U;index<pattern_length;index++)if(x509_ascii_lower(pattern[index])!=x509_ascii_lower((uint8_t)hostname[index]))return 0;
    return 1;
}

int x509_certificate_hostname_validate(const x509_certificate_view_t* certificate,const char* hostname){
    const uint8_t* cursor;uint32_t remaining,hostname_length;der_tlv_view_t name;uint8_t has_dns_name=0U;
    if(!certificate||x509_hostname_length(hostname,&hostname_length)!=0)return -1;
    if(certificate->subject_alt_names){
        cursor=certificate->subject_alt_names;remaining=certificate->subject_alt_names_length;
        while(remaining){
            if(next_tlv(&cursor,&remaining,&name)!=0)return -2;
            if(name.tag==0x82U){has_dns_name=1U;if(x509_dns_name_match(name.value,name.length,hostname,hostname_length))return 0;}
        }
        if(has_dns_name)return -3;
    }
    if(certificate->common_name&&x509_dns_name_match(certificate->common_name,certificate->common_name_length,hostname,hostname_length))return 0;
    return -4;
}

static int x509_dns_subtree_match(const uint8_t* constraint,uint32_t constraint_length,const char* hostname,uint32_t hostname_length){
    uint32_t offset=0U,suffix_length,index;uint8_t require_descendant=0U;
    if(!constraint||constraint_length==0U||!x509_dns_name_valid((const uint8_t*)hostname,hostname_length))return 0;
    if(constraint[0]=='.'){if(constraint_length==1U)return 0;offset=1U;require_descendant=1U;}
    suffix_length=constraint_length-offset;
    if(!x509_dns_name_valid(constraint+offset,suffix_length)||hostname_length<suffix_length)return 0;
    if(hostname_length==suffix_length){if(require_descendant)return 0;for(index=0U;index<suffix_length;index++)if(x509_ascii_lower(constraint[offset+index])!=x509_ascii_lower((uint8_t)hostname[index]))return 0;return 1;}
    if(hostname[hostname_length-suffix_length-1U]!='.')return 0;
    for(index=0U;index<suffix_length;index++)if(x509_ascii_lower(constraint[offset+index])!=x509_ascii_lower((uint8_t)hostname[hostname_length-suffix_length+index]))return 0;
    return 1;
}

int x509_certificate_name_constraints_dns_validate(const x509_certificate_view_t* certificate,const char* hostname){
    uint32_t hostname_length,index;uint8_t permitted=0U;
    if(!certificate||x509_hostname_length(hostname,&hostname_length)!=0)return -1;
    if(!certificate->name_constraints_present)return 0;
    for(index=0U;index<certificate->name_constraints_dns_excluded_count;index++)if(x509_dns_subtree_match(certificate->name_constraints_dns_excluded[index],certificate->name_constraints_dns_excluded_length[index],hostname,hostname_length))return -2;
    if(certificate->name_constraints_dns_permitted_count==0U)return 0;
    for(index=0U;index<certificate->name_constraints_dns_permitted_count;index++)if(x509_dns_subtree_match(certificate->name_constraints_dns_permitted[index],certificate->name_constraints_dns_permitted_length[index],hostname,hostname_length)){permitted=1U;break;}
    return permitted?0:-3;
}

static int x509_bytes_equal(const uint8_t* left,uint32_t left_length,const uint8_t* right,uint32_t right_length){uint32_t index;if(!left||!right||left_length!=right_length)return 0;for(index=0U;index<left_length;index++)if(left[index]!=right[index])return 0;return 1;}

static int x509_signature_algorithm_is_sha256_rsa(const x509_certificate_view_t* certificate){
    static const uint8_t sha256_rsa_algorithm[]={0x06U,0x09U,0x2aU,0x86U,0x48U,0x86U,0xf7U,0x0dU,0x01U,0x01U,0x0bU,0x05U,0x00U};
    return certificate&&x509_bytes_equal(certificate->signature_algorithm,certificate->signature_algorithm_length,sha256_rsa_algorithm,sizeof(sha256_rsa_algorithm));
}

static int x509_signature_algorithm_is_ecdsa_sha256(const x509_certificate_view_t* certificate){
    static const uint8_t ecdsa_sha256_algorithm[]={0x06U,0x08U,0x2aU,0x86U,0x48U,0xceU,0x3dU,0x04U,0x03U,0x02U};
    return certificate&&x509_bytes_equal(certificate->signature_algorithm,certificate->signature_algorithm_length,ecdsa_sha256_algorithm,sizeof(ecdsa_sha256_algorithm));
}

int x509_certificate_chain_validate_one(const x509_certificate_view_t* leaf,const x509_certificate_view_t* trust_anchor,uint32_t* workspace,uint16_t workspace_length){
    sha256_ctx_t hash;uint8_t digest[32];int status;
    if(!leaf||!trust_anchor||!workspace||!leaf->tbs_certificate_der||!leaf->signature||!leaf->issuer||!trust_anchor->subject)return -1;
    if(!x509_bytes_equal(leaf->issuer,leaf->issuer_length,trust_anchor->subject,trust_anchor->subject_length))return -2;
    if(leaf->authority_key_identifier&&(!trust_anchor->subject_key_identifier||!x509_bytes_equal(leaf->authority_key_identifier,leaf->authority_key_identifier_length,trust_anchor->subject_key_identifier,trust_anchor->subject_key_identifier_length)))return -3;
    sha256_init(&hash);sha256_update(&hash,leaf->tbs_certificate_der,leaf->tbs_certificate_der_length);sha256_final(&hash,digest);
    if(x509_signature_algorithm_is_sha256_rsa(leaf)){
        if(x509_rsa_public_key_validate(trust_anchor)!=0)return -4;
        if(leaf->signature_length>65535U||trust_anchor->rsa_modulus_length>65535U||trust_anchor->rsa_exponent_length>65535U)return -5;
        status=rsa_pkcs1_v15_sha256_verify(trust_anchor->rsa_modulus,(uint16_t)trust_anchor->rsa_modulus_length,trust_anchor->rsa_exponent,(uint16_t)trust_anchor->rsa_exponent_length,digest,leaf->signature,(uint16_t)leaf->signature_length,workspace,workspace_length);
    }else if(x509_signature_algorithm_is_ecdsa_sha256(leaf)){
        if(x509_ec_p256_public_key_validate(trust_anchor)!=0||leaf->signature_length>ECDSA_P256_DER_SIGNATURE_MAX)return -5;
        status=ecdsa_p256_sha256_verify(trust_anchor->ec_p256_public_key,digest,leaf->signature,(uint16_t)leaf->signature_length,workspace,workspace_length);
    }else return -5;
    return status==0?0:-6;
}

static int x509_certificate_ca_authorized(const x509_certificate_view_t* certificate){return certificate&&certificate->basic_constraints_present&&certificate->basic_constraints_ca&&certificate->key_usage_present&&certificate->key_usage_key_cert_sign?0:-1;}
int x509_certificate_chain_validate_two(const x509_certificate_view_t* leaf,const x509_certificate_view_t* intermediate,const x509_certificate_view_t* trust_anchor,uint32_t* workspace,uint16_t workspace_length){if(!leaf||!intermediate||!trust_anchor)return -1;if(x509_certificate_ca_authorized(intermediate)!=0)return -2;if(trust_anchor->path_len_present&&trust_anchor->path_len_constraint<1U)return -3;if(x509_certificate_chain_validate_one(leaf,intermediate,workspace,workspace_length)!=0)return -4;if(x509_certificate_chain_validate_one(intermediate,trust_anchor,workspace,workspace_length)!=0)return -5;return 0;}

int x509_certificate_chain_validate_three(const x509_certificate_view_t* leaf,const x509_certificate_view_t* intermediate_one,const x509_certificate_view_t* intermediate_two,const x509_certificate_view_t* trust_anchor,uint32_t* workspace,uint16_t workspace_length){
    if(!leaf||!intermediate_one||!intermediate_two||!trust_anchor)return -1;
    if(x509_certificate_ca_authorized(intermediate_one)!=0||x509_certificate_ca_authorized(intermediate_two)!=0)return -2;
    if(intermediate_two->path_len_present&&intermediate_two->path_len_constraint<1U)return -3;
    if(trust_anchor->path_len_present&&trust_anchor->path_len_constraint<2U)return -4;
    if(x509_certificate_chain_validate_one(leaf,intermediate_one,workspace,workspace_length)!=0)return -5;
    if(x509_certificate_chain_validate_one(intermediate_one,intermediate_two,workspace,workspace_length)!=0)return -6;
    if(x509_certificate_chain_validate_one(intermediate_two,trust_anchor,workspace,workspace_length)!=0)return -7;
    return 0;
}

int x509_rsa_public_key_validate(const x509_certificate_view_t* certificate){
    static const uint8_t rsa_encryption_oid[]={0x06U,0x09U,0x2aU,0x86U,0x48U,0x86U,0xf7U,0x0dU,0x01U,0x01U,0x01U};
    const uint8_t *modulus,*exponent;uint32_t modulus_length,exponent_length,value=0U;uint32_t i;
    if(!certificate||!certificate->subject_public_key_algorithm||certificate->subject_public_key_algorithm_length!=(uint32_t)(sizeof(rsa_encryption_oid)+2U)||!certificate->rsa_modulus||!certificate->rsa_exponent)return -1;
    for(i=0U;i<sizeof(rsa_encryption_oid);i++)if(certificate->subject_public_key_algorithm[i]!=rsa_encryption_oid[i])return -2;
    if(certificate->subject_public_key_algorithm[sizeof(rsa_encryption_oid)]!=0x05U||certificate->subject_public_key_algorithm[sizeof(rsa_encryption_oid)+1U]!=0x00U)return -2;
    modulus=certificate->rsa_modulus;modulus_length=certificate->rsa_modulus_length;
    if(modulus_length==0U)return -3;
    if(modulus[0]==0U){if(modulus_length==1U||(modulus[1]&0x80U)==0U)return -3;modulus++;modulus_length--;}
    else if((modulus[0]&0x80U)!=0U)return -3;
    exponent=certificate->rsa_exponent;exponent_length=certificate->rsa_exponent_length;
    while(exponent_length>1U&&*exponent==0U){exponent++;exponent_length--;}
    if(exponent_length==0U||exponent_length>4U||(*exponent&0x80U)!=0U)return -4;
    for(i=0U;i<exponent_length;i++)value=(value<<8U)|exponent[i];
    if(value<3U||(value&1U)==0U)return -5;
    return 0;
}

int x509_ec_p256_public_key_validate(const x509_certificate_view_t* certificate){
    if(!certificate||!certificate->subject_public_key_algorithm||!certificate->ec_p256_public_key)return -1;
    if(!x509_spki_is_ec_p256_algorithm(certificate->subject_public_key_algorithm,certificate->subject_public_key_algorithm_length))return -2;
    if(certificate->ec_p256_public_key_length!=ECDSA_P256_PUBLIC_KEY_LENGTH||certificate->ec_p256_public_key[0]!=0x04U)return -3;
    return 0;
}

typedef struct {uint16_t year;uint8_t month,day,hour,minute,second;} x509_time_t;
static int x509_time_digit(uint8_t value,uint8_t* out){if(value<'0'||value>'9')return -1;*out=(uint8_t)(value-'0');return 0;}
static int x509_time_parse(const uint8_t* input,uint32_t length,x509_time_t* out){uint8_t d[14],i;uint8_t days;uint16_t year;
    if(!input||!out||(length!=13U&&length!=15U)||input[length-1U]!='Z')return -1;
    for(i=0U;i<(uint8_t)(length-1U);i++)if(x509_time_digit(input[i],&d[i])!=0)return -2;
    if(length==13U)year=(uint16_t)((d[0]*10U+d[1]>=50U?1900U:2000U)+d[0]*10U+d[1]);else year=(uint16_t)(d[0]*1000U+d[1]*100U+d[2]*10U+d[3]);
    out->year=year;i=(uint8_t)(length==13U?2U:4U);out->month=(uint8_t)(d[i]*10U+d[i+1U]);out->day=(uint8_t)(d[i+2U]*10U+d[i+3U]);out->hour=(uint8_t)(d[i+4U]*10U+d[i+5U]);out->minute=(uint8_t)(d[i+6U]*10U+d[i+7U]);out->second=(uint8_t)(d[i+8U]*10U+d[i+9U]);
    if(out->month<1U||out->month>12U||out->hour>23U||out->minute>59U||out->second>59U)return -3;
    days=(uint8_t)((out->month==4U||out->month==6U||out->month==9U||out->month==11U)?30U:31U);if(out->month==2U)days=(uint8_t)(((out->year%4U==0U&&(out->year%100U!=0U||out->year%400U==0U))?29U:28U));if(out->day<1U||out->day>days)return -4;return 0;
}
static int x509_time_compare(const x509_time_t* left,const x509_time_t* right){if(left->year!=right->year)return left->year<right->year?-1:1;if(left->month!=right->month)return left->month<right->month?-1:1;if(left->day!=right->day)return left->day<right->day?-1:1;if(left->hour!=right->hour)return left->hour<right->hour?-1:1;if(left->minute!=right->minute)return left->minute<right->minute?-1:1;if(left->second!=right->second)return left->second<right->second?-1:1;return 0;}
int x509_certificate_valid_at(const x509_certificate_view_t* certificate,const char* utc_time){x509_time_t before,after,current;if(!certificate||!utc_time||x509_time_parse(certificate->not_before,certificate->not_before_length,&before)!=0||x509_time_parse(certificate->not_after,certificate->not_after_length,&after)!=0||x509_time_parse((const uint8_t*)utc_time,15U,&current)!=0)return -1;if(x509_time_compare(&before,&after)>0)return -2;return x509_time_compare(&current,&before)<0||x509_time_compare(&current,&after)>0?-3:0;}

int x509_certificate_tls_identity_validate(const x509_certificate_view_t* leaf,const x509_certificate_view_t* trust_anchor,const char* hostname,const char* utc_time,uint32_t* workspace,uint16_t workspace_length){
    if(!leaf||(leaf->extended_key_usage_present&&!leaf->extended_key_usage_server_auth))return -1;
    if(x509_certificate_chain_validate_one(leaf,trust_anchor,workspace,workspace_length)!=0)return -2;
    if(x509_certificate_hostname_validate(leaf,hostname)!=0)return -3;
    if(x509_certificate_valid_at(leaf,utc_time)!=0)return -4;
    return 0;
}
int x509_certificate_tls_identity_validate_two(const x509_certificate_view_t* leaf,const x509_certificate_view_t* intermediate,const x509_certificate_view_t* trust_anchor,const char* hostname,const char* utc_time,uint32_t* workspace,uint16_t workspace_length){if(!leaf||(leaf->extended_key_usage_present&&!leaf->extended_key_usage_server_auth))return -1;if(x509_certificate_chain_validate_two(leaf,intermediate,trust_anchor,workspace,workspace_length)!=0)return -2;if(x509_certificate_name_constraints_dns_validate(intermediate,hostname)!=0)return -3;if(x509_certificate_hostname_validate(leaf,hostname)!=0)return -4;if(x509_certificate_valid_at(leaf,utc_time)!=0||x509_certificate_valid_at(intermediate,utc_time)!=0||x509_certificate_valid_at(trust_anchor,utc_time)!=0)return -5;return 0;}

int x509_certificate_tls_identity_validate_three(const x509_certificate_view_t* leaf,const x509_certificate_view_t* intermediate_one,const x509_certificate_view_t* intermediate_two,const x509_certificate_view_t* trust_anchor,const char* hostname,const char* utc_time,uint32_t* workspace,uint16_t workspace_length){
    if(!leaf||(leaf->extended_key_usage_present&&!leaf->extended_key_usage_server_auth))return -1;
    if(x509_certificate_chain_validate_three(leaf,intermediate_one,intermediate_two,trust_anchor,workspace,workspace_length)!=0)return -2;
    if(x509_certificate_name_constraints_dns_validate(intermediate_one,hostname)!=0||x509_certificate_name_constraints_dns_validate(intermediate_two,hostname)!=0||x509_certificate_name_constraints_dns_validate(trust_anchor,hostname)!=0)return -3;
    if(x509_certificate_hostname_validate(leaf,hostname)!=0)return -4;
    if(x509_certificate_valid_at(leaf,utc_time)!=0||x509_certificate_valid_at(intermediate_one,utc_time)!=0||x509_certificate_valid_at(intermediate_two,utc_time)!=0||x509_certificate_valid_at(trust_anchor,utc_time)!=0)return -5;
    return 0;
}
