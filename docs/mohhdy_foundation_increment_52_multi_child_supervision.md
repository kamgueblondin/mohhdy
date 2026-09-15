# Lot Foundation 52 — Supervision multi-enfant locale

## Objet

Ce lot permet à une tâche parent de **recenser ses enfants directs actifs** et d’attendre la prochaine terminaison de l’un d’eux sans connaître son PID à l’avance. Il complète la supervision ciblée déjà disponible : `children` fournit l’état présent des enfants, tandis que `wait-any-result` attend une sortie future puis restitue le dernier résultat enfant enregistré par le mécanisme post-mortem existant.

| Élément | Contrat livré |
|---|---|
| ABI ajoutée | `SYS_TASK_CHILDREN = 65`, `SYS_TASK_WAIT_ANY = 66`, `MAX_SYSCALLS = 67` |
| Instantané | `os_task_children_t` contient `count` et au plus `OS_TASK_CHILD_CAPACITY` entrées `os_proc_t` |
| Commandes Ring 3 | `children` et `wait-any-result` |
| Inventaire | PID, PPID, état, type et nom des enfants directs actifs du parent appelant |
| Attente | le parent attend la prochaine sortie d’un enfant direct, quel que soit son PID |
| Refus explicite | `OS_TASK_NO_DIRECT_CHILD` (`-72`) lorsqu’aucun enfant direct actif n’est supervisable |

## Sémantique

`SYS_TASK_CHILDREN` remplit un instantané local et non atomique. Il ne retient que les enfants encore présents dans la file de tâches et respecte la capacité locale existante de quatre enfants directs. Il expose notamment l’état `SUSPENDED`, afin qu’un parent puisse distinguer un enfant actif prêt, en attente ou suspendu sans pouvoir déduire une réservation ou un verrouillage à partir de cet instantané.

`SYS_TASK_WAIT_ANY` inscrit le parent courant comme waiter de chacun de ses enfants directs présents, puis place le parent dans l’état d’attente. Le premier enfant qui emprunte le chemin normal de sortie ou de terminaison autorisée réveille le parent et alimente les mécanismes de résultat et de notification déjà établis. Un appel sans enfant direct actif échoue immédiatement avec `OS_TASK_NO_DIRECT_CHILD`.

> `wait-any-result` est une composition Ring 3 : il invoque d’abord l’attente sans PID, puis consulte le dernier résultat de l’historique local. Il affiche `wait-any-result ok <pid> <code> <raison>` seulement lorsqu’un résultat est disponible.

Cette composition conserve la convention des raisons existantes : `1` correspond à une sortie normale et `2` à une terminaison demandée. L’événement IPC enfant demeure un canal best-effort séparé ; la lecture du résultat historique ne dépend donc pas de la livraison de cet événement.

## Vérification

| Preuve | Résultat |
|---|---|
| `make all` | noyau i386, programmes Ring 3 et initrd construits avec succès |
| `make test-all` | **230/230** tests Unity et robustesse réussis |
| Test de tâche | inventaire de deux enfants directs, visibilité d’un enfant suspendu, inscription des waiters, réveil sur le premier départ et refus après retrait du dernier enfant |
| Test ABI | syscalls 65–66, contenu de `os_task_children_t`, attente et refus `OS_TASK_NO_DIRECT_CHILD` vérifiés |
| Contrat QEMU `spawn` | `children ok 1` expose l’enfant renommé ; `wait-any-result` retourne le PID et le résultat de `waitchild` |
| `make qemu-smoke` | core, extras, persistance, supervision/terminaison, et exec réussis |

## Limites

L’inventaire est local, volatil, borné à quatre entrées et non atomique. Une entrée peut donc devenir obsolète avant une commande ultérieure ; il ne s’agit ni d’une réservation, ni d’un handle, ni d’une capability, ni d’une vue exhaustive de l’arbre de tâches.

L’attente est prospective, limitée à la filiation directe et ne crée ni zombie, ni collecte POSIX, ni sélection par priorité, ni délai, ni annulation, ni surveillance d’ancêtres, de pairs ou de descendants indirects. Elle réutilise un waiter unique par enfant : un conflit de waiter existant reste refusé par le contrôle local déjà appliqué. Les résultats restent dans la fenêtre circulaire volatile de quatre sorties et peuvent être effacés, compactés ou écrasés selon les opérations de supervision post-mortem antérieures.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | ABI 65–66, `MAX_SYSCALLS = 67`, erreur `-72` et structure `os_task_children_t` |
| `kernel/task/task.[ch]` | inventaire direct et attente sans PID |
| `kernel/syscall/syscall.[ch]` | dispatch et adaptateurs 65–66 |
| `userspace/shell.c` | wrappers, aide, `children` et `wait-any-result` |
| `tests/framework/kernel_mocks.c` | miroir Unity de la logique et du dispatch |
| `tests/unit/kernel/test_task.c` | preuve de supervision multi-enfant et de réveil |
| `tests/unit/kernel/test_syscall.c` | preuve ABI 65–66 |
| `tests/scripts/ci_qemu_spawn.py` | contrat QEMU de l’inventaire et de l’attente sans PID |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)

[2] [Supervision d’enfant — lot Foundation 43](mohhdy_foundation_increment_43_task_wait.md)

[3] [Supervision post-mortem — lot Foundation 48](mohhdy_foundation_increment_48_postmortem_supervision.md)

[4] [Terminaison groupée locale — lot Foundation 51](mohhdy_foundation_increment_51_group_termination.md)
