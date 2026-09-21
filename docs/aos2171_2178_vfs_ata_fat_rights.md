# AOS-2171...2178 - ATA/FAT I/O behind verified rights

**Status: incremental (Tranche 4 suite).** After AOS-2163...2170, protected
mount I/O already goes through `vfsvirtual` under a temporary
right-source-prefix capability. This lot closes the remaining mediator
bypass: when a trusted worker is alive, `vfsserver` must not fall back to
local `*_mounted_backend` ATA/FAT/initrd/overlay syscalls.

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
| Kernel | ATA PIO / FAT still Ring 0 | Owner bypass remains for degraded path |

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

## Limits

- No driver extraction from the kernel.
- No shared memory, no multi-request worker, no US-010 / US-016.
- Degraded mode without worker still uses local owner backend path.

## References

- [AOS-2163 storage-worker](aos2163_2170_vfs_storage_worker_separation.md)
- [PLAN_SUITE Tranche 4](PLAN_SUITE_IMPLEMENTATION.md)
- [ETAT_REEL](ETAT_REEL.md)
