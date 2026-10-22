# Plan du Lot 23 - Mohhdy OS

## Objectif
Perfectionnement de la Securite Web Runtime, Resilience Multi-Noeuds P2P/Raft & Supervision Avancee de Production (US-039 / US-070 / US-110 / US-120).

## Tableau des Taches et Statuts

| Tache | Description | Statut |
|---|---|---|
| 1. Ecosysteme Web Runtime & Securite CSP | Consolider le filtrage dynamique CSP (webrt_csp_enforce, webrt_csp_validate_display_list_resource) et l'isolation des origines pour les ressources d'affichage, scripts, styles et medias. Renforcer la tracabilite des refus CSP, d'acces d'origines non autorisees et des autorisations MCP dans le journal d'audit de securite osui-audit (US-039 / US-041). | Termine |
| 2. Reseau P2P Mesh, Anti-Entropie & Consensus Raft | Affiner l'anti-entropie du journal Raft lors des reconnexions apres partitions reseau et la gestion des instantanes (p2p_raft_compact, raft_install_snapshot). Consolider le calcul du cout de lien EWMA (p2p_calc_link_cost), le controle de debit AIMD sous charge/perte de paquets (p2p_qos_note_loss) et l'invalidation automatique du cache P2P lors de deconnexions (p2p_cache_on_peer_down) (US-070 / US-076). | Termine |
| 3. Metriques de Production, Diagnostic & Auto-Scaling | Consolider l'autoscaling dynamique sous scenarios de charge extreme (prod_autoscale_workload_eval) et le moniteur de performance global (prod_perf_monitor_eval). Valider la verification d'integrite FNV-1a de la feuille de route (/roadmap.txt, prod_roadmap_eval) et l'inclusion du score de sante global dans les bundles de support diagnostic (prod-support-bundle, prod_support_bundle_text) (US-110 / US-120). | Termine |
| 4. Documentation & Tests de non-regression | Validation de la suite complete (make test-all, benchmarks GGUF KVM, extract_guest_commands.py --check, tests QEMU OS-UI runtime et GUI). | Termine |
