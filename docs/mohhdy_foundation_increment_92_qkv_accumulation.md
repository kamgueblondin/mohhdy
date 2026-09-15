# MOHHDY Foundation — Incrément 92 : accumulation QKV caller-owned

**État :** implémenté sur la branche de travail du lot 92.

## Objectif

Le lot 92 ajoute `gpt2_gguf_project_qkv_fat16`, qui enchaîne les projections ligne par ligne d’une matrice GGUF `[C,3C]` et sépare les résultats dans trois buffers caller-owned: `query`, `key` et `value`.

Le même buffer quantifié est réutilisé pour chaque ligne. La fonction ne charge donc pas la matrice QKV complète et ne crée aucun workspace caché. Les capacités des trois sorties sont vérifiées avant toute lecture.

## Séparation

Les indices de sortie `[0,C)` sont écrits dans `query`, `[C,2C)` dans `key` et `[2C,3C)` dans `value`. Chaque scalaire est produit par `gpt2_gguf_project_qkv_row_fat16`, qui conserve les contrôles de forme `[C,3C]`, d’index, de lecture FAT16 et de type quantifié.

| Segment | Indices source | Buffer destination |
| --- | ---: | --- |
| Query | `0 ... C-1` | `query[0 ... C-1]` |
| Key | `C ... 2C-1` | `key[0 ... C-1]` |
| Value | `2C ... 3C-1` | `value[0 ... C-1]` |

## Tests

Les tests vérifient que les capacités `query`, `key` et `value` sont contrôlées avant lecture, et qu’un pointeur de sortie nul est refusé. Les validations de forme QKV et de projection d’une ligne restent actives. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le prochain incrément pourra connecter ces trois buffers à un contexte d’attention et préparer le cache KV séquentiel, sans modifier la contrainte caller-owned.
