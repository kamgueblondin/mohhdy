# AOS-1113 à AOS-1128 — écriture bornée d’un cluster FAT16

Le backend FAT16 expose désormais `fat16_write_cluster_range` pour modifier une plage d’octets dans un cluster déjà existant. L’API vérifie le montage, la présence d’un writer explicite, la validité du numéro de cluster et le bornage `offset + length` par rapport à la taille du cluster.

L’écriture est réalisée par lecture-modification-écriture sectorielle. Les octets voisins de la plage restent préservés, tandis que le callback writer reçoit uniquement des secteurs valides dans le volume. Le buffer source est caller-owned et aucun scratch dynamique n’est créé ; le scratch sectoriel statique existant est réutilisé conformément aux contraintes freestanding.

Ce lot ne modifie pas la FAT et n’alloue pas de cluster. Il établit donc la primitive sûre nécessaire à l’écriture ultérieure d’une chaîne FAT, d’une entrée 8.3/LFN ou d’un fichier FAT32, sans annoncer prématurément ces fonctionnalités.

> Une plage de données ne peut être écrite que dans un cluster existant et explicitement borné par le volume monté.

| Élément | Garantie |
|---|---|
| API | `fat16_write_cluster_range` |
| Allocation | Aucune |
| Granularité backend | Secteur de 512 octets |
| Partiel | Lecture-modification-écriture |
| Bornage | Cluster et plage contrôlés |
| Mode | Writer explicite uniquement |
| Tests | **415/415 verts** |

Auteur : **Manus AI**

