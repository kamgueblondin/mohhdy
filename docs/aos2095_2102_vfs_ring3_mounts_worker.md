# AOS-2095…2102 — Table de montages VFS formatée séquentiellement en Ring 3

**Statut : livré localement, en validation de non-régression.** Ce macro-lot étend le worker `vfsvirtual` à la troisième vue virtuelle du médiateur : `vfs-mounts`. Le médiateur reste propriétaire de la table de montages et de ses règles ; le worker ne reçoit qu’une entrée de montage à la fois et produit une ligne textuelle bornée.

> La délégation est séquentielle : une seule requête worker demeure en vol. `vfsserver` accumule les lignes validées dans 80 octets, puis publie une réponse VFS unique corrélée au client initial.

## Architecture livrée

| Élément | Responsabilité | Limite explicite |
|---|---|---|
| `vfsserver` | Lit la table locale, soumet une entrée, accumule les réponses et conserve l’API publique. | Pas de parallélisme ni de transfert de backend physique. |
| `vfsvirtual` | Valide le préfixe et le bit de mutabilité, produit `<prefix> ro` ou `<prefix> rw`. | Ne reçoit ni table globale ni capacité ATA/FAT/overlay. |
| Shell Ring 3 | Attend une réponse de lecture VFS jusqu’à 24 tours coopératifs, au lieu du budget IPC général de 8 tours. | L’attente reste strictement bornée et s’applique aux seules lectures VFS. |
| Client VFS | Reçoit une seule réponse `OS_IPC_VFS_READ_REPLY`, comme avant la délégation. | Aucun changement de l’ABI publique. |

## ABI privée et assemblage

`OS_IPC_VFS_WORKER_MOUNT` contient un préfixe NUL-paddé de 48 octets et un indicateur `writable` limité à `0` ou `1`, soit une charge de **49 octets**. Chaque message conserve le `request_id` de la demande VFS publique.

| Contrôle | Comportement |
|---|---|
| Préfixe | Le parseur impose un chemin sûr terminé par `/`. |
| Droit | Toute valeur différente de `0` ou `1` est rejetée. |
| Corrélation | Le médiateur valide PID worker, type de réponse et `request_id`. |
| Capacité | Le buffer final est limité à 80 octets. Si la ligne suivante excéderait cette borne, le médiateur retourne les lignes déjà assemblées. |
| Réponse | Une erreur worker produit une réponse VFS invalide ; un worker absent ou occupé laisse le générateur local historique répondre. |

La dernière règle reproduit volontairement la sémantique antérieure de `vfs-mounts`, qui arrêtait la construction à la première ligne ne tenant plus dans le buffer public.

## Validation

```bash
make -s -C tests ../build/./unit/kernel/test_vfs_service
./build/unit/kernel/test_vfs_service
make -s qemu-vfs-service
```

La campagne locale exécute **29/29** tests du protocole VFS, dont la validation du message de montage et le rejet des tailles ou droits invalides. Le contrat QEMU démarre `vfsvirtual`, exige `vfsserver delegated vfs-mounts`, vérifie au moins quatre traces `vfsvirtual format mount`, puis contrôle les lignes `initrd/ ro`, `overlay/ rw`, `fat16/ ro` et `fat32/ ro`. Il vérifie aussi une table dynamique pleine, en conservant la troncature historique à 80 octets.

## Limites explicites

Les pilotes ATA, FAT16, FAT32 et overlay restent noyau. La transaction de montages est séquentielle, volatile et sans reprise si le worker disparaît après soumission. Aucun partage de pages, DMA utilisateur, transfert de propriété de buffer ou exécution concurrente n’est introduit. Cette livraison externalise le formatage de vues virtuelles, non les opérations de stockage physique.

## Références internes

- [Médiateur VFS Ring 3](../userspace/vfs_server.c)
- [Worker VFS virtuel Ring 3](../userspace/vfs_virtual_worker.c)
- [Shell : attente IPC VFS bornée](../userspace/shell.c)
- [ABI VFS et IPC](../include/os_vfs_service.h)
- [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
- [Régressions Unity VFS](../tests/unit/kernel/test_vfs_service.c)
- [Worker VFS AOS-2081…2088](aos2081_2088_vfs_ring3_virtual_worker.md)
- [Statistiques worker AOS-2089…2094](aos2089_2094_vfs_ring3_stats_worker.md)

---

**Auteur : Manus AI**

**Date de validation : 22 août 2026**
