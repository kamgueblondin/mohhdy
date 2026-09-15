# MOHHDY Foundation — Incrément 05 : corrélation IPC requête-réponse

## But

Le cinquième incrément Foundation ajoute un identifiant de requête opaque, `request_id`, aux messages IPC. Il permet au client `vfs-read` d’accepter uniquement la réponse VFS qui correspond à la requête qu’il vient d’émettre, plutôt que de supposer que le premier message reçu est le bon.

> Cet incrément reste une **corrélation locale et non bloquante**. Il n’implémente ni un RPC synchrone, ni la conservation d’une réponse non correspondante, ni un contrôle d’accès.

## Contrat ABI

| Élément | Contrat livré |
|---|---|
| `os_ipc_payload_t` | Ajout d’un `uint32_t request_id` opaque, distinct des 96 octets de données |
| `os_ipc_message_t` | Le noyau restitue exactement le même identifiant au destinataire |
| FIFO noyau | La copie, le retrait et l’effacement propagent aussi `request_id` |
| Requête VFS | `os_vfs_make_read_request()` renseigne l’identifiant fourni par le client |
| Réponse VFS | `vfsserver` recopie l’identifiant de la requête dans la réponse |
| Lecture VFS | Le shell génère un identifiant non nul monotone durant sa session et filtre les réponses discordantes pendant trois cessions CPU au plus |

La taille de `OS_IPC_MAX_DATA` reste **96 octets**. Aucun numéro de syscall n’est créé ni modifié : `SYS_IPC_SEND` et `SYS_IPC_RECV` conservent leurs numéros et leur comportement non bloquant.

## Parcours vérifié

`vfs-read hello.txt` résout d’abord le service nommé `vfs`, génère son premier identifiant `1`, envoie la requête et cède au plus trois fois le CPU. Le serveur répond avec le même identifiant. Le shell ne décode le statut et les données que si le type, la longueur et `request_id` sont tous valides ; une réponse d’un autre identifiant est retirée puis ignorée pendant cette fenêtre bornée.

Le contrat QEMU exige la sortie `request 1 data` lors de la lecture VFS de démonstration. Les tests Unity couvrent séparément la copie FIFO de l’identifiant et le rejet du mauvais identifiant par le parseur VFS.

L’extension des structures révèle aussi une exigence de build : la taille de `task_t` embarque un endpoint IPC et les binaires Ring 3 interprètent directement `os_ipc_message_t`. Les Makefiles du noyau et de l’espace utilisateur déclarent donc désormais les en-têtes ABI comme dépendances ; une modification future de ces structures force la reconstruction des objets concernés au lieu de mélanger des dispositions mémoire incompatibles.

## Limites et suite

| Limite | Conséquence |
|---|---|
| Fenêtre de trois cessions | Une réponse lente ou un message discordant peut conduire à un échec local borné |
| Boîte aux lettres de quatre messages | Une réponse ignorée est retirée ; elle n’est pas réinsérée ni routée vers un autre client |
| Identifiant 32 bits local | Il est réinitialisé au redémarrage du shell et peut théoriquement reboucler après `2^32 - 1` requêtes |
| Aucun capability ni authentification | L’identifiant n’empêche pas l’émission de messages ou l’usurpation de contenu par une tâche malveillante |
| Backend VFS noyau | La logique initrd/overlay/ATA et l’ABI directe `SYS_READFILE` ne sont pas externalisées |

Les étapes suivantes réalistes sont une table de requêtes en attente côté client, la conservation ou le dispatch des messages non correspondants et, séparément, des droits de publication/découverte. Elles devront rester compatibles avec les limites de mémoire et l’exécution freestanding i386.

## Fichiers concernés

| Fichier | Rôle |
|---|---|
| `include/os_syscalls.h` | ABI des messages IPC avec `request_id` |
| `kernel/ipc.c` | Propagation FIFO de l’identifiant |
| `include/os_vfs_service.h` | Encodage et vérification du contrat VFS corrélé |
| `userspace/vfs_server.c` | Recopie de l’identifiant dans la réponse |
| `userspace/shell.c` | Génération, filtrage et affichage de l’identifiant |
| `tests/unit/kernel/test_ipc.c` | Preuve de propagation FIFO |
| `tests/unit/kernel/test_vfs_service.c` | Preuve de rejet d’une réponse discordante |
| `tests/integration/test_qemu_vfs_service.py` | Contrat QEMU VFS, corrélation et cycle de vie |
| `Makefile` et `userspace/Makefile` | Reconstruction automatique après changement d’ABI partagée |
