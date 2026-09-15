# Lot Foundation 53 — Compteur cumulatif de terminaisons enfants

## Objet

Ce lot ajoute au parent un **compteur cumulatif de départs de ses enfants directs**. Il complète l’historique post-mortem borné : même lorsqu’un résultat a été acquitté, oublié ou évincé de la fenêtre circulaire, le parent conserve le nombre total de sorties directes qu’il a observées depuis sa création.

| Élément | Contrat livré |
|---|---|
| ABI ajoutée | `SYS_TASK_CHILD_EXIT_COUNT = 67`, `MAX_SYSCALLS = 68` |
| Sortie ABI | `os_task_child_exit_count_t`, avec un champ non signé `count` de 32 bits |
| Commande Ring 3 | `child-exit-count` |
| Valeur | total des départs d’enfants directs rapportés au parent courant depuis sa création |
| Chemins comptés | sortie normale `SYS_EXIT` et terminaison autorisée, y compris `kill-children` |
| Effacement | aucun : les acquittements, oublis et rotations de l’historique ne remettent pas le compteur à zéro |

## Sémantique

Le compteur appartient à chaque tâche parent et est initialisé à zéro à sa création. Il est incrémenté dans le chemin commun `task_report_parent_exit()` avant l’alimentation de l’historique post-mortem et avant la notification IPC best-effort. Ainsi, une boîte IPC pleine n’empêche jamais la progression du compteur.

Le syscall reçoit un pointeur `EBX` vers `os_task_child_exit_count_t`. Il retourne un statut dans `EAX` et écrit la valeur dans `count`, ce qui conserve une représentation non signée complète sur 32 bits au lieu de confondre une grande valeur avec un code d’erreur négatif. La commande `child-exit-count` affiche cette valeur sans conversion signée.

> Le comptage est effectué pour le parent direct présent au moment du départ. Lorsqu’un enfant a été réattribué, une sortie ultérieure est donc attribuée à son parent direct courant, conformément à la filiation locale existante.

## Vérification

| Preuve | Résultat |
|---|---|
| `make all` | noyau i386, programmes Ring 3 et initrd construits avec succès |
| `make test-all` | **231/231** tests Unity et robustesse réussis |
| Test de tâche | compteur initial nul, deux terminaisons autorisées, stabilité après acquittement de l’historique et refus de buffer nul |
| Test ABI | syscall 67, statut, valeur structurée et conservation après acquittement vérifiés |
| Contrat QEMU `spawn` | `child-exit-count ok 1` après terminaison groupée, puis `child-exit-count ok 2` après la sortie de `waitchild`; ses yields bornés évitent une sortie prématurée avant l’attente du parent |
| `make qemu-smoke` | core, extras, persistance, spawn et exec réussis |

## Limites

Le compteur est local, volatile, non atomique et stocké sur 32 bits : il effectue naturellement un retour à zéro après débordement. Il ne fournit ni horodatage, ni détail par PID, ni liste d’exceptions, ni garantie de livraison d’événement, ni historique exhaustif, ni persistance à travers la fin ou la recréation du parent.

Il ne compte ni les départs de pairs, d’ancêtres, de descendants indirects, ni les sorties intervenues avant la création du parent. Il ne constitue ni une réservation, ni une capability, ni une identité, ni une primitive de collecte ou de synchronisation. La supervision ciblée, multi-enfant et post-mortem existante conserve ses propres limites et fenêtres bornées.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | ABI 67, `MAX_SYSCALLS = 68` et structure de sortie 32 bits |
| `kernel/task/task.[ch]` | stockage parent, incrément sur sortie directe et lecture locale |
| `kernel/syscall/syscall.[ch]` | dispatch et adaptateur de sortie structurée |
| `userspace/shell.c` | wrapper, affichage non signé, aide et commande `child-exit-count` |
| `tests/framework/kernel_mocks.c` | miroir du stockage, du rapport de sortie et de l’ABI |
| `tests/unit/kernel/test_task.c` | preuve de cumul et d’indépendance vis-à-vis de l’acquittement |
| `tests/unit/kernel/test_syscall.c` | preuve du statut et du buffer ABI 67 |
| `tests/scripts/ci_qemu_spawn.py` | preuves QEMU des valeurs 1 puis 2 |
| `userspace/wait_child.c` | séquence bornée de yields pour une fenêtre d’attente QEMU déterministe |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)

[2] [Résultat de terminaison d’enfant — lot Foundation 46](mohhdy_foundation_increment_46_child_exit_result.md)

[3] [Historique borné des résultats enfants — lot Foundation 47](mohhdy_foundation_increment_47_child_result_history.md)

[4] [Supervision multi-enfant — lot Foundation 52](mohhdy_foundation_increment_52_multi_child_supervision.md)
