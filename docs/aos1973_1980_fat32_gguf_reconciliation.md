# AOS-1973 à AOS-1980 — Réconciliation FAT32 LFN et runtime GGUF

## Objet

Ce macro-lot ne modifie pas les algorithmes du noyau. Il aligne la documentation de référence sur les fonctionnalités déjà présentes dans le code et couvertes par les tests : gestion FAT32 des noms longs Unicode, et exécution de la génération locale par le runtime GPT-2 GGUF quantifié.

| Domaine | État vérifié | Limite conservée |
|---|---|---|
| FAT32 LFN | Création, publication multi-entrée, lecture, listage, renommage et suppression par nom long sont disponibles. | FAT32 est exposé au VFS en lecture, listage et statut ; les mutations VFS restent limitées à l’overlay. |
| Encodage LFN | La conversion UTF-8/UTF-16LE valide les séquences, couvre le BMP et les paires substituts pour les points de code non-BMP. | Les noms restent bornés par `OS_NAME_MAX`, 13 unités UTF-16 par entrée et 20 entrées LFN. |
| Runtime GGUF | Le chargement prépare la génération quantifiée Q3_K/Q4_K/Q6_K, le cache KV, puis exécute la génération échantillonnée top-k avec des espaces de travail caller-owned. | Les contraintes de taille du modèle et de capacité mémoire de la plateforme i386 demeurent inchangées. |

## Preuves FAT32

Les helpers [`lfn_utf8.h`](../kernel/fs/lfn_utf8.h) convertissent les noms UTF-8 validés vers UTF-16LE et reconstituent l’UTF-8 ; ils émettent et vérifient les paires substituts. Le module [`fat32.c`](../kernel/fs/fat32.c) compose ensuite ces helpers dans `fat32_create_lfn_file`, `fat32_read_file`, `fat32_rename_lfn_file`, `fat32_unlink_file` et `fat32_list_root`. Ces chemins demeurent déterministes : les buffers appartiennent à l’appelant et aucune allocation dynamique n’est introduite.

La régression [`test_fat32.c`](../tests/unit/kernel/test_fat32.c) couvre les conversions UTF-8/UTF-16, l’encodage d’entrée, la création d’un nom long, la lecture par alias et par nom long, le listage, le renommage, la suppression, ainsi que les aller-retours UTF-8 BMP et non-BMP.

> La réconciliation initiale séparait les primitives caller-owned du backend VFS. Les lots AOS-1989…2004 ont depuis relié FAT16 et FAT32 au médiateur pour la lecture, le listage et le statut, sans ouvrir de mutation FAT via VFS.

## Preuves GGUF

Le chargeur [`gpt2_gguf_loader.c`](../kernel/llm/gpt2_gguf_loader.c) valide les tenseurs Q3_K/Q4_K/Q6_K, prépare une structure de génération et exécute un token de génération avec cache KV. Le runtime [`gpt2_gguf_infer.c`](../kernel/llm/gpt2_gguf_infer.c) expose la génération échantillonnée, tandis que les interfaces syscall raccordent ce chemin à la session IA locale. La mention historique indiquant que la génération quantifiée était non livrée est donc supprimée.

| Contrôle de non-régression | Portée |
|---|---|
| `test_fat32` | Contrats de volume, LFN UTF-8/UTF-16, cycle de vie du nom long. |
| `test_gpt2_gguf` | Validation GGUF, préparation de génération et contraintes des tenseurs quantifiés. |
| `test_gpt2_gguf_infer` | Exécution du chemin d’inférence GGUF. |
| `make -s test-all` | Validation globale de tous les modules concernés et de leurs intégrations. |

## Résultat

Les descriptions historiques de limitations FAT32 LFN et GGUF ont été réconciliées avec le code exécutable. Les limitations toujours réelles concernent désormais les mutations FAT via VFS et non plus le branchement FAT32 de lecture, listage et statut.

## Références

[1] [Conversion LFN UTF-8/UTF-16LE](../kernel/fs/lfn_utf8.h)

[2] [Primitives FAT32 LFN](../kernel/fs/fat32.c)

[3] [Tests FAT32 LFN](../tests/unit/kernel/test_fat32.c)

[4] [Préparation et génération GPT-2 GGUF](../kernel/llm/gpt2_gguf_loader.c)

[5] [Runtime d’inférence GPT-2 GGUF](../kernel/llm/gpt2_gguf_infer.c)
