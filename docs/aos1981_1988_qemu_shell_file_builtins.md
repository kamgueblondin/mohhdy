# AOS-1981 à AOS-1988 — Smoke QEMU des builtins shell de fichiers

## Objectif

Ce macro-lot transforme les commandes shell `sort`, `head` et `tail` en contrats de régression QEMU réellement exécutés. Elles étaient présentes dans le shell, mais le scénario syscall ne vérifiait auparavant que `grep` et `wc`. Le test s’exécute désormais dans la séquence `qemu-smoke` utilisée par la CI.

| Commande | Fixture | Assertion de comportement |
|---|---|---|
| `wc hi.txt` | `ping\n` | Le compteur publié reste `1` ligne, `1` mot et `5` caractères. |
| `sort lines.txt` | `zulu\nalpha\nmango\n` | La sortie doit conserver l’ordre lexicographique `alpha`, `mango`, `zulu`. |
| `head -2 lines.txt` | même fixture | La sortie doit être exactement les deux premières lignes : `zulu`, puis `alpha`. |
| `tail -1 lines.txt` | même fixture | La sortie doit être exactement la dernière ligne : `mango`. |

## Scénario de test

Le script [`ci_qemu_syscalls.py`](../tests/scripts/ci_qemu_syscalls.py) produit la fixture uniquement par les commandes publiques `write` et `append`. Il emploie les frappes HMP, attend le marqueur de succès de chaque commande puis vérifie les fragments de sortie ordonnés. Les fichiers transitoires `lines.txt` et `hi.txt` sont supprimés avant le contrôle final du répertoire.

> Le contrôle ne se limite pas aux marqueurs `sort ok`, `head ok` et `tail ok` : il valide les lignes émises dans leur ordre, ce qui détecte une permutation, une sélection incorrecte ou une fuite de ligne.

Le harnais démarre désormais QEMU avec `QEMU_MEMORY=1024M` par défaut, paramétrable par l’environnement. Cette valeur est cohérente avec le runtime GPT-2 local et les pools VMM statiques ; elle évite un échec de création de la tâche shell avant l’injection des commandes. Le délai de commande par défaut est porté à 30 secondes pour laisser terminer la génération locale sous QEMU TCG. L’attendu IA est le préfixe réellement produit par le chemin local, `[GPT-2 local]`, et non le marqueur de l’ancien chemin de compatibilité.

Chaque ligne injectée est désormais reconstruite à partir des touches HMP puis comparée à l’écho `SYS_GETS: ligne lue:` du guest. En cas de double scan-code PS/2 sous TCG, le harnais réémet la ligne complète, dans la limite configurable `KEY_RETRIES=3`, avant toute assertion métier. La garde demeure observable dans le journal et ne transforme pas une sortie de commande erronée en succès.

## Intégration CI

L’orchestrateur [`ci_qemu_smoke.sh`](../tests/scripts/ci_qemu_smoke.sh) ajoute un boot `syscall` isolé, avec un timeout configurable `SYSCALL_TIMEOUT` de 300 secondes. Il réinitialise le disque d’overlay avant ce boot, ce qui préserve l’indépendance des scénarios core, extras, persistance, spawn, syscalls et exec.

| Propriété | Garantie |
|---|---|
| Isolation | Chaque sous-scenario démarre un guest frais avec un overlay réinitialisé, sauf le test explicite de persistance. |
| Déterminisme | Les données sont courtes, ASCII et créées par le shell lui-même ; aucun service réseau n’est requis. Les lignes HMP sont validées par leur écho série avant l’assertion. |
| Couverture réelle | Les commandes sont traitées par le shell ELF Ring 3 dans QEMU, et non par une réimplémentation de test hôte. |
| Mémoire | Aucun chemin noyau ou shell n’ajoute d’allocation dynamique ; le changement ne concerne que la capacité QEMU du harnais. |

## Validation attendue

La validation locale exécute la compilation syntaxique Python, la vérification Bash, `make -s test-all`, `make -s kernel-only` et `make -s qemu-smoke`. La dernière commande inclut désormais le scénario syscall enrichi et doit conclure par les marqueurs `sort ok 3 lines.txt`, `head ok 2 lines.txt` et `tail ok 1 lines.txt`.

## Références

[1] [Implémentations shell `wc`, `sort`, `head` et `tail`](../userspace/shell.c)

[2] [Scénario QEMU syscall](../tests/scripts/ci_qemu_syscalls.py)

[3] [Orchestrateur QEMU de CI](../tests/scripts/ci_qemu_smoke.sh)
