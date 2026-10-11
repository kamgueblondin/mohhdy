#!/usr/bin/env python3
"""Fleet services between three QEMU guests over the P2P layer (no internet).

US-084 continuity: a shell session (cwd, variables, recent history) handed
off from alpha to beta. US-083 multi-device sync: a file replicated to all
devices, last writer wins. Phase 8: metrics and alert logs collected on
alpha from beta and gamma; deploy staged canary -> all -> rollback with
per-node acknowledgements and checksum verification.
Every command is typed once: the fleet layer acknowledges each chunk and
re-sends what is lost (bounded). Payloads above one datagram are chunked
(a 390+ byte file is synced and deployed). The guests also report any
console-loop stall over 3 s ("shell stall"); none may appear once P2P is up.
"""
from __future__ import print_function

import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_p2p as p2p  # noqa: E402

base = p2p.base
p2p.SECRETS = (b"release-one", b"proj=mohhdy")
NODES = [
    p2p.inst("a", "52:54:00:a0:80:0a", "alpha", "10.77.0.1"),
    p2p.inst("b", "52:54:00:a0:80:0b", "beta", "10.77.0.2"),
    p2p.inst("c", "52:54:00:a0:80:0c", "gamma", "10.77.0.3"),
]
for node in NODES:
    for key in ("log", "err", "mon"):
        node[key] = node[key].replace("p2p-", "fleet-")
cmd, log, say = p2p.cmd, p2p.log, p2p.say


def remote(node, line, local_needle, targets, timeout=90):
    """Type line ONCE on node and wait for every (target, needle). Delivery
    is the fleet layer's job (chunk acks + bounded re-send): no retyping."""
    starts = dict((id(t), len(base.log_text(t["log"]))) for t, _ in targets)
    cmd(node, line, local_needle)
    for target, needle in targets:
        try:
            p2p.wait_log(target, needle, timeout, starts[id(target)])
        except Exception as error:  # noqa: BLE001
            raise RuntimeError("%r on %s: %r never reached %s: %s"
                               % (line, node["label"], needle, target["label"], error))


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
                    p2p.wait_log(node, "p2p peer %s up id " % other["name"], 180)
        say("p2p mesh up (%.0fs)" % (time.monotonic() - t0))
        # X25519 key agreement with each new peer runs inside the pump and
        # takes ~3 s per peer on a CI runner (TCG): stalls are checked from
        # here on, once every link is keyed.
        mesh_at = dict((n["label"], len(log(n))) for n in NODES)

        # US-084 session handoff alpha -> beta.
        cmd(a, "mkdir /work", "(-.-)")
        cmd(a, "cd /work", "(-.-)")
        cmd(a, "export proj=mohhdy", "(-.-)")
        remote(a, "session-handoff beta", "session-handoff ok to beta cwd /work bytes ",
               [(b, "fleet session offered by alpha bytes ")])
        cmd(b, "session-resume", "session-resume ok from alpha cwd /work vars ")
        cmd(b, "pwd", "/work")
        cmd(b, "env", "=mohhdy")
        cmd(b, "history", "export proj=mohhdy")
        cmd(c, "session-resume", "session-resume error no session offered")
        say("US-084 session handoff ok (%.0fs)" % (time.monotonic() - t0))

        # US-083 multi-device sync, last writer wins.
        cmd(a, "write /notes.txt hello", "(-.-)")
        remote(a, "sync-push /notes.txt", "sync-push ok /notes.txt v",
               [(b, "fleet sync applied /notes.txt v"), (c, "fleet sync applied /notes.txt v")])
        cmd(c, "cat /notes.txt", "hello")
        cmd(c, "write /notes.txt edited-on-gamma", "(-.-)")
        remote(c, "sync-push /notes.txt", "sync-push ok /notes.txt v",
               [(a, "bytes 16 from gamma"), (b, "bytes 16 from gamma")])
        cmd(a, "cat /notes.txt", "edited-on-gamma")
        cmd(b, "sync-status", "sync-status ok files 1")
        line = "line-of-the-big-file-0123456789-abcdefghijklmnopqrstuvwxyz-end"
        cmd(a, "write /big.txt " + line, "(-.-)")
        for _ in range(5):
            cmd(a, "append /big.txt " + line, "(-.-)")
        remote(a, "sync-push /big.txt", "sync-push ok /big.txt v1 bytes 3",
               [(b, "fleet sync applied /big.txt v1 bytes 3"), (c, "fleet sync applied /big.txt v1 bytes 3")])
        cmd(c, "wc /big.txt", "(-.-)")
        cmd(a, "sync-status", "failed 0 pending 0")
        say("US-083 sync ok (%.0fs)" % (time.monotonic() - t0))

        # Phase 8 collector: metrics and alert log from beta and gamma.
        cmd(c, "prod-alert-add hot custom above 50 1", "prod-alert-add ok hot")
        cmd(c, "prod-inject 90", "prod-alert FIRING hot v=90")
        remote(b, "fleet-report alpha", "fleet-report ok mem_used_kb=",
               [(a, "fleet metric from beta: mem_used_kb=")])
        remote(c, "fleet-report alpha", "fleet-report ok mem_used_kb=",
               [(a, "fleet metric from gamma: mem_used_kb="), (a, "fleet log from gamma: FIRING hot")])
        cmd(a, "fleet-collect", "fleet-collect ok nodes 2")
        out = log(a)
        if "fleet node gamma log FIRING hot" not in out or "fleet node beta reports " not in out:
            raise RuntimeError("collector table incomplete: %s" % out[-800:])
        say("phase 8 collector ok (%.0fs)" % (time.monotonic() - t0))

        # Phase 8 staged deploy: canary beta, then all, then rollback.
        cmd(a, "write /r1.txt release-one", "(-.-)")
        cmd(a, "write /r2.txt release-two", "(-.-)")
        cmd(a, "cp /big.txt /r3.txt", "(-.-)")
        remote(a, "deploy-stage web /r1.txt beta", "deploy-stage ok web v",
               [(b, " canary applied from alpha"), (a, "ok from beta")])
        cmd(c, "cat /app/web", "(-.-)")
        if "release-one" in log(c)[-400:]:
            raise RuntimeError("gamma got the canary build")
        remote(a, "deploy-promote web", "deploy-promote ok web v",
               [(c, " all applied from alpha"), (a, "ok from gamma")])
        cmd(c, "cat /app/web", "release-one")
        remote(a, "deploy-stage web /r2.txt gamma", "deploy-stage ok web v",
               [(c, " canary applied from alpha")])
        cmd(c, "cat /app/web", "release-two")
        remote(a, "deploy-rollback web", "deploy-rollback ok web v",
               [(b, " rollback applied from alpha"), (c, " rollback applied from alpha")])
        cmd(c, "cat /app/web", "release-one")
        cmd(b, "cat /app/web", "release-one")
        cmd(a, "deploy-status web", "stage rollback acks ")
        remote(a, "deploy-stage big /r3.txt beta", "deploy-stage ok big v1",
               [(b, "fleet deploy big v1 canary applied")])
        cmd(b, "cat /app/big", "abcdefghijklmnopqrstuvwxyz-end")
        cmd(a, "deploy-promote nothing", "deploy-promote error")
        say("phase 8 staged deploy ok (%.0fs)" % (time.monotonic() - t0))
        for node in NODES:
            text = log(node)
            up_at = mesh_at[node["label"]]
            if "shell stall" in text[up_at:]:
                raise RuntimeError("%s console loop stalled with P2P up: %s"
                                   % (node["label"], text[text.find("shell stall", up_at):][:120]))
            if " down" in "".join(l for l in text.splitlines() if l.startswith("p2p peer ") or "p2p peer " in l):
                raise RuntimeError("%s saw a peer go down" % node["label"])
        if hub.p2p_plaintext:
            raise RuntimeError("plaintext secret seen on the wire")
        say("PASS fleet frames=%d sealed=%d plaintext=0 in %.0fs"
            % (hub.p2p_frames, hub.p2p_sealed, time.monotonic() - t0))
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
        print("[fleet] FAIL: %s" % error)
        sys.exit(1)
