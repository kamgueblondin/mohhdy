# Lot Foundation 41 — Cycle de vie parent-enfant

## Objet

Ce lot étend la filiation directe introduite par le lot 40 à la terminaison des tâches. Le PID parent est maintenant visible dans la sortie `ps`, et la primitive noyau de suppression vérifie l’identité du demandeur avant de retirer une tâche de la file. Un parent direct peut terminer son enfant ; une tâche sans lien est refusée. Les protections historiques de la tâche noyau et de l’auto-terminaison par `kill` sont conservées.

| Élément | Contrat livré |
|---|---|
| Hiérarchie visible | `os_proc_t.parent_pid` et colonne `PPID` dans `ps` |
| Autorisation de `kill` | parent direct uniquement |
| Auto-terminaison via `kill` | refus existant (`-3`) conservé |
| Tâche noyau | PID 0 toujours protégé (`-2`) |
| Tâche inconnue | refus existant (`-1`) |
| Tâche sans lien | `OS_TASK_CONTROL_DENIED` (`-64`) |
| Nettoyage services | exécuté seulement après une terminaison autorisée |

## Politique appliquée

`sys_kill()` transmet le PID de la tâche courante à `task_kill()`. Après les contrôles de PID noyau, d’existence et d’auto-terminaison, le noyau n’autorise la suppression que si le demandeur est le `parent_pid` de la cible. Le réveil éventuel d’un parent `exec`, le retrait de la file et le nettoyage du registre de services conservent leur ordre antérieur, mais ne sont atteints qu’en cas d’autorisation réussie.

La commande `ps` affiche `PID`, `PPID`, état, type et nom. `task-metrics <pid>` reste l’instantané détaillé de référence et expose le même parent.

> La limite est volontairement étroite : l’autorité ne se transmet pas aux ancêtres, ne descend pas récursivement et ne s’applique pas à IPC, VFS, services, mémoire ou réseau.

## Limites

Ce contrôle est une garde locale de cycle de vie, pas un modèle de processus complet. Il n’existe pas de groupe de processus, session, signal, zombie, waitpid, adoption d’orphelin, terminaison récursive, capability, ACL, identité authentifiée, délégation, journal persistant ou politique multi-utilisateur. Un parent tué ne réattribue pas ses enfants ; la filiation est directe, volatile et non persistante.

Les limites du lot 39 restent inchangées : la priorité CPU `1..3` ne garantit pas de temps processeur et une tâche haute priorité continuellement prête peut affamer une tâche basse priorité. La limitation du `kill` ne fournit ni quota CPU/mémoire/I/O ni isolation d’espace d’adressage supplémentaire.

## Vérification

| Preuve | Résultat |
|---|---|
| `make test-all` | **222/222** tests Unity et robustesse réussis |
| Test de tâche | un parent termine son enfant ; un pair reçoit `OS_TASK_CONTROL_DENIED` ; auto-terminaison refusée |
| Test syscall | `ps` restitue `parent_pid` ; un parent termine son enfant et ne peut terminer une tâche sans lien |
| `make qemu-smoke` | smoke BIOS complet réussi ; après `spawn idle`, le shell confirme `Parent : 1`, `ps` confirme `PID 2 / PPID 1`, puis `kill 2` retire l’enfant |

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | parent ajouté à `os_proc_t` |
| `kernel/task/task.[ch]` | terminaison avec demandeur et parent restitué dans `ps` |
| `kernel/syscall/syscall.c` | PID courant transmis, nettoyage post-autorisation |
| `userspace/shell.c` | colonne `PPID` de `ps` |
| `tests/framework/kernel_mocks.c` | modèle Unity conforme |
| `tests/unit/kernel/test_task.c` | preuve directe de cycle de vie parent-enfant |
| `tests/unit/kernel/test_syscall.c` | hiérarchie et refus ABI |
| `tests/scripts/ci_qemu_spawn.py` | assertion PPID dans l’image bootée |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)
[2] [Filiation et autorité locale — lot Foundation 40](mohhdy_foundation_increment_40_task_authority.md)
