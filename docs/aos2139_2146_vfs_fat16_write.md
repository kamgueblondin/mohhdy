# AOS-2139…2146 — Création FAT16 8.3 via le VFS Ring 3

**Statut : livré et validé localement.** Ce macro-lot relie au VFS la primitive noyau `fat16_create_file()` déjà validée par les tests de volume. Une écriture publique `vfs-write fat16/<nom-8.3> <texte>` crée désormais un fichier régulier à la racine du volume FAT16 du disque IDE, puis le rend immédiatement lisible et listable par les chemins VFS existants.

> Le médiateur `vfsserver` reste l’unique client Ring 3 des syscalls backend. Aucune tâche utilisateur arbitraire ne reçoit un accès direct à l’écriture FAT16.

## Chemin d’exécution

| Étape | Responsable | Garantie |
|---|---|---|
| Parse de `vfs-write` et corrélation IPC publique | Shell et VFS | ABI publique inchangée, données limitées à 44 octets |
| Résolution de `fat16/` et extraction du suffixe | `vfsserver` | Chemin limité à un nom racine 8.3 |
| Prévention d’écrasement | `vfsserver` | Un nom déjà présent est refusé avant toute mutation |
| Vérification de capacité | Noyau | Droit backend `mutate` exigé sur le service publié `vfs` |
| Création du fichier | Noyau FAT16 | Clusters, chaînes FAT dupliquées, données et entrée de racine via `fat16_create_file()` |
| Écriture secteur | ATA PIO maître | Writer attaché explicitement après montage FAT16 réussi |
| Réponse et compteur VFS | `vfsserver` | Réponse publique corrélée, même voie que les écritures overlay |

Le syscall `SYS_VFS_FAT16_CREATE` reçoit un nom 8.3 relatif, un buffer caller-owned et une taille. Il est séparé de `SYS_VFS_BACKEND_WRITE`, qui reste la voie historique de l’overlay. Il exige le même droit backend `mutate` et appelle `fat16_create_file()` avec l’attribut fichier régulier `0x20`.

## Contrôles et persistance

Le montage FAT16 attachait déjà un lecteur ATA au LBA 64 et une fenêtre multi-secteurs. Il attache maintenant, après succès du montage, un writer explicitement passé à `fat16_attach_writer()`. Aucun writer implicite n’est créé : si l’attachement échoue, les primitives FAT16 renvoient leur erreur de volume et la création n’est pas annoncée comme un succès.

`vfsserver` n’expose qu’une création de fichier racine et appelle son `stat` FAT16 avant la soumission noyau. Ainsi, le remplacement d’un nom existant n’est pas publié. Les opérations `mkdir`, `rmdir`, suppression et renommage restent nulles dans la table d’opérations FAT16 du VFS.

## Preuves

| Commande | Résultat exigé |
|---|---|
| `make -s -C tests -B '../build/./unit/kernel/test_fat16'` | 19/19, y compris writer explicite et création persistante |
| `make -s -C tests -B '../build/./unit/kernel/test_syscall'` | 64/64 avec harness noyau i386 non-PIE |
| `make -s qemu-vfs-service` | Création `vfs-write fat16/new.txt qemu-fat16`, lecture de `fat16/new.txt`, et liste contenant `NEW.TXT` |
| `make -s test-all` | 487/487 |

Le test QEMU conserve d’abord la vérification des deux entrées fixture. Il crée ensuite `new.txt`, attend la réponse publique d’écriture, relit la charge `qemu-fat16`, puis exige trois entrées à la racine et le nom normalisé `NEW.TXT`. La garde FAT16 rejette aussi un second nom court identique, même s’il est situé au-delà de la première page exposée par le VFS.

## Correctif du harness de test

Les mocks syscall manipulent des registres i386 de 32 bits. Les tests noyau sont donc construits avec `-m32 -fno-pie -no-pie` : ce modèle évite la troncature de pointeurs de pile 64 bits vers `EBX` et `ECX`. Le noyau freestanding produit n’est pas concerné par ce réglage de tests hôte.

## Limites explicites

La livraison ne crée que des fichiers réguliers 8.3 à la racine FAT16, sans écrasement. Les LFN, sous-répertoires, suppression, renommage, remplacement atomique et toute mutation FAT32 demeurent hors périmètre. La transaction de création dépend de l’ATA PIO : le rollback FAT tente de libérer la chaîne si une étape préalable échoue, sans pouvoir annuler une panne physique persistante déjà écrite.

Aucun backend ATA, pilote FAT ou worker de stockage ne quitte le noyau : `vfsserver` applique les politiques de chemin et de capacité, tandis que la logique FAT et les E/S de secteurs restent au noyau.

## Références internes

- [API FAT16](../kernel/fs/fat16.h)
- [Implémentation FAT16](../kernel/fs/fat16.c)
- [Dispatch des syscalls](../kernel/syscall/syscall.c)
- [Médiateur VFS](../userspace/vfs_server.c)
- [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
- [Création FAT16 précédente](aos1177_fat16_create_file.md)

---

**Auteur : Manus AI**

**Date de validation : 22 août 2026**
