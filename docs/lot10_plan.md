# Lot 10 plan (branch cursor/lot10-geant)

Base: main 600fcfa (merge of PR #158). Pull request #159.

| Item | Scope | Status |
|------|-------|--------|
| 1 | Web Runtime (US-039): CSP enforcement in web rendering pipeline | done: `webrt_csp_enforce` in `webrt.c` |
| 2 | PromptMessage CA (US-059/060): CA key rotation and revocation management | done: `pm-ca-rotate` in `promptmessage.c` / `shell.c` |
| 3 | HAL Platform (US-078/081): Dynamic battery discharge, sensor telemetry & power profile refresh | done: `plat_sensor_tick`, `plat_power_policy` in `platform.c` |
| 4 | Collab Economy (US-097): Dynamic points conversion and market fee computation | done: `collab_pay_convert` in `collab.c` |
| 5 | Production (US-120): Dynamic roadmap progress evaluation and FNV checksum | done: `prod_roadmap_eval` in `prod.c` |

## Summary of Lot 10 Implementations

- **US-039**: Full CSP directive enforcement in `userspace/webrt.c`. Script and style resource restrictions evaluated against document origin during page rendering.
- **US-059 / US-060**: CA Key Rotation and Revocation (`pm-ca-rotate`). Enables Schnorr CA key pair rotation with atomic key re-signing and revocation verification.
- **US-078 / US-081**: Dynamic sensor simulation and battery level drainage over tick cycles in `userspace/platform.c`. Auto-adjusts refresh rate (15Hz/30Hz/60Hz) according to active power policy profile.
- **US-097**: Points conversion engine (`collab-pay-convert`) calculating point-to-token rates, fee structures, and escrow time-lock validations.
- **US-120**: Dynamic roadmap parser (`prod-roadmap`) reading `/roadmap.txt` and computing per-phase completion metrics and FNV checksums.
