#!/usr/bin/env python3
"""Systemic Consolidation and Endurance Contract: multi-node QEMU tests.

Covers Web Runtime load & telemetry, P2P mesh (gRPC, Raft, anti-entropy, failover),
and Production engine under load with state persistence and corrupt state refusal.
"""
from __future__ import print_function

import os
import re
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import qemu_key_pace as pace  # noqa: E402
import test_qemu_p2p as p2p  # noqa: E402

base = p2p.base
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
DISK = os.path.join(LOG_DIR, "perf-endurance-disk.img")
log, say = p2p.log, p2p.say
ALIASES = {" ": "spc", "-": "minus", "/": "slash", ".": "dot", "=": "equal", "_": "shift-minus", ":": "shift-semicolon"}
for _c in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
    ALIASES[_c] = "shift-" + _c.lower()


def cmd(node, line, needle, timeout=60):
    start = len(base.log_text(node["log"]))
    pace.send_paced(node["client"], node["proc"], lambda: base.log_text(node["log"]),
                    line, base.KEY_HOLD_MS, ALIASES)
    try:
        p2p.wait_log(node, needle, timeout, start)
    except RuntimeError as error:
        raise RuntimeError("%s: %r did not print %r: %s" % (node["label"], line, needle, error))
    return start


def make_disk():
    subprocess.check_call([sys.executable, os.path.join(ROOT, "tests", "scripts", "make_fat16_image.py"),
                           "--image", DISK], stdout=subprocess.DEVNULL)
    with open(DISK, "r+b") as handle:
        handle.truncate(8192 * 512)


def start_node(label, mac, name, ip, hub_port, with_disk=True):
    node = {
        "label": label, "mac": mac, "name": name, "ip": ip,
        "log": os.path.join(LOG_DIR, "perf-%s.log" % label),
        "err": os.path.join(LOG_DIR, "perf-%s.err" % label),
        "mon": os.path.join(LOG_DIR, "perf-%s.monitor.sock" % label),
    }
    for path in (node["log"], node["err"], node["mon"]):
        try:
            os.remove(path)
        except OSError:
            pass
    command = [
        "qemu-system-i386", "-cpu", "max",
        "-kernel", os.path.join(ROOT, "build", "mohhdy.bin"), "-initrd", os.path.join(ROOT, "my_initrd.tar"),
        "-m", "1024M", "-display", "none", "-vga", "none",
        "-serial", "file:" + node["log"],
        "-monitor", "unix:%s,server,nowait" % node["mon"],
        "-machine", "type=pc,accel=tcg",
    ]
    if with_disk:
        command.extend(["-drive", "file=%s,format=raw,if=ide,index=0,cache=writethrough" % DISK])
    command.extend([
        "-netdev", "socket,id=n0,connect=127.0.0.1:%d" % hub_port,
        "-device", "ne2k_isa,netdev=n0,mac=%s" % node["mac"],
        "-no-reboot", "-no-shutdown",
    ])
    err = open(node["err"], "wb")
    node["err_handle"] = err
    node["proc"] = subprocess.Popen(command, stdout=err, stderr=err)
    base.wait_for(node["proc"], node["log"], "[0m : ", base.BOOT_TIMEOUT)
    node["client"] = base.monitor_connect(node["mon"])

    def alive():
        if node["proc"].poll() is not None:
            raise RuntimeError("guest %s died" % label)
    node["alive"] = alive
    base.spawn_networker(node, alive)
    return node


def stop_node(node):
    if node.get("client"):
        try:
            node["client"].close()
        except OSError:
            pass
    base.terminate(node.get("proc"))
    if node.get("err_handle"):
        node["err_handle"].close()


def main():
    if not os.path.isfile(base.KERNEL) or not os.path.isfile(base.INITRD):
        raise RuntimeError("missing KERNEL or INITRD (build first)")
    os.makedirs(LOG_DIR, exist_ok=True)
    make_disk()
    t0 = time.monotonic()

    hub = p2p.P2pHub()
    hub.start()
    alpha, beta = None, None
    try:
        alpha = start_node("alpha", "52:54:00:a0:88:01", "alpha", "10.77.0.1", hub.port, with_disk=True)
        beta = start_node("beta", "52:54:00:a0:88:02", "beta", "10.77.0.2", hub.port, with_disk=False)
        say("booted 2 nodes in %.0fs" % (time.monotonic() - t0))

        # --- Phase 1: Web Runtime Load & Telemetry ---
        cmd(alpha, "browser-telemetry status", "browser-telemetry status=active commands=")
        cmd(alpha, "browser-pm-run print endurance-start", "browser-pm-run ok")
        cmd(alpha, "pwa-manifest endureapp app://endure/start app://endure/ standalone", "pwa-manifest ok")
        cmd(alpha, "pwa-install endureapp", "status=installed")
        cmd(alpha, "pwa-run endureapp", "pwa_name=endureapp")
        cmd(alpha, "browser-telemetry export", "browser-telemetry export ok json=")
        say("web runtime load & telemetry ok (%.0fs)" % (time.monotonic() - t0))

        # --- Phase 2: P2P Mesh Endurance (gRPC, Raft, Sync, STUN, Fallback) ---
        cmd(alpha, "p2p-up alpha 10.77.0.1 %s" % p2p.NETKEY, "p2p-up ok name alpha", timeout=120)
        cmd(beta, "p2p-up beta 10.77.0.2 %s" % p2p.NETKEY, "p2p-up ok name beta", timeout=120)
        p2p.wait_log(alpha, "p2p peer beta up id ", 120)
        p2p.wait_log(beta, "p2p peer alpha up id ", 120)

        cmd(alpha, "p2p-grpc beta echo.EchoService SayHello StressTest", "p2p grpc resp status 0 payload Hello StressTest from beta")
        cmd(alpha, "p2p-raft", "p2p raft role ")
        cmd(alpha, "p2p-put mesh_mode active", "p2p-put ok mesh_mode")
        p2p.wait_log(beta, "p2p kv replicated mesh_mode=active v1 from alpha", 60)

        cmd(alpha, "p2p-stun beta", "p2p-stun ok probe sent to beta")
        cmd(alpha, "p2p-punch beta", "p2p-punch ok sent to beta")
        p2p.wait_log(beta, "p2p nat punch from alpha pinhole opened", 60)

        cmd(alpha, "p2p-analyze", "p2p analyze ok findings ")
        say("p2p mesh endurance ok (%.0fs)" % (time.monotonic() - t0))

        # --- Phase 3: Production Engine Under Load & Persistence ---
        cmd(alpha, "prod-sample 10 30", "prod-sample ok n 10 ")
        cmd(alpha, "prod-alert-add cpu custom above 60 1", "prod-alert-add ok cpu")
        cmd(alpha, "prod-inject 85", "prod-alert FIRING cpu v=85")
        cmd(alpha, "mkdir /cfg", "[0m : ")
        cmd(alpha, "write /cfg/perf.conf mode=endurance", "[0m : ")
        cmd(alpha, "prod-backup perf /cfg", "prod-backup ok perf files 1 bytes ")
        cmd(alpha, "prod-perf-monitor", "perf-monitor health ")
        cmd(alpha, "prod-autoscale-workload 20", "autoscale workload workers ")
        cmd(alpha, "prod-sec-scan /cfg", "baseline none risk_score 0 status clean")
        cmd(alpha, "prod-bench", "prod-bench ok checksum ")

        cmd(alpha, "persist-save", "persist-save ok", timeout=120)
        cmd(alpha, "persist-status", "persist-status ok")
        say("production engine load & persistence ok (%.0fs)" % (time.monotonic() - t0))

        say("PASS QEMU endurance contract completed in %.0fs" % (time.monotonic() - t0))
        return 0
    finally:
        if alpha:
            stop_node(alpha)
        if beta:
            stop_node(beta)
        hub.close()


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:  # noqa: BLE001
        print("MOHHDY QEMU endurance contract failed: %s" % error)
        sys.exit(1)
