# Incrément Foundation 26 — Création de répertoires VFS médiée

## Objet

L’incrément 26 ajoute `vfs-mkdir <chemin>`, une création de répertoire médiée par `vfsserver` Ring 3. L’opération est corrélée, refuse les chemins hors montage et ne cible que les montages dont la source déclarée est l’overlay.

| Élément | Contrat |
|---|---|
| Commande shell | `vfs-mkdir <chemin>` |
| Requête / réponse | `OS_IPC_VFS_MKDIR` / `OS_IPC_VFS_MKDIR_REPLY` |
| Requête | Chemin sûr, NUL-terminé, 48 octets fixes |
| Réponse | Statut corrélé de 4 octets |
| Syscall backend | `SYS_VFS_OVERLAY_MKDIR` (43) |
| ABI | 44 syscalls, indices 0–43 |

Le serveur compare le chemin à sa table locale de montages. Un préfixe initrd ou un chemin absent est refusé par la politique VFS ; un préfixe `overlay/` ou alias overlay transmet uniquement le suffixe relatif au syscall backend réservé au propriétaire vivant de `vfs`.

## Sémantique

`overlay_mkdir` conserve ses garanties existantes : le chemin ne peut pas désigner la racine, ne peut pas exister, ne peut pas masquer un fichier initrd et son parent doit être un répertoire overlay réel. La mutation est sauvegardée dans l’overlay ATA. Une création réussie incrémente la génération volatile de page VFS, de sorte qu’une continuation `vfs-list-observe` précédente est explicitement obsolète.

Le contrat QEMU crée `overlay/newdir`, vérifie la réponse corrélée et confirme que l’entrée apparaît dans `vfs-list overlay/` puis via l’alias overlay `work/`. Les listings ultérieurs sont ajustés pour compter ce répertoire persistant de scénario.

## Vérification et limites

La suite Unity totalise **209 tests**, avec un aller-retour MKDIR borné et corrélé. Le contrat QEMU VFS complet est vert ; sa cadence de frappe est portée à 1,10 seconde et ses vérifications sensibles attendent des résultats fonctionnels plutôt que des traces serveur génériques, ce qui évite les alias parasites et les faux négatifs PS/2.

L’incrément n’ajoute pas de suppression de répertoire dédiée, de transaction, de permissions POSIX, de capability, de snapshot ni d’externalisation du backend noyau. Les montages, le registre et la génération de page restent volatils.
