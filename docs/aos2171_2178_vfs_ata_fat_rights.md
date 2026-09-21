# AOS-2171...2178 - ATA/FAT I/O behind verified rights

**Status: incremental (Tranche 4 suite).** After AOS-2163...2170, protected
mount I/O already goes through `vfsvirtual` under a temporary
right-source-prefix capability. AOS-2171 closes the mediator local
fallback when a trusted worker is alive. AOS-2172 closes the matching
kernel owner bypass for FAT16/FAT32 while `vfs-virtual` is published.

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
| Kernel | ATA PIO / FAT still Ring 0 | Owner FAT bypass only if no `vfs-virtual` |

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

Drivers stay Ring 0. Initrd/overlay owner bypass is unchanged in this step.
This is not "microkernel done" and not full ATA driver extraction.

Unit proof: `test_owner_fat_bypass_closes_when_storage_worker_live`.

## Limits

- No driver extraction from the kernel.
- No shared memory, no multi-request worker, no US-010 / US-016.
- Degraded mode without worker still uses local owner backend path.
- ATA PIO driver itself remains in Ring 0; only the FAT owner bypass gate moved.

## References

- [AOS-2163 storage-worker](aos2163_2170_vfs_storage_worker_separation.md)
- [PLAN_SUITE Tranche 4](PLAN_SUITE_IMPLEMENTATION.md)
- [ETAT_REEL](ETAT_REEL.md)
