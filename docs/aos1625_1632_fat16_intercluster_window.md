# AOS-1625 à AOS-1632 — fenêtre FAT16 inter-clusters et cache de métadonnées isolé

> **Statut : livré et mesuré.** Ce macro-lot conserve une fenêtre de données FAT16 à travers les transitions de clusters et isole les consultations de la table FAT dans un cache statique distinct. Il ne crée aucune allocation dynamique et préserve les formats FAT16, GGUF, les logits et le top-k.

## Problème traité

La fenêtre FAT16 de 4 Kio a réduit fortement les transactions ATA pour les huit secteurs d’un cluster GGUF. Lors d’une transition de cluster, le lecteur devait néanmoins consulter la FAT. La lecture générique de cette métadonnée pouvait remplacer la fenêtre de poids par une fenêtre centrée sur la FAT, ce qui supprimait l’anticipation de données suivante.

| Chemin | Fenêtre précédente | Fenêtre inter-clusters |
|---|---|---|
| Fenêtre de données | 8 secteurs, 4 Kio | jusqu’à 16 secteurs, 8 Kio |
| Transition de cluster | lookup FAT partageant la fenêtre de données | cache dédié d’un secteur FAT |
| Lecture de la chaîne FAT | lecture générique pouvant évincer les poids | lecture mono-secteur dans un cache FAT isolé |
| Écriture FAT16 | invalide la fenêtre de données | invalide la fenêtre de données **et** le cache FAT |

## Architecture de lecture

La fenêtre de données est bornée à 16 secteurs, même si le buffer caller-owned est plus grand. Cette taille couvre deux clusters FAT16 contigus de 4 Kio du disque `GPT2.GGU`, sans dépasser le comportement ATA validé lors de l’initialisation du modèle.

Le lecteur `read_fat_entry()` ne passe plus par la fenêtre de données. Il conserve le dernier secteur de table FAT dans un cache statique de 512 octets, indexé par LBA. Les transitions successives au sein d’un même secteur de FAT n’engendrent donc pas de transaction ATA supplémentaire, tout en préservant la fenêtre de matrices quantifiées déjà chargée.

> **Invariant :** toute écriture FAT16 invalide les deux caches avant la transmission au writer. Les opérations de création de fichier, d’allocation, de chaînage et de rollback ne peuvent donc lire une entrée FAT obsolète.

## Test de régression

Le vecteur de fenêtre FAT16 monte un volume synthétique à clusters de huit secteurs, attache une fenêtre caller-owned puis lit neuf secteurs consécutifs sur deux clusters adjacents. Il vérifie les octets lus et l’unicité de la transaction multi-secteurs. Les tests existants couvrent en parallèle les écritures, allocations et créations de fichiers qui imposent l’invalidation du cache FAT.

| Vérification | Résultat local |
|---|---|
| Module FAT16 | 16/16 tests verts |
| Lecture de neuf secteurs sur deux clusters | une transaction groupée et contenu identique |
| Écriture, allocation et création FAT16 | régressions historiques vertes après invalidation |
| Build freestanding i386 | réussi |
| Smoke QEMU standard | réussi, incluant la persistance renforcée |
| Smoke QEMU GGUF réel | premier token et continuation sans repli |

Le vecteur inter-clusters renforce le module FAT16 existant ; la suite complète a validé **457 tests sur 457**, sans échec ni test ignoré, avant la publication de la pull request.

## Mesures QEMU TCG

Les mesures utilisent le même modèle Q3_K réel, le même disque FAT16 et le même scénario shell : `ai bonjour`, puis `ai-continue`. Elles comparent ce macro-lot à la fenêtre de 4 Kio livrée précédemment.

| Mesure | Fenêtre 4 Kio | Fenêtre inter-clusters 8 Kio | Gain |
|---|---:|---:|---:|
| Premier token Q3_K réel | 93,64 s | 48,89 s | −44,75 s (−47,78 %) |
| Continuation `ai-continue` | 36,45 s | 21,73 s | −14,72 s (−40,38 %) |

La mesure QEMU TCG ne se transpose pas directement au matériel physique. Elle montre toutefois que la conservation de la fenêtre de données et l’amortissement des lookups FAT réduisent le coût des lectures profondes du modèle dans un scénario strictement identique.

## Limites et suite

La fenêtre reste volontairement bornée à 16 secteurs : une tentative à 32 secteurs n’a pas validé l’initialisation GGUF dans le chemin ATA PIO existant et a été retirée. Le prochain axe porte sur la projection de vocabulaire Q3_K et le coût de calcul par ligne, sans modifier le top-k exact ni l’état RNG.

## Références

[1]: ../kernel/fs/fat16.c "Fenêtre de données, cache FAT et invalidation"
[2]: ../kernel/kernel.c "Fenêtre ATA caller-owned du volume GPT2.GGU"
[3]: ../tests/unit/kernel/test_fat16.c "Régression de fenêtre inter-clusters"
[4]: ../tests/scripts/ci_qemu_gguf_local_smoke.py "Smoke QEMU de génération GGUF réelle"
[5]: aos1617_1624_fat16_read_window.md "Fenêtre FAT16 de 4 Kio de référence"

L’implémentation est définie dans le lecteur [1], attachée au boot [2], couverte par les tests [3] et mesurée par le smoke réel [4] par rapport à la référence précédente [5].
