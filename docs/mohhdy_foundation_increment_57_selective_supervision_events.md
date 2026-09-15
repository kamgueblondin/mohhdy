# Lot Foundation 57 — Consultation et oubli sélectifs des événements de supervision

## Objet

Ce lot complète la fenêtre locale de supervision par une recherche et une suppression ciblées, identifiées par la séquence locale d’un événement retenu. Il permet à un parent de traiter une transition précise sans acquitter l’intégralité de son journal.

| Élément | Contrat livré |
|---|---|
| ABI ajoutée | `SYS_TASK_SUPERVISION_EVENT_FIND = 72`, `SYS_TASK_SUPERVISION_EVENT_FORGET = 73` |
| Plage ABI | syscalls 0–73 ; `MAX_SYSCALLS = 74` |
| Erreur dédiée | `OS_TASK_NO_SUPERVISION_EVENT = -74` |
| Commandes Ring 3 | `task-event <séquence>` et `task-events-forget <séquence>` |
| Clé de sélection | séquence locale non nulle de l’événement |

## Recherche par séquence

`SYS_TASK_SUPERVISION_EVENT_FIND` reçoit la séquence en `EBX` et un pointeur `os_task_supervision_event_t*` en `ECX`. Le noyau ne cherche que dans la fenêtre bornée du parent courant, du plus ancien au plus récent. Une séquence nulle, évincée, acquittée ou déjà oubliée retourne `OS_TASK_NO_SUPERVISION_EVENT`.

| Condition | Retour | Effet |
|---|---:|---|
| Événement retenu | `0` | copie de l’événement dans le buffer appelant |
| Séquence absente ou nulle | `OS_TASK_NO_SUPERVISION_EVENT` | aucune modification |
| Parent absent ou buffer nul | `OS_TASK_NOT_FOUND` | aucune garantie utile |

La recherche est lecture seule : elle ne change ni le nombre d’entrées, ni l’ordre, ni la génération, ni la séquence suivante.

## Oubli sélectif

`SYS_TASK_SUPERVISION_EVENT_FORGET` reçoit uniquement la séquence en `EBX`. Après une correspondance, le noyau compacte la fenêtre de quatre entrées maximum en préservant l’ordre relatif des événements restants, incrémente la génération et retourne le nombre d’entrées conservées.

> L’oubli ne réutilise pas la séquence supprimée. Les événements futurs continuent avec la séquence locale suivante, afin qu’une ancienne référence ne désigne jamais une nouvelle transition.

La suppression d’une séquence absente ne modifie rien et retourne `OS_TASK_NO_SUPERVISION_EVENT`. L’opération ne touche ni l’historique des sorties enfants, ni le compteur cumulatif, ni la filiation, ni les messages IPC.

## Interface Ring 3

| Commande | Sortie réussie |
|---|---|
| `task-event 4` | `task-event ok 4 <action> <enfant> <lié> <détail> <ticks>` |
| `task-events-forget 4` | `task-events-forget ok 4 <nombre_restant>` |

Une séquence qui n’est plus retenue produit un diagnostic explicite de transition absente. Les commandes n’acceptent pas la séquence zéro ni une valeur négative.

## Vérification

| Preuve | Résultat |
|---|---|
| `make all` | noyau i386, Ring 3 et initrd construits avec succès |
| `make test-all` | **237/237** tests Unity et robustesse réussis |
| Test de tâche | recherche, oubli intermédiaire, compaction ordonnée, génération et refus après retrait |
| Test ABI | dispatch 72–73, copie de l’événement, suppression et état compacté |
| Contrat QEMU `spawn` | délégation recherchée par séquence, oubliée sélectivement puis fenêtre vide à la génération suivante |
| `make qemu-smoke` | core, extras, persistance, spawn et exec réussis ; budget spawn porté à 240 s pour les preuves complètes sans assouplissement |

## Limites

La sélection reste locale, volatile, non atomique et limitée à quatre entrées. Une séquence peut déjà être absente parce que la fenêtre a été écrasée, acquittée ou modifiée par une opération concurrente. Ce lot ne fournit ni réservation, verrou, transaction, lecteur multiple, suppression par PID ou action, restauration, corbeille, recherche globale, filtre, persistance, intégrité, identité, capability, ACL, signature, chiffrement, synchronisation, export ou audit sécurisé.

L’oubli sélectif n’efface pas une transition du noyau, un message IPC, un résultat enfant ou une trace externe : il retire seulement une copie retenue dans le journal du parent appelant.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | ABI 72–73 et erreur publique dédiée |
| `kernel/task/task.[ch]` | recherche, compaction et oubli par séquence |
| `kernel/syscall/syscall.[ch]` | adaptateurs et dispatch ABI 72–73 |
| `userspace/shell.c` | wrappers et commandes de consultation/oubli |
| `tests/framework/kernel_mocks.c` | miroir de sélection et dispatch |
| `tests/unit/kernel/test_task.c` | preuve noyau de compaction sélective |
| `tests/unit/kernel/test_syscall.c` | preuve ABI 72–73 |
| `tests/scripts/ci_qemu_spawn.py` | contrat Ring 3 de recherche et oubli |
| `tests/scripts/ci_qemu_smoke.sh` | budget spawn de 240 secondes |

## Références

[1] [Consommation post-mortem du journal — lot Foundation 56](mohhdy_foundation_increment_56_supervision_events_lifecycle.md)

[2] [État réel de MOHHDY](ETAT_REEL.md)
