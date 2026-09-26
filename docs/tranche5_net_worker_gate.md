# Tranche 5 - net-driver worker gate (slice 1) and net IPC relay (slice 2)

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

Slice 1 text (superseded for 99-108 by slice 2 below): refused calls were
not forwarded to the worker. Since slice 2 the socket syscalls 99-108 are
relayed; LLM (91-98) and peer (128-130) calls are still refused with -59
and must not be assumed retried.

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
- Slice 1 had no IPC forwarding (see slice 2 below for the socket calls).
- The kernel still runs deferred DHCP lease maintenance at the top of
  `syscall_handler()` whatever the caller (kernel-internal, not gated).
- The shell `ai-*` commands are refused while `net-driver` is registered;
  it is not started at boot, so existing flows are unaffected.

## Slice 2 - net IPC relay for the socket syscalls

### What changed

While `net-driver` is registered, the socket syscalls 99-108
(`SYS_SOCKET_OPEN` .. `SYS_SOCKET_ACCEPT_ACK`: open, accept SYN-ACK, send,
feed, receive, close, listen, accept SYN, build SYN-ACK, accept ACK) issued
by any other task are no longer refused with -59. The kernel forwards them
to the worker:

1. The caller's syscall is intercepted before the gate
   (`syscall_net_relay()` in `kernel/syscall/syscall.c`). Arguments and
   input bytes (payload of `send`, segment of `feed`, views of the accept
   calls) are copied from the caller and validated like the direct path.
2. The kernel sends ONE IPC message to the worker mailbox:
   `sender_pid = 0` (only the kernel can send as 0), type
   `OS_IPC_NET_RELAY_REQUEST`, `request_id` = job id, data =
   `os_net_relay_request_t` (syscall number, 3 scalar args, up to
   `OS_NET_RELAY_MAX_IN` = 72 input bytes, output capacity). The request
   fits the existing 96-byte IPC payload; no new IPC channel.
3. The worker (`userspace/net_worker.c`) runs the same syscall itself (its
   PID passes the slice 1 gate: that is its privileged path), then answers
   with `SYS_NET_RELAY_REPLY` (137): job id, result, up to
   `OS_NET_RELAY_MAX_OUT` = 256 output bytes (segment built by `send` /
   `build SYN-ACK`, bytes read by `receive`). Only the live worker PID may
   reply (-59 for anyone else); a reply whose job id does not match is
   counted `stale` and dropped.
4. Meanwhile the caller re-enters its own `int 0x80` (rewind + yield, the
   same pattern as the ATA RPC gate) until the reply is stored, then the
   kernel copies the outputs into the caller's buffers in the caller's own
   address space and returns the worker's result.

One request is in flight at a time (single slot, `kernel/net_relay.c`,
pure logic, unit tested); other callers wait their turn.

Failure handling (never a replay):

- Worker killed or gone with a request in flight: the caller gets
  `OS_NET_RELAY_ABORTED` (-88) once (`[NET] relay aborted: net-driver
  lost`), counter `aborted`.
- No reply in time: `OS_NET_RELAY_TIMEOUT` (-87) (`[NET] relay timeout`),
  counter `timeouts`. The timeout needs BOTH 500 ticks (5 s at 100 Hz) AND
  3 caller turns while waiting, so a shell sitting in `SYS_GETS` (which
  starves every task) does not look like a dead worker.
- Caller killed while waiting: the slot is freed lazily by the next caller;
  a late reply is stale.
- Oversized input (> 72 bytes) returns `OS_SOCKET_BUFFER_SMALL` without
  forwarding; invalid user pointers return `OS_SOCKET_BAD_ARGUMENT`.
- A timed-out or aborted request may still sit in (or be executed by) the
  worker afterwards; its reply is dropped as stale. The caller must treat
  -87 / -88 as "outcome unknown".

Still refused with -59 when the worker is live (not relayed):

| Group | Syscalls |
|---|---|
| LLM network session | 91-98 |
| Guest-guest peer | 128-130 |

Counters: `SYS_NET_RELAY_STATUS` (138, public, read-only) returns
`forwarded`, `completed`, `aborted`, `timeouts`, `denied` (gated calls
refused with -59), `stale`, `pending`, `worker_pid`. Shell command
`net-relay-status`. The worker prints `net-driver relay op <n> rc <r>
reply <0|err> total <k>` per request.

Degraded mode (no `net-driver`) is unchanged: socket calls run locally and
the relay counters do not move.

### Proofs

Unit (`make test-kernel`, `tests/unit/kernel/test_net_relay.c`): relayed
subset (99-108 only, not LLM/peer/status/reply), request fits the IPC
payload, error codes distinct; single-slot round trip, one request at a
time, reply only from the worker with the right job id, double reply stale,
take by another pid refused; timeout (needs ticks and polls), abort, IPC
send failure (no forward counted), owner dropped then late reply stale;
the in-guest TCP loopback sequence on the socket registry.

QEMU (`make qemu-net-worker`, NE2000 ISA on QEMU user netdev, no disk, no
public host), about 190 s locally:

```text
netclaim local ok                                 degraded
netrelay tcp loopback ok ping pong mode local forwarded 0 completed 0
net-relay ok worker 0 fwd 0 ...                   no relay without worker
net-driver ready / net-driver gated syscalls ok   worker positive proof
netclaim worker-required enforced                 LLM + peer still -59
net-driver relay op 105 rc 0 ...                  netclaim listen relayed
netclaim socket relayed
netrelay tcp loopback ok ping pong mode relay forwarded 14 completed 14
net-driver relay op 99..108 rc >= 0 reply 0       worker ran each call
netrelay unsupported still worker-required        LLM + peer still -59
netrelay forged reply refused                     SYS_NET_RELAY_REPLY -59
net-relay ok worker <pid> fwd 16 done 16 aborted 0 timeouts 0 denied >= 4
task-suspend <worker>; netclaim -> [NET] relay timeout, rc -87
task-suspend <worker>; netclaim; kill <worker> -> [NET] relay aborted, rc -88
service-find: service indisponible; netclaim local ok; netrelay mode local
```

`netrelay` (`userspace/net_relay_client.c`) connects two sockets of the
kernel registry to each other inside the guest: active open, passive
listen, SYN / SYN-ACK / ACK, "ping" one way, "pong" the other way, close
both (14 calls). `make qemu-net-worker` now also runs in CI (job
integration-qemu-rest).

### Limits (read before claiming anything)

- The NE2000 driver, IRQ handler, frame rings and the TCP/TLS/socket
  registry all stay in Ring 0. The worker runs socket syscalls on behalf of
  others; it does not own the NIC. Not "driver out of the kernel", not
  "microkernel done".
- The socket syscalls 99-108 are a TCP segment codec over the Ring 0
  registry: `send` builds a segment into a user buffer and `feed` consumes
  one; they never put a frame on the NIC. So the relay proof is a TCP
  exchange between two registry sockets inside one guest, not TCP over the
  wire to the QEMU user-net host or to a second guest. Wire TCP for
  relayed callers would need a kernel "emit segment / poll segment" pair
  on NE2000 (ARP, IPv4 framing, RX demux) reserved to the worker: not
  done.
- One request in flight; one relayed call per cooperative turn of the
  caller (the shell hands out turns with `yield` in the QEMU proof).
- Inputs are capped at 72 bytes per call (IPC payload), outputs at 256.
