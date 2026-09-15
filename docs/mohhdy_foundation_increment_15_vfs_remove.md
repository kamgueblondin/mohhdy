# MOHHDY Foundation — Incrément 15 : suppression VFS médiée

> **Statut : livré et vérifié localement.** L’incrément 15 complète l’ensemble minimal lecture-écriture du médiateur VFS par une suppression IPC corrélée, limitée au montage déclaratif `overlay/`.

## Objectif

Après l’écriture sous `overlay/`, une gestion cohérente du cycle de vie des fichiers nécessite une suppression qui ne contourne pas le propriétaire publié du service `vfs`. Le shell possédait déjà la commande de suppression historique du système de fichiers ; cet incrément ajoute une voie VFS explicitement médiée et refuse toute suppression hors du montage écrivable.

| Élément | Contrat livré |
|---|---|
| ABI | `SYS_VFS_OVERLAY_UNLINK = 34` ; `MAX_SYSCALLS = 35`. |
| Autorisation | Seul le PID utilisateur vivant publié pour le nom `vfs` peut appeler la primitive backend. |
| Chemin autorisé | Uniquement un fichier non vide sous `overlay/`, transformé en suffixe relatif avant l’appel noyau. |
| Requête IPC | `OS_IPC_VFS_REMOVE`, chemin borné de 48 octets et `request_id` opaque. |
| Réponse IPC | `OS_IPC_VFS_REMOVE_REPLY`, statut de quatre octets et identifiant de corrélation inchangé. |
| Client | `vfs-remove <chemin>` et `vfs-backend-remove-probe <fichier>`. |

## Politique de montage

`vfsserver` traite une demande `vfs-remove overlay/note.txt` en retirant le préfixe `overlay/`, puis appelle la primitive backend réservée avec `note.txt`. Une cible `initrd/no.txt`, une racine de montage, un chemin global ou un chemin qui contient `..` ne peut jamais atteindre `overlay_unlink`. Les erreurs de l’overlay — notamment un fichier absent — sont renvoyées dans le statut de réponse et rendues visibles par le shell.

La commande de sonde directe doit afficher `vfs-backend-remove-probe denied` depuis le shell, même lorsque `vfsserver` est actif. Elle démontre que le chemin backend n’est pas accessible au client non propriétaire.

## Contrat QEMU

Le contrat VFS démarre sur un disque overlay isolé, vérifie le refus direct et le refus sous `initrd/`, écrit `overlay/note.txt`, le relit via VFS, le supprime via VFS puis exige l’échec de sa relecture. Les assertions existantes de corrélation IPC, message différé, montages virtuels, transfert, révocation et purge sont conservées.

```bash
make test-all          # 196/196
make integration-qemu  # six contrats QEMU
make ci
```

## Limites honnêtes

La suppression reste une opération non transactionnelle sur l’overlay noyau. Elle n’offre ni capability, identité vérifiée, contrôle d’accès par répertoire, suppression récursive, corbeille, audit durable, verrouillage, snapshot atomique ni montages dynamiques. L’initrd est toujours lecture seule, et les syscalls historiques de fichiers demeurent disponibles en dehors de la politique VFS.
