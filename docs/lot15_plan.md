# Lot 15 Plan - Consolidation Sécurité Web Runtime, Résilience P2P Mesh & Métriques de Production (US-039 / US-063 / US-120)

Base: Lot 15 implementation branch

| Item | Scope | Status |
|------|-------|--------|
| 1 | Sécurité Web Runtime & Inspection CSP (US-039 / US-031): Filtrage strict des directives CSP pour scripts, styles, images, médias et connecteurs réseau (`webrt_csp_enforce`, `webrt_csp_validate_display_list_resource`) avec traçabilité et journalisation d'audit (`osui-audit`, `g_webrt_audit_cb`) | done: `webrt.c`, `osui_runtime.c` |
| 2 | Maillage P2P & Résilience des Nœuds (US-063 / US-070): Routage dynamique réactualisé selon le RTT EWMA (`rtt_ewma_ms`), la charge et les pertes de paquets (`p2p_calc_link_cost`, `link_cost`), purge automatique du cache P2P distribué et de la file d'invalidation lors des déconnexions de pairs (`p2p_cache_on_peer_down`) | done: `p2p.c`, `p2p.h` |
| 3 | Métriques de Production & Évaluation Feuille de Route (US-120 / Garde 1): Évaluation dynamique de la feuille de route avec calcul et vérification d'intégrité FNV-1a (`prod_roadmap_parse`, `prod_roadmap_eval`), exposition FNV et métriques de santé via `prod-roadmap` et télémétrie | done: `prod.c`, `prod.h`, `shell_prod.c` |
| 4 | Documentation & Tests de non-régression: Plan Lot 15, mise à jour d'ETAT_REEL, roadmap, todo, alignement du registre des commandes, cas de tests unitaires et intégration QEMU | done: `docs/lot15_plan.md`, `ETAT_REEL.md`, `mohhdy_execution_and_roadmap.md`, `todo.md`, `test_osui_runtime.c`, `test_p2p.c` |

## Summary of Lot 15 Implementations

- **US-039 / US-031**: Evolved Web Runtime CSP enforcement (`webrt_csp_enforce`, `webrt_csp_validate_display_list_resource`) across scripts, styles, images, media streams, and fetch/connect directives, logging all CSP violations into the `osui-audit` security audit trail.
- **US-063 / US-070**: Consolidated P2P link cost calculations (`p2p_calc_link_cost`) using sliding window EWMA RTT (`rtt_ewma_ms`), packet loss percentage, and bucket load. Improved mesh network resilience with automatic cache invalidation and waiting queue cleanup on peer disconnection (`p2p_cache_on_peer_down`).
- **US-120 / Garde 1**: Updated roadmap evaluation (`prod_roadmap_eval`, `prod_roadmap_parse`) with FNV-1a checksum integrity verification (`checksum_fnv`), exposing phase progress and checksum telemetry in `prod-roadmap` and OS-UI telemetry outputs.
