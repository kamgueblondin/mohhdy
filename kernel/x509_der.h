#ifndef MOHHDY_X509_DER_H
#define MOHHDY_X509_DER_H

#include <stdint.h>

typedef struct { uint8_t tag; const uint8_t* value; uint32_t length; uint32_t total_length; } der_tlv_view_t;
#define X509_NAME_CONSTRAINTS_MAX_DNS 4U
typedef struct {
    const uint8_t* certificate; uint32_t certificate_length;
    const uint8_t* tbs_certificate; uint32_t tbs_certificate_length;
    const uint8_t* tbs_certificate_der; uint32_t tbs_certificate_der_length;
    const uint8_t* serial; uint32_t serial_length;
    const uint8_t* issuer; uint32_t issuer_length;
    const uint8_t* subject; uint32_t subject_length;
    const uint8_t* not_before; uint32_t not_before_length;
    const uint8_t* not_after; uint32_t not_after_length;
    const uint8_t* subject_public_key_info; uint32_t subject_public_key_info_length;
    const uint8_t* subject_public_key_algorithm; uint32_t subject_public_key_algorithm_length;
    const uint8_t* rsa_modulus; uint32_t rsa_modulus_length;
    const uint8_t* rsa_exponent; uint32_t rsa_exponent_length;
    /* Vue SEC1 non compressée de la clé id-ecPublicKey secp256r1. */
    const uint8_t* ec_p256_public_key; uint32_t ec_p256_public_key_length;
    const uint8_t* common_name; uint32_t common_name_length;
    const uint8_t* subject_alt_names; uint32_t subject_alt_names_length;
    uint8_t basic_constraints_present; uint8_t basic_constraints_ca;
    uint8_t path_len_present; uint32_t path_len_constraint;
    const uint8_t* subject_key_identifier; uint32_t subject_key_identifier_length;
    const uint8_t* authority_key_identifier; uint32_t authority_key_identifier_length;
    uint8_t key_usage_present; uint8_t key_usage_key_cert_sign;
    uint8_t extended_key_usage_present; uint8_t extended_key_usage_server_auth;
    uint8_t name_constraints_present; uint8_t name_constraints_dns_permitted_count; uint8_t name_constraints_dns_excluded_count;
    const uint8_t* name_constraints_dns_permitted[X509_NAME_CONSTRAINTS_MAX_DNS];
    uint32_t name_constraints_dns_permitted_length[X509_NAME_CONSTRAINTS_MAX_DNS];
    const uint8_t* name_constraints_dns_excluded[X509_NAME_CONSTRAINTS_MAX_DNS];
    uint32_t name_constraints_dns_excluded_length[X509_NAME_CONSTRAINTS_MAX_DNS];
    const uint8_t* signature_algorithm; uint32_t signature_algorithm_length;
    const uint8_t* signature; uint32_t signature_length;
} x509_certificate_view_t;

int der_tlv_parse(const uint8_t* input,uint32_t length,der_tlv_view_t* out);
int x509_certificate_parse(const uint8_t* certificate,uint32_t length,x509_certificate_view_t* out);
/* Compare une identité DNS ASCII au SAN dNSName, ou au CN seulement sans dNSName présent. */
int x509_certificate_hostname_validate(const x509_certificate_view_t* certificate,const char* hostname);
/* Applique les sous-arbres DNS NameConstraints d'une CA à une identité DNS ASCII. */
int x509_certificate_name_constraints_dns_validate(const x509_certificate_view_t* certificate,const char* hostname);
/* Vérifie notBefore <= instant UTC <= notAfter ; l’instant est fourni comme YYYYMMDDhhmmssZ. */
int x509_certificate_valid_at(const x509_certificate_view_t* certificate,const char* utc_time);
/* Vérifie une feuille RSA/SHA-256 signée directement par l’ancre X.509 fournie par l’appelant. */
int x509_certificate_chain_validate_one(const x509_certificate_view_t* leaf,const x509_certificate_view_t* trust_anchor,
                                        uint32_t* workspace,uint16_t workspace_length);
/* Vérifie leaf -> intermédiaire -> ancre avec deux vérifications RSA/SHA-256 et workspace caller-owned. */
int x509_certificate_chain_validate_two(const x509_certificate_view_t* leaf,const x509_certificate_view_t* intermediate,
                                        const x509_certificate_view_t* trust_anchor,uint32_t* workspace,uint16_t workspace_length);
/* Exige chaîne RSA directe, hostname DNS et période UTC pour une identité TLS utilisable. */
int x509_certificate_tls_identity_validate(const x509_certificate_view_t* leaf,const x509_certificate_view_t* trust_anchor,
                                           const char* hostname,const char* utc_time,uint32_t* workspace,uint16_t workspace_length);
/* Vérifie leaf -> intermédiaire 1 -> intermédiaire 2 -> ancre avec trois signatures RSA/SHA-256. */
int x509_certificate_chain_validate_three(const x509_certificate_view_t* leaf,const x509_certificate_view_t* intermediate_one,
                                          const x509_certificate_view_t* intermediate_two,const x509_certificate_view_t* trust_anchor,
                                          uint32_t* workspace,uint16_t workspace_length);
/* Exige chaîne RSA leaf-intermédiaire-ancre, hostname et dates UTC pour les trois certificats. */
int x509_certificate_tls_identity_validate_two(const x509_certificate_view_t* leaf,const x509_certificate_view_t* intermediate,
                                               const x509_certificate_view_t* trust_anchor,const char* hostname,const char* utc_time,
                                               uint32_t* workspace,uint16_t workspace_length);
/* Exige une chaîne RSA à deux intermédiaires, hostname, NameConstraints et dates UTC. */
int x509_certificate_tls_identity_validate_three(const x509_certificate_view_t* leaf,const x509_certificate_view_t* intermediate_one,
                                                 const x509_certificate_view_t* intermediate_two,const x509_certificate_view_t* trust_anchor,
                                                 const char* hostname,const char* utc_time,uint32_t* workspace,uint16_t workspace_length);
/* Vérifie l’OID rsaEncryption, le module positif et l’exposant public impair pris en charge par RSA. */
int x509_rsa_public_key_validate(const x509_certificate_view_t* certificate);
/* Vérifie id-ecPublicKey + namedCurve secp256r1 et une clé SEC1 non compressée. */
int x509_ec_p256_public_key_validate(const x509_certificate_view_t* certificate);

#endif
