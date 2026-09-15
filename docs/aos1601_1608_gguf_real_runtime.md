# AOS-1601 à AOS-1608 — exécution GGUF réelle, lecture FAT16 profonde et smoke de continuation

> **Statut : livré et mesuré.** Ce macro-lot corrige les deux bornes qui empêchaient le forward GPT-2 GGUF Q3_K réel d’atteindre sa projection de vocabulaire, puis transforme le smoke QEMU en validation effective d’un premier jeton local et d’une continuation `ai-continue`.

## Constat initial

Les lots précédents validaient les kernels quantifiés et le chemin shell avec des fixtures courtes. Sur le modèle Q3_K de déploiement, le premier forward pouvait cependant s’arrêter avant le token avec un code de capacité, puis le shell basculait vers le programme de compatibilité. Le marqueur textuel historique du smoke reconnaissait également ce repli comme une réponse locale, ce qui ne démontrait pas l’inférence réelle.

| Sujet | Cause identifiée | Correction |
|---|---|---|
| Projection MLP `ffn_down` | Le buffer de ligne était limité à trois blocs Q6_K, soit 630 octets. Une ligne `ffn_down` de largeur `4 × 768` nécessite jusqu’à 2 520 octets au format Q6_K. | Le buffer statique est dimensionné à partir de la largeur cachée maximale `4C` et du pire bloc Q6_K. |
| Lecture profonde FAT16 | Les gardes des lecteurs augmentaient pour chaque secteur après avoir compté les clusters sautés. Une lecture légitime à la fin d’un grand fichier pouvait ainsi devenir `OS_FAT16_CORRUPT`. | La garde progresse uniquement lors du franchissement d’un cluster, pour `fat16_read_file_range` et `fat16_file_read`. |
| Smoke local | Le smoke acceptait le préfixe commun au diagnostic « indisponible ». | Le scénario rejette explicitement le repli, lance `ai-continue` et mesure séparément les deux tours. |

> **Invariant de sûreté :** la garde reste bornée par le nombre de clusters ; seule son unité de comptage est corrigée. Une chaîne cyclique ou une valeur FAT invalide est toujours rejetée.

## Buffer de ligne du profil GPT-2

Le runtime statique utilise `GPT2_GGUF_INFER_MAX_CHANNELS = 768` et une largeur MLP maximale de `4C = 3 072`. Les lignes quantifiées sont lues dans un buffer caller-owned interne avant déquantification et produit scalaire. Le dimensionnement précédent couvrait une projection à 768 canaux, mais pas la matrice descendante MLP dont l’entrée est de 3 072 canaux.

| Quantification | Octets par bloc de 256 valeurs | Blocs pour 3 072 valeurs | Buffer minimal |
|---|---:|---:|---:|
| Q3_K | 110 | 12 | 1 320 octets |
| Q4_K | 144 | 12 | 1 728 octets |
| Q6_K | 210 | 12 | 2 520 octets |

La nouvelle borne choisit Q6_K, le format le plus large supporté. Le stockage reste un tableau statique ; aucune allocation dynamique n’est ajoutée.

## Garde de chaîne FAT16

Un fichier FAT16 est suivi par clusters, mais une lecture peut consommer plusieurs secteurs à l’intérieur du même cluster. La garde doit donc mesurer les **transitions de clusters**, et non les itérations de secteurs. Cette distinction est essentielle pour le fichier `GPT2.GGU`, qui est volumineux et utilise des clusters de 4 Kio.

Le vecteur `test_reads_deep_multisector_cluster_without_false_corruption` construit un volume FAT16 valide de plus de 4 085 clusters, lit 3 000 octets depuis une position profonde d’un fichier à clusters de 4 Kio, puis répète la lecture avec un curseur. Il vérifie les deux implémentations et verrouille la régression qui faisait échouer les lectures profondes après quelques secteurs.

## Smoke QEMU de génération réelle

Le script `ci_qemu_gguf_local_smoke.py` suit maintenant ce parcours : démarrage avec `GPT2.GGU`, sélection de `gpt2.gguf`, `ai bonjour`, puis `ai-continue`. Il rejette un diagnostic `indisponible` ou `session indisponible`, et publie deux mesures distinctes. La limite par défaut de génération est de 600 secondes pour tenir compte de QEMU TCG.

| Mesure observée sous QEMU TCG | Durée |
|---|---:|
| Premier token après `ai bonjour` | 529,67 s |
| Continuation après `ai-continue` | 175,05 s |

Ces durées valident le chemin réel complet, y compris le cache KV et la session persistante, mais ne constituent **ni une promesse de performance matérielle, ni un objectif atteint**. Elles montrent que le forward quantifié, notamment les 50 257 projections de sortie et l’émulation i386 TCG, reste le goulot dominant. Le sous-objectif historique inférieur à une seconde reste non satisfait.

## Validation

| Vérification | Résultat |
|---|---|
| Test FAT16 ciblé | 15/15 verts, y compris la lecture profonde par plage et curseur. |
| Build freestanding i386 | Réussi après le redimensionnement du buffer. |
| Smoke GGUF avec délai étendu | Premier token réel et continuation réels, sans repli shell. |
| Allocation dynamique | Aucune allocation introduite ; les modifications n’ajoutent ni `kmalloc`, ni `malloc`, ni `calloc`, ni `realloc`. |

La suite complète a validé **456 tests sur 456**, grâce au nouveau vecteur FAT16 et sans échec ni test ignoré.

## Limites et suite

La livraison priorise la correction fonctionnelle et l’honnêteté de mesure. Le prochain axe est l’accélération ciblée de la projection de sortie Q3_K et des kernels de produit scalaire sous i386, idéalement avec une stratégie qui évite de relire ou de déquantifier 50 257 lignes pour chaque tour. Toute optimisation devra conserver l’échantillonnage top-k complet, l’équivalence RNG, l’absence d’allocation dynamique et une mesure QEMU explicite.

## Références

[1]: ../kernel/llm/gpt2_gguf_infer.c "Borne statique de ligne du runtime GPT-2 GGUF"
[2]: ../kernel/fs/fat16.c "Lecteurs FAT16 et garde de progression de chaîne"
[3]: ../tests/unit/kernel/test_fat16.c "Vecteur FAT16 profond multi-secteurs"
[4]: ../tests/scripts/ci_qemu_gguf_local_smoke.py "Smoke QEMU de génération et continuation GGUF"

Les corrections sont implémentées dans le runtime [1] et le lecteur FAT16 [2], puis couvertes par le vecteur de régression [3] et le smoke réel [4].
