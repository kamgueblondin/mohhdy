# Lot 14 Plan - Optimisation Performances & Robustesse Navigateur-OS / GGUF

Base: Lot 14 implementation branch

| Item | Scope | Status |
|------|-------|--------|
| 1 | Moteur Web, Performance & Inspection VFS (US-043 / US-045): Metriques de performance du navigateur (`web-perf`, `browser-telemetry perf`) et inspection VFS/DOM (`browser-dom-tree`, `browser-eval`) | done: `telemetry_sched_cycles`, `perf_min_us`, `perf_max_us`, `perf_avg_us`, `perf_last_us`, `render_ms` in `osui_runtime.c` & `osui_gui.c` |
| 2 | Evaluation de Latence & Benchmarks Inference GGUF (Garde 3): Cache KV multi-tours consolide avec rembobinage dynamique du prefixe commun (`gpt2_gguf_cache_match_prefix`, `gpt2_gguf_align_cache_prefix`) | done: prefix matching & rewind in `kernel/llm/gpt2_gguf_infer.c` + `make gguf-kvm-benchmark-check` |
| 3 | Documentation & Tests: Plan, ETAT_REEL, roadmap, todo, registre de commandes et tests unitaires/integration | done: `docs/lot14_plan.md`, `ETAT_REEL.md`, `mohhdy_execution_and_roadmap.md`, `todo.md`, `test_osui_runtime.c`, `test_osui_gui.c` |

## Summary of Lot 14 Implementations

- **US-043 / US-045**: Evolved browser performance telemetry (`web-perf`, `browser-telemetry perf`) with cycle latency calculation and scheduling tick overhead tracking (`telemetry_sched_cycles`, `perf_min_us`, `perf_max_us`, `perf_avg_us`, `perf_last_us`, `render_ms`).
- **US-045 / VFS-as-Web**: Extended VFS/DOM tree inspection in `/browser` and `/fs` for target node inspection, DOM hierarchy display (`browser-dom-tree`), and sandboxed expression evaluation (`browser-eval`).
- **Garde 3 / GGUF**: Consolidated multi-turn KV cache handling in `kernel/llm/gpt2_gguf_infer.c` (`gpt2_gguf_cache_match_prefix`, `gpt2_gguf_align_cache_prefix`) to rewind matching prefix KV entries on multi-turn prompt changes. Validated KVM latency benchmark protocol (`make gguf-kvm-benchmark-check`).
