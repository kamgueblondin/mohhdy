# Incrément Foundation 27 — Suppression de répertoires VFS médiée

## Objet

L’incrément 27 ajoute `vfs-rmdir <chemin>`, une suppression de répertoire vide médiée par `vfsserver` Ring 3. L’opération est corrélée, ne cible que les montages dont la source déclarée est l’overlay et reste distincte de `vfs-remove`, qui demeure l’opération de suppression de fichier.

| Élément | Contrat |
|---|---|
| Commande shell | `vfs-rmdir <chemin>` |
| Requête / réponse | `OS_IPC_VFS_RMDIR` / `OS_IPC_VFS_RMDIR_REPLY` |
| Requête | Chemin sûr, NUL-terminé, 48 octets fixes |
| Réponse | Statut signé corrélé de 4 octets |
| Syscall backend | `SYS_VFS_OVERLAY_RMDIR` (44) |
| ABI | 45 syscalls, indices 0–44 |

Le serveur compare le chemin global à sa table locale de montages. Sous `overlay/` ou un alias overlay déclaré, il ne transmet que le suffixe relatif au backend réservé au propriétaire vivant de `vfs`. Sous un montage initrd ou hors montage, il retourne `OS_VFS_STATUS_NOT_MOUNTED` avant toute primitive de stockage.

## Sémantique et protection

Le syscall backend vérifie d’abord que le chemin désigne effectivement un répertoire overlay à l’aide de `overlay_is_dir`. Il appelle seulement ensuite `overlay_unlink`. Un fichier ne peut donc pas être retiré par `vfs-rmdir`; il doit suivre le chemin distinct `vfs-remove`.

`overlay_unlink` conserve son refus des répertoires non vides et retourne `OV_ERR_NOTEMPTY` (`-5`). Le shell affiche alors le diagnostic `vfs-rmdir: repertoire non vide`. Une suppression réussie incrémente `vfs_list_generation`, afin qu’une continuation antérieure de `vfs-list-observe` soit déclarée obsolète. Les compteurs historiques `reads`, `writes`, `removes` et `renames` ne changent pas : `removes` continue de représenter les requêtes `vfs-remove` et non les opérations de répertoire.

> Cette médiation est une politique locale : l’overlay ATA, `overlay_unlink` et les syscalls historiques restent dans le noyau. Elle n’apporte ni permissions POSIX, ni capability par répertoire, ni transaction, ni verrouillage, ni snapshot atomique.

## Vérification

Un test Unity supplémentaire valide la requête RMDIR de 48 octets, sa réponse de quatre octets, la restitution du statut négatif et le rejet d’un `request_id` discordant. La suite `make test-all` totalise ainsi **210/210** tests réussis.

Le contrat QEMU VFS crée `overlay/newdir`, confirme son apparition dans le listage, appelle `vfs-rmdir overlay/newdir`, puis confirme que `vfs-list overlay/` redevient vide. Les contrôles ultérieurs des listes overlay et de l’alias `work/` vérifient également l’absence durable de ce répertoire. Le contrat VFS, les contrats IPC et de délégation de service, ainsi que les smokes QEMU `spawn` et `exec`, ont été exécutés avec succès.

La batterie combinée `make integration-qemu` a validé ses contrats cœur et IRQ0 puis a rencontré une instabilité ponctuelle de saisie PS/2 dans le smoke IA sur `ai-model list`; une relance isolée de ce même smoke a réussi. Cette fluctuation, étrangère au chemin VFS, reste à stabiliser dans le test IA sans assouplir ses assertions.
