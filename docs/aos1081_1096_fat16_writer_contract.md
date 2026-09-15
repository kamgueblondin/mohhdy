# AOS-1081 à AOS-1096 — contrat writer sectoriel FAT16

Le volume FAT16 accepte désormais un callback `fat16_write_sector_fn` caller-owned via `fat16_attach_writer`. Le montage conserve un mode lecture seule par défaut : aucun writer n’est installé implicitement et les volumes existants restent compatibles.

`fat16_write_sector` vérifie le montage, la présence explicite du writer, le buffer caller-owned et la plage LBA du volume avant de déléguer une écriture d’un secteur de 512 octets. Les erreurs du backend sont converties en erreur FAT16 et aucun état global n’est modifié.

Ce sous-lot établit la primitive de stockage nécessaire aux prochaines écritures de FAT, d’entrées de répertoire et de données. Il ne prétend pas encore implémenter l’allocation de clusters, les noms longs LFN, FAT32 ou la création atomique de fichiers ; ces fonctions restent à livrer au-dessus de ce contrat borné.

> L’écriture disque est explicitement injectée par le caller ; le FAT16 historique reste lecture seule tant qu’elle n’est pas attachée.

| Élément | Garantie |
|---|---|
| Mode par défaut | Lecture seule |
| Writer | Callback explicite caller-owned |
| Granularité | Un secteur de 512 octets |
| Bornage | LBA contrôlé contre le volume monté |
| Erreur backend | Retour FAT16, pas d’état partiel |
| Allocation | Aucune |

Validation locale : **415/415 tests verts**, dont le test d’attachement, d’écriture et de rejet hors volume.

Auteur : **Manus AI**

