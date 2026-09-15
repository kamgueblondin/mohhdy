# AOS-1681 à AOS-1688 — lecture FAT32 par alias 8.3

Ce macro-lot ajoute `fat32_read_file`, une lecture FAT32 bornée par alias 8.3 dans un buffer fourni par l’appelant. Le lecteur localise l’entrée courte racine, valide la taille et les attributs, puis parcourt la chaîne FAT32 avec une garde bornée par `cluster_count`.

Le buffer de cluster statique déjà utilisé par le créateur FAT32 est réemployé : aucune allocation dynamique, aucun cache de fichier persistant et aucune copie de la chaîne complète ne sont introduits. Les erreurs de cluster invalide, EOC prématuré, chaîne trop longue, buffer insuffisant ou nom absent sont retournées avant toute lecture non bornée.

| Élément | Garantie |
|---|---|
| Nom | Alias 8.3 validé par le parseur FAT32 existant. |
| Données | Copie bornée par `max` et par la taille de l’entrée. |
| Chaîne | Lecture cluster par cluster avec garde de corruption. |
| Mémoire | Buffer caller-owned pour la sortie ; workspace statique existant pour le cluster. |

Le test FAT32 lit les données du fichier LFN créé via son alias `SESSION.BIN`, ce qui valide la compatibilité avec les fichiers qui portent une séquence LFN et une entrée courte associée. La suite complète confirme **457/457** tests réussis.

## Limites

La sélection directe par nom long LFN dans le lecteur FAT32 et son exposition VFS restent les incréments suivants. Le parcours de données est toutefois déjà commun et prêt à être réutilisé.

## Références internes

- [Fondations LFN FAT32](aos1321_fat32_lfn.md)
- [Renommage FAT32 LFN](aos1673_1680_fat32_lfn_rename.md)
- [Contrats FAT32](../kernel/fs/fat32.h)
