# Lot Foundation 45 — Gouvernance observable des tâches

## Objet

Ce lot rassemble trois garanties locales de gouvernance des tâches : un **nom mutable et observable**, une **capacité globale bornée** et une **notification IPC best-effort** au parent direct lors de la sortie d’un enfant. Il prolonge la filiation, la supervision et la capacité locale livrées dans les lots 40 à 44 sans modifier leurs règles d’autorité.

| Élément | Contrat livré |
|---|---|
| ABI ajoutée | `SYS_TASK_SET_NAME = 54`, `SYS_TASK_CAPACITY = 55` ; `MAX_SYSCALLS = 56` |
| Renommage | `task-name <pid> <nom>` pour soi ou un enfant direct |
| Validation de nom | 1 à 31 caractères ASCII imprimables ; `OS_TASK_BAD_NAME = -67` sinon |
| Capacité globale | `OS_TASK_GLOBAL_CAPACITY = 16` tâches actives, noyau inclus |
| Refus global | `OS_TASK_GLOBAL_LIMIT = -68`, avant lecture de l’initrd, chargement ELF ou allocation |
| Observation | `task-capacity` retourne actif, capacité et disponibilité |
| Événement de sortie | charge `OS_IPC_TASK_EVENT` émise par le noyau vers le parent direct |

## Renommage et autorité

`task_set_name()` applique la même autorité locale que `task_set_priority()` : la tâche cible peut se renommer elle-même et son parent direct peut la renommer. Les pairs, ancêtres indirects et enfants ne possèdent aucun droit de mutation implicite. La sortie `ps` reflète ensuite immédiatement le nouveau nom.

Le nom est volontairement borné à la largeur déjà utilisée par `os_proc_t` et `task_t`. Les caractères de contrôle, les noms vides et les chaînes de 32 caractères ou plus sont refusés. Le nom ne constitue ni une identité, ni une capability, ni une protection contre l’usurpation.

## Capacité globale

La file active est maintenant limitée à seize tâches, y compris la tâche noyau. `SYS_EXEC` et `SYS_SPAWN` vérifient d’abord la capacité directe du parent puis la capacité globale ; les deux refus interviennent avant l’allocation d’un nouvel espace d’adressage, d’une pile utilisateur ou d’une structure de tâche. `create_task_from_initrd_file()` répète le garde-fou afin que le chemin noyau de chargement reste borné.

`SYS_TASK_CAPACITY` remplit `os_task_capacity_t`, un instantané avec `active`, `capacity` et `available`. Il ne réserve aucune place : une création concurrente, une terminaison ou un changement de file peut modifier sa valeur avant l’appel suivant.

## Événement parent-enfant

Quand un enfant utilisateur quitte normalement ou est retiré par un `kill` autorisé, le noyau produit un message synthétique de type `OS_IPC_TASK_EVENT` avec `sender_pid = 0`, le PID de l’enfant et l’une des raisons suivantes.

| Raison | Valeur | Déclencheur |
|---|---:|---|
| `OS_TASK_EVENT_EXITED` | 1 | `SYS_EXIT` de l’enfant |
| `OS_TASK_EVENT_KILLED` | 2 | `kill` autorisé par le parent direct |

Le message est ajouté **avant** le réveil éventuel du waiter et avant la réattribution des descendants. Le shell le restitue via `ipc-recv`, par exemple `task-event child 3 reason exited`.

> La notification est best-effort : une boîte IPC parent pleine la perd silencieusement et elle ne retarde jamais la terminaison. Elle n’est pas persistante, accusée, ordonnée contre les autres messages, ni un substitut à `waitpid`, aux zombies, aux signaux ou aux codes de sortie.

## Vérification

| Preuve | Résultat |
|---|---|
| `make test-all` | **228/228** tests Unity et robustesse réussis |
| Tests de tâches | nommage autorisé/refusé, capacité vide et saturée, événement `exited`, événement `killed` |
| Test ABI | dispatch des syscalls 54 et 55, autorité et contenu d’instantané |
| Contrat QEMU `spawn` | `task-name`, `task-capacity`, `ps` renommé et réception des deux événements validés |
| `make qemu-smoke` | core, extras, persistance, création/gouvernance/supervision et exec réussis |

## Limites

La capacité est fixe, globale, volatile et non atomique. Elle ne représente ni quota mémoire, CPU, I/O ou IPC, et ne récupère pas encore les ressources VMM allouées par un chargement échoué. Le compteur ne compte pas les tâches terminées déjà retirées et ne propose aucune attente, souscription, réservation ou politique d’admission.

La notification ne cible que le parent direct encore vivant et utilisateur. Elle n’est pas émise vers les ancêtres, pairs ou services, ne contient aucun statut de sortie, horodatage, nom ni compteur, et peut être perdue si la file de quatre messages est pleine. `wait` reste indépendant : son réveil ne dépend pas de la réception de l’événement.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | ABI 54–55, erreurs, capacité globale et format d’événement |
| `kernel/task/task.[ch]` | plafond, instantané, renommage et émission IPC parent |
| `kernel/syscall/syscall.[ch]` | dispatch, adaptateurs et gardes `spawn`/`exec` |
| `userspace/shell.c` | commandes `task-name`, `task-capacity` et affichage `ipc-recv` |
| `tests/framework/kernel_mocks.c` | miroir de gouvernance et dispatch ABI |
| `tests/unit/kernel/test_task.c` | preuves de capacité, nommage et événements |
| `tests/unit/kernel/test_syscall.c` | preuve ABI des appels 54–55 |
| `tests/scripts/ci_qemu_spawn.py` | scénario QEMU de gouvernance visible |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)
[2] [Capacité locale d’enfants — lot Foundation 44](mohhdy_foundation_increment_44_child_capacity.md)
