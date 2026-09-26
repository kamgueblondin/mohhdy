# Tranche 4 - Ring 3 ATA PIO driver behind IPC (slices 1, 2 and 3)

Status: slices 1, 2 and 3 delivered. The storage driver is still NOT fully
out of Ring 0: the kernel keeps a complete PIO path, used at boot (overlay
load and FAT mounts, before any task exists) and as the fallback when the
driver is absent or dead. Do not read this as "microkernel done".

## Slice 3 summary (what moved in this slice)

- FAT16 (master) and FAT32 (slave) sector I/O goes through the Ring 3 driver
  whenever it is live. The FAT callbacks in `kernel/kernel.c`
  (`fat_disk_io`) call `syscall_ata_fat_io()` first; Ring 0 PIO is used only
  when that returns "not routed".
- Mechanism: synchronous sector RPC from inside the calling syscall. FAT
  code is synchronous and runs in the syscall of some user task T (the VFS
  worker, the shell...). The kernel submits one job of up to 8 sectors
  (`ata_job_io_submit`, new ops `OS_ATA_JOB_IO_READ/WRITE`, drive 0 or 1),
  saves T's kernel continuation (`boot/kctx.s`, setjmp/longjmp style on T's
  own per-task kernel stack), marks T `TASK_BLOCKED_KERNEL` and schedules the
  driver. The driver fetches the job first (it slots in between overlay
  chunks), runs the PIO at CPL 3 and completes it; `SYS_ATA_JOB_DONE` marks T
  ready and the scheduler resumes T inside its syscall, on its own kernel
  stack. Data moves through a kernel bounce buffer (copy in T's context
  before/after, copy into/out of the driver buffer in the driver's context),
  so no user page of T is touched from the driver's address space.
- Re-entrancy: FAT keeps static sector buffers. While T is blocked, the
  scheduler runs only the driver (then T), and any other task that enters a
  syscall is rewound over `int 0x80` and retried later. The FAT write path
  still takes `cli` around its read-modify-write; the RPC switches to the
  driver anyway (its own EFLAGS has IF=1), and this scheduling rule is what
  keeps other FAT callers out meanwhile.
- The -85 window is gone for FAT: FAT no longer calls kernel PIO while a
  driver is live, so the "kernel PIO refused while the driver holds the
  claim" case cannot hit FAT. Counter `fat_kernel_pio_live` (FAT sectors moved
  by Ring 0 PIO while a driver was live) is expected to stay 0 and is checked
  by the QEMU contract.
- Driver loss or stall: if the driver dies during an RPC (service purge on
  exit, kill or fault), T is resumed and the callback redoes the request
  through Ring 0 PIO (the claim of a dead driver is dropped). If the driver
  stops making progress (not READY, or no completion for 300 ticks = 3 s),
  the RPC is aborted (`fat_rpc_aborts`); if that driver still holds the claim
  the fallback PIO is refused (-85) and the FAT call fails instead of
  hanging.
- Boot spawn: when an IDE disk is present the kernel creates `atadriver` at
  boot, right after the shell (the shell keeps PID 1, the driver is PID 2),
  and puts it first in the round-robin order so it registers before the shell
  reaches its input loop. It has no user parent; only the root shell may stop
  it (`kill <pid>`, new narrow rule in `task_kill` for tasks flagged
  `boot_service`). Without an IDE disk no driver is spawned.
- Overlay durability with the boot driver: in slice 2 `write` returned once
  the flush was queued (asynchronous). With the driver now live by default
  that broke "write ok means on disk" (the reboot-persistence smoke caught
  it: the shell input loop can starve the driver). From a user syscall the
  caller now waits with the same RPC mechanism until the driver has written
  the whole snapshot (`OS_ATA_JOB_FLUSH_DONE` with no newer snapshot
  queued). If the driver dies meanwhile, the purge path persists through
  Ring 0; if it stalls (3 s without an accepted chunk), the caller returns
  and the flush stays queued.
- Boot-time snapshot ordering (decision): the overlay snapshot is still
  loaded by Ring 0 PIO during kernel init, because no task can run at that
  point, and the FAT volumes are mounted the same way. When the boot driver
  registers, its post-registration snapshot reload is skipped
  (`[ATA] boot driver registered; kernel boot load kept`): disk and RAM are
  identical at that point (every overlay write before registration was
  persisted synchronously by Ring 0 PIO). A driver registered later (respawn
  after a kill) still reloads the snapshot through its PIO, as in slice 2.
- Driver-side counters: the driver prints
  `atadriver fat io rd=<n> wr=<n> kfat=<n>` after each burst of FAT jobs
  (its own sector counts; `kfat` is the kernel `fat_kernel_pio_live`). The
  shell command `ata-status` prints the kernel view (`SYS_ATA_STATUS`,
  extended with `fat_driver_read_sectors`, `fat_driver_write_sectors`,
  `fat_kernel_pio_sectors`, `fat_kernel_pio_live`, `fat_rpc_aborts`,
  `boot_driver_pid`).
- GGUF: not changed specifically. GGUF weights are read through the same
  FAT16 volume callbacks, so a GGUF read issued from a user syscall while the
  driver is live also takes the driver path; the boot-time GGUF header probe
  uses Ring 0 PIO (before any task). Not QEMU-proven here: the GGUF targets
  need a local checkpoint that CI does not have.
- ipc-foundation contract: the `ipc recv from 1 type 0 data bonjour` match
  now uses the normalized log (timer `[SCHED]`/`TIMER_ALIVE` lines removed)
  and falls back to the tokens in order, like the #65 fix for the spawn line.

Still Ring 0 after slice 3: the boot-time overlay load and FAT mounts (no
task exists yet), the complete PIO path as fallback, and the job queue and
FAT/overlay logic themselves (only port I/O runs at CPL 3). No shared memory
page: data moves by kernel copy, 8 sectors per round trip, two context
switches per FAT job.

Known limits: one sector RPC at a time system-wide; while it runs every
other task is paused (fine for this single-CPU guest, not a scalable
design). A driver killed in the middle of a PIO command can leave the
controller busy until its own timeout; the Ring 0 fallback then waits on BSY
like any other access. The mid-RPC driver-crash path is implemented but only
the kill-between-requests fallback is QEMU-proven.

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

Durability note (slice 2; changed in slice 3, see above): with the driver
live, `write` returned once the overlay was updated in RAM and the flush was
queued. Slice 3 makes the calling syscall wait for the driver flush.

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

Slice 3 unit (`make test-kernel`): `test_ata_job.c` `test_fat_io_job`
(IO job slots between overlay chunks, stale overlay completion while the IO
job is handed out, read data copy, driver failure, cancel then stale late
completion, driver loss frees the slot, counters).

Slice 3 QEMU (`make qemu-ata-driver`, about 215 s locally, same fixture
disk, two boots):

1. Boot 1: `[ATA] boot atadriver spawned`, `[ATA] boot driver registered;
   kernel boot load kept`, `atadriver ring3 pio ready ... kpio=0`;
   `ata-status` shows `driver == boot > 0`; `service-find ata-driver` agrees.
2. `atarogue` (driver live): register, claim/job and sector IPC refused,
   raw port #GP kills only it.
3. A second `atadriver` prints `atadriver register failed` (service taken).
4. `ataclient` round trip, overlay and FAT16 fences (as slice 2).
5. `write t4s2 viadrv`: flush through the driver, kernel overlay PIO
   counter unchanged.
6. `vfs-write fat16/t4s3.txt viadrvfat` then `vfs-read`: the driver prints
   `atadriver fat io rd=65 wr=4 kfat=0`; `ata-status` before/after shows
   `fatrd`/`fatwr` growing, `fatkpio` unchanged (17 = boot mount sectors),
   `fatkpiolive 0`, `aborts 0`.
7. `kill <boot driver pid>` from the root shell: `service-find` reports it
   gone; `vfs-write fat16/t4fb.txt viakernel` and read back succeed with
   `fatkpio` growing (Ring 0 fallback); `write t4note ok4` persists via
   Ring 0. The host checks both FAT payloads in the FAT16 area and both
   overlay files in LBA 0-63.
8. Boot 2: kernel boot load, boot driver live again, `cat t4s2`/`t4note`,
   `fat16-cat t4s3.txt` and `t4fb.txt` read back through the driver.
9. Boot driver killed, `spawn atadriver`: `atadriver snapshot load ok ...
   loads=1` (reload through the driver), `cat t4s2`.

## Next steps

- Serve the boot-time overlay load and FAT mounts from the driver (needs the
  kernel to finish init in a task context, or a deferred mount).
- Replace the per-chunk kernel copy with a shared page, and allow more than
  one outstanding sector RPC.
- Move FAT/overlay logic itself out of Ring 0 (today only port I/O left).
