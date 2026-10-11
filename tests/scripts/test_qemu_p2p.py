#!/usr/bin/env python3
"""Phase 5 P2P (US-061..US-075): three QEMU guests on one shared Ethernet.

Each guest spawns the Ring 3 networker (strict default kernel) and runs a
P2P node (p2p-up). The hub only floods frames between the guests (no DHCP,
DNS, TLS or public internet). Scenarios:
  1. discovery + encrypted message + health/stats (no plaintext on wire);
  2. replication, partition then anti-entropy sync, majority consensus;
  3. link failure detection and relayed (routed) delivery.
"""
from __future__ import print_function

import os
import re
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_ne2k_guest_tls_server as base  # noqa: E402
from qemu_shared_ethernet_hub import SharedEthernetHub  # noqa: E402

LOG_DIR = base.LOG_DIR
NETKEY = "mohhdy-lab-key"
SECRETS = (b"hello-secret-beta", b"via-relay-msg")


def inst(label, mac, name, ip):
    return {
        "label": label, "mac": mac, "name": name, "ip": ip,
        "log": os.path.join(LOG_DIR, "p2p-%s.log" % label),
        "err": os.path.join(LOG_DIR, "p2p-%s.err" % label),
        "mon": os.path.join(LOG_DIR, "p2p-%s.monitor.sock" % label),
    }


NODES = [
    inst("a", "52:54:00:a0:50:0a", "alpha", "10.77.0.1"),
    inst("b", "52:54:00:a0:50:0b", "beta", "10.77.0.2"),
    inst("c", "52:54:00:a0:50:0c", "gamma", "10.77.0.3"),
]


class P2pHub(SharedEthernetHub):
    """Flood-only hub that records the P2P datagrams it carries."""

    def __init__(self):
        SharedEthernetHub.__init__(self, respond=False, full_tls=False, proxy_peer_syn_ack=False)
        self.p2p_frames = 0
        self.p2p_plaintext = 0
        self.p2p_sealed = 0
        self.p2p_hello = 0

    def _handle_frame(self, connection, frame):
        if len(frame) >= 42 and frame[12:14] == b"\x08\x00" and frame[23] == 17:
            port = struct.unpack("!H", frame[36:38])[0]
            if port == 7700:
                payload = frame[42:]
                self.p2p_frames += 1
                if payload[:3] == b"MP2" and len(payload) > 4:
                    if payload[4] == 1:
                        self.p2p_hello += 1
                    elif payload[4] == 2:
                        self.p2p_sealed += 1
                for secret in SECRETS:
                    if secret in payload:
                        self.p2p_plaintext += 1
        SharedEthernetHub._handle_frame(self, connection, frame)


def say(msg):
    base.say("[p2p] " + msg)


def log(node, start=0):
    return base.normalized_log(base.log_text(node["log"])[start:])


def wait_log(node, needle, timeout, start=0):
    base.wait_for(node["proc"], node["log"], needle, timeout, start)


def type_line(node, line):
    """p2p active: the shell reads keys with SYS_GETC (no SYS_GETS echo)."""
    aliases = {" ": "spc", ".": "dot", "-": "minus", "/": "slash", "=": "equal",
               "$": "shift-4", ">": "shift-dot", "<": "shift-comma", "'": "apostrophe"}
    for char in line:
        if node["proc"].poll() is not None:
            raise RuntimeError("QEMU %s stopped during keyboard input" % node["label"])
        base.send_key(node["client"], aliases.get(char, char.lower()))
        time.sleep(0.2)
    base.send_key(node["client"], "ret")


def cmd(node, line, needle, timeout=60):
    start = len(base.log_text(node["log"]))
    type_line(node, line)
    try:
        wait_log(node, needle, timeout, start)
    except RuntimeError as error:
        raise RuntimeError("%s: %r did not print %r: %s" % (node["label"], line, needle, error))
    return start


def main():
    if not os.path.isfile(base.KERNEL) or not os.path.isfile(base.INITRD):
        raise RuntimeError("missing KERNEL or INITRD (build first)")
    os.makedirs(LOG_DIR, exist_ok=True)
    hub = P2pHub()
    hub.start()
    t0 = time.monotonic()
    try:
        for node in NODES:
            base.start_guest(node, hub.port)
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline and hub.client_count < 3:
            time.sleep(0.1)
        if hub.client_count < 3:
            raise RuntimeError("hub expected 3 QEMU clients, got %d" % hub.client_count)
        for node in NODES:
            base.wait_for(node["proc"], node["log"], "(-.-)", base.BOOT_TIMEOUT)
            node["client"] = base.monitor_connect(node["mon"])
        say("3 guests booted in %.0fs" % (time.monotonic() - t0))

        def alive():
            for node in NODES:
                if node["proc"].poll() is not None:
                    raise RuntimeError("guest %s died" % node["label"])
            if hub.error is not None:
                raise RuntimeError("hub error: %s" % hub.error)

        for node in NODES:
            base.spawn_networker(node, alive)
        for node in NODES:
            base.run_command(node, "p2p-up %s %s %s" % (node["name"], node["ip"], NETKEY),
                             "p2p-up ok name %s" % node["name"], alive, timeout=120)
        a, b, c = NODES

        # Scenario 1: discovery, encrypted message, health, traffic stats.
        for node in NODES:
            for other in NODES:
                if other is not node:
                    wait_log(node, "p2p peer %s up id " % other["name"], 180)
        for node in NODES:
            if "unkeyed" in log(node):
                raise RuntimeError("%s reported an unkeyed peer" % node["label"])
        say("scenario 1: discovery complete (%.0fs)" % (time.monotonic() - t0))
        cmd(a, "p2p-peers", "p2p peers ok 2 up 2")
        if "p2p peers gamma up ip 10.77.0.3 id " not in log(a):
            raise RuntimeError("alpha peer table lacks gamma: %s" % log(a)[-800:])
        cmd(a, "p2p-send beta hello-secret-beta", "p2p-send ok to beta")
        wait_log(b, "p2p msg from alpha: hello-secret-beta", 60)
        cmd(b, "p2p-poll 4", "p2p-poll ok")
        cmd(a, "p2p-health", "p2p health ok 2 up 2")
        if "p2p health beta up rtt_ms " not in log(a):
            raise RuntimeError("no rtt for beta: %s" % log(a)[-800:])
        cmd(b, "p2p-stats", "p2p stats ok tx_bytes ")
        if "p2p stats msg tx 0 rx 1" not in log(b):
            raise RuntimeError("beta stats lack the message: %s" % log(b)[-800:])
        if hub.p2p_sealed == 0 or hub.p2p_hello == 0 or hub.p2p_plaintext:
            raise RuntimeError("wire check failed: sealed=%d hello=%d plaintext=%d"
                               % (hub.p2p_sealed, hub.p2p_hello, hub.p2p_plaintext))
        say("scenario 1 ok: encrypted message, health, stats (%.0fs)" % (time.monotonic() - t0))

        # Scenario 2: replication, partition + sync, consensus.
        cmd(a, "p2p-put color blue", "p2p-put ok color sent 2")
        wait_log(b, "p2p kv replicated color=blue v1 from alpha", 60)
        wait_log(c, "p2p kv replicated color=blue v1 from alpha", 60)
        cmd(c, "p2p-block alpha", "p2p-block ok alpha")
        cmd(c, "p2p-block beta", "p2p-block ok beta")
        start_c = len(base.log_text(c["log"]))
        cmd(a, "p2p-put mode eco", "p2p-put ok mode")
        wait_log(b, "p2p kv replicated mode=eco v1 from alpha", 60)
        cmd(c, "p2p-unblock alpha", "p2p-unblock ok alpha")
        cmd(c, "p2p-unblock beta", "p2p-unblock ok beta")
        if "mode=eco" in log(c, start_c):
            raise RuntimeError("gamma received mode=eco while partitioned")
        cmd(c, "p2p-poll 5", "p2p-poll ok", timeout=30)
        cmd(c, "p2p-sync alpha", "p2p-sync ok asked 1")
        wait_log(c, "p2p kv synced mode=eco v1 from alpha", 60, start_c)
        wait_log(c, "p2p sync from alpha items ", 60, start_c)
        cmd(c, "p2p-kv", "p2p kv ok 2")
        cmd(a, "p2p-get nokey", "p2p-get pending asked 2")
        wait_log(a, "p2p kv miss nokey at beta", 60)
        wait_log(a, "p2p kv miss nokey at gamma", 60)
        cmd(c, "p2p-get color", "p2p-get ok local color=blue v1")
        say("scenario 2: replication + partition sync ok (%.0fs)" % (time.monotonic() - t0))
        cmd(b, "p2p-propose leader beta", "p2p-propose ok leader sent 2")
        wait_log(b, "p2p propose committed leader=beta", 90)
        wait_log(a, "p2p kv committed leader=beta", 60)
        wait_log(c, "p2p kv committed leader=beta", 60)
        if "p2p vote yes" not in log(a) or "p2p vote yes" not in log(c):
            raise RuntimeError("missing votes")
        say("scenario 2 ok: majority consensus committed (%.0fs)" % (time.monotonic() - t0))

        # Scenario 3: link alpha<->gamma fails; detection, then relay via beta.
        cmd(a, "p2p-block gamma", "p2p-block ok gamma")
        cmd(c, "p2p-block alpha", "p2p-block ok alpha")
        wait_log(a, "p2p peer gamma down", 90)
        cmd(a, "p2p-send gamma via-relay-msg", "p2p-send ok to gamma")
        wait_log(c, "p2p msg from alpha: via-relay-msg (relayed)", 60)
        start_b = cmd(b, "p2p-stats", "p2p stats ok tx_bytes ")
        relayed = re.search(r"relayed (\d+)", log(b, start_b))
        if not relayed or int(relayed.group(1)) < 1:
            raise RuntimeError("beta relayed nothing: %s" % log(b, start_b)[-400:])
        cmd(a, "p2p-health", "p2p health ok 2 up 1")
        if "p2p health gamma down blocked" not in log(a) or "down_events 1" not in log(a):
            raise RuntimeError("alpha health lacks the failed link: %s" % log(a)[-800:])
        if hub.p2p_plaintext:
            raise RuntimeError("plaintext secret seen on the wire")
        for node in NODES:
            if "[NET] relay aborted" in log(node) or "Ring 0 fallback" in log(node):
                raise RuntimeError("%s used a fallback path" % node["label"])
        # US-075: automatic analysis names the dead link and the relay
        start_a = cmd(a, "p2p-analyze", "p2p analyze ok findings ")
        if "p2p analyze finding gamma down-for-s " not in log(a, start_a) or "verdict degraded" not in log(a, start_a):
            raise RuntimeError("alpha analysis missed the failed link: %s" % log(a, start_a)[-600:])
        start_b = cmd(b, "p2p-analyze", "p2p analyze ok findings ")
        if "p2p analyze finding node relayed " not in log(b, start_b):
            raise RuntimeError("beta analysis missed the relay: %s" % log(b, start_b)[-600:])
        say("scenario 3 ok: failure detected, relayed delivery (%.0fs)" % (time.monotonic() - t0))
        for node in NODES:
            if "p2p stall" in log(node):
                raise RuntimeError("%s console loop stalled: %s" % (node["label"], log(node)[log(node).find("p2p stall"):][:120]))
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
                node["err_handle"].close()
        hub.close()


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:  # noqa: BLE001
        print("MOHHDY P2P three-guest contract failed: %s" % error)
        sys.exit(1)
