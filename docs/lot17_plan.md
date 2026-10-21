# Plan du Lot 17 - Mohhdy OS

## Objectif
Consolidation Web Runtime, Robustesse Inférence GGUF KVM & Synchronisation P2P/Production (US-040 / US-076 / US-121).

## Tableau des Tâches et Statuts

| Tâche | Description | Statut |
|---|---|---|
| 1. Moteur Web & Sandbox VFS Extended | Enrichissement de l'évaluation DOM/CSS (support `aspect-ratio` media query) et traçabilité sandbox VFS dans `osui-audit` (`vfs_read`, `vfs_scope_denied`, `vfs_traversal_denied`, `vfs_fs_save`, `vfs_fs_load`, `fetch_allowed`). | Terminé |
| 2. Réseau P2P & Résilience Consensus Mesh | Purge systématique des propositions obsolètes lors du basculement de leader sous partition réseau et réinitialisation propre du cache lors de `raft_install_snapshot` et `p2p_raft_compact`. | Terminé |
| 3. Métriques Production, Autoscale & Diagnostic | Prise en charge des scénarios de stress de charge dans `prod_autoscale_workload_eval` et intégration de l'état de santé global (`health_score`) dans `prod_support_bundle_text`. | Terminé |
| 4. Tests & Intégration | Ajout de cas de tests unitaires, régénération et vérification du registre de commandes (`extract_guest_commands.py --check`), `test-all`, benchmarks KVM et contrats QEMU (`test_qemu_osui_runtime.py`, `test_qemu_osui_gui.py`). | Terminé |
