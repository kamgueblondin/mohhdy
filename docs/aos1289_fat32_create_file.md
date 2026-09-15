# AOS-1289 à AOS-1304 — création transactionnelle de fichier FAT32

> **État :** implémenté ; validation historique du lot : **419 tests verts**. La validation courante est maintenue dans `docs/todo.md`.

`fat32_create_file` orchestre les primitives FAT32 caller-owned. Il valide l’alias 8.3, calcule le nombre de clusters requis, réserve chaque cluster avec un marqueur EOC, relie les clusters successifs, remplit un buffer statique borné à 128 secteurs, écrit chaque cluster puis publie l’entrée racine seulement après succès de toutes les écritures.

En cas d’échec d’allocation, d’écriture, de chaînage ou de publication, le chemin de rollback parcourt la chaîne déjà réservée avec une garde bornée par le nombre de clusters du volume et remet les entrées FAT à zéro dans toutes les copies. L’appelant conserve la propriété du buffer source ; le buffer statique interne n’est qu’un tampon de secteurisation et ne contient pas d’état de fichier persistant.

| Propriété | Garantie |
|---|---|
| Données | buffer source caller-owned |
| Taille de cluster | jusqu’à 128 secteurs de 512 octets |
| Publication | après écriture complète |
| Rollback | libération FAT de la chaîne partielle |
| Nom | 8.3 ASCII, sans LFN |
| Allocation dynamique | aucune |
| Tests noyau | 35/35 |
| Validation au moment du lot | 419 tests verts |

> **Note historique réconciliée.** Les capacités FAT32 LFN bornées de publication, reconstruction, lecture, suppression et renommage ont depuis été livrées par [AOS-1321](aos1321_fat32_lfn.md), [AOS-1657…1664](aos1657_1664_fat32_lfn_unlink.md), [AOS-1673…1680](aos1673_1680_fat32_lfn_rename.md), [AOS-1729…1736](aos1729_1736_fat32_lfn_read.md) et [AOS-1737…1752](aos1737_1744_fat_lfn_utf8.md). Cette page conserve le contrat historique de création 8.3 de ce lot initial.

**Auteur :** Manus AI
