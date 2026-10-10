#!/usr/bin/env python3
"""Consolidation contract: state survives a cold reboot on the same disk.

Boot 1 (IDE disk with the standard FAT16 fixture, NE2000 on a private hub,
Ring 3 networker and the boot atadriver): P2P node + signed collab ledger,
production metrics, a firing alert, a backup, feedback, and the autoscaler
starting and stopping real worker tasks; persist-save writes everything to
the overlay, flushed to the disk through the Ring 3 atadriver. The guest is
then killed. Boot 2 on the same image: persist-load restores the node seed
(same node id and signing key), the ledger (same audit digest, sequence
continues), metrics, alert state, backup (restorable) and feedback. A
tampered chunk is refused by checksum. No public network.
"""
from __future__ import print_function

import os
import re
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_p2p as p2p  # noqa: E402

base = p2p.base
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
DISK = os.path.join(LOG_DIR, "persistence-disk.img")
cmd, log, say = p2p.cmd, p2p.log, p2p.say


def node_for(boot):
    node = p2p.inst("p%d" % boot, "52:54:00:a0:71:01", "alpha", "10.77.0.1")
    for key in ("log", "err", "mon"):
        node[key] = node[key].replace("p2p-", "persist-")
    return node


def make_disk():
    subprocess.check_call([sys.executable, os.path.join(ROOT, "tests", "scripts", "make_fat16_image.py"),
                           "--image", DISK], stdout=subprocess.DEVNULL)
    with open(DISK, "r+b") as handle:
        handle.truncate(8192 * 512)


def start(node, hub_port):
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
        "-drive", "file=%s,format=raw,if=ide,index=0,cache=writethrough" % DISK,
        "-netdev", "socket,id=n0,connect=127.0.0.1:%d" % hub_port,
        "-device", "ne2k_isa,netdev=n0,mac=%s" % node["mac"],
        "-no-reboot", "-no-shutdown",
    ]
    err = open(node["err"], "wb")
    node["err_handle"] = err
    node["proc"] = subprocess.Popen(command, stdout=err, stderr=err)
    base.wait_for(node["proc"], node["log"], "(-.-)", base.BOOT_TIMEOUT)
    node["client"] = base.monitor_connect(node["mon"])

    def alive():
        if node["proc"].poll() is not None:
            raise RuntimeError("guest died")
    node["alive"] = alive
    base.spawn_networker(node, alive)


def stop(node):
    if node.get("client"):
        try:
            node["client"].close()
        except OSError:
            pass
    base.terminate(node.get("proc"))
    if node.get("err_handle"):
        node["err_handle"].close()


def audit(node):
    begin = cmd(node, "collab-audit", "collab audit entries ")
    match = re.search(r"collab audit entries (\d+) applied (\d+) rejected (\d+) digest ([0-9a-f]{16})", log(node, begin))
    if not match:
        raise RuntimeError("audit line missing")
    return match.groups()


def fingerprint(node):
    begin = cmd(node, "collab-keys", "collab-keys ok known 1 rejected 0 signing on")
    match = re.search(r"collab key alpha fpr ([0-9a-f]{16})", log(node, begin))
    if not match:
        raise RuntimeError("own key not listed under the node name: %s" % log(node, begin)[-300:])
    return match.group(1)


def boot1(hub):
    node = node_for(1)
    start(node, hub.port)
    try:
        cmd(node, "p2p-up alpha 10.77.0.1 %s" % p2p.NETKEY, "p2p-up ok name alpha", timeout=120)
        cmd(node, "collab-join", "collab-join ok seq 1")
        cmd(node, "collab-offer cpu 5 2", "collab-offer ok seq 2")
        cmd(node, "collab-profile set city paris shared", "collab-profile ok seq 3")
        cmd(node, "collab-forget", "collab-forget ok seq 4")
        cmd(node, "persist-passphrase correct-horse-9", "persist-passphrase ok (kept in RAM only)")
        cmd(node, "p2p-put color blue", "p2p-put ok color")
        fpr = fingerprint(node)
        # US-060: PromptMessage certificate signed with this node's key
        cmd(node, "write /t.pm expect 4 == 4", "write ok /t.pm")
        cmd(node, "pm-certify /t.pm", "pm-certify signed /t.pm.sig author " + fpr)
        cmd(node, "pm-verify /t.pm", "pm-verify signature ok author " + fpr)
        cmd(node, "write /u.pm expect 6 == 6", "write ok /u.pm")
        cmd(node, "pm-certify /u.pm", "pm-certify signed /u.pm.sig")
        cmd(node, "cp /u.pm.sig /t.pm.sig", "cp ok ")
        cmd(node, "pm-verify /t.pm", "pm-verify signature bad /t.pm.sig")
        before = audit(node)
        # production state
        cmd(node, "prod-sample 5 10", "prod-sample ok n 5 ")
        cmd(node, "prod-alert-add app custom above 50 1", "prod-alert-add ok app")
        cmd(node, "prod-inject 80", "prod-alert FIRING app v=80")
        cmd(node, "mkdir /cfg", "(-.-)")
        cmd(node, "write /cfg/a.conf mode=eco", "(-.-)")
        cmd(node, "prod-backup daily /cfg", "prod-backup ok daily files 1 bytes ")
        cmd(node, "prod-feedback 5 durable", "prod-feedback ok total 1")
        # autoscaling with real worker tasks
        begin = cmd(node, "prod-scale-run 10 10 10", "prod-scale-run ok workers 3 live_in_ps 3 ups 2 downs 0", timeout=90)
        steps = re.findall(r"prod-scale step queue 10 workers (\d) pids", log(node, begin))
        if steps != ["2", "2", "3"]:
            raise RuntimeError("scale-up steps %r" % steps)
        begin = cmd(node, "prod-scale-run 0 0 0 0", "prod-scale-run ok workers 1 live_in_ps 1 ups 2 downs 2", timeout=90)
        steps = re.findall(r"prod-scale step queue 0 workers (\d) pids", log(node, begin))
        if steps != ["3", "2", "2", "1"]:
            raise RuntimeError("scale-down steps %r" % steps)
        cmd(node, "prod-scale-stop", "prod-scale-stop ok workers 0 live_in_ps 0")
        say("boot 1: ledger %r, scaler 1->3->1->0 real tasks" % (before,))
        begin = cmd(node, "persist-save", "persist-save ok", timeout=120)
        flushes = log(node, begin).count("atadriver snapshot flush ok")
        if flushes < 5:  # one overlay flush per chunk, done by the Ring 3 driver
            raise RuntimeError("persist-save not flushed by atadriver (%d flushes)" % flushes)
        out = log(node)
        for name in ("node 32 bytes", "prod ", "collab ", "p2pkv ", "collab key encrypted aes128-gcm"):
            if "persist-save " + name not in out:
                raise RuntimeError("persist-save missing %s" % name)
        cmd(node, "persist-status", "persist-status ok", timeout=60)
        # automatic periodic save, then the save on a clean shutdown
        cmd(node, "persist-auto 5", "persist-auto ok every 5 s")
        cmd(node, "prod-inject 90", "prod-inject ok custom 90")
        p2p.wait_log(node, "persist autosave ok blobs 1", 60, len(base.log_text(node["log"])) - 4000)
        cmd(node, "persist-auto 3600", "persist-auto ok every 3600 s")  # next change only saved by shutdown
        cmd(node, "prod-feedback 4 second", "prod-feedback ok total 2")
        cmd(node, "shutdown", "persist shutdown save ok blobs 1", timeout=60)
        time.sleep(2)
        return fpr, before
    finally:
        stop(node)


def boot2(hub, fpr, before):
    node = node_for(2)
    start(node, hub.port)
    try:
        if "Overlay FS charge depuis le disque IDE" not in log(node):
            raise RuntimeError("overlay not loaded from disk at boot 2")
        cmd(node, "persist-status", "persist-status ok", timeout=60)
        cmd(node, "persist-load", "persist-load ok", timeout=60)
        if "persist-load collab 4 entries key 0" not in log(node):
            raise RuntimeError("ledger not restored: %s" % log(node)[-600:])
        cmd(node, "p2p-up alpha 10.77.0.1 %s" % p2p.NETKEY, "p2p-up ok name alpha", timeout=120)
        if "persist-load collab key locked (persist-unlock PASSPHRASE)" not in log(node):
            raise RuntimeError("encrypted key was not locked without passphrase")
        if "p2p kv restored items 1" not in log(node):
            raise RuntimeError("p2p kv not restored")
        cmd(node, "p2p-get color", "p2p-get ok local color=blue v1")
        cmd(node, "collab-keys", "collab-keys ok known 1 rejected 0 signing off")
        cmd(node, "persist-save", "persist-save skip collab key locked")
        cmd(node, "persist-unlock wrong-pass-0", "persist-unlock error wrong passphrase")
        cmd(node, "persist-unlock correct-horse-9", "persist-unlock ok signing on")
        if fingerprint(node) != fpr:
            raise RuntimeError("signing key changed across reboot")
        after = audit(node)
        if after != before:
            raise RuntimeError("ledger changed across reboot: %r != %r" % (after, before))
        cmd(node, "collab-offer gpu 3 1", "collab-offer ok seq 5")
        cmd(node, "collab-profile show alpha", "collab profile alpha shared (none)")
        cmd(node, "prod-alerts", ["prod-rule app custom > 50 for 1 state firing"][0])
        cmd(node, "prod-alerts", "prod-alerts ok fired 1 resolved 0")
        cmd(node, "prod-metrics", "prod-metrics ok samples 7")
        cmd(node, "prod-backups", "prod-backup daily dir /cfg files 1 tick ")
        cmd(node, "write /cfg/a.conf broken", "(-.-)")
        cmd(node, "prod-restore daily", "prod-restore ok files 1")
        cmd(node, "cat /cfg/a.conf", "mode=eco")
        cmd(node, "prod-feedback summary", "prod-feedback total 2 avg 4.5")
        # a damaged chunk is refused, nothing is applied from it
        cmd(node, "write /persist/prod.0 garbage", "(-.-)")
        cmd(node, "persist-load", "persist-load error", timeout=60)
        if "persist-load prod corrupt (checksum)" not in log(node):
            raise RuntimeError("corrupt chunk not detected")
        cmd(node, "prod-metrics", "prod-metrics ok samples 7")
        return after
    finally:
        stop(node)


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    make_disk()
    t0 = time.monotonic()
    hub = p2p.P2pHub()
    hub.start()
    try:
        fpr, before = boot1(hub)
        say("boot 1 saved, key %s (%.0fs)" % (fpr, time.monotonic() - t0))
        after = boot2(hub, fpr, before)
        say("PASS persistence across cold reboot: digest %s key %s in %.0fs" % (after[3], fpr, time.monotonic() - t0))
        return 0
    finally:
        hub.close()


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:  # noqa: BLE001
        print("MOHHDY persistence contract failed: %s" % error)
        sys.exit(1)
