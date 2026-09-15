# AOS-489 à AOS-496 — Contraintes de nom DNS X.509 pour les intermédiaires TLS

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** DER/X.509 freestanding, chaîne TLS à un intermédiaire, DNS ASCII

## Objectif

Ce macro-lot ajoute un sous-ensemble strict, borné et sans allocation dynamique de l’extension X.509 **NameConstraints**. L’objectif est de limiter les identités DNS qu’un certificat intermédiaire est autorisé à certifier sur le chemin TLS `leaf → intermediate → trust anchor`.

> RFC 5280 définit `NameConstraints` comme une extension réservée aux certificats CA. Les sous-arbres exclus invalident toujours une identité, même lorsqu’elle apparaît aussi dans un sous-arbre autorisé [1].

La mise en œuvre ne conserve que des pointeurs et longueurs vers le DER fourni par l’appelant. Elle n’introduit ni `kmalloc`, ni copie de certificat, ni buffer global mutable.

## Sous-ensemble pris en charge

| Élément | Décision d’implémentation |
|---|---|
| OID d’extension | `id-ce-nameConstraints` / `2.5.29.30` (`55 1d 1e`). |
| Forme de nom | `dNSName` uniquement, tag contextuel `[2]` (`82`). |
| Sous-arbres | Jusqu’à **4** sous-arbres autorisés et **4** sous-arbres exclus, référencés dans la vue `x509_certificate_view_t`. |
| Distances | `minimum` implicite, ou explicitement égal à zéro ; `maximum` et toute distance non nulle sont refusés. |
| Comparaison DNS | ASCII, insensible à la casse, à frontière de label. |
| Priorité | Toute correspondance avec un sous-arbre exclu est refusée avant l’évaluation des sous-arbres permis. |
| Extension vide ou dupliquée | Refusée au parsing. |
| Forme de nom non DNS | Refusée de façon conservative dans ce sous-ensemble. |

La structure ASN.1 contrôlée est `NameConstraints ::= SEQUENCE { permittedSubtrees [0] OPTIONAL, excludedSubtrees [1] OPTIONAL }`. Chaque champ présent doit contenir une `SEQUENCE OF GeneralSubtree` non vide [1].

## Sémantique DNS

Une contrainte `example.test` accepte `example.test` et ses descendants comme `api.example.test`. Une contrainte `.example.test` n’accepte que les descendants, par exemple `api.example.test`, et refuse le domaine racine `example.test`. Une identité doit correspondre à un sous-arbre autorisé lorsqu’une liste `permittedSubtrees` DNS est présente. Elle est refusée dès qu’elle correspond à l’un des sous-arbres exclus.

| Sous-arbre permis | Sous-arbre exclu | Identité | Résultat |
|---|---|---|---|
| `example.test` | `bad.example.test` | `api.example.test` | Acceptée. |
| `example.test` | `bad.example.test` | `bad.example.test` | Refusée : exclusion prioritaire. |
| `example.test` | — | `api.wrong.test` | Refusée : hors sous-arbre permis. |
| `.example.test` | — | `example.test` | Refusée : le point initial exige un descendant. |
| `.example.test` | — | `api.example.test` | Acceptée. |

## Intégration à la politique TLS

`x509_certificate_tls_identity_validate_two` vérifie désormais, après l’authentification cryptographique `leaf → intermediate → trust anchor` et avant la validation du hostname de la feuille, que l’hôte TLS demandé respecte les contraintes DNS exposées par l’intermédiaire. Une violation interrompt la politique avec un statut d’erreur ; aucune mutation transactionnelle du contexte réseau n’est effectuée par cette routine pure de validation.

La chaîne directe `leaf → trust anchor` reste inchangée : ce macro-lot porte explicitement sur les contraintes imposées par l’intermédiaire dans le chemin à deux certificats.

## Contrat mémoire et robustesse

| Propriété | Garantie |
|---|---|
| Allocation dynamique | Aucune. |
| Propriété des données | Pointeurs vers le DER caller-owned. |
| Bornes | Maximum fixe de quatre sous-arbres DNS par liste. |
| Framing DER | Séquences, tags, longueurs, unicité et contenu non vide vérifiés. |
| Échec conservatif | Toute forme de nom ou distance non prise en charge est rejetée plutôt qu’ignorée. |
| Priorité de sécurité | `excludedSubtrees` est évalué avant `permittedSubtrees`. |

## Tests et validation locale

Le test Unity X.509 ajoute un certificat DER synthétique qui encode une extension critique `NameConstraints` avec un sous-arbre permis `example.test` et un sous-arbre exclu `bad.example.test`. Il couvre le parsing, les compteurs publiés, le succès, le rejet par exclusion, le rejet hors domaine, la comparaison insensible à la casse, le cas d’un point initial et l’intégration dans la politique TLS à intermédiaire.

| Vérification | Résultat |
|---|---|
| Binaire Unity X.509 | **11/11** tests réussis. |
| Suite noyau | **32/32** exécutables de test réussis. |
| Suite complète | **368/368** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU `qemu-ai-provider` | Réussi. |
| Smoke QEMU `qemu-ne2k-status` | Réussi. |

## Limites connues

Ce lot ne prend pas encore en charge `directoryName`, `rfc822Name`, URI, IP, `otherName`, IDNA/Unicode, l’intersection de contraintes de plusieurs CA, les distances `minimum` non nulles ou `maximum`, les chaînes de profondeur arbitraire, les contraintes portées par l’ancre, ECDSA/EdDSA, CRL ou OCSP. Le rejet des formes non DNS est volontairement conservatif pour éviter qu’une contrainte critique puisse être contournée silencieusement.

## Références

[1] [RFC 5280, section 4.2.1.10 — Name Constraints](https://www.rfc-editor.org/rfc/rfc5280.html#section-4.2.1.10)
