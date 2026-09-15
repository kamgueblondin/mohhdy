# AOS-745 à AOS-752 — Vérification ECDSA P-256/SHA-256 bornée

**Statut : implémenté et validé.** Ce macro-lot ajoute au noyau i386 freestanding un vérificateur de signatures **ECDSA sur secp256r1 (P-256) avec SHA-256**. Il constitue le socle cryptographique requis pour authentifier ultérieurement des certificats X.509 et des messages TLS signés en ECDSA. La norme FIPS 186-5 décrit ECDSA comme un mécanisme de génération et de vérification de signatures numériques, tandis que RFC 5480 définit la représentation des clés ECC dans les certificats et la forme non compressée d’un point public [1] [2].

> **Portée de sécurité.** Le module vérifie une signature déjà associée à une clé publique P-256 et à un condensat SHA-256 de 32 octets. Il n’effectue pas encore l’extraction `id-ecPublicKey` depuis X.509, la validation d’une chaîne de certificats, le contrôle d’hôte ou la négociation de la suite TLS ECDHE-ECDSA.

## Contrat public

L’interface `kernel/ecdsa_p256.h` expose `ecdsa_p256_sha256_verify`. Elle ne stocke aucun état cryptographique mutable global et ne possède aucun buffer interne dépendant de la taille d’une entrée réseau.

| Élément | Contrat |
|---|---|
| Clé publique | Exactement 65 octets, SEC1 non compressé : `0x04 || X || Y`. |
| Condensat | Exactement 32 octets SHA-256, fourni par l’appelant. |
| Signature | `SEQUENCE(INTEGER r, INTEGER s)` DER, maximum 72 octets. |
| Workspace | `ECDSA_P256_WORKSPACE_WORDS` = **2 048** mots de 32 bits appartenant à l’appelant. |
| Succès | Retour `0` uniquement si la signature est mathématiquement valide. |
| Échec | Retour négatif pour une entrée invalide, une taille insuffisante ou un échec de vérification. |
| Allocation dynamique | **Interdite** : aucun appel à `kmalloc`, `malloc` ou mécanisme équivalent. |

Le format SEC1 non compressé est cohérent avec RFC 5480 : le premier octet `0x04` désigne cette forme, alors que les autres préfixes doivent être rejetés lorsqu’ils ne représentent pas une clé attendue [2].

## Conception de l’implémentation

Les valeurs de champ et les scalaires sont stockés dans huit limbs `uint32_t` little-endian. Le module réutilise l’arithmétique multi-précision déjà présente dans `bigint.c`, tout en encapsulant ses objets sans les initialiser destructivement. Cette distinction est essentielle : une vue bigint doit référencer les limbs P-256 existants sans effacer l’opérande de courbe.

| Sous-système | Réalisation livrée |
|---|---|
| Domaine P-256 | Constantes du premier `p`, de l’ordre `n`, du coefficient `b` et du générateur `G`. |
| Champ premier | Addition, soustraction, multiplication et inversion modulo `p`. |
| Groupe | Points Jacobiennes, doublement, addition, multiplication scalaire binaire. |
| Ordre de sous-groupe | Inversion de `s` et calcul de `u1 = e·s⁻¹ mod n`, `u2 = r·s⁻¹ mod n`. |
| Vérification | Calcul de `u1·G + u2·Q`, conversion affine, réduction modulo `n`, comparaison avec `r`. |
| Robustesse d’aliasing | Les wrappers de produit et de réduction préservent explicitement leurs entrées lorsqu’une sortie est réutilisée en place. |

L’inversion modulaire est réalisée par exponentiation avec `p−2` ou `n−2`, selon le petit théorème de Fermat. Les multiplications scalaires ne réalisent aucune allocation et utilisent des variables automatiques de taille fixe. Le seul workspace transmis par l’appelant est réservé aux exponentiations multi-limbs ; une taille inférieure au contrat est rejetée avant toute opération cryptographique.

## Validation des entrées et DER

La clé publique est refusée si son préfixe n’est pas `0x04`, si `X` ou `Y` est hors champ, ou si le point ne satisfait pas l’équation de P-256. Ce contrôle appartient au chemin de vérification et évite d’appliquer l’arithmétique de groupe à une clé SEC1 mal formée.

Le parseur de signature accepte uniquement une séquence DER de longueur courte et exactement deux entiers positifs. Il refuse les longueurs indéfinies ou longues, les entiers vides, négatifs ou trop grands, les zéros de tête non nécessaires, les scalaires nuls et les scalaires hors de l’intervalle `[1, n−1]`.

| Contrôle DER/scalaire | Comportement |
|---|---|
| En-tête différent de `SEQUENCE` | Rejet. |
| Longueur de séquence incohérente ou longue | Rejet. |
| `INTEGER` négatif, vide ou > 33 octets | Rejet. |
| Zéro de tête non canonique | Rejet. |
| `r = 0`, `s = 0`, `r ≥ n` ou `s ≥ n` | Rejet. |
| Point hors courbe, coordonnées ≥ `p` | Rejet. |

## Tests et non-régression

Le nouveau test Unity `test_ecdsa_p256` utilise un vecteur P-256/SHA-256 généré localement par OpenSSL puis figé dans le dépôt. Il valide le chemin accepté et quatre familles de rejet. L’intégration au lanceur global garantit que le test est compilé avec `ecdsa_p256.c` et `bigint.c` dans la même configuration i386 freestanding que le reste de la suite.

| Validation | Résultat |
|---|---|
| Vecteur signature ECDSA valide | Accepté. |
| Signature dont un octet est altéré | Rejetée. |
| Point public modifié et hors courbe | Rejeté. |
| DER non canonique | Rejeté. |
| Workspace inférieur à 2 048 mots | Rejeté. |
| Test isolé Unity | 3/3 scénarios passés. |
| Suite complète `make test-all` | **390/390** passés. |
| Build i386 `make test-build` | Réussi. |
| Smoke QEMU fournisseur IA | Réussi. |
| Smoke QEMU NE2000 | Réussi. |

## Fichiers concernés

| Fichier | Rôle |
|---|---|
| `kernel/ecdsa_p256.h` | Contrat P-256/SHA-256 et constantes de tailles. |
| `kernel/ecdsa_p256.c` | Vérification ECDSA, arithmétique de courbe, DER strict et validation de point. |
| `tests/unit/kernel/test_ecdsa_p256.c` | Vecteur valide et tests de rejet Unity. |
| `Makefile` | Compilation de `build/ecdsa_p256.o` dans l’image noyau. |
| `tests/Makefile` | Construction ciblée du binaire Unity P-256. |
| `tests/scripts/run_all_tests.sh` | Linkage P-256/bigint du test dans la suite globale i386. |

## Suite prévue

Le prochain macro-lot logique doit étendre le parseur X.509 DER afin d’extraire `id-ecPublicKey` avec `namedCurve secp256r1`, fournir la clé SEC1 non compressée au vérificateur et authentifier les signatures ECDSA-SHA256 de certificats. Après cette étape, la suite TLS `TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256` pourra vérifier la signature `ServerKeyExchange` avant d’accepter les paramètres ECDHE.

## Références

[1] [NIST, *FIPS 186-5 — Digital Signature Standard (DSS)*](https://csrc.nist.gov/pubs/fips/186-5/final)

[2] [IETF, *RFC 5480 — Elliptic Curve Cryptography Subject Public Key Information*](https://www.rfc-editor.org/info/rfc5480/)
