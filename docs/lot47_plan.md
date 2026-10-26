# Lot 47 - one PR, status per item

Base: `origin/main` a8a2885 (merge of PR #203, Lot 46). Branch
`cursor/lot47`, one PR (#204), opened as a draft at the first push.

Remote branch `lot46-b-large-files` (5d609c6) was compared with main
before starting: its only change not on main is a v3 restore bound of 128
nodes. Main accepts up to 288 v3 records and bounds the restore by bytes
(the v3 area is 32 KiB), which is the limit that matters; this is a design
difference, not a missing fix. Nothing was ported, the branch was neither
merged nor rebased.

| Item | Status | Proof |
|---|---|---|
| 1. US-G04 interface configuration | done | `test_net_ifcfg` (5 tests), `make qemu-osui-web` step 5, `make qemu-gui-panels` (/net entries 6 and 7) |
| 2. US-G03 JPEG baseline, PNG interlaced and 16-bit | done | `test_imgdec` (14 tests, reference images), `make qemu-gui-parity` (img-info and viewer) |
| 3. Test-only initrd | done | `make strict-image-check` (production image checked free of the 28 test programs), every QEMU gate boots the test image |
| 4. Roadmap items | 2 done (below) | see below |

## 1. US-G04 - interface configuration (`net-if`)

- `net-if` shows mode, address, mask, gateway and lease;
  `net-if static IP MASK GW` sets a static address;
  `net-if dhcp` goes back to DHCP. One code path:
  `userspace/net_ifcfg.c`, used by the shell and by the /net panel
  (entry 6 shows, entry 7 changes, armed by a first Enter, run on the
  second).
- Path: `SYS_PEER_DATA` op `OS_PEER_IFCONFIG` (14), relayed by the kernel
  to the Ring 3 networker. The networker applies it to the single
  interface lease (`boot_llm_lease`). Static: `net_dhcp_static_apply`
  refuses a zero, loopback or multicast address, a non-contiguous mask,
  /0, /32, the network or broadcast address, and a gateway off the subnet
  or equal to the address. The change is checked on a copy first.
  DHCP: a DHCP-only exchange (`ne2k_dhcp_acquire`, 64 attempts at most);
  the static address is kept until the DHCP lease arrives. The DHCP
  renewal does not run while the static mode is on.
- Rights: reading is open; any change is allowed only to the root shell
  task (`task_root_shell_pid`). The kernel checks this before the relay
  (`net_ifconfig_allowed`, `kernel/net_relay.h`), so the networker never
  sees a refused request. GUI panels run in the root shell task, so they
  have the same rights as the terminal.
- QEMU (`make qemu-osui-web` step 5):
  - `net-if` shows dhcp 10.32.0.15.
  - The test-only program `ifcrogue`, spawned by the shell, is refused
    both changes (`denied (root shell only)`) and may read.
  - `static 10.32.0.77` is applied; `10.32.0.255` (broadcast) is refused.
  - `web-serve` on port 9138 answers HTTP 200 to the host peer on
    10.32.0.77.
  - `net-if dhcp` brings back 10.32.0.15, and port 9139 answers 200.
- Limit: a TCP connection already open across an address change is not
  tested; listeners opened after the change use the new address.

## 2. US-G03 - JPEG baseline, PNG every bit depth and Adam7

- Shared `userspace/imgdec.c` (+ `imgdec_jpeg.inc`), used by `img-info`
  and the viewer. Freestanding, no allocation, every loop bounded.
- PNG: bit depths 1, 2, 4, 8 and 16 for every color type the
  specification allows, and Adam7 interlace. Samples are normalized to
  8 bits (16-bit keeps the high byte; gray at 1/2/4 bits is scaled to
  0-255; palette indices are kept as indices). The caller scratch is
  `imgdec_scratch_need()` (inflated rows + 8-bit image); larger images
  are refused with "image trop grande".
- JPEG baseline:
  - Supported: SOF0/SOF1 8-bit Huffman, gray or YCbCr, sampling factors
    1..2 (4:4:4, 4:2:2, 4:2:0), restart markers (DRI/RSTn), 8 and 16-bit
    quantization tables.
  - Decoding: integer IDCT (Q12 table, coefficients clamped), nearest
    chroma upsampling.
  - Refused with "variante non supportee" (the probe still gives the
    size): progressive, lossless, arithmetic, 12-bit, CMYK, multi-scan.
- Reference images, made by `tests/scripts/gen_image_samples.py`:
  - PNG files are written by the script itself.
  - JPEG files were made once with Pillow and are committed, because
    libjpeg output can change between versions.
  - All of them are in `initrd_content/samples/`.
- Measured against the true pattern: 4:4:4 at most 4 levels off, 4:2:0
  and restart at most 8, gray at most 1. The unit tests allow 8 / 14 / 4.
- Robustness: `test_jpeg_refusals_and_bounds` (corrupted entropy data,
  truncated scan) and a local ASan/UBSan fuzz run of 3000 random
  corruptions per sample (not in CI).

## 3. Test-only initrd

- `make pack-initrd` builds two images:
  - `my_initrd.tar` (production: run, iso, run-gpt2);
  - `my_initrd_test.tar` (the production image plus the 28 programs in
    `TEST_ONLY_PROGRAMS`: rogue and claim probes, outflood, ifcrogue,
    netrelay, netwire, aiclient and others). These programs are copied
    to `build/initrd_test_root`, never to `initrd_content/bin`, and stale
    copies are removed from there.
- Every QEMU test script boots `my_initrd_test.tar`.
- `make strict-image-check` is unchanged for the kernel images. It now
  also fails if any test program is in the production initrd, if the
  test image lost one, or if the production image lost the shell.
  `check_no_secrets.py` scans both initrds, and
  `check_initrd_capacity.py` counts the test programs against the
  kernel file table.

## 4. Roadmap items taken (from `docs/ETAT_REEL.md` and the US master)

- US-G04 rights proof in QEMU (above, `ifcrogue`): the gap left by item 1
  (only a unit test covered the rights predicate) is closed.
- US-120: `/roadmap.txt` stays hand-kept, but `make roadmap-check` (CI,
  build job) fails when a phase 9 line differs from the US master table,
  or when the counts and percentages of the prod-roadmap QEMU test differ
  from the ones computed from the file. US-120 stays partial: phases 6-8
  are not cross-checked (the master has no status table for them).
- Not taken, and why:
  - US-076 (ARM port): drivers, FS and shell are not ported, more than
    one lot of work.
  - US-078 and US-097: simulated by nature (no sensor, no real payment).
  - US-080 and US-081 (screen relayout, battery): no hardware in QEMU to
    prove them.

Phase 9 of `prod-roadmap`: done 18, partial 0 (100%).
