# MOHHDY Foundation — Incrément 80 : preuve de sélection des blocs disque

**État :** implémenté sur la branche de travail du lot 80.

## Objectif

Le lot 79 démontrait qu’une matrice GGUF Q4_K pouvait contenir deux lignes et que les bornes de `row_index` étaient respectées. Toutefois, une fixture entièrement nulle ne permettait pas de distinguer une lecture correcte du bloc zéro d’une lecture accidentellement répétée du même emplacement physique. Le lot 80 rend cette preuve observable en plaçant des marqueurs distincts au début des deux super-blocs Q4_K.

La fixture calcule le début des données depuis la fin réelle des métadonnées GGUF, puis applique `general.alignment = 32`. Elle écrit `0x11` au premier octet du bloc zéro et `0x77` au premier octet du bloc un, séparé par exactement `GPT2_Q4_K_BLOCK_BYTES` octets. Aucun buffer dynamique n’est introduit dans le noyau ou dans le test d’intégration.

## Contrat vérifié

| Élément | Valeur vérifiée |
| --- | --- |
| fichier | `GPT2.GGU` via nom FAT16 8.3 |
| type | Q4_K |
| forme | `shape[0]=256`, `shape[1]=2` |
| taille d’un super-bloc | `GPT2_Q4_K_BLOCK_BYTES` = 144 octets |
| marqueur bloc 0 | `0x11` |
| marqueur bloc 1 | `0x77` |
| sélection | bloc 0 puis bloc 1 |
| invalidité | capacité d’un octet refusée |

## Preuve d’intégration

`gpt2_gguf_read_tensor_fat16` lit le premier octet de la matrice et retourne `0x11`. `gpt2_gguf_read_quant_block_fat16` lit ensuite le bloc zéro et retrouve `0x11`, puis lit explicitement le bloc un et retrouve `0x77`. Le second marqueur étant placé à la distance exacte d’un super-bloc Q4_K, cette assertion prouve que l’indexation de bloc et la sélection de l’offset physique ne réutilisent pas silencieusement le premier bloc.

La vérification de capacité reste active : une demande de lecture avec un buffer d’un seul octet est rejetée. Les tests de ligne 2D et de rejet de `row_index = 2` du lot 79 sont conservés. Les valeurs restantes de la fixture demeurent nulles; ce lot prouve la sélection des données sur FAT16, mais ne prétend pas valider le forward GPT-2 complet.

## Validation

La suite `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**. La compilation des exécutables Unity et les tests FAT16, GGUF, quantification et bornes restent verts. Les avertissements historiques du framework 32-bit ne changent pas le résultat fonctionnel.

## Limites et suite

Le chemin couvre désormais l’indexation, le mapping de rôle, la lecture par plage, le curseur, l’accumulation de super-blocs et la sélection de lignes GGUF depuis FAT16. Il ne réalise toujours pas le parcours complet des matrices GPT-2, l’application des biais, le batch, le cache KV intégré au forward ni la génération autoregressive complète. La prochaine étape doit rester caller-owned et conserver la validation Unity/QEMU avant toute intégration plus large.
