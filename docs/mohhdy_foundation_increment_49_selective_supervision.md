# Lot Foundation 49 — Supervision sélective des résultats enfants

## Objet

Ce lot rend la fenêtre post-mortem locale exploitable **par enfant**. Le parent peut rechercher un résultat retenu, acquitter une seule entrée et continuer à consulter les autres résultats chronologiques. La commande `wait-result` bénéficie aussi d’un repli : si l’enfant a déjà quitté la file active mais que son résultat est encore retenu, le shell le restitue sans nouvelle attente.

| Élément | Contrat livré |
|---|---|
| ABI ajoutée | `SYS_TASK_CHILD_RESULT_FIND = 60`, `SYS_TASK_CHILD_RESULT_FORGET = 61`, `MAX_SYSCALLS = 62` |
| Recherche Ring 3 | `child-result-any <pid>` |
| Acquittement Ring 3 | `child-results-forget <pid>` |
| Retrait | l’entrée ciblée est supprimée et les entrées restantes sont compactées dans l’ordre chronologique |
| Génération | tout retrait réussi fait progresser la génération d’historique |
| Absence | `OS_TASK_NO_CHILD_RESULT = -69` si le PID ne figure plus dans la fenêtre locale |

## Recherche et repli d’attente

`SYS_TASK_CHILD_RESULT_FIND` cherche le PID demandé dans les quatre résultats retenus du parent appelant. Contrairement à `child-result`, qui désigne le résultat retenu le plus récent, `child-result-any` peut restituer une entrée plus ancienne encore présente dans la fenêtre.

`wait-result <pid>` continue d’abord à appeler l’attente prospective. Si l’enfant est déjà absent de la file, le shell cherche alors le résultat historique exact. Un résultat conservé produit la même forme stable, par exemple `wait-result ok 3 0 1`; une tâche absente sans résultat retenu est refusée explicitement.

> Le repli ne rend pas `wait-result` rétroactif de manière générale : il ne fonctionne que tant que le résultat précis est conservé dans la petite fenêtre locale du parent.

## Acquittement sélectif

`SYS_TASK_CHILD_RESULT_FORGET` retire une seule entrée, réinitialise le stockage circulaire sous une forme compacte et conserve l’ordre des résultats restants. Le dernier résultat retenu est recalculé à partir de la dernière entrée restante ; si la fenêtre devient vide, `child-result` ne retourne plus de résultat.

Le retrait ne cible que l’historique du parent appelant et retourne la nouvelle génération positive. Une observation fondée sur l’ancienne génération devient `OS_TASK_HISTORY_STALE` ; aucune réservation, transaction ou verrouillage n’est apporté.

## Vérification

| Preuve | Résultat |
|---|---|
| `make test-all` | **228/228** tests Unity et robustesse réussis |
| Test de tâche | recherche, forget, compactage, recalcul du dernier résultat et génération stale vérifiés |
| Test ABI | syscalls 60–61, résultat trouvé et génération de forget vérifiés |
| Contrat QEMU `spawn` | repli `wait-result`, recherche du kill, forget `→4`, historique compact, stale puis clear `→5` validés |
| `make qemu-smoke` | core, extras, persistance, création/supervision sélective et exec réussis |
| Budget QEMU `spawn` | porté de 120 s à 150 s pour couvrir les assertions ajoutées, sans assouplir leurs marqueurs |

## Limites

La recherche ne couvre que les quatre résultats retenus du parent direct ; une entrée écrasée, acquittée ou appartenant à un autre parent n’est pas visible. Le budget QEMU de création reste borné à 150 s : son augmentation couvre le temps clavier supplémentaire des assertions sélectionnelles, et non une relance ou un affaiblissement des résultats attendus. Le retrait est séquentiel, local, non atomique et compactant ; il ne propose ni suppression par plage, ni lot atomique, ni audit, ni persistance, ni annulation.

Il n’y a toujours aucun zombie, `waitpid`, statut de signal POSIX, timeout, groupe de processus, héritage d’autorité, journal durable ou attente rétrospective hors de la fenêtre. Un PID réutilisé dans une évolution future n’est pas distingué par une génération de processus ou une identité cryptographique.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | ABI 60–61 |
| `kernel/task/task.[ch]` | recherche, retrait compact et recalcul du dernier résultat |
| `kernel/syscall/syscall.[ch]` | dispatch et adaptateurs sélectifs |
| `userspace/shell.c` | `child-result-any`, `child-results-forget`, repli de `wait-result` |
| `tests/framework/kernel_mocks.c` | miroir des primitives et du dispatch |
| `tests/unit/kernel/test_task.c` | preuves de recherche, forget et stale |
| `tests/unit/kernel/test_syscall.c` | preuves ABI 60–61 |
| `tests/scripts/ci_qemu_spawn.py` | scénario de supervision sélective visible |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)
[2] [Supervision post-mortem — lot Foundation 48](mohhdy_foundation_increment_48_postmortem_supervision.md)
