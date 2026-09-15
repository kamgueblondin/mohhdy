# AOS-2081…2088 — Première délégation VFS vers un worker Ring 3

**Statut : livré localement, en validation de non-régression.** Ce macro-lot fait progresser la migration microkernel sans prétendre déplacer prématurément les pilotes de stockage : le médiateur public `vfsserver` délègue désormais la lecture de la ressource virtuelle `vfs-info` à un second processus Ring 3, `vfsvirtual`.

> Le service public reste `vfs`. Le worker ne reçoit aucune capacité backend et n’accède ni à l’ATA, ni à FAT, ni à l’overlay. Il ne connaît qu’un canal IPC privé, une requête à la fois et une réponse strictement bornée.

## Architecture livrée

| Élément | Rôle | Limite de privilège |
|---|---|---|
| Shell Ring 3 | Résout `vfs` et envoie `OS_IPC_VFS_READ`. | Aucun accès backend direct. |
| `vfsserver` Ring 3 | Conserve les montages, les politiques et l’API publique ; délègue uniquement `vfs-info`. | Une requête worker en vol ; réponse corrélée au client initial. |
| `vfsvirtual` Ring 3 | Publie `vfs-virtual`, construit la valeur statique de `vfs-info` et répond. | Aucun syscall VFS, ATA ou FAT ; buffers automatiques de taille fixe. |
| Noyau | Transporte IPC, tâches et services ; conserve les syscalls de backend physique existants. | Les pilotes stockage ne sont pas encore externalisés. |

Le médiateur recherche explicitement `vfs-virtual` au moment de la demande. Si le worker est absent, déjà sollicité ou si l’envoi échoue, `vfsserver` préserve le comportement historique et sert localement `vfs-info`. Une réponse worker est acceptée seulement si son PID, son type et son `request_id` correspondent à l’unique transaction active. Les réponses malformées sont converties en statut VFS invalide, puis la transaction est libérée.

## ABI privée et bornes

Le canal privé ajoute `OS_IPC_VFS_WORKER_READ` et `OS_IPC_VFS_WORKER_READ_REPLY`. Il ne modifie ni les identifiants ni le format des requêtes publiques `OS_IPC_VFS_READ` et `OS_IPC_VFS_READ_REPLY`.

| Message | Charge utile | Borne | Contrôle |
|---|---:|---:|---|
| Requête worker | Chemin NUL-paddé | 48 octets | Chemin sûr et type dédié. |
| Réponse worker | Statut, taille, données | 88 octets | Taille ≤ 80, `request_id` identique, type dédié. |
| Transaction médiateur | PID worker, PID client, requête | Une entrée statique | Réponse rejetée si PID ou corrélation discordants. |

Les tests Unity couvrent l’encodage/décodage du nouveau canal, les données bornées, une taille invalide et un `request_id` discordant. Le contrat QEMU démarre d’abord le worker, vérifie `service-find vfs-virtual`, puis exige les traces `vfsserver delegated vfs-info` et `vfsvirtual read vfs-info` avant de contrôler le contenu de la réponse publique.

## Validation

```bash
make -s -C userspace all
make -s test-all
make -s qemu-vfs-service
```

La campagne locale valide la compilation freestanding du worker et du médiateur, **485/485** tests globaux, **27/27** tests du protocole VFS et le contrat QEMU VFS complet avec la traversée inter-processus observée.

## Limites explicites

Cette livraison n’externalise pas encore les pilotes ATA, FAT16, FAT32 ou overlay : `vfsserver` utilise toujours les syscalls backend existants pour les montages physiques. Elle ne fournit pas non plus de transfert mémoire partagé, de pages prêtées, de DMA utilisateur, de parallélisme de plusieurs requêtes worker, de persistance de la transaction ou de tolérance à la disparition du worker après soumission. Elle constitue un premier déplacement réel et vérifié de calcul VFS hors du médiateur public, pas une migration microkernel achevée.

Aucune allocation dynamique n’est introduite. Les buffers IPC, le descripteur de transaction et les données du worker sont tous statiques ou automatiques et bornés.

## Références internes

- [Médiateur VFS Ring 3](../userspace/vfs_server.c)
- [Worker VFS virtuel Ring 3](../userspace/vfs_virtual_worker.c)
- [ABI VFS et IPC](../include/os_vfs_service.h)
- [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
- [Régressions Unity VFS](../tests/unit/kernel/test_vfs_service.c)
- [État réel](ETAT_REEL.md)
- [Backlog](todo.md)

---

**Auteur : Manus AI**

**Date de validation : 22 août 2026**
