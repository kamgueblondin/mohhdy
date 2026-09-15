# MOHHDY Foundation — Incrément 75 : pont GGUF vers kernels quantifiés

**État :** implémenté sur la branche de travail du lot 75.

## Objectif

Le lot 75 ajoute `gpt2_gguf_dot_quant_block_fat16`. Cette API lit un super-bloc Q3_K, Q4_K ou Q6_K depuis FAT16 dans un scratch fourni par l’appelant, vérifie qu’il contient exactement 256 valeurs et appelle le kernel quantifié correspondant avec le vecteur d’activation caller-owned.

Le pont ne crée aucune allocation et ne modifie pas les kernels mathématiques déjà validés. Il impose une capacité de scratch au moins égale à la taille du format, contrôle le type GGUF et refuse les appels dont la longueur n’est pas exactement `GPT2_QK_K`. Le résultat est écrit dans un `float` fourni par l’appelant.

| Type | Taille scratch minimale | Kernel appelé |
| --- | ---: | --- |
| Q3_K | 110 octets | `gpt2_q3_k_dot_f32` |
| Q4_K | 144 octets | `gpt2_q4_k_dot_f32` |
| Q6_K | 210 octets | `gpt2_q6_k_dot_f32` |

## Validation

La fixture `GPT2.GGU` est chargée puis indexée comme tenseur Q4_K. Le test lit son super-bloc de 144 octets, exécute le kernel avec 256 activations nulles et vérifie un produit nul; une longueur de 32 valeurs est rejetée. Le runner global et la cible Makefile lient explicitement `gpt2_quant.c` au test FAT16 afin d’exercer le chemin complet. La suite `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.

## Limites et suite

Le pont traite un seul super-bloc à la fois et ne réalise pas encore une ligne complète de matrice, une accumulation multi-blocs ou un forward GPT-2. La prochaine étape pourra ajouter un accumulateur borné sur plusieurs blocs et connecter les métadonnées de forme GGUF aux dimensions d’activation, sans charger tout le checkpoint en mémoire.
