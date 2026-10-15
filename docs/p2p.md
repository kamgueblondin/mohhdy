# Phase 5 - P2P network (US-061 to US-075)

Status: delivered in the guest as C code, proven by unit tests and one
three-guest QEMU contract (`make qemu-p2p`). No public internet, no DHCP,
no host service: the QEMU hub only floods Ethernet frames between guests.

## Transport

* `SYS_PEER_DATA` gains two sub-operations, `OS_PEER_P2P_SEND` and
  `OS_PEER_P2P_RECV` (see `include/os_syscalls.h`). They carry UDP datagrams
  on port 7700 inside Ethernet broadcast frames (`kernel/net_p2p.c`). The
  node gives its own static IPv4 address; no lease is needed.
* On the default strict kernel the call is relayed to the Ring 3
  `networker`, which owns the NE2000 and runs the framing code. The strict
  kernel image does not link it (stub answers UNAVAILABLE).
* While a RECV polls the card, frames that are not P2P datagrams are
  dropped (counted). P2P and `web-serve` are therefore exclusive
  (`p2p-up` refuses while `web-serve` is active).

## Node protocol (`userspace/p2p.c`)

24-byte header: magic `MP2`, version, type, ttl, source id, destination id,
message id, via (next hop). Types:

* HELLO (clear, broadcast every 2 s): name, IP, X25519 public key and the
  list of directly reachable peers, authenticated by HMAC-SHA256 under the
  network key (PSK given to `p2p-up`). Wrong key: ignored (`bad_hello`).
* SEALED: AES-128-GCM under a per-pair key = SHA-256("mohhdy-p2p-v1" |
  network key | X25519 shared secret | ids). Nonce = sender id + counter;
  the header (minus ttl and via) is authenticated; replays are dropped
  (message id cache and strictly increasing counter).

Inner messages: ping/pong (RTT), text message, put (replication), sync
request/items (anti-entropy, both directions), get request/response
(remote fetch), propose/vote/commit (majority consensus).

These are the guest TLS primitives (X25519, AES-GCM, SHA-256 from the
in-tree TLS stack) used directly; it is not a TLS session and there are no
certificates: a node that knows the network key is a member.

## Shell commands

`p2p-up NAME IP [NETKEY]`, `p2p-down`, `p2p-peers`, `p2p-health`,
`p2p-stats`, `p2p-kv`, `p2p-send PEER TEXT`, `p2p-put K V`, `p2p-get K`,
`p2p-sync [PEER]`, `p2p-propose K V`, `p2p-block PEER`, `p2p-unblock PEER`,
`p2p-limit N`, `p2p-poll SECONDS`, `p2p-raft [elect]`, `p2p-raft-compact`,
`p2p-raft-put KEY VALUE`,
`p2p-grpc PEER SERVICE METHOD [PAYLOAD]`, `p2p-stun [PEER]`, `p2p-punch PEER`,
`p2p-cache`, `p2p-route`. While a node is up the console reads
keys without blocking and pumps the node between keys (same mechanism as
background `web-serve`); events print asynchronously (`p2p peer X up`,
`p2p msg from X: ...`, `p2p kv replicated ...`).

## User stories

| US | Status | What exists |
|----|--------|-------------|
| US-061 protocol | done | framing + header + types above; QEMU 3 guests |
| US-062 discovery | done | authenticated HELLO, peer table, up/down; QEMU |
| US-063 routing | done | multi-hop P2P routing with path costs (hop count, RTT, load weight) and route discovery (`p2p-route`) |
| US-064 consensus | partial | Raft log in the guest: pre-vote, membership quorum, append entries, commit index, compaction of the applied prefix, logical index past the physical window. Term and vote persisted in the p2pkv trailer. Not Paxos. No InstallSnapshot, no leader lease. Physical window 16. `p2p-raft`, `p2p-raft-compact`, `p2p-raft-put` |
| US-065 replication | done | put pushed to all peers, last writer wins (version, origin); QEMU |
| US-066 encryption | done | X25519 + AES-128-GCM per pair, PSK-authenticated, replay protection; QEMU checks no plaintext on the wire |
| US-067 gRPC | partial | typed RPC over sealed frames: request id, type, status, deadline. Not HTTP/2 gRPC. `p2p-grpc` |
| US-068 bandwidth | done | per-peer message budget per second (`p2p-limit`), dynamic congestion window scaling, throttle counters |
| US-069 fault tolerance | done | failure detection (10 s), Raft retry, relay rerouting, resync after partition; QEMU. The key/value store, node identity, and Raft term/vote persist across a reboot (`persist-*`) |
| US-070 distributed cache | done | remote get with miss reporting, local TTL cache with hits/misses and invalidation (`p2p-cache`) |
| US-071 synchronization | done | two-way digest anti-entropy (`p2p-sync`); QEMU after a partition |
| US-072 health monitor | done | per-peer RTT, last seen, sent/recv, auth failures, replays, throttled, down events; QEMU |
| US-073 adaptive QoS | done | priority queues for control/heartbeat vs data frames (`priority_high_tx`) and adaptive QoS counters |
| US-074 NAT traversal | simulated | virtual NAT / STUN binding (`p2p-stun`) and hole punching (`p2p-punch`). Not a real NAT |
| US-075 traffic analysis | done | per-type tx/rx counters, bytes, relayed, duplicates, foreign frames (`p2p-stats`); `p2p-analyze` applies automated rules and gives a verdict; QEMU |

## Limits (honest)

* Key seed in the guest is TSC + ticks mixed with SHA-256: weak entropy in
  emulation, fine for a lab, not a CSPRNG.
* At most 6 peers, 16 replicated items (key < 20 bytes, value < 48).
* Raft keeps a physical window of 16 entries. `p2p-raft-compact` drops the
  applied prefix and the next index can pass 16 (unit proof: index 17).
  There is no InstallSnapshot: a follower whose log is behind the snapshot
  is a gap. Append entries are retried by the leader. User datagrams are
  still not retransmitted; replication is repaired by `p2p-sync`, the
  one-round proposal by timeout.
* Typed RPC is not HTTP/2 gRPC. Deadlines are guest ticks. A repeated
  request id replays the last reply (`rpc_replays`).
* STUN and NAT punch stay virtual. There is no public internet and no
  real NAT on the QEMU hub.
* The membership secret is a shared key typed on the console; it is not
  stored by the guest. Raft term and vote are stored in the p2pkv blob
  trailer (`R` `F`, version 1). Old blobs without the trailer still load.

## Proofs

* `tests/unit/kernel/test_net_p2p.c`: framing round trip and rejections.
* `tests/unit/userspace/test_p2p.c`: 3-4 nodes on an in-process bus:
  discovery and pair keys, wrong network key, encryption/tamper/replay,
  replication + LWW + partition sync + remote get, consensus commit /
  reject / timeout, failure detection + relay + heal, rate limit + stats,
  bad inputs, Raft log replication and catch-up, typed RPC id and deadline.
* `tests/scripts/test_qemu_p2p.py` (`make qemu-p2p`, CI job "QEMU P2P three
  guests"): three guests with the Ring 3 networker on the strict kernel;
  discovery, encrypted message (hub asserts no plaintext), health, stats,
  replication, partition then sync, majority commit, Raft pre-vote,
  hub wire cut (scenario 2b, not `p2p-block`), compaction and commit of
  index 3, link failure detection and relayed delivery. Scenario 2 still
  uses `p2p-block`.
