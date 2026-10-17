# Lot 8 plan (branch cursor/lot8-geant)

Base: main 1de7236 (merge of PR #156).

| Item | Scope | Status |
|------|-------|--------|
| 1 | Root cause of the NE2000 TLS multipair flake (guest stalls after server_hello, ai-tls-poll rc -96), regression test | done: root cause in `rtc_read_utc` (CMOS update window), stable reader, `test_rtc`, `rtc-check` in qemu-smoke |
| 2 | US-001: UHCI USB tablet driver in Ring 3, bounded DMA buffer granted by the kernel | done: `bin/usbdriver`, kernel-checked TDs, `make qemu-usb-driver`; US-001 closed on the default image (decision in ETAT_REEL) |
| 3 | US-076 (ARM kernel), US-077 (mobile interface), US-083 (multi-device sync) | partial: ARMv7-A core (isolated CI job), mobile shell model, LWW device sync in one guest |
| 4 | Partials US-099, 103, 104, 112, 115, 120; US-067 TLS for gRPC if feasible | US-103 Schnorr NIZK (toy group), US-120 computed roadmap; US-099, 104, 112, 115 unchanged; US-067 TLS not done |

The table is updated as items land. Anything simulated is marked "simule".
