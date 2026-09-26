# Tranche 4 - first real slice: Ring 3 ATA PIO driver behind IPC

Status: first slice delivered. The storage driver is NOT fully out of Ring 0.
Do not read this as "microkernel done".

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

## What did NOT move (kept as kernel fallback)

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

QEMU (`make qemu-ata-driver`, scratch 4 MiB IDE disk, about 80 s):

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

## Next slice

- Multi-sector transfer protocol or a bounded shared buffer page mapped into
  the driver, so a full sector (then the 64-sector snapshot) fits in a few
  round trips.
- Route overlay snapshot flush/load through the driver when it is live,
  with the kernel PIO path only as fallback when no driver is registered,
  plus an in-kernel arbitration flag so kernel PIO never interleaves with a
  live driver.
- Then FAT16/FAT32 sector I/O behind the same client, and finally removing
  the kernel PIO path from the default boot once a driver is spawned by init.
