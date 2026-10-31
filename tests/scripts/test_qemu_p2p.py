#!/usr/bin/env python3
"""Phase 5 P2P (US-061..US-075): three QEMU guests on one shared Ethernet.

Each guest spawns the Ring 3 networker (strict default kernel) and runs a
P2P node (p2p-up). The hub only floods frames between the guests (no DHCP,
DNS, TLS or public internet). Scenarios:
  1. discovery + encrypted message + health/stats (no plaintext on wire);
  2. replication, hub wire cut, then anti-entropy sync, majority consensus;
  2b. Raft pre-vote, leader lease, hub wire cut, InstallSnapshot, index 3;
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
SECRETS = (b"hello-secret-beta", b"via-relay-msg", b"devsyncnote")


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
    """Flood-only hub that records the P2P datagrams it carries.

    cut_pair drops frames between two learned guest MACs in both directions.
    That is a wire cut on the shared segment, not an application p2p-block.
    """

    def __init__(self):
        SharedEthernetHub.__init__(self, respond=False, full_tls=False, proxy_peer_syn_ack=False)
        self.p2p_frames = 0
        self.p2p_plaintext = 0
        self.p2p_sealed = 0
        self.p2p_hello = 0
        self._mac_of = {}
        self.cut = set()

    def cut_pair(self, mac_a, mac_b):
        self.cut.add((mac_a, mac_b))
        self.cut.add((mac_b, mac_a))

    def heal_pair(self, mac_a, mac_b):
        self.cut.discard((mac_a, mac_b))
        self.cut.discard((mac_b, mac_a))

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
            try:
                self._send_frame(connection, frame)
            except OSError:
                pass

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
    aliases = {" ": "spc", ".": "dot", "-": "minus", "/": "slash", "=": "equal"}
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
            base.wait_for(node["proc"], node["log"], "[0m : ", base.BOOT_TIMEOUT)
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

        # Lot 9 (US-083): devsync between two guests over the sealed P2P
        # link. alpha is device A, beta device B; each pulls the other's
        # delta (version vector request, delta reply answered by the idle
        # pump), then a concurrent write converges by last-writer-wins.
        cmd(a, "devsync-set a note devsyncnote", "devsync-set ok A clock 1")
        cmd(a, "devsync-set a mode eco", "devsync-set ok A clock 2")
        cmd(b, "devsync-set b color blue", "devsync-set ok B clock 1")
        start = cmd(b, "devsync-pull b alpha a", "devsync-pull ", timeout=60)
        if "devsync-pull B from alpha:A ok delta_bytes " not in log(b, start) or " applied 2 " not in log(b, start):
            raise RuntimeError("beta pull from alpha: %s" % log(b, start)[-600:])
        start = cmd(a, "devsync-pull a beta b", "devsync-pull ", timeout=60)
        if "devsync-pull A from beta:B ok" not in log(a, start) or " applied 1 " not in log(a, start):
            raise RuntimeError("alpha pull from beta: %s" % log(a, start)[-600:])
        # nothing new: an empty delta
        start = cmd(a, "devsync-pull a beta b", "devsync-pull ", timeout=60)
        if " applied 0 " not in log(a, start):
            raise RuntimeError("alpha second pull not empty: %s" % log(a, start)[-600:])
        cmd(a, "devsync-set a mode fast", "devsync-set ok A")
        cmd(b, "devsync-set b mode slow", "devsync-set ok B")
        cmd(a, "devsync-pull a beta b", "devsync-pull A from beta:B ok", timeout=60)
        cmd(b, "devsync-pull b alpha a", "devsync-pull B from alpha:A ok", timeout=60)
        digests = []
        for node, dev in ((a, "a"), (b, "b")):
            start = cmd(node, "devsync-show %s" % dev, "devsync-show ")
            m = re.search(r"devsync-show [AB] live 3 clock \d+ vv [\d,]+ conflicts \d+ digest ([0-9a-f]{8})(.*)", log(node, start))
            if not m or "note=devsyncnote" not in m.group(2) or "color=blue" not in m.group(2):
                raise RuntimeError("%s devsync state: %s" % (node["label"], log(node, start)[-600:]))
            digests.append((m.group(1), "mode=fast" in m.group(2), "mode=slow" in m.group(2)))
        if digests[0] != digests[1] or digests[0][1] == digests[0][2]:
            raise RuntimeError("devsync did not converge: %r" % digests)
        if hub.p2p_plaintext:
            raise RuntimeError("devsync plaintext seen on the wire")
        say("scenario 1b ok: devsync A<->B over sealed p2p, digest %s (%.0fs)" % (digests[0][0], time.monotonic() - t0))

        # Lot 6 (US-068/073): shaping. With 2 data messages per second
        # allowed, a burst of 6 is queued, not dropped, and drained by the
        # pump; beta gets all six. Measured byte rates are non zero.
        cmd(a, "p2p-limit 2", "p2p-limit ok per_second 2")
        start_burst = len(base.log_text(b["log"]))
        start_q = cmd(a, "p2p-burst beta 6 shaped-msg", "p2p-burst sent ")
        burst = re.search(r"p2p-burst sent (\d+) queued (\d+) refused (\d+)", log(a, start_q))
        if not burst or int(burst.group(2)) < 1 or int(burst.group(3)) != 0:
            raise RuntimeError("alpha burst was not shaped: %s" % log(a, start_q)[-400:])
        for _round in range(4):
            cmd(a, "p2p-poll 2", "p2p-poll ok")
            cmd(b, "p2p-poll 2", "p2p-poll ok")
        deadline = time.monotonic() + 60
        while log(b, start_burst).count("p2p msg from alpha: shaped-msg") < 6:
            if time.monotonic() > deadline:
                raise RuntimeError("beta got %d of 6 shaped messages" % log(b, start_burst).count("p2p msg from alpha: shaped-msg"))
            cmd(a, "p2p-poll 2", "p2p-poll ok")
            cmd(b, "p2p-poll 2", "p2p-poll ok")
        cmd(a, "p2p-limit 0", "p2p-limit ok per_second 0")
        start_q = cmd(a, "p2p-qos", "p2p qos window ")
        shaped = re.search(r"meas_tx_bps (\d+) meas_rx_bps (\d+) queued (\d+) shaped_sent (\d+) shaped_dropped (\d+)", log(a, start_q))
        if not shaped or int(shaped.group(1)) == 0 or int(shaped.group(4)) < int(burst.group(2)) or int(shaped.group(5)) != 0:
            raise RuntimeError("alpha qos shaping counters: %s" % log(a, start_q)[-400:])
        say("scenario 1b ok: burst of 6 shaped (queued %s), all delivered, meas_tx_bps %s" % (burst.group(2), shaped.group(1)))
        # Lot 67 (US-068/US-073): a paced probe train of 24 to beta feeds the
        # bandwidth estimator (delivery rate of acked probes, min RTT).
        cmd(a, "p2p-probe beta 24", "p2p-probe started to beta count 24")
        deadline = time.monotonic() + 90
        while True:
            cmd(a, "p2p-poll 4", "p2p-poll ok")
            cmd(b, "p2p-poll 4", "p2p-poll ok")
            start_q = cmd(a, "p2p-bwe", "p2p bwe state ")
            bwe = re.search(r"p2p bwe state (\w+) btlbw_bps (\d+) min_rtt_ms (\d+) bdp (\d+) pacing_bps (\d+) cwnd (\d+) "
                            r"samples (\d+) probes_sent (\d+) acked (\d+) lost (\d+) left (\d+)", log(a, start_q))
            if bwe and int(bwe.group(11)) == 0 and int(bwe.group(9)) + int(bwe.group(10)) >= 24:
                break
            if time.monotonic() > deadline:
                raise RuntimeError("probe train did not finish: %s" % log(a, start_q)[-400:])
        if int(bwe.group(9)) < 20 or int(bwe.group(2)) == 0 or int(bwe.group(3)) == 0:
            raise RuntimeError("bandwidth estimator: %s" % bwe.group(0))
        say("scenario 1b ok: bwe %s" % bwe.group(0))
        # Lot 6 (US-075): a flood from gamma is attributed to gamma by
        # beta's per-peer baseline. Gamma plays a misbehaving sender that
        # ignores its own QoS controller (a well-behaved burst is shaped
        # by the adaptive window, as scenario 1b shows).
        start_f = cmd(c, "p2p-flood beta 40", "p2p-flood sent ")
        if "p2p-flood sent 40 unshaped" not in log(c, start_f):
            raise RuntimeError("gamma flood not sent: %s" % log(c, start_f)[-400:])
        for _round in range(3):
            cmd(b, "p2p-poll 3", "p2p-poll ok")
        start_an = cmd(b, "p2p-analyze", "p2p analyze ok findings ")
        if "p2p analyze finding gamma rate-anomaly " not in log(b, start_an) or " peer gamma " not in log(b, start_an):
            raise RuntimeError("beta did not attribute the flood to gamma: %s" % log(b, start_an)[-800:])
        say("scenario 1c ok: flood attributed to gamma")

        # Scenario 2: replication, then a real hub wire cut (not p2p-block).
        cmd(a, "p2p-put color blue", "p2p-put ok color sent 2")
        wait_log(b, "p2p kv replicated color=blue v1 from alpha", 60)
        wait_log(c, "p2p kv replicated color=blue v1 from alpha", 60)
        hub.cut_pair(a["mac"], c["mac"])
        hub.cut_pair(b["mac"], c["mac"])
        start_c = len(base.log_text(c["log"]))
        cmd(a, "p2p-put mode eco", "p2p-put ok mode")
        wait_log(b, "p2p kv replicated mode=eco v1 from alpha", 60)
        if "mode=eco" in log(c, start_c):
            raise RuntimeError("gamma received mode=eco during the hub cut")
        hub.heal_pair(a["mac"], c["mac"])
        hub.heal_pair(b["mac"], c["mac"])
        cmd(c, "p2p-poll 5", "p2p-poll ok", timeout=30)
        cmd(c, "p2p-sync alpha", "p2p-sync ok asked 1")
        wait_log(c, "p2p kv synced mode=eco v1 from alpha", 60, start_c)
        wait_log(c, "p2p sync from alpha items ", 60, start_c)
        cmd(c, "p2p-kv", "p2p kv ok 2")
        cmd(a, "p2p-get nokey", "p2p-get pending asked 2")
        wait_log(a, "p2p kv miss nokey at beta", 60)
        wait_log(a, "p2p kv miss nokey at gamma", 60)
        cmd(c, "p2p-get color", "p2p-get ok local color=blue v1")
        say("scenario 2: replication + hub wire cut sync ok (%.0fs)" % (time.monotonic() - t0))
        cmd(b, "p2p-propose leader beta", "p2p-propose ok leader sent 2")
        wait_log(b, "p2p propose committed leader=beta", 90)
        wait_log(a, "p2p kv committed leader=beta", 60)
        wait_log(c, "p2p kv committed leader=beta", 60)
        if "p2p vote yes" not in log(a) or "p2p vote yes" not in log(c):
            raise RuntimeError("missing votes")
        say("scenario 2 ok: majority consensus committed (%.0fs)" % (time.monotonic() - t0))

        # US-064: Raft log on three guests. Pre-vote and a leader lease, then
        # a hub wire cut. The leader compacts the entry gamma missed and
        # gamma installs that snapshot after the wire is healed.
        cmd(a, "p2p-raft elect", "p2p-raft election started")
        if "p2p raft prevote started" not in log(a):
            raise RuntimeError("alpha did not start a pre-vote")
        cmd(a, "p2p-poll 1", "p2p-poll ok")
        cmd(b, "p2p-poll 1", "p2p-poll ok")
        cmd(c, "p2p-poll 1", "p2p-poll ok")
        cmd(a, "p2p-poll 1", "p2p-poll ok")
        cmd(a, "p2p-raft", "p2p raft role leader")
        if "lease 1" not in log(a):
            raise RuntimeError("leader report has no lease: %s" % log(a)[-400:])
        start_b = cmd(b, "p2p-raft", "p2p raft role follower")
        if "lease 1" not in log(b, start_b):
            raise RuntimeError("follower report has no lease: %s" % log(b, start_b)[-400:])
        cmd(a, "p2p-raft-put tone blue", "p2p-raft-put ok index 1")
        cmd(a, "p2p-poll 1", "p2p-poll ok")
        cmd(b, "p2p-poll 1", "p2p-poll ok")
        cmd(c, "p2p-poll 1", "p2p-poll ok")
        wait_log(b, "p2p raft commit index 1 tone=blue", 30)
        wait_log(c, "p2p raft commit index 1 tone=blue", 30)
        hub.cut_pair(a["mac"], c["mac"])
        hub.cut_pair(b["mac"], c["mac"])
        gamma_at_cut = len(base.log_text(c["log"]))
        cmd(a, "p2p-raft-put late yes", "p2p-raft-put ok index 2")
        cmd(a, "p2p-poll 1", "p2p-poll ok")
        cmd(b, "p2p-poll 1", "p2p-poll ok")
        wait_log(b, "p2p raft commit index 2 late=yes", 30)
        if "late=yes" in log(c, gamma_at_cut):
            raise RuntimeError("gamma applied late=yes during the hub cut")
        cmd(a, "p2p-raft-compact", "p2p-raft-compact ok index 2")
        cmd(a, "p2p-raft-put next ok", "p2p-raft-put ok index 3")
        # Lot 46: pre-vote under a real partition. Gamma, cut from both
        # peers, waits out the leader lease and campaigns: its pre-vote gets
        # no majority, so its term does not move, and after the heal the
        # leader keeps its term (no disruptive election).
        term_re = r"p2p raft role (\w+) term (\d+) .*? term_changes (\d+)"
        st_a = cmd(a, "p2p-raft", "p2p raft role leader")
        a_term = re.search(term_re, log(a, st_a)).group(2)
        st_c = cmd(c, "p2p-raft", " term_changes ")
        c_before = re.search(term_re, log(c, st_c)).groups()
        time.sleep(12)
        st_e = cmd(c, "p2p-raft elect", "p2p-raft ")
        if "p2p-raft lease held" in log(c, st_e):
            time.sleep(6)
            cmd(c, "p2p-raft elect", "p2p-raft election started")
        wait_log(c, "p2p raft prevote started", 20, st_e)
        cmd(c, "p2p-poll 2", "p2p-poll ok")
        st_c = cmd(c, "p2p-raft", " term_changes ")
        c_after = re.search(term_re, log(c, st_c)).groups()
        if c_after[0] == "leader" or c_after[1:] != c_before[1:]:
            raise RuntimeError("isolated gamma pre-vote moved its term: %r -> %r" % (c_before, c_after))
        say("scenario 2b: isolated gamma pre-vote, term stays %s (role %s)" % (c_after[1], c_after[0]))
        hub.heal_pair(a["mac"], c["mac"])
        hub.heal_pair(b["mac"], c["mac"])
        # Two rounds: the leader sends the snapshot, the follower answers,
        # then the leader sends index 3. One round leaves that reply queued.
        # The cut now outlasts the lease (pre-vote check above), so the
        # peers may have marked gamma down: poll until it rejoins (8 max).
        healed_at = len(base.log_text(c["log"]))
        for _round in range(8):
            cmd(a, "p2p-poll 2", "p2p-poll ok")
            cmd(b, "p2p-poll 2", "p2p-poll ok")
            cmd(c, "p2p-poll 2", "p2p-poll ok")
            if _round >= 1 and "p2p raft snapshot installed index 2" in log(c, healed_at):
                break
        wait_log(c, "p2p raft snapshot installed index 2", 40)
        wait_log(b, "p2p raft commit index 3 next=ok", 30)
        wait_log(c, "p2p raft commit index 3 next=ok", 30)
        st_a = cmd(a, "p2p-raft", "p2p raft role ")
        got = re.search(term_re, log(a, st_a)).groups()
        if got[0] != "leader" or got[1] != a_term:
            raise RuntimeError("heal after gamma pre-vote disrupted the leader: %r (term was %s)" % (got, a_term))
        # Lot 6 (US-070): cache line written through the Raft log, same
        # value and version on the three guests, older fill refused.
        start_cr = cmd(a, "p2p-cache raft cfg2 green", "p2p-cache raft ok cfg2 index ")
        idx = re.search(r"p2p-cache raft ok cfg2 index (\d+)", log(a, start_cr)).group(1)
        for _round in range(2):
            cmd(a, "p2p-poll 2", "p2p-poll ok")
            cmd(b, "p2p-poll 2", "p2p-poll ok")
            cmd(c, "p2p-poll 2", "p2p-poll ok")
        for node in (a, b, c):
            wait_log(node, "p2p cache raft cfg2=green v%s" % idx, 40)
        cmd(c, "p2p-cache get cfg2", "p2p-cache ok cfg2=green")
        cmd(c, "p2p-cache putv cfg2 old %s" % idx, "p2p-cache stale refused cfg2")
        say("scenario 2b: raft prevote, lease, snapshot, hub cut ok (%.0fs)" % (time.monotonic() - t0))

        # Lot 46 PR D (US-064): the leader itself is partitioned (hub wire
        # cut alpha<->beta and alpha<->gamma). After its lease expires the
        # majority side (beta, gamma) elects beta with a higher term and
        # commits a new entry; the isolated old leader cannot commit its own
        # entry (no quorum). After the heal alpha steps down to follower at
        # the new term, its uncommitted entry is replaced and it applies the
        # majority's entry.
        hub.cut_pair(a["mac"], b["mac"])
        hub.cut_pair(a["mac"], c["mac"])
        alpha_at_cut = len(base.log_text(a["log"]))
        at_cut = dict((n["label"], len(base.log_text(n["log"]))) for n in (a, b, c))
        start_s = cmd(a, "p2p-raft-put stale alpha", "p2p-raft-put ")
        stale_isolated = "p2p-raft-put ok index" in log(a, start_s)
        cmd(a, "p2p-poll 1", "p2p-poll ok")
        time.sleep(12.0)   # P2P_RAFT_LEASE_TICKS is 10 s
        # Lot 46: beta or gamma may win (gamma campaigns too once its own
        # pre-vote timer ran, see 2b); any majority leader is valid Raft.
        elected = None
        for _try in range(6):
            cmd(b, "p2p-raft elect", "p2p-raft ")
            for _round in range(2):
                cmd(b, "p2p-poll 1", "p2p-poll ok")
                cmd(c, "p2p-poll 1", "p2p-poll ok")
            for node in (b, c):
                start_r = cmd(node, "p2p-raft", "p2p raft role ")
                if "p2p raft role leader" in log(node, start_r):
                    elected = node
                    break
            if elected:
                break
            time.sleep(2.0)
        if not elected:
            raise RuntimeError("no leader elected while alpha was partitioned: %s" % log(b)[-600:])
        other = c if elected is b else b
        term_b = int(re.search(r"p2p raft role leader term (\d+)", log(elected, start_r)).group(1))
        start_p = cmd(elected, "p2p-raft-put split beta", "p2p-raft-put ok index ")
        for _round in range(2):
            cmd(elected, "p2p-poll 1", "p2p-poll ok")
            cmd(other, "p2p-poll 1", "p2p-poll ok")
        wait_log(other, "split=beta", 30)
        # Lot 66 (US-069): a plain kv write on the majority side while alpha
        # is cut; it is not a Raft entry, so only the anti-entropy started
        # by the heal (no p2p-sync typed) can bring it to alpha.
        cmd(b, "p2p-put hz side", "p2p-put ok hz")
        cmd(a, "p2p-poll 1", "p2p-poll ok")
        if "split=beta" in log(a, alpha_at_cut) or "commit index" in log(a, alpha_at_cut):
            raise RuntimeError("isolated alpha committed during the partition: %s" % log(a, alpha_at_cut)[-600:])
        hub.heal_pair(a["mac"], b["mac"])
        hub.heal_pair(a["mac"], c["mac"])
        for _round in range(3):
            cmd(b, "p2p-poll 2", "p2p-poll ok")
            cmd(a, "p2p-poll 2", "p2p-poll ok")
            cmd(c, "p2p-poll 2", "p2p-poll ok")
        wait_log(a, "split=beta", 40)
        for _round in range(6):
            if "hz=side" in log(a, alpha_at_cut):
                break
            cmd(b, "p2p-poll 2", "p2p-poll ok")
            cmd(a, "p2p-poll 2", "p2p-poll ok")
        if "hz=side" not in log(a, alpha_at_cut):
            st = cmd(a, "p2p-kv", "p2p kv ok ")
            raise RuntimeError("alpha lacks hz=side after the heal: %s" % log(a, st)[-1200:])
        wait_log(a, "p2p kv synced hz=side v1 from ", 20, alpha_at_cut)
        for node in (a, b, c):
            if "p2p-sync " in log(node, at_cut[node["label"]]):
                raise RuntimeError("a manual p2p-sync ran during 2c")
        heals = []
        for node in (a, b):
            st = cmd(node, "p2p-stats", " heal_syncs ")
            heals.append(int(re.search(r" heal_syncs (\d+)", log(node, st)).group(1)))
        if max(heals) < 1:
            raise RuntimeError("no heal anti-entropy counted: %r" % heals)
        say("scenario 2c: heal anti-entropy brought hz=side to alpha without p2p-sync (heal_syncs %r)" % heals)
        start_a = cmd(a, "p2p-raft", "p2p raft role ")
        m = re.search(r"p2p raft role (\w+) term (\d+)", log(a, start_a))
        if not m or m.group(1) != "follower" or int(m.group(2)) != term_b:
            raise RuntimeError("old leader did not step down to the new term %d: %r" % (term_b, m.groups() if m else None))
        for node in (a, b, c):
            if "stale=alpha" in log(node, at_cut[node["label"]]):
                raise RuntimeError("%s applied the uncommitted entry of the isolated leader" % node["label"])
        say("scenario 2c: leader partitioned, " + elected["name"] + " elected term %d, old leader stepped down after heal%s (%.0fs)" %
            (term_b, "" if stale_isolated else " (its put was refused)", time.monotonic() - t0))

        # Scenario 3: link alpha<->gamma fails; detection, then relay via beta.
        qos_re = r"p2p qos window (\d+) srtt_ms (\d+) min_rtt_ms (\d+) losses (\d+) ping_losses (\d+)"
        start_q = cmd(a, "p2p-qos", "p2p qos window ")
        ping_losses0 = int(re.search(qos_re, log(a, start_q)).group(5))
        block_at = len(base.log_text(a["log"]))
        cmd(a, "p2p-block gamma", "p2p-block ok gamma")
        cmd(c, "p2p-block alpha", "p2p-block ok alpha")
        # US-068/073: the first unanswered PING to gamma after the block
        # halves the window; beta PONGs grow it back by one per round trip.
        # Lot 66: read the window right after that loss (ping_losses moved),
        # not whatever an earlier scenario left (the heal anti-entropy of 2c
        # gives the window time to regrow to 16 before this block).
        qos = None
        for _round in range(15):
            cmd(a, "p2p-poll 1", "p2p-poll ok")
            start_q = cmd(a, "p2p-qos", "p2p qos window ")
            qos = re.search(qos_re, log(a, start_q))
            if qos and int(qos.group(5)) > ping_losses0:
                break
        if not qos or int(qos.group(5)) <= ping_losses0 or int(qos.group(1)) >= 16 or int(qos.group(3)) < 1:
            raise RuntimeError("alpha qos did not adapt to the ping loss: %s" % log(a, start_q)[-400:])
        wait_log(a, "p2p peer gamma down", 90, block_at)
        say("scenario 3: adaptive window %s after ping loss, srtt %s ms" % (qos.group(1), qos.group(2)))
        cmd(a, "p2p-send gamma via-relay-msg", "p2p-send ok to gamma")
        wait_log(c, "p2p msg from alpha: via-relay-msg (relayed)", 60)
        start_b = cmd(b, "p2p-stats", "p2p stats ok tx_bytes ")
        relayed = re.search(r"relayed (\d+)", log(b, start_b))
        if not relayed or int(relayed.group(1)) < 1:
            raise RuntimeError("beta relayed nothing: %s" % log(b, start_b)[-400:])
        start_h = cmd(a, "p2p-health", "p2p health ok 2 up 1")
        health = log(a, start_h)
        # The hub cut in scenario 2b can mark gamma down before this block,
        # so down_events is at least 1 and may be higher. The blocked line
        # is the scenario 3 proof.
        down = re.search(r"p2p health gamma down blocked\b[^\n]*\bdown_events ([1-9][0-9]*)", health)
        if not down:
            raise RuntimeError("alpha health lacks the failed link: %s" % health[-800:])
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

        # Scenario 4 (US-070): versioned invalidation acked by both peers;
        # gamma is blocked from alpha, so its INV and ACK go through beta.
        cmd(b, "p2p-cache putv cfg old 4", "p2p-cache putv ok cfg")
        cmd(c, "p2p-cache putv cfg old 4", "p2p-cache putv ok cfg")
        cmd(a, "p2p-cache inv cfg 5", "p2p-cache inv sent peers 2")
        wait_log(b, "p2p cache invalidated cfg", 60)
        wait_log(c, "p2p cache invalidated cfg", 60)
        wait_log(a, "p2p cache inv acked cfg v5 acks 2", 60)
        cmd(b, "p2p-cache putv cfg old 5", "p2p-cache stale refused cfg")
        cmd(c, "p2p-cache get cfg", "p2p-cache miss cfg")
        cmd(b, "p2p-cache", "stale_refused 1")
        say("scenario 4 ok: acked versioned cache invalidation (%.0fs)" % (time.monotonic() - t0))
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
