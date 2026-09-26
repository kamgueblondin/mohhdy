# Tranche 5 - net-driver worker gate on network syscalls

Status: guest QEMU prototype increment. The NE2000 driver, the TCP/socket
registry, the LLM network session and the peer (guest-guest) path all stay
in Ring 0. This slice only restricts **who may call** them.

## Before

PR #60 added `userspace/net_worker.c` (`networker`, registers the service
name `net-driver`) and `service_registry_net_io_via_worker()`, but no kernel
path called that predicate. Any Ring 3 task could drive the NIC and the
socket syscalls whether or not `net-driver` was registered. The built
`userspace/networker` binary was also committed to Git.

## Gate

When `net-driver` is registered, the syscall dispatcher refuses these
syscalls for every task except the `net-driver` PID, returning
`OS_NET_WORKER_REQUIRED` (-59):

| Group | Syscalls |
|---|---|
| LLM network session (DHCP, DNS, TCP, TLS, HTTP, SSE on NE2000) | 91-98 `SYS_LLM_ACQUIRE_START` .. `SYS_LLM_OPENAI_CREDENTIAL` |
| TCP socket registry | 99-108 `SYS_SOCKET_OPEN` .. `SYS_SOCKET_ACCEPT_ACK` |
| Guest-guest peer (NE2000 SYN-ACK, TLS server) | 128-130 `SYS_PEER_LISTEN` .. `SYS_PEER_TLS_POLL` |

Read-only diagnostics stay open to everyone: `SYS_NET_STATUS` (89) and
`SYS_LLM_SESSION_STATUS` (90).

Without a registered `net-driver` (degraded mode, the default boot and every
existing ne2k/tls/peer QEMU contract) nothing changes. When the worker is
killed its registration is purged and the degraded path reopens.

Refused calls are **not** forwarded to the worker: there is no network IPC
protocol yet, so a non-worker task gets -59 and must not assume a retry.

Code: `service_registry_net_syscall_gated()` /
`service_registry_net_syscall_allowed()` in `kernel/service_registry.c`,
checked at the top of `syscall_handler()` in `kernel/syscall/syscall.c`.

## Error code

-59 was free on main. -60 is reserved for `OS_VFS_BACKEND_WORKER_REQUIRED`
(AOS-2177, PR #62), -61 is `OS_VFS_BACKEND_DENIED`, -65 is
`OS_TASK_NOT_CHILD`. The unit test asserts -59 does not collide with them.

## Proofs

- Unit: `test_net_syscalls_only_via_net_driver_when_live`
  (`tests/unit/kernel/test_service_registry.c`).
- QEMU: `make qemu-net-worker` (`tests/integration/test_qemu_net_worker.py`,
  NE2000 ISA on QEMU user netdev, no public host):

```text
netclaim local ok                     (degraded, before networker)
net-driver ready
net-driver gated syscalls ok          (positive: worker PID reaches socket/peer)
service-find ok net-driver <pid>
netclaim worker-required enforced     (negative: socket, peer, LLM poll = -59)
Carte Ethernet : detectee             (net-status still open)
service-find: service indisponible    (after kill of networker)
netclaim local ok                     (degraded path reopened)
```

## Build hygiene

`userspace/networker` is no longer tracked (`git rm --cached`) and is listed
in `.gitignore` with the new `userspace/netclaim` proof binary. Both are
still built by `make userspace-all` and packed in the initrd.

## Limits

- NE2000 driver, IRQ3 handler, frame rings and TCP/TLS stacks remain Ring 0.
  This is not "driver out of the kernel" and not "microkernel done".
- No IPC forwarding: the worker is only an authorization anchor so far.
- The kernel still runs deferred DHCP lease maintenance at the top of
  `syscall_handler()` whatever the caller (kernel-internal, not gated).
- The shell `ai-*` commands are refused while `net-driver` is registered;
  it is not started at boot, so existing flows are unaffected.
