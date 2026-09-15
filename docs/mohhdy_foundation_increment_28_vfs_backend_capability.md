# Incrément Foundation 28 — Délégation révocable de backend VFS

## Objet

L’incrément 28 introduit une première **capacité backend VFS**. Le propriétaire public de `vfs` peut déléguer à une tâche Ring 3 vivante le droit d’appeler les primitives backend VFS, sans transférer le nom du service ni modifier sa découverte publique.

| Élément | Contrat |
|---|---|
| Commande shell | `vfs-backend-grant <pid>` |
| Requête / réponse | `OS_IPC_VFS_BACKEND_GRANT` / `OS_IPC_VFS_BACKEND_GRANT_REPLY` |
| Charge IPC | PID bénéficiaire sur 4 octets, statut corrélé sur 4 octets |
| Syscall noyau | `SYS_SERVICE_BACKEND_GRANT` (45) |
| ABI | 46 syscalls, indices 0–45 |
| Capacité | Au plus quatre délégations backend simultanées |

`vfsserver` reste le seul détenteur publié de `vfs`. Il reçoit la demande IPC, valide le PID et appelle le nouveau syscall. Le noyau autorise ensuite les syscalls backend VFS pour le propriétaire actuel ou un bénéficiaire dont l’entrée reste liée à ce même propriétaire.

## Révocation et limites

Une délégation est supprimée lors d’un transfert du nom, de son retrait, de la purge du propriétaire ou de la terminaison du bénéficiaire. Ainsi, elle ne survit pas à un changement de mandant ni à la réutilisation ultérieure d’un PID. Le client `vfscapclaim` démontre qu’un programme ne possédant pas `vfs` reste en attente, puis atteint `SYS_VFS_BACKEND_READ` seulement après délégation, alors que `service-find vfs` continue de retourner le PID de `vfsserver`.

> La capacité est locale au noyau, volatile, non persistante et non transférable par son bénéficiaire. Elle couvre l’ensemble des primitives backend VFS, pas un sous-chemin ou une opération particulière. Elle n’est pas une identité vérifiée, un token cryptographique, une ACL POSIX, une sandbox mémoire ou une externalisation du stockage hors noyau.

## Vérification

Le registre possède un test Unity qui couvre la délégation, sa révocation lors du transfert de nom et la purge du PID bénéficiaire. La suite `make test-all` totalise **211/211** tests réussis.

Le contrat QEMU VFS lance `vfscapclaim`, vérifie son attente avant délégation, appelle `vfs-backend-grant <pid>`, attend la lecture backend réussie et confirme que le propriétaire public de `vfs` reste `vfsserver`. Le contrat VFS complet est réussi.
