# AOS-1905…1912 — Contrat public de retrait de tâche hors contexte actif

## Objet

Le fichier d’en-tête des tâches exposait `remove_task()` sans implémentation de production. Ce lot complète ce contrat public pour les appels souhaitant retirer une tâche **non courante** de la file d’ordonnancement.

La fonction vérifie d’abord que la cible est non nulle, que la file existe, qu’elle ne désigne pas la tâche courante et qu’elle appartient effectivement à la liste circulaire. Elle la détache ensuite par le primitive interne unique et réutilise la réclamation sûre des ressources Ring 3.

| Cas | Résultat |
|---|---|
| Cible nulle, file vide ou cible absente | Aucun effet. |
| Tâche courante | Refus implicite : le scheduler reste le seul chemin de sortie sûr. |
| Tâche non courante de la file | Détachement atomique ; les ressources Ring 3 réelles sont réclamées. |

## Sûreté

La garde sur la tâche courante évite de libérer une pile noyau ou un VMM encore actifs. Les tâches Ring 3 réelles sont traitées par la primitive déjà employée par les chemins de sortie différée et de suppression forcée. Aucun buffer ni allocation supplémentaire n’est introduit.

> Le contrat public de retrait ne court-circuite pas le changement de contexte : seule une tâche hors exécution peut être détachée par cet appel.

## Validation

| Commande | Résultat |
|---|---:|
| `make -s test-kernel` | 38/38 suites réussies |
| `make -s test-all` | 479/479 tests réussis |
| `make -s kernel-only` | Réussi |
| `git diff --check` | Réussi |

## Référence

[1] [Intel 64 and IA-32 Architectures Software Developer’s Manual — Task Switching](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
