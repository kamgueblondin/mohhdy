# AOS-1145 à AOS-1160 — liaison de chaînes FAT16

Le volume FAT16 expose désormais `fat16_link_clusters` pour remplacer l’EOC d’un cluster source par le numéro d’un cluster cible déjà alloué. L’opération vérifie que les deux clusters appartiennent au volume, que la source est terminale, que la cible n’est ni libre ni marquée BAD, et refuse une auto-liaison.

La nouvelle valeur est écrite dans toutes les copies FAT. Le cluster cible doit donc être réservé préalablement par `fat16_allocate_cluster`. Aucun cluster n’est créé implicitement et aucune entrée de répertoire n’est modifiée par cette API.

Le chaînage conserve les buffers statiques du module et restaure le secteur courant si le writer échoue. Le parcours de lecture des fichiers peut ainsi suivre une chaîne multi-clusters lorsque les couches supérieures auront publié la taille et le premier cluster correspondants.

> L’allocation réserve un cluster ; la liaison séparée construit explicitement la chaîne FAT.

| Élément | Garantie |
|---|---|
| API | `fat16_link_clusters` |
| Source | Cluster existant terminé par EOC |
| Cible | Cluster existant déjà alloué |
| Réplication | Toutes les FAT |
| Allocation implicite | Interdite |
| Mémoire | Buffers statiques, aucun `kmalloc` |
| Tests | **415/415 verts** |

Auteur : **Manus AI**

