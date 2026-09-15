# Incrément Foundation 23 — Listage VFS médié de sous-répertoire

## Objet

L’incrément 23 étend le listage VFS livré précédemment. La commande `vfs-list` ne se limite plus à la racine d’un préfixe monté : elle accepte désormais la racine ou un **sous-répertoire** d’un montage déclaré, tout en conservant le médiateur Ring 3, la corrélation de réponse et le choix exclusif d’une source.

> Cette extension n’ajoute ni syscall ni type IPC. Elle élargit uniquement la grammaire sûre de la requête LIST existante et le routage local du serveur `vfs`.

| Élément | Contrat livré |
|---|---|
| Commande | `vfs-list <repertoire/>` |
| Exemple initrd | `vfs-list initrd/bin/` |
| Exemple overlay | `vfs-list overlay/sous-repertoire/` si le répertoire existe |
| Syscalls backend | `SYS_VFS_INITRD_LISTDIR` (39) et `SYS_VFS_OVERLAY_LISTDIR` (40) |
| Messages IPC | `OS_IPC_VFS_LIST` et `OS_IPC_VFS_LIST_REPLY`, inchangés |
| ABI | 41 syscalls (`0` à `40`), inchangée |

## Validation de chemin et routage

Le chemin de listage doit être NUL-terminé dans la charge IPC de 48 octets, être sûr selon la politique VFS existante, ne contenir aucun composant `..` et se terminer par `/`. Le suffixe terminal distingue un répertoire d’un chemin de fichier au niveau du contrat client.

Le serveur compare le préfixe initial à sa table de montages. Pour une racine telle que `initrd/`, il remet `/` au backend. Pour un sous-répertoire tel que `initrd/bin/`, il retire uniquement `initrd/` et transmet `bin/` au backend initrd. Un alias dynamique applique la même règle et continue de sélectionner sa source propre.

| Demande | Suffixe backend | Source consultée | Résultat attendu |
|---|---|---|---|
| `initrd/` | `/` | Initrd | Liste de la racine initrd |
| `initrd/bin/` | `bin/` | Initrd | Liste de `bin/` dans l’initrd |
| `work/` avec alias overlay | `/` | Overlay | Liste de la racine overlay |
| `absent/` | Aucun | Aucune | `OS_VFS_STATUS_NOT_MOUNTED` |
| `initrd/hello.txt/` | Aucun backend valide | Initrd | Refus : ce n’est pas un répertoire |

Les syscalls backend confirment ensuite que le suffixe désigne un répertoire réel à l’aide de `initrd_is_dir` ou `overlay_is_dir`. Cette seconde vérification évite qu’un fichier artificiellement suffixé par `/` soit présenté comme une liste vide.

## Réponse et bornes

Le format de réponse reste exactement celui de l’incrément 22. Il ne traverse jamais la limite de charge IPC de 96 octets.

| Champ | Taille | Signification |
|---|---:|---|
| `status` | 4 octets | Succès, page partielle ou erreur VFS |
| `count` | 4 octets | Nombre de noms sérialisés, de 0 à 4 |
| `data` | 72 octets | Noms séparés par `\n`, padding nul |
| Réponse LIST | 80 octets | Taille IPC fixe et corrélée |

Une cinquième entrée backend ne sert qu’à signaler une page partielle. Les noms qui ne tiennent pas dans les 72 octets de données déclenchent aussi l’état partiel. Le protocole ne garantit ni l’ordre, ni la pagination par curseur, ni l’atomicité devant une mutation concurrente.

## Vérification

La suite Unity ajoute un cas qui accepte `initrd/bin/` et rejette `initrd/bin` ainsi que `initrd/bin/shell`. Elle compte désormais **206 tests** au total, dont 21 pour le protocole VFS. Le contrat QEMU VFS vérifie `vfs-list initrd/bin/`, observe une page initrd contenant notamment `shell`, puis poursuit les vérifications de montage, d’alias, de mutations, de statistiques, de transfert et de révocation.

Les scénarios QEMU Foundation utilisent des tentatives limitées à trois pour les commandes sensibles aux doubles frappes PS/2 ; chaque tentative attend le même marqueur fonctionnel strict. Cette mesure stabilise l’observation sans modifier les invariants contrôlés.

## Limites honnêtes

Le listage reste une opération locale du médiateur `vfs`, et non une abstraction de répertoire complète. Il est limité à quatre noms et une page, sans curseur, filtrage, tri, métadonnées par entrée, verrouillage ou snapshot atomique. Les montages et alias sont volatils, le registre n’apporte aucune capability et les backends initrd/overlay restent noyau. Les syscalls de fichiers historiques ne sont pas retirés : le mécanisme reste une politique Ring 3, pas une isolation microkernel.

## Fichiers principaux

| Fichier | Modification |
|---|---|
| `include/os_vfs_service.h` | Validation de chemin LIST se terminant par `/` |
| `kernel/syscall/syscall.c` | Rejet backend des chemins qui ne sont pas des répertoires |
| `userspace/vfs_server.c` | Résolution racine/suffixe de sous-répertoire par montage |
| `userspace/shell.c` | Syntaxe et diagnostics `vfs-list <repertoire/>` |
| `tests/unit/kernel/test_vfs_service.c` | Cas Unity de sous-répertoire et de refus |
| `tests/integration/test_qemu_vfs_service.py` | Contrat QEMU `initrd/bin/` et relances strictement bornées |
