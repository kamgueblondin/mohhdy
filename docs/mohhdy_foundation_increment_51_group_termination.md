# Lot Foundation 51 — Terminaison groupée locale des enfants

## Objet

Ce lot permet à une tâche parent de terminer, par une demande explicite, **tous ses enfants directs actifs**. La commande applique le même chemin de terminaison que `kill` à chaque cible : résultat enfant `killed`, notification IPC best-effort, réveil éventuel d’un waiter, réattribution des descendants de la cible et retrait de la file de tâches.

| Élément | Contrat livré |
|---|---|
| ABI ajoutée | `SYS_TASK_KILL_CHILDREN = 64`, `MAX_SYSCALLS = 65` |
| Commande Ring 3 | `kill-children` |
| Résultat | nombre d’enfants directs effectivement retirés |
| Cibles | instantané borné des enfants directs du parent appelant, y compris `SUSPENDED` |
| Sortie | chaque cible reçoit `OS_TASK_EXIT_KILLED` et `OS_TASK_EVENT_KILLED` via les mécanismes existants |

## Sémantique

La primitive capture d’abord les PID des enfants directs présents dans la capacité locale, puis les traite un par un. Cette séparation évite de parcourir une liste chaînée pendant son retrait. Chaque enfant conserve la sémantique de `task_kill` : son waiter est réveillé, ses descendants directs sont réattribués au parent sortant et un événement de sortie est tenté sans bloquer la terminaison.

> Les enfants devenus directs par réattribution pendant l’appel ne sont pas ajoutés à l’instantané déjà capturé. Un second `kill-children` explicite peut les traiter.

Un parent sans enfant reçoit `0`. Un appel exécuté sans tâche courante échoue avec `OS_TASK_NOT_FOUND`. Il n’existe aucun contrôle global sur des tâches de même nom, des pairs, des ancêtres ou des descendants indirects.

## Vérification

| Preuve | Résultat |
|---|---|
| `make test-all` | **229/229** tests Unity et robustesse réussis |
| Test de tâche | deux enfants directs, dont un suspendu, supprimés ; descendant réattribué puis traité au second appel |
| Test ABI | syscall 64, comptage de retrait, résultat de sortie et second appel vide vérifiés |
| Contrat QEMU `spawn` | enfant `P` visible, `kill-children ok 1`, événement killed, résultat `-128`, capacité et `ps` nettoyés |
| `make qemu-smoke` | core, extras, persistance, création/terminaison groupée et exec réussis |

## Limites

La terminaison groupée est locale, non atomique, non transactionnelle et limitée à l’instantané des quatre enfants directs au plus. Elle ne crée ni groupe de processus, ni signal POSIX, ni terminaison récursive automatique, ni cascade lors de la sortie du parent, ni délai, ni annulation, ni journal durable, ni capability, ni identité persistante.

Les notifications IPC restent best-effort et peuvent être perdues lorsque la boîte parent est pleine. Les résultats post-mortem restent soumis à la fenêtre locale bornée existante ; un PID absent de cette fenêtre ne devient pas récupérable par cette primitive.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | ABI 64 et `MAX_SYSCALLS = 65` |
| `kernel/task/task.[ch]` | instantané, terminaison directe groupée et réattribution existante |
| `kernel/syscall/syscall.[ch]` | dispatch et adaptateur `SYS_TASK_KILL_CHILDREN` |
| `userspace/shell.c` | wrapper, aide et commande `kill-children` |
| `tests/framework/kernel_mocks.c` | miroir de l’instantané et du dispatch |
| `tests/unit/kernel/test_task.c` | preuve d’instantané, suspension et réattribution |
| `tests/unit/kernel/test_syscall.c` | preuve ABI 64 |
| `tests/scripts/ci_qemu_spawn.py` | preuve d’arrêt groupé d’un enfant suspendu |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)
[2] [Suspension locale — lot Foundation 50](mohhdy_foundation_increment_50_task_suspension.md)
