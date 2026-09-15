# AOS-1721 à AOS-1728 — montage VFS FAT32 en lecture seule

Ce macro-lot ajoute `OS_VFS_MOUNT_SOURCE_FAT32` et le montage protégé `fat32/` au serveur VFS Ring 3. La source utilise exclusivement les syscalls FAT32 lecture et listage du volume secondaire réel.

| Opération VFS | Source `fat32/` |
|---|---|
| Lecture | Autorisée par `SYS_FAT32_READ`. |
| Stat | Autorisé par recherche dans le listage de racine. |
| Listage et pagination | Autorisés, racine FAT32 uniquement. |
| Écriture, suppression, renommage, mkdir, rmdir | Refusés : réservés à `overlay/`. |

Le montage est protégé et s’ajoute aux sources `initrd/`, `overlay/` et `fat16/`. Les buffers IPC, les structures de répertoire et les droits de backend restent inchangés. Aucune allocation dynamique, mutation FAT32 ni ouverture de capacité supplémentaire n’est introduite.

La construction complète et la suite de non-régression valident **457/457 tests**. Le volume FAT32 lui-même est déjà validé par le smoke QEMU multi-disque ; le serveur VFS s’y connecte désormais en lecture seule.

## Références internes

- [Syscalls FAT32 lecture seule](aos1713_1720_fat32_read_syscalls.md)
- [Image FAT32 secondaire](aos1705_1712_fat32_secondary_image_smoke.md)
- [Montage VFS FAT16 lecture seule](aos1665_1672_vfs_fat16_readonly_mount.md)
