#!/usr/bin/env python3
"""Lot 48 phase 5 on three QEMU guests (US-070, US-068/073, US-075, US-069).

Same shared Ethernet hub as qemu-p2p (wire cuts between guest MACs), plus
two impairments the hub applies to frames between two guests: a fixed
delay and a full drop for a time. Scenarios, all on the real Ring 3 stack:
  1. US-070: gamma is cut long enough to be declared down and misses an
     invalidation; on reconnect alpha pushes the fence, gamma logs it and
     refuses a stale fill at the old version.
  2. US-068/073: a queueing delay on beta -> alpha makes alpha's adaptive
     window back off; a burst to beta is then shaped (queued, none
     dropped, all delivered) with no manual p2p-limit; a 4 s drop counts a
     ping loss; once healed the window grows back to 16.
  3. US-075: beta and gamma flood alpha; alpha names both by itself (one
     "p2p traffic alert" line each, no p2p-analyze command), beta and
     gamma report nothing, then alpha reports the end of each episode.
  4. US-069 continuous: three crash/reboot cycles (beta, gamma, beta); in
     each, a key put during the crash reaches the rebooted node.
"""
from __future__ import print_function

import os
import re
import sys
import threading
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_p2p as p3  # noqa: E402

base = p3.base
p3.SECRETS = (b"shaped-five", b"flood")
NETKEY = p3.NETKEY


def inst(label, mac, name, ip):
    node = p3.inst(label, mac, name, ip)
    for key in ("log", "err", "mon"):
        node[key] = node[key].replace("p2p-", "p2p-phase5-")
    return node


NODES = [
    inst("a", "52:54:00:a0:52:0a", "alpha", "10.77.0.1"),
    inst("b", "52:54:00:a0:52:0b", "beta", "10.77.0.2"),
    inst("c", "52:54:00:a0:52:0c", "gamma", "10.77.0.3"),
]


class ImpairHub(p3.P2pHub):
    """P2pHub plus per-direction delay and drop (src MAC -> dst MAC)."""

    def __init__(self):
        p3.P2pHub.__init__(self)
        self.delay = {}
        self.drop = set()
        self.dropped = 0
        self.delayed = 0

    def _flood(self, source, frame):
        src = None
        if len(frame) >= 12:
            src = ":".join("%02x" % b for b in bytearray(frame[6:12]))
            self._mac_of[id(source)] = src
        with self._lock:
            clients = list(self._clients)
        for connection in clients:
            if connection is source:
                continue
            dst = self._mac_of.get(id(connection))
            if src and dst and (src, dst) in self.cut:
                continue
            if src and dst and (src, dst) in self.drop:
                self.dropped += 1
                continue
            wait = self.delay.get((src, dst), 0) if src and dst else 0
            if wait:
                self.delayed += 1
                timer = threading.Timer(wait, self._send_late, (connection, frame))
                timer.daemon = True
                timer.start()
                continue
            try:
                self._send_frame(connection, frame)
            except OSError:
                pass

    def _send_late(self, connection, frame):
        try:
            self._send_frame(connection, frame)
        except OSError:
            pass


def say(msg):
    base.say("[p2p-phase5] " + msg)


def qos(node):
    start = p3.cmd(node, "p2p-qos", "p2p qos window ")
    text = p3.log(node, start)
    match = re.search(r"p2p qos window (\d+) .* losses (\d+) ping_losses (\d+) delay_backoffs (\d+) .*"
                      r"queued (\d+) shaped_sent (\d+) shaped_dropped (\d+)", text)
    if not match:
        raise RuntimeError("%s: bad p2p-qos line: %s" % (node["label"], text[-400:]))
    keys = ("window", "losses", "ping_losses", "delay_backoffs", "queued", "shaped_sent", "shaped_dropped")
    return dict(zip(keys, (int(v) for v in match.groups())))


def boot(node, alive):
    base.wait_for(node["proc"], node["log"], "[0m : ", base.BOOT_TIMEOUT)
    node["client"] = base.monitor_connect(node["mon"])
    base.spawn_networker(node, alive)
    base.run_command(node, "p2p-up %s %s %s" % (node["name"], node["ip"], NETKEY),
                     "p2p-up ok name %s" % node["name"], alive, timeout=120)


def crash(node, crashed):
    crashed.add(node["label"])
    node["client"].close()
    node["client"] = None
    base.terminate(node["proc"])
    if node.get("err_handle"):
        node["err_handle"].close()


def main():
    if not os.path.isfile(base.KERNEL) or not os.path.isfile(base.INITRD):
        raise RuntimeError("missing KERNEL or INITRD (build first)")
    os.makedirs(p3.LOG_DIR, exist_ok=True)
    hub = ImpairHub()
    hub.start()
    t0 = time.monotonic()
    crashed = set()
    try:
        for node in NODES:
            base.start_guest(node, hub.port)
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline and hub.client_count < 3:
            time.sleep(0.1)
        if hub.client_count < 3:
            raise RuntimeError("hub expected 3 QEMU clients, got %d" % hub.client_count)

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
        a, b, c = NODES
        for node in NODES:
            for other in NODES:
                if other is not node:
                    p3.wait_log(node, "p2p peer %s up id " % other["name"], 180)
        say("3 guests discovered and keyed (%.0fs)" % (time.monotonic() - t0))

        # 1. US-070: invalidation missed during a partition, fence pushed on reconnect.
        p3.cmd(b, "p2p-cache putv cfg old 4", "p2p-cache putv ok cfg")
        p3.cmd(c, "p2p-cache putv cfg old 4", "p2p-cache putv ok cfg")
        mark = {n["label"]: len(base.log_text(n["log"])) for n in NODES}
        hub.cut_pair(a["mac"], c["mac"])
        hub.cut_pair(b["mac"], c["mac"])
        p3.wait_log(a, "p2p peer gamma down", 90, mark["a"])
        p3.wait_log(c, "p2p peer alpha down", 90, mark["c"])
        p3.cmd(a, "p2p-cache inv cfg 5", "p2p-cache inv sent peers ")
        # beta (reachable) gets v5; alpha's retries to gamma end while it is cut
        p3.wait_log(b, "p2p cache fence cfg v5 from alpha", 30, mark["b"])
        p3.wait_log(a, "p2p cache inv incomplete cfg acks 1", 60, mark["a"])
        hub.heal_pair(a["mac"], c["mac"])
        hub.heal_pair(b["mac"], c["mac"])
        p3.wait_log(a, "p2p peer gamma up id ", 90, mark["a"])
        p3.wait_log(c, "p2p cache fence cfg v5 from ", 60, mark["c"])
        p3.cmd(c, "p2p-cache putv cfg old 5", "p2p-cache stale refused cfg")
        p3.cmd(c, "p2p-cache putv cfg new 6", "p2p-cache putv ok cfg")
        p3.cmd(b, "p2p-cache putv cfg old 5", "p2p-cache stale refused cfg")
        st = p3.cmd(a, "p2p-cache", "p2p cache hits ")
        p3.wait_log(a, " fence_raised ", 20, st)
        if re.search(r"fence_pushed (\d+)", p3.log(a, st)) is None or " fence_pushed 0 " in p3.log(a, st):
            raise RuntimeError("alpha pushed no fence: %s" % p3.log(a, st)[-400:])
        say("scenario 1 ok (US-070): gamma missed inv v5 while cut, got the fence on reconnect, stale fill refused (%.0fs)"
            % (time.monotonic() - t0))

        # 2. US-068/073: adaptive window under a queueing delay and a drop.
        q0 = qos(a)
        hub.delay[(b["mac"], a["mac"])] = 0.4
        deadline = time.monotonic() + 90
        q1 = qos(a)
        while not (q1["delay_backoffs"] > q0["delay_backoffs"] and q1["window"] < 16):
            if time.monotonic() > deadline:
                raise RuntimeError("alpha window did not back off under delay: %r -> %r" % (q0, q1))
            time.sleep(2)
            q1 = qos(a)
        start_b = len(base.log_text(b["log"]))
        burst_at = p3.cmd(a, "p2p-burst beta 12 shaped-five", "p2p-burst sent ")
        burst = re.search(r"p2p-burst sent (\d+) queued (\d+) refused (\d+)", p3.log(a, burst_at))
        if not burst or int(burst.group(2)) < 1 or int(burst.group(3)) != 0:
            raise RuntimeError("burst not shaped by the adaptive window (no p2p-limit): %s window %d"
                               % (p3.log(a, burst_at)[-300:], q1["window"]))
        hub.delay.clear()
        deadline = time.monotonic() + 90
        while p3.log(b, start_b).count("p2p msg from alpha: shaped-five") < 12:
            if time.monotonic() > deadline:
                raise RuntimeError("beta got %d of 12 shaped messages"
                                   % p3.log(b, start_b).count("p2p msg from alpha: shaped-five"))
            time.sleep(2)
        hub.drop.add((b["mac"], a["mac"]))
        time.sleep(4)
        hub.drop.clear()
        deadline = time.monotonic() + 120
        q2 = qos(a)
        while q2["window"] < 16:
            if time.monotonic() > deadline:
                raise RuntimeError("alpha window did not grow back: %r" % q2)
            time.sleep(3)
            q2 = qos(a)
        if q2["ping_losses"] <= q0["ping_losses"] or q2["shaped_dropped"] != q0["shaped_dropped"]:
            raise RuntimeError("drop not seen as a loss or shaped messages dropped: %r -> %r" % (q0, q2))
        say("scenario 2 ok (US-068/073): delay -> window %d (backoffs %d), burst queued %s, 12/12 delivered, "
            "drop -> ping_losses %d, window back to %d (%.0fs)"
            % (q1["window"], q1["delay_backoffs"], burst.group(2), q2["ping_losses"], q2["window"],
               time.monotonic() - t0))

        # 3. US-075: two flooders, automatic per-peer report on alpha only.
        time.sleep(6)   # baselines settle after scenario 2
        mark = {n["label"]: len(base.log_text(n["log"])) for n in NODES}
        p3.cmd(b, "p2p-flood alpha 40", "p2p-flood sent 40 unshaped")
        p3.cmd(c, "p2p-flood alpha 40", "p2p-flood sent 40 unshaped")
        p3.wait_log(a, "p2p traffic alert peer beta rate ", 30, mark["a"])
        p3.wait_log(a, "p2p traffic alert peer gamma rate ", 30, mark["a"])
        p3.wait_log(a, "p2p traffic normal peer gamma", 60, mark["a"])
        for node in (b, c):
            if "p2p traffic alert" in p3.log(node, mark[node["label"]]):
                raise RuntimeError("%s reported an alert without a flood" % node["label"])
        an = p3.cmd(a, "p2p-analyze", "p2p analyze ok findings ")
        alerts = re.search(r"p2p analyze alerts (\d+)", p3.log(a, an))
        if not alerts or int(alerts.group(1)) < 2:
            raise RuntimeError("alpha alert count: %s" % p3.log(a, an)[-600:])
        say("scenario 3 ok (US-075): alpha reported beta and gamma by itself (alerts %s) (%.0fs)"
            % (alerts.group(1), time.monotonic() - t0))

        # 4. US-069 continuous: crash/reboot cycles.
        for cycle, node in enumerate((b, c, b), 1):
            others = [n for n in NODES if n is not node]
            cmark = {n["label"]: len(base.log_text(n["log"])) for n in others}
            crash(node, crashed)
            for other in others:
                p3.wait_log(other, "p2p peer %s down" % node["name"], 90, cmark[other["label"]])
            key = "cycle%d" % cycle
            p3.cmd(a, "p2p-put %s v%d" % (key, cycle), "p2p-put ok %s" % key)
            node["log"] = re.sub(r"(-boot\d+)?\.log$", "-boot%d.log" % (cycle + 1), node["log"])
            node["err"] = re.sub(r"(-boot\d+)?\.err$", "-boot%d.err" % (cycle + 1), node["err"])
            base.start_guest(node, hub.port)
            crashed.discard(node["label"])
            boot(node, alive)
            for other in others:
                p3.wait_log(other, "p2p peer %s rejoined id " % node["name"], 120, cmark[other["label"]])
            p3.wait_log(node, "p2p kv synced %s=v%d " % (key, cycle), 90)
            p3.cmd(node, "p2p-get %s" % key, "p2p-get ok local %s=v%d" % (key, cycle))
            say("scenario 4 cycle %d ok (US-069): %s crashed, rebooted, rejoined, got %s (%.0fs)"
                % (cycle, node["name"], key, time.monotonic() - t0))
        st = p3.cmd(a, "p2p-stats", "p2p stats ok tx_bytes ")
        p3.wait_log(a, " rejoins ", 20, st)
        rejoins = re.search(r"\brejoins (\d+)", p3.log(a, st))
        if not rejoins or int(rejoins.group(1)) < 3:
            raise RuntimeError("alpha rejoins: %s" % p3.log(a, st)[-400:])
        if hub.p2p_plaintext:
            raise RuntimeError("plaintext seen on the wire")
        for node in NODES:
            if "[NET] relay aborted" in p3.log(node) or "Ring 0 fallback" in p3.log(node):
                raise RuntimeError("%s used a fallback path" % node["label"])
        say("PASS frames=%d sealed=%d delayed=%d dropped=%d plaintext=0 in %.0fs"
            % (hub.p2p_frames, hub.p2p_sealed, hub.delayed, hub.dropped, time.monotonic() - t0))
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
        print("MOHHDY P2P phase 5 contract failed: %s" % error)
        sys.exit(1)
