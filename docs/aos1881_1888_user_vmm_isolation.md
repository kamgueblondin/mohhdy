# AOS-1881…1888 — Isolation VMM utilisateur et rollback de création Ring 3

## Objet

Ce macro-lot corrige le prérequis de propriété mémoire qui empêchait toute libération sûre d’un espace utilisateur. Un répertoire créé pour une tâche Ring 3 héritait des pointeurs de tables du noyau ; un mapping ELF ou de pile utilisateur pouvait alors modifier une table partagée. La destruction ultérieure de cette table aurait risqué d’endommager les mappings noyau.

Le VMM privatise désormais la table concernée avant tout mapping marqué `PAGE_USER`. La copie conserve les entrées noyau visibles par la tâche, tandis que l’entrée utilisateur est écrite uniquement dans la table privée. Un bitmap compact de 32 mots identifie les tables possédées par le répertoire utilisateur.

| Élément | Garantie |
|---|---|
| Mapping `PAGE_USER` | Clonage préalable de la table, puis écriture isolée. |
| Tables noyau héritées | Conservées et jamais libérées par le destructeur utilisateur. |
| Pages ELF et pile | Restituées au PMM uniquement si marquées présentes et utilisateur. |
| Répertoire actif ou noyau | Refusé par `vmm_destroy_user_directory()`. |

## Rollback transactionnel

`vmm_map_page_in_directory()` retourne désormais un statut. Le chargeur ELF restitue la page physique immédiatement si son mapping échoue ; l’appelant restaure ensuite l’espace noyau avant de détruire le répertoire utilisateur partiellement construit. L’allocation de pile utilisateur applique le même contrat, et les échecs d’allocation de structure de tâche ou de pile noyau déclenchent aussi la destruction du répertoire déjà créé.

> Le nettoyage ne s’exécute qu’après le retour explicite au répertoire précédent. Il ne peut donc pas détruire l’espace d’adressage encore utilisé par le processeur.

Les tables privées sont elles-mêmes fournies par le PMM et restituées avec `pmm_free_page()`. Le chemin ajouté ne crée aucune nouvelle allocation de tas ; le bitmap de propriété n’ajoute que 128 octets par répertoire VMM.

## Validation

La compilation noyau et la suite noyau complète ont été exécutées après la mise en place des tables privées, puis après leur migration du tas vers le PMM. Les 38 suites unitaires noyau restent vertes, y compris la suite de tâches qui exerce les allocations de masse de sa fixture.

| Commande | Résultat |
|---|---:|
| `make -s kernel-only` | Réussi |
| `make -s test-kernel` | 38/38 suites réussies |
| Vérification de tables privées PMM | Réussi à la compilation et aux tests de tâches |

## Références

[1] [Intel 64 and IA-32 Architectures Software Developer’s Manual — Paging](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
