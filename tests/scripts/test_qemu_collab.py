#!/usr/bin/env python3
"""Phase 7 collaborative ledger (US-091..US-105): three QEMU guests.

Each guest runs the Ring 3 networker, a P2P node (phase 5) and the collab
ledger on top. The hub only floods frames (no DHCP/DNS/internet). Proves:
points grant, shared resource booking with capacity, distributed task with
escrow and deterministic arbitration (right result paid, wrong result
refunded), reputation, governance vote, private vs shared profile with
right to be forgotten, support ticket, anti-entropy sync, and the same
audit digest on the three guests. Lot 7 adds escrow contracts with an
arbiter, a dispute settled by a ruling of the uninvolved member and an
auction with a rejected low bid. No real payment exists anywhere.
"""
from __future__ import print_function

import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_p2p as p2p  # noqa: E402

base = p2p.base
p2p.SECRETS = (b"paris", b"bxexample", b"raise grant")
NODES = [
    p2p.inst("a", "52:54:00:a0:70:0a", "alpha", "10.77.0.1"),
    p2p.inst("b", "52:54:00:a0:70:0b", "beta", "10.77.0.2"),
    p2p.inst("c", "52:54:00:a0:70:0c", "gamma", "10.77.0.3"),
]
for node in NODES:
    for key in ("log", "err", "mon"):
        node[key] = node[key].replace("p2p-", "collab-")
cmd, log, say = p2p.cmd, p2p.log, p2p.say


def wait_log(node, needle, timeout):
    """Wait for a ledger entry; datagrams are not retransmitted, so after a
    quiet spell ask the peers for missing entries (collab-sync) and retry."""
    if not needle.startswith("collab got "):
        return p2p.wait_log(node, needle, timeout)
    for attempt in range(4):
        try:
            return p2p.wait_log(node, needle, 20)
        except Exception:  # noqa: BLE001 - base.wait_for timeout
            if attempt == 3:
                raise
            say("%s: %r missing, syncing" % (node["label"], needle))
            cmd(node, "collab-sync", "collab-sync ok asked ")


def main():
    # own CI job since lot 9: nothing else has created test_logs yet
    os.makedirs(p2p.LOG_DIR, exist_ok=True)
    hub = p2p.P2pHub()
    hub.start()
    t0 = time.monotonic()
    try:
        for node in NODES:
            base.start_guest(node, hub.port)
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline and hub.client_count < 3:
            time.sleep(0.1)
        for node in NODES:
            base.wait_for(node["proc"], node["log"], "[0m : ", base.BOOT_TIMEOUT)
            node["client"] = base.monitor_connect(node["mon"])

        def alive():
            for node in NODES:
                if node["proc"].poll() is not None:
                    raise RuntimeError("guest %s died" % node["label"])

        for node in NODES:
            base.spawn_networker(node, alive)
        for node in NODES:
            base.run_command(node, "p2p-up %s %s %s" % (node["name"], node["ip"], p2p.NETKEY),
                             "p2p-up ok name %s" % node["name"], alive, timeout=120)
        a, b, c = NODES
        for node in NODES:
            for other in NODES:
                if other is not node:
                    wait_log(node, "p2p peer %s up id " % other["name"], 180)
        say("p2p mesh up (%.0fs)" % (time.monotonic() - t0))

        # US-091 points: one grant per member.
        for node in NODES:
            cmd(node, "collab-join", "collab-join ok seq 1")
        for node in NODES:
            for other in NODES:
                if other is not node:
                    wait_log(node, "collab got join from %s seq 1" % other["name"], 60)
        # US-092/094 shared resource with capacity 1.
        cmd(a, "collab-offer gpu-slot 10 1", "collab-offer ok seq 2")
        wait_log(b, "collab got offer from alpha seq 2", 60)
        cmd(b, "collab-reserve alpha 2", "collab-reserve ok seq 2")
        wait_log(c, "collab got reserve from beta seq 2", 60)
        cmd(c, "collab-reserve alpha 2", "collab-reserve ok seq 2")
        # US-096/099/101 task with escrow, right result paid.
        cmd(a, "collab-task 20 sum 100", "collab-task ok seq 3")
        wait_log(c, "collab got task from alpha seq 3", 60)
        cmd(c, "collab-claim alpha 3", "collab-claim ok seq 3")
        cmd(c, "collab-work alpha 3", "collab-work computed sum 100 = 5050")
        wait_log(a, "collab got done from gamma seq 4", 60)
        cmd(a, "collab-review 3", "collab-review result correct, accepting")
        # wrong result refunded.
        cmd(b, "collab-task 15 primes 100", "collab-task ok seq 3")
        wait_log(c, "collab got task from beta seq 3", 60)
        cmd(c, "collab-claim beta 3", "collab-claim ok seq 5")
        cmd(c, "collab-work beta 3 wrong", "collab-work computed primes 100 = 26")
        wait_log(b, "collab got done from gamma seq 6", 60)
        cmd(b, "collab-review 3", "collab-review result wrong, rejecting")
        say("resources and tasks done (%.0fs)" % (time.monotonic() - t0))
        # US-095 reputation (rights from the booking and the paid task).
        cmd(b, "collab-rate alpha 4", "collab-rate ok seq 5")
        cmd(a, "collab-rate gamma 5", "collab-rate ok seq 5")
        # US-100 governance.
        cmd(a, "collab-propose raise grant", "collab-propose ok seq 6")
        wait_log(b, "collab got propose from alpha seq 6", 60)
        wait_log(c, "collab got propose from alpha seq 6", 60)
        cmd(b, "collab-vote alpha 6 yes", "collab-vote ok seq 6")
        cmd(c, "collab-vote alpha 6 yes", "collab-vote ok seq 7")
        # US-102/103/104 personal data.
        cmd(b, "collab-profile set email bxexample private", "collab-profile ok private kept local")
        cmd(b, "collab-profile set city paris shared", "collab-profile ok seq 7")
        wait_log(a, "collab got profile from beta seq 7", 60)
        cmd(a, "collab-profile show beta", "collab profile beta shared city=paris;")
        cmd(b, "collab-export", "collab export ok entries 7")
        if "collab export field email=bxexample private" not in log(b):
            raise RuntimeError("export lacks the private field")
        cmd(b, "collab-forget", "collab-forget ok seq 8")
        wait_log(a, "collab got redact from beta seq 8", 60)
        cmd(a, "collab-profile show beta", "collab profile beta shared (none)")
        # US-105 support.
        cmd(c, "collab-ticket how to reserve", "collab-ticket ok seq 8")
        wait_log(a, "collab got ticket from gamma seq 8", 60)
        cmd(a, "collab-answer gamma 8 use collab-reserve", "collab-answer ok seq 7")
        say("reputation, governance, privacy, support done (%.0fs)" % (time.monotonic() - t0))

        # US-093 per-node signatures: gamma forges alpha -> gamma transfers.
        # A forged datagram can be lost like any other, so each forgery is
        # re-sent until both honest guests logged its rejection.
        for node in NODES:
            cmd(node, "collab-keys", "collab-keys ok known 3 rejected 0 signing on")
        for mode, reason in (("victimkey", "bad-signature"), ("ownkey", "key-mismatch"), ("unsigned", "unsigned")):
            needle = "collab reject %s transfer claimed from alpha seq 8 via gamma" % reason
            for attempt in range(4):
                start_a, start_b = len(log(a)), len(log(b))
                cmd(c, "collab-forge alpha 50 %s" % mode, "collab-forge sent %s to " % mode)
                try:
                    p2p.wait_log(a, needle, 15)
                    p2p.wait_log(b, needle, 15)
                    break
                except Exception:  # noqa: BLE001
                    if attempt == 3:
                        raise
                    say("forgery %s not seen yet, re-sending" % mode)
            del start_a, start_b
        for node in (a, b):
            start = cmd(node, "collab-keys", "collab-keys ok known 3 rejected ")
            match = re.search(r"collab-keys ok known 3 rejected (\d+) signing on", log(node, start))
            if not match or int(match.group(1)) < 3:
                raise RuntimeError("forgeries not all rejected on %s" % node["label"])
        say("forged entries rejected by signature checks (%.0fs)" % (time.monotonic() - t0))

        # Anti-entropy then identical audit (US-098) on the three guests. A
        # peer can flap down for a moment under load (heartbeat), so sync is
        # repeated until the three ledgers hold all 23 entries.
        digests = []
        for attempt in range(5):
            for node in NODES:
                cmd(node, "collab-sync", "collab-sync ok asked ")
            for node in NODES:
                cmd(node, "p2p-poll 4", "p2p-poll ok", timeout=30)
            digests = []
            for node in NODES:
                start = cmd(node, "collab-audit", "collab audit entries ")
                match = re.search(r"collab audit entries (\d+) applied (\d+) rejected (\d+) digest ([0-9a-f]{16})", log(node, start))
                if not match:
                    raise RuntimeError("audit line missing on %s" % node["label"])
                digests.append((match.group(1), match.group(4)))
            say("audit round %d: %r" % (attempt + 1, digests))
            if all(d[0] == "23" for d in digests):
                break
        if any(d[0] != "23" for d in digests) or len(set(digests)) != 1:
            raise RuntimeError("ledger diverged: %r" % digests)
        digests = [d[1] for d in digests]
        audit = log(a)
        if "collab rejected reserve gamma seq 2 reason capacity-full" not in audit:
            raise RuntimeError("capacity rejection missing in audit")
        start = cmd(a, "collab-balances", "collab balances ok members 3")
        out = log(a, start)
        for needle in ("collab acct alpha balance 90 rep 4.0 ratings 1", "collab acct beta balance 90 rep none",
                       "collab acct gamma balance 120 rep 5.0 ratings 1"):
            if needle not in out:
                raise RuntimeError("balance missing %r: %s" % (needle, out[-600:]))
        cmd(c, "collab-tasks", "collab tasks ok 2")
        if "'sum 100' reward 20 state paid worker gamma" not in log(c) or "'primes 100' reward 15 state refused worker gamma" not in log(c):
            raise RuntimeError("task states wrong: %s" % log(c)[-800:])
        cmd(c, "collab-votes", "yes 2 no 0 members 3 status passed")
        cmd(b, "collab-tickets", "collab tickets ok open 0 answered 1")
        say("phase 7 core ledger verified (%.0fs)" % (time.monotonic() - t0))

        # Lot 7: escrow contracts (US-099), disputes with arbitration
        # (US-101) and auctions (US-094), all folded from the shared ledger.
        def emit(node, line, prefix):
            start = cmd(node, line, prefix + " ok seq ")
            match = re.search(re.escape(prefix) + r" ok seq (\d+)", log(node, start))
            if not match:
                raise RuntimeError("%s: no seq in %r" % (node["label"], log(node, start)[-300:]))
            return int(match.group(1))

        def seen(nodes, kind, origin, seq):
            for node in nodes:
                wait_log(node, "collab got %s from %s seq %d" % (kind, origin["name"], seq), 60)

        s1 = emit(a, "collab-escrow-create beta gamma 30 design", "collab-escrow-create")
        seen((b, c), "escrow", a, s1)
        r1 = emit(a, "collab-escrow-release alpha %d" % s1, "collab-escrow-release")
        seen((b, c), "settle", a, r1)
        s2 = emit(b, "collab-contract-create alpha none 20 repair", "collab-contract-create")
        seen((a, c), "escrow", b, s2)
        d2 = emit(a, "collab-dispute-open beta %d work not delivered" % s2, "collab-dispute-open")
        seen((b, c), "dispute", a, d2)
        v2 = emit(c, "collab-dispute-vote beta %d payer" % s2, "collab-dispute-vote")
        seen((a, b), "ruling", c, v2)
        au = emit(c, "collab-auction lamp 10", "collab-auction")
        seen((a, b), "auction", c, au)
        b1 = emit(a, "collab-market-bid gamma %d 12" % au, "collab-market-bid")
        seen((b, c), "bid", a, b1)
        b2 = emit(b, "collab-market-bid gamma %d 15" % au, "collab-market-bid")
        seen((a, c), "bid", b, b2)
        b3 = emit(a, "collab-market-bid gamma %d 14" % au, "collab-market-bid")
        seen((b, c), "bid", a, b3)
        cl = emit(c, "collab-auction-close %d" % au, "collab-auction-close")
        seen((a, b), "close", c, cl)
        say("escrow, dispute and auction entries emitted (%.0fs)" % (time.monotonic() - t0))
        # Lot 9 US-099: contracts with a program, executed by the fold on
        # every guest once the conditions hold (sign-offs).
        k1 = emit(a, "collab-contract-deploy beta gamma 10 pay if payee arb", "collab-contract-deploy")
        seen((b, c), "contract", a, k1)
        o1 = emit(b, "collab-signoff alpha %d" % k1, "collab-signoff")
        seen((a, c), "signoff", b, o1)
        o2 = emit(c, "collab-signoff alpha %d" % k1, "collab-signoff")
        seen((a, b), "signoff", c, o2)
        k2 = emit(c, "collab-contract-deploy alpha none 20 split 25 if sigs 1", "collab-contract-deploy")
        seen((a, b), "contract", c, k2)
        o3 = emit(b, "collab-signoff gamma %d" % k2, "collab-signoff")
        seen((a, c), "signoff", b, o3)
        say("contract programs deployed and signed off (%.0fs)" % (time.monotonic() - t0))
        digests = []
        for attempt in range(5):
            for node in NODES:
                cmd(node, "collab-sync", "collab-sync ok asked ")
            for node in NODES:
                cmd(node, "p2p-poll 4", "p2p-poll ok", timeout=30)
            digests = []
            for node in NODES:
                start = cmd(node, "collab-audit", "collab audit entries ")
                match = re.search(r"collab audit entries (\d+) applied (\d+) rejected (\d+) digest ([0-9a-f]{16})", log(node, start))
                if not match:
                    raise RuntimeError("audit line missing on %s" % node["label"])
                digests.append((match.group(1), match.group(4)))
            say("lot 7 audit round %d: %r" % (attempt + 1, digests))
            if all(d[0] == "38" for d in digests):
                break
        if any(d[0] != "38" for d in digests) or len(set(digests)) != 1:
            raise RuntimeError("ledger diverged after lot 7 phase: %r" % digests)
        digests = [d[1] for d in digests]
        if "collab rejected bid alpha seq %d reason bid-too-low" % b3 not in log(a):
            raise RuntimeError("low bid not rejected in audit")
        for node in NODES:
            start = cmd(node, "collab-balances", "collab balances ok members 3")
            out = log(node, start)
            for needle in ("collab acct alpha balance 55 ", "collab acct beta balance 115 ", "collab acct gamma balance 130 "):
                if needle not in out:
                    raise RuntimeError("lot 7 balance missing %r on %s: %s" % (needle, node["label"], out[-600:]))
        start = cmd(b, "collab-escrows", "collab escrows ok 4")
        out = log(b, start)
        for needle in ("collab escrow alpha#%d 'pay if payee arb' payee beta arbiter gamma amount 10 state released "
                       "program clauses 1 signoffs 2 fired 1 to_payee 10 rulings payer 0 payee 0" % k1,
                       "collab escrow gamma#%d 'split 25 if sigs 1' payee alpha arbiter none amount 20 state split "
                       "program clauses 1 signoffs 1 fired 1 to_payee 5 rulings payer 0 payee 0" % k2,
                       "collab escrow alpha#%d 'design' payee beta arbiter gamma amount 30 state released rulings payer 0 payee 0" % s1,
                       "collab escrow beta#%d 'repair' payee alpha arbiter none amount 20 state refunded rulings payer 1 payee 0" % s2):
            if needle not in out:
                raise RuntimeError("escrow view missing %r: %s" % (needle, out[-600:]))
        # Lot 8 US-103: Schnorr NIZK in the guest (secret stays on the node).
        start = cmd(b, "collab-zk-prove 4242 pay10", "collab-zk-prove public ")
        out = log(b, start)
        if "verify ok tampered rejected wrong-message rejected wrong-public rejected" not in out:
            raise RuntimeError("zk proof: %s" % out[-600:])
        # Lot 9 US-103: the same proof in a 2048-bit group, 256-bit subgroup.
        start = cmd(b, "collab-zk-strong 4242 pay10", "about 112-bit group, not constant time", timeout=120)
        out = log(b, start)
        if ("collab-zk-strong group p2048 q256 check ok public " not in out
                or "proof_bytes 288 verify ok tampered rejected wrong-message rejected wrong-public rejected" not in out):
            raise RuntimeError("zk strong proof: %s" % out[-600:])
        m = re.search(r"prove_ms (\d+) verify_ms (\d+)", out)
        say("zk 2048/256 proof in the guest: prove %s ms verify %s ms" % (m.group(1), m.group(2)) if m else "zk timings missing")
        start = cmd(a, "collab-auctions", "collab auctions ok 1")
        needle = "collab auction gamma#%d 'lamp' reserve 10 best 15 bidder beta bids 2 state sold" % au
        if needle not in log(a, start):
            raise RuntimeError("auction view missing %r: %s" % (needle, log(a, start)[-600:]))
        if hub.p2p_plaintext:
            raise RuntimeError("plaintext seen on the wire (%d frames)" % hub.p2p_plaintext)
        say("PASS digest %s sealed=%d plaintext=0 in %.0fs" % (digests[0], hub.p2p_sealed, time.monotonic() - t0))
        return 0
    finally:
        for node in NODES:
            if node.get("client"):
                try:
                    node["client"].close()
                except OSError:
                    pass
            base.terminate(node.get("proc"))
            if node.get("err_handle"):
                node["err_handle"].close()
        hub.close()


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:  # noqa: BLE001
        print("MOHHDY collab three-guest contract failed: %s" % error)
        sys.exit(1)
