# Plan du Lot 22 - Mohhdy OS

## Objectif
Consolidation de l'Ecosysteme OS-UI / PromptMessage, Resilience Economique Collaborative & Surveillance de Production Continue (US-041 / US-060 / US-082 / US-116).

## Tableau des Taches et Statuts

| Tache | Description | Statut |
|---|---|---|
| 1. Extension Ecosysteme OS-UI & Directives PromptMessage | Consolider l'execution sandboxee des composants OS-UI et PromptMessage (.pm / PMC1) dans `cmd_browser_pm_run` avec verification des certificats CA (/etc/pm_ca.pub), signatures Schnorr (csig_verify) et revocations CRL (pm_crl_has). Etendre la tracabilite d'audit de securite (osui-audit, audit_log_add) pour la rotation de cles CA (pm_ca_rotate_status) et la gestion des sessions utilisateur (cmd_session_new, cmd_session_use, cmd_session_end, cmd_session_ttl, cmd_session_restore, cmd_session_cleanup) (US-041 / US-060). | Termine |
| 2. Reseau P2P, Consensus & Economie Collaborative | Consolider la synchronisation d'etat (p2p_sync), le consensus Raft (p2p_raft_elect, raft_start_election, p2p_raft_append, raft_send_one, p2p_raft_compact) sous partitions reseau et charge de messages, la resolution LWW du registre collaboratif et le scellement AES-128-GCM des cles de signature (g_collab_locked, shell_persist_line) (US-075 / US-082). | Termine |
| 3. Metriques de Production, Alertes & Auto-Scaling | Affiner la prediction du risque de maintenance (prod_maint_model_risk, prod_predict), l'autoscaling de charge (prod_autoscale_workload_eval), la surveillance de performance (prod_perf_monitor_eval), l'evaluation dynamique de la feuille de route (prod_roadmap_eval, prod_roadmap_parse, checksum_fnv) et la generation des bundles de diagnostic (prod-support-bundle, prod_support_bundle_text) avec empreinte FNV-1a verifiee (US-112 / US-116). | Termine |
| 4. Documentation & Tests de non-regression | Validation de la suite complete (make test-all, benchmarks GGUF KVM, extract_guest_commands.py --check, tests QEMU OS-UI runtime et GUI). | Termine |
