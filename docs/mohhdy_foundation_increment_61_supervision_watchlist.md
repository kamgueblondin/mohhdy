# Foundation 61 — Watchlist locale de supervision

## Objet

Le lot Foundation 61 complète la souscription IPC et le filtre par action par un **ciblage local d’enfants directs**. Un parent peut limiter les notifications détaillées de supervision aux PID retenus dans une watchlist bornée, tandis que le journal de supervision, les résultats de sortie et l’événement historique de sortie continuent de couvrir toutes les transitions.

> La watchlist restreint uniquement la tentative de livraison de `OS_IPC_TASK_SUPERVISION_EVENT`. Elle ne masque ni ne supprime les transitions retenues dans `task-events`.

| Élément | Contrat livré |
|---|---|
| Mise à jour | `SYS_TASK_SUPERVISION_WATCH = 78` |
| Lecture d’état | `SYS_TASK_SUPERVISION_WATCH_STATUS = 79` |
| Plage ABI | `MAX_SYSCALLS = 80` |
| Capacité | `OS_TASK_SUPERVISION_WATCH_CAPACITY = 4` |
| Erreurs | `OS_TASK_BAD_WATCH = -77`, `OS_TASK_WATCH_FULL = -78`, `OS_TASK_NO_SUPERVISION_WATCH = -79` |
| Shell | `task-events-watch`, `task-events-unwatch`, `task-events-watch-clear`, `task-events-watch-status` |

## Sémantique

Par défaut, `enabled = 0` et aucun PID n’est retenu : tous les enfants directs restent admissibles aux notifications détaillées, sous réserve de la souscription et du filtre par action. L’ajout d’un enfant direct vivant à la liste active le mode watchlist ; dès lors, seuls les PID retenus autorisent la livraison IPC détaillée. Ajouter deux fois le même enfant est idempotent et conserve le compteur.

| Opération | Arguments | Résultat |
|---|---|---|
| Ajouter | `EBX = PID enfant`, `ECX = 1` | Ajoute un enfant direct vivant ; retourne le compteur retenu |
| Retirer | `EBX = PID enfant`, `ECX = 0` | Retire le PID ; retourne le compteur restant |
| Effacer/désactiver | `EBX = 0`, `ECX = 0` | Vide la liste et rétablit l’admissibilité de tous les enfants directs |
| Lire l’état | `EBX = os_task_supervision_watch_status_t*` | Retourne `enabled`, `count` et les PID conservés |

Un PID absent retourne `OS_TASK_NOT_FOUND`, un processus qui n’est pas un enfant direct retourne `OS_TASK_NOT_CHILD`, une valeur `ECX` différente de `0` ou `1` retourne `OS_TASK_BAD_WATCH`, une cinquième entrée retourne `OS_TASK_WATCH_FULL` et le retrait d’un PID non retenu retourne `OS_TASK_NO_SUPERVISION_WATCH`.

## Cycle de vie et délégation

Le noyau enregistre et tente d’émettre l’événement détaillé avant de retirer automatiquement le PID d’une watchlist lorsque l’enfant sort ou est tué. Lors d’une délégation, l’ancien parent peut recevoir son événement `delegate-out` si l’enfant est watché, puis le PID est retiré de sa liste avant le changement de `parent_pid`. Le nouveau superviseur n’hérite pas de la watchlist et doit explicitement surveiller son nouvel enfant s’il souhaite le cibler.

| Élément | Invariant |
|---|---|
| Journal `task-events` | Toutes les transitions restent enregistrées, watchées ou non |
| Résultats enfants | Indépendants de la watchlist |
| Événement historique de sortie | `OS_IPC_TASK_EVENT` indépendant de la watchlist |
| Sortie / kill | PID retiré automatiquement après la notification détaillée potentielle |
| Délégation sortante | PID retiré de la liste de l’ancien parent après `delegate-out` |
| Boîte IPC saturée | Le message détaillé peut être perdu sans retarder la transition ou le nettoyage |

## Usage shell

```text
task-events-watch 5
task-events-notify on
# ... transition de l’enfant 5 ...
ipc-recv
task-events-unwatch 5
task-events-watch-clear
task-events-watch-status
```

La sortie `task-events-watch-status ok <enabled> <count> [pid...]` expose l’instantané local. Elle ne réserve ni les PID ni leurs relations de parenté entre deux syscalls.

## Validation

| Niveau | Vérification |
|---|---|
| Tâches Unity | Contrôle d’enfant direct, filtre détaillé par PID, journal complet, retrait explicite et purge automatique à la sortie |
| Syscalls Unity | ABI 78/79, état initial, rejet d’un non-enfant, ajout, notification reçue et remise à zéro |
| Suite complète | `make test-all` : **245/245** tests réussis |
| Image i386 | `make clean && make all` réussi |
| QEMU spawn | Ajout de l’enfant délégué à la watchlist, notification `delegate-out` reçue, puis contrat historique intégral |

## Limites explicites

La watchlist est locale, volatile, non atomique et limitée à quatre PID. Elle ne propose ni filtre combiné par PID dans le shell, ni priorité, durée, quota, réservation, attente bloquante, garantie ou retransmission, accusé de réception, ordre global, lecteur multiple, souscription distante, héritage lors d’une délégation, persistance, capability, identité vérifiée, ACL, chiffrement ou audit de sécurité. Elle ne remplace pas le journal local, et aucune entrée ne survit à la fin de la tâche.
