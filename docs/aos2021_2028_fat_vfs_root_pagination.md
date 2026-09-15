# AOS-2021 à AOS-2028 — Pagination native des racines FAT dans le VFS

## Objet

Les montages VFS `fat16/` et `fat32/` étaient déjà disponibles en lecture, statut et listage. Toutefois, une requête `vfs-list-page` chargeait auparavant la première tranche de la racine dans le médiateur Ring 3, puis l’interprétait localement ; l’index demandé n’était donc pas transmis au parcours FAT. Ce macro-lot rend la pagination **native** pour les deux volumes, depuis le parcours de répertoire du noyau jusqu’au médiateur VFS.

| Élément | Contrat livré |
|---|---|
| FAT16 | `fat16_list_root_page(volume, start, out, capacity)` saute `start` entrées logiques, conserve le traitement LFN et remplit un buffer fourni par l’appelant. |
| FAT32 | `fat32_list_root_page(volume, start, out, capacity)` applique le même contrat au parcours de la chaîne de répertoire racine FAT32. |
| Compatibilité | `fat16_list_root` et `fat32_list_root` restent des wrappers de la page zéro ; les appelants historiques gardent leur comportement. |
| ABI Ring 3 | `SYS_FAT16_LIST_PAGE` et `SYS_FAT32_LIST_PAGE` transportent le buffer, sa capacité et l’index de départ vers les primitives noyau. |
| Médiateur VFS | `vfs-list-page fat16/ <start>` et `vfs-list-page fat32/ <start>` délèguent l’index demandé au backend FAT correspondant. |
| Mémoire | Aucun appel à `kmalloc`, `malloc`, `calloc` ou `realloc` n’est introduit : les entrées, LFN et pages utilisent exclusivement des buffers statiques ou appartenant à l’appelant. |

> La pagination compte les **entrées logiques publiées**, et non les slots physiques LFN. Ainsi, demander la page commençant à l’index `1` ignore réellement le premier nom de fichier, y compris lorsque celui-ci est précédé de slots LFN.

## Chemin d’exécution

Les appels système `113` et `114` sont ajoutés après les appels FAT historiques, sans renuméroter l’ABI existante. Ils reçoivent `EBX = tableau os_dirent_t`, `ECX = capacité` et `EDX = index de départ`. Les relais `sys_fat16_list_page` et `sys_fat32_list_page` vérifient le contexte utilisateur et la validité minimale du buffer avant d’appeler les nouvelles primitives de système de fichiers.

Le serveur VFS appelle ces relais uniquement pour les sources FAT. Les chemins `initrd/` et `overlay/` continuent d’emprunter leurs syscalls paginés antérieurs. La réponse VFS conserve sa limite IPC : au plus quatre lignes sont sérialisées, avec `next` lorsque des entrées restent disponibles.

## Couverture de test

| Niveau | Vérification effectuée | Résultat |
|---|---|---|
| Unity FAT16 | Création de deux fichiers, appel avec `start = 1`, retour de `SECOND.TXT`, puis page vide à `start = 2`. | 19/19 réussis. |
| Unity FAT32 | Même scénario sur la racine FAT32 créée sur la fixture en mémoire. | 8/8 réussis. |
| Suite complète | Construction et exécution de toutes les catégories Unity et robustesse. | 481/481 réussis. |
| Noyau i386 | Compilation freestanding complète avec les nouveaux numéros ABI et le dispatch noyau. | Réussie. |
| QEMU | Contrat VFS multi-disque FAT16 maître / FAT32 esclave, listage, lecture, statut, alias, droits et pagination virtuelle. | Réussi. |
| Hygiène | `git diff --check` et recherche des allocations dynamiques dans les fichiers concernés. | Réussis. |

Le contrat QEMU conserve les validations de lecture et de statut sur les fixtures `FATOK.TXT` et `FAT32OK.TXT`. Il garantit ainsi que l’introduction du relais paginé ne modifie pas les droits lecture seule ni les conventions de montage établies.

## Portée et suites

Cette livraison ferme AOS-2021 à AOS-2028 : la couche VFS peut désormais demander une page de racine FAT16 ou FAT32 sans reconstruire artificiellement la pagination après un listage de départ. Le comportement des mutations reste inchangé : les volumes FAT montés dans le VFS sont toujours exposés en lecture seule, tandis que les mutations restent limitées à l’overlay.

## Références

[1] [Contrat FAT16](../kernel/fs/fat16.h)

[2] [Contrat FAT32](../kernel/fs/fat32.h)

[3] [ABI Ring 3 / noyau](../include/os_syscalls.h)

[4] [Dispatch des syscalls](../kernel/syscall/syscall.c)

[5] [Médiateur VFS Ring 3](../userspace/vfs_server.c)

[6] [Tests unitaires FAT16](../tests/unit/kernel/test_fat16.c)

[7] [Tests unitaires FAT32](../tests/unit/kernel/test_fat32.c)

[8] [Contrat QEMU VFS multi-disque](../tests/integration/test_qemu_vfs_service.py)
