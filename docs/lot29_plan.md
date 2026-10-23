# Plan du Lot 29 - Mohhdy OS

## Objectif
Extension et consolidation industrielle (~5000 lignes de code C Ring 3 / Freestanding) couvrant les sous-systemes majeurs d'infrastructure systeme, Web Runtime a composants Shadow DOM / Event Loop, Mesh P2P Zero-Trust & Securite / CI-CD autonome (US-002 / US-032 / US-043 / US-064 / US-075 / US-108 / US-118).

## Tableau des Taches et Statuts

| Tache | Description | Statut |
|---|---|---|
| 1. Documentation d'architecture Lot 29 | Creation de `docs/lot29_plan.md` (ASCII 7-bit) decrivant l'architecture, les specifications, le tableau des taches et les metriques attendues. | Termine |
| 2. Subsysteme VFS & Journaling Daemon (US-002 / US-034) | Developper dans `userspace/ramfs.h` et `userspace/ramfs.c` la table circulaire de journaux de mutations VFS (`vfs_mutation_log_t`) avec roll-forward automatique post-crash, l'isolation transactionnelle avec verrouillage hierarchique de repertoires (`vfs_dir_lock_tree`), et la verification d'integrite de table de noeuds VFS avec detection de cycles (`vfs_integrity_check`). | Termine |
| 3. Moteur Web Runtime Shadow DOM & Complete Event Loop (US-032 / US-043) | Developper dans `userspace/webrt.h` et `userspace/webrt.c` l'isolation Shadow Roots (`open`/`closed`) pour Custom Elements, la propagation d'evenements DOM (phases capture, target, bubbling & event path), et la gestion fine Microtasks (Promises) / Macrotasks (Timers/Events) dans `webrt_event_loop_tick`. | Termine |
| 4. Reseau P2P WAN Zero-Trust Mesh & Dynamic Re-keying (US-064 / US-075) | Developper dans `userspace/p2p.h` et `userspace/p2p.c` la rotation dynamique des clefs de session AES-128-GCM (Re-keying) sous controle de temps/volume, la verification Zero-Trust avec revocation instantanee sur rejeu/anomalie, et l'extension du journal Raft avec consensus 2PC (Two-Phase Commit). | Termine |
| 5. Moteur Pipeline CI/CD Autonome & Detection de Regression (US-108 / US-118) | Developper dans `userspace/prod.h` et `userspace/prod.c` le moteur de pipeline CI/CD en Ring 3 (validation ELF, verification checksums FNV-1a, matrice de test), le detecteur de regression de performance (`prod_perf_regression_eval`), et le systeme d'alerte adapte base sur la derivee seconde (acceleration CPU/memoire). | Termine |
| 6. Suites de Tests Unitaires Lot 29 | Ajouter les suites de tests unitaires `test_lot29_*` dans `tests/unit/userspace/test_ramfs.c`, `tests/unit/userspace/test_webrt.c`, `tests/unit/userspace/test_p2p.c` et `tests/unit/userspace/test_prod.c`. | Termine |
| 7. Mise a jour de la Documentation System & Roadmap | Mettre a jour en ASCII 7-bit `docs/ETAT_REEL.md`, `docs/mohhdy_execution_and_roadmap.md` et `docs/todo.md`. | Termine |
| 8. Verification et Pre-commit | Completer les etapes de verification pre-commit (make test-all, guest command registry, strict image check, secrets check, tree clean check, QEMU OSUI integration tests). | En cours |
| 9. Soumission du Lot 29 | Finaliser et soumettre les modifications. | A faire |

## Metriques Attendues
- Couverture fonctionnelle Ring 3 Freestanding C : 100% sans libc dynamique.
- Conformite ASCII 7-bit : 100% sur l'ensemble des fichiers source et documentation.
- Non-regression : 100% des tests unitaires et d'integration valides (`make test-all`).
