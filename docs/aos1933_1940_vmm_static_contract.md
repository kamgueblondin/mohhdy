# AOS-1933…1940 — Contrat VMM sans tas dynamique

## Objet

Après le transfert des tables de pagination vers le PMM et des répertoires utilisateur vers des pools statiques, le VMM ne doit plus conserver de chemin historique vers le tas. Ce lot verrouille explicitement ce contrat.

`vmm_destroy_user_directory()` accepte uniquement un répertoire utilisateur identifié comme conteneur statique. Il restitue les pages utilisateur et les tables privées au PMM, mais ne tente jamais de libérer le conteneur VMM, sa table de pointeurs ou son répertoire matériel par le tas.

| Répertoire | Origine | Destruction autorisée |
|---|---|---|
| Noyau | PMM | Refusée |
| Actif | Toute origine | Refusée |
| Utilisateur statique | Pool de tâches | Pages et tables privées PMM uniquement |
| Répertoire non statique | Ancien modèle | Refusée |

## Garanties

> Le module VMM ne réalise plus d’appel à `kmalloc`, `kmalloc_aligned`, `kfree`, `malloc`, `calloc` ou `realloc`.

Le sous-système de tas reste initialisé par le noyau pour les composants qui l’emploient encore, mais le VMM ne lui délègue plus de gestion de tables ou de répertoires. Les chemins de création et de destruction se limitent donc à des capacités statiques ou à des pages PMM alignées.

## Validation

| Commande | Résultat |
|---|---:|
| `make -s test-kernel` | 38/38 suites réussies |
| `make -s test-all` | 479/479 tests réussis |
| `make -s kernel-only` | Réussi |
| Recherche d’allocations dans `kernel/mem/vmm.c` | Aucune occurrence |
| `git diff --check` | Réussi |

## Référence

[1] [Intel 64 and IA-32 Architectures Software Developer’s Manual — Paging](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
