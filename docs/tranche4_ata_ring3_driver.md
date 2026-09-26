# Tranche 4 - Ring 3 ATA PIO driver behind IPC (slices 1 and 2)

Status: slices 1 and 2 delivered. The storage driver is NOT fully out of
Ring 0: FAT16/FAT32/GGUF sector I/O and the boot-time overlay load still use
the kernel PIO path. Do not read this as "microkernel done".

## Slice 2 summary (what moved in this slice)

- Overlay snapshot flush (LBA 0-63, 64 sectors) goes through the Ring 3
  driver whenever `ata-driver` is live. `overlay_save_disk()` then only queues
  a job; the driver pulls it in chunks of 8 sectors (`SYS_ATA_JOB_FETCH`, the
  kernel copies from its snapshot buffer into the driver buffer), writes them
  with its own CPL 3 PIO and reports each chunk (`SYS_ATA_JOB_DONE`). The
  kernel PIO overlay write runs only when no driver exists.
- Overlay snapshot load through the driver: when a driver registers, the
  kernel queues a read job; the driver reads LBA 0-63 and the kernel restores
  the overlay from those bytes, unless RAM changed in the meantime (then the
  load is dropped, RAM wins). The boot-time load still uses kernel PIO,
  because no driver exists yet at that point.
- Kernel-side exclusion: `SYS_ATA_CLAIM` / `SYS_ATA_RELEASE`. The IOPB opens
  the ATA ports only while the live driver holds the claim, and every kernel
  PIO call (`kernel/ata.c`) is refused with `OS_ATA_CONTROLLER_BUSY` (-85)
  during that window, so kernel PIO and driver never drive the controller
  concurrently. A claim left by a dead driver is dropped automatically.
- Driver loss: if the driver dies with a flush queued or in flight, the kernel
  immediately persists the snapshot through the Ring 0 PIO fallback.
- Client write fences: the kernel publishes (`SYS_ATA_STATUS`) the first
  master LBA past the mounted FAT16 volume and whether the slave holds FAT32;
  the driver refuses client writes to LBA 0-63, to the FAT16 volume and to a
  FAT32 slave.
- Counters (`SYS_ATA_STATUS`, public, read-only): driver flushes and loads,
  skipped loads, failed chunks, kernel PIO overlay writes, kernel PIO calls
  refused while claimed, fallback flushes, pending job, fences.

Durability note: with the driver live, `write` returns once the overlay is
updated in RAM and the flush is queued; the snapshot reaches the disk when
the driver runs (asynchronous). Without driver the kernel path is synchronous
as before.

Side fix: `syscall_user_range()` used to require the page after the last byte
to be mapped too, which rejected buffers ending in the top user stack page.
It now stops at the page holding the last byte.

New syscalls 131-135 (`MAX_SYSCALLS` 136): 131-134 are reserved to the live
`ata-driver` owner (`OS_ATA_DRIVER_REQUIRED` otherwise), 135 is public.
New codes: -85 `OS_ATA_CONTROLLER_BUSY`, -86 `OS_ATA_JOB_STALE`.

Still Ring 0 after slice 2: FAT16/FAT32 sector I/O, GGUF reads, the boot-time
overlay load, and the whole kernel PIO path as fallback. The driver is spawned
by hand (not by init). No shared memory page: data moves by kernel copy,
8 sectors per round trip.

The sections below describe slice 1 and stay valid unless noted.

## What actually left Ring 0

1. ATA port I/O capability. A Ring 3 task named `atadriver` that owns the
   service name `ata-driver` executes `in`/`out` on ports 0x1F0-0x1F7 and
   0x3F6 directly at CPL 3. This is enforced by the CPU, not by a syscall:
   the kernel TSS now carries a full 8 KiB I/O permission bitmap (IOPB) and
   the scheduler opens exactly those nine ports only while the live
   `ata-driver` owner is the running task. Every other task (and the driver
   after it exits or is killed) sees all 65536 ports denied.
2. Sector read/write for Ring 3 clients. `atadriver` serves bounded sector
   windows over the existing IPC mailbox: `OS_IPC_ATA_READ` /
   `OS_IPC_ATA_WRITE` carry LBA, drive, 16-byte-aligned offset and length
   (max `OS_ATA_IPC_WINDOW` = 64 bytes), and the reply `OS_IPC_ATA_REPLY`
   carries the status and data. A write is read-modify-write of one full
   sector followed by an ATA cache flush (0xE7), all done in Ring 3.

## What did NOT move in slice 1 (kept as kernel fallback)

Note: slice 2 moved the overlay flush and the post-boot load (see above).

- The overlay snapshot flush/load (LBA 0-63, 32 KiB) still uses the kernel
  Ring 0 PIO path (`kernel/ata.c`, `rep insw`/`rep outsw`). The IPC
  payload is 96 bytes and there is no shared memory, so routing a 32 KiB
  snapshot through 64-byte windows (512 round trips per snapshot) is not a
  realistic slice. The kernel path is therefore the degraded fallback, and
  today it is also the only path for the overlay and FAT volumes, driver
  live or not.
- FAT16/FAT32 sector I/O, the GGUF reader and all other in-kernel storage
  users are unchanged.
- There is no arbitration lock between the kernel PIO path and the Ring 3
  driver on the shared controller. In practice the kernel only touches the
  disk on explicit overlay/FAT operations, but concurrent use is not proven
  safe. To limit damage, the driver refuses writes to LBA 0-63 (the kernel
  overlay snapshot, `OS_ATA_KERNEL_RESERVED_LBAS`). The FAT region is not
  fenced by the driver.

## Security side effects (also real changes)

- Before this slice the TSS had `iomap_base = 0` inside a 104-byte limit, so
  the CPU interpreted TSS bytes as an I/O bitmap and some low ports were
  reachable from Ring 3. The new IOPB denies every port by default.
- A fault taken at CPL 3 (for example the #GP raised by a denied `in`) now
  kills only the faulting task (`[FAULT] user task killed: int=... err=...
  eip=...`, parent notified as killed). Before, any fault halted the kernel.
  Kernel-mode faults still halt as before.

## Capability rules (kernel, `kernel/service_registry.c`)

- `ata-driver` may only be registered by, or granted to, a task whose binary
  name is `atadriver`; otherwise `OS_ATA_DRIVER_REQUIRED` (-58).
- `ata-client` (the only IPC peer the driver serves) may only be held by the
  binary `ataclient`, same error code.
- `service_registry_ata_ports_granted(pid)` is true only for the live
  `ata-driver` owner; the scheduler calls `tss_set_ata_io()` on every switch,
  which toggles the nine bits only on transitions.
- The driver itself rejects any sector IPC whose sender is not the current
  `ata-client` owner, replying `OS_ATA_DRIVER_REQUIRED`.

Task names are binary basenames, not verified identities. This is a local
capability for the pedagogical guest, not an authenticated ACL.

## Proofs

Unit (`make test-kernel`):

- `tests/unit/kernel/test_io_bitmap.c`: deny-all default, ATA grant opens
  exactly 0x1F0-0x1F7 and 0x3F6 (0x1EF, 0x1F8, 0x3F5, 0x3F7, 0x60,
  0x20, 0x300, 0x3F8 stay denied), revoke restores deny-all, single-port
  set/clear.
- `tests/unit/kernel/test_service_registry.c`
  `test_ata_driver_name_and_port_grant`: name policy for both service names
  and port grant only for the live owner.

Slice 2 unit (`make test-kernel`): `tests/unit/kernel/test_ata_job.c`
(claim exclusion and stale claims, flush then load round trip, stale and
failed chunks, mutation during load, driver loss fallback, client fences).

Slice 2 QEMU (`make qemu-ata-driver`, about 110 s, FAT16 fixture disk
extended to 4 MiB, two QEMU boots on the same disk):

1. Boot 1, no driver: `atarogue` refused `ata-driver`, `ata-client`, the
   claim and the job queue (`atarogue claim and job refused`), then #GP.
2. `atadriver` starts (`kpio=0`); the load job on the empty disk is dropped.
3. `ataclient` round trip past the FAT16 fence, writes to LBA 0 and to the
   last FAT16 sector refused.
4. `write t4s2 viadrv`: the driver prints
   `atadriver snapshot flush ok gen=.. flushes=1 loads=0 kpio=0`, i.e. the
   snapshot went through the driver and the kernel PIO overlay counter did
   not move. The test also checks `viadrv` is in LBA 0-63 of the disk.
5. Rogue again with the driver live: refused everywhere, #GP.
6. Boot 2 (new QEMU process): `cat t4s2` prints `viadrv` (boot load by kernel
   PIO); a new `atadriver` prints `atadriver snapshot load ok ... loads=1`.
7. Driver killed: `write t4note ok4` persists via kernel PIO; the test checks
   `t4note` is in the on-disk snapshot.

Slice 1 QEMU (historical steps, still covered by the contract above):

1. No driver: `atarogue` is refused `ata-driver` and `ata-client`, then its
   raw `in 0x1F7` raises #GP and only that task is killed; the shell keeps
   answering.
2. Driver live: `atadriver ring3 pio ready`; `ataclient` writes and reads
   back `mohhdy-ring3-ata` at LBA 2000 through the driver
   (`ataclient ring3 sector roundtrip ok`) and a write to LBA 0 is refused
   (`ataclient overlay region refused`).
3. Driver live: `atarogue` is still refused at register, at the sector IPC
   (`atarogue sector ipc refused`) and at the port (#GP).
4. Driver and client killed: `service-find ata-driver` reports unavailable
   and the kernel Ring 0 overlay path still persists `write t4note ok4` /
   `cat t4note`.

## Next slice (slice 3)

- FAT16/FAT32 sector I/O through the driver job queue when it is live
  (today they are refused with -85 during a claim window and otherwise use
  kernel PIO), which requires making the FAT callers tolerate an async
  completion or blocking the caller until the job is done.
- Spawn `atadriver` from init so the post-boot state normally runs through
  the driver, then drop the boot-time kernel load once the driver can serve
  it early.
- Optional shared page to replace the per-chunk kernel copy.
