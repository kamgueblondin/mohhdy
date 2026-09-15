# AOS-1129 à AOS-1144 — allocation bornée de cluster FAT16

Le volume FAT16 expose désormais `fat16_allocate_cluster` pour rechercher le premier cluster libre dans la plage valide, le marquer `EOC` et répliquer l’entrée dans toutes les copies FAT déclarées par le BPB. Le cluster n’est publié dans `out_cluster` qu’après l’écriture réussie de toutes les copies.

L’API exige un volume monté et un writer explicite. La recherche utilise les entrées FAT existantes sans allocation mémoire. En cas d’erreur d’écriture, la copie sectorielle courante est restaurée autant que le backend le permet et aucun cluster n’est retourné comme alloué.

Ce lot complète la primitive d’écriture de données par la première opération de gestion de chaîne FAT. La création de répertoire, les entrées 8.3/LFN, la liaison de chaînes multi-clusters et FAT32 restent des couches supérieures à ajouter ; aucun de ces éléments n’est simulé par cette API.

> Un cluster est considéré alloué seulement lorsque son entrée EOC est écrite dans chaque copie FAT du volume.

| Élément | Garantie |
|---|---|
| API | `fat16_allocate_cluster` |
| Recherche | Premier cluster libre borné |
| Marquage | EOC FAT16 `0xFFF8` |
| Réplication | Toutes les copies FAT |
| Publication | Après succès complet uniquement |
| Allocation mémoire | Aucune |
| Tests | **415/415 verts** |

Auteur : **Manus AI**

