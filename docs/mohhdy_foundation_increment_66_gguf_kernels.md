# Incrément Foundation 66 — kernels GGUF K-quants

**État :** implémenté sur la branche `manus/mohhdy-foundation-gguf-inference`, validation locale en cours de préparation pour pull request.

## Objectif

L’incrément ajoute au noyau freestanding i386 les produits scalaires nécessaires aux trois familles quantifiées observées dans le profil GPT-2 GGUF local : **Q3_K**, **Q4_K** et **Q6_K**. Le chemin historique Q8_0 et la conversion FP16→FP32 sont conservés comme références.

## Contrat binaire

| Type | Valeurs par super-bloc | Taille | Décomposition utilisée |
|---|---:|---:|---|
| Q3_K | 256 | 110 octets | `hmask[32]`, `qs[64]`, `scales[12]`, `d` FP16 |
| Q4_K | 256 | 144 octets | `d`, `dmin`, `scales[12]`, `qs[128]` |
| Q6_K | 256 | 210 octets | `ql[128]`, `qh[64]`, `scales[16]`, `d` FP16 |

Les fonctions `gpt2_q3_k_dot_f32`, `gpt2_q4_k_dot_f32` et `gpt2_q6_k_dot_f32` refusent les pointeurs nuls, les longueurs nulles et les longueurs qui ne sont pas des multiples de 256. Elles ne font aucune allocation et n’appellent ni libc ni service réseau.

Les formules de dépaquetage suivent les implémentations de référence GGML [1] [2]. Pour Q4_K, le helper de reconstruction des scales/minima respecte le packing 6 bits du format. Le parseur structurel expose maintenant `q3_k_tensors`, `q4_k_tensors` et `q6_k_tensors` dans `gpt2_gguf_info_t`.

## Tests et validation

La suite Unity complète atteint **256 tests réussis**, dont les tests de comparaison numérique des trois kernels sur des super-blocs synthétiques. Les tests GGUF vérifient également que les trois types sont classés comme supportés au niveau kernel et ne sont plus comptés comme quantifications inconnues. `make clean && make all` reconstruit l’image i386, l’initrd et les programmes Ring 3.

## Limite honnête

Cet incrément ne transforme pas encore un fichier GGUF en modèle GPT-2 exécutable de bout en bout. `gpt2_model.c` attend encore le checkpoint FP32 `llm.c v3`, tandis que `gpt2_gguf.c` produit seulement un rapport structurel et ne conserve pas une table de tenseurs avec noms, dimensions et offsets réutilisable par `gpt2_infer.c`. La prochaine tranche doit ajouter cette représentation, le mapping GPT-2 et une sélection de modèle contrôlée avant de déclarer une génération GGUF réelle.

La latence sous QEMU TCG ne doit pas être présentée comme inférieure à une seconde : le chemin FP32 observé reste de l’ordre de plusieurs secondes pour une courte réponse. Une mesure native ou KVM est nécessaire pour conclure sur l’objectif de performance.

## Références

[1] [llama.cpp — `ggml-quants.h`](https://raw.githubusercontent.com/ggml-org/llama.cpp/master/ggml/src/ggml-quants.h), définitions des APIs de déquantification GGML.

[2] [llama.cpp — `ggml-quants.c`](https://raw.githubusercontent.com/ggml-org/llama.cpp/master/ggml/src/ggml-quants.c), implémentations de référence Q3_K, Q4_K et Q6_K.

[3] [Spécification GGUF](https://github.com/ggml-org/ggml/blob/master/docs/gguf.md), structure du conteneur et des types de tenseurs.
