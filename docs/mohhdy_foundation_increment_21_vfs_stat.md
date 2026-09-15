# Incrément MOHHDY Foundation 21 — Métadonnées VFS médiées

## Objectif

Cet incrément étend le médiateur `vfsserver` à la consultation de métadonnées. La commande `vfs-stat <chemin>` renvoie la taille et le type d’une entrée montée, sans permettre au shell d’appeler directement le backend noyau. La résolution respecte la table locale de montages, y compris les alias dynamiques.

> Le médiateur transmet seulement le suffixe relatif au montage déclaré. Il n’existe aucun repli d’un backend à l’autre : une entrée `initrd/` appelle uniquement l’initrd, une entrée `overlay/` appelle uniquement l’overlay.

| Élément | Valeur livrée |
|---|---|
| Commande utilisateur | `vfs-stat <chemin>` |
| Message demande | `OS_IPC_VFS_STAT` (`0x5646530e`) |
| Message réponse | `OS_IPC_VFS_STAT_REPLY` (`0x5646530f`) |
| Requête IPC | chemin VFS borné à 48 octets |
| Réponse IPC | `status`, `size`, `flags` — 12 octets, corrélés par `request_id` |
| Syscalls backend | `SYS_VFS_INITRD_STAT` (37), `SYS_VFS_OVERLAY_STAT` (38) |
| ABI totale | 39 syscalls, numéros 0–38 |

## Routage et contrôle

Les nouveaux syscalls n’acceptent que le propriétaire utilisateur courant du service nommé `vfs`. Toute autre tâche Ring 3 reçoit `OS_VFS_BACKEND_DENIED`. Le serveur sélectionne le backend depuis la source déclarée par le montage, appelle `initrd_stat` ou `overlay_stat`, puis retourne les champs `os_dirent_t.size` et `os_dirent_t.flags` dans une réponse IPC corrélée.

La commande affiche notamment :

```text
vfs-stat initrd/hello.txt
vfs-stat ok size 35 flags file request 12
```

Un chemin sans préfixe déclaré est refusé par `vfsserver` avec `vfs-stat: chemin hors montage`. Les sources virtuelles comme `vfs-info` restent des contenus du médiateur de lecture ; elles ne sont pas exposées comme des entrées persistantes par ce nouveau chemin de métadonnées.

## Validation

Deux tests Unity supplémentaires vérifient l’encodage et le décodage de la requête et réponse de statut, la taille exacte de 12 octets, les drapeaux, la corrélation et les refus de chemin, taille, type ou identifiant invalides. La suite compte **203/203** tests Unity et robustesse.

Le contrat QEMU VFS vérifie une entrée initrd (`35` octets, type fichier), une entrée overlay immédiatement après écriture (`5` octets, type fichier) et le refus d’un chemin hors montage. Les six contrats de `make integration-qemu` réussissent. Les relances QEMU restent bornées à trois tentatives et exigent toujours le même marqueur fonctionnel ; elles compensent les rebonds PS/2 observés sous QEMU TCG, sans relâcher les assertions de politique, de contenu ou de corrélation.

## Limites honnêtes

Le résultat est un instantané local non atomique : une mutation peut survenir immédiatement après la réponse. Il n’apporte ni verrouillage, ni capacité, ni autorisation par répertoire, ni attributs POSIX complets, ni horodatage, ni propriétaire, ni liens symboliques, ni répertoires virtuels persistants. Les métadonnées restent fournies par les backends noyau ; cet incrément ne les externalise pas et ne transforme pas MOHHDY en microkernel.
