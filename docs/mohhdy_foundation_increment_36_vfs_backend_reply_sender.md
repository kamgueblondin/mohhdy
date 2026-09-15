# Incrément Foundation 36 — Émetteur des réponses de capacité backend VFS

## Objet

L’incrément 36 durcit les commandes de capacité backend VFS en liant une réponse IPC corrélée au PID du médiateur `vfs` résolu avant l’envoi de la requête. Une réponse qui possède le bon type et le bon `request_id`, mais dont `sender_pid` ne correspond pas à ce PID, n’est pas décodée comme une réponse valide.

| Commande protégée | Réponse attendue |
|---|---|
| `vfs-backend-grant` | `OS_IPC_VFS_BACKEND_GRANT_REPLY` |
| `vfs-backend-grant-read` | `OS_IPC_VFS_BACKEND_GRANT_SCOPED_REPLY` |
| `vfs-backend-grant-mutate` | `OS_IPC_VFS_BACKEND_GRANT_SCOPED_REPLY` |
| `vfs-backend-revoke` | `OS_IPC_VFS_BACKEND_REVOKE_REPLY` |
| `vfs-backend-status` | `OS_IPC_VFS_BACKEND_STATUS_REPLY` |
| `vfs-backend-list` | `OS_IPC_VFS_BACKEND_LIST_REPLY` |
| `vfs-backend-observe` | `OS_IPC_VFS_BACKEND_OBSERVE_REPLY` |

Le contrôle est appliqué à la fois aux messages retirés de la file différée et à ceux qui arrivent directement dans la boucle d’attente. Les messages hors séquence restent placés dans la file différée conformément au contrat existant ; un message corrélé du mauvais émetteur ne peut toutefois pas produire un succès, un refus métier ou un masque observé.

> Ce contrôle établit l’origine locale de la réponse par PID, mais ne fournit ni identité cryptographique, ni capability non forgeable, ni protection contre l’usurpation d’un PID par un noyau compromis.

## Vérification

La compilation Ring 3 est réussie et `make test-all` reste vert avec **217/217** tests. Le contrat QEMU VFS complet atteint sa fin attendue, notamment `rc ok 0` après le retrait du service VFS. Les commandes de capacité continuent donc d’accepter les réponses légitimes du médiateur dans l’image réellement amorcée.

Les autres familles de réponses VFS — lecture, écriture, métadonnées, répertoires et montages — ne sont pas incluses dans cet incrément. Leur durcissement doit être traité séparément afin de préserver une couverture explicite et testable de chaque chemin de corrélation.
