#!/usr/bin/env python3
"""Phase 5 P2P on four QEMU guests (US-063 multi-hop, US-069 crash/rejoin).

Same shared Ethernet hub as qemu-p2p, one more guest (delta). The hub
imposes the topology by dropping frames between guest MACs (wire cuts):
  1. line alpha - beta - gamma - delta: alpha reaches delta through two
     relays (beta forwards to a further relay, gamma delivers);
  2. diamond alpha-beta, alpha-gamma, beta-delta, gamma-delta (alpha-delta
     and beta-gamma cut), then beta's QEMU process is killed (node crash):
     alpha and delta detect it, alpha still reaches delta through gamma;
  3. beta reboots (fresh QEMU, empty state, new identity under the same
     name) and rejoins: alpha logs the rejoin and pushes the replicated
     store back (anti-entropy), beta receives the key put during its crash
     and a direct message.
"""
from __future__ import print_function

import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_p2p as p3  # noqa: E402

base = p3.base
p3.SECRETS = (b"two-relays-msg", b"around-beta", b"welcome-back")
NETKEY = p3.NETKEY


def inst(label, mac, name, ip):
    node = p3.inst(label, mac, name, ip)
    for key in ("log", "err", "mon"):
        node[key] = node[key].replace("p2p-", "p2p-mesh-")
    return node


NODES = [
    inst("a", "52:54:00:a0:51:0a", "alpha", "10.77.0.1"),
    inst("b", "52:54:00:a0:51:0b", "beta", "10.77.0.2"),
    inst("c", "52:54:00:a0:51:0c", "gamma", "10.77.0.3"),
    inst("d", "52:54:00:a0:51:0d", "delta", "10.77.0.4"),
]


def say(msg):
    base.say("[p2p-mesh] " + msg)


def stat(node, name):
    start = p3.cmd(node, "p2p-stats", "p2p stats ok tx_bytes ")
    match = re.search(r"\b%s (\d+)" % name, p3.log(node, start))
    if not match:
        raise RuntimeError("%s: no %s in stats: %s" % (node["label"], name, p3.log(node, start)[-400:]))
    return int(match.group(1))


def boot(node, hub, alive):
    base.wait_for(node["proc"], node["log"], "[0m : ", base.BOOT_TIMEOUT)
    node["client"] = base.monitor_connect(node["mon"])
    base.spawn_networker(node, alive)
    base.run_command(node, "p2p-up %s %s %s" % (node["name"], node["ip"], NETKEY),
                     "p2p-up ok name %s" % node["name"], alive, timeout=120)


def main():
    if not os.path.isfile(base.KERNEL) or not os.path.isfile(base.INITRD):
        raise RuntimeError("missing KERNEL or INITRD (build first)")
    os.makedirs(p3.LOG_DIR, exist_ok=True)
    hub = p3.P2pHub()
    hub.start()
    t0 = time.monotonic()
    crashed = set()
    try:
        for node in NODES:
            base.start_guest(node, hub.port)
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline and hub.client_count < 4:
            time.sleep(0.1)
        if hub.client_count < 4:
            raise RuntimeError("hub expected 4 QEMU clients, got %d" % hub.client_count)

        def alive():
            for node in NODES:
                if node["label"] not in crashed and node["proc"].poll() is not None:
                    raise RuntimeError("guest %s died" % node["label"])
            if hub.error is not None:
                raise RuntimeError("hub error: %s" % hub.error)

        for node in NODES:
            base.wait_for(node["proc"], node["log"], "[0m : ", base.BOOT_TIMEOUT)
            node["client"] = base.monitor_connect(node["mon"])
        for node in NODES:
            base.spawn_networker(node, alive)
        for node in NODES:
            base.run_command(node, "p2p-up %s %s %s" % (node["name"], node["ip"], NETKEY),
                             "p2p-up ok name %s" % node["name"], alive, timeout=120)
        a, b, c, d = NODES
        for node in NODES:
            for other in NODES:
                if other is not node:
                    p3.wait_log(node, "p2p peer %s up id " % other["name"], 180)
        say("4 guests discovered and keyed (%.0fs)" % (time.monotonic() - t0))

        # 1. Line topology: two relays between alpha and delta (US-063).
        mark = {n["label"]: len(base.log_text(n["log"])) for n in NODES}
        hub.cut_pair(a["mac"], c["mac"])
        hub.cut_pair(a["mac"], d["mac"])
        hub.cut_pair(b["mac"], d["mac"])
        p3.wait_log(a, "p2p peer delta down", 90, mark["a"])
        p3.wait_log(a, "p2p peer gamma down", 90, mark["a"])
        p3.wait_log(b, "p2p peer delta down", 90, mark["b"])
        p3.cmd(a, "p2p-send delta two-relays-msg", "p2p-send ok to delta")
        p3.wait_log(d, "p2p msg from alpha: two-relays-msg (relayed)", 60)
        multi = stat(b, "relayed_multi")
        last = stat(c, "relayed")
        if multi < 1 or last < 1:
            raise RuntimeError("two-relay path not used: beta relayed_multi %d gamma relayed %d" % (multi, last))
        say("scenario 1 ok: alpha -> beta -> gamma -> delta (%.0fs)" % (time.monotonic() - t0))

        # 2. Diamond, then crash beta (US-069).
        hub.heal_pair(a["mac"], c["mac"])
        hub.heal_pair(b["mac"], d["mac"])
        hub.cut_pair(b["mac"], c["mac"])
        p3.wait_log(a, "p2p peer gamma up id ", 60, mark["a"])
        crash_mark = {n["label"]: len(base.log_text(n["log"])) for n in NODES}
        crashed.add("b")
        b["client"].close()
        b["client"] = None
        base.terminate(b["proc"])
        if b.get("err_handle"):
            b["err_handle"].close()
        p3.wait_log(a, "p2p peer beta down", 90, crash_mark["a"])
        p3.wait_log(d, "p2p peer beta down", 90, crash_mark["d"])
        p3.cmd(a, "p2p-put after crash", "p2p-put ok after")
        relayed_before = stat(c, "relayed")
        p3.cmd(a, "p2p-send delta around-beta", "p2p-send ok to delta")
        p3.wait_log(d, "p2p msg from alpha: around-beta (relayed)", 60)
        if stat(c, "relayed") <= relayed_before:
            raise RuntimeError("gamma did not relay after the crash")
        say("scenario 2 ok: beta crashed, alpha -> gamma -> delta (%.0fs)" % (time.monotonic() - t0))

        # 3. Beta reboots with an empty state and rejoins.
        b["log"] = b["log"].replace(".log", "-boot2.log")
        b["err"] = b["err"].replace(".err", "-boot2.err")
        rejoin_mark = {n["label"]: len(base.log_text(n["log"])) for n in (a, c, d)}
        base.start_guest(b, hub.port)
        crashed.discard("b")
        boot(b, hub, alive)
        p3.wait_log(a, "p2p peer beta rejoined id ", 120, rejoin_mark["a"])
        p3.wait_log(d, "p2p peer beta rejoined id ", 120, rejoin_mark["d"])
        p3.wait_log(b, "p2p kv synced after=crash v1 from alpha", 90)
        p3.cmd(b, "p2p-get after", "p2p-get ok local after=crash")
        p3.cmd(a, "p2p-send beta welcome-back", "p2p-send ok to beta")
        p3.wait_log(b, "p2p msg from alpha: welcome-back", 60)
        if stat(a, "rejoins") < 1:
            raise RuntimeError("alpha did not count the rejoin")
        if hub.p2p_plaintext:
            raise RuntimeError("plaintext secret seen on the wire")
        for node in NODES:
            if "[NET] relay aborted" in p3.log(node) or "Ring 0 fallback" in p3.log(node):
                raise RuntimeError("%s used a fallback path" % node["label"])
        say("scenario 3 ok: beta rebooted, rejoined, store pushed back (%.0fs)" % (time.monotonic() - t0))
        say("PASS frames=%d hello=%d sealed=%d plaintext=0 in %.0fs"
            % (hub.p2p_frames, hub.p2p_hello, hub.p2p_sealed, time.monotonic() - t0))
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
                try:
                    node["err_handle"].close()
                except OSError:
                    pass
        hub.close()


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:  # noqa: BLE001
        print("MOHHDY P2P four-guest mesh contract failed: %s" % error)
        sys.exit(1)
