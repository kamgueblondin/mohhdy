# Lot Foundation 38 — Télémétrie de ressources par tâche

## Objet

Ce lot ajoute un instantané de télémétrie consultable pour une tâche noyau ou Ring 3 donnée. Le nouveau syscall `SYS_TASK_METRICS` (51) remplit `os_task_metrics_t` pour un PID vivant ; la commande shell `task-metrics <pid>` présente cet instantané, ainsi que le résumé global du PMM déjà disponible.

| Élément | Contrat livré |
|---|---|
| ABI | `SYS_TASK_METRICS = 51`, `MAX_SYSCALLS = 52` |
| Entrée | PID signé et pointeur de sortie `os_task_metrics_t` |
| Sortie | PID, état public, type kernel/user, tick de création, âge, ticks exécutés et nombre de sélections |
| Erreur | `OS_TASK_NOT_FOUND` (`-62`) pour PID absent, PID négatif ou sortie nulle |
| Shell | `task-metrics <pid>` et entrée correspondante dans `help` |

## Sémantique des compteurs

Chaque tâche mémorise `created_ticks`, `last_scheduled_ticks`, `run_ticks` et `switch_count`. La tâche noyau initiale est créée avec une première sélection ; les tâches ELF créées depuis l’initrd démarrent avec des compteurs nuls, à l’exception de leur horodatage de création. À chaque passage par `schedule()`, le temps écoulé depuis la dernière sélection de la tâche sortante est ajouté à son cumul, puis la tâche entrante reçoit l’horodatage courant et une sélection supplémentaire.

Lorsqu’une tâche est `RUNNING` au moment de la consultation, `task_fill_metrics()` ajoute également le quantum courant non encore comptabilisé à l’instantané renvoyé. Le compteur stocké n’est pas modifié par la lecture : la requête reste donc observatoire.

> Les valeurs sont des instantanés approximatifs à granularité PIT. Elles ne constituent ni une mesure comptable, ni une réservation CPU, ni un mécanisme d’ordonnancement ou de contrôle d’accès.

## Limites explicites

La télémétrie n’est pas atomique et utilise des compteurs 32 bits susceptibles de déborder après une durée très longue. Elle ne mesure pas la mémoire privée par processus : le champ PMM affiché par le shell est un état physique global du système, car les tâches noyau partagent encore l’architecture mémoire pédagogique existante. Aucun quota, priorité, temps bloqué, temps noyau/utilisateur séparé, consommation par cgroup, persistance, audit ou export réseau n’est fourni.

## Vérification

La suite Unity complète reconstruit et exécute **218/218** tests avec succès. Un nouveau scénario déterministe vérifie l’instantané d’une tâche courante, y compris son quantum non encore sauvegardé, et le refus d’un PID absent. Le contrat QEMU VFS de non-régression termine avec `rc ok 0`; il vérifie le boot BIOS, le shell Ring 3, le médiateur VFS, les profils backend, les montages, les mutations et les révocations existantes.

Le lot étend l’ABI de 0–50 à **0–51** sans modifier le protocole IPC, les droits VFS, les formats de fichiers ou les contrats de délégation backend.

## Utilisation

```text
ps
task-metrics 0
task-metrics <pid>
```

`ps` fournit la liste des PID actifs et `task-metrics` retourne une erreur explicite pour un PID qui n’est plus présent.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | Structure publique, syscall et code d’erreur |
| `kernel/task/task.[ch]` | Compteurs et construction de l’instantané |
| `kernel/syscall/syscall.[ch]` | Dispatch et validation de `SYS_TASK_METRICS` |
| `userspace/shell.c` | Wrapper, commande et aide utilisateur |
| `tests/framework/kernel_mocks.c` | Double déterministe de l’horloge et de la télémétrie |
| `tests/unit/kernel/test_task.c` | Preuve d’instantané et de PID absent |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)
[2] [Contrat de vérification QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
