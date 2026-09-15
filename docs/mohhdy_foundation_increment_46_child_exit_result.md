# Lot Foundation 46 — Résultat de terminaison d’enfant

## Objet

Ce lot complète la supervision directe par un résultat de terminaison consultable après le départ d’un enfant. Le parent conserve un **unique dernier résultat local** : PID enfant, code de sortie, raison et tick de terminaison. La consultation ne crée ni zombie, ni file d’historique, ni capacité d’attente rétrospective.

| Élément | Contrat livré |
|---|---|
| ABI ajoutée | `SYS_TASK_CHILD_RESULT = 56`, `MAX_SYSCALLS = 57` |
| Commande Ring 3 | `child-result <pid>` |
| Sortie normale | le code transmis dans `EBX` à `SYS_EXIT` est conservé |
| Retrait autorisé | code synthétique `OS_TASK_EXIT_KILLED = -128` |
| Raison | `OS_TASK_EVENT_EXITED = 1` ou `OS_TASK_EVENT_KILLED = 2` |
| Absence de résultat | `OS_TASK_NO_CHILD_RESULT = -69` |
| Conservation | un seul résultat, remplacé au prochain départ d’enfant direct |

## Chemin de terminaison

Lors d’un `SYS_EXIT`, le noyau lit le code placé dans `EBX` par le programme Ring 3. Avant de réveiller un waiter ou de réattribuer les descendants, il enregistre ce code chez le parent direct encore actif, puis émet l’événement IPC best-effort livré par le lot 45. Lors d’un `kill` autorisé, le même chemin enregistre `-128` et la raison `killed`.

L’enregistrement précède la réattribution, ce qui garantit qu’il est associé au parent direct qui détenait la relation au moment du départ. Il ne dépend pas de l’espace disponible dans la boîte IPC : l’événement peut être perdu, mais le dernier résultat reste consultable tant qu’il n’est pas remplacé et que le parent vit.

> Le résultat est un instantané local du **dernier** enfant direct sorti. Il n’est ni un journal, ni une réservation, ni une preuve d’identité et ne restitue pas un enfant plus ancien après l’arrivée d’un nouveau résultat.

## Consultation

`SYS_TASK_CHILD_RESULT` reçoit le PID enfant dans `EBX` et un `os_task_exit_result_t*` dans `ECX`. Il ne lit que l’enregistrement de l’appelant. Si le PID demandé ne correspond pas au dernier enfant mémorisé, il retourne `OS_TASK_NO_CHILD_RESULT` sans divulguer d’état de tâche tiers.

| Champ | Sens |
|---|---|
| `child_pid` | PID de l’enfant direct terminé |
| `exit_code` | code `SYS_EXIT` ou `OS_TASK_EXIT_KILLED` |
| `reason` | `exited` ou `killed` |
| `finished_ticks` | tick PIT local non atomique à l’enregistrement |

Le shell affiche une forme stable, par exemple `child-result ok 3 0 1` après le retour normal de `waitchild`, ou `child-result ok 2 -128 2` après un `kill` autorisé.

## Vérification

| Preuve | Résultat |
|---|---|
| `make test-all` | **228/228** tests Unity et robustesse réussis |
| Test de tâche | code normal, PID, raison, absence de résultat et sentinelle de kill vérifiés |
| Test ABI | syscall 56, structure de résultat et refus explicite vérifiés |
| Contrat QEMU `spawn` | résultat `-128/2` après `kill`, puis `0/1` après sortie de `waitchild` |
| `make qemu-smoke` | core, extras, persistance, création/supervision/résultat et exec réussis |

## Limites

Le mécanisme ne conserve qu’un résultat par parent ; il écrase donc celui d’un enfant précédent. Il n’offre pas de zombie, de table de plusieurs résultats, de collecte destructive, de code de signal, de groupe de processus, de timeout, de blocage jusqu’à disponibilité d’un résultat, de droits d’ancêtre ou d’historique durable.

`wait` reste une attente prospective : il exige qu’un enfant direct soit encore présent. `child-result` est une consultation rétrospective très limitée du dernier départ seulement. Les ticks et la structure sont volatils, non atomiques et non persistants. Les erreurs de chargement d’ELF, les exceptions noyau et les arrêts matériels ne produisent pas de résultat de sortie fiable.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | syscall 56, erreur, sentinelle et structure de résultat |
| `kernel/task/task.[ch]` | stockage par parent, rapport de sortie et lecture locale |
| `kernel/syscall/syscall.[ch]` | capture de `EBX` sur `SYS_EXIT` et dispatch de consultation |
| `userspace/shell.c` | `child-result <pid>` |
| `tests/framework/kernel_mocks.c` | miroir du rapport et du syscall |
| `tests/unit/kernel/test_task.c` | résultats normal et forcé |
| `tests/unit/kernel/test_syscall.c` | ABI de consultation |
| `tests/scripts/ci_qemu_spawn.py` | assertions QEMU des deux résultats |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)
[2] [Gouvernance observable des tâches — lot Foundation 45](mohhdy_foundation_increment_45_task_governance.md)
