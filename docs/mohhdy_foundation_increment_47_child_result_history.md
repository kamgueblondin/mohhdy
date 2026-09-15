# Lot Foundation 47 — Historique borné de résultats enfants

## Objet

Ce lot étend le dernier résultat enfant du lot 46 en une **fenêtre circulaire de quatre terminaisons** retenues localement par le parent direct. Le noyau restitue cet instantané dans l’ordre chronologique, du résultat le plus ancien conservé au plus récent, sans faire vivre de zombie ni allonger la durée de vie des tâches terminées.

| Élément | Contrat livré |
|---|---|
| ABI ajoutée | `SYS_TASK_CHILD_RESULT_LIST = 57`, `MAX_SYSCALLS = 58` |
| Capacité | `OS_TASK_EXIT_HISTORY_CAPACITY = 4` résultats par parent |
| Consultation Ring 3 | `child-results` |
| Ordre | du plus ancien retenu au plus récent |
| Saturation | la cinquième terminaison écrase la plus ancienne entrée |
| Portée | historique local du parent appelant uniquement |
| Compatibilité | `child-result <pid>` conserve le raccourci vers le dernier résultat |

## Réception et rotation

À chaque sortie normale ou retrait autorisé, `task_report_parent_exit()` construit le résultat (PID, code, raison, tick), met à jour le raccourci « dernier résultat », puis ajoute une copie à l’historique circulaire du parent direct. Lorsque la fenêtre est pleine, l’index du plus ancien avance d’une position avant la prochaine restitution.

L’événement IPC best-effort est ensuite envoyé comme précédemment. L’historique est indépendant de la file IPC : une notification peut être perdue si la boîte du parent est pleine, mais le résultat rejoint tout de même l’historique tant que le parent direct vivant peut le recevoir.

> L’ordre restitué est chronologique **dans la fenêtre retenue**, pas un historique global. Une entrée sortie de la fenêtre est définitivement perdue.

## Interface de consultation

`SYS_TASK_CHILD_RESULT_LIST` reçoit un `os_task_exit_history_t*` dans `EBX`. Sa structure publique porte un compteur et quatre `os_task_exit_result_t` au plus. Les cases après `count` sont remises à zéro dans l’instantané ; aucune donnée de mémoire noyau ne fuit lorsque la fenêtre n’est pas remplie.

Le shell affiche chaque entrée avec un marqueur stable : `child-result-entry <pid> <code> <raison>`, puis `child-results ok <count>`. Après un `kill` d’un enfant puis la sortie normale de `waitchild`, la démonstration QEMU montre dans cet ordre le résultat `-128/2`, puis `0/1`.

## Vérification

| Preuve | Résultat |
|---|---|
| `make test-all` | **228/228** tests Unity et robustesse réussis |
| Test de tâche | insertion, ordre, capacité quatre et écrasement du résultat le plus ancien |
| Test ABI | syscall 57, compteur et première entrée vérifiés |
| Contrat QEMU `spawn` | historique des résultats `killed` puis `exited` validé dans l’ordre |
| `make qemu-smoke` | core, extras, persistance, création/supervision/historique et exec réussis |
| Stabilité PS/2 | persistance cadence les touches à 0,65 s et retente au plus trois fois sur le même marqueur strict |

## Limites

L’historique est petit, volatile, non atomique et écrasable : quatre résultats seulement par parent, sans persistance, pagination, curseur, abonnement, verrouillage, accusé, quota, horodatage fiable ou réservation. La relance limitée du smoke de persistance absorbe uniquement les doublons PS/2 transitoires de QEMU TCG : elle ne relâche ni le texte de commande ni le marqueur fonctionnel attendu. Il ne contient ni nom d’enfant, ni statut d’exception matériel, ni signal POSIX, ni détail de chargement, ni provenance vérifiée.

Il ne s’agit toujours pas d’un mécanisme de zombies, de `waitpid`, de collecte destructive, de hiérarchie récursive, de groupe de processus ou d’attente rétrospective générale. La consultation est limitée à l’appelant et les ancêtres, pairs, enfants et tiers ne reçoivent aucun accès implicite.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | syscall 57, capacité et structure d’historique |
| `kernel/task/task.[ch]` | stockage circulaire, rotation et instantané chronologique |
| `kernel/syscall/syscall.[ch]` | dispatch et adaptateur de consultation |
| `userspace/shell.c` | commande `child-results` |
| `tests/framework/kernel_mocks.c` | miroir de la fenêtre et du syscall |
| `tests/unit/kernel/test_task.c` | preuve de rotation et écrasement |
| `tests/unit/kernel/test_syscall.c` | preuve ABI de l’instantané |
| `tests/scripts/ci_qemu_spawn.py` | assertions d’historique QEMU |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)
[2] [Résultat de terminaison d’enfant — lot Foundation 46](mohhdy_foundation_increment_46_child_exit_result.md)
