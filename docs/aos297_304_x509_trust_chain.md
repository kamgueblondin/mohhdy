# AOS-297 à AOS-304 — chaîne de confiance X.509 RSA minimale

## Périmètre livré

Ce macro-lot transforme le parseur X.509 en brique de vérification de confiance minimale. Une feuille X.509 peut être validée directement contre une ancre X.509 déjà parsée et fournie par l’appelant, à condition que la feuille soit signée avec `sha256WithRSAEncryption` par cette ancre.

| Élément | Garantie livrée |
|---|---|
| Ancre de confiance | Le certificat ancre et sa vue `x509_certificate_view_t` restent caller-owned. |
| Liaison d’émetteur | Le champ `issuer` de la feuille doit correspondre octet pour octet au `subject` de l’ancre. |
| Clé de l’ancre | L’OID `rsaEncryption`, le module positif et l’exposant RSA sont validés avant l’usage. |
| Signature de la feuille | Le DER complet du TBSCertificate est hashé avec SHA-256, puis vérifié par RSA PKCS#1 v1.5. |
| Workspace | La vérification RSA reçoit le workspace `uint32_t` de l’appelant et applique sa borne habituelle. |
| Vues ajoutées | Le parseur publie le DER TBSCertificate complet, l’algorithme de signature et la signature BIT STRING. |

## API

`x509_certificate_chain_validate_one(leaf, trust_anchor, workspace, workspace_length)` valide une feuille directement émise par une seule ancre. Elle exige l’égalité issuer/subject, une ancre RSA valide, l’algorithme `sha256WithRSAEncryption` et une signature de taille compatible avec le module RSA de l’ancre. Elle retourne le succès seulement après vérification cryptographique.

Le vecteur unitaire est généré localement et figé dans les sources : une feuille RSA/SHA-256 de 1024 bits, émise par une racine de test RSA de 1024 bits. Il vérifie la réussite cryptographique, le rejet d’un subject d’ancre altéré et le rejet d’une signature tronquée.

> Cette validation lie une feuille à une ancre explicite. Elle constitue une **chaîne à un seul saut**, pas une PKI générique ni une validation HTTPS de production complète.

## Contrat mémoire

Le certificat feuille, le certificat ancre, leurs vues et le workspace RSA sont tous fournis par l’appelant. Les fonctions n’allouent aucune mémoire dynamique et n’utilisent pas `kmalloc`.

## Limites explicites

Les intermédiaires, les chaînes de longueur arbitraire, l’auto-signature de l’ancre, les dates, `basicConstraints`, `keyUsage`, `extendedKeyUsage`, `pathLenConstraint`, `nameConstraints`, CRL, OCSP, ECDSA, EdDSA, SHA-384/SHA-512, les politiques de révocation et la rotation/configuration persistante des ancres sont hors périmètre. Le validateur hostname doit être appelé séparément, et l’orchestrateur NE2000 n’appelle pas encore automatiquement les validations hostname et chaîne avant le flight client. X25519/bigint reste non constante-temps.
