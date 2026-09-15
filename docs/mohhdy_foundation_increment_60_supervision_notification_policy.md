# Foundation 60 — Politique locale des notifications de supervision

## Objet

Le lot Foundation 60 complète la souscription IPC de supervision du lot 59 par une **politique locale de filtrage**. Une tâche utilisateur peut conserver toutes les transitions dans son journal tout en ne recevant, dans sa boîte IPC, que les catégories de transitions qui l’intéressent. Un instantané distinct rend cette configuration observable sans provoquer de mutation.

> Le filtre s’applique seulement à la tentative de livraison IPC. Il ne supprime, ne masque et ne modifie jamais une transition du journal de supervision.

| Élément | Contrat livré |
|---|---|
| Configuration du filtre | `SYS_TASK_SUPERVISION_NOTIFY_FILTER = 76` |
| Lecture d’état | `SYS_TASK_SUPERVISION_NOTIFY_STATUS = 77` |
| Plage ABI | `MAX_SYSCALLS = 78` |
| Structure publique | `os_task_supervision_notify_status_t` |
| Erreur de masque | `OS_TASK_BAD_NOTIFY_FILTER = -76` |
| Commandes shell | `task-events-filter`, `task-events-notify-status` |

## Masque de notification

Le masque comporte un bit par action de supervision. Une tâche nouvelle initialise son masque à `OS_TASK_SUPERVISION_NOTIFY_ALL`; tant que la souscription reste désactivée, aucun message n’est toutefois délivré. Activer ou désactiver la souscription via `SYS_TASK_SUPERVISION_NOTIFY` ne réinitialise pas le masque configuré.

| Bit | Constante | Transition IPC admise |
|---:|---|---|
| 0 | `OS_TASK_SUPERVISION_NOTIFY_EXIT` | sortie normale ou terminaison autorisée |
| 1 | `OS_TASK_SUPERVISION_NOTIFY_SUSPEND` | suspension réussie |
| 2 | `OS_TASK_SUPERVISION_NOTIFY_RESUME` | reprise réussie |
| 3 | `OS_TASK_SUPERVISION_NOTIFY_DELEGATE_OUT` | délégation sortante |
| 4 | `OS_TASK_SUPERVISION_NOTIFY_DELEGATE_IN` | délégation entrante |

`SYS_TASK_SUPERVISION_NOTIFY_FILTER` reçoit le masque dans `EBX`. Un masque nul est valide : la souscription peut rester active tout en supprimant toutes les tentatives de livraison. Un bit hors de `OS_TASK_SUPERVISION_NOTIFY_ALL` est refusé par `OS_TASK_BAD_NOTIFY_FILTER`, sans modifier la politique existante.

## Instantané local

`SYS_TASK_SUPERVISION_NOTIFY_STATUS` écrit la structure suivante dans le buffer fourni par l’appelant.

```c
typedef struct {
    uint32_t enabled;
    uint32_t mask;
} os_task_supervision_notify_status_t;
```

`enabled` vaut strictement `0` ou `1`. `mask` est la politique configurée, y compris lorsque `enabled` vaut `0`. L’instantané est local, volatil et non atomique : il ne réserve pas la configuration, ne crée pas de capability et ne garantit pas sa stabilité entre deux syscalls.

## Usage shell

Le shell prend en charge un filtre global, une catégorie isolée ou un filtre vide :

```text
task-events-notify on
task-events-filter delegate-out
task-events-notify-status
# task-events-notify-status ok 1 8
```

Les valeurs disponibles sont `all`, `exit`, `suspend`, `resume`, `delegate-out`, `delegate-in` et `none`. La sortie de `ipc-recv` conserve le format structuré du lot 59 lorsqu’une action admise survient.

## Invariants

| Propriété | Comportement |
|---|---|
| Journal local | Enregistre toutes les transitions valides, indépendamment du masque |
| Résultat de sortie | Inchangé et enregistré même si `exit` est filtré |
| Événement historique de sortie | `OS_IPC_TASK_EVENT` reste indépendant du filtre détaillé |
| Boîte IPC pleine | Perte best-effort du seul message détaillé admis, sans effet sur la transition |
| Désactivation | Arrête immédiatement les notifications et conserve le masque |
| Délégation | Chaque superviseur applique son propre filtre local à son événement entrant ou sortant |

## Validation

| Niveau | Vérification |
|---|---|
| Tâches Unity | État initial, filtre `suspend`, filtre vide, filtre `resume`, bit invalide, désactivation et conservation du journal |
| Syscalls Unity | ABI 76/77, instantané initial, rejet de bit inconnu et absence de livraison pour une action filtrée |
| Suite complète | `make test-all` : **243/243** tests réussis |
| Image i386 | `make clean && make all` réussi |
| QEMU spawn | Souscription active, notification structurée de délégation reçue et contrat de supervision historique préservé |

## Limites explicites

Cette politique ne fournit pas de filtre combiné depuis le shell, de filtrage par PID enfant, de priorité, de quota, de réservation d’emplacement IPC, de garantie ou retransmission, d’accusé de réception, d’ordre global, de lecteur multiple, de souscription distante, de persistance, de transaction, de capability, d’identité vérifiée, d’ACL, de chiffrement ou d’audit de sécurité. La configuration est propre à la tâche appelante et disparaît avec elle.

Un lot ultérieur devrait définir des filtres combinables, puis des sélecteurs par enfant et des garanties de livraison seulement après avoir précisé leur coût, leur concurrence et les droits nécessaires.
