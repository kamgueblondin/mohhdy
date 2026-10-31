#!/usr/bin/env python3
"""Lot 65 (US-G21): a live P2P node driven from the /mesh desktop panel.

One guest with an NE2000 (QEMU user network), taken by the Ring 3
networker started at boot. From the GUI only: /mesh brings the node up (state change,
armed then confirmed), the read-only reports answer from the live node
(p2p-stats, p2p-peers, p2p-health, p2p-qos, p2p-kv), a replicated key is
written and read back; /keys joins the collab ledger and posts a task
and an auction on the live node; /mesh brings the node down. The shell is the
same task as on the console, so rights are the kernel's.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_gui_parity as gp  # noqa: E402

g = gp.g


def main():
    os.makedirs(g.LOG_DIR, exist_ok=True)
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    g.prepare_test_disk()
    proc = monitor = None
    try:
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", g.KERNEL,
                "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-serial", "file:" + g.LOG, "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
                "-netdev", "user,id=n0,net=10.77.0.0/24", "-device", "ne2k_isa,netdev=n0",
            ] + g.qemu_disk_args(), cwd=g.ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            monitor = g.monitor_connect()
            time.sleep(0.6)
            g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            # the boot starts the Ring 3 networker, which takes the NE2000
            g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
            rows = [
                ("p2p-up", "solo 10.77.0.9 mohhdy-lab-key", "p2p-up ok name solo", True),
                ("p2p-stats", "", "p2p stats ok tx_bytes ", False),
                ("p2p-peers", "", "p2p peers ok 0 up 0", False),
                ("p2p-health", "", "p2p health ok", False),
                ("p2p-qos", "", "p2p qos window 16 ", False),
                ("p2p-put", "colour blue", "p2p-put ok colour", True),
                ("p2p-get", "colour", "p2p-get ok local colour=blue", False),
                ("p2p-kv", "", "p2p kv ok 1", False),
                ("p2p-down", "", "p2p-down ok", True),
                ("p2p-stats", "", "p2p-stats error p2p down", False),
            ]
            for cmd, arg, frag, mut in rows[:8]:
                gp.run_entry(monitor, proc, "/mesh", "Maillage P2P", cmd, arg, frag, mut)
            # US-G24: the collab family on the live node, from /keys
            keys = [
                ("collab-join", "", "collab-join ok seq ", True),
                ("collab-keys", "", "signing on", False),
                ("collab-task", "5 sum 10", "collab-task ok seq 2", True),
                ("collab-work", "solo 2", "collab-work computed sum 10 = 55", True),
                ("collab-auction", "lamp 7", "collab-auction ok seq ", True),
                ("collab-auctions", "", "'lamp' reserve 7 best 0", False),
                ("collab-export", "", "collab export join seq 1 'solo'", False),
            ]
            for cmd, arg, frag, mut in keys:
                gp.run_entry(monitor, proc, "/keys", "Cles collab", cmd, arg, frag, mut)
            # Lot 66 (US-070): nine lines in the 8-slot cache from /cmd; the
            # least recently used one is evicted, read back from /mesh.
            gp.cmd_steps(monitor, proc, [("p2p-cache put c%d v%d 6000" % (i, i), "p2p-cache put ok c%d" % i)
                                         for i in range(9)])
            for cmd, arg, frag, mut in [
                    ("p2p-cache get", "c0", "p2p-cache miss c0", False),
                    ("p2p-cache get", "c8", "p2p-cache ok c8=v8", False),
                    ("p2p-cache", "", " evictions 1 expired 0 ", False)]:
                gp.run_entry(monitor, proc, "/mesh", "Maillage P2P", cmd, arg, frag, mut)
            for cmd, arg, frag, mut in rows[8:]:
                gp.run_entry(monitor, proc, "/mesh", "Maillage P2P", cmd, arg, frag, mut)
            g.screendump_rgb(monitor, "qemu-gui-mesh-end.ppm")
        gp.say("QEMU /mesh live node contract passed.")
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:  # noqa: BLE001
        print("MOHHDY /mesh live node contract failed: %s" % error)
        sys.exit(1)
