# MOHHDY Foundation — Incrément 79 : matrice GGUF multi-lignes

**État :** implémenté sur la branche de travail du lot 79.

## Objectif

Le lot 79 transforme la fixture GGUF en matrice quantifiée Q4_K réellement multi-lignes. `output.weight` possède maintenant la forme `256 × 2`: chaque ligne contient un super-bloc de 256 valeurs et les deux lignes occupent deux blocs Q4_K consécutifs dans le fichier FAT16.

Cette fixture vérifie que la façade `gpt2_gguf_dot_quant_row_fat16` sélectionne correctement la ligne zéro et la ligne un, tandis qu’une demande de ligne deux est rejetée. L’accumulation tensorielle sur 512 activations reste également testée, ce qui couvre le parcours des deux blocs depuis le stockage disque.

| Élément | Valeur de test |
| --- | --- |
| forme GGUF | `shape[0]=256`, `shape[1]=2` |
| type quantifié | Q4_K |
| blocs par ligne | 1 |
| blocs totaux | 2 |
| stockage | FAT16, fichier court `GPT2.GGU` |
| calcul | produit scalaire FP32 borné |

## Validation

Le test charge le fichier depuis FAT16, mappe `output.weight`, vérifie la forme 2D, calcule les lignes 0 et 1 avec des activations nulles et refuse `row_index = 2`. La suite `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.

Le scénario QEMU shell extras a également été stabilisé: sa cadence de frappe par défaut passe de 0,80 s à 0,15 s et son délai de commande à 12 s. Les marqueurs fonctionnels, les retries et les assertions restent inchangés; le scénario local passe sous 90 s.

## Limites et suite

Le chemin traite maintenant une vraie forme multi-lignes mais ne réalise pas encore le parcours de toutes les matrices GPT-2, les biais, le batch ou le forward autoregressif. Les valeurs de fixture sont nulles; la validation mathématique des kernels est couverte séparément, tandis que le raccord disque vérifie ici les offsets et les bornes.
