# AOS-1689 à AOS-1696 — sélection ATA primaire maître/esclave

Ce macro-lot étend le pilote ATA PIO LBA28 primaire pour sélectionner explicitement le **maître** ou l’**esclave**. Les interfaces historiques `ata_read_sectors` et `ata_write_sectors` demeurent des wrappers strictement compatibles du maître primaire.

| API | Rôle |
|---|---|
| `ata_present_drive(drive)` | Indique si le maître ou l’esclave est détecté. |
| `ata_read_sectors_drive(drive, lba, count, buffer)` | Lit un intervalle LBA28 sur le disque choisi. |
| `ata_write_sectors_drive(drive, lba, count, buffer)` | Écrit un intervalle LBA28 sur le disque choisi. |
| `ata_read_sectors` / `ata_write_sectors` | Compatibilité historique : délégation au maître. |

Le registre ATA `0x1F6` reçoit le bit esclave lorsque nécessaire. La détection de l’esclave reste non bloquante : l’absence de second disque ne retire pas le maître déjà validé. Le pilote ne crée aucun buffer ni état dynamique supplémentaire.

> Cette extension est le prérequis matériel d’une image FAT32 IDE séparée. Le volume GGUF/FAT16 du maître primaire reste inchangé.

La construction i386 réussit et la suite complète valide **457/457 tests**. Les tests matériels QEMU multi-disque et le montage du volume FAT32 constituent l’incrément suivant.

## Références internes

- [Montage VFS FAT16 lecture seule](aos1665_1672_vfs_fat16_readonly_mount.md)
- [Lecture FAT32 par alias](aos1681_1688_fat32_alias_read.md)
- [Contrat ATA](../kernel/ata.h)
