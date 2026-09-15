# AOS-1713 à AOS-1720 — syscalls FAT32 de lecture et listage

Ce macro-lot expose le volume FAT32 secondaire statique aux programmes Ring 3 par deux appels ABI lecture seule : `SYS_FAT32_READ` (111) et `SYS_FAT32_LIST` (112).

| Appel | Registres | Contrat |
|---|---|---|
| `SYS_FAT32_READ` | EBX nom 8.3, ECX buffer, EDX capacité | Lit le fichier depuis le volume FAT32 secondaire dans un buffer utilisateur borné. |
| `SYS_FAT32_LIST` | EBX tableau `os_fat16_dirent_t`, ECX capacité | Liste la racine FAT32 dans le tableau fourni par l’appelant. |

Les deux chemins refusent les appels non Ring 3, les arguments nuls et les capacités nulles avant toute opération de disque. Ils délèguent aux primitives FAT32 existantes, n’ajoutent aucun cache de fichier et n’exposent aucune mutation. Les codes d’erreur FAT existants sont conservés.

La construction i386 réussit et la suite complète valide **457/457 tests**. Le prochain incrément connecte ces appels au médiateur VFS sous un montage `fat32/` lecture seule.

## Références internes

- [Lecture FAT32 par alias](aos1681_1688_fat32_alias_read.md)
- [Image FAT32 secondaire](aos1705_1712_fat32_secondary_image_smoke.md)
- [ABI des syscalls](../include/os_syscalls.h)
