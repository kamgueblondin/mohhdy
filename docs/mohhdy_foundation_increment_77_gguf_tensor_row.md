# MOHHDY Foundation — Incrément 77 : ligne de tenseur GGUF bornée

**État :** implémenté sur la branche de travail du lot 77.

## Objectif

Le lot 77 ajoute `gpt2_gguf_dot_quant_row_fat16`, une façade orientée ligne qui relie la forme GGUF à l’accumulation multi-blocs. Pour un tenseur 1D, la seule ligne est `row_index = 0`. Pour un tenseur 2D, `shape[0]` est la largeur d’une ligne et `shape[1]` le nombre de lignes. L’appel exige que `count` corresponde exactement à `shape[0]` et soit un multiple de 256.

L’offset du premier super-bloc est calculé en 64-bit puis borné avant d’être converti en offset i386. Chaque bloc de la ligne est ensuite lu depuis FAT16 et accumulé avec le scratch caller-owned. Les dimensions supérieures à 2, les lignes absentes, les largeurs nulles ou non supportées et les tailles incompatibles sont refusées avant le calcul.

| Forme | Interprétation |
| --- | --- |
| 1 dimension | une ligne de largeur `shape[0]` |
| 2 dimensions | `shape[1]` lignes, chacune de largeur `shape[0]` |
| plus de 2 dimensions | refusé par le contrat Foundation |

## Validation

La fixture Q4_K contient deux super-blocs et représente une ligne 1D de 512 valeurs. Le test calcule la ligne zéro avec 512 activations nulles et rejette `row_index = 1`. Les tests d’accumulation multi-blocs, de super-bloc unique et de longueur invalide restent actifs. La suite `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.

## Limites et suite

La façade ne traite encore qu’une ligne à la fois et ne relie pas les dimensions à un batch ou à une matrice GPT-2 complète. La prochaine étape pourra tester une vraie forme 2D et préparer la sélection de lignes pour les rôles GPT-2, tout en conservant le chargement progressif depuis FAT16.
