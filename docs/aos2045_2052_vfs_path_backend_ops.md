# AOS-2045 à AOS-2052 — Table d’opérations des backends de chemins VFS

## Objet

Le médiateur VFS Ring 3 sélectionnait auparavant la primitive de chaque source au moyen de branches imbriquées dans les chemins de lecture, statut, listage, pagination et mutation. Le fonctionnement était correct, mais chaque nouvelle source devait modifier plusieurs sélecteurs indépendants. Ce macro-lot **externalise le dispatch** dans une table statique d’opérations backend.

| Élément | Contrat livré |
|---|---|
| Table statique | Chaque source déclare son identifiant, ses callbacks de lecture, statut, liste et liste paginée. |
| Mutabilité explicite | Les callbacks `write`, `mkdir`, `rmdir`, `remove` et `rename` sont non nuls uniquement pour `overlay`. |
| Sources couvertes | `initrd`, `overlay`, `fat16` et `fat32` sont représentées dans une table unique. |
| Résolution | Le médiateur cherche les opérations depuis l’identifiant de source du montage, puis transmet seulement le suffixe de chemin relatif. |
| Échec sûr | Une source inconnue ou une opération absente retourne le statut de montage non disponible ; aucune primitive par défaut n’est exécutée. |
| Mémoire | Les opérations, montages et buffers restent statiques ou fournis par l’appelant ; aucune allocation dynamique n’est introduite. |

> La table constitue une frontière de backend explicite dans le médiateur Ring 3. Ajouter une nouvelle source ne requiert plus de réécrire les branches de dispatch de chaque opération de chemins.

## Mise en œuvre

La structure `vfs_backend_ops_t` centralise les signatures des dix opérations pertinentes. `vfs_backend_ops_for()` effectue une recherche bornée dans les quatre entrées statiques. Les fonctions de haut niveau (`read_mounted_backend`, `stat_mounted_backend`, listage, pagination et mutations) ne connaissent plus les familles de source ; elles obtiennent l’opération depuis la table puis appliquent les mêmes contrôles de préfixe et de chemin relatif.

La politique d’écriture n’est pas relâchée. Les montages initrd et FAT ont leurs callbacks de mutation à zéro, tandis que la seule entrée overlay publie les cinq opérations mutable. Un alias dynamique `work/` vers overlay hérite donc du même comportement sans branche spécifique.

## Validation

| Niveau | Vérification | Résultat |
|---|---|---|
| Compilation Ring 3 | `vfsserver` construit avec les callbacks typés et la table statique. | Réussi. |
| QEMU VFS | Lecture, statut et liste des sources initrd, FAT16 et FAT32 ; alias dynamiques des quatre sources. | Réussi. |
| QEMU overlay dynamique | `work/` écrit, lit, renomme, relit, supprime puis liste un fichier avant le retrait du montage. | Réussi. |
| QEMU politique lecture seule | Les mutations à travers initrd restent refusées. | Réussi. |
| Suite complète | `make -s test-all`. | 483/483 réussis. |
| Noyau i386 et hygiène | `make -s kernel-only`, `git diff --check` et absence d’allocation dynamique dans le médiateur. | Réussis. |

## Portée

L’externalisation concerne le dispatch local Ring 3, non la migration d’un backend vers un service processus séparé. Elle réduit le couplage du médiateur et prépare cette évolution sans modifier l’ABI VFS, les droits de capacité ni le format des messages IPC.

## Références

[1] [Médiateur VFS et table d’opérations](../userspace/vfs_server.c)

[2] [Contrat QEMU multi-disque et alias dynamiques](../tests/integration/test_qemu_vfs_service.py)

[3] [État réel de l’architecture VFS](ETAT_REEL.md)
