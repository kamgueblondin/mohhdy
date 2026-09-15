# MOHHDY Foundation — Incrément 07 : conservation locale des messages IPC différés

## But

Le shell Ring 3 ne détruit plus une réponse IPC qui ne correspond pas à la requête VFS active. Une file locale fixe de quatre messages conserve par valeur ces messages, puis `ipc-recv` les restitue en ordre FIFO. Avant d’attendre, `vfs-read` consulte aussi cette file afin de consommer immédiatement une réponse VFS déjà corrélée.

| Élément | Contrat livré |
|---|---|
| Stockage | `os_ipc_deferred_t`, quatre `os_ipc_message_t`, sans allocation dynamique |
| Conservation | Tout message reçu par `vfs-read` dont le type ou `request_id` ne correspond pas est placé dans la file locale |
| Récupération | `ipc-recv` lit d’abord la file différée puis l’endpoint noyau |
| Corrélation | `vfs-read` extrait prioritairement le couple type/référence exact |
| Saturation | La cinquième conservation est refusée par `OS_IPC_FULL` et la lecture VFS échoue explicitement ; aucun écrasement silencieux |

Le contrat QEMU injecte `ipc-send 1 deferred` avant `vfs-read hello.txt`. La lecture VFS reste correcte et `ipc-recv` affiche ensuite `deferred`, ce qui prouve que le message concurrent n’a pas été perdu.

## Validation

La suite comporte 186 tests Unity et robustesse, dont quatre nouveaux tests de FIFO, extraction corrélée, conservation des autres messages et saturation. Le contrat VFS QEMU couvre le cas de concurrence observé avec le shell et `vfsserver` Ring 3.

## Limites

Cette file appartient au shell, est volatile et ne fait ni routage inter-services, ni attente bloquante, ni persistance. Elle ne résout pas la concurrence de multiples requêtes depuis des processus distincts, ne fournit pas de politique de priorité et ne peut conserver que quatre messages. Les services non-shell doivent encore gérer leurs messages indépendamment.
