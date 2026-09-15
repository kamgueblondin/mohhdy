# AOS-1665 à AOS-1672 — montage VFS FAT16 en lecture seule

## Objet

Ce macro-lot raccorde le volume FAT16 déjà monté par le noyau au médiateur VFS Ring 3. Il ajoute la source de montage `OS_VFS_MOUNT_SOURCE_FAT16` et le montage protégé `fat16/`, sans allocation dynamique et sans changer les sources `initrd/` ou `overlay/`.

## Contrat

| Montage | Droits fonctionnels | Implémentation |
|---|---|---|
| `initrd/` | Lecture, métadonnées, listage | Inchangé. |
| `overlay/` | Lecture, écriture, suppression, renommage, répertoires | Inchangé. |
| `fat16/` | Lecture, métadonnées et listage de racine | Nouveau, lecture seule. |

Le serveur VFS appelle les syscalls FAT16 préexistants pour la lecture et le listage. La racine `fat16/` est un montage protégé et les opérations d’écriture, de création de répertoire, de suppression et de renommage continuent à vérifier explicitement que la source est `overlay`.

> Aucun état de fichier, cache de noms ou allocation dynamique n’est ajouté : les réponses continuent d’utiliser les buffers IPC bornés et les structures publiques `os_dirent_t` et `os_fat16_dirent_t`, dont la représentation est compatible.

## Chemins pris en charge

La lecture transmet exclusivement le suffixe du montage au lecteur FAT16. `fat16/FICHIER.TXT` et les LFN ASCII supportés par le volume suivent donc le même chemin de validation que les syscalls FAT16 directs. Le listage accepte uniquement la racine du volume, cohérente avec l’absence de sous-répertoires FAT16 dans ce périmètre.

## Validation

La construction i386 complète réussit avec le serveur VFS étendu. La suite de non-régression valide **457 tests sur 457**, notamment FAT16, FAT32, protocole VFS, IPC, services et userspace. Les chemins d’écriture overlay ne sont pas modifiés.

## Limites

Les volumes FAT32, les sous-répertoires FAT16 et le montage de volumes FAT écrits depuis VFS restent hors de ce lot. La source FAT16 est intentionnellement lecture seule afin de préserver les invariants du volume de déploiement GPT2.GGU.

## Références internes

- [Recherche FAT16 par LFN](aos1649_1656_fat16_lfn_read_lookup.md)
- [Suppression FAT32 par LFN](aos1657_1664_fat32_lfn_unlink.md)
- [Contrats de service VFS](../include/os_vfs_service.h)
