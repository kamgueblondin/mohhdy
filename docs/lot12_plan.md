# Lot 12 Plan - Moteur de session, conteneurisation Navigateur-OS & Latence GGUF KVM

Base: Lot 12 implementation branch

| Item | Scope | Status |
|------|-------|--------|
| 1 | Moteur de session & Conteneurisation Onglets (US-031 / US-038): Persistance de session isolee par site/origine & stockage local chiffre sur disque | done: `extract_origin_from_url`, `origin_ok`, `browser-fs-save` and `browser-fs-load` in `osui_runtime.c` |
| 2 | Vues d'inspection VFS & FS-as-Web (US-045): Inspection VFS et vue du systeme de fichiers sandboxe integree aux panneaux natifs `/browser` et `/fs` | done: `osui_gui_browser_inspect` and `osui_gui_vfs_inspect` in `osui_gui.c` / `osui_gui.h` |
| 3 | Evaluation Latence GGUF KVM (Garde 3): Validation du protocole de benchmark avec reutilisation du cache KV multi-tours | done: `gpt2_gguf_infer_preload_kv_cache` in `gpt2_gguf_infer.c` and `benchmark_qemu_gguf_kvm_latency.py` |
| 4 | Documentation & Tests: Plan, ETAT_REEL, roadmap, todo, registre de commandes et tests unitaires | done: `docs/lot12_plan.md`, `test_osui_runtime.c`, `test_osui_gui.c`, `test_gpt2_gguf_infer.c` |

## Summary of Lot 12 Implementations

- **US-031 / US-038**: Evolved Browser-OS tab container with site/origin isolated session persistence. Enhanced URL origin extraction in `extract_origin_from_url` and `origin_ok` (`userspace/osui_runtime.c`) to isolate sessions and tab storage per site origin (HTTP/HTTPS domains). Verified encrypted disk persistence (`browser-fs-save`, `browser-fs-load`, `ENC_V1:`) preserving tab origins and storage KV entries across save/load cycles.
- **US-045**: Integrated sandboxed VFS inspection and FS-as-Web views directly into native OS-UI panels (`/browser` and `/fs`) in `userspace/osui_gui.c` and `userspace/osui_gui.h` (`osui_gui_browser_inspect`, `osui_gui_vfs_inspect`).
- **Garde 3**: Validated KVM latency benchmark protocol with multi-turn KV cache reuse (`gpt2_gguf_infer_preload_kv_cache`, `gpt2_gguf_cache_matches`, `gpt2_gguf_infer_kv_cache_count` in `kernel/llm/gpt2_gguf_infer.c` and `tests/scripts/benchmark_qemu_gguf_kvm_latency.py`).
