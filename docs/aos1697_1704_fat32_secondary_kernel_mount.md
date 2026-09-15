# AOS-1697 à AOS-1704 — volume FAT32 secondaire statique au noyau

Ce macro-lot introduit une instance FAT32 globale statique accessible par `fat32_root()`, puis tente son montage au démarrage sur le disque ATA primaire esclave, au LBA zéro. L’adaptateur de secteur utilise `ata_read_sectors_drive(ATA_DRIVE_SLAVE, ...)`.

Le montage est **non bloquant** : il n’est tenté que lorsqu’un esclave ATA est détecté. Les configurations historiques à disque unique poursuivent donc exactement le montage FAT16 maître au LBA 64, l’overlay et l’initialisation GGUF sans dépendance FAT32.

| Élément | Garantie |
|---|---|
| État FAT32 | Instance globale statique ; aucune allocation dynamique. |
| Support | Disque ATA primaire esclave, LBA zéro. |
| Absence d’esclave | Aucun échec de boot ; FAT16 maître inchangé. |
| Volume GGUF | Toujours FAT16 maître LBA 64. |

La construction i386 réussit et la suite complète valide **457/457 tests**. La prochaine étape est la fabrication de l’image FAT32 esclave et son test QEMU multi-disque, avant l’exposition VFS FAT32.

## Références internes

- [Sélection ATA maître/esclave](aos1689_1696_ata_multidrive.md)
- [Lecture FAT32 par alias](aos1681_1688_fat32_alias_read.md)
- [Contrats FAT32](../kernel/fs/fat32.h)
