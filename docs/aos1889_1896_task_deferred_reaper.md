# AOS-1889…1896 — Réclamation différée des tâches Ring 3 terminées

## Objet

Ce macro-lot complète l’isolation VMM précédente par une réclamation différée des ressources d’une tâche Ring 3 terminée. Une tâche ne peut pas libérer sa propre pile noyau ni son espace d’adressage pendant que le processeur l’utilise. Le scheduler la détache donc immédiatement de la file circulaire, sélectionne une remplaçante, puis conserve son pointeur dans un emplacement de réclamation différée.

La libération est exécutée au prochain passage dans l’ordonnanceur, après qu’une autre tâche et son répertoire VMM sont devenus actifs.

| Moment | Action |
|---|---|
| Détection de `TASK_TERMINATED` | La tâche est détachée de la file ; son état CPU n’est pas sauvegardé. |
| Bascule | Le scheduler active la tâche de remplacement, sa pile noyau et son répertoire VMM. |
| Passage de scheduler suivant | Le reaper libère le VMM utilisateur inactif, la pile noyau et la structure de tâche. |

## Invariants de sûreté

Le reaper ignore les tâches noyau et ne détruit un répertoire utilisateur que par `vmm_destroy_user_directory()`, lequel refuse le répertoire noyau et tout répertoire encore actif. Les ressources ne sont donc jamais libérées depuis la pile de la tâche concernée, ni depuis le VMM qu’elle exécute encore.

> Le détachement de la file et la libération mémoire sont volontairement séparés par un changement de contexte effectif.

Aucune allocation dynamique n’est ajoutée au reaper : il ne fait que restituer les ressources déjà possédées par la tâche. Les allocations historiques de création de tâche restent inchangées.

## Validation

La suite complète a été exécutée après l’intégration du reaper. Les modules tâches, VMM, PMM, systèmes de fichiers, GPT-2 et réseau restent couverts par le même gate de non-régression.

| Commande | Résultat |
|---|---:|
| `make -s test-all` | 479/479 tests réussis |
| `make -s kernel-only` | Réussi |
| `git diff --check` | Réussi |

## Référence

[1] [Intel 64 and IA-32 Architectures Software Developer’s Manual — Task Switching and Paging](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
