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
`p2p-limit N`, `p2p-poll SECONDS`. While a node is up the console reads
keys without blocking and pumps the node between keys (same mechanism as
background `web-serve`); events print asynchronously (`p2p peer X up`,
`p2p msg from X: ...`, `p2p kv replicated ...`).

## User stories

| US | Status | What exists |
|----|--------|-------------|
| US-061 protocol | done | framing + header + types above; QEMU 3 guests |
| US-062 discovery | done | authenticated HELLO, peer table, up/down; QEMU |
| US-063 routing | partial | one relay hop chosen from neighbor lists when the direct link is down; QEMU. No multi-hop path costs |
| US-064 consensus | partial | single-round majority vote with commit, reject, timeout; QEMU commit. Not Raft/Paxos: no leader term, no log |
| US-065 replication | done | put pushed to all peers, last writer wins (version, origin); QEMU |
| US-066 encryption | done | X25519 + AES-128-GCM per pair, PSK-authenticated, replay protection; QEMU checks no plaintext on the wire |
| US-067 gRPC | not delivered | no HTTP/2 or protobuf in the guest |
| US-068 bandwidth | partial | per-peer message budget per second (`p2p-limit`), throttle counters; unit tests only |
| US-069 fault tolerance | partial | failure detection (10 s), relay rerouting, resync after partition; QEMU. No state persistence across reboot |
| US-070 distributed cache | partial | remote get with miss reporting; unit tests only |
| US-071 synchronization | done | two-way digest anti-entropy (`p2p-sync`); QEMU after a partition |
| US-072 health monitor | done | per-peer RTT, last seen, sent/recv, auth failures, replays, throttled, down events; QEMU |
| US-073 adaptive QoS | partial | control frames bypass the user budget; no adaptation to measured load |
| US-074 NAT traversal | not delivered | single shared segment, no NAT to traverse in the lab |
| US-075 traffic analysis | partial | per-type tx/rx counters, bytes, relayed, duplicates, foreign frames (`p2p-stats`); no automatic analysis |

## Limits (honest)

* Key seed in the guest is TSC + ticks mixed with SHA-256: weak entropy in
  emulation, fine for a lab, not a CSPRNG.
* At most 6 peers, 16 replicated items (key < 20 bytes, value < 48).
* Datagrams are not retransmitted: user messages may be lost; replication
  is repaired by `p2p-sync`, consensus by timeout.
* The membership secret is a shared key typed on the console; it is not
  stored by the guest.

## Proofs

* `tests/unit/kernel/test_net_p2p.c`: framing round trip and rejections.
* `tests/unit/userspace/test_p2p.c`: 3-4 nodes on an in-process bus:
  discovery and pair keys, wrong network key, encryption/tamper/replay,
  replication + LWW + partition sync + remote get, consensus commit /
  reject / timeout, failure detection + relay + heal, rate limit + stats,
  bad inputs.
* `tests/scripts/test_qemu_p2p.py` (`make qemu-p2p`, CI job "QEMU P2P three
  guests"): three guests with the Ring 3 networker on the strict kernel;
  discovery, encrypted message (hub asserts no plaintext), health, stats,
  replication, partition then sync, majority commit, link failure detection
  and relayed delivery.
