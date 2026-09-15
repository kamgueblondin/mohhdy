# Foundation 59 — Notifications IPC de supervision locale

## Objet

Le lot Foundation 59 ajoute une **souscription IPC locale et explicite** aux transitions de supervision d’une tâche utilisateur. Il évite au parent de devoir interroger uniquement le journal via `task-events` lorsqu’il souhaite réagir à une transition, tout en préservant le journal, les résultats de sortie et l’événement historique de sortie déjà publiés.

> La notification est une livraison best-effort d’un événement déjà retenu dans le journal de supervision local. Elle ne constitue ni une preuve d’audit ni une garantie de livraison.

| Élément | Contrat livré |
|---|---|
| Syscall | `SYS_TASK_SUPERVISION_NOTIFY = 75` |
| Plage ABI | `MAX_SYSCALLS = 76` |
| Commande shell | `task-events-notify <on|off>` |
| Erreur de contrat | `OS_TASK_BAD_NOTIFY = -75` |
| Type IPC noyau | `OS_IPC_TASK_SUPERVISION_EVENT = 0x54415302` |
| Taille de charge | `OS_TASK_SUPERVISION_EVENT_SIZE = 24` octets |

## Contrat d’activation

`SYS_TASK_SUPERVISION_NOTIFY` reçoit dans `EBX` une valeur strictement égale à `0` ou `1`. La valeur `1` active la réception de notifications pour la tâche Ring 3 appelante ; `0` la désactive. L’opération est idempotente et retourne l’état appliqué. Toute autre valeur retourne `OS_TASK_BAD_NOTIFY`.

La souscription est stockée dans l’état privé de la tâche. Elle est donc **locale, volatile et automatiquement perdue** lorsque la tâche disparaît. Elle ne crée aucun droit sur les tâches enfants et n’influe ni sur la filiation, ni sur la capacité, ni sur l’ordonnancement, ni sur le journal lui-même.

## Transitions notifiées

Lorsqu’une souscription est active, le noyau construit d’abord l’entrée du journal local, attribue sa séquence et met à jour sa génération. Il tente ensuite de déposer la notification dans la boîte IPC de cette même tâche.

| Action retenue | Moment d’émission | `related_pid` | `detail` |
|---|---|---:|---:|
| `OS_TASK_SUPERVISION_EXIT` | sortie normale ou terminaison autorisée d’un enfant direct | `0` | raison `exited` ou `killed` |
| `OS_TASK_SUPERVISION_SUSPEND` | suspension réussie d’un enfant direct | `0` | `0` |
| `OS_TASK_SUPERVISION_RESUME` | reprise réussie d’un enfant direct | `0` | `0` |
| `OS_TASK_SUPERVISION_DELEGATE_OUT` | enfant cédé par le superviseur sortant | PID superviseur entrant | `0` |
| `OS_TASK_SUPERVISION_DELEGATE_IN` | enfant reçu par le superviseur entrant | PID superviseur sortant | `0` |

La délégation peut donc déclencher une notification distincte chez chacun des deux superviseurs, à condition que chacun ait activé sa propre souscription locale.

## Message IPC

Le noyau envoie le message par l’endpoint IPC existant avec `sender_pid = 0` et `request_id = 0`. Les 24 octets de charge, en little-endian, se présentent comme suit.

| Décalage | Taille | Champ |
|---:|---:|---|
| 0 | 4 | `sequence` |
| 4 | 4 | `action` |
| 8 | 4 | `child_pid` signé |
| 12 | 4 | `related_pid` signé |
| 16 | 4 | `detail` |
| 20 | 4 | `ticks` |

Les helpers publics `os_task_make_supervision_event()` et `os_task_parse_supervision_event()` encadrent ce format. Le shell décode automatiquement le message via `ipc-recv`, par exemple :

```text
task-supervision-event 4 delegate-out 5 4 0 123
```

## Compatibilité et comportement best-effort

Le message historique `OS_IPC_TASK_EVENT` des sorties d’enfants reste inchangé. Avec une souscription active, une sortie peut donc produire à la fois une notification détaillée de supervision et l’événement historique de sortie, chacun dans l’ordre imposé par le chemin noyau.

La boîte IPC conserve sa capacité de quatre messages. Si elle est pleine, l’envoi de notification échoue silencieusement pour ne jamais retarder, annuler ou modifier la transition de supervision, le journal, le résultat enfant, le réveil d’un waiter, la réattribution ou le retrait de tâche.

## Validation

| Niveau | Vérification |
|---|---|
| Tâches Unity | Souscription, refus de valeur invalide, notifications de suspension, reprise, délégation et sortie, désabonnement et coexistence avec l’événement historique |
| Syscalls Unity | Dispatch ABI `75`, activation, désactivation, refus de `2` et réception IPC de la suspension |
| Image i386 | `make clean && make all` réussi |
| QEMU spawn | Activation, délégation, `ipc-recv` de `delegate-out`, puis contrat historique intégral |
| Suite complète | `make test-all` : **241/241** tests réussis |

## Limites explicites

Cette livraison ne fournit **aucune** attente IPC bloquante, garantie de livraison, retransmission, accusé de réception, curseur, filtre par enfant ou par action, ordre total inter-tâches, souscription à distance, lecteur multiple, persistance, transaction, quota, capability, identité vérifiée, ACL, chiffrement ou audit sécurisé. Le journal et la notification sont locaux, volatils et non atomiques ; le message peut être perdu quand l’endpoint est saturé tandis que la transition reste effective.

La notification prépare une supervision réactive minimale. Une évolution ultérieure devrait définir, avec tests de concurrence, une politique de filtre et de rejet, des accusés de réception, puis éventuellement une persistance et des autorisations explicites avant toute prétention d’audit ou de sécurité.
