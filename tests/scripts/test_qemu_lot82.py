#!/usr/bin/env python3
"""Lot 82: min()/max()/clamp() with percentages, subgrid edge offsets, masonry columns in QEMU offline.

- web-open /web/lot82.html
- #a/#b: grid-template-columns:min(50%, 60px) 1fr on 200px = 60/140
- #i0/#i1: a column subgrid with padding-left:4px: 46px first track, second at the parent's line
- #m0..#m2: grid-template-columns:masonry packs the third item after the shorter row
- #g1/#g2: column-gap:clamp(4px, 10%, 30px) = 20px
"""
from __future__ import print_function

import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot82-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot82-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot82-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot82-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402

ROOT = g.ROOT


def say(msg):
    print("[lot82] %s" % msg, flush=True)


def main():
    lw.ALIASES.setdefault("#", "shift-3")
    if not os.path.isfile(os.path.join(lw.PKI, "test_ca.pem")):
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "lot67_gen_pki.py"), lw.PKI], check=True)
    os.makedirs(g.LOG_DIR, exist_ok=True)
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    hp = int(os.environ.get("LOT82_PORT", "18082"))
    srv_log = open(os.path.join(g.LOG_DIR, "lot82-web-server.log"), "w")
    srv = subprocess.Popen(
        [sys.executable, os.path.join(ROOT, "scripts", "lot67_web_server.py"), lw.PKI, str(hp), str(hp + 1)],
        stdout=srv_log, stderr=subprocess.STDOUT)
    proc = mon = qmp = None
    t0 = time.monotonic()
    try:
        g.prepare_test_disk()
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4",
                "-cpu", os.environ.get("LOT82_CPU", "max"),
                "-kernel", g.KERNEL, "-initrd", g.INITRD, "-m", "1024M",
                "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet",
                "-serial", "file:" + g.LOG,
                "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-qmp", "unix:%s,server,nowait" % g.QMP_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
                "-netdev", "user,id=n0", "-device", "ne2k_isa,netdev=n0",
            ] + g.qemu_disk_args(), cwd=ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            mon = g.monitor_connect()
            qmp = g.qmp_connect()
            g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
            time.sleep(0.5)

            lw.run(mon, proc, "web-open /web/lot82.html", "osui web", 60, fail=["error="])

            def box(ident, want):
                dump = lw.run(mon, proc, "web-inspect /web/lot82.html %s" % ident, "web-inspect", 30)
                say("inspect %s: %r" % (ident, dump[-400:]))
                m = re.search(r"box (\d+) (\d+) (\d+)x(\d+)", dump)
                if not m:
                    raise RuntimeError("lot82 #%s missing box: %r" % (ident, dump[-300:]))
                got = tuple(int(v) for v in m.groups())
                for k, v in enumerate(want):
                    if v is not None and got[k] != v:
                        raise RuntimeError("lot82 #%s box %r want %r" % (ident, got, want))
                return got

            a = box("a", (8, None, 60, 10))
            box("b", (68, a[1], 140, 10))
            i0 = box("i0", (12, None, 46, 10))
            box("i1", (58, i0[1], 50, 10))
            m0 = box("m0", (8, None, 30, None))
            box("m1", (8, m0[1] + 10, 20, None))
            box("m2", (28, m0[1] + 10, 10, None))
            g1 = box("g1", (8, None, 30, 10))
            box("g2", (58, g1[1], 30, 10))
            say("lot82 qemu ok in %.1fs" % (time.monotonic() - t0))
            return 0
    finally:
        if mon:
            try:
                mon.close()
            except Exception:
                pass
        if qmp:
            try:
                qmp.close()
            except Exception:
                pass
        if proc and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except Exception:
                proc.kill()
        srv.terminate()
        try:
            srv.wait(timeout=5)
        except Exception:
            srv.kill()
        srv_log.close()


if __name__ == "__main__":
    try:
        sys.exit(main() or 0)
    except Exception as e:
        print("[lot82] FAIL: %s" % e, flush=True)
        sys.exit(1)
