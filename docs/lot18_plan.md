# Plan du Lot 18 - Mohhdy OS

## Objectif
Consolidation Web Runtime, Robustesse Consensus P2P & Métriques de Production / Sécurité (US-041 / US-075 / US-112).

## Tableau des Tâches et Statuts

| Tâche | Description | Statut |
|---|---|---|
| 1. Moteur Web, Styles CSS & Sandbox VFS | Extension de l'évaluation dynamique des variables CSS (`var()`), gestion étendue `WEBRT_VARS` à 32 slots et traçabilité sandbox VFS/stockage local dans `osui-audit`. | Terminé |
| 2. Réseau P2P & Consensus Mesh | Détection fine des anomalies de trafic, régulation AIMD adaptative sous forte latence et synchronisation anti-entropie Raft avec invalidation de cache. | Terminé |
| 3. Métriques Production, Scanner Sécurité & Roadmap | Extension de la base de signatures de sécurité (`prod-secdb`) avec `MSA-2026-0006`, contrôle FNV-1a de la feuille de route (`prod-roadmap`) et mise à jour de `prod-support-bundle`. | Terminé |
| 4. Tests & Intégration | Ajout de cas de tests unitaires, vérification du registre de commandes (`extract_guest_commands.py --check`), validation `make test-all`, benchmarks KVM et contrats QEMU (`test_qemu_osui_runtime.py`, `test_qemu_osui_gui.py`). | Terminé |
