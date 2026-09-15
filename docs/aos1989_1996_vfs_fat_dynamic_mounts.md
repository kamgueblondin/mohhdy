# AOS-1989 à AOS-1996 — Montages FAT dynamiques dans le VFS

## Objet

Ce macro-lot rend les sources **FAT16** et **FAT32** disponibles dans `vfs-mount-add`, alors que le protocole IPC et le médiateur Ring 3 les connaissaient déjà. Un alias dynamique peut désormais être créé avec `vfs-mount-add <prefixe/> fat16` ou `vfs-mount-add <prefixe/> fat32`.

| Source | Accès VFS autorisé | Mutation |
|---|---|---|
| `initrd` | lecture, listage, statut | refusée |
| `overlay` | lecture, listage, statut | autorisée |
| `fat16` | lecture, listage, statut | refusée |
| `fat32` | lecture, listage, statut | refusée |

> Les mutations restent strictement limitées aux montages `overlay`. L’ouverture des alias FAT ne crée aucun chemin d’écriture sur les volumes FAT.

## Contrat de mise en œuvre

Le serveur VFS possède quatre montages protégés au démarrage : `initrd/`, `overlay/`, `fat16/` et `fat32/`. Sa table statique contient désormais huit entrées, ce qui conserve **quatre alias dynamiques** après ces montages de base et permet de tester FAT16 et FAT32 simultanément. Aucune allocation dynamique n’est introduite.

Les opérations de lecture, listage et statut routent le suffixe du montage vers les syscalls FAT existants. La recherche de métadonnées FAT compare les noms 8.3 sans casse ASCII, de manière cohérente avec la lecture depuis le shell où les frappes HMP sont normalisées en minuscules.

| Invariant | Garantie |
|---|---|
| Bornes | Les chemins et entrées utilisent les limites ABI VFS existantes. |
| Isolation | Le backend reçoit uniquement le chemin relatif au montage déclaré. |
| Lecture seule | `write`, `mkdir`, `rmdir`, `remove` et `rename` restent limités à `overlay`. |
| Mémoire | Table de montages et buffers restent statiques ; aucune allocation dynamique. |

## Validation QEMU

Le scénario `test_qemu_vfs_service.py` initialise désormais son disque avec `make_fat16_image.py`, qui conserve l’overlay en tête de disque et place une fixture FAT16 à LBA 64. Il crée `media/` par `vfs-mount-add media/ fat16`, puis vérifie le listage des deux entrées, la lecture de `FATOK.TXT` et son statut de fichier de 17 octets.

Le harnais HMP est accéléré et vérifie l’écho `SYS_GETS: ligne lue:` avant toute assertion métier. Les diagnostics timer asynchrones sont retirés uniquement de la comparaison textuelle. Les reprises sont bornées et les cessions explicites après les créations de tâches rendent l’ordonnancement coopératif déterministe.

## Références

[1] [Commande de montage VFS du shell](../userspace/shell.c)

[2] [Médiateur VFS Ring 3](../userspace/vfs_server.c)

[3] [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)

[4] [Fixture FAT16 déterministe](../tests/scripts/make_fat16_image.py)
