# MOHHDY Foundation — Incrément 76 : accumulation GGUF multi-blocs

**État :** implémenté sur la branche de travail du lot 76.

## Objectif

Le lot 76 ajoute `gpt2_gguf_dot_quant_tensor_fat16`, qui accumule plusieurs super-blocs quantifiés d’un tenseur GGUF sans charger le tenseur complet. L’API conserve un scratch caller-owned, lit chaque bloc avec le contrat du lot 75 et appelle le kernel Q3_K, Q4_K ou Q6_K sur la tranche d’activations correspondante.

La longueur d’activation doit être un multiple non nul de 256. Chaque bloc est validé séparément par l’API mono-bloc, ce qui maintient les contrôles de type, de taille, d’offset et de capacité. L’accumulation FP32 est réalisée dans un scalaire fourni par l’implémentation; aucune allocation dynamique ni modification de l’ABI noyau n’est introduite.

| Contrôle | Garantie |
| --- | --- |
| dimensions | `count` strictement positif et multiple de 256 |
| mémoire | un seul scratch fourni par l’appelant |
| stockage | lecture FAT16 d’un bloc à la fois |
| calcul | dispatch vers le kernel quantifié correspondant |
| résultat | somme FP32 des produits partiels |

## Validation

La fixture disque contient désormais un tenseur Q4_K de 512 valeurs, soit deux super-blocs de 144 octets et 480 octets de fichier. Le test accumule les deux blocs avec 512 activations nulles et vérifie un résultat nul; les tests mono-bloc et de longueur invalide restent actifs. La suite `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.

## Limites et suite

Le chemin reste une primitive de ligne de poids: il ne réalise pas encore le parcours des matrices GPT-2, les biais, les dimensions de batch ou les accumulations de plusieurs lignes. Un prochain lot pourra ajouter un descripteur de ligne borné et relier cette accumulation au mapping des rôles GPT-2.
