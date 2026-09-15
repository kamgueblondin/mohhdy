# AOS-1617 à AOS-1624 — fenêtre de lecture FAT16 multi-secteurs pour GGUF

> **Statut : livré et mesuré.** Ce macro-lot introduit une fenêtre de lecture FAT16 de 4 Kio, fournie explicitement par le noyau, afin de diminuer les transactions ATA lors du forward GPT-2 GGUF Q3_K réel. Aucun buffer dynamique ni modification du top-k n’est introduit.

## Problème traité

Le curseur FAT16 lisait les poids quantifiés séquentiels un secteur de 512 octets à la fois. Même avec le transfert PIO `rep insw`, chaque secteur déclenchait une transaction ATA distincte. Le modèle `GPT2.GGU` est organisé avec des clusters FAT16 de 4 Kio ; huit secteurs contigus peuvent donc être lus de façon anticipée avec une seule commande ATA multi-secteurs.

| Aspect | Lecture historique | Fenêtre AOS-1617…1624 |
|---|---|---|
| Granularité de remplissage | 1 secteur, 512 octets | jusqu’à 8 secteurs, 4 096 octets |
| Stockage | cache de secteur dans le curseur | fenêtre statique caller-owned du volume, plus cache de secteur historique |
| Callback requis | `read_sector(lba, buffer)` | `read_sectors(lba, count, buffer)` optionnel |
| Repli | non applicable | lecture mono-secteur historique si la fenêtre est absente |
| Allocation dynamique | aucune | aucune |

## Contrat d’API et sûreté

`fat16_attach_read_window()` attache au volume monté trois ressources entièrement fournies par l’appelant : un callback multi-secteurs, un buffer et sa capacité. Le lecteur ne demande jamais plus de huit secteurs et limite chaque requête aux secteurs restants du volume.

La fonction interne de lecture procède de la manière suivante : elle sert d’abord un secteur présent dans la fenêtre ; sinon, elle remplit la fenêtre au LBA demandé via le callback multi-secteurs ; si aucune fenêtre n’est attachée, elle conserve exactement le callback mono-secteur précédent. Une écriture FAT16 invalide explicitement la fenêtre afin d’empêcher une lecture périmée.

> **Invariant :** l’API FAT16 historique, les pointeurs caller-owned, les contrôles de bornes LBA et les codes de corruption sont conservés. L’optimisation ne change que le nombre de transactions de lecture pour les secteurs physiques contigus.

## Intégration noyau

Au montage du disque ATA, le noyau attache une fenêtre statique de `8 × 512` octets et l’adaptateur `ata_read_sectors()`. L’absence ou l’échec d’attache n’empêche pas le boot : le pilote FAT16 poursuit avec le lecteur mono-secteur déjà validé.

Cette intégration est particulièrement favorable à `GPT2.GGU`, dont les lectures de matrices et de projection top-k utilisent un curseur séquentiel. Les transitions de clusters restent validées par la FAT ; une fenêtre peut contenir des secteurs anticipés inutilisés si une chaîne est fragmentée, mais elle ne les expose pas comme des données du fichier hors de l’adresse demandée.

## Test de non-régression

Le vecteur `test_cursor_uses_attached_multisector_window` construit un volume FAT16 valide à clusters de 4 Kio. Il positionne un fichier sur un cluster hors de la fenêtre de racine, attache un buffer de 4 Kio puis lit deux secteurs consécutifs avec un curseur. Le test vérifie :

| Propriété | Vérification |
|---|---|
| Résultat fonctionnel | les 1 024 octets lus sont identiques à l’image synthétique |
| Regroupement | le callback multi-secteurs n’est appelé qu’une seule fois |
| Chaîne FAT | le fichier reste dans un cluster de huit secteurs, comme `GPT2.GGU` |
| Regressions FAT16 | les autres tests de lecture, écriture, curseur et lecture profonde restent verts |

Le module FAT16 atteint ainsi **16/16** tests ciblés. La suite complète valide **457 tests sur 457**, sans échec ni test ignoré, grâce à ce nouveau vecteur.

## Mesures QEMU TCG

Le smoke sélectionne `gpt2.gguf`, exige un premier token réel sans repli, puis exécute `ai-continue`. La référence est le lot ATA PIO précédent, mesuré dans le même environnement QEMU TCG.

| Mesure | Référence ATA PIO | Fenêtre FAT16 4 Kio | Gain |
|---|---:|---:|---:|
| Premier token Q3_K réel | 508,94 s | 93,64 s | −415,30 s (−81,60 %) |
| Continuation `ai-continue` | 182,27 s | 36,45 s | −145,82 s (−80,00 %) |

Ces résultats confirment que le coût dominant résidait dans le nombre de transactions secteurs, non dans le calcul Q3_K seul. Ils restent des mesures d’émulation TCG, donc non transférables telles quelles à un matériel physique ; leur rôle est de comparer deux chemins de code avec le même modèle et le même scénario.

## Validation

| Vérification | Résultat |
|---|---|
| Test FAT16 ciblé | 16/16 verts, incluant la fenêtre multi-secteurs. |
| Build freestanding i386 | Réussi ; le noyau augmente d’environ 4 Kio pour la fenêtre statique. |
| Smoke QEMU standard | Réussi : cœur, shell, persistance, tâches et `exec`. |
| Smoke QEMU GGUF réel | Réussi : premier token et continuation sans repli. |
| Allocation dynamique | Aucune fonction `kmalloc`, `malloc`, `calloc` ou `realloc` ajoutée. |

La suite complète a validé **457 tests sur 457** avant la publication de la pull request.

## Suite

La fenêtre de lecture réduit fortement l’overhead FAT16/ATA. Le prochain axe est l’analyse fine de la projection de vocabulaire Q3_K et de l’échantillonnage top-k sur les 50 257 lignes, avec maintien de l’équivalence RNG et du workspace statique.

## Références

[1]: ../kernel/fs/fat16.h "Contrat de fenêtre de lecture FAT16"
[2]: ../kernel/fs/fat16.c "Remplissage et invalidation de fenêtre multi-secteurs"
[3]: ../kernel/kernel.c "Adaptateur ATA et fenêtre statique du volume de déploiement"
[4]: ../tests/unit/kernel/test_fat16.c "Vecteur de lecture cursorisée multi-secteurs"
[5]: ../tests/scripts/ci_qemu_gguf_local_smoke.py "Smoke QEMU de génération GGUF réelle"

L’extension est définie par le contrat [1], implémentée dans le lecteur [2], attachée au boot [3], testée par le vecteur [4] et mesurée par le smoke [5].
