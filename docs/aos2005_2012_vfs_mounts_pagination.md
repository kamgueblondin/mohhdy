# AOS-2005 à AOS-2012 — Pagination de la table virtuelle VFS

## Objet

La source virtuelle `vfs-mounts` est bornée par la réponse IPC de 80 octets. Avec quatre montages protégés et quatre alias dynamiques, sa représentation complète ne tient plus dans une lecture unique. Ce macro-lot expose donc la même table via `vfs-list-page vfs-mounts <depart>`, sans élargir l’ABI IPC ni introduire d’allocation dynamique.

| Invariant | Garantie |
|---|---|
| Source admise | `vfs-mounts` est le seul nom virtuel accepté par `OS_IPC_VFS_LIST_PAGE`; les chemins physiques restent des répertoires terminés par `/`. |
| Pagination | Une page transporte au plus quatre entrées et fournit `next_start` ou la sentinelle `end`. |
| Droits affichés | L’overlay est affiché `rw`; initrd, FAT16 et FAT32 sont affichés `ro`. |
| Mémoire | La table, le buffer de page et le protocole IPC sont statiques et bornés. |
| Compatibilité | `vfs-read vfs-mounts` demeure borné à 80 octets; la pagination permet l’inventaire complet. |

## Validation

Le contrat QEMU VFS construit les alias `assets/`, `work/`, `media32/` et `media/` en plus des quatre montages protégés. Il vérifie ensuite :

- la première page `vfs-list-page vfs-mounts 0`, partielle, avec quatre entrées et `next 4` ;
- la seconde page `vfs-list-page vfs-mounts 4`, complète, avec quatre entrées et `next end` ;
- la présence des deux alias FAT dans l’inventaire paginé ;
- les parcours de listage, lecture et statut des fixtures FAT16 et FAT32 réelles.

## Références

[1] [Contrat ABI de pagination VFS](../include/os_vfs_service.h)

[2] [Médiateur VFS Ring 3](../userspace/vfs_server.c)

[3] [Contrat QEMU VFS multi-disque](../tests/integration/test_qemu_vfs_service.py)
