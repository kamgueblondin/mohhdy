# AOS-2127…2132 — Pagination `vfs-mounts` formatée par worker Ring 3

**Statut : livré localement, validation complète en cours.** Ce macro-lot étend l’isolation déjà appliquée au contenu complet de `vfs-mounts` aux pages publiques produites par `vfs-list-page vfs-mounts <départ>`. Le worker `vfsvirtual` formate les lignes de montage séquentiellement ; le médiateur reste propriétaire de la table, de l’index de départ, de la génération et du protocole public de pagination.

> Le worker produit seulement une ligne `prefixe ro|rw` à la fois. Il ne choisit ni le montage à exposer, ni la page suivante, ni la génération publique.

## Répartition des responsabilités

| Responsabilité | `vfsserver` | `vfsvirtual` |
|---|---:|---:|
| Table de montages et source backend | Oui | Non |
| Validation du chemin `vfs-mounts` et de `start` | Oui | Non |
| Choix des entrées de la page | Oui | Non |
| Formatage de chaque ligne `prefixe ro|rw` | Non | Oui |
| Agrégation dans les 68 octets publics | Oui | Non |
| `count`, `next_start`, statut tronqué et génération | Oui | Non |
| Repli local si worker absent, occupé, silencieux ou erreur IPC | Oui | Non |

## Protocole et bornes

Aucun type IPC ni aucune réponse publique ne sont ajoutés. Le médiateur réutilise `OS_IPC_VFS_WORKER_MOUNT` déjà employé par la lecture complète de `vfs-mounts`. Il soumet l’entrée au `start` demandé, accumule les lignes répondant au même `request_id`, puis soumet l’entrée suivante jusqu’à quatre lignes ou jusqu’à la fin de la table.

La réponse publique reste `OS_IPC_VFS_LIST_PAGE_REPLY` avec au plus 68 octets de texte, quatre entrées, un `next_start` ou la sentinelle `end`. Une ligne qui ne pourrait pas tenir déclenche le même repli local borné ; la politique historique de troncature demeure dans le médiateur.

Le shell réserve désormais 24 tours coopératifs à `vfs-list-page`, comme à `vfs-read`, pour permettre les quatre échanges privés séquentiels d’une page déléguée. Les pages physiques gardent exactement la même API et la même sortie utilisateur.

## Contrat QEMU

`make qemu-vfs-service` vérifie dans une image i386 réelle :

| Page | Résultat public attendu | Preuve worker attendue |
|---|---|---|
| `vfs-list-page vfs-mounts 0` | `partiel count 4 next 4` | `vfsserver delegated mount page` et quatre `vfsvirtual format mount` |
| `vfs-list-page vfs-mounts 4` | `ok count 4 next end` | `vfsserver delegated mount page` et quatre formats worker |
| `vfs-list-observe vfs-mounts` | Génération fournie par médiateur | Aucun déplacement de politique vers le worker |

Les scénarios de retrait, redécouverte, transaction en vol, expiration d’un worker silencieux, FAT16, FAT32, capacités et overlay restent exécutés dans le même contrat.

## Limites explicites

`vfs-list-observe` reste local, car il publie la génération et applique la règle d’obsolescence du médiateur. Le worker n’accède à aucun backend ni à la table de montages. La pagination est une vue volatile, non atomique et sans réservation : une mutation de table peut toujours rendre l’instantané suivant différent.

Aucune allocation dynamique ni migration ATA/FAT/overlay hors noyau n’est introduite.

## Validation

```bash
make -s -C userspace vfsserver shell vfsvirtual
make -s qemu-vfs-service
```

Le contrat QEMU passe avec la délégation des deux pages `vfs-mounts` et les comportements VFS existants.

## Références internes

- [Médiateur VFS](../userspace/vfs_server.c)
- [Worker virtuel Ring 3](../userspace/vfs_virtual_worker.c)
- [Shell Ring 3](../userspace/shell.c)
- [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
- [Pagination worker précédente AOS-2095…2102](aos2095_2102_vfs_ring3_mounts_worker.md)

---

**Auteur : Manus AI**

**Date de validation : 22 août 2026**
