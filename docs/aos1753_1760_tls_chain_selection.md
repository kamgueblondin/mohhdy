# AOS-1753…1760 — Sélection bornée de chaîne TLS serveur

## Objet

Ce macro-lot ajoute une sélection **transactionnelle et sans allocation dynamique** de la chaîne de certificats reçue lors d’un handshake TLS. Le noyau accepte une chaîne directe, une chaîne avec un intermédiaire ou une chaîne avec deux intermédiaires. Lorsque deux intermédiaires sont présents, les deux ordres possibles sont évalués afin de tolérer une liste serveur non ordonnée.

| Profondeur retenue | Validation appliquée |
|---|---|
| 0 | feuille → ancre |
| 1 | feuille → intermédiaire → ancre |
| 2 | feuille → intermédiaire 1 → intermédiaire 2 → ancre |

## Conception

La fonction `net_tls_handshake_chain_select()` reçoit un handshake déjà analysé, une ancre de confiance et un workspace fourni par l’appelant. Elle ne modifie pas le handshake : la sélection est préparée dans une structure locale puis publiée uniquement après une validation de chaîne réussie. La fonction essaie la chaîne directe, puis la chaîne à un intermédiaire, enfin les deux permutations des deux intermédiaires.

La sélection s’appuie exclusivement sur les validateurs X.509 existants. Ils vérifient l’identité `issuer`/`subject`, les identifiants de clé d’autorité lorsque présents, les autorisations de CA, les contraintes de longueur de chemin et les signatures RSA ou ECDSA. Aucune heuristique basée uniquement sur le nom d’émetteur n’est introduite.

> La mémoire est entièrement caller-owned ou automatique. Le macro-lot n’introduit aucun appel à `kmalloc`, `malloc`, `calloc`, `realloc` ou `free`.

## Validation

Le test TLS utilise une chaîne ECDSA réelle feuille → intermédiaire → racine. Il vérifie la sélection de profondeur 1, le pointeur vers l’intermédiaire retenu, l’absence de second intermédiaire et le rejet d’un workspace insuffisant ou d’un handshake nul.

| Vérification | Résultat |
|---|---|
| Sélection ECDSA avec intermédiaire valide | Réussie |
| Workspace insuffisant | Rejeté |
| Handshake nul | Rejeté |
| `make -s test-all` | **463/463 tests réussis** |

## Limites

La liste TLS reste volontairement bornée à une feuille et deux intermédiaires. Les chaînes plus longues sont refusées par le parseur existant. La sélection ne remplace pas la vérification d’identité : `net_tls_handshake_validate_server_identity()` réapplique ensuite la politique d’hôte et de dates à la chaîne sélectionnée.

## Références

[1] [RFC 5246 — TLS 1.2 Certificate](https://www.rfc-editor.org/rfc/rfc5246#section-7.4.2)  
[2] [RFC 5280 — Certification Path Validation](https://www.rfc-editor.org/rfc/rfc5280#section-6)
