# AOS-1257 à AOS-1272 — écriture et chaînage FAT32

> **État :** fondation implémentée ; validation historique du lot : **419 tests verts**. L’état courant est maintenu dans `docs/ETAT_REEL.md`.

Ce macro-lot étend le volume FAT32 avec un writer explicitement attaché par l’appelant. `fat32_write_fat_entry` réalise une lecture-modification-écriture de l’entrée quatre octets, conserve les quatre bits réservés de poids fort et réplique la nouvelle valeur 28 bits dans chaque copie FAT. `fat32_allocate_cluster` recherche une entrée libre et la marque EOC ; `fat32_link_clusters` remplace uniquement l’EOC d’un cluster source par un cluster cible déjà réservé.

| Opération | Garantie |
|---|---|
| Attachement writer | explicite, aucun writer implicite au montage |
| Entrée FAT | 4 octets, masque 28 bits, bits réservés préservés |
| Réplication | toutes les FAT déclarées dans le BPB |
| Allocation | première entrée libre, marquée `0x0ffffff8` |
| Chaînage | source EOC et cible déjà allouée uniquement |
| Allocation dynamique | aucune |
| Tests noyau | 35/35 |
| Validation au moment du lot | 419 tests verts |

Les entrées de répertoire FAT32, l’écriture de données par cluster et le rollback multi-clusters ont ensuite été livrés dans des lots distincts. Les primitives LFN bornées sont également disponibles ; la publication multi-entrée et la reconstruction restent à réaliser. Cette séparation permet de réutiliser le contrat writer sans publier une entrée de fichier avant la persistance complète de sa chaîne.

**Auteur :** Manus AI
