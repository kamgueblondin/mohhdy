# Lot Foundation 43 — Supervision et attente d’enfant

## Objet

Ce lot complète la filiation et le cycle de vie par une supervision minimale de l’enfant direct. Les métriques de tâche exposent le nombre actuel d’enfants directs ; la nouvelle commande `wait <pid>` met le parent en attente jusqu’au départ de son enfant direct. Le réveil est effectué dans le noyau lorsque l’enfant sort normalement ou est retiré après une terminaison autorisée.

| Élément | Contrat livré |
|---|---|
| ABI | `SYS_TASK_WAIT` (`53`), `MAX_SYSCALLS = 54` |
| Métrique | `direct_children` ajouté à `os_task_metrics_t` |
| Commande | `wait <pid>` |
| Cible autorisée | enfant direct uniquement |
| Refus hors filiation | `OS_TASK_NOT_CHILD` (`-65`) |
| Réveil | sortie `SYS_EXIT` ou `kill` autorisé de l’enfant |

## Politique appliquée

`task_wait_for_child()` valide l’existence du parent et de la cible, exige que `child.parent_pid == requester_pid`, conserve l’identité du parent dans `child.waiter_pid`, puis place le parent en état `WAITING`. Le dispatch de `SYS_TASK_WAIT` appelle l’ordonnanceur seulement lorsque cette transition est réussie.

Au départ de l’enfant, `task_wake_waiter()` remet son parent en état `READY` et efface `waiter_pid`. La même primitive est déjà utilisée par `exec`, ce qui conserve une seule convention noyau pour le réveil d’un parent superviseur.

Le compteur `direct_children` est un instantané non atomique de la file de tâches. Il compte seulement les tâches dont le parent enregistré est exactement le PID interrogé ; une réattribution de l’incrément 42 se reflète donc naturellement dans l’instantané suivant.

> L’attente ne confère aucun nouveau droit de contrôle. Elle est limitée à l’enfant direct et ne transforme pas le parent en administrateur d’une arborescence complète.

## Preuve QEMU

Le programme Ring 3 `waitchild` est inclus dans l’initrd comme client de preuve. Il annonce son démarrage, cède une fois le CPU, annonce sa fin puis retourne de `main`, ce qui exécute `SYS_EXIT`. Le smoke QEMU réalise ensuite la séquence suivante :

| Étape | Assertion |
|---|---|
| `spawn waitchild` | enfant créé et PID décodé malgré l’entrelacement série |
| `task-metrics 1` | `Enfants directs : 1` |
| `wait <pid>` | parent bloqué, enfant termine, parent réveillé, `wait ok <pid>` |
| `task-metrics 1` | `Enfants directs : 0` |

La sortie détaillée de télémétrie allonge le scénario clavier QEMU. Les budgets du smoke restent stricts mais passent à 150 s pour extras et 120 s pour spawn, après une validation réelle sur QEMU TCG ; aucune assertion n’est relâchée.

## Limites

Il ne s’agit pas d’un `waitpid` complet. Aucun statut de sortie, zombie, signal, délai, interruption, attente de n’importe quel enfant, attente d’ancêtre, attente multiple, collecte de ressource, adoption, groupe de processus, session, capability, ACL ou identité authentifiée n’est livré. Un enfant déjà retiré ne peut pas être attendu rétrospectivement ; le parent doit installer l’attente pendant que l’enfant existe.

Le compteur et l’état de tâche demeurent volatils, non atomiques et 32 bits. L’ordonnancement conserve ses limites précédentes : pas de priorité temps réel, de quota ou de prévention de famine.

## Vérification

| Preuve | Résultat |
|---|---|
| `make test-all` | **225/225** tests Unity et robustesse réussis |
| Test de tâche | compteur direct, attente, état `WAITING`, réveil `READY`, nettoyage du waiter et refus hors filiation |
| Test syscall | `SYS_TASK_WAIT` accepté pour l’enfant direct et rejeté depuis une tâche sans lien |
| `make qemu-smoke` | core, extras, persistance, `spawn`/PPID/priorité/kill, `wait`, et `exec` réussis |

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | syscall 53, erreur `-65`, métrique `direct_children` |
| `kernel/task/task.[ch]` | comptage et attente validée de l’enfant |
| `kernel/syscall/syscall.[ch]` | dispatch et adaptateur `SYS_TASK_WAIT` |
| `userspace/shell.c` | wrapper, `wait <pid>`, télémétrie enrichie et aide |
| `userspace/wait_child.c` | programme Ring 3 de preuve |
| `userspace/Makefile`, `Makefile` | compilation et empaquetage initrd de `waitchild` |
| `tests/framework/kernel_mocks.c` | double de supervision |
| `tests/unit/kernel/test_task.c`, `test_syscall.c` | preuves directe et ABI |
| `tests/scripts/ci_qemu_spawn.py` | contrat QEMU blocage/réveil et compteur |
| `tests/scripts/ci_qemu_smoke.sh` | budgets QEMU ajustés aux diagnostics enrichis |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)
[2] [Réattribution des enfants — lot Foundation 42](mohhdy_foundation_increment_42_task_reparenting.md)
