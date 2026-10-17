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
`p2p-cache [inv KEY [VER] | putv KEY VAL VER]`, `p2p-qos`, `p2p-route`. While a node is up the console reads
keys without blocking and pumps the node between keys (same mechanism as
background `web-serve`); events print asynchronously (`p2p peer X up`,
`p2p msg from X: ...`, `p2p kv replicated ...`).

## User stories

| US | Status | What exists |
|----|--------|-------------|
| US-061 protocol | done | framing + header + types above; QEMU 3 guests |
| US-062 discovery | done | authenticated HELLO, peer table, up/down; QEMU |
| US-063 routing | partial | distance vector in HELLO (peer id + hop count, at most 3 hops, also the frame TTL); frames relayed hop by hop (`relayed_multi`). Unit proof on a 4-node line (`test_multi_relay_chain`) and QEMU proof with two relays (`make qemu-p2p-mesh`, 4 guests, line alpha-beta-gamma-delta by hub wire cuts). A destination must have been discovered and keyed once |
| US-064 consensus | done | Raft log in the guest: pre-vote, membership quorum, append entries, commit index, compaction, InstallSnapshot for a follower behind the snapshot, leader lease of 10 s renewed when a heartbeat reaches a peer. Term and vote persisted in the p2pkv trailer. Not Paxos. Physical window 16. `p2p-raft`, `p2p-raft-compact`, `p2p-raft-put` |
| US-065 replication | done | put pushed to all peers, last writer wins (version, origin); QEMU |
| US-066 encryption | done | X25519 + AES-128-GCM per pair, PSK-authenticated, replay protection; QEMU checks no plaintext on the wire |
| US-067 gRPC | partial | typed RPC over sealed frames: request id, type, status, deadline. Not HTTP/2 gRPC. `p2p-grpc` |
| US-068 bandwidth | partial | per-peer message budget (`p2p-limit`) and the adaptive data window of US-073. Control frames are not capped. Not a bandwidth estimator |
| US-069 fault tolerance | partial | failure detection (10 s), relay, resync. QEMU scenario 2 is a hub wire cut (`cut_pair`), scenario 3 is a blocked link. The key/value store, node identity, and Raft term/vote persist across a reboot (`persist-*`). `make qemu-p2p-mesh`: a relay's QEMU process is killed, traffic reroutes, the node reboots under a new identity with the same name, the peers reuse its slot (`rejoins`) and push the store back. Not a complete fault-tolerance system |
| US-070 distributed cache | partial | remote get with miss reporting, local TTL cache, versioned entries with fences (`p2p-cache putv KEY VAL VER`, a stale version is refused), invalidation `p2p-cache inv KEY [VER]` acked per peer (`P2P_I_CACHE_INV_ACK`), re-sent every second up to 5 times, `incomplete` when acks never come. QEMU scenario 4 (relayed through beta). Not a consensus-backed cache |
| US-071 synchronization | done | two-way digest anti-entropy (`p2p-sync`); QEMU after a partition |
| US-072 health monitor | done | per-peer RTT, last seen, sent/recv, auth failures, replays, throttled, down events; QEMU |
| US-073 adaptive QoS | partial | EWMA smoothed RTT and minimum RTT; window cut by a quarter when RTT is above twice the minimum, halved on loss (one loss per outage), plus one per success; control frames bypass `admit()`. `p2p-qos`. Unit proof and QEMU scenario 3 (window below 16 after the gamma cut). Not a full congestion controller |
| US-074 NAT traversal | partial | real RFC 5389 STUN Binding codec (`userspace/stun.c`, XOR-MAPPED-ADDRESS, transaction id per server) inside the sealed link; the server answers with the frame's IPv4 source; two servers classify `direct` / `endpoint_independent` / `endpoint_dependent` (`p2p-stun PEER`). QEMU proves `direct` between guests. The NAT is simule (address rewrite on the unit bus); hole punching (`p2p-punch`) is still simulated. Not a real NAT traversal |
| US-075 traffic analysis | partial | per-type counters (`p2p-stats`) and `p2p-analyze`: fixed rules (`congestion`, `queue-delay`, dead link, relay) plus a learned per-type receive baseline (EWMA x16, 5 windows of warm-up, anomaly above 4x baseline + 8 frames/s, finding `rate-anomaly`). This is an EWMA baseline, not machine learning. A rate anomaly ranks as `degraded`; only crypto failures make the verdict `suspicious` |

## Limits (honest)

* Key seed in the guest is TSC + ticks mixed with SHA-256: weak entropy in
  emulation, fine for a lab, not a CSPRNG.
* At most 6 peers, 16 replicated items (key < 20 bytes, value < 48).
* Raft keeps a physical window of 16 entries. `p2p-raft-compact` drops the
  applied prefix and the next index can pass 16 (unit proof: index 17).
  A follower whose match is behind `snapshot_index` receives InstallSnapshot
  (chunks of the kv image taken at compact time) and then the later entries.
  The leader lease is 10 s. It is renewed when a heartbeat reaches a peer.
  A fully cut leader does not extend it. A follower that still holds the
  lease does not campaign. Append entries are retried by the leader. User
  datagrams are still not retransmitted; replication is repaired by
  `p2p-sync`, the one-round proposal by timeout.
* Typed RPC is not HTTP/2 gRPC. Deadlines are guest ticks. A repeated
  request id replays the last reply (`rpc_replays`).
* STUN messages are real RFC 5389 Binding messages, but the NAT is simule
  (unit bus) and NAT punch stays virtual. There is no public internet, no
  public STUN server and no real NAT on the QEMU hub. The mapped port is
  `P2P_PORT` (the datagram API does not expose the UDP source port).
* The membership secret is a shared key typed on the console; it is not
  stored by the guest. Raft term and vote are stored in the p2pkv blob
  trailer (`R` `F`, version 1). Old blobs without the trailer still load.

## Proofs

* `tests/unit/kernel/test_net_p2p.c`: framing round trip and rejections.
* `tests/unit/userspace/test_p2p.c`: 3-4 nodes on an in-process bus:
  discovery and pair keys, wrong network key, encryption/tamper/replay,
  replication + LWW + partition sync + remote get, consensus commit /
  reject / timeout, failure detection + relay + heal, rate limit + stats,
  bad inputs, Raft log replication and catch-up, InstallSnapshot, leader
  lease kept alive by heartbeats, QoS window and cache invalidation,
  typed RPC id and deadline, 4-node multi-relay chain, adaptive QoS
  controller, versioned cache with acked invalidation, learned traffic
  baseline.
* `tests/scripts/test_qemu_p2p.py` (`make qemu-p2p`, CI job "QEMU P2P three
  guests"): three guests with the Ring 3 networker on the strict kernel;
  discovery, encrypted message (hub asserts no plaintext), health, stats,
  replication, hub wire cut then sync (scenario 2, `cut_pair`, not
  `p2p-block`), majority commit, Raft pre-vote, leader lease, hub wire
  cut with compaction and InstallSnapshot (scenario 2b), commit of index
  3, link failure detection and relayed delivery, `p2p-qos` after the
  cut (scenario 3), acked cache invalidation relayed through beta
  (scenario 4).

## Lot 9

- US-083 transport: `devsync-pull DEV PEER REMOTE_DEV [SECONDS]` sends the
  local version vector to a peer guest over the sealed P2P channel (app
  payload `D Q`); the peer answers with the delta of the entries not seen
  (`D R`, at most 396 bytes) and the puller applies it. Proven both ways in
  `make qemu-p2p` scenario 1b, with a concurrent write resolved by
  last-writer-wins and no plaintext on the wire.
- US-067: gRPC over the guest TLS session, see `make qemu-net-peer-tls-worker`
  (`grpc-tls-serve`, `grpc-tls-call`).
