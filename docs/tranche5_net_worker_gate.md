# Tranche 5 - net-driver worker gate (slice 1), net IPC relay (slice 2), wire TCP via the worker (slice 3)

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
  done in slice 2; slice 3 below adds it for an active open.
- One request in flight; one relayed call per cooperative turn of the
  caller (the shell hands out turns with `yield` in the QEMU proof).
- Inputs are capped at 72 bytes per call (IPC payload), outputs at 256.

## Slice 3 - wire TCP through the worker

### What changed

Relayed socket calls from a non-worker task now produce real Ethernet
frames on the NE2000, and only the worker can make that happen.

New syscalls (`include/os_syscalls.h`, `MAX_SYSCALLS` 139 -> 145):

| # | Name | Who | What |
|---|---|---|---|
| 139 | `SYS_NET_WIRE_CONNECT` | live `net-driver` PID only | open a registry socket, bind it to the wire (local/remote IP and port), ARP-resolve the on-link peer, emit SYN, consume SYN-ACK, emit ACK |
| 140 | `SYS_NET_WIRE_SEND` | worker only | build a data segment from the registry socket, wrap it in IPv4/Ethernet, emit it, poll until the peer ACKs it |
| 141 | `SYS_NET_WIRE_RECV` | worker only | poll frames, demux them to the bound socket, ACK received data, return the bytes |
| 142 | `SYS_NET_WIRE_CLOSE` | worker only | emit FIN, consume the peer FIN, ACK it, close and unbind |
| 143 | `SYS_NET_WIRE_STATUS` | public, read-only | counters (below) |
| 144 | `SYS_SOCKET_CONNECT` | public, gated and relayed | "connect" for applications: relayed to the worker, which runs `SYS_NET_WIRE_CONNECT` |

139-142 are refused with `OS_NET_WORKER_REQUIRED` (-59) for every other
task, **including in degraded mode** (no worker registered): nobody but the
worker drives raw frames from a syscall. `SYS_SOCKET_CONNECT` without a
worker is refused too (there is no local fallback for the wire path).
Each refusal is counted in `refused`.

Flow for a plain task (the `netwire` proof client):

1. `SYS_SOCKET_CONNECT {10.32.0.15:40007 -> 10.32.0.2:7}` is intercepted
   by the slice 2 relay (144 was added to the relayed set, 16-byte input)
   and forwarded to the worker over IPC.
2. The worker (`userspace/net_worker.c`) calls `SYS_NET_WIRE_CONNECT`.
   The kernel (`kernel/net_wire.c`, called from `kernel/kernel.c` with the
   boot NE2000 and dedicated frame buffers) opens a registry socket, binds
   it, sends an ARP request, feeds the ARP reply to the cache, sends the
   SYN through `ne2k_tcp_segment()` (IPv4 + Ethernet framing), accepts the
   SYN-ACK and sends the ACK. The socket id goes back to the caller.
3. The caller's `SYS_SOCKET_SEND`, `SYS_SOCKET_RECEIVE`, `SYS_SOCKET_CLOSE`
   are relayed as before. For a wire-bound socket the worker uses
   `SYS_NET_WIRE_SEND` / `RECV` / `CLOSE`; for any other socket the kernel
   answers `OS_NET_WIRE_NOT_BOUND` (-119) and the worker keeps the slice 2
   segment-codec path (so `netrelay` is unchanged). `SYS_SOCKET_SEND` still
   returns the emitted TCP segment to the caller (same contract).
4. Receive demux: every polled frame is classified by
   `net_wire_demux()`. A TCP frame is fed to a socket only if its IPv4
   destination/source and TCP destination/source ports match that
   socket's binding (a 4-tuple can be bound once). ARP requests/replies
   for a bound local IP are answered/learned. Everything else (other
   hosts, other ports, UDP, IPv4 fragments) is counted `dropped` and never
   reaches a socket.

Counters (`SYS_NET_WIRE_STATUS`, shell `net-wire-status`): `connects`,
`frames_tx`, `frames_rx` (frames emitted/consumed on the worker path),
`arp_tx`, `sends`, `recvs`, `closes`, `demuxed`, `dropped`,
`arp_replies`, `peer_fins`, `refused`, `bound`, `worker_pid`. The worker
log line gains `wire <n>` (wire calls it made).

New error codes: `OS_NET_WIRE_TIMEOUT` (-126), `OS_NET_WIRE_UNAVAILABLE`
(-127, no NE2000), `OS_NET_WIRE_NOT_BOUND` (-119), taken from gaps no other
`OS_*` code uses (the unit test asserts they differ from their neighbours).

### Proofs

Unit (`make test-kernel`, `tests/unit/kernel/test_net_wire.c`): bind rules
(one owner per 4-tuple, bad ids/zero IP/zero port refused), demux (match,
Ethernet padding ignored, wrong port/peer/destination, truncated, fragment,
UDP, unbound -> drop), ARP demux only for a bound local IP, status and
`refused` counters, unbound ops answer NOT_BOUND, connect without a device
answers UNAVAILABLE, relay ABI (connect request fits the 72-byte relay
input, 144 relayed, 139-143 never relayed).

QEMU (`make qemu-net-wire`, `tests/integration/test_qemu_net_wire.py`).
The NE2000 is connected with `-netdev socket,connect=127.0.0.1:<port>` to
`tests/scripts/qemu_wire_echo_peer.py`, a local process that owns
10.32.0.2, answers ARP and runs a TCP echo on port 7. No user-net, no
public internet. The peer verifies the IPv4 and TCP checksums of every
guest frame. About 60 s locally:

```text
net-wire ok worker 0 connects 0 tx 0 rx 0 ... refused 0 bound 0   boot
netwire raw wire refused 4 of 4                  degraded, direct 139-142
netwire connect worker-required                  degraded, relayed connect
net-wire ok worker 0 ... tx 0 ... refused 5      no frame without worker
net-driver relay op 144 rc 0 reply 0 total 1 wire 1
netwire connect ok socket 0
net-driver relay op 101 rc 0 reply 0 total 2 wire 2
netwire send ok segment 36
net-driver relay op 103 rc 0 reply 0 total 3 wire 3
net-driver relay op 104 rc 0 reply 0 total 4 wire 4
netwire echo ok bytes 16 relayed 4 frames tx 7 rx 4 demuxed 3 refused 4 bound 0
net-wire ok worker 3 connects 1 tx 7 rx 4 arp 1 sends 1 recvs 1 closes 1
  demuxed 3 dropped 0 fins 1 refused 9 bound 0
host: wire tcp: guest->peer frames 7, peer->guest frames 4 (kernel rx 4),
  arp 1, relayed calls 4, echo 16 bytes, raw wire refused 9,
  peer checksum errors 0
```

The 7 guest frames are ARP request, SYN, ACK, data, ACK of the echo, FIN,
ACK of the peer FIN; the 4 peer frames are ARP reply, SYN-ACK, echo
(PSH+ACK), FIN+ACK. The test requires the kernel `tx` counter to equal the
frames the peer received, the peer to have seen exactly one SYN, one data
segment carrying `mohhdy-wire-echo`, one FIN and zero checksum errors, and
no frame at all before the worker exists. It runs in CI in the `OS-UI
guest C` job (the shortest job) so the wall time does not grow.

### What runs where (read before claiming anything)

| Piece | Ring |
|---|---|
| `netwire` application, socket calls | Ring 3 |
| `networker` (`net-driver`): decides and issues every wire operation, relays results | Ring 3 |
| Relay (IPC request/reply, user copies) | Ring 0 kernel code, one request at a time |
| `net_wire.c`: bindings, demux, ARP, poll loops | Ring 0, entered only from the worker's syscalls |
| NE2000 driver (`ne2k.c`: port I/O, TX/RX ring DMA), IRQ handler | Ring 0 |
| TCP state machine and socket registry (`net_tcp.c`, `net_socket.c`), TLS, LLM session, peer path | Ring 0 |

So the worker owns the **right** to put frames on the wire, not the NIC.
This is not "driver out of the kernel" and not "microkernel done".

### Limits

- Active open only, to an on-link peer given by IP (no DHCP/DNS/gateway
  on this path; the proof uses static 10.32.0.15 -> 10.32.0.2). No listen
  or accept on the wire path (the peer/TLS server path stays separate and
  still -59 for non-workers).
- Polling, bounded per call (default 200 rounds, max 2000, one timer tick
  per idle round); SYN is re-sent every 100 idle rounds; data segments
  are not retransmitted (no RTO on this path yet): a lost data segment
  gives `OS_NET_WIRE_TIMEOUT`. Payload per call <= 72 bytes in (relay
  input) and 256 out.
- The wire path and the LLM/peer paths share the NIC and the ARP cache;
  they are not meant to run at the same time (a frame for the other path
  polled by one of them is dropped).
- The relay timeout (-87) still applies: a wire call must finish within
  5 s / 3 caller turns.

### Next step: NE2000 port I/O in Ring 3 (steps 1 and 2 done in the suite below)

The ATA driver already runs its port I/O from Ring 3 through the TSS I/O
permission bitmap (`kernel/io_bitmap.c`, granted to `atadriver` on task
switch). The equivalent for the NIC is not a small change, which is why it
is not in this PR:

1. Grant ports 0x300-0x31F in the IOPB to the `net-driver` PID only, and
   stop every kernel path (LLM, peer, wire, the IRQ handler that acks the
   ISR) from touching them while the worker holds the grant.
2. Build `ne2k.c` (probe, rings, remote DMA TX/RX) into the worker, with
   the kernel forwarding the NE2000 IRQ as an IPC notification.
3. Move ARP/IPv4 framing and the demux of `net_wire.c` into the worker, so
   the kernel keeps only the relay and the socket registry, then decide
   whether the TCP state machine follows.

Step 1 alone (grant without moving the driver) would only add a second
writer to the same ports, so it is deliberately not done as a "slice".



## Tranche 5 suite - the NE2000 driven from Ring 3 through the IOPB

Steps 1 and 2 of the plan above. Step 3 (framing out of Ring 0) is not done.

### What changed

- **Ownership (`kernel/net_nic_owner.c`, pure logic, unit tested).** The NIC
  owner is `0` (kernel) or the PID of the live `net-driver`. Only that PID
  can claim, only if an NE2000 was probed at boot (`OS_NET_NIC_ABSENT` -143
  otherwise); the claim is idempotent and a stale owner must be dropped
  before another worker can claim.
- **`SYS_NET_NIC` (146).** `CLAIM`, `PUMP`, `IRQ` are reserved to the live
  owner (`OS_NET_WORKER_REQUIRED` otherwise); `STATUS` is public
  (`os_net_nic_status_t`: owner, claims, reclaims, IRQs forwarded, kernel
  accesses refused, kernel syscalls gated, pumps, frames out/in, worker
  TX ok/failed).
- **IOPB.** `io_bitmap_apply_ne2k()` opens exactly 0x300-0x31F.
  `schedule()` calls `tss_set_nic_io()` on every switch: the window is open
  only while the owner runs and is still the live `net-driver`. Any other
  task doing `in`/`out` there takes #GP (only that task dies), as for ATA.
  The NE2000 grant is independent of the ATA grant (unit tested).
- **Kernel hands off.** Every kernel NE2000 access goes through a gated
  `ne2k_io_t` (`kernel_nic_inb/outb`): while a worker owns the card the
  access is dropped and counted (`kernel_refused`, expected and measured 0).
  Before that, the entry points refuse: LLM 91-98 and peer 128-130 answer
  `OS_NET_NIC_WORKER_OWNED` (-141) for everybody, the worker included
  (counted in `kernel_gated`), and the DHCP maintenance tick is skipped.
- **IRQ3.** `ne2k_irq_handler()` only counts the event for the owner
  (`nic_owner_irq()`); the PIC EOI stays in the stub. The worker enables
  PRX|PTX in the card's IMR, reads and acks the ISR with its own PIO and
  collects the count with `OS_NET_NIC_IRQ`.
- **Driver in the worker.** The NE2000 hardware core was split out of
  `ne2k.c` into `kernel/ne2k_hw.c` (probe, PROM/MAC, rings, remote-DMA TX,
  RX ring poll), compiled twice: into the kernel and, with
  `-DMOHHDY_RING3`, into `networker`. After `CLAIM` the worker re-probes the
  card at CPL 3, re-reads the station PROM (compared with the MAC the
  kernel probed: `prom-match 1`) and reconfigures the rings.
- **Resumable wire engine (`net_wire.c`).** The slice 3 blocking loops
  became a one-op-at-a-time state machine (`net_wire_op_connect/send/recv/
  close`, `net_wire_op_step`). Frames leave through a sink (`ctx->emit`)
  instead of `ne2k_tx_submit`. The Ring 0 blocking entry points
  (`net_wire_connect`...) still drive it on `ctx->io` when no worker owns
  the card (same rounds, same counters, same SYN cadence).
- **Pump protocol.** With a worker owner, `SYS_NET_WIRE_*` and the relayed
  `SYS_SOCKET_CONNECT` return `OS_NET_WIRE_PENDING` (-142). The worker then
  loops on `OS_NET_NIC_PUMP`: the kernel copies out up to 4 frames the
  engine queued, the worker transmits them with its own remote DMA, polls
  its RX ring and feeds the next frame (or an idle round after one timer
  tick, spinning without yielding like the Ring 0 `wire_pause()` so the
  op still completes inside the 5 s relay deadline). `done`/`result` end
  the op; a RECV writes its length directly into the worker's variable.
- **Worker loss.** When the `net-driver` PID is purged, the kernel closes
  the window, cancels the pending op (no replay), re-probes and
  re-initialises the card from Ring 0 with IMR off ("NE2000 back in Ring 0
  after worker loss"); a new worker can claim again.

### What runs where now

| Piece | Ring |
|---|---|
| NE2000 port I/O: probe, PROM, rings, remote-DMA TX, RX ring poll, ISR ack | **Ring 3** (`networker`, IOPB 0x300-0x31F) |
| IRQ3 | Ring 0 stub + EOI, **counted** for the worker; serviced by the worker |
| ARP/IPv4/TCP framing, 4-tuple demux, TCP state machine, socket registry | Ring 0 (`net_wire.c`, `net_tcp.c`, `net_socket.c`), driven by the worker's pumps |
| TLS, LLM session, peer path | Ring 0, refused (-141) while the worker owns the card |
| Boot probe, degraded path without worker, reclaim after worker loss | Ring 0 |

### Proofs

- Unity: `test_net_nic_owner` (6 tests: claim rules, ports follow the owner,
  drop/reclaim, IRQ forwarding, counters, ABI), `test_io_bitmap` (+2:
  NE2000 window only, independent from ATA), `test_net_wire` (+2: the
  engine through an emit sink: ARP request, one op at a time, ARP reply
  learned then SYN framed to the learned MAC, cancel frees the binding,
  timeout after the requested rounds, refusing sink counts nothing, Ring 0
  blocking entry points refuse a sink context).
- `make qemu-net-wire` (extended; the NIC is now declared `irq=3`, QEMU's
  `ne2k_isa` defaults to IRQ 9): the ports are handed to the worker, the
  worker's own PROM read matches (`prom-match 1`), the same 16-byte echo
  passes, and the worker's last report must show owner = worker PID,
  worker TX = frames handed out = frames the peer received (7), worker RX =
  frames fed in (4), 0 TX failure, IRQ3 taken by the worker (9, 10 counted
  by the kernel), 0 kernel port access, 0 gated call. Then the worker is
  killed: the kernel logs the reclaim, and a respawned worker (new PID)
  claims the card again with `prom-match 1`.
- `make qemu-net-worker` unchanged and passing (the worker claims the
  user-net NIC; the stalled/killed worker paths still time out / abort and
  the kernel reclaims the card).

### Limits (read before claiming anything)

- This is "NIC port I/O and IRQ servicing out of Ring 0", not "network stack
  out of Ring 0": framing, demux, TCP and TLS stay in the kernel (step 3).
- While a worker owns the card the kernel LLM/TLS/peer paths are refused;
  they are not yet re-routed through the worker. Without a worker nothing
  changes (`networker` is not started at boot).
- Polling is still the pacing mechanism; IRQ3 is counted and acked, the
  worker does not block on it yet.
- One wire op at a time, at most 4 frames per pump, 1536 bytes per frame.
