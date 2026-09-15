# AOS-1925…1932 — Tables VMM allouées par le PMM

## Objet

La création à la demande d’une table de pages dans `vmm_get_page()` utilisait encore une allocation de tas alignée. Ce lot la remplace par une page allouée directement par le PMM, conformément au modèle de propriété des tables de pagination du noyau.

| Chemin VMM | Fournisseur de mémoire |
|---|---|
| Répertoire noyau | PMM |
| Tables d’identité du noyau | PMM |
| Table créée à la demande | PMM |
| Table utilisateur privatisée | PMM |

La table obtenue est toujours mise à zéro avant publication dans le répertoire. Les conteneurs VMM dynamiques historiques restent pris en charge par le destructeur, tandis que les tables de pages créées pendant le mapping sont physiquement alignées et gérées par le PMM.

> Les structures de pagination sont désormais allouées selon un modèle unique : pages PMM alignées et contrôlées, sans dépendance au tas pour créer une table VMM.

## Validation

| Commande | Résultat |
|---|---:|
| `make -s test-kernel` | 38/38 suites réussies |
| `make -s test-all` | 479/479 tests réussis |
| `make -s kernel-only` | Réussi |
| Recherche d’allocations dans `kernel/mem/vmm.c` | Aucun appel d’allocation de tas |
| `git diff --check` | Réussi |

## Référence

[1] [Intel 64 and IA-32 Architectures Software Developer’s Manual — Paging](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
