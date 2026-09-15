# AOS-1673 à AOS-1680 — renommage FAT32 LFN sans déplacement de données

Ce lot ajoute `fat32_rename_lfn_file`. L’opération valide l’alias ou le LFN source par ordinaux et checksum, conserve strictement la chaîne de données existante, puis publie les nouveaux fragments LFN et l’alias court associé. Aucun cluster n’est alloué, déplacé ou libéré.

Le contrat est volontairement borné : le nouveau nom long doit utiliser le même nombre d’entrées LFN que l’ancien. Cette règle évite tout déplacement de répertoire et garantit que la publication s’effectue dans les slots existants. Les noms restent ASCII, de longueur bornée et sans séparateur.

| Garantie | Comportement |
|---|---|
| Chaîne de données | Préservée intégralement. |
| Séquence LFN | Validée par ordinaux et checksum avant modification. |
| Publication | Nouveaux fragments LFN, puis alias court avec nouveau checksum. |
| Mémoire | Aucun `malloc`, `kmalloc` ou état persistant supplémentaire. |
| Périmètre | Renommage de même cardinalité LFN ; les transformations de cardinalité restent refusées. |

Le scénario Unity renomme `Persistent-LLM-Session` en `Persistent-LLM-Record`, vérifie le listage sous le nouveau nom, préserve la taille, puis supprime le fichier renommé. La suite complète valide **457/457** tests.

## Références internes

- [Fondations LFN FAT32](aos1321_fat32_lfn.md)
- [Suppression FAT32 par LFN](aos1657_1664_fat32_lfn_unlink.md)
- [Contrats FAT32](../kernel/fs/fat32.h)
