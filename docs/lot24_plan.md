# Plan du Lot 24 - Mohhdy OS

## Objectif
Consolidation de la Sandbox Web Runtime, Resilience Reseau Mesh P2P/Raft & Supervision Haute Disponibilite de Production (US-039 / US-070 / US-076 / US-110 / US-120).

## Tableau des Taches et Statuts

| Tache | Description | Statut |
|---|---|---|
| 1. Ecosysteme Web Runtime & Securite CSP | Consolider l'evaluation dynamique CSP (webrt_csp_enforce, webrt_csp_validate_display_list_resource) et l'isolation des origines pour tous les types de ressources (scripts, styles, images, medias, connect/fetch). Renforcer la tracabilite des refus CSP, d'acces d'origines non autorisees et des autorisations MCP dans le journal d'audit de securite osui-audit (US-039 / US-041). | Termine |
| 2. Reseau P2P Mesh, Anti-Entropie & Consensus Raft | Affiner la synchronisation anti-entropie du journal Raft lors des reconnexions post-partition reseau et de la compaction des instantanes (p2p_raft_compact, raft_install_snapshot). Consolider le calcul du cout de lien EWMA (p2p_calc_link_cost), le controle de debit reactif AIMD en cas de perte/charge (p2p_qos_note_loss), et l'invalidation automatique du cache P2P lors de la deconnexion de pairs (p2p_cache_on_peer_down) (US-070 / US-076). | Termine |
| 3. Metriques de Production, Diagnostic & Auto-Scaling | Consolider l'autoscaling dynamique sous scenarios de charge extreme (prod_autoscale_workload_eval) en doublant la capacite ajustee sous condition de stress (queue_depth > 50 ou cpu_load_pct > 90%). Valider le moniteur de performance global (prod_perf_monitor_eval), le controle d'integrite FNV-1a de la feuille de route (/roadmap.txt, prod_roadmap_eval), et l'inclusion systematique du score de sante global dans les bundles de support diagnostic (prod-support-bundle, prod_support_bundle_text) (US-110 / US-120). | Termine |
| 4. Documentation, Tests Unitaires & Non-Regression | Validation complete via cas de tests unitaires dedies (test_lot24_*), verification de l'alignement du registre des commandes (extract_guest_commands.py --check), suite complete de tests (make test-all, benchmarks GGUF KVM, strict-image-check, secrets-check, tree-clean-check, scripts QEMU OS-UI runtime & GUI). | Termine |
