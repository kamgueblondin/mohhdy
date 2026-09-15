# MOHHDY Foundation — Incrément 78 : sélection de lignes GGUF 2D

**État :** implémenté sur la branche de travail du lot 78.

## Objectif

Le lot 78 vérifie la représentation 2D des tenseurs GGUF utilisés par les poids GPT-2. La façade `gpt2_gguf_dot_quant_row_fat16` interprète `shape[0]` comme la largeur de ligne et `shape[1]` comme le nombre de lignes. Elle calcule le premier super-bloc de la ligne en 64-bit, refuse les lignes inexistantes et réutilise le dispatch quantifié sans charger la matrice entière.

La fixture `output.weight` encode désormais deux dimensions (`512 × 1`) tout en conservant deux super-blocs Q4_K dans le fichier FAT16. Cette forme minimale exerce le contrat 2D sans augmenter artificiellement la chaîne de clusters; une prochaine fixture pourra ajouter plusieurs lignes réelles lorsque le générateur disque sera étendu.

| Contrôle | Résultat attendu |
| --- | --- |
| dimensions 2D | `shape[0]` devient la largeur d’activation |
| nombre de lignes | `row_index < shape[1]` |
| offset | multiplication bornée avant conversion i386 |
| calcul | accumulation Q4_K par super-bloc |
| stockage | lecture progressive FAT16 |

## Validation

Le test charge `GPT2.GGU`, mappe `output.weight`, vérifie `dimensions == 2`, calcule la ligne zéro sur 512 activations nulles et rejette `row_index = 1` puisque la forme ne contient qu’une ligne. La suite `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.

## Limites et suite

Le chemin valide déjà les formes jusqu’à deux dimensions, mais le test disque ne contient encore qu’une ligne. Cette étape ne réalise pas le forward GPT-2, la sélection de batch ou les biais; elle fournit le contrat de forme nécessaire à un futur mapping des rôles GPT-2 vers des lignes réellement multi-rangées.
