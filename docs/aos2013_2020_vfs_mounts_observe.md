# AOS-2013 à AOS-2020 — Observation cohérente de `vfs-mounts`

## Objet

La pagination de `vfs-mounts` expose les huit montages du médiateur, mais une lecture répartie sur plusieurs pages peut devenir incohérente après un ajout ou un retrait d’alias. Ce macro-lot étend `vfs-list-observe` à la source virtuelle, afin d’associer l’inventaire à la génération volatile des montages.

| Élément | Contrat |
|---|---|
| Chemin virtuel | Seul le nom exact `vfs-mounts` est admis en plus des répertoires physiques terminés par `/`. |
| Génération | Une génération attendue non nulle différente de l’état courant retourne `OS_VFS_STATUS_STALE`. |
| Page | La réponse contient quatre entrées au plus, un index suivant ou `end`, et la génération observée. |
| Sécurité mémoire | Les buffers et la table VFS restent statiques ; la charge IPC n’est pas élargie. |
| Portée | La source virtuelle est seulement observable : elle n’ouvre aucun montage, droit ou chemin de mutation. |

## Validation QEMU

Le contrat VFS crée quatre alias dynamiques en plus de `initrd/`, `overlay/`, `fat16/` et `fat32/`. Il vérifie ensuite qu’une observation de `vfs-mounts` à l’index zéro retourne une première page partielle de quatre entrées, `next 4` et une génération publiée. Une seconde requête utilisant volontairement la génération historique `1` est refusée comme obsolète.

La même exécution confirme les lectures, listages et statuts FAT16/FAT32 sur deux disques IDE, ainsi que les contrôles de droits VFS existants.

## Références

[1] [ABI VFS et validateur de pagination virtuelle](../include/os_vfs_service.h)

[2] [Médiateur VFS Ring 3](../userspace/vfs_server.c)

[3] [Scénario QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
