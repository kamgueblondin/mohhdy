# MOHHDY Foundation — Incrément 11 : registre de montages VFS

> **Statut : livré et vérifié localement.** Cet incrément ajoute une politique de chemin au médiateur `vfsserver` Ring 3. Elle réduit la surface du backend réservé sans prétendre externaliser le stockage hors du noyau.

## Objectif

Le propriétaire du service `vfs` devait jusqu’ici pouvoir relayer tout chemin valide vers `SYS_VFS_BACKEND_READ`. L’incrément 11 introduit une liste bornée de montages que le médiateur déclare à son démarrage. Une lecture backend n’est autorisée que lorsqu’elle appartient à l’un de ces préfixes ; le backend reçoit alors uniquement le chemin relatif au montage.

| Élément | Comportement livré |
|---|---|
| Liste de montages | `vfsserver` déclare actuellement `initrd/` au démarrage. |
| Découverte | `vfs-read vfs-mounts` retourne la liste virtuelle `initrd/`. |
| Lecture autorisée | `vfs-read initrd/hello.txt` délègue `hello.txt` au backend réservé. |
| Refus de politique | `vfs-read hello.txt` répond `OS_VFS_STATUS_NOT_MOUNTED` et affiche `vfs-read: chemin hors montage`. |
| Sources virtuelles | `vfs-info` et `vfs-mounts` restent entièrement produites en Ring 3. |

## Sémantique du préfixe

Le helper partagé `os_vfs_match_mount()` ne reconnaît qu’un préfixe de répertoire terminé par `/`. Le chemin demandé doit être sûr selon la politique existante — NUL-terminé dans `OS_VFS_PATH_MAX`, non vide et sans `..` — et doit contenir un suffixe non vide après le préfixe.

| Chemin | Montage `initrd/` | Résultat |
|---|---:|---|
| `initrd/hello.txt` | Oui | Le backend reçoit `hello.txt`. |
| `initrd/` | Non | La racine du montage n’est pas une lecture de fichier. |
| `initrdx/hello.txt` | Non | La frontière de composant n’est pas franchie. |
| `hello.txt` | Non | Chemin global hors du montage déclaré. |
| `initrd/../secret` | Non | Chemin non sûr, rejeté avant le backend. |

La comparaison est disponible dans `include/os_vfs_service.h`, ce qui verrouille la convention utilisée par le médiateur et les tests sans introduire un nouvel appel système. Le statut négatif `OS_VFS_STATUS_NOT_MOUNTED` vaut `-61` et se distingue des erreurs de format (`-60`) et du refus noyau `OS_VFS_BACKEND_DENIED`.

## Sécurité et continuité de service

La politique s’exécute dans `vfsserver`, donc elle suit le propriétaire courant du nom `vfs`. Après `vfs-grant`, le bénéficiaire reçoit toujours le droit backend noyau comme dans l’incrément 10 ; il ne devient pas automatiquement un serveur implémentant cette politique. Cette distinction est volontaire : le registre de services reste une association nom–PID volatile, sans capability ni identité authentifiée.

Le contrat QEMU vérifie successivement l’annonce du montage, la lecture de `vfs-mounts`, le refus d’un chemin hors montage, la lecture de `initrd/hello.txt`, la conservation d’un message IPC concurrent, la source `vfs-info`, le transfert à `vfsclaim`, la révocation de l’ancien serveur et la purge du nouveau propriétaire.

## Limites honnêtes

Le registre est **statique** dans le binaire `vfsserver` et ne constitue pas encore une API de montage dynamique. Il ne gère ni démontage, ni plusieurs serveurs de systèmes de fichiers, ni résolution de chemins absolus, ni droits par utilisateur, ni persistance. Le backend initrd/overlay/ATA est toujours exécuté dans le noyau et `SYS_READFILE` historique reste disponible aux programmes existants. La prochaine étape réaliste est d’associer la politique de montage à un descripteur de serveur ou à une capability non forgeable avant de déplacer le backend hors de Ring 0.

## Vérification

```bash
make test-all
make integration-qemu
make ci
```

Les tests unitaires du protocole vérifient l’acceptation du préfixe, le suffixe relatif et les refus de racine, de frontière et de chemin non sûr. Le contrat QEMU VFS couvre le comportement visible dans une image i386 bootée.
