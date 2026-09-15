# Incrément Foundation 32 — Consultation médiée du masque de capacité backend VFS

## Objet

L’incrément 32 rend **observable par le seul propriétaire public vivant de `vfs`** le profil backend effectivement délégué à un PID. Il ne crée aucun droit supplémentaire : il expose, via le médiateur Ring 3, le masque déjà appliqué par le registre pour contrôler les primitives backend.

| Élément | Contrat |
|---|---|
| Commande shell | `vfs-backend-status <pid>` |
| Syscall | `SYS_SERVICE_BACKEND_STATUS` (`48`) |
| ABI | `MAX_SYSCALLS = 49` ; plage `0–48` |
| Requête IPC | `OS_IPC_VFS_BACKEND_STATUS` (`0x56465320`), PID sur 4 octets |
| Réponse IPC | `OS_IPC_VFS_BACKEND_STATUS_REPLY` (`0x56465321`), statut `int32` et masque `uint32` sur 8 octets |
| Masques affichés | `read` (`1`), `mutate` (`2`) et `full` (`3`) |
| Refus | capacité absente, PID invalide ou appelant qui ne possède pas publiquement `vfs` |

Le shell résout d’abord le serveur `vfs`, lui envoie une requête corrélée par `request_id`, et conserve les messages non corrélés dans sa file différée habituelle. `vfsserver` valide le PID, appelle le nouveau syscall comme propriétaire de `vfs`, puis renvoie le statut et le masque. En cas de refus, il normalise le masque retourné à zéro ; le shell affiche `vfs-backend-status: capacite absente ou refusee`.

## Propriété et révocation

Le registre `service_registry_backend_rights()` vérifie que l’appelant possède actuellement le nom public demandé avant de lire une entrée de délégation. Une demande provenant d’un autre PID retourne `OS_SERVICE_NOT_OWNER`; une entrée révoquée, purgée ou inexistante retourne `OS_SERVICE_NOT_FOUND`.

> La consultation est un **instantané de contrôle**, et non une autorisation, une réservation ou une preuve cryptographique. Entre la réponse et toute primitive backend ultérieure, la délégation peut être retirée, purgée ou remplacée.

Les profils restent globaux aux primitives backend VFS : `read` couvre lecture, métadonnées et listage ; `mutate` couvre écriture, suppression, renommage, création et retrait de répertoire ; `full` réunit les deux. Il n’existe toujours ni ACL par chemin, ni identité vérifiée, ni expiration, ni journal persistant, ni transfert de capacité.

## Vérification

Un test Unity vérifie que le propriétaire de `vfs` reçoit le masque mutation, qu’un propriétaire différent est refusé et que la consultation échoue après révocation. `make test-all` valide **214/214** tests.

Le contrat QEMU VFS amorce l’image complète, délègue successivement les profils complet, lecture seule et mutation seule, puis vérifie les sorties `full`, `read` et `mutate`. Il vérifie également le refus de consultation immédiatement après révocation du profil complet, avant de terminer le client de preuve. `make qemu-vfs-service` réussit intégralement.
