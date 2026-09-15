# MOHHDY Foundation — Incrément 87 : tailles de stockage quantifiées

**État :** implémenté sur la branche de travail du lot 87.

## Objectif

Le lot 86 valide les axes 2D GPT-2. Le lot 87 ajoute `gpt2_gguf_validate_tensor_size`, qui recalcule `byte_size` à partir du produit des axes et du type GGUF avant toute lecture FAT16 ou tout appel de kernel quantifié.

Les tailles de super-blocs sont celles utilisées par les kernels locaux:

| Type | Valeurs par bloc | Octets par bloc |
| --- | ---: | ---: |
| Q3_K | 256 | 110 |
| Q4_K | 256 | 144 |
| Q6_K | 256 | 210 |

Les tenseurs F32 et F16 utilisent respectivement quatre et deux octets par élément. Les produits d’axes sont contrôlés contre les dépassements 32 bits et les types inconnus sont rejetés.

## Contrat

La fonction retourne `0` uniquement si les axes sont non nuls, compatibles avec le type et si la taille calculée correspond exactement à `tensor->byte_size`. Une forme quantifiée non multiple de 256, une taille tronquée ou un produit trop grand retourne `-9`; un type non supporté retourne `-4`.

La validation ne prend pas possession du tenseur, ne lit pas le blob et n’alloue aucune mémoire. Elle est donc utilisable avant `gpt2_gguf_read_tensor_fat16` ou les primitives `gpt2_gguf_dot_quant_*_fat16`.

## Tests

Le test GGUF vérifie un Q4_K nominal, une taille de données décrémentée et une forme de 255 éléments. Les tests de rangs, axes QKV/MLP et bornes des lots précédents restent actifs. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le prochain incrément pourra intégrer cette vérification dans le contexte de forward et refuser automatiquement une matrice avant sa lecture paginée FAT16.
