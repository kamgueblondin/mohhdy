# Lot 11 Plan - Extensions Moteur Web, P2P mesh avance & Production

Base: Lot 11 implementation branch

| Item | Scope | Status |
|------|-------|--------|
| 1 | Moteur Web & Securite (US-039 / US-031): Dynamic CSP origin validation for display list resources | done: `webrt_csp_validate_display_list_resource` and audit callback in `webrt.c` / `osui_runtime.c` |
| 2 | Reseau P2P & Resilience Mesh (US-063 / US-070): RTT EWMA sliding window & automatic cache invalidation on disconnect | done: `p->rtt_ewma_ms`, `link_cost`, and `p2p_cache_on_peer_down` in `p2p.c` / `p2p.h` |
| 3 | Telemetrie et Energie HAL (US-081): Power profile integration with OS-UI scheduling tick | done: `plat_power_schedule_tick` in `platform.c` and `osui_stage_tick` in `osui_runtime.c` |
| 4 | Documentation & Tests: Plan, ETAT_REEL, roadmap, todo, command registry and unit tests | done: `docs/lot11_plan.md`, `test_webrt.c`, `test_p2p.c`, `test_platform.c` |

## Summary of Lot 11 Implementations

- **US-039 / US-031**: Extended CSP verification with `webrt_csp_validate_display_list_resource` in `userspace/webrt.c` and `userspace/webrt.h`. Dynamically validates image, style, and script resource origins during display list construction, logging unauthorized load attempts to the audit log (`osui-audit`).
- **US-063 / US-070**: Reinforced degraded link detection in P2P routing table with smoothed EWMA RTT sliding window (`rtt_ewma_ms` in `link_cost`). Implemented automatic stale cache entry invalidation (`p2p_cache_on_peer_down`) triggered on peer disconnections.
- **US-081**: Integrated power profile manager (`plat_power_schedule_tick` in `userspace/platform.c`) into the OS-UI runtime scheduling tick (`osui_stage_tick` in `userspace/osui_runtime.c`) to dynamically adapt background activity levels (`background_ms`).
