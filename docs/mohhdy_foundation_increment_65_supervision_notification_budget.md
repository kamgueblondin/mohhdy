# Foundation 65 — Budget local de notifications de supervision

## Objet

Le lot Foundation 65 ajoute à chaque parent utilisateur un **budget local de tentatives de notification détaillée**. Il permet de borner le nombre de messages IPC synthétiques que la supervision peut tenter d’émettre, tout en conservant intégralement le journal local des transitions et les autres mécanismes de résultats enfants.

| Élément | Contrat livré |
|---|---|
| Configuration | `SYS_TASK_SUPERVISION_NOTIFY_BUDGET = 85` |
| État | `SYS_TASK_SUPERVISION_NOTIFY_BUDGET_STATUS = 86` |
| ABI | `MAX_SYSCALLS = 87` |
| Structure | `os_task_supervision_notify_budget_status_t { limit, used }` |
| Shell | `task-events-budget <n|off>`, `task-events-budget-status` |

La valeur `0`, y compris via `task-events-budget off`, signifie **absence de limite**. Toute configuration, limitée ou illimitée, remet `used` à zéro.

## Sémantique

Le budget intervient uniquement après les mécanismes qui qualifient une notification détaillée : souscription active, filtre d’action, watchlist éventuelle ou enfant prioritaire. Si une limite non nulle est atteinte, le noyau ne tente pas d’écrire un nouveau message détaillé dans l’endpoint IPC. Lorsqu’une tentative est admise, `used` est incrémenté avant le dépôt best-effort ; une boîte pleine consomme donc également une unité du budget.

| Cas | Résultat |
|---|---|
| `limit = 0` | Tentatives détaillées non limitées ; `used` reste un compteur d’usage depuis la dernière configuration |
| `limit > 0` et `used < limit` | La tentative détaillée est admise puis comptabilisée |
| `limit > 0` et `used >= limit` | Aucune tentative IPC détaillée supplémentaire ; le journal demeure alimenté |
| Reconfiguration du budget | La nouvelle limite est appliquée localement et `used` revient à zéro |
| Souscription désactivée, filtre ou watchlist non admissible | Aucun usage du budget, car aucune tentative détaillée n’est qualifiée |
| Rediffusion explicite par `task-event-replay` | La rediffusion conserve sa voie existante et n’est pas soumise au budget de souscription initiale |

Le budget ne modifie ni la rétention de `task-events`, ni les générations de journal, ni les résultats de sortie, ni les compteurs `child-exit-count`, ni la télémétrie déjà acquittable des livraisons. Il limite seulement la nouvelle tentative détaillée issue d’une transition de supervision admissible.

## Utilisation shell

```text
$ task-events-budget 2
task-events-budget ok 2
$ task-events-budget-status
task-events-budget-status ok 2 0
```

Après une transition détaillée admise, le statut devient par exemple `task-events-budget-status ok 2 1`. La commande `task-events-budget off` rétablit le mode illimité et remet l’usage local à zéro.

## Validation

La suite `make test-all` exécute **253/253** tests verts. Les nouveaux tests Unity de tâche vérifient l’état initial, le plafonnement, la préservation du journal, la réinitialisation et le retour au mode illimité. Le test syscall valide les ABI 85 et 86 à travers `syscall_handler`.

La reconstruction i386 par `make clean && make all` est réussie. Le scénario `tests/scripts/ci_qemu_spawn.py` configure `task-events-budget 2`, observe l’état `2 0`, réalise une délégation détaillée et confirme la livraison réelle par `ipc-recv`. La consommation exacte `used = 1` est vérifiée par les tests Unity et syscall ; cette séparation évite une frappe redondante fragile sous QEMU TCG. Le smoke QEMU global `make qemu-smoke` est également vert, avec un budget de 240 secondes conservé pour le scénario `spawn`.

## Limites explicites

Ce budget est local au parent, volatile, non atomique et exprimé sur 32 bits. Il ne réserve pas de place dans la FIFO, ne garantit aucune livraison, ne protège pas contre la saturation et ne fournit ni délai, ni renouvellement automatique, ni fenêtre glissante, ni règle par PID, action ou séquence. Il ne bloque ni le journal de supervision, ni les résultats de sortie, ni les rediffusions explicites. Il n’apporte ni persistance, capability, ACL, identité vérifiée, chiffrement, accusé de réception, retransmission automatique ni audit de sécurité.
