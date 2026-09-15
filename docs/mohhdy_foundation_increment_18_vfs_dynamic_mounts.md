# Incrément MOHHDY Foundation 18 — Montages VFS dynamiques bornés

## Objectif

Cet incrément permet au médiateur `vfsserver` Ring 3 d’ajouter ou de retirer des **alias de montage** à l’exécution. Il conserve le modèle de sécurité déjà livré : le noyau ne reçoit aucun nouveau syscall, les mutations backend restent réservées au propriétaire courant du service nommé `vfs`, et chaque demande reçoit une réponse IPC corrélée par `request_id`.

| Élément | Contrat livré |
|---|---|
| Capacité totale | Cinq entrées au maximum : `initrd/`, `overlay/` et trois alias dynamiques |
| Sources admises | `initrd` en lecture seule et `overlay` en lecture-écriture-suppression-renommage |
| Entrées de démarrage | `initrd/` et `overlay/` sont protégées contre le retrait |
| Préfixe dynamique | Chemin sûr, terminé par `/`, sans `..`, limité à 13 octets utiles |
| Ambiguïté | Les préfixes égaux ou qui se recouvrent sont refusés |
| Réinitialisation | La table est locale à `vfsserver` et revient à ses deux montages de démarrage après redémarrage ou transfert du serveur |

> Un alias n’est pas un nouveau backend. Il donne seulement un second préfixe à une source déjà implémentée par le médiateur.

## Protocole IPC

Les valeurs de syscall restent `0–35`. Le protocole VFS partagé ajoute quatre types de message dans la charge IPC existante de 96 octets.

| Requête / réponse | Type | Charge utile | Résultat |
|---|---:|---:|---|
| Ajout | `OS_IPC_VFS_MOUNT_ADD` (`0x5646530a`) | préfixe de 48 octets + source `u32` | installe un alias si la table et la politique l’autorisent |
| Réponse d’ajout | `OS_IPC_VFS_MOUNT_ADD_REPLY` (`0x5646530b`) | statut `i32` | garde le `request_id` du client |
| Retrait | `OS_IPC_VFS_MOUNT_REMOVE` (`0x5646530c`) | préfixe de 48 octets | retire un alias dynamique exact |
| Réponse de retrait | `OS_IPC_VFS_MOUNT_REMOVE_REPLY` (`0x5646530d`) | statut `i32` | garde le `request_id` du client |

Les statuts `OS_VFS_STATUS_MOUNT_FULL` (`-62`) et `OS_VFS_STATUS_MOUNT_EXISTS` (`-63`) distinguent respectivement une table saturée et un préfixe déjà publié. Les préfixes invalides, recouvrants ou protégés sont refusés par `OS_VFS_STATUS_INVALID` (`-60`) ; un retrait d’alias absent retourne `OS_VFS_STATUS_NOT_MOUNTED` (`-61`).

## Interface shell

Le shell ne parle pas directement au backend. Il résout d’abord `vfs`, construit la requête, attend la réponse corrélée en conservant les messages discordants dans sa FIFO différée, puis affiche le résultat.

```text
vfs-mount-add assets/ initrd
vfs-mount-add work/ overlay
vfs-read assets/hello.txt
vfs-write work/note.txt texte
vfs-mount-remove work/
vfs-read vfs-mounts
```

La source virtuelle `vfs-mounts` est construite à partir de la table locale. Son format reste borné à 80 octets grâce à trois alias de 13 octets utiles au maximum. Les opérations d’écriture, suppression et renommage n’acceptent qu’une entrée de source `overlay`, même lorsqu’elle est désignée par un alias dynamique.

## Vérification

La suite porte à **200/200** les tests Unity et robustesse. Deux tests supplémentaires vérifient l’encodage, la validation de longueur et de source, le retrait et la corrélation des réponses de montage.

Le contrat QEMU VFS vérifie notamment les conditions suivantes :

| Cas | Résultat attendu |
|---|---|
| `assets/ → initrd` | lecture de `assets/hello.txt` depuis l’initrd |
| `work/ → overlay` | écriture puis lecture d’un fichier via l’alias |
| Capacité | le quatrième alias est refusé quand la table contient déjà cinq entrées |
| Protection | `vfs-mount-remove initrd/` est refusé |
| Retrait | `work/` devient immédiatement hors montage après retrait |
| Observabilité | les quatre compteurs VFS restent cohérents et ne comptent pas les requêtes de gestion de montage |

Les six contrats QEMU de `make integration-qemu` sont réussis. Les entrées clavier PS/2 du contrat VFS sont cadencées à 0,90 s et certaines commandes longues sont relancées au plus trois fois, uniquement jusqu’à l’apparition de leur marqueur fonctionnel attendu. Cette relance ne transforme pas un échec métier en succès : elle protège exclusivement contre les doubles frappes observées sous QEMU TCG.

## Limites honnêtes

La table n’est ni persistante, ni transactionnelle, ni synchronisée entre plusieurs instances de serveur. Elle ne contient aucun descripteur de périphérique, identité vérifiée, capability, point de montage hiérarchique, drapeau avancé, métrique, démontage forcé ou notification d’abonnement. Les aliases ne déplacent pas le backend initrd ou overlay hors du noyau et ne constituent pas un système de fichiers général, des montages de disque ou un microkernel.

Un service `vfs` transféré est un nouveau processus avec sa propre table initiale ; les aliases précédents ne sont donc pas transmis. Une évolution future devra définir des capacités de gestion de montage, un backend externalisé, une persistance explicite et une sémantique de reprise avant de prétendre fournir des montages système généraux.
