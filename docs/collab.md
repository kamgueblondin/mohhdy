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

Trust is authenticated by the network key and per-pair AES-GCM (phase 5),
and entries are signed per node with Schnorr signatures (#121) using trust
on first use (TOFU) key binding.

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

`collab-join`, `collab-pay PEER N`, `collab-pay-convert POINTS CURRENCY`,
`collab-offer NAME PRICE CAP`, `collab-reserve OWNER SEQ`,
`collab-escrow-create PAYEE ARBITER|none AMOUNT [NAME]` (alias `collab-contract-create`),
`collab-escrow-release PAYER SEQ`, `collab-escrow-refund PAYER SEQ`, `collab-contract-exec PAYER SEQ release|refund`,
`collab-dispute-open PAYER SEQ EVIDENCE`, `collab-dispute-vote PAYER SEQ payer|payee`,
`collab-auction ITEM RESERVE`, `collab-market-bid SELLER SEQ AMOUNT`, `collab-auction-close SEQ`,
`collab-market-search`, `collab-market-price`, `collab-privacy-proof`, `collab-compliance`,
`collab-task REWARD SPEC`, `collab-claim REQ SEQ`,
`collab-work REQ SEQ`, `collab-review SEQ`, `collab-rate PEER STARS`,
`collab-propose TEXT`, `collab-vote PROPOSER SEQ yes|no`,
`collab-profile set KEY VALUE shared|private`, `collab-profile show PEER`,
`collab-forget`, `collab-export`, `collab-ticket TEXT`,
`collab-ticket-route SEQ HELPER`, `collab-ticket-assign SEQ HELPER`,
`collab-answer PEER SEQ TEXT`, `collab-sync`, and the views
`collab-balances`, `collab-audit`, `collab-tasks`, `collab-offers`,
`collab-votes`, `collab-tickets`, `collab-escrows`, `collab-auctions`.
`collab-pay-convert` uses fixed rates: simule, no real money.

## User stories

| US | Status | Notes |
|----|--------|-------|
| US-091 points | done | grant, transfers, overdraft refused; QEMU |
| US-092 shared resources | done | offers with capacity, bookings; QEMU |
| US-093 distributed authentication | done | membership by network key + per-pair AEAD (phase 5) and per-node Schnorr signatures (#121) |
| US-094 resource marketplace | done | priced offers, paid bookings, search, price; Lot 7: auctions folded from the ledger (best bid locked, outbid refunded, low bid rejected `bid-too-low`, close pays the seller); QEMU |
| US-095 reputation | done | ratings gated by completed exchanges, average shown; QEMU |
| US-096 distributed tasks | done | post with escrow, claim, compute on another guest, review; QEMU |
| US-097 decentralized payment | partial | points and escrow are real ledger state; `collab-pay-convert` uses fixed rates and is labelled simule; no real money or chain |
| US-098 transparent audit | done | canonical digest identical on all guests, rejected entries listed; QEMU |
| US-099 smart contracts | partial | Lot 7: escrow contracts with optional arbiter, state machine open/released/refunded/disputed folded from the ledger (`collab-escrow-*`, `collab-contract-*`); QEMU. Fixed templates, not a programmable contract language |
| US-100 governance | done | proposals and majority votes; QEMU |
| US-101 conflict resolution | done | Lot 7: dispute freezes an escrow, ruling by the arbiter or a majority of uninvolved members settles it (`collab-dispute-open`, `collab-dispute-vote`); QEMU |
| US-102 personal data manager | done | local profile with private/shared fields, export; QEMU |
| US-103 advanced privacy | partial | private fields stay local, redaction; Lot 7: `collab-privacy-proof` is a hash commitment (open with value and secret, wrong value rejected), not a zero-knowledge proof |
| US-104 regulatory compliance | partial | export and right to be forgotten; Lot 7: `collab-compliance` computes 4 checks (signing on, all entries signed, no forgery accepted, private fields local); not a legal audit |
| US-105 community support | done | P2P tickets, routing, assignments and answers (`collab-ticket-*`, `collab-answer`) |

## Limits

96 entries per ledger, 8 members, 16 offers/tasks, 8 proposals; ledger in
RAM (lost at reboot); a node that never receives the redact entry keeps its
copy of the redacted profile.

## Per-node signatures (consolidation lot, #121)

`collab-join` derives a per-node keypair (Schnorr over the RFC 5114
1024-bit MODP group with a 160-bit prime-order subgroup, `userspace/csig.c`;
deterministic nonce SHA-256(sk || message); Montgomery exponentiation).
Every entry is signed over its 32 header bytes plus SHA-256 of its original
text, so a later redaction of a profile text keeps the signature valid. The
wire message carries entry, text hash, signature and the origin's public
key. A receiver binds the first public key it verifies for an origin
(trust on first use) and from then on rejects: entries signed with another
key (`key-mismatch`), invalid signatures (`bad-signature`), keys outside
the subgroup (`bad-key`) and, once it signs itself, unsigned entries
(`unsigned`). Duplicates are not re-verified. `collab-keys` lists
fingerprints and the rejection count; `collab-forge NAME AMOUNT
victimkey|ownkey|unsigned` is the test hook used by `make qemu-collab`.

Limits: trust on first use (a forger who reaches a node before the real
origin's first entry could bind its own key; keys are not certified); the
1024/160 group is legacy strength; signing and verification are not constant
time; the secret key is kept in RAM and, with `persist-save`, in clear on the
disk.

## Lot 9

- US-099: contract language MCL. `collab-contract-deploy PAYEE ARBITER|none AMOUNT PROGRAM`
  compiles a program of clauses `pay|refund|split N if COND...` (conditions:
  `sigs N`, `payee`, `arb`, `after N` Lamport ticks); `collab-signoff PAYER SEQ`
  adds a sign-off. Every replica evaluates the program after each entry, so the
  outcome is the same everywhere (`collab-escrows` shows `program clauses N
  signoffs N fired K to_payee N`). No real money: points only.
- US-104: `collab-compliance` runs six computed checks (signing on, all
  signed, no forgery accepted with signatures re-verified, private data kept
  local, erasure effective, keys bound) and prints a GDPR article map. It is a
  self-check, not a certification.
- US-103: `collab-zk-strong SECRET MESSAGE` proves knowledge of a discrete log
  in a 2048-bit group with a 256-bit prime-order subgroup (`csig_zk_*` in
  `userspace/csig.c`). Parameters come from `scripts/gen_zk_group.py` (fixed
  labels, re-run to check) and are re-checked in the guest (q | p-1, g^q = 1,
  Fermat on p). Known answer test against an independent Python run. Not
  constant time. `collab-zk-prove` (62-bit toy group) is kept for comparison.
