# AOS-761 à AOS-768 — TLS 1.2 ECDHE_ECDSA authentifié

**Statut : implémenté et validé.** Ce macro-lot ajoute la suite TLS 1.2 `TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256` (`0xC02B`) à la pile bare-metal i386. Il l’associe à un certificat serveur P-256/X.509 et vérifie cryptographiquement le `ServerKeyExchange` avant que la clé X25519 éphémère puisse être acceptée ou employée dans le flight client.

RFC 8422 décrit `ECDHE_ECDSA` comme un échange ECDH éphémère authentifié avec ECDSA : le certificat serveur doit contenir une clé apte à ECDSA, et les paramètres éphémères transmis dans `ServerKeyExchange` doivent être signés avec la clé privée associée [1]. TLS 1.2 encode aussi explicitement le couple algorithme de hachage / algorithme de signature dans les éléments signés [2].

> **Principe de sécurité.** L’état du handshake ne passe jamais à `SERVER_KEY_EXCHANGE_RECEIVED` tant que la signature ECDSA/SHA-256 des paramètres X25519 n’a pas été vérifiée avec la clé P-256 du certificat serveur.

## Négociation des suites

Le ClientHello TLS 1.2 annonce désormais les deux suites AES-128-GCM éphémères, par ordre de préférence ECDSA puis RSA. Les extensions déjà présentes déclarent X25519 comme groupe pris en charge et le format de point non compressé. Cette annonce est cohérente avec le registre de groupes ECC TLS, où `x25519` vaut 29 [1].

| Suite | Identifiant | Certificat serveur exigé | Signature ServerKeyExchange | Échange éphémère accepté |
|---|---:|---|---|---|
| `TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256` | `0xC02B` | `id-ecPublicKey` + `secp256r1` P-256 | ECDSA avec SHA-256 (`4,3`) | X25519 (groupe 29, 32 octets) |
| `TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256` | `0xC02F` | `rsaEncryption` valide | RSA PKCS#1 v1.5 avec SHA-256 (`4,1`) | X25519 (groupe 29, 32 octets) |

La fonction `net_tls_cipher_suite_is_ecdhe_aes128_gcm` regroupe les deux cas uniquement pour les étapes communes X25519, PRF et AES-GCM. Les validations de certificat et de signature restent séparées et sélectionnées exclusivement à partir de la suite négociée.

## Authentification de ServerKeyExchange

Le nouveau chemin `net_tls_handshake_accept_server_key_exchange_ecdsa` applique les contrôles ci-dessous avant de publier la clé éphémère au handshake.

| Étape | Contrôle appliqué | Échec |
|---|---|---|
| État | Certificat reçu, X.509 déjà parsé, aléa serveur présent | Rejet sans transition d’état. |
| Suite | `0xC02B` exactement | Rejet de toute confusion RSA/ECDSA. |
| Paramètres | `named_curve = x25519`, clé publique de 32 octets | Rejet des formes ECDH non prises en charge. |
| Certificat | `id-ecPublicKey`, `secp256r1`, SEC1 non compressé de 65 octets | Rejet avant calcul de signature. |
| Signature TLS | `hash = SHA-256`, `signature = ECDSA`, DER borné à 72 octets | Rejet des algorithmes et longueurs non canoniques. |
| Preuve | `SHA-256(client_random || server_random || ServerECDHParams)` | Vérification P-256 avec le workspace caller-owned. |

La valeur hashée couvre exactement les deux aléas TLS et tous les octets `ServerECDHParams`, c’est-à-dire le type de courbe, le groupe, la longueur et la clé publique éphémère. Une signature ECDSA altérée, un point X25519 de mauvaise taille, un mauvais identifiant de signature ou un workspace insuffisant échouent avant l’acceptation du message.

## État transactionnel et compatibilité

Le dispatcher `net_tls_handshake_accept_server_message_authenticated` choisit RSA ou ECDSA après `ServerHello`. À la réception du certificat, il valide la forme de clé compatible avec la suite ; à `ServerKeyExchange`, il appelle uniquement le vérificateur compatible. En cas d’échec, il restaure la totalité de la structure de handshake et n’ajoute aucun octet au transcript.

Le code X25519 partagé, le calcul de master secret, le key block AES-GCM et le flight client acceptent maintenant les deux suites ECDHE authentifiées. Cela préserve la même confidentialité persistante conditionnelle à des clés éphémères fraîches, propriété mise en avant pour ECDHE par RFC 8422 [1]. Le parcours RSA est conservé ; son fixture TCP a été corrigé pour employer un vrai vecteur RSA/X25519 au lieu d’une forme P-256 incompatible avec la suite RSA.

## Mémoire et contraintes bare-metal

Aucun chemin ajouté ne contient d’allocation dynamique. Les certificats, messages TLS et signatures demeurent des vues dans les buffers de l’appelant. Le hash SHA-256 est local et fixe. Le workspace P-256 de 2 048 mots reste explicitement fourni par l’appelant ; il est réutilisé par TCP comme espace cryptographique pour RSA ou ECDSA selon la suite.

| Ressource | Propriété |
|---|---|
| ClientHello | Tableau local fixe de 65 octets, recopié dans un record caller-owned. |
| Certificat et ServerKeyExchange | Vues sur les buffers TLS reçus. |
| Aléas et digest | Tableaux locaux fixes de 32 octets. |
| Vérification ECDSA | Workspace P-256 de 2 048 mots détenu par l’appelant. |
| État / transcript | Copie transactionnelle existante, sans allocation. |
| Allocation dynamique | **Aucune.** |

## Tests et validations

Le test TLS introduit un certificat P-256 DER et un `ServerKeyExchange` X25519 signé en ECDSA/SHA-256 avec la clé correspondante. Le vecteur est validé localement avec OpenSSL avant d’être figé. Il couvre le dispatcher authentifié complet, l’acceptation de la clé X25519, le transcript et le rejet de la signature altérée. Le test TCP continue de couvrir la fragmentation, la consommation exacte et le rollback, avec une signature RSA réelle sur paramètres X25519.

| Vérification | Résultat |
|---|---|
| `test_net_tls_record` ciblé | 24/24 scénarios passés. |
| `test_net_tcp` ciblé | Tous les scénarios passés, dont le flux RSA/X25519 transactionnel. |
| `make test-build` | Image i386 freestanding compilée avec succès. |
| `make test-all` | **392/392** tests passés. |
| `make qemu-ai-provider` | Smoke fournisseur IA réussi. |
| `make qemu-ne2k-status` | Smoke NE2000 réussi. |

## Fichiers modifiés

| Fichier | Rôle |
|---|---|
| `kernel/net_tls_record.h` | Suite `0xC02B`, codes de signature et API ECDSA de ServerKeyExchange. |
| `kernel/net_tls_record.c` | ClientHello bi-suite, dispatch cohérent, vérification ECDSA et compatibilité X25519. |
| `tests/unit/kernel/tls_ecdsa_vectors.inc` | Fixture ServerKeyExchange X25519 réellement signée en ECDSA. |
| `tests/unit/kernel/test_net_tls_record.c` | Régression TLS ECDHE_ECDSA positive et signature altérée. |
| `tests/unit/kernel/test_net_tcp.c` | Fixture RSA/X25519 réel et assertions de fragmentation ajustées. |

## Limites et suite logique

Ce lot ne modifie pas la politique de confiance, la révocation, le client-auth ECDSA ni le support de toutes les courbes TLS. La chaîne ECDSA validée est actuellement la chaîne directe couverte par AOS-753 à AOS-760. Le prochain approfondissement logique consiste à étendre les chaînes ECDSA à intermédiaires, à intégrer d’autres groupes compatibles si nécessaire, puis à exercer un handshake HTTPS de bout en bout contre un serveur de test contrôlé.

## Références

[1] [IETF, *RFC 8422 — Elliptic Curve Cryptography (ECC) Cipher Suites for TLS Versions 1.2 and Earlier*](https://www.rfc-editor.org/rfc/rfc8422.html)

[2] [IETF, *RFC 5246 — The Transport Layer Security (TLS) Protocol Version 1.2*](https://datatracker.ietf.org/doc/html/rfc5246)
