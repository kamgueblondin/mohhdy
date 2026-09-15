# Lot Foundation 39 — Politique CPU par tâche

## Objet

Ce lot transforme la télémétrie de tâche livrée précédemment en premier contrôle de ressources actionnable. Il ajoute une priorité CPU locale, configurable par PID, et l’intègre à la sélection des tâches Ring 3 prêtes. Le mécanisme reste volontairement borné et pédagogique : il ne prétend pas implémenter un planificateur CFS, des quotas ni une allocation prédictive de ressources.

| Élément | Contrat livré |
|---|---|
| ABI | `SYS_TASK_SET_PRIORITY = 52`, `MAX_SYSCALLS = 53` |
| Entrée | `EBX = PID`, `ECX = priorité` |
| Plage valide | `1` bas, `2` normal, `3` haut |
| Observation | `os_task_metrics_t.priority` et `task-metrics <pid>` |
| Commande | `task-priority <pid> <1|2|3>` |
| Erreurs | `OS_TASK_NOT_FOUND` (`-62`) et `OS_TASK_BAD_PRIORITY` (`-63`) |

## Politique d’ordonnancement

Le noyau conserve la règle de sûreté existante : une tâche Ring 3 prête est préférée à la tâche noyau après le premier saut vers le shell, afin de ne jamais réutiliser un ancien cadre utilisateur pour le noyau. Parmi les tâches Ring 3 `READY`, il choisit la priorité numérique la plus élevée. Lorsque plusieurs candidates ont la même priorité, le parcours circulaire commence après la tâche courante : le round-robin historique départage donc les égales sans introduire d’ordre fixe.

La priorité par défaut de toute nouvelle tâche est **normale** (`2`). Le réglage n’induit pas de rescheduling forcé au milieu d’un quantum : il s’applique au prochain passage sûr dans `schedule()`. L’écriture de priorité ne modifie ni les ticks d’exécution, ni le compteur de commutations, ni l’état de cycle de vie de la tâche.

> Une priorité haute donne une préférence locale de sélection, pas une garantie de temps CPU. Une tâche haute priorité continuellement prête peut retarder une tâche basse priorité ; cette première tranche ne comprend ni vieillissement, ni quota ni prévention de famine.

## Sécurité et limites

Le système reste un prototype monolithique sans identité de processus, capability, ACL ou isolateur de ressources. Toute tâche Ring 3 pouvant invoquer le syscall peut modifier la priorité d’un PID vivant : il s’agit d’une politique d’administration locale, non d’une délégation sécurisée. Les comptes de télémétrie associés restent non atomiques, volatils et limités aux compteurs PIT 32 bits ; la métrique PMM demeure globale au système et ne mesure pas une mémoire privée par processus.

La fonctionnalité ne fournit pas de CFS, de délai maximum, de priorité temps réel, de réservation CPU, de budget mémoire ou I/O, de classes de service, d’apprentissage de charge, de contrôle énergétique, de SMP ni de préemption dans les cadres noyau. Elle constitue une étape vérifiable de l’US-002, pas la réalisation du gestionnaire de ressources intelligent décrit par la vision MOHHDY.

## Vérification

| Preuve | Résultat |
|---|---|
| `make test-all` | **220/220** tests Unity et robustesse réussis |
| Test de tâche | sélection de la tâche Ring 3 de priorité haute, visibilité de l’instantané, rejet des bornes et PID absent |
| Test syscall | priorité appliquée via `SYS_TASK_SET_PRIORITY`, restituée via `SYS_TASK_METRICS`, valeur invalide sans mutation |
| `make qemu-smoke` | smoke BIOS complet réussi ; le shell exécute `task-priority 1 3` puis confirme `task-metrics ok 1 3` |

## Utilisation

```text
ps
task-priority <pid> 1
task-priority <pid> 2
task-priority <pid> 3
task-metrics <pid>
```

Les valeurs hors de `1..3` sont rejetées. `task-metrics` expose la priorité effective dans l’instantané retourné.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | Syscall 52, constantes de priorité, erreur et champ métrique |
| `kernel/task/task.[ch]` | Priorité par tâche, choix de la plus haute priorité et setter borné |
| `kernel/syscall/syscall.[ch]` | Dispatch et adaptation ABI |
| `userspace/shell.c` | Wrapper, commande, aide et affichage |
| `tests/framework/kernel_mocks.c` | Double d’ordonnanceur et dispatch conformes |
| `tests/unit/kernel/test_task.c` | Preuve de sélection de priorité |
| `tests/unit/kernel/test_syscall.c` | Preuve ABI de réglage et observation |
| `tests/scripts/ci_qemu_shell_extras.py` | Preuve bootée du shell |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)
[2] [Télémétrie par tâche — lot Foundation 38](mohhdy_foundation_increment_38_task_metrics.md)
