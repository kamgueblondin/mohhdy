# AOS-2147…2154 — Suppression FAT16 8.3 racine médiée par VFS

**Statut : livré et validé localement.** Ce macro-lot complète la création FAT16 précédemment exposée par une suppression bornée. La commande `vfs-remove fat16/<nom-8.3>` retire désormais un fichier régulier 8.3 de la racine du volume FAT16 ATA maître, sous la même capacité backend `mutate` que `vfs-write`.

> Le médiateur `vfsserver` demeure l’unique client Ring 3 des syscalls backend. Une tâche utilisateur ne peut pas appeler directement la suppression FAT16 ni contourner la vérification de capacité noyau.

## Contrat public et chemin d’exécution

| Étape | Responsable | Garantie |
|---|---|---|
| Parse et corrélation de `vfs-remove` | Shell et VFS | Chemin public validé et réponse corrélée par `request_id` |
| Résolution de `fat16/` | `vfsserver` | Seul le suffixe sans `/` est remis au backend FAT16 |
| Appel backend dédié | `vfsserver` | `SYS_VFS_FAT16_UNLINK` ; aucun réemploi de `unlink` overlay |
| Vérification d’autorité | Noyau | Droit backend `mutate` exigé sur le service publié `vfs` |
| Validation FAT16 | `fat16_unlink_file()` | Nom court 8.3, entrée de racine classique, ni LFN, ni label, ni répertoire |
| Publication de la suppression | FAT16 / ATA PIO maître | Premier octet de l’entrée de racine fixé à `0xE5`, écriture de secteur explicite |
| Restitution des ressources | FAT16 | Parcours de chaîne borné et mise à zéro de chaque entrée des copies FAT |
| Observation VFS | `vfsserver` | Réponse corrélée, compteur `removes` et génération de listage incrémentés en cas de succès |

Le syscall ABI `SYS_VFS_FAT16_UNLINK` est le numéro 117 ; `MAX_SYSCALLS` passe à 118. Il reçoit seulement le nom relatif et appelle `fat16_unlink_file(fat16_root(), name)` après le contrôle `SERVICE_BACKEND_RIGHT_MUTATE`. Il reste distinct de `SYS_VFS_OVERLAY_UNLINK` : aucune politique overlay ni aucun repli de backend ne s’applique à FAT16.

## Sécurité de la mutation et persistance

La primitive noyau reconstruit le nom 8.3 avec le même contrôle que la création, puis parcourt au plus `root_entries` entrées de la racine. Elle refuse les séquences LFN, les labels de volume et les répertoires. Un fichier vide FAT16 valide sans cluster est supprimé par son seul marquage ; pour un fichier normal, le premier cluster est borné avant toute libération.

L’entrée est marquée supprimée et écrite sur ATA **avant** la restitution de la chaîne. Cette séquence évite de laisser une entrée visible qui référence des clusters déjà libérés si une étape ultérieure échoue. La libération traverse la chaîne avec un garde borné à `cluster_count`, lit chaque lien, puis écrit la valeur libre `0` dans chaque copie FAT. Une erreur d’E/S est propagée : le lot ne prétend pas offrir une atomicité face à une panne physique après une écriture de secteur déjà confirmée.

## Preuves exécutées

| Commande | Résultat observé |
|---|---|
| `make -s -C tests -B '../build/./unit/kernel/test_fat16' && ./build/unit/kernel/test_fat16` | **20/20** ; suppression de `PERSIST.BIN`, marqueur `0xE5`, libération des deux clusters dans les deux FAT, relecture absente et réutilisation de cluster 3 |
| `make -s -C tests -B '../build/./unit/kernel/test_syscall' && ./build/unit/kernel/test_syscall` | **64/64** ; borne ABI de syscall et harness i386 conservés |
| `make -s qemu-vfs-service` | `MOHHDY Foundation VFS service contract passed` ; création, relecture, listage de `NEW.TXT`, suppression, lecture négative et retour à deux entrées FAT16 |
| `make -s test-all` | **488/488** ; non-régression complète, aucun échec ni test ignoré |

Le contrat QEMU utilise d’abord `vfs-write fat16/new.txt qemu-fat16`, relit le contenu et exige trois entrées à la racine incluant `NEW.TXT`. Il envoie ensuite `vfs-remove fat16/new.txt`, exige la réponse publique de succès, vérifie que la lecture devient « fichier absent », puis impose un listage de deux entrées où `NEW.TXT` ne figure plus. La régression Unity crée aussi `TRAIL.BIN` après `PERSIST.BIN`, supprime ce dernier, puis rejette un second `TRAIL.BIN` : un emplacement `0xE5` réutilisable n’interrompt donc jamais le parcours anti-doublon de la racine.

## Limites explicites

La livraison ne publie que la création et la suppression de **fichiers réguliers 8.3 à la racine FAT16**. Aucun écrasement, remplacement transactionnel, renommage, LFN VFS, sous-répertoire, suppression de répertoire ou mutation FAT32 n’est exposé. FAT32 reste lecture seule derrière le VFS, même si le noyau possède des primitives FAT32 distinctes.

Aucun pilote ATA, volume FAT ni logique de chaîne ne quitte le noyau. `vfsserver` conserve les politiques de chemin et de capacité ; la validation FAT, les écritures de secteurs et la persistance ATA PIO demeurent en Ring 0. Aucun chemin modifié n’introduit `malloc`, `calloc`, `realloc`, `free` ou `new`.

## Références internes

- [Contrat FAT16](../kernel/fs/fat16.h)
- [Implémentation FAT16](../kernel/fs/fat16.c)
- [Dispatch des syscalls](../kernel/syscall/syscall.c)
- [Médiateur VFS](../userspace/vfs_server.c)
- [Régression Unity FAT16](../tests/unit/kernel/test_fat16.c)
- [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
- [Création FAT16 VFS](aos2139_2146_vfs_fat16_write.md)

---

**Auteur : Manus AI**

**Date de validation : 23 août 2026**
