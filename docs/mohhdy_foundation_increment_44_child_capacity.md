# Lot Foundation 44 — Capacité locale d’enfants par parent

## Objet

Ce lot prolonge la supervision parent-enfant en bornant le nombre de tâches enfants directes qu’un parent peut créer. Chaque tâche existante peut posséder au plus **quatre enfants directs**. La limite est appliquée avant le chargement d’un programme et avant l’allocation de sa tâche dans les chemins `spawn` et `exec`.

| Élément | Contrat livré |
|---|---|
| Capacité | `OS_TASK_CHILD_CAPACITY = 4` enfants directs |
| Refus | `OS_TASK_CHILD_LIMIT = -66` |
| Chemins protégés | `SYS_SPAWN` et `SYS_EXEC` |
| Observation | `task-metrics <pid>` expose déjà `direct_children` |
| Libération | immédiate après sortie, `kill` autorisé ou réattribution qui retire le lien direct |

## Politique appliquée

`task_can_create_child(pid)` vérifie d’abord l’existence du parent, puis compare `task_count_direct_children(pid)` à la capacité fixée. Les adaptateurs `sys_spawn()` et `sys_exec()` appellent cette primitive avant `create_task_from_initrd_file()`. Une demande au-delà de la limite ne charge donc aucun ELF, ne crée aucune tâche et ne consomme aucune page supplémentaire pour l’enfant rejeté.

Le shell détecte `OS_TASK_CHILD_LIMIT` et affiche une erreur explicite plutôt que de la présenter comme un programme introuvable. Le compteur direct déjà fourni par la télémétrie permet de diagnostiquer l’occupation locale.

> La limite est **par parent direct**, non globale. La réattribution de l’incrément 42 peut modifier instantanément la capacité disponible d’un ancêtre, car le compteur reflète la filiation directe courante.

## Vérification

| Preuve | Résultat |
|---|---|
| `make test-all` | **226/226** tests Unity et robustesse réussis |
| Test de tâches | quatre enfants admis, cinquième refusé par `-66`, emplacement immédiatement libéré après `kill` autorisé |
| Compilation | noyau i386 et shell Ring 3 compilés avec la garde avant allocation |
| `make qemu-smoke` | core, extras, persistance, `spawn`/PPID/priorité/kill, supervision `wait` et `exec` réussis |

Le contrat QEMU conserve le scénario réel de création, d’attente et de terminaison ordinaire d’un enfant. La saturation est couverte de façon déterministe par Unity, sans allonger inutilement la séquence clavier QEMU de plusieurs créations concurrentes.

## Limites

La capacité est fixe, volatile, non atomique et globale à chaque parent. Elle ne constitue ni un quota mémoire, ni une limite CPU, ni un quota d’IPC, ni une limite hiérarchique récursive. Il n’existe ni configuration par utilisateur, groupe, service ou chemin, ni réservation, délai, admission transactionnelle, priorité de parent, isolation VMM, capability, ACL ou identité vérifiée.

Le noyau n’implémente toujours pas la collecte de statut de sortie, les zombies, un `waitpid` complet, les signaux, les groupes de processus ou les politiques de reprise après saturation.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | capacité publique et erreur `OS_TASK_CHILD_LIMIT` |
| `kernel/task/task.[ch]` | primitive de contrôle de capacité par parent |
| `kernel/syscall/syscall.c` | garde avant allocation dans `spawn` et `exec` |
| `userspace/shell.c` | message de refus explicite de `spawn` |
| `tests/framework/kernel_mocks.c` | miroir Unity de la primitive |
| `tests/unit/kernel/test_task.c` | saturation et libération de capacité |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)
[2] [Supervision et attente d’enfant — lot Foundation 43](mohhdy_foundation_increment_43_task_wait.md)
