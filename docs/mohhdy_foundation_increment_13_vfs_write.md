# MOHHDY Foundation — Incrément 13 : écriture VFS médiée

> **Statut : livré et vérifié localement.** L’incrément 13 étend le médiateur VFS Ring 3 avec une écriture IPC corrélée et bornée vers le seul montage déclaratif écrivable, `overlay/`.

## Objectif

La politique de montages précédente rendait `initrd/` lisible depuis le médiateur VFS, tandis que les écritures restaient limitées aux syscalls de fichiers historiques du shell. Cet incrément fournit une voie explicite de médiation d’écriture, sans prétendre externaliser complètement le backend ATA/overlay qui demeure exécuté dans le noyau.

| Élément | Contrat livré |
|---|---|
| ABI | `SYS_VFS_BACKEND_WRITE = 31` ; `MAX_SYSCALLS = 32`. |
| Réservation | Seul le propriétaire utilisateur vivant du nom `vfs` peut appeler l’écriture backend réservée. |
| Montages | `initrd/` est déclaré lecture seule ; `overlay/` est déclaré lecture-écriture. |
| Requête IPC | `OS_IPC_VFS_WRITE`, exactement 96 octets : chemin de 48 octets, longueur explicite et au plus 44 octets de données. |
| Réponse IPC | `OS_IPC_VFS_WRITE_REPLY`, statut de quatre octets et `request_id` identique à la requête. |
| Client | `vfs-write <chemin> <texte>` et `vfs-backend-write-probe <fichier> <texte>` dans le shell Ring 3. |

## Politique appliquée

`vfsserver` ne transmet jamais le chemin global au backend. Une requête `overlay/note.txt` est contrôlée avec le préfixe de montage puis convertie en suffixe relatif `note.txt`. Une requête sous `initrd/`, sans préfixe ou avec `..` reçoit `OS_VFS_STATUS_NOT_MOUNTED` ou `OS_VFS_STATUS_INVALID` avant toute écriture. Les sources virtuelles `vfs-info` et `vfs-mounts` demeurent construites par le serveur Ring 3 ; `vfs-mounts` annonce désormais les deux droits : `initrd/ ro` et `overlay/ rw`.

L’overlay historique renvoie son nombre d’octets effectivement écrits. Le médiateur le normalise en `OS_VFS_STATUS_OK` pour que le protocole IPC transporte un statut et non une taille ambiguë. Les erreurs négatives de l’overlay sont en revanche propagées sans être déguisées en succès.

## Démonstration

Après `spawn vfsserver`, les scénarios suivants sont observables :

```text
vfs-backend-write-probe note.txt denied
vfs-write initrd/no.txt denied
vfs-write overlay/note.txt vfsok
cat note.txt
```

La première commande prouve que le shell n’accède pas directement à la voie backend réservée. La seconde est refusée par la politique de montage. La troisième produit une réponse VFS corrélée, et la lecture standard de l’overlay affiche ensuite `vfsok`.

## Vérification

Les tests Unity vérifient la requête de 96 octets, le chemin, la charge, le dépassement de 44 octets et le `request_id` de la réponse. Le contrat QEMU VFS utilise un disque overlay isolé, vérifie les refus, l’écriture autorisée et la relecture, puis conserve les contrôles antérieurs de corrélation IPC, transfert et révocation.

```bash
make test-all
make integration-qemu
make ci
```

## Limites honnêtes

Ce mécanisme n’introduit ni capability, ni identité vérifiée, ni journal transactionnel, ni atomicité multi-fichiers, ni verrouillage, ni contrôle d’accès par répertoire. La capacité est de 44 octets par requête IPC et le shell ne prend actuellement qu’un argument texte sans espaces pour `vfs-write`. `SYS_WRITEFILE` historique reste disponible, l’overlay et l’ATA restent noyau, et la réservation est uniquement fondée sur le PID propriétaire volatil du registre `vfs`.
