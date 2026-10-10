#!/usr/bin/env python3
"""Phase 7 collaborative ledger (US-091..US-105): three QEMU guests.

Each guest runs the Ring 3 networker, a P2P node (phase 5) and the collab
ledger on top. The hub only floods frames (no DHCP/DNS/internet). Proves:
points grant, shared resource booking with capacity, distributed task with
escrow and deterministic arbitration (right result paid, wrong result
refunded), reputation, governance vote, private vs shared profile with
right to be forgotten, support ticket, anti-entropy sync, and the same
audit digest on the three guests. No real payment exists anywhere.
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
            base.wait_for(node["proc"], node["log"], "(-.-)", base.BOOT_TIMEOUT)
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
