# AOS-721 à AOS-728 — Chaîne TLS RSA à deux intermédiaires

**Auteur :** Manus AI

**Statut :** implémenté et validé localement

**Périmètre :** support borné de la chaîne `feuille → intermédiaire 1 → intermédiaire 2 → ancre` dans le handshake TLS 1.2

## Objectif

Le parser du message TLS `Certificate` retient désormais la feuille et jusqu’à deux certificats intermédiaires. La profondeur est volontairement fixe : une quatrième entrée est rejetée. Cette borne conserve des structures statiques, une consommation mémoire prévisible et aucun besoin d’allocation dynamique, tout en couvrant les chaînes RSA à deux intermédiaires nécessaires à certaines hiérarchies publiques.

| Élément | Contrat |
|---|---|
| Profondeur acceptée | Feuille plus zéro, un ou deux intermédiaires. |
| Chaîne de deux intermédiaires | `leaf → intermediate_one → intermediate_two → trust_anchor`. |
| Profondeur supérieure | Rejet explicite au parsing `Certificate`. |
| Cryptographie | RSA PKCS#1 v1.5 SHA-256, workspace fourni par l’appelant. |
| Buffers | Pointeurs vers le message TLS déjà accumulé ; aucune copie ni allocation dynamique. |

## Validation de confiance

`x509_certificate_chain_validate_three` vérifie trois signatures RSA SHA-256. Les deux intermédiaires doivent être des CA autorisées avec `BasicConstraints CA:true` et `KeyUsage keyCertSign`. La contrainte `pathLen` de l’intermédiaire le plus proche de l’ancre doit autoriser au moins une CA sous-jacente, et l’ancre doit autoriser au moins deux niveaux si elle définit cette contrainte.

`x509_certificate_tls_identity_validate_three` ajoute la vérification de l’EKU serveur de la feuille, du hostname, des périodes de validité des quatre certificats et des NameConstraints DNS de chaque CA concernée. La validation est réalisée avant la génération du flight X25519 et avant toute publication de session TLS complète.

> Toute erreur de parsing, d’autorisation CA, de liaison AKI/SKI, de contrainte de chemin, de signature, de date, de hostname ou de NameConstraints conserve le rollback de l’orchestrateur TLS.

| Vérification | Feuille | Intermédiaire 1 | Intermédiaire 2 | Ancre |
|---|---:|---:|---:|---:|
| Signature RSA SHA-256 | Oui | Oui | Oui | Clé de vérification |
| CA + `keyCertSign` | Non requis | Requis | Requis | Ancre immuable |
| `pathLen` | Non applicable | Géré par l’émetteur | Au moins 1 si présent | Au moins 2 si présent |
| Date UTC | Oui | Oui | Oui | Oui |
| NameConstraints DNS | Identité évaluée | Oui | Oui | Oui si présent |

## Parser et orchestration NE2000

`net_tls_certificate_parse` publie `certificate`, `intermediate` et `intermediate_two`. Les structures de handshake conservent leurs vues X.509 et indicateurs de validité associés. Quand un second intermédiaire est présent, `ne2k_tls_client_poll_internal` sélectionne obligatoirement la validation à trois signatures ; il ne peut pas dégrader la chaîne vers une validation directe ou à un seul intermédiaire.

| Liste `Certificate` reçue | Chemin appliqué |
|---|---|
| Feuille seule | Validation directe contre l’ancre. |
| Feuille + un intermédiaire | Validation existante à deux signatures. |
| Feuille + deux intermédiaires | Nouvelle validation à trois signatures. |
| Plus de deux intermédiaires | Rejet au parsing. |

## Régressions ajoutées

Une nouvelle fixture DER RSA SHA-256 génère une chaîne complète `leaf → int1 → int2 → root`, avec SAN `api.example.test`, EKU serveur, AKI/SKI, `BasicConstraints`, `keyCertSign` et `pathLen`. Les tests Unity vérifient le succès de la chaîne et les refus lorsque `pathLen` est insuffisant, quand un intermédiaire perd son droit CA ou lorsque la liaison émetteur/sujet est altérée. Un test de parser confirme la capture du second intermédiaire et le rejet d’un quatrième certificat.

| Vérification | Résultat |
|---|---|
| Nouvelle chaîne RSA à deux intermédiaires | Validée. |
| `pathLen` trop faible | Rejeté. |
| CA intermédiaire invalide | Rejeté. |
| Émetteur/sujet altéré | Rejeté. |
| Quatrième entrée Certificate | Rejetée. |
| Compilation freestanding i386 | Réussie. |
| Smokes QEMU fournisseur et NE2000 | Réussis. |
| Suite complète | Réussie : **385/385** tests. |

## Limites connues

La profondeur est volontairement limitée à deux intermédiaires et ECDSA n’est pas encore implémenté. La compatibilité live dépend toujours de la chaîne réellement présentée par l’endpoint, de RDRAND, du RTC et de l’ancre compilée. Le credential OpenAI, la révocation, la fermeture/annulation, timeout/retry, outils, multimodal et Unicode complet restent hors périmètre.

## Références

[1] [AOS-713 à AOS-720 — ancre X.509 RSA immuable pour TLS](aos713_720_tls_immutable_trust_anchor.md)

[2] [AOS-705 à AOS-712 — entropie matérielle RDRAND pour TLS](aos705_712_tls_rdrand_entropy.md)

[3] [Let’s Encrypt — Chains of Trust](https://letsencrypt.org/certificates/)
