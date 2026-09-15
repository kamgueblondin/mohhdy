# Lot Foundation 40 — Filiation de tâche et autorité locale

## Objet

Ce lot prolonge la politique CPU locale du lot 39 par un garde-fou minimal contre la mutation arbitraire de priorité. Chaque tâche utilisateur créée par `spawn` ou `exec` porte désormais l’identifiant de son créateur direct. Cette filiation est exposée par `task-metrics` et employée par le noyau pour limiter la commande de priorité à la tâche cible ou à son parent direct.

| Élément | Contrat livré |
|---|---|
| Filiation | `parent_pid` dans `task_t` et `os_task_metrics_t` |
| Racine | `parent_pid = -1` pour la tâche noyau et une tâche sans créateur utilisateur |
| Création | `SYS_SPAWN` et `SYS_EXEC` affectent le PID de la tâche courante à l’enfant |
| Autorité de priorité | cible elle-même ou parent direct uniquement |
| Refus | `OS_TASK_CONTROL_DENIED` (`-64`) |
| Interface | `task-metrics <pid>` affiche `Parent` ; `task-priority` précise le refus |

## Politique appliquée

La validation est effectuée dans `task_set_priority()` avant toute mutation : la priorité doit être dans `1..3`, le PID cible doit exister, puis le PID demandeur doit être identique à celui de la cible ou au `parent_pid` de cette cible. Les codes d’erreur restent distincts : priorité invalide (`-63`), tâche absente (`-62`) et autorité insuffisante (`-64`).

> Le shell n’est pas une autorité de sécurité : l’identité de la tâche courante est injectée par le noyau au point de syscall. Un client Ring 3 ne peut donc pas se déclarer parent en falsifiant un argument de commande.

Le lot conserve le comportement du lot 39. Une tâche haute priorité continuellement prête peut affamer une tâche basse priorité : le mécanisme est une préférence CPU locale, non un planificateur temps réel ou un système de quotas. Le smoke QEMU affecte donc la priorité normale à `idle` après `spawn`, ce qui prouve l’autorité parent-enfant tout en conservant le round-robin requis pour le scénario.

## Portée et limites

La filiation est directe et volatile. Elle ne constitue ni une arborescence complète, ni une capacité transférable, ni une identité authentifiée, ni une ACL, ni une délégation, ni un contrôle d’accès général. L’ancêtre d’un enfant indirect n’a pas de privilège implicite ; un enfant ne peut pas modifier la priorité de son parent, d’un frère ou d’une tâche sans lien. Les opérations `kill`, IPC, VFS, services et accès fichiers ne sont pas réécrites par ce lot.

La filiation n’apporte aucun quota CPU/mémoire/I/O, aucune prévention de famine, aucun vieillissement, aucun cgroup, aucune réservation, aucune comptabilité hiérarchique, aucun nettoyage récursif, aucune isolation d’espace d’adressage supplémentaire ni permission d’administration persistante. Elle établit seulement une frontière de contrôle testée pour la mutation de priorité.

## Vérification

| Preuve | Résultat |
|---|---|
| `make test-all` | **221/221** tests Unity et robustesse réussis |
| Test de tâche | parent direct et cible autorisés ; enfant, pair et tâche sans lien refusés ; parent visible dans les métriques |
| Test syscall | une tâche sans lien reçoit `OS_TASK_CONTROL_DENIED` et la priorité cible reste inchangée |
| `make qemu-smoke` | smoke BIOS complet réussi ; `spawn idle`, `task-metrics <pid>` confirme `Parent : 1`, puis le shell applique `task-priority <pid> 2` avant `yield`, `ps` et `kill` |

Le parseur du smoke `spawn` accepte l’entrelacement d’un changement de contexte dans la sortie série `spawn ok pid`, en recherchant la terminaison stable `<pid> idle`. Cette adaptation rend le test fidèle au comportement réel de l’ordonnanceur préemptif sans relâcher ses assertions fonctionnelles.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | `parent_pid` dans les métriques et code `OS_TASK_CONTROL_DENIED` |
| `kernel/task/task.[ch]` | stockage de la filiation, publication métrique et vérification d’autorité |
| `kernel/syscall/syscall.c` | attribution du parent dans `spawn`/`exec`, identité courante transmise au setter |
| `userspace/shell.c` | parent affiché et erreur d’autorité lisible |
| `tests/framework/kernel_mocks.c` | double de filiation et dispatch conforme |
| `tests/unit/kernel/test_task.c` | preuves d’autorité parent, cible et refus |
| `tests/unit/kernel/test_syscall.c` | refus ABI depuis une tâche sans lien |
| `tests/scripts/ci_qemu_spawn.py` | filiation et mutation autorisée dans l’image bootée |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)
[2] [Politique CPU par tâche — lot Foundation 39](mohhdy_foundation_increment_39_task_priority.md)
