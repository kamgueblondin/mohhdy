# Lot Foundation 48 — Supervision post-mortem bornée

## Objet

Ce lot complète l’historique enfant des lots 46 et 47 par trois opérations de supervision locales : **attendre puis lire le résultat**, **acquitter complètement l’historique**, et **observer un instantané à une génération attendue**. Le périmètre reste volontairement borné : aucune tâche terminée n’est retenue comme zombie et aucun résultat n’est persisté.

| Élément | Contrat livré |
|---|---|
| ABI ajoutée | `SYS_TASK_CHILD_RESULT_ACK = 58`, `SYS_TASK_CHILD_RESULT_OBSERVE = 59`, `MAX_SYSCALLS = 60` |
| Attente Ring 3 | `wait-result <pid>` compose `SYS_TASK_WAIT` puis `SYS_TASK_CHILD_RESULT` |
| Acquittement | `child-results-clear` efface l’historique et le dernier résultat parent |
| Observation | `child-results-observe <génération>` retourne un instantané seulement si la génération correspond |
| Stale | `OS_TASK_HISTORY_STALE = -70`, avec la génération courante exposée |
| Génération | démarre à 1 et progresse après toute sortie directe ou tout acquittement |

## Attente informative

`wait-result <pid>` est une composition Ring 3 stricte : il réutilise l’attente prospective de `SYS_TASK_WAIT`, puis consulte immédiatement le dernier résultat mémorisé de l’enfant. Le noyau enregistre ce résultat avant de réveiller le waiter ; le shell peut donc afficher, après la reprise, une ligne telle que `wait-result ok 3 0 1`.

Cette commande n’ajoute pas de nouvelle attente noyau et ne transforme pas `wait` en `waitpid`. Si l’enfant avait déjà quitté la file avant l’appel, l’attente historique reste indisponible ; il faut consulter le résultat ou l’historique qui subsiste chez le parent.

## Acquittement et observation

`SYS_TASK_CHILD_RESULT_ACK` remet à zéro le raccourci `child-result`, le compteur et l’index de la fenêtre circulaire. Il retourne la nouvelle génération positive. Une sortie enfant ultérieure réutilise la fenêtre et fait à nouveau progresser la génération.

`SYS_TASK_CHILD_RESULT_OBSERVE` reçoit une génération attendue et un `os_task_exit_history_observation_t*`. Si elle correspond, l’instantané retourne la génération et les résultats en ordre chronologique. En cas d’écart, l’appel retourne `OS_TASK_HISTORY_STALE`, place tout de même la génération courante dans la sortie, et laisse l’historique vide.

> La génération est un indicateur d’invalidation local ; elle ne réserve pas l’historique, ne verrouille aucune tâche et ne garantit pas qu’un nouvel événement n’interviendra pas après la lecture.

## Vérification

| Preuve | Résultat |
|---|---|
| `make test-all` | **228/228** tests Unity et robustesse réussis |
| Test de tâche | observation fraîche et stale, rotation, acquittement et invalidation du dernier résultat |
| Test ABI | syscalls 58–59, génération exposée et historique vide après acquittement |
| Contrat QEMU `spawn` | `wait-result`, observe stale `2→3`, observe fraîche `3`, clear `→4` et liste vide validés |
| `make qemu-smoke` | core, extras, persistance, création/supervision post-mortem et exec réussis |

## Limites

L’acquittement efface toute la fenêtre d’un seul coup ; il n’y a ni suppression sélective, ni curseur, ni pagination, ni partage, ni souscription, ni journal. Les générations sont des entiers 32 bits volatils, non atomiques et peuvent théoriquement reboucler ; la valeur zéro est évitée après un rebouclage.

Aucun zombie, code de signal POSIX, groupe de processus, attente rétrospective générale, timeout, collecte destructive, persistance ou droit d’ancêtre n’est fourni. `wait-result` dépend du dernier résultat local et peut être invalidé par un acquittement ou un départ enfant concurrent avant sa consultation.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | ABI 58–59, erreur stale et structure d’observation |
| `kernel/task/task.[ch]` | génération, acquittement et observation conditionnelle |
| `kernel/syscall/syscall.[ch]` | dispatch et adaptateurs post-mortem |
| `userspace/shell.c` | `wait-result`, `child-results-clear`, `child-results-observe` |
| `tests/framework/kernel_mocks.c` | miroir de génération et dispatch ABI |
| `tests/unit/kernel/test_task.c` | preuves stale/fraîche et clear |
| `tests/unit/kernel/test_syscall.c` | preuves des syscalls 58–59 |
| `tests/scripts/ci_qemu_spawn.py` | scénario supervision post-mortem visible |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)
[2] [Historique borné de résultats enfants — lot Foundation 47](mohhdy_foundation_increment_47_child_result_history.md)
