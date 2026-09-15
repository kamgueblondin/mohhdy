# AOS-1633 à AOS-1640 — décodage Q3_K sans branche

> **Statut : livré et mesuré.** Le kernel Q3_K reconstruit les mêmes codes quantifiés signés sans branche conditionnelle par valeur. Les formats GGUF, l’ordre d’accumulation, les logits, le top-k et l’état RNG restent inchangés ; aucune allocation dynamique n’est introduite.

## Optimisation

Le produit scalaire Q3_K parcourt 256 valeurs par super-bloc. Pour chaque paire, le chemin précédent testait séparément le bit de masque afin de soustraire quatre lorsque le code devait être négatif. Cette forme produit deux branches conditionnelles par paire de valeurs.

La nouvelle forme conserve le même code bas sur deux bits et applique arithmétiquement la correction de signe : la condition produit zéro ou un, puis multiplie implicitement la correction fixe de quatre. Les valeurs reconstruites demeurent strictement dans l’intervalle `−4…3`.

| Propriété | Avant | Après |
|---|---|---|
| Extraction du code | 2 bits | identique |
| Correction de signe | deux branches conditionnelles | deux soustractions conditionnelles arithmétiques |
| Échelles Q3_K | identiques | identiques |
| Ordre des multiplications et additions | identique | identique |
| Allocation dynamique | absente | absente |

## Équivalence et bornes

Le vecteur Q3_K existant est réexécuté avec le nouveau chemin, au même titre que les tests FAT16/GGUF qui comparent la projection top-k en flux au comportement de référence. Le changement est limité à la reconstruction de `q0` et `q1` dans `gpt2_q3_k_dot_f32()` ; il ne modifie ni les structures GGUF ni les buffers de travail.

| Vérification | Résultat local |
|---|---|
| Module quantification | 8/8 tests verts |
| Format et décodage Q3_K | mêmes résultats de référence |
| Build freestanding i386 | réussi |
| Smoke QEMU GGUF réel | premier token et `ai-continue` réussis |

La suite complète est relancée avant la publication de la pull request.

## Mesures QEMU TCG

La comparaison utilise le modèle GPT-2 Q3_K réel, le disque FAT16 de déploiement, la fenêtre inter-clusters de 8 Kio déjà validée et le même scénario shell : `ai bonjour`, puis `ai-continue`.

| Mesure | Référence inter-clusters | Décodage sans branche | Gain |
|---|---:|---:|---:|
| Premier token Q3_K réel | 48,89 s | 45,43 s | −3,46 s (−7,07 %) |
| Continuation `ai-continue` | 21,73 s | 20,83 s | −0,90 s (−4,14 %) |

Ces chronométrages sont propres à QEMU TCG et ne constituent pas une prédiction de performance sur matériel physique. Une répétition ultérieure de la référence a produit 49,04 s puis 22,48 s, ce qui confirme une variabilité de plusieurs secondes dans l’émulation. Le changement est conservé pour son équivalence fonctionnelle et l’élimination déterministe de branches, mais la différence de latence QEMU doit être lue comme une observation de comparaison, non comme une garantie reproductible.

## Limites et suite

Cette optimisation ne modifie pas le nombre de lignes de vocabulaire projetées : tous les logits sont toujours calculés afin de préserver le top-k exact. Les optimisations qui ont augmenté le buffer de lignes ou vectorisé ce kernel sans gain mesurable ont été volontairement écartées. Le prochain axe reste l’amortissement des lectures et calculs de projection sans approximation ni allocation dynamique.

## Références

[1]: ../kernel/llm/gpt2_quant.c "Produit scalaire Q3_K sans branche"
[2]: ../tests/unit/kernel/test_gpt2_quant.c "Vecteurs de référence des kernels quantifiés"
[3]: ../tests/scripts/ci_qemu_gguf_local_smoke.py "Smoke QEMU du runtime GGUF réel"
[4]: aos1625_1632_fat16_intercluster_window.md "Référence de fenêtre FAT16 inter-clusters"

La modification est localisée dans le kernel [1], validée par les vecteurs de quantification [2] et mesurée par le smoke réel [3] par rapport à la référence précédente [4].
