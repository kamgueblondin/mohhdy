# Lot Foundation 55 — Journal borné des transitions de supervision

## Objet

Ce lot ajoute une trace locale, structurée et observable des transitions de supervision des enfants directs. Il fournit une base factuelle minimale pour le diagnostic et les travaux d’audit ultérieurs, sans prétendre être un système d’audit de sécurité, une journalisation persistante ou un mécanisme de contrôle d’accès.

| Élément | Contrat livré |
|---|---|
| ABI ajoutée | `SYS_TASK_SUPERVISION_EVENTS = 69`, `MAX_SYSCALLS = 70` |
| Sortie ABI | `os_task_supervision_events_t` avec génération, nombre d’entrées et fenêtre circulaire de quatre événements |
| Commande Ring 3 | `task-events` |
| Propriétaire | parent courant ; chaque superviseur possède son journal local |
| Rétention | quatre événements, retournés du plus ancien au plus récent |

## Événements enregistrés

Le noyau enregistre les transitions uniquement après leur validation effective. Le tableau suivant décrit la charge utile publique `os_task_supervision_event_t`.

| Action | `child_pid` | `related_pid` | `detail` |
|---|---:|---:|---:|
| `OS_TASK_SUPERVISION_EXIT` | enfant terminé | `0` | motif de sortie existant : `OS_TASK_EVENT_EXITED` ou `OS_TASK_EVENT_KILLED` |
| `OS_TASK_SUPERVISION_SUSPEND` | enfant suspendu | `0` | `0` |
| `OS_TASK_SUPERVISION_RESUME` | enfant repris | `0` | `0` |
| `OS_TASK_SUPERVISION_DELEGATE_OUT` | enfant délégué | nouveau superviseur | `0` |
| `OS_TASK_SUPERVISION_DELEGATE_IN` | enfant reçu | ancien superviseur | `0` |

Chaque entrée contient aussi une séquence locale monotone non nulle et le tick d’horloge associé. La génération progresse à chaque ajout ; la séquence et la génération peuvent repartir à une valeur basse après débordement 32 bits, sans garantie de continuité globale.

> La fenêtre est maintenue même si la boîte IPC parent est pleine. Le journal documente donc la transition noyau effective, alors que la notification IPC de sortie reste best-effort.

## Sémantique de rétention et de délégation

Lorsqu’un cinquième événement est ajouté, l’entrée la plus ancienne est écrasée. Le journal est volatile, non atomique et lié à la durée de vie du superviseur. Une délégation crée un événement `delegate-out` dans l’historique de l’ancien parent et un événement `delegate-in` dans celui du nouveau superviseur. Une sortie ultérieure appartient seulement au journal du parent direct alors courant.

Le syscall prend en `EBX` un pointeur vers la structure de sortie et retourne son statut dans `EAX`. La commande `task-events` imprime d’abord `task-events ok <generation> <count>`, puis chaque entrée sous la forme `task-event <sequence> <action> <enfant> <lié> <détail> <ticks>`.

## Vérification

| Preuve | Résultat |
|---|---|
| `make all` | noyau i386, Ring 3 et initrd construits avec succès |
| `make test-all` | **235/235** tests Unity et robustesse réussis |
| Test de tâche | ordre, séquences, fenêtre circulaire, délégation sortante/entrante et sortie attribuée au nouveau superviseur |
| Test ABI | syscall 69, instantané suspend/reprise et historiques séparés de délégation vérifiés |
| Contrat QEMU `spawn` | séquences suspend, sorties killed/exited et délégation sortante vérifiées dans le shell Ring 3 |
| `make qemu-smoke` | core, extras, persistance, spawn et exec réussis |

## Limites

Ce mécanisme n’est **pas** un audit de sécurité complet. Il ne fournit ni persistance disque, horodatage fiable, signature, intégrité cryptographique, identité vérifiée, ACL, capability, consentement, souscription, filtre, recherche, accusé de lecture, export, synchronisation, confidentialité, rétention configurable, table d’événements globale, historique exhaustif, transaction ni garantie de livraison.

Le journal ne trace que les transitions de supervision implémentées dans ce noyau. Il n’enregistre ni les échecs de contrôle, ni les appels bruts à l’ABI, ni les opérations VFS, IPC ou mémoire, ni les changements de priorité et de nom. Il ne transforme pas la filiation directe en politique de sécurité et reste limité à quatre entrées par parent.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | ABI 69, `MAX_SYSCALLS = 70`, actions et structures publiques |
| `kernel/task/task.[ch]` | anneau local, séquences, générations, projection et branchement aux transitions |
| `kernel/syscall/syscall.[ch]` | adaptateur et dispatch ABI 69 |
| `userspace/shell.c` | wrapper et commande `task-events` |
| `tests/framework/kernel_mocks.c` | miroir du journal, des transitions et du dispatch |
| `tests/unit/kernel/test_task.c` | ordre, éviction, délégation et sortie supervisée |
| `tests/unit/kernel/test_syscall.c` | preuve ABI 69 |
| `tests/scripts/ci_qemu_spawn.py` | preuve Ring 3 du journal observable |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)

[2] [Délégation locale de supervision — lot Foundation 54](mohhdy_foundation_increment_54_supervision_delegation.md)

[3] [Résultat de terminaison d’enfant — lot Foundation 46](mohhdy_foundation_increment_46_child_exit_result.md)
