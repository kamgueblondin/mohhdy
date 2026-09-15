# Lot Foundation 58 — Instantané consolidé de supervision locale

## Objet

Ce lot ajoute une projection unique des indicateurs de supervision déjà détenus localement par un parent : enfants actifs, enfants suspendus, départs cumulés, événements retenus et génération du journal. Il réduit le nombre d’appels nécessaires à une interface Ring 3 de diagnostic, sans changer les mécanismes de contrôle ou de cycle de vie.

| Élément | Contrat livré |
|---|---|
| ABI ajoutée | `SYS_TASK_SUPERVISION_SUMMARY = 74` |
| Plage ABI | syscalls 0–74 ; `MAX_SYSCALLS = 75` |
| Structure | `os_task_supervision_summary_t` |
| Shell | `task-summary` |
| Retour | statut dans `EAX`, structure écrite dans le buffer `EBX` |

## Données projetées

`os_task_supervision_summary_t` contient les champs suivants.

| Champ | Signification |
|---|---|
| `generation` | génération courante du journal local de supervision |
| `active_children` | nombre d’enfants directs non terminés |
| `suspended_children` | sous-ensemble des enfants directs actuellement suspendus |
| `child_exit_count` | total cumulatif des départs directs depuis la création du parent |
| `retained_events` | nombre d’entrées actuellement retenues dans la fenêtre de supervision |

`SYS_TASK_SUPERVISION_SUMMARY` ne prend aucune entrée autre que le pointeur de sortie en `EBX`. Il opère exclusivement sur le parent appelant et retourne `OS_TASK_NOT_FOUND` lorsqu’aucune tâche courante ou aucun buffer valide ne peut être traité.

## Cohérence et sémantique

L’instantané recueille les informations à partir de l’état local courant. Les enfants comptés sont les enfants directs dont l’état n’est pas `TASK_TERMINATED`; les enfants suspendus sont comptés séparément. Le compteur de départs reste indépendant de l’historique de sorties et de toute rotation, observation ou suppression du journal.

> L’instantané est **non atomique** : si une transition survient pendant sa collecte, ses champs peuvent refléter des instants légèrement différents. Il ne réserve aucun enfant, événement ou résultat.

La commande `task-summary` imprime une ligne stable :

```text
task-summary ok <generation> <actifs> <suspendus> <départs> <événements_retenus>
```

## Vérification

| Preuve | Résultat |
|---|---|
| `make all` | noyau i386, Ring 3 et initrd construits avec succès |
| `make test-all` | **239/239** tests Unity et robustesse réussis |
| Test de tâche | enfants actifs/suspendus, génération, événements et départs cumulés |
| Test ABI | dispatch 74 et projection structurée au parent courant |
| Contrat QEMU `spawn` | `task-summary ok 6 1 0 2 0` après délégation et oubli sélectif |
| `make qemu-smoke` | core, extras, persistance, spawn et exec réussis |

## Limites

Cet agrégat ne fournit ni instantané atomique, verrou, transaction, souscription, delta, historique, filtre, recherche, réservation, seuil, quota, priorité, persistance, export, intégrité, identité, capability, ACL, signature, chiffrement ou audit de sécurité. Les compteurs sont locaux, volatils, sur 32 bits et susceptibles de déborder. La fenêtre d’événements reste limitée à quatre entrées et peut changer ou s’écraser avant toute lecture.

Le résumé ne crée aucune autorité nouvelle : il ne permet ni de suspendre, reprendre, terminer, déléguer, attendre, acquitter ou oublier un enfant supplémentaire.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | ABI 74 et structure publique de résumé |
| `kernel/task/task.[ch]` | projection consolidée locale |
| `kernel/syscall/syscall.[ch]` | adaptateur et dispatch ABI 74 |
| `userspace/shell.c` | wrapper et commande `task-summary` |
| `tests/framework/kernel_mocks.c` | miroir de projection et dispatch |
| `tests/unit/kernel/test_task.c` | preuve noyau du résumé |
| `tests/unit/kernel/test_syscall.c` | preuve ABI 74 |
| `tests/scripts/ci_qemu_spawn.py` | assertion Ring 3 consolidée |

## Références

[1] [Supervision événementielle sélective — lot Foundation 57](mohhdy_foundation_increment_57_selective_supervision_events.md)

[2] [État réel de MOHHDY](ETAT_REEL.md)
