# Foundation 62 — Télémétrie locale de livraison de supervision

## Objet

Le lot Foundation 62 rend visible le résultat des tentatives **best-effort** de livraison IPC détaillée créées par la souscription de supervision. Chaque parent utilisateur peut consulter et acquitter trois compteurs locaux : les tentatives retenues, les livraisons effectives et les pertes dues à une boîte IPC saturée.

> Ces compteurs décrivent uniquement `OS_IPC_TASK_SUPERVISION_EVENT`. Ils ne comptent ni le journal `task-events`, ni les résultats enfants, ni l’événement historique `OS_IPC_TASK_EVENT` de sortie.

| Élément | Contrat livré |
|---|---|
| Lecture | `SYS_TASK_SUPERVISION_DELIVERY_STATS = 80` |
| Acquittement | `SYS_TASK_SUPERVISION_DELIVERY_STATS_ACK = 81` |
| Plage ABI | `MAX_SYSCALLS = 82` |
| Structure | `os_task_supervision_delivery_stats_t` |
| Shell | `task-events-notify-stats`, `task-events-notify-stats-clear` |

## Sémantique

Une tentative est comptée seulement après que la souscription est active, que le filtre d’action autorise la transition, que la watchlist l’autorise et que le message détaillé a été construit. Le noyau incrémente ensuite `delivered` si l’envoi à l’endpoint IPC réussit, ou `dropped` si l’endpoint est saturé. Ainsi, dans l’état stable, `attempted` correspond à `delivered + dropped`, hors débordement 32 bits.

| Champ | Signification |
|---|---|
| `attempted` | Messages détaillés effectivement proposés au transport IPC après toutes les politiques locales |
| `delivered` | Tentatives acceptées par l’endpoint IPC local |
| `dropped` | Tentatives refusées parce que l’endpoint local est plein |

La lecture est un instantané local et non atomique. L’acquittement met les trois compteurs à zéro et ne modifie ni l’état de souscription, ni le masque d’action, ni la watchlist, ni le journal, ni l’historique de résultats, ni les messages déjà présents dans l’endpoint.

## Usage shell

```text
task-events-notify on
# ... transitions d’enfants ...
task-events-notify-stats
task-events-notify-stats-clear
```

La sortie `task-events-notify-stats ok <attempted> <delivered> <dropped>` permet de distinguer une absence de tentative volontairement filtrée d’une perte de message à saturation. Elle ne lit, ne vide et ne reconnaît aucun message IPC individuel.

## Validation

| Niveau | Vérification |
|---|---|
| Tâches Unity | Livraison réussie, perte lorsque l’endpoint est rempli, invariant des trois compteurs et acquittement |
| Syscalls Unity | ABI 80/81, lecture après transition puis remise à zéro |
| Suite complète | `make test-all` : **247/247** tests réussis |
| Image i386 | `make clean && make all` réussi |
| QEMU spawn | Notification `delegate-out` ciblée, puis `task-events-notify-stats ok 1 1 0` |

## Limites explicites

Les compteurs sont locaux, volatils, non atomiques et sur 32 bits. Ils ne sont ni persistants, ni réservés, ni protégés contre le débordement, ni associés à une séquence, un PID, une action, une fenêtre de temps ou une identité. Ils ne comptent pas les transitions filtrées, les messages applicatifs, les événements de sortie historiques, les erreurs de construction théoriques ni les pertes après qu’un endpoint a accepté un message. Ils ne fournissent ni quota, priorité, avertissement asynchrone, attente, retransmission, accusé de réception, garantie de livraison, ordre global, lecteur multiple, souscription distante, capability, ACL, chiffrement ou audit de sécurité.
