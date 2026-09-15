# AOS-217 — Interface RSA caller-owned : rapport d’état

Cette PR introduit uniquement le contrat public `rsa_pkcs1_v15_sha256_verify`. Il fixe dès maintenant les entrées de la future vérification RSA PKCS#1 v1.5 SHA-256 et impose un workspace fourni par l’appelant. Aucune allocation dynamique n’est autorisée.

| Composant | État |
|---|---|
| Interface C caller-owned | Intégrée |
| Contrôle des arguments | Non implémenté |
| Arithmétique bigint | Non implémentée |
| Exponentiation modulaire RSA | Non implémentée |
| Décodage PKCS#1 v1.5 | Non implémenté |
| Vérification SHA-256 | Non implémentée |
| Tests RSA / vecteurs | Non implémentés |
| Intégration ServerKeyExchange | Non implémentée |

La fonction ne doit pas être appelée par le handshake avant l’ajout de son implémentation et de vecteurs de référence. La suite de tests actuelle valide la base déjà fusionnée, mais ne fournit aucune assurance sur RSA.

> Cette PR est un point de cadrage API et non une livraison de cryptographie RSA. Elle ne rend pas les certificats ni `ServerKeyExchange` authentifiés.

## Validation de la base

Le build i386 et la suite Unity existante sont verts : **332/332 tests**, zéro échec et zéro test ignoré. Ces résultats ne constituent pas des tests RSA.
