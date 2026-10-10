# Phase 7 - Collaborative layer (US-091 to US-105)

Status: delivered in the guest as C code (`userspace/collab.c`, pure and
host-tested; shell front end `userspace/shell_collab.c`), on top of the
phase 5 P2P node (#117, sealed `P2P_I_APP` payloads). Proven by unit tests
and a three-guest QEMU contract (`make qemu-collab`). No real money, no
payment provider, no blockchain: points are local accounting units.

## Model

A replicated append-only ledger. Each entry has a type, origin node id,
per-origin sequence, Lamport clock, counterparty, reference, amount, aux
value and a 40-byte text. Every node folds the entries it knows in the
same canonical order (Lamport, origin, sequence); invalid entries are
rejected with the same reason everywhere, so balances, bookings, tasks,
votes and profiles converge. `collab-audit` prints entry counts and a
SHA-256 digest of the canonical ledger (with the verdict of each entry):
equal digests mean equal ledgers. Missing entries are repaired with
`collab-sync` (two-way anti-entropy by origin and sequence).

Trust is membership-level: transport is authenticated by the network key
and per-pair AES-GCM (phase 5), but entries are not signed per node, so a
member relaying entries could forge another member's entry. There is no
signing primitive for this in the tree yet (the TLS stack only verifies).

## Rules applied by the fold

* join: one grant of 100 points per member (a second join is rejected).
* transfer: positive amount, enough points, to a joined member.
* offer / reserve: a reservation pays the price to the owner, within the
  offer capacity; owners cannot book their own offer.
* task / claim / done / accept / reject: the reward is held in escrow when
  the task is posted. Tasks are deterministic (`sum N`, `primes N`,
  `fnv TEXT`), so every node recomputes the result: accept is valid only for
  a right result (pays the worker), reject only for a wrong one (refunds the
  requester). This is the conflict resolution rule.
* rate: 1 to 5 stars, only between parties of a completed booking or paid
  task; a new rating replaces the previous one of the same rater.
* propose / vote: one vote per member; passed when yes > members / 2,
  rejected when no >= members / 2.
* profile / redact: only fields marked `shared` leave the node; `private`
  fields stay local. `collab-forget` wipes the local profile and emits a
  redact entry: every node that receives it blanks the earlier profile texts
  of that member in its ledger copy (`[redacted]`).
* ticket / answer: support requests answered by another member.

## Commands

`collab-join`, `collab-pay PEER N`, `collab-offer NAME PRICE CAP`,
`collab-reserve OWNER SEQ`, `collab-task REWARD SPEC`, `collab-claim REQ SEQ`,
`collab-work REQ SEQ`, `collab-review SEQ`, `collab-rate PEER STARS`,
`collab-propose TEXT`, `collab-vote PROPOSER SEQ yes|no`,
`collab-profile set KEY VALUE shared|private`, `collab-profile show PEER`,
`collab-forget`, `collab-export`, `collab-ticket TEXT`,
`collab-answer PEER SEQ TEXT`, `collab-sync`, and the views
`collab-balances`, `collab-audit`, `collab-tasks`, `collab-offers`,
`collab-votes`, `collab-tickets`.

## User stories

| US | Status | Notes |
|----|--------|-------|
| US-091 points | done | grant, transfers, overdraft refused; QEMU |
| US-092 shared resources | done | offers with capacity, bookings; QEMU |
| US-093 distributed authentication | partial | membership by network key + per-pair AEAD (phase 5); no per-node signatures |
| US-094 resource marketplace | partial | priced offers and paid bookings; no search, auction or price discovery |
| US-095 reputation | done | ratings gated by completed exchanges, average shown; QEMU |
| US-096 distributed tasks | done | post with escrow, claim, compute on another guest, review; QEMU |
| US-097 decentralized payment | not delivered | no real payment by design; points only |
| US-098 transparent audit | done | canonical digest identical on all guests, rejected entries listed; QEMU |
| US-099 smart contracts | partial | fixed built-in contracts (escrow task, booking); no user-defined contracts |
| US-100 governance | done | proposals and majority votes; QEMU |
| US-101 conflict resolution | partial | deterministic re-verification of task results; no human dispute process |
| US-102 personal data manager | done | local profile with private/shared fields, export; QEMU |
| US-103 advanced privacy | partial | private fields never leave the node, redaction on forget, sealed transport (hub checks no plaintext); no anonymity |
| US-104 regulatory compliance | partial | data export and right to be forgotten mechanics only; no legal compliance claim |
| US-105 community support | done | tickets and answers; QEMU |

## Limits

96 entries per ledger, 8 members, 16 offers/tasks, 8 proposals; ledger in
RAM (lost at reboot); a node that never receives the redact entry keeps its
copy of the redacted profile.
