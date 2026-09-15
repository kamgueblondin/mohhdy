# AOS-1965…1972 — Rollback transactionnel du chargement ELF

## Objet

Le chargeur ELF retourne immédiatement un échec lorsqu’une page physique ne peut pas être obtenue ou mappée. La remarque historique suggérant un nettoyage local restait obsolète depuis la mise en place du cycle de vie VMM transactionnel.

| Étape | Garantie |
|---|---|
| Chargement ELF | Toute erreur retourne `0` sans publier de tâche. |
| Appelant des tâches | Restaure d’abord le répertoire de pages actif précédent. |
| Destruction VMM | `task_destroy_user_vmm()` détruit alors le répertoire partiel hors contexte actif. |
| Restitution mémoire | Les pages utilisateur mappées et les tables privées sont rendues au PMM. |

> Le rollback reste centralisé au niveau du propriétaire du répertoire VMM. Le chargeur n’essaie donc jamais de détruire l’espace d’adressage qu’il venait d’activer.

## Validation

| Commande | Résultat |
|---|---:|
| `make -s test-all` | 479/479 tests réussis |
| `git diff --check` | Réussi |
| Recherche du TODO de rollback ELF | Aucune occurrence |

## Référence

[1] [System V ABI — ELF gABI](https://refspecs.linuxfoundation.org/elf/gabi4+/contents.html)
