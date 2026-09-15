# Lot Foundation 54 — Délégation locale de supervision

## Objet

Ce lot permet à un parent de transférer la supervision d’un de ses enfants **directs** vers une autre tâche utilisateur active. La filiation de l’enfant est remplacée de façon atomique : l’ancien parent perd immédiatement son autorité locale, tandis que le nouveau superviseur obtient les primitives existantes de contrôle, d’inventaire, d’attente et de résultat post-mortem.

| Élément | Contrat livré |
|---|---|
| ABI ajoutée | `SYS_TASK_DELEGATE_CHILD = 68`, `MAX_SYSCALLS = 69` |
| Entrées ABI | `EBX = PID enfant direct`, `ECX = PID du nouveau superviseur` |
| Commande Ring 3 | `task-delegate <pid_enfant> <pid_superviseur>` |
| Nouveau refus | `OS_TASK_BAD_DELEGATE = -73` |
| Mutation | remplacement du `parent_pid` de l’enfant, sans copie ni création de tâche |

## Invariants et contrôle d’autorité

L’appelant doit être le parent direct actuel de l’enfant. Le nouveau superviseur doit exister, être une tâche utilisateur vivante, être distinct de l’appelant et de l’enfant, disposer d’une place dans sa capacité locale, et ne pas appartenir au sous-arbre de l’enfant. Le noyau suit la chaîne des parents du nouveau superviseur pour empêcher l’introduction d’un cycle.

Une délégation est refusée lorsque l’enfant possède déjà un waiter actif. Cette règle évite de dissocier une attente installée de son parent d’origine. Le transfert ne crée aucun événement IPC ni résultat post-mortem : l’événement et le résultat d’une terminaison ultérieure appartiendront au nouveau parent direct, via les mécanismes existants.

> La délégation porte sur une seule relation parent-enfant directe. Elle ne propage pas d’autorité aux descendants, ne transfère pas l’historique de sorties, ni le compteur cumulatif de l’ancien parent.

## Effets observables

Après un succès, `children`, `task-metrics`, `task-suspend`, `task-resume`, `kill-children`, `wait`, `wait-any-result` et les résultats enfants suivent la nouvelle filiation. L’ancien parent ne peut plus contrôler l’enfant puisqu’il n’est plus son parent direct. Si l’enfant termine, ses descendants sont réattribués à son nouveau parent conformément à la règle de réattribution existante.

| Cas | Résultat |
|---|---|
| Appelant non parent direct | `OS_TASK_NOT_CHILD` |
| PID enfant ou superviseur absent | `OS_TASK_NOT_FOUND` |
| Superviseur noyau, terminé, identique à l’enfant ou à l’appelant | `OS_TASK_BAD_DELEGATE` |
| Superviseur descendant de l’enfant ou chaîne parentale invalide | `OS_TASK_BAD_DELEGATE` |
| Attente déjà installée sur l’enfant | `OS_TASK_BAD_STATE` |
| Capacité locale du superviseur pleine | `OS_TASK_CHILD_LIMIT` |

## Vérification

| Preuve | Résultat |
|---|---|
| `make all` | noyau i386, programmes Ring 3 et initrd construits avec succès |
| `make test-all` | **233/233** tests Unity et robustesse réussis |
| Test de tâche | attente active refusée, cycle refusé, transfert valide, perte d’autorité de l’ancien parent, contrôle par le nouveau parent et réattribution du descendant |
| Test ABI | dispatch 68, transfert, refus de l’ancien parent et redélégation par le nouveau superviseur |
| Contrat QEMU `spawn` | deux enfants `idle`, délégation réussie, inventaire de l’ancien parent réduit à un enfant et métrique du délégué pointant vers le nouveau parent |
| `make qemu-smoke` | core, extras, persistance, spawn et exec réussis ; budget spawn porté à 180 s pour couvrir les assertions de délégation sans assouplissement |

## Limites

La délégation est locale, volatile et non persistante. Elle ne fournit ni capability, ACL, identité vérifiée, consentement du superviseur, délégation multi-niveau, annulation automatique, audit, événement de transfert, transaction inter-processus, verrouillage, réservation de capacité, arbre récursif, groupe de processus, collecte de zombie ou récupération d’historique.

Le nouveau superviseur reçoit seulement les droits déjà associés à la filiation directe. Les résultats enfant déjà stockés, leurs générations, les acquittements, le compteur de sorties et les événements best-effort de l’ancien parent ne sont ni déplacés ni fusionnés. Le contrôle de cycle est borné par la capacité globale de tâches et ne transforme pas la filiation en modèle de sécurité général.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | ABI 68, `MAX_SYSCALLS = 69` et erreur `OS_TASK_BAD_DELEGATE` |
| `kernel/task/task.[ch]` | transfert contrôlé de `parent_pid` et protection de cycle |
| `kernel/syscall/syscall.[ch]` | adaptateur et dispatch 68 |
| `userspace/shell.c` | wrapper, aide, commande et diagnostics `task-delegate` |
| `tests/framework/kernel_mocks.c` | miroir Unity du transfert et du dispatch |
| `tests/unit/kernel/test_task.c` | invariants de cycle, attente et autorité après transfert |
| `tests/unit/kernel/test_syscall.c` | preuve ABI 68 |
| `tests/scripts/ci_qemu_spawn.py` | preuve Ring 3 du changement de parent et de l’inventaire |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)

[2] [Filiation avec autorité locale — lot Foundation 40](mohhdy_foundation_increment_40_task_authority.md)

[3] [Réattribution des enfants — lot Foundation 42](mohhdy_foundation_increment_42_task_reparenting.md)

[4] [Supervision multi-enfant — lot Foundation 52](mohhdy_foundation_increment_52_multi_child_supervision.md)
