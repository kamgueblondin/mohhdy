# AOS-2171...2178 - ATA/FAT I/O behind verified rights

**Status: incremental (Tranche 4 suite).** After AOS-2163...2170, protected
mount I/O already goes through `vfsvirtual` under a temporary
right-source-prefix capability. AOS-2171 closes the mediator local
fallback when a trusted worker is alive. AOS-2172 closes the matching
kernel owner bypass for FAT16/FAT32 while `vfs-virtual` is published.
AOS-2173 closes the same owner bypass for initrd/overlay.
AOS-2174 closes owner bypass for `SOURCE_ALL` and any valid combination
(ATA-backed generic backend path) while `vfs-virtual` is published.
AOS-2175 adds the smallest worker-mediated ATA I/O slice: overlay
`read`/`stat` syscalls are exercisable only by the live `vfs-virtual`
PID (grants alone do not authorize the vfs owner locally).

> Drivers stay Ring 0. This does not claim "microkernel done" or US-001
> complete. It only places ATA/FAT I/O further behind rights already checked.

## Exit criteria

- With a live trusted worker, a failed storage delegation returns
  `OS_VFS_STATUS_INVALID` and logs `vfsserver storage rights local refused`
  instead of calling FAT/ATA/initrd/overlay backend syscalls as vfs owner.
- Without a live worker, the historical local fallback remains (degraded).
- Capabilities, no-replay, and public diagnostics without prefix stay.
- `make qemu-vfs-service` and `make qemu-ipc-foundation` stay green.

## Architecture

| Element | Role | Limit |
|---|---|---|
| Shell | Public VFS IPC only | No backend syscall |
| `vfsserver` | Policy + temporary grant to worker | No local ATA/FAT when worker live |
| `vfsvirtual` | Syscalls under grant | Only from PID `vfs` |
| Kernel | ATA PIO / FAT still Ring 0 | Owner bypass for any valid backend scope only if no `vfs-virtual` ; overlay read/stat only by worker PID when live (AOS-2175) |

## Proofs

```text
make -s -C userspace all
make -s test-all
make -s qemu-vfs-service
make -s qemu-ipc-foundation
```

QEMU already requires `vfsserver delegated storage ...` / `vfsvirtual storage ...`
on protected mounts (AOS-2163). Happy path must not emit
`vfsserver storage rights local refused`.


## AOS-2172 - kernel owner FAT bypass gated by storage worker

When `vfs-virtual` is registered, `service_registry_owner_bypasses_backend`
returns false for `FAT16` / `FAT32` sources. The vfs owner must use an explicit
right-source-prefix grant (the temporary grant already issued to the worker).
Without a live worker, the historical owner bypass remains (degraded mode).

Drivers stay Ring 0. This is not "microkernel done" and not full ATA driver
extraction.

Unit proof: `test_owner_fat_bypass_closes_when_storage_worker_live`.


## AOS-2173 - kernel owner initrd/overlay bypass gated by storage worker

Same gate as AOS-2172 for `INITRD` / `OVERLAY` sources. When `vfs-virtual` is
published, the vfs owner no longer bypasses backend rights for initrd or
overlay syscalls; an explicit right-source-prefix grant is required. Degraded
mode without the worker keeps the historical owner path unchanged.

Drivers stay Ring 0. No microkernel claim. ATA PIO remains in-kernel.

Unit proof: `test_owner_initrd_overlay_bypass_closes_when_storage_worker_live`.


## AOS-2174 - ATA-backed generic backend owner bypass gated by storage worker

Same gate as AOS-2172/2173 for `OS_SERVICE_BACKEND_SOURCE_ALL` and any valid
source combination (for example overlay|fat16). The generic backend read path
and combined ATA-backed scopes no longer bypass rights via vfs ownership when
`vfs-virtual` is published; an explicit grant covering the requested scope is
required. A `vfs` name handoff while the worker is live issues a SOURCE_ALL
grant to the new owner so ATA-backed generic I/O stays behind rights rather
than owner bypass. Degraded mode without the worker keeps the historical
owner path.

Drivers stay Ring 0. This is not full ATA driver extraction and not
"microkernel done".

Unit proof: `test_owner_ata_generic_bypass_closes_when_storage_worker_live`.


## AOS-2175 - worker-mediated ATA overlay read/stat slice

When `vfs-virtual` is published, `SYS_VFS_OVERLAY_READ` and
`SYS_VFS_OVERLAY_STAT` succeed only for the worker PID. An explicit
OVERLAY grant on the vfs owner is not enough to call those ATA-backed
paths locally; the temporary grant issued to the worker is the intended
exercise path. Without a live worker, degraded local exercise remains.

QEMU needles for the slice:

```text
vfsvirtual storage stat overlay/note.txt
vfsvirtual ata-backed stat overlay/note.txt
vfsvirtual storage read overlay/note.txt
vfsvirtual ata-backed read overlay/note.txt
```

Drivers stay Ring 0. This is not full ATA driver extraction and not
"microkernel done".

Unit proof: `test_ata_overlay_io_only_via_worker_when_live`.

## Limits

- No driver extraction from the kernel.
- No shared memory, no multi-request worker, no US-010 / US-016.
- Degraded mode without worker still uses local owner backend path.
- ATA PIO driver itself remains in Ring 0; owner bypass gates (including SOURCE_ALL) moved, driver not extracted.
- AOS-2175 mediates only overlay read/stat via the worker PID; mutate/list and historical SYS_READFILE stay for later slices.

## References

- [AOS-2163 storage-worker](aos2163_2170_vfs_storage_worker_separation.md)
- [PLAN_SUITE Tranche 4](PLAN_SUITE_IMPLEMENTATION.md)
- [ETAT_REEL](ETAT_REEL.md)
