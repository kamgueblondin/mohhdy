# Lot Foundation 50 — Suspension locale d’enfant

## Objet

Ce lot ajoute un contrôle de cycle de vie léger pour le **parent direct** : il peut suspendre une tâche enfant prête, l’observer comme suspendue, puis la rendre à nouveau planifiable. La tâche reste allouée, conserve son PID, sa filiation, son contexte et sa place dans la capacité globale ; elle n’est ni terminée ni réattribuée.

| Élément | Contrat livré |
|---|---|
| ABI ajoutée | `SYS_TASK_SUSPEND = 62`, `SYS_TASK_RESUME = 63`, `MAX_SYSCALLS = 64` |
| État public | `OS_TASK_SUSPENDED = 3`, affiché `P` dans `ps` et `task-metrics` |
| Commandes Ring 3 | `task-suspend <pid>`, `task-resume <pid>` |
| Autorité | parent direct d’une tâche utilisateur uniquement |
| Transitions | `READY → SUSPENDED → READY` |
| Erreur d’état | `OS_TASK_BAD_STATE = -71` |

## Sémantique

`task-suspend <pid>` ne réussit que lorsque la cible est un enfant direct utilisateur dans l’état `READY`. Elle devient alors `SUSPENDED`. L’ordonnanceur ne sélectionne que les tâches `READY` et ignore donc naturellement la cible, sans avoir besoin d’un second chemin de planification. `task-resume <pid>` rétablit uniquement une cible `SUSPENDED` vers `READY`.

Le parent direct garde la même autorité locale qu’avec les contrôles de tâche existants. Un PID absent retourne `OS_TASK_NOT_FOUND`, un tiers ou une cible non enfant retourne `OS_TASK_CONTROL_DENIED`, et toute transition redondante ou incompatible retourne `OS_TASK_BAD_STATE`.

> La suspension est un état local de planification. Elle ne produit ni événement IPC, ni résultat enfant, ni sortie, ni réattribution, ni réduction de capacité.

## Vérification

| Preuve | Résultat |
|---|---|
| `make test-all` | **228/228** tests Unity et robustesse réussis |
| Test de tâche | autorité directe, double suspension/refus, reprise et état `READY` vérifiés |
| Test ABI | registres des syscalls 62–63, transition et refus d’état vérifiés |
| Contrat QEMU `spawn` | `task-suspend`, état `P` visible dans `ps`, absence d’exécution avant reprise, `task-resume` puis `idle ok` validés |
| `make qemu-smoke` | core, extras, persistance, création/suspension/supervision et exec réussis |

## Limites

La suspension ne vise qu’un enfant direct déjà prêt. Elle ne suspend pas le parent lui-même, une tâche en attente, la tâche noyau, un ancêtre, un pair ou un descendant indirect. Elle ne fournit pas de signal POSIX, de gel atomique multicœur, de timeout, de priorité spéciale, de groupe de processus, de suspension récursive, de persistance, d’événement de changement d’état, d’accusé ou de capability.

Une tâche suspendue reste comptée dans les capacités locale et globale. Elle peut toujours être terminée par son parent direct ; ce lot n’introduit aucune garantie de temps réel, aucune protection contre un parent malveillant et aucune identité de processus persistante.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | ABI 62–63, état suspendu et erreur de transition |
| `kernel/task/task.[ch]` | état interne, transitions parent-enfant et projection ABI |
| `kernel/syscall/syscall.[ch]` | dispatch et adaptateurs de suspension/reprise |
| `userspace/shell.c` | rendu `P`, wrappers et commandes `task-suspend`/`task-resume` |
| `tests/framework/kernel_mocks.c` | miroir de transitions et dispatch ABI |
| `tests/unit/kernel/test_task.c` | preuves d’autorité et d’état |
| `tests/unit/kernel/test_syscall.c` | preuve ABI 62–63 |
| `tests/scripts/ci_qemu_spawn.py` | preuve d’état suspendu et de reprise réelle |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)
[2] [Supervision sélective — lot Foundation 49](mohhdy_foundation_increment_49_selective_supervision.md)
