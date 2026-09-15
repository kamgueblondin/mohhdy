# Incrément Foundation 24 — Pagination VFS médiée

## Objet

L’incrément 24 ajoute une pagination explicite au listage VFS, sans modifier le message `OS_IPC_VFS_LIST` historique. La nouvelle commande `vfs-list-page <repertoire/> <depart>` interroge une page logique d’un répertoire monté et reçoit soit l’index de continuation, soit la sentinelle `end`.

| Élément | Contrat |
|---|---|
| Commande | `vfs-list-page <repertoire/> <depart>` |
| Requête | `OS_IPC_VFS_LIST_PAGE` (`0x56465312`) |
| Réponse | `OS_IPC_VFS_LIST_PAGE_REPLY` (`0x56465313`) |
| Syscalls initrd / overlay | `41` / `42` |
| ABI | 43 syscalls, indices 0–42 |
| Taille de page visible | Quatre noms au plus |

La requête transmet un chemin de répertoire sûr de 48 octets et un index logique de 32 bits. La réponse corrélée de 80 octets contient le statut, le nombre de noms retournés, l’index suivant ou `0xffffffff`, et 68 octets de noms séparés par des sauts de ligne.

## Routage et curseur

Le serveur `vfs` retire le préfixe de montage avant d’appeler exclusivement le backend initrd ou overlay associé. Les deux nouveaux syscalls restent réservés au propriétaire vivant du service `vfs`; ils valident que le suffixe est un répertoire et retournent au plus cinq entrées. La cinquième permet de déterminer si une page suivante existe.

| Page demandée | Réponse typique initrd | Interprétation |
|---|---|---|
| `vfs-list-page initrd/ 0` | `partiel count 4 next 4` | Lire à partir de 4 |
| `vfs-list-page initrd/ 4` | `ok count 4 next end` | Fin de la liste logique courante |

Le curseur est un index logique volatile de la source observée. Il n’est pas une capacité, un snapshot, une garantie de cohérence ou un jeton persistant. Une mutation entre deux pages peut modifier le contenu associé à un index.

## Vérification et limites

La suite Unity compte désormais **207 tests**, dont un cas corrélé PAGE qui couvre le chemin, le curseur, la réponse de 80 octets et le prochain index. Le contrat QEMU VFS vérifie les pages `0 → 4` puis `4 → end` de l’initrd, avant de poursuivre les tests de montage, mutation, transfert et révocation.

La pagination reste limitée à quatre noms affichables par réponse, sans tri contractuel, métadonnées par entrée, verrouillage, atomicité ni reprise stable après modification. Les fichiers initrd et nœuds overlay sont bornés à 64 entrées internes ; les backends restent noyau et les syscalls historiques directs sont conservés.
