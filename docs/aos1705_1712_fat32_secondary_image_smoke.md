# AOS-1705 à AOS-1712 — image FAT32 secondaire et smoke QEMU multi-disque

Ce macro-lot ajoute `scripts/make_fat32_secondary_image.py` et la cible `make fat32-secondary-disk`. L’image constitue un disque ATA esclave distinct, avec un BPB FAT32 valide, une racine au cluster 2 et la fixture `FAT32OK.TXT`.

La géométrie comporte 70 000 secteurs avec un secteur par cluster et deux FAT de 600 secteurs. Elle dépasse donc le seuil de 65 525 clusters exigé par le monteur FAT32 du noyau. Cette contrainte distingue explicitement une image FAT32 réelle d’une petite image qui serait classée FAT16 par la géométrie.

Le smoke QEMU attache l’overlay sur le maître et l’image FAT32 sur l’esclave. La trace de démarrage confirme successivement :

> `FAT32 secondaire monte.`
>
> `FAT16: volume lecture seule monte`

Ainsi, le nouveau volume est monté sans interférer avec le volume FAT16 maître contenant le profil GGUF. Le processus de génération ne crée aucun état noyau dynamique.

## Références internes

- [Sélection ATA maître/esclave](aos1689_1696_ata_multidrive.md)
- [Volume FAT32 secondaire statique](aos1697_1704_fat32_secondary_kernel_mount.md)
- [Lecture FAT32 par alias](aos1681_1688_fat32_alias_read.md)
