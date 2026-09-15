# AOS-233 à AOS-240 — Flux TLS authentifié, X.509 RSA et paramètres ECDHE

## Objet du macro-lot

Ce macro-lot transforme la vérification RSA précédemment disponible en un **flux de handshake TLS explicitement authentifié**. Il ne remplace pas le dispatcher historique, qui conserve son rôle de parseur de compatibilité, mais introduit un chemin distinct qui accepte le `client_random` et le workspace RSA indispensables à la vérification cryptographique.

| Élément | Comportement livré |
|---|---|
| `net_tls_handshake_accept_server_message_authenticated` | Dispatch transactionnel : restauration de l’état et du transcript à la moindre erreur. |
| Certificate | Parsing DER/X.509 puis validation de la clé `rsaEncryption` exploitable par la primitive RSA courante. |
| ServerKeyExchange | Vérification RSA/SHA-256 obligatoire avant transition d’état. |
| Paramètres ECDHE | Validation de forme pour secp256r1 et X25519, sans calcul de secret partagé. |
| AlgorithmIdentifier | Vérification de l’OID `rsaEncryption` et du paramètre DER `NULL`. |

## Contrat du dispatcher authentifié

L’appelant fournit à chaque message serveur le `client_random` original, le transcript caller-owned et, pour `ServerKeyExchange`, le workspace RSA en limbs. Le dispatcher analyse et transcrit `ServerHello`. Lors de `Certificate`, il exige que le certificat expose une clé RSA au format `rsaEncryption` avec un module DER positif et un exposant public impair représentable sur 32 bits. Lors de `ServerKeyExchange`, il exige une signature `sha256` / `rsa`, calcule le hash TLS et délègue à la vérification PKCS#1 v1.5.

En cas d’échec de parsing, de certificat, de forme ECDHE, de signature, ou de capacité insuffisante du transcript, l’état de handshake et la longueur du transcript sont restaurés à leur valeur d’entrée. Le flux n’introduit aucune copie cachée, aucune allocation dynamique ni zone globale de session.

## Formes ECDHE acceptées

La validation actuelle traite uniquement la **structure** de la clé éphémère annoncée. Elle accepte un point non compressé de 65 octets commençant par `0x04` pour la courbe TLS `23` (secp256r1), ainsi qu’une clé de 32 octets pour la courbe TLS `29` (X25519).

> Cette vérification de forme ne calcule pas ECDHE, ne vérifie pas l’appartenance du point à la courbe et ne dérive aucun secret partagé. Elle ne doit donc pas être interprétée comme une implémentation de P-256 ou X25519.

## Validation X.509 RSA

Le lecteur DER conserve désormais le contenu de l’`AlgorithmIdentifier` de la clé publique du sujet. Le validateur RSA impose l’OID DER de `rsaEncryption`, un paramètre `NULL`, un module positif encodé canoniquement et un exposant impair au moins égal à trois, sur quatre octets au maximum. Cette validation cible le chemin RSA disponible ; elle ne constitue pas une validation de certificat complète.

## Couverture de test

Les tests ajoutés et étendus vérifient un ServerKeyExchange secp256r1 de forme valide signé par RSA, le rejet d’une signature modifiée, le rejet d’une longueur de clé éphémère invalide, la validation de l’AlgorithmIdentifier RSA, et la restauration transactionnelle du dispatcher authentifié après un Certificate DER invalide. La validation de soumission produit **337/337 tests réussis**, un build i386 freestanding réussi et les smoke tests `qemu-ai-provider` et `qemu-ne2k-status` réussis.

## Limites explicites

La chaîne de confiance X.509, les dates, le nom d’hôte, les usages de clé, les signatures de certificat, ECDSA, l’arithmétique de courbe P-256, X25519, le secret partagé ECDHE, la sélection complète des suites, HTTP sécurisé et les appels LLM HTTPS de bout en bout restent non implémentés. Le dispatcher historique est conservé pour les tests de parsing mais **ne doit pas être utilisé pour déclarer un handshake cryptographiquement authentifié**.
