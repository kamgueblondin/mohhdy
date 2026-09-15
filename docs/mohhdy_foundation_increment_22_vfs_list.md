# Incrément Foundation 22 — Listage VFS médié par source

## Objet

Cet incrément ajoute un **listage de répertoire VFS médié**. Le shell ne lit plus une liste de répertoire par l’ABI générique lorsqu’il passe par `vfs-list` : il découvre le service nommé `vfs`, transmet un préfixe de montage corrélé et reçoit une page de noms construite par `vfsserver` en Ring 3.

> Le listage ne fusionne jamais l’initrd et l’overlay. Un préfixe de montage détermine une source unique, y compris lorsqu’il s’agit d’un alias dynamique.

| Élément | Contrat livré |
|---|---|
| Commande shell | `vfs-list <prefixe/>` |
| Entrée initrd | `SYS_VFS_INITRD_LISTDIR` (`39`) |
| Entrée overlay | `SYS_VFS_OVERLAY_LISTDIR` (`40`) |
| Borne ABI | `MAX_SYSCALLS = 41` |
| Requête IPC | `OS_IPC_VFS_LIST` (`0x56465310`) |
| Réponse IPC | `OS_IPC_VFS_LIST_REPLY` (`0x56465311`) |
| Sélection de source | Préfixe de montage déclaré, égalité exacte |

## Contrat IPC

La requête transporte un préfixe terminé par `/` dans le champ de chemin de 48 octets. Elle est refusée lorsqu’il n’est pas NUL-terminé, ne se termine pas par `/`, contient `..` ou ne correspond à aucun montage déclaré dans le serveur. La réponse recopie le `request_id` opaque du client et reste strictement dans les 96 octets de charge IPC.

| Champ de réponse | Taille | Sémantique |
|---|---:|---|
| `status` | 4 octets | `0`, `OS_VFS_STATUS_TRUNCATED` ou une erreur VFS |
| `count` | 4 octets | Nombre de noms effectivement sérialisés, entre 0 et 4 |
| `data` | 72 octets | Noms séparés par `\n`, complétés par des zéros |
| Total | 80 octets | Taille exacte de `OS_VFS_LIST_REPLY` |

Le serveur demande au backend au plus cinq entrées : quatre sont potentiellement retournées et la cinquième permet de signaler qu’une page est partielle. Un nom qui ne peut pas tenir dans les 72 octets restants n’est pas sérialisé et déclenche également l’état `partiel`.

## Routage et privilèges

Les deux syscalls backend vérifient que l’appelant est une tâche utilisateur et le propriétaire courant du nom de service `vfs`. Ils refusent sinon l’accès avec `OS_VFS_BACKEND_DENIED`. Le backend initrd appelle uniquement `initrd_listdir`, tandis que le backend overlay appelle uniquement `overlay_listdir` avec un point de départ nul. Le syscall historique `SYS_LISTDIR` est conservé pour les commandes historiques ; il reste fusionné et ne participe pas au chemin médié.

Le médiateur compare le préfixe de la requête avec la table locale des montages. Il appelle ensuite exclusivement le backend de la source associée à la racine de cette source. Ainsi, `vfs-list initrd/` ne révèle pas les fichiers overlay et `vfs-list work/`, lorsque `work/` est un alias overlay, révèle la même source persistante que `overlay/` sans fusion avec l’initrd.

## Utilisation

Après `spawn vfsserver`, les exemples suivants interrogent des montages déclarés :

```text
vfs-list initrd/
vfs-list overlay/
vfs-mount-add work/ overlay
vfs-list work/
```

Une réponse normale affiche `vfs-list ok`, le nombre d’entrées puis les noms. Une réponse `vfs-list partiel` reste un succès fonctionnel limité : seule la première page peut être affichée. Une demande telle que `vfs-list absent/` retourne un diagnostic de montage absent.

## Vérification

Les tests Unity du protocole contrôlent l’encodage et le décodage de la requête, la corrélation du `request_id`, la taille exacte de 80 octets, le padding nul ainsi que le rejet des préfixes et comptes invalides. La suite contient désormais **205 tests**, dont 20 pour le protocole VFS.

Le contrat QEMU `make qemu-vfs-service` vérifie l’enchaînement suivant :

| Scénario | Propriété démontrée |
|---|---|
| `vfs-list initrd/` | Page partielle bornée et noms provenant de l’archive seule |
| `vfs-list overlay/` initial | Source overlay vide, sans repli initrd |
| `vfs-list missing/` | Refus explicite d’un préfixe absent |
| Alias `work/` overlay | Listage de la source overlay par l’alias dynamique |
| `vfs-list overlay/` après écriture | Persistance de la source et visibilité des noms overlay existants |

## Limites connues

Le mécanisme n’est pas encore un parcours de répertoire général. Il ne liste que la **racine** d’un montage déclaré, ne renvoie qu’une page de **quatre noms** au plus, n’expose aucun curseur de pagination, aucun tri contractuel, aucun type ni aucune taille par nom, et ne fournit pas d’instantané atomique face aux mutations. Les montages restent locaux au serveur `vfs`, volatils et non transférés ; les backends initrd et overlay restent dans le noyau. Enfin, la médiation ne retire pas les syscalls historiques génériques : elle constitue une politique Ring 3 ciblée, non une isolation par capability.

## Fichiers principaux

| Fichier | Rôle |
|---|---|
| `include/os_syscalls.h` | Syscalls 39–40 et borne `MAX_SYSCALLS` |
| `include/os_vfs_service.h` | Types, encodeurs et parseurs `LIST` bornés |
| `kernel/syscall/syscall.c` | Garde de propriétaire et appels initrd/overlay distincts |
| `userspace/vfs_server.c` | Routage par montage et sérialisation de la page |
| `userspace/shell.c` | Commande corrélée `vfs-list` |
| `tests/unit/kernel/test_vfs_service.c` | Deux cas Unity supplémentaires |
| `tests/integration/test_qemu_vfs_service.py` | Contrat QEMU de routage et de persistance |
