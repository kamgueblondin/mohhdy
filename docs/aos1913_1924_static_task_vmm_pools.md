# AOS-1913…1924 — Pools statiques pour tâches Ring 3, piles noyau et VMM

## Objet

Ce macro-lot supprime les allocations de tas du cycle de vie des tâches Ring 3. Les structures de tâche, leurs piles noyau et les conteneurs de répertoire VMM sont désormais fournis par des pools statiques bornés à `OS_TASK_GLOBAL_CAPACITY`.

| Ressource | Avant | Après |
|---|---|---|
| Structure `task_t` | Allocation de tas à la création | Slot statique réutilisable |
| Pile noyau Ring 3 | Allocation de 4 Kio à la création | Bloc statique aligné par slot (16 Kio depuis le lot TLS/HTTP local, pour les instantanes handshake) |
| `vmm_directory_t`, pointeurs de tables et répertoire matériel | Allocations de tas distinctes | Conteneurs statiques par slot |
| Tables privées utilisateur | N/A | Pages PMM caller-owned, libérées au nettoyage |

## Sémantique

Le pool partage la borne déjà exposée par le système : `OS_TASK_GLOBAL_CAPACITY = 16`. L’acquisition initialise le slot entièrement ; la libération intervient seulement après la destruction réussie du VMM utilisateur, puis réinitialise le slot pour une réutilisation ultérieure.

Les conteneurs VMM statiques portent un marqueur explicite. `vmm_destroy_user_directory()` continue de restituer les pages utilisateur et les tables privées fournies par le PMM, mais ne passe jamais les conteneurs statiques à `kfree()`.

> La mémoire de contrôle des tâches est bornée à la compilation ; seuls les cadres physiques mappés aux segments ELF, aux piles utilisateur et aux tables privées transitent encore par le PMM, puis sont restitués au nettoyage.

## Garanties

| Invariant | Garantie |
|---|---|
| Capacité | Aucun dépassement : un slot libre est requis avant création. |
| Rollback | Un échec ELF, pile utilisateur ou initialisation rend le VMM statique au pool. |
| Sortie / `kill` | La réclamation VMM précède la remise à disposition du slot de tâche. |
| Allocation de tas dans `task.c` | Aucune. |
| Mappings noyau | Conservés partagés ; seules les tables utilisateur privées sont détruites. |

## Validation

| Commande | Résultat |
|---|---:|
| `make -s test-kernel` | 38/38 suites réussies |
| `make -s test-all` | 479/479 tests réussis |
| `make -s kernel-only` | Réussi |
| Recherche d’allocations dans `kernel/task/task.c` | Aucun appel `kmalloc`, `kfree`, `malloc`, `calloc` ou `realloc` |
| `git diff --check` | Réussi |

## Référence

[1] [Intel 64 and IA-32 Architectures Software Developer’s Manual — Paging and Task Switching](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
