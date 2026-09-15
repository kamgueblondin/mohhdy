# MOHHDY Foundation — Incrément 85 : validation des formes de couche GGUF

**État :** implémenté sur la branche de travail du lot 85.

## Objectif

Le lot 84 sécurisait l’accès aux rôles d’une couche. Le lot 85 ajoute `gpt2_gguf_validate_layer`, qui vérifie les invariants nécessaires avant d’envoyer une couche vers les kernels quantifiés: les dix rôles doivent être présents, `channels` doit être non nul et multiple de `GPT2_QK_K`, chaque tenseur doit avoir un rang 1 ou 2, une première dimension non nulle et une taille de données non nulle.

Pour les tenseurs Q3_K, Q4_K et Q6_K, la première dimension doit également être alignée sur `GPT2_QK_K`. La fonction reste purement structurelle et caller-owned; elle ne lit pas de données de poids, ne charge pas le checkpoint et n’alloue aucune mémoire.

## Contrat d’erreur

| Condition | Retour |
| --- | ---: |
| couche ou pointeur invalide | `-1` |
| masque incomplet | `-8` |
| canaux non alignés, rang ou forme invalides | `-9` |
| couche complète et structure valide | `0` |

Cette validation ne prétend pas encore prouver l’ordre sémantique exact des axes pour QKV, attention et MLP. Cette étape nécessite une fixture GGUF à formes 2D complètes et sera traitée avant le branchement du forward.

## Tests et non-régression

Le test GGUF accepte une couche Q4_K de 256 éléments, rejette `channels = 255`, rejette une forme quantifiée de 255 éléments et rejette un masque absent. La suite `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**, avec les tests de bornes GGUF toujours verts.

## Suite

Le prochain lot pourra ajouter une fixture de matrices 2D GPT-2 et vérifier les relations `channels`, `3 × channels` et `4 × channels` avant de connecter la lecture FAT16 paginée au calcul quantifié.
