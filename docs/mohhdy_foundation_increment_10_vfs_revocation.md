# MOHHDY Foundation — Incrément 10 : révocation VFS par transfert de propriétaire

## But

Le serveur `vfsserver` accepte désormais une requête IPC `OS_IPC_VFS_GRANT` bornée à un PID positif. Il transfère alors son propre nom `vfs` par `SYS_SERVICE_GRANT`. Comme `SYS_VFS_BACKEND_READ` vérifie le propriétaire **courant** de ce nom à chaque appel, l’ancien serveur perd immédiatement la voie backend et le bénéficiaire `vfsclaim` l’obtient.

| Étape | Garantie vérifiée |
|---|---|
| Shell | `vfs-grant <pid>` envoie une demande IPC au propriétaire publié de `vfs` |
| Médiateur | `vfsserver` déclenche le transfert de son propre nom, sans accepter un nom arbitraire |
| Bénéficiaire | `vfsclaim` lit `hello.txt` avec `SYS_VFS_BACKEND_READ` après transfert |
| Ancien propriétaire | Son arrêt ne retire pas `vfs`, car il n’en est plus propriétaire |
| Nouveau propriétaire | Son arrêt purge `vfs` ; `vfs-read` devient indisponible |

## Validation

Le contrat QEMU démarre `vfsserver`, vérifie une lecture et la source virtuelle, démarre `vfsclaim`, transfère le nom, observe `vfsclaim backend granted`, tue l’ancien serveur et confirme que la recherche désigne toujours `vfsclaim`. Une lecture via le protocole échoue alors explicitement car le nouveau propriétaire n’est pas un serveur de réponses. Après la terminaison de `vfsclaim`, le registre est purgé.

La suite locale contient 187 tests Unity et robustesse, plus six contrats QEMU.

## Limites

Cette révocation est une conséquence du changement de PID dans un registre volatile ; elle ne donne ni token non forgeable, ni identité, ni liste de droits fine. Le bénéficiaire peut appeler le backend réservé mais le stockage initrd/overlay, ATA, les pilotes et la politique générale des montages restent noyau. Le transfert est définitif à ce stade : l’ancien propriétaire n’a pas de révocation unilatérale après délégation.
