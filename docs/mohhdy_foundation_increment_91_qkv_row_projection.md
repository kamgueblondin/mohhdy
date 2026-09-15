# MOHHDY Foundation — Incrément 91 : projection QKV par ligne

**État :** implémenté sur la branche de travail du lot 91.

## Objectif

Le lot 91 compose la lecture FAT16 d’une ligne quantifiée et le calcul caller-owned pour exposer `gpt2_gguf_project_qkv_row_fat16`. Cette primitive traite une sortie QKV à la fois à partir d’une matrice GGUF de forme `[C,3C]`, sans charger la matrice complète ni allouer de workspace noyau.

L’appelant fournit le modèle, le tenseur, la configuration `channels`, l’index de sortie, le vecteur d’entrée, le buffer de ligne et le résultat. La fonction vérifie la forme exacte, impose `output_index < 3C`, lit la ligne quantifiée via FAT16, puis délègue au calcul multi-blocs Q3_K/Q4_K/Q6_K déjà validé.

## Garanties

| Vérification | Résultat |
| --- | --- |
| forme `[C,3C]` et index `< 3C` | projection de la sortie demandée |
| axe ou index incompatible | `-9` |
| largeur non compatible | propagée par le calcul de ligne |
| buffer insuffisant | `-6` |
| type K-Quant non supporté | `-4` |

Le chemin est séquentiel et caller-owned: lecture d’une ligne, calcul, restitution d’un scalaire. Il ne conserve pas de cache implicite et ne prend possession d’aucun buffer.

## Tests

La fixture FAT16 construit une vue QKV `[256,768]` sur le tenseur quantifié, valide la sortie 0 avec une entrée nulle et rejette l’index `3C` ainsi qu’une forme `[C,2C]`. La suite `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le prochain incrément pourra accumuler les `3C` sorties, puis séparer les segments query, key et value pour préparer l’attention autoregressive.
