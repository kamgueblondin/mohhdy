# AOS-2155…2162 — Renommage FAT16 8.3 racine médié par VFS

**Statut : livré et validé localement.** Ce macro-lot complète la création et la suppression FAT16 VFS par un renommage strictement borné. La commande `vfs-rename fat16/<ancien-8.3> fat16/<nouveau-8.3>` change désormais le nom court d’un fichier régulier à la racine du volume FAT16 ATA maître, sous la capacité backend `mutate`.

> Le renommage FAT16 ne déplace aucun cluster et ne modifie aucune donnée. Il réécrit exclusivement les onze octets 8.3 de l’entrée de racine source après vérification complète de la cible.

## Contrat et chemin d’exécution

| Étape | Responsable | Garantie |
|---|---|---|
| Parse de `vfs-rename` | Shell et VFS | Deux chemins corrélés par `request_id` |
| Résolution `fat16/` | `vfsserver` | Deux suffixes obligatoirement relatifs, non vides et sans `/` |
| Appel backend dédié | `vfsserver` | `SYS_VFS_FAT16_RENAME`, distinct de `SYS_VFS_OVERLAY_RENAME` |
| Vérification d’autorité | Noyau | Droit backend `mutate` exigé sur le service `vfs` |
| Validation FAT16 | `fat16_rename_file()` | Ancien et nouveau noms 8.3, entrée classique seulement, ni LFN, ni label, ni répertoire |
| Prévention de collision | FAT16 | Parcours entier de racine avant écriture ; une cible existante est refusée |
| Persistance | ATA PIO maître | Réécriture du seul secteur de racine concerné par writer explicitement attaché |
| Visibilité VFS | `vfsserver` | Réponse corrélée, compteur `renames` et génération de listage incrémentés à succès |

`SYS_VFS_FAT16_RENAME` est le syscall 118 et porte `MAX_SYSCALLS` à 119. Le noyau vérifie les droits avant d’appeler la primitive FAT16. Le médiateur n’expose cette opération que dans la table du backend FAT16 ; initrd et FAT32 restent sans callback de mutation, tandis que l’overlay conserve son renommage complet historique.

## Invariants de stockage

La primitive transforme les deux noms ASCII en représentations 8.3, puis parcourt au plus `root_entries` entrées. Elle mémorise l’index de la source et refuse tout nom cible déjà présent, que la cible précède ou suive la source. Les entrées supprimées `0xE5` et la fin logique ne font pas masquer un doublon accessible plus loin ; les séquences LFN, labels et répertoires sont hors contrat et sont refusés.

Après ce parcours, la primitive recharge le secteur de la source et réécrit ses onze octets de nom. Les champs attributs, taille, date, premier cluster et les deux copies FAT restent inchangés. Cette opération n’est pas annoncée comme atomique face à une panne ATA persistante après écriture d’un secteur déjà confirmée.

## Preuves exécutées

| Commande | Résultat observé |
|---|---|
| `make -s -C tests -B '../build/./unit/kernel/test_fat16' && ./build/unit/kernel/test_fat16` | **21/21** ; renommage de deux clusters, conservation de la chaîne `3 → 4`, contenu relu sous le nouveau nom, ancien nom absent et collision `TARGET.BIN` refusée |
| `make -s -C tests -B '../build/./unit/kernel/test_syscall' && ./build/unit/kernel/test_syscall` | **64/64** ; dispatch i386 compatible avec la borne ABI 119 |
| `make -s qemu-vfs-service` | `MOHHDY Foundation VFS service contract passed` ; `NEW.TXT` devient `RENAMED.TXT`, l’ancien nom devient absent, le contenu est relu, puis le nouveau nom est supprimé |
| `make -s test-all` | **489/489** ; non-régression complète, aucun échec ni test ignoré |

Le contrat QEMU crée d’abord `fat16/new.txt` avec le contenu `qemu-fat16`. Il exige ensuite le succès de `vfs-rename fat16/new.txt fat16/renamed.txt`, l’échec de lecture de l’ancien nom, la relecture du contenu sous `renamed.txt` et la présence de `RENAMED.TXT` dans le listage à trois entrées. Il retire enfin `renamed.txt` et vérifie le retour à deux entrées.

## Limites explicites

La livraison fournit uniquement un renommage de fichier régulier 8.3 dans la racine FAT16. Elle n’offre ni LFN VFS, ni sous-répertoire, ni renommage de répertoire, ni écrasement de cible, ni remplacement transactionnel, ni changement de montage, ni mutation FAT32. La création, la suppression et le renommage FAT16 restent sous la capacité `mutate` du médiateur ; ils ne transforment pas le volume FAT en service Ring 3.

Aucun chemin ajouté n’emploie `malloc`, `calloc`, `realloc`, `free` ou `new`. Les buffers de secteur restent statiques ou caller-owned et les pilotes ATA/FAT restent en Ring 0.

## Références internes

- [Contrat FAT16](../kernel/fs/fat16.h)
- [Implémentation FAT16](../kernel/fs/fat16.c)
- [Dispatch des syscalls](../kernel/syscall/syscall.c)
- [Médiateur VFS](../userspace/vfs_server.c)
- [Régression Unity FAT16](../tests/unit/kernel/test_fat16.c)
- [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
- [Suppression FAT16 VFS](aos2147_2154_vfs_fat16_unlink.md)

---

**Auteur : Manus AI**

**Date de validation : 23 août 2026**
