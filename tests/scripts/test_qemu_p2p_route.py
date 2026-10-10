#!/usr/bin/env python3
"""Phase 5 US-063 multi-hop routing: four QEMU guests on a line topology.

The hub carries frames only along alpha-beta, beta-gamma and gamma-delta
(alpha-gamma, alpha-delta and beta-delta are cut from boot, by MAC). The
guests must discover the far end through relayed hellos, key the pair, and
deliver a sealed message over three hops (alpha -> beta -> gamma -> delta)
and back. No DHCP, DNS, TLS or public internet: the hub only floods.
"""
from __future__ import print_function

import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_ne2k_guest_tls_server as base  # noqa: E402
import test_qemu_p2p as p2p  # noqa: E402
from qemu_shared_ethernet_hub import SharedEthernetHub  # noqa: E402

NETKEY = "mohhdy-route-key"
SECRETS = (b"multi-hop-secret", b"multi-hop-back")


def inst(label, mac, name, ip):
    return {
        "label": label, "mac": mac, "name": name, "ip": ip,
        "log": os.path.join(base.LOG_DIR, "route-%s.log" % label),
        "err": os.path.join(base.LOG_DIR, "route-%s.err" % label),
        "mon": os.path.join(base.LOG_DIR, "route-%s.monitor.sock" % label),
    }


NODES = [
    inst("a", "52:54:00:a0:60:0a", "alpha", "10.77.0.1"),
    inst("b", "52:54:00:a0:60:0b", "beta", "10.77.0.2"),
    inst("c", "52:54:00:a0:60:0c", "gamma", "10.77.0.3"),
    inst("d", "52:54:00:a0:60:0d", "delta", "10.77.0.4"),
]


def mac_bytes(text):
    return bytes(int(x, 16) for x in text.split(":"))


CUTS = set()
for x, y in (("a", "c"), ("a", "d"), ("b", "d")):
    mx = mac_bytes([n["mac"] for n in NODES if n["label"] == x][0])
    my = mac_bytes([n["mac"] for n in NODES if n["label"] == y][0])
    CUTS.add((mx, my))
    CUTS.add((my, mx))


class LineHub(SharedEthernetHub):
    """Flood-only hub with a link matrix (line topology) and wire checks."""

    def __init__(self):
        SharedEthernetHub.__init__(self, respond=False, full_tls=False, proxy_peer_syn_ack=False)
        self.conn_mac = {}
        self.dropped = 0
        self.plaintext = 0
        self.leaked = 0

    def _handle_frame(self, connection, frame):
        if len(frame) >= 14:
            self.conn_mac[connection] = bytes(frame[6:12])
            for secret in SECRETS:
                if secret in frame:
                    self.plaintext += 1
        SharedEthernetHub._handle_frame(self, connection, frame)

    def _flood(self, source, frame):
        src = bytes(frame[6:12])
        with self._lock:
            clients = list(self._clients)
        for connection in clients:
            if connection is source:
                continue
            dst = self.conn_mac.get(connection)
            if dst is None or (src, dst) in CUTS:
                self.dropped += 1
                continue
            try:
                self._send_frame(connection, frame)
            except OSError:
                pass


def say(msg):
    base.say("[p2p-route] " + msg)


def main():
    if not os.path.isfile(base.KERNEL) or not os.path.isfile(base.INITRD):
        raise RuntimeError("missing KERNEL or INITRD (build first)")
    os.makedirs(base.LOG_DIR, exist_ok=True)
    hub = LineHub()
    hub.start()
    t0 = time.monotonic()
    try:
        for node in NODES:
            base.start_guest(node, hub.port)
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline and hub.client_count < 4:
            time.sleep(0.1)
        if hub.client_count < 4:
            raise RuntimeError("hub expected 4 QEMU clients, got %d" % hub.client_count)
        for node in NODES:
            base.wait_for(node["proc"], node["log"], "(-.-)", base.BOOT_TIMEOUT)
            node["client"] = base.monitor_connect(node["mon"])
        say("4 guests booted in %.0fs" % (time.monotonic() - t0))

        def alive():
            for node in NODES:
                if node["proc"].poll() is not None:
                    raise RuntimeError("guest %s died" % node["label"])
            if hub.error is not None:
                raise RuntimeError("hub error: %s" % hub.error)

        for node in NODES:
            base.spawn_networker(node, alive)
        # a guest receives nothing until the hub has learned its MAC from
        # its first frame (unknown destinations are dropped): the cuts hold
        # from the very first hello
        for node in NODES:
            base.run_command(node, "p2p-up %s %s %s" % (node["name"], node["ip"], NETKEY),
                             "p2p-up ok name %s" % node["name"], alive, timeout=120)
        a, b, c, d = NODES
        p2p.wait_log(a, "p2p peer beta up id ", 180)
        p2p.wait_log(d, "p2p peer gamma up id ", 180)
        p2p.wait_log(a, "p2p peer gamma reachable hops 2 via beta", 240)
        p2p.wait_log(a, "p2p peer delta reachable hops 3 via beta", 240)
        p2p.wait_log(d, "p2p peer alpha reachable hops 3 via gamma", 240)
        for node, far in ((a, ("gamma", "delta")), (b, ("delta",)), (d, ("alpha", "beta"))):
            for name in far:
                if "p2p peer %s up id " % name in p2p.log(node):
                    raise RuntimeError("%s saw %s directly despite the cut" % (node["label"], name))
        say("line topology discovered through relayed hellos (%.0fs)" % (time.monotonic() - t0))
        p2p.cmd(a, "p2p-route delta", "p2p-route delta next beta hops 3")
        p2p.cmd(a, "p2p-route beta", "p2p-route beta next beta hops 1")
        p2p.cmd(d, "p2p-route alpha", "p2p-route alpha next gamma hops 3")
        p2p.cmd(a, "p2p-send delta multi-hop-secret", "p2p-send ok to delta")
        p2p.wait_log(d, "p2p msg from alpha: multi-hop-secret (relayed)", 90)
        p2p.cmd(d, "p2p-send alpha multi-hop-back", "p2p-send ok to alpha")
        p2p.wait_log(a, "p2p msg from delta: multi-hop-back (relayed)", 90)
        for node in (b, c):
            start = p2p.cmd(node, "p2p-stats", "p2p stats ok tx_bytes ")
            relayed = re.search(r"relayed (\d+)", p2p.log(node, start))
            if not relayed or int(relayed.group(1)) < 2:
                raise RuntimeError("%s relayed too little: %s" % (node["label"], p2p.log(node, start)[-400:]))
        if hub.plaintext:
            raise RuntimeError("plaintext secret seen on the wire")
        if hub.dropped == 0:
            raise RuntimeError("the hub dropped nothing: the cuts were not exercised")
        for node in NODES:
            if "p2p stall" in p2p.log(node):
                raise RuntimeError("%s console loop stalled" % node["label"])
            if "Ring 0 fallback" in p2p.log(node):
                raise RuntimeError("%s used a fallback path" % node["label"])
        say("PASS 3-hop delivery both ways, cut frames dropped=%d, plaintext=0 in %.0fs"
            % (hub.dropped, time.monotonic() - t0))
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
        print("MOHHDY P2P multi-hop four-guest contract failed: %s" % error)
        sys.exit(1)
