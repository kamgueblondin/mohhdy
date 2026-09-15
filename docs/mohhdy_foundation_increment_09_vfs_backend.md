# MOHHDY Foundation — Incrément 09 : backend VFS réservé au service publié

## But

`vfsserver` n’appelle plus `SYS_READFILE` pour les chemins ordinaires du protocole VFS. Il emploie désormais `SYS_VFS_BACKEND_READ = 29`. Le noyau n’autorise ce syscall que si l’appelant Ring 3 est le propriétaire courant du nom de service `vfs` dans le registre.

| Appelant | Résultat |
|---|---|
| Shell ou autre tâche non propriétaire | `OS_VFS_BACKEND_DENIED` |
| Propriétaire vivant de `vfs` | Relais vers le backend initrd/overlay existant |
| `vfs-info` | Reste construit dans `vfsserver` Ring 3, sans backend noyau |

Le contrat QEMU exige d’abord `vfs-backend-probe hello.txt` depuis le shell et observe `denied`. Il lance ensuite `vfsserver`, vérifie `vfs-read hello.txt`, le message IPC différé, `vfs-info` et le nettoyage du service après terminaison.

## Limites

Le mécanisme lie la voie backend dédiée au registre de service volatile, mais ne remplace pas la protection mémoire par capabilities. `SYS_READFILE` reste disponible pour les commandes historiques du shell, le backend initrd/overlay et ATA PIO demeurent noyau, et aucun pilote ou montage général n’est externalisé.
