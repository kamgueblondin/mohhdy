# AOS-1657 à AOS-1664 — suppression FAT32 par alias et LFN

## Objet

Ce macro-lot ajoute `fat32_unlink_file`, une suppression bornée de fichier FAT32 qui accepte soit l’alias 8.3, soit un **nom long LFN ASCII validé**. Elle complète la création et le listage LFN déjà disponibles, sans allocation dynamique et sans extension de l’état résident.

## Recherche et intégrité

La recherche traverse la chaîne du répertoire racine au moyen de `fat32_dir_slot`. Les fragments LFN ne sont associés à une entrée courte que si les ordinaux sont complets, dans l’ordre attendu, et si le checksum de l’alias correspond. Les séquences supprimées, invalides et les labels de volume sont ignorés.

| Nom demandé | Comportement |
|---|---|
| Alias 8.3 valide | Correspondance historique préservée. |
| LFN ASCII valide | Correspondance insensible à la casse après validation intégrale de la séquence. |
| Nom absent | `OS_FAT16_NOT_FOUND`. |
| Nom vide, séparateur ou octet hors ASCII | `OS_FAT16_BAD_PATH`. |

## Suppression

Après identification, toutes les entrées LFN de la séquence et l’entrée courte associée sont marquées supprimées (`0xE5`). La première unité de la chaîne de données est ensuite transmise au libérateur FAT32 existant, qui parcourt et remet à zéro les entrées FAT dans une boucle bornée par `cluster_count`.

> Les données ne sont pas écrasées : comme le prévoit FAT, les clusters sont rendus disponibles à la réallocation. L’API reste freestanding et ne dépend d’aucun `malloc`, `kmalloc`, cache de noms ni buffer persistant supplémentaire.

## Validation

Le vecteur FAT32 existant crée `Persistent-LLM-Session` sur l’alias `SESSION.BIN`, puis vérifie la suppression via le nom long en casse différente. Il contrôle le marquage des quatre entrées de répertoire concernées, la libération de l’entrée FAT du premier cluster, l’absence au listage et le refus d’une seconde suppression.

## Limites

Le renommage LFN, l’UTF-8 au-delà de l’ASCII, les paires substituts UTF-16 et le raccordement VFS FAT32 restent distincts. Cette séparation évite de mêler une opération destructive de chaîne avec la publication multi-entrée d’un nouveau nom.

## Références internes

- [Fondations LFN FAT32](aos1321_fat32_lfn.md)
- [Contrats FAT32](../kernel/fs/fat32.h)
- [Recherche FAT16 par LFN](aos1649_1656_fat16_lfn_read_lookup.md)
