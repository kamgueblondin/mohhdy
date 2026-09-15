# AOS-2089…2094 — Statistiques VFS formatées par un worker Ring 3

**Statut : livré localement, en validation de non-régression.** Ce macro-lot étend la première délégation VFS inter-processus. Après `vfs-info`, le médiateur `vfsserver` transmet désormais un instantané explicite de ses compteurs à `vfsvirtual`, qui formate la représentation textuelle de `vfs-stats` avant que le médiateur ne relaie la réponse corrélée au client.

> Le worker ne lit aucun état global et ne reçoit aucune capacité de backend. Il obtient seulement quatre entiers sérialisés par le médiateur au moment de la requête ; l’instantané reste volontairement non atomique et volatil.

## Périmètre livré

| Élément | Comportement livré | Invariant |
|---|---|---|
| Médiateur `vfsserver` | Incrémente d’abord `vfs_read_requests`, capture `reads`, `writes`, `removes` et `renames`, puis les transmet au worker. | Une transaction worker active au plus. |
| Worker `vfsvirtual` | Parse `OS_IPC_VFS_WORKER_STATS`, produit quatre lignes de texte bornées et utilise la réponse worker existante. | Aucun syscall VFS, ATA, FAT ou overlay. |
| Réponse publique | `vfsserver` vérifie PID, type et `request_id`, puis reconstruit une réponse publique `OS_IPC_VFS_READ_REPLY`. | La compatibilité de `vfs-stats` est conservée. |
| Repli | Si le service worker est absent, déjà occupé ou refuse l’envoi, le calcul local antérieur sert la réponse. | Aucune dépendance obligatoire au worker. |

## ABI privée bornée

`OS_IPC_VFS_WORKER_STATS` utilise une charge de **16 octets** : quatre entiers 32 bits little-endian dans l’ordre `reads`, `writes`, `removes`, `renames`. Le message garde le `request_id` public du client. Le worker retourne les données formatées par `OS_IPC_VFS_WORKER_READ_REPLY`, dont la taille est déjà plafonnée à 80 octets.

| Contrôle | Garantie |
|---|---|
| Type et taille | Le parseur rejette tout type distinct ou toute charge autre que 16 octets. |
| Valeurs | Les quatre compteurs sont décodés sans pointeur ni allocation. |
| Corrélation | Le médiateur n’accepte qu’une réponse de son PID worker attendu, du type attendu et du `request_id` en cours. |
| Mémoire | Buffers automatiques ou statiques de taille fixe ; aucune allocation dynamique. |

## Validation

```bash
make -s -C tests ../build/./unit/kernel/test_vfs_service
./build/unit/kernel/test_vfs_service
make -s qemu-vfs-service
```

La campagne locale valide **28/28** tests du protocole VFS, dont la sérialisation de l’instantané de statistiques et le refus d’une longueur invalide. Le contrat QEMU démarre `vfsvirtual`, vérifie les traces `vfsserver delegated vfs-stats` et `vfsvirtual format stats`, puis contrôle les valeurs publiques `reads`, `writes`, `removes` et `renames` au début et à la fin de son scénario VFS complet.

## Limites explicites

Les pilotes ATA, FAT16, FAT32 et overlay demeurent dans le noyau. Le worker ne possède ni mémoire partagée, ni pages prêtées, ni DMA utilisateur, ni file parallèle de transactions, ni journal persistant. La délégation actuelle déplace la construction de vues virtuelles et le formatage d’instantanés, mais non les accès physiques aux systèmes de fichiers.

## Références internes

- [Médiateur VFS Ring 3](../userspace/vfs_server.c)
- [Worker VFS virtuel Ring 3](../userspace/vfs_virtual_worker.c)
- [ABI VFS et IPC](../include/os_vfs_service.h)
- [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
- [Régressions Unity VFS](../tests/unit/kernel/test_vfs_service.c)
- [Premier worker VFS AOS-2081…2088](aos2081_2088_vfs_ring3_virtual_worker.md)

---

**Auteur : Manus AI**

**Date de validation : 22 août 2026**
