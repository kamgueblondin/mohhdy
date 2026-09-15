# Lot Foundation 42 — Réattribution des enfants

## Objet

Ce lot rend la filiation de tâches durable lorsqu’un parent quitte le système. Avant le retrait d’une tâche volontairement sortante ou terminée par son parent, le noyau réattribue chacun de ses enfants directs au propre parent du sortant. Si le sortant est une racine de filiation, ses enfants deviennent explicitement des racines avec `parent_pid = -1`.

| Situation du parent sortant | Parent assigné à l’enfant direct |
|---|---|
| Parent avec un parent vivant ou référencé | Le parent du sortant |
| Parent racine (`parent_pid = -1`) | `-1` : enfant devenu racine |
| Descendant indirect | Inchangé jusqu’au départ de son parent direct |

## Politique appliquée

La primitive `task_reparent_children()` parcourt la file circulaire des tâches avant le retrait de la tâche sortante. Elle ne modifie que les tâches dont le `parent_pid` est exactement le PID de la tâche qui part. Cette règle est appelée sur les deux chemins de retrait : `SYS_EXIT` et `task_kill()` après autorisation, avant le changement d’état et le retrait de la file.

> La réattribution est une conservation de cohérence de filiation. Elle ne confère pas rétroactivement une capacité : l’autorité de priorité et de terminaison reste limitée au parent direct désormais enregistré.

La remontée est progressive. Si un parent intermédiaire disparaît, son enfant est d’abord réattribué au grand-parent. Si cet enfant disparaît ensuite, ses propres enfants sont à leur tour réattribués au même grand-parent. Aucun parcours récursif ou mutation d’ancêtre indirect n’est effectué en une seule opération.

## Limites

Le mécanisme ne fournit ni adoption par une tâche d’initialisation, ni `waitpid`, ni zombies, ni groupes de processus, ni sessions, ni signaux, ni terminaison en cascade, ni nettoyage récursif, ni compteur de descendants, ni persistance de la hiérarchie, ni capacité transférable, ni identité vérifiée. Une racine qui se termine laisse ses enfants racines ; il n’existe pas de superviseur automatique.

Les règles antérieures restent inchangées : `kill` est réservé au parent direct, une tâche ne se termine pas par son propre `kill`, la tâche noyau demeure protégée, et les priorités restent un mécanisme local sans garantie de latence ni prévention de famine.

## Vérification

| Preuve | Résultat |
|---|---|
| `make test-all` | **223/223** tests Unity et robustesse réussis |
| Test de tâche | départ d’un parent intermédiaire : l’enfant est réattribué au grand-parent ; départ suivant : le petit-enfant est réattribué au même grand-parent ; enfant d’une racine : `parent_pid = -1` |
| `make qemu-smoke` | smoke BIOS complet réussi, y compris `spawn`, PPID, priorité parent-enfant, `kill`, persistance et `exec` |

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `kernel/task/task.[ch]` | primitive de réattribution et appel avant retrait forcé |
| `kernel/syscall/syscall.c` | appel avant retrait sur `SYS_EXIT` |
| `tests/framework/kernel_mocks.c` | double Unity de la réattribution |
| `tests/unit/kernel/test_task.c` | preuve de remontée progressive et de racine orpheline |

## Références

[1] [État réel de MOHHDY](ETAT_REEL.md)
[2] [Cycle de vie parent-enfant — lot Foundation 41](mohhdy_foundation_increment_41_task_lifecycle_authority.md)
