# AOS-1729 à AOS-1736 — lecture FAT32 par LFN ASCII validé

Ce macro-lot étend `fat32_read_file` : la fonction accepte désormais un alias 8.3 ou un nom long FAT32 ASCII. La recherche reconstruit les fragments LFN dans un buffer automatique borné, exige des ordinaux continus et vérifie le checksum contre l’entrée courte associée avant toute lecture de la chaîne de données.

| Chemin | Validation |
|---|---|
| Alias 8.3 | Parseur 8.3 FAT32 historique. |
| LFN ASCII | Caractères bornés, ordinaux LFN, checksum et comparaison insensible à la casse. |
| Données | Parcours de chaîne FAT32 déjà validé, borne par la taille de fichier et `cluster_count`. |

Les séquences supprimées, rompues ou dont le checksum ne correspond pas ne permettent pas une correspondance LFN. Aucun état persistant, cache de nom ou allocation dynamique n’est introduit.

Le vecteur FAT32 crée `Persistent-LLM-Session`, puis le lit par `persistent-llm-session` en casse différente avant de vérifier listage, renommage et suppression. La suite complète valide **457/457 tests**.

## Références internes

- [Fondations LFN FAT32](aos1321_fat32_lfn.md)
- [Lecture FAT32 par alias](aos1681_1688_fat32_alias_read.md)
- [Montage VFS FAT32](aos1721_1728_vfs_fat32_readonly_mount.md)
