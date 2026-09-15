# AOS-225 à AOS-232 — bigint multi-limb, RSA PKCS#1 v1.5 et ServerKeyExchange TLS

## Portée livrée

Ce macro-lot rend disponible une **vérification RSA PKCS#1 v1.5 avec SHA-256** dans le noyau i386 freestanding. Toutes les structures numériques, sorties et zones de travail restent fournies par l’appelant ; aucun appel à `kmalloc`, aucune allocation dynamique et aucune dépendance à une bibliothèque de cryptographie hôte ne sont introduits dans le code noyau.

| Composant | État livré |
|---|---|
| `bigint_add`, `bigint_subtract`, `bigint_multiply` | Opérations multi-limb, capacité fixe, contrôle de débordement. |
| Réduction modulaire | Réduction binaire bornée, appliquée bit par bit avec reste inférieur au module. |
| `bigint_modexp_u32` | Square-and-multiply multi-limb pour un exposant public sur 32 bits. |
| `rsa_pkcs1_v15_sha256_verify` | Exponentiation publique, décodage EMSA-PKCS1-v1_5 SHA-256 et contrôle de tout l’encodage. |
| TLS `ServerKeyExchange` | API authentifiée explicite `net_tls_handshake_accept_server_key_exchange_rsa`. |
| Tests | Vecteur RSA-512 local pour SHA-256(`abc`), rejets digest/signature altérés, et intégration TLS signée. |

## Contrat mémoire et bornes

La vérification RSA accepte le module, l’exposant, le digest et la signature en entrée non modifiée. Le workspace est un tableau de `uint32_t` aligné, appartenant à l’appelant. Pour une taille de module `c = ceil(modulus_length / 4)` limbs, il doit compter au moins **`7 × c` limbs**. Trois zones représentent le module, la signature et le résultat, tandis que quatre zones sont réservées à l’exponentiation modulaire.

| Taille de clé | `c` | Workspace minimal | Allocation dynamique |
|---:|---:|---:|---|
| RSA-512 | 16 limbs | 112 limbs / 448 octets | Aucune |
| RSA-2048 | 64 limbs | 448 limbs / 1 792 octets | Aucune |

La multiplication modulaire utilise une stratégie de doublement-addition suivie d’une réduction binaire. Elle favorise la simplicité de vérification, la portabilité i386 et des bornes de mémoire strictes plutôt que la performance d’une réduction Montgomery ou Barrett. L’exposant RSA doit tenir dans un entier non signé 32 bits, ce qui couvre l’exposant de vérification usuel `65537`.

> Cette première implémentation est **fonctionnelle mais non optimisée**. Les branches dépendantes des données ne permettent pas de présenter l’implémentation comme résistante aux canaux auxiliaires. Elle ne manipule cependant aucune clé privée dans le chemin de vérification RSA.

## Vérification TLS disponible

`net_tls_handshake_accept_server_key_exchange_rsa` reçoit le `client_random`, le message `ServerKeyExchange` et le workspace RSA. Après parsing strict, elle calcule :

```text
SHA-256(client_random || server_random || ServerECDHParams)
```

Elle impose `sha256` / `rsa` dans le champ `DigitallySigned`, vérifie la signature avec la clé RSA extraite du certificat X.509, puis seulement alors fait progresser l’automate vers `SERVER_KEY_EXCHANGE_RECEIVED`. Une signature corrompue laisse l’état du handshake à `CERTIFICATE_RECEIVED`.

L’ancienne fonction `net_tls_handshake_accept_server_key_exchange` reste un parseur et une transition d’état historique. Elle est désormais documentée comme **non authentifiée**. Le dispatch générique existant ne peut pas appeler l’API authentifiée car son contrat ne reçoit ni le `client_random` ni le workspace RSA ; il ne faut donc pas le considérer comme un handshake TLS validé cryptographiquement.

## Validation

Les tests ajoutés couvrent un vecteur RSA-512 PKCS#1 v1.5 SHA-256 construit localement, un digest falsifié, une signature falsifiée, une exponentiation bigint sur deux limbs et l’acceptation/rejet TLS signée. La validation de soumission a produit **335/335 tests réussis**, un build i386 freestanding réussi et les smoke tests `qemu-ai-provider` et `qemu-ne2k-status` réussis.

## Limites explicites

Cette livraison ne rend pas encore un client HTTPS ni un appel LLM sécurisé de bout en bout fonctionnel. Les éléments suivants restent nécessaires : validation de chaîne X.509, ancres de confiance, dates, nom d’hôte, usages de clé, prise en charge des signatures ECDSA, ECDHE P-256/X25519, secret partagé, négociation complète des suites et raccordement du dispatcher TLS au chemin authentifié avec des buffers applicatifs sûrs.

La politique PKCS#1 v1.5 est présente pour l’interopérabilité avec TLS 1.2 RSA ; aucun nouveau design ne doit l’employer à la place de RSA-PSS lorsqu’un protocole moderne permet ce dernier.
