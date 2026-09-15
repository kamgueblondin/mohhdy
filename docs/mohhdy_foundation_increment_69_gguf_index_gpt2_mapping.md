# MOHHDY Foundation — Incrément 69 : index GGUF et mapping GPT-2

**État :** implémenté sur la branche de travail du lot 69.

## Objectif

Cet incrément transforme la vue ponctuelle de tenseurs GGUF en index réutilisable. `gpt2_gguf_build_index` valide d’abord l’enveloppe GGUF GPT-2 v3 existante, puis construit dans une structure fournie par l’appelant la table bornée de tous les descripteurs de tenseurs. Les noms restent des vues dans le blob source : aucune allocation dynamique, aucune copie de modèle et aucun pointeur de données non contrôlé ne sont introduits dans le noyau i386.

La nouvelle recherche `gpt2_gguf_index_find` permet de retrouver un tenseur sans rescanner les métadonnées et les descripteurs du conteneur. Chaque entrée conserve la forme, le type, l’offset relatif à la zone de données et la taille calculée selon F32, F16, Q8_0, Q3_K, Q4_K ou Q6_K. Les contrôles de dimension, d’alignement, de taille de bloc et de plage sont identiques à ceux de la primitive de lecture existante.

## Mapping sémantique GPT-2

Le format GGUF emploie des noms stables pour les tenseurs structurants du modèle GPT-2. `gpt2_gguf_map_role` les expose sous forme d’un enum noyau afin que le futur runtime ne dépende pas de chaînes dispersées dans son chemin d’inférence.

| Rôle | Nom GGUF exact |
| --- | --- |
| Embedding des tokens | `token_embd.weight` |
| Embedding des positions | `position_embd.weight` |
| Poids de normalisation de sortie | `output_norm.weight` |
| Biais de normalisation de sortie | `output_norm.bias` |
| Projection de sortie | `output.weight` |

Le mapping ne prétend pas encore exécuter le forward GPT-2 complet. Il fournit la base bornée nécessaire pour sélectionner les tenseurs avant l’ajout progressif des blocs d’attention, des projections QKV et des couches feed-forward.

## Validation

La suite `make test-all` atteint **262 tests réussis**, sans échec ni test ignoré. La fixture ajoute cinq descripteurs Q4_K et vérifie la construction de l’index, la recherche de `output.weight`, les cinq rôles GPT-2 et le rejet d’un rôle invalide. Les tests précédents de parse GGUF, de kernels Q3_K/Q4_K/Q6_K, de robustesse et de FAT16 restent verts.

| Contrôle | Résultat |
| --- | --- |
| Compilation des tests i386 (`-m32`) | Réussie |
| Tests unitaires kernel | 13/13 suites vertes |
| Tests userspace | 2/2 suites vertes |
| Test de robustesse GGUF | Vert |
| Total Unity | 262 réussis, 0 échec |

## Limites et suite

La structure est volontairement bornée à `GPT2_GGUF_MAX_TENSORS` entrées et doit être fournie par l’appelant. Le lot ne charge pas encore un fichier depuis FAT16, ne mappe pas les 149 tenseurs d’un checkpoint GPT-2 réel et ne réalise pas de multiplication quantifiée dans un forward complet. Ces étapes sont réservées au prochain groupe cohérent, après validation CI et smoke QEMU de cette tranche.

Le blob GGUF reste une donnée non fiable. Toute utilisation de l’index doit conserver le blob vivant et ne dériver une adresse de données qu’après les validations de plage déjà effectuées. Cette discipline est particulièrement importante pour un noyau i386 sans protection mémoire complète entre toutes les étapes du runtime LLM.
