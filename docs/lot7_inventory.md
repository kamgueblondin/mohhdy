# Lot 7 inventory: Phases 6, 7, 8 (US-076 to US-120)

Date: 17 October 2026. Base: `a44e95c` (merge of #155). Method: read the
code behind every command that `docs/platform.md`, `docs/collab.md` and
`docs/production.md` mark "done", not only the tables.

## Finding: several "done" rows are hard-coded output

Commit `1965281` (Phase 5-8 features) added functions whose result does not
depend on any input or state, while the doc tables were switched to "done".
They are listed here and handled in this lot (replaced by real code, or
renamed and marked `simule`, or removed from the claims).

| US | Function | What it really did |
|----|----------|--------------------|
| US-078 sensors | `plat_sensor_read` | constant values (accel 9.8, light 350, battery 95, charging) |
| US-081 energy | `plat_power_status_report` | battery taken from the constant sensor |
| US-086 legacy emulation | `plat_emul_run` | prints `status ok_simulated`, runs nothing |
| US-097 payment | `collab_point_convert` | fixed USD/EUR/TOKEN rates, no payment |
| US-099 smart contracts | `collab_contract_create/exec` | a task entry with text `contract:`, then accept/reject; no contract state |
| US-101 conflicts | `collab_dispute_*` | proposal, vote and answer entries; nothing decides or settles |
| US-094 marketplace | `collab_market_bid` | a booking entry named `bid`; no auction |
| US-103 privacy | `collab_privacy_proof_verify` | accepts any proof whose first byte is not 0 |
| US-104 compliance | `collab_compliance_check/report` | score always 100, always `verified` |
| US-108 CI/CD | `prod_pipeline_run` | prints 5/5 stages ok, runs nothing |
| US-112 security | `prod_sec_scan_run` | prints `cve_passed 120 vulns 0`, checks nothing |
| US-115 maintenance | `prod_maint_predict` | risk = 25 x firing alerts, MTBF a constant |
| US-119 business metrics | `prod_biz_metrics_get` | `active_users` constant 1, KPI an arbitrary formula |

## Real before this lot (kept)

- Phase 6: US-079 gestures (single and two-pointer classifiers), US-080
  layout computation, US-082 local notification queue with TTL, US-084
  continuity export/import of a session record, US-085 device registry and
  hotplug rules, US-087 ELF check, US-088 migration archive, US-089
  aggregated view, US-090 local deployment with rollback.
- Phase 7: US-091, 092, 093, 095, 096, 098, 100, 102, 105 (ledger replicated
  and signed, QEMU `make qemu-collab`).
- Phase 8: US-107, 109, 111, 113, 114 (thresholds, FOR, hysteresis,
  learned threshold), 116, 117, 118; US-110 real tasks scaled (`prod-scale-run`).

## Open, not feasible offline in this tree

- US-076 ARM kernel: no ARM toolchain, boot code or image in the tree.
- US-077 native mobile UI, US-078 real sensors: no mobile hardware.
- US-097 real decentralized payment: excluded (no money, no chain).

## Plan of this lot

See the PR description; each item gets a unit test and, when it runs in the
guest, a QEMU proof.
