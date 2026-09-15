# AOS-1897…1904 — Réclamation forcée des tâches Ring 3 détachées

## Objet

Les opérations `task_kill()` et `task_kill_direct_children()` détachaient déjà les tâches cibles de la file d’ordonnancement, mais les ressources de tâches Ring 3 réelles pouvaient rester retenues jusqu’à une réclamation future. Ce lot applique la même primitive de libération contrôlée aux tâches supprimées hors contexte actif.

| Chemin | Traitement après détachement |
|---|---|
| Sortie de la tâche courante | Réclamation différée au passage ultérieur du scheduler. |
| `task_kill()` sur un enfant non courant | Réclamation immédiate après détachement. |
| `task_kill_direct_children()` | Réclamation immédiate de chaque enfant détaché. |

## Sûreté

La primitive factorisée ne libère une tâche que si elle est Ring 3 et possède une pile noyau réelle. Cette garde évite d’interpréter les fixtures unitaires ou des structures incomplètes comme des contextes de production. Pour une tâche réelle, elle détruit le VMM utilisateur par le destructeur qui refuse tout espace actif, restitue la pile noyau puis libère la structure de tâche.

> Les suppressions forcées ne s’exécutent jamais depuis le VMM ni depuis la pile de la tâche cible : la cible est distincte de la tâche qui émet la demande.

La réclamation ne crée aucune allocation dynamique. Elle restitue seulement les ressources déjà attribuées au cycle de vie Ring 3.

## Validation

| Commande | Résultat |
|---|---:|
| `make -s test-all` | 479/479 tests réussis |
| `make -s kernel-only` | Réussi |
| `git diff --check` | Réussi |

La suite de tâches, les composants VMM/PMM, le réseau et les tests GPT-2 restent tous verts sous le gate de non-régression.

## Référence

[1] [Intel 64 and IA-32 Architectures Software Developer’s Manual — Task Switching and Paging](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
