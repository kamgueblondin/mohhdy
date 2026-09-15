# Incrément Foundation 33 — Inventaire médié des délégations backend VFS

## Objet

L’incrément 33 permet au propriétaire public vivant de `vfs` de consulter, en une requête, **l’ensemble borné des délégations backend actives**. Il complète la consultation unitaire de l’incrément 32 sans conférer de droit supplémentaire, ni exposer directement le registre noyau au shell.

| Élément | Contrat |
|---|---|
| Commande shell | `vfs-backend-list` |
| Syscall | `SYS_SERVICE_BACKEND_LIST` (`49`) |
| ABI | `MAX_SYSCALLS = 50` ; plage `0–49` |
| Requête IPC | `OS_IPC_VFS_BACKEND_LIST` (`0x56465322`), charge nulle |
| Réponse IPC | `OS_IPC_VFS_BACKEND_LIST_REPLY` (`0x56465323`), 40 octets |
| Réponse | statut `int32`, compteur `uint32`, puis quatre couples PID `int32` / masque `uint32` au plus |
| Bornage | quatre délégations, conformément à `OS_SERVICE_BACKEND_CAPACITY` |

Le shell résout `vfs`, transmet une requête corrélée par `request_id` et conserve les messages hors séquence dans sa file différée bornée. `vfsserver` appelle le syscall sous l’identité de son propriétaire public, puis retourne les couples actifs dans une réponse de taille fixe. Le shell affiche d’abord `vfs-backend-list ok count <n>`, puis une ligne stable par entrée : `pid <pid> rights <read|mutate|full>`.

## Contrôle et cohérence

Le registre efface intégralement la structure de sortie avant de valider le nom et le propriétaire. Un appel qui ne provient pas du propriétaire courant de `vfs` retourne `OS_SERVICE_NOT_OWNER` avec une sortie vide. Une entrée retirée par `vfs-backend-revoke`, par terminaison du bénéficiaire, par transfert ou par purge du propriétaire ne figure plus dans l’instantané suivant.

> L’inventaire ne délivre ni capability, ni jeton, ni réservation. C’est un instantané non atomique : une délégation peut changer immédiatement après la réponse et doit être contrôlée de nouveau par chaque primitive backend.

Le codec refuse un compteur supérieur à quatre, un PID nul ou négatif, un masque vide ou hors contrat, ainsi qu’une donnée non nulle au-delà du compteur déclaré. Une réponse d’erreur impose un compteur nul et des entrées vides.

## Vérification

Les tests Unity couvrent le registre — profils `read`, `mutate` et `full`, refus par un autre propriétaire et mise à jour après révocation — ainsi que le codec IPC — requête nulle, corrélation, réponse d’erreur vide et rejet de données résiduelles. `make test-all` valide **216/216** tests.

Le contrat QEMU VFS amorce l’image complète. Il vérifie l’inventaire contenant le profil complet, l’inventaire vide après révocation, puis les inventaires lecture seule et mutation seule avant la terminaison des clients de preuve. La relance bornée de `vfs-rmdir` a été alignée sur son marqueur de succès réel afin de tolérer une double frappe PS/2 transitoire sans masquer une erreur VFS.
