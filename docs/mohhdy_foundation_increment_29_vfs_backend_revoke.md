# Incrément Foundation 29 — Révocation explicite de capacité backend VFS

## Objet

L’incrément 29 complète la délégation backend VFS par une **révocation explicite**, corrélée et vérifiable. Le serveur propriétaire de `vfs` peut retirer immédiatement le droit backend accordé à un PID, sans céder, retirer ou republier le nom de service.

| Élément | Contrat |
|---|---|
| Commande shell | `vfs-backend-revoke <pid>` |
| Requête / réponse | `OS_IPC_VFS_BACKEND_REVOKE` / `OS_IPC_VFS_BACKEND_REVOKE_REPLY` |
| Charge IPC | PID bénéficiaire sur 4 octets, réponse de statut corrélée sur 4 octets |
| Syscall noyau | `SYS_SERVICE_BACKEND_REVOKE` (46) |
| ABI | 47 syscalls, indices 0–46 |

`vfsserver` parse la demande et appelle le syscall réservé. Le registre n’efface qu’une entrée dont le nom, le mandant propriétaire et le PID bénéficiaire correspondent. Une entrée absente retourne `OS_SERVICE_NOT_FOUND`; un autre propriétaire reçoit `OS_SERVICE_NOT_OWNER`. Dans tous les cas, le nom public `vfs` n’est pas modifié.

## Garanties et limites

Une révocation réussie retire immédiatement l’autorisation évaluée par les syscalls backend VFS. Le client `vfscapclaim` continue de sonder le backend : il annonce d’abord l’attente, puis l’accès autorisé après `vfs-backend-grant`, et enfin `backend revoked` après `vfs-backend-revoke`.

> La révocation reste locale, volatile et par PID. Elle ne fournit pas de listes d’ACL, de droit restreint à un chemin, de durée d’expiration, de journal durable, d’accusé d’événement ou d’identité cryptographiquement vérifiée. Les purges automatiques lors d’un transfert, retrait ou arrêt de tâche demeurent complémentaires à cette commande explicite.

## Vérification

Un test Unity supplémentaire vérifie l’octroi, la révocation explicite, la conservation du PID propriétaire de `vfs`, le refus d’une seconde révocation et le refus d’un mandant étranger. `make test-all` valide **212/212** tests.

Le contrat QEMU VFS lance `vfscapclaim`, attend sa permission après octroi, exécute `vfs-backend-revoke`, exige son diagnostic de perte d’accès puis vérifie à nouveau que `service-find vfs` désigne toujours `vfsserver`. Le contrat VFS complet réussit.
