# AOS-769 à AOS-776 — Chaînes X.509 ECDSA P-256 à intermédiaire

**Statut : implémenté et validé.** Ce macro-lot démontre et sécurise la validation d’une chaîne X.509 ECDSA/SHA-256 de trois certificats — racine, intermédiaire, feuille serveur — dans la pile bare-metal i386. Il rend également le workspace cryptographique statique du chemin noyau DHCP→TLS→LLM compatible avec le vérificateur P-256 précédemment intégré.

La spécification ECC TLS distingue le mécanisme d’authentification de l’échange éphémère et n’impose pas que les signatures de la chaîne soient du même type que la signature du `ServerKeyExchange` [1]. La pile sélectionne donc explicitement l’algorithme de signature de chaque certificat X.509 puis utilise RSA ou ECDSA selon l’émetteur réellement déclaré par la chaîne.

> **Garantie du lot.** Une chaîne ECDSA racine–intermédiaire–feuille n’est acceptée que si chacune des deux signatures `ecdsa-with-SHA256`, les liaisons issuer/subject et AKI/SKI, les contraintes CA et de longueur de chemin, l’identité TLS et les dates sont toutes valides.

## Périmètre livré

| Élément | Comportement |
|---|---|
| Chaîne de test | Racine P-256 autosignée, intermédiaire P-256 CA `pathLen=0`, feuille P-256 pour `api.example.test`, toutes signées ECDSA/SHA-256. |
| Validation de signature | `x509_certificate_chain_validate_two` exécute les deux vérifications ECDSA avec les clés publiques des émetteurs. |
| Politique TLS | `x509_certificate_tls_identity_validate_two` impose hostname SAN, date, EKU serveur, usages de clé et contraintes de chaîne. |
| Flux réseau | Les façades `ne2k_tls_client_poll_chain_two` et `ne2k_tls_client_poll_received_chain` appliquent déjà ces validateurs avant de publier `peer_identity_validated`. |
| Workspace noyau | `KERNEL_LLM_TLS_WORKSPACE_WORDS` est lié à `ECDSA_P256_WORKSPACE_WORDS`, soit 2 048 mots de 32 bits. |
| Allocation | Aucun `kmalloc`, aucun buffer dynamique et aucune copie de certificat ne sont ajoutés. |

## Chaîne et séquence de contrôle

La feuille est signée par l’intermédiaire, puis l’intermédiaire par la racine. La validation suit la relation ci-dessous, en utilisant uniquement des vues DER contenues dans les buffers détenus par l’appelant.

```text
Feuille P-256 (SAN api.example.test)
       │  ECDSA/SHA-256, issuer/AKI
       ▼
Intermédiaire P-256 (CA, pathLen=0)
       │  ECDSA/SHA-256, issuer/AKI
       ▼
Racine P-256 (ancre immuable)
```

L’implémentation existante de `ne2k_tls_client_poll_internal` garde une copie transactionnelle du client et de la connexion. Lorsque le certificat TLS est reçu, elle parse les certificats intermédiaires, appelle le validateur de chaîne et ne pose `peer_identity_validated = 1` qu’après succès. Tout rejet restaure le client, la connexion, le transcript et les compteurs de consommation.

## Workspace cryptographique borné

L’ancien buffer statique de 224 mots couvrait les vérifications RSA de test mais ne permettait pas l’inversion modulaire P-256. Le contexte noyau emploie désormais 2 048 mots, la capacité définie par le contrat `ecdsa_p256.h`. Les deux tableaux historiques — nommé `rsa_workspace` par compatibilité d’ABI et le workspace X25519 distinct — restent des objets statiques noyau effacés lors de la purge de session.

| Ressource | Taille | Propriété |
|---|---:|---|
| Workspace de validation RSA/ECDSA | 2 048 × 32 bits | Statique noyau, caller-owned au niveau des API TLS/NE2000. |
| Workspace X25519 | 2 048 × 32 bits | Statique noyau existant, séparé du validateur de signature. |
| Certificats DER | 427, 435 et 477 octets dans les fixtures | Vues bornées, sans allocation. |
| Signature ECDSA | DER, au plus 72 octets | Vérifiée par `ecdsa_p256_sha256_verify`. |

## Régressions ajoutées

Le nouveau scénario Unity parse les trois certificats DER générés et vérifiés localement avec OpenSSL, valide chacune de leurs clés P-256, vérifie la chaîne puis l’identité TLS de `api.example.test`. Il rejette explicitement les cas suivants : workspace inférieur à 2 048 mots, signature de feuille altérée, intermédiaire non-CA et contrainte de chemin de la racine réduite à zéro.

| Vérification | Résultat |
|---|---|
| `test_x509_der` ciblé | 14/14 scénarios réussis, dont la nouvelle chaîne ECDSA à intermédiaire. |
| Build i386 (`make test-build`) | Image freestanding réussie. |
| Régression (`make test-all`) | **393/393** tests passés. |
| `make qemu-ai-provider` | Smoke fournisseur IA réussi. |
| `make qemu-ne2k-status` | Smoke NE2000 réussi. |

## Fichiers modifiés

| Fichier | Rôle |
|---|---|
| `kernel/kernel.c` | Dimensionne le workspace TLS noyau avec la constante P-256 officielle. |
| `tests/unit/kernel/x509_ecdsa_chain_vectors.inc` | Fixtures DER racine, intermédiaire et feuille P-256. |
| `tests/unit/kernel/test_x509_der.c` | Chaîne ECDSA à intermédiaire et rejets cryptographiques/politiques. |

## Limites et suite logique

Le lot couvre une chaîne à un intermédiaire, déjà représentée par les façades TLS réseau. La validation de chaînes plus profondes existe pour RSA et devra être exercée avec des fixtures ECDSA dédiées si le déploiement le nécessite. La prochaine étape fonctionnelle consiste à exécuter un handshake HTTPS complet contre un serveur de test contrôlé avec certificat ECDSA et chaîne intermédiaire, puis à valider les échanges HTTP LLM de bout en bout.

## Références

[1] [IETF, *RFC 8422 — Elliptic Curve Cryptography (ECC) Cipher Suites for TLS Versions 1.2 and Earlier*](https://www.rfc-editor.org/rfc/rfc8422.html)

[2] [NIST, *FIPS 186-5 — Digital Signature Standard (DSS)*](https://csrc.nist.gov/pubs/fips/186-5/final)
