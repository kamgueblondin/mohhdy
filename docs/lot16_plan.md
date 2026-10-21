# Plan du Lot 16 - Mohhdy OS

## Objectif
Extension Sécurité Audit (`osui-audit`), Consolidation QoS P2P Mesh & Robustesse Démarrage/VFS (US-045 / US-068 / US-081).

## Tableau des Tâches et Statuts

| Tâche | Description | Statut |
|---|---|---|
| 1. Sécurité Audit Web & MCP | Enrichissement de `osui-audit` pour violations CSP, refus d'origine (`origin_denied`), et accès MCP. Persistance overlay (`browser-fs-save`/`load`). | Terminé |
| 2. QoS P2P Mesh Adaptatif | Intégration des pénalités d'anomalie de trafic par pair dans `p2p_calc_link_cost` et réaction AIMD. Réallocation dynamique des routes. | Terminé |
| 3. Diagnostic & Métriques Prod | Inclusion de l'empreinte FNV-1a de la feuille de route (`roadmap fnv`) dans `prod_support_bundle_text` et consolidation de `prod_maint_model_risk`. | Terminé |
| 4. Tests & Intégration | Ajout de cas de tests unitaires, validation `extract_guest_commands.py`, `test-all`, benchmarks et contrats QEMU. | Terminé |
