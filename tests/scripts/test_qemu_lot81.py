#!/usr/bin/env python3
"""Lot 81: inline-grid, masonry, shrink-to-fit floats, aspect-ratio, calc() gaps/tracks in QEMU offline.

- web-open /web/lot81.html
- #i1/#i2: two inline-grid boxes side by side on one line
- #m1..#m3: masonry rows pack the third item under the shorter column
- #fl: a float shrinks to its 40px content; #ar: height 20px aspect-ratio 1.5 gives 30px
- #g1/#g2: column-gap calc(10% + 4px) = 24px; #t1/#t2: calc(50% - 10px) 1fr = 90/110
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot81-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot81-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot81-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot81-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402

ROOT = g.ROOT


def say(msg):
    print("[lot81] %s" % msg, flush=True)


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
    hp = int(os.environ.get("LOT81_PORT", "18081"))
    srv_log = open(os.path.join(g.LOG_DIR, "lot81-web-server.log"), "w")
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
                "-cpu", os.environ.get("LOT81_CPU", "max"),
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

            lw.run(mon, proc, "web-open /web/lot81.html", "osui web", 60, fail=["error="])

            def box(ident, want):
                dump = lw.run(mon, proc, "web-inspect /web/lot81.html %s" % ident, "web-inspect", 30)
                say("inspect %s: %r" % (ident, dump[-400:]))
                m = re.search(r"box (\d+) (\d+) (\d+)x(\d+)", dump)
                if not m:
                    raise RuntimeError("lot81 #%s missing box: %r" % (ident, dump[-300:]))
                got = tuple(int(v) for v in m.groups())
                for k, v in enumerate(want):
                    if v is not None and got[k] != v:
                        raise RuntimeError("lot81 #%s box %r want %r" % (ident, got, want))
                return got

            i1 = box("i1", (8, None, 30, None))
            box("i2", (38, i1[1], 30, None))
            m1 = box("m1", (8, None, 50, 20))
            box("m2", (58, m1[1], 50, 10))
            box("m3", (58, m1[1] + 10, 50, 10))
            box("fl", (8, None, 40, 10))
            box("ar", (8, None, 30, 20))
            g1 = box("g1", (8, None, 50, 10))
            box("g2", (82, g1[1], 50, 10))
            t1 = box("t1", (8, None, 90, 10))
            box("t2", (98, t1[1], 110, 10))
            say("lot81 qemu ok in %.1fs" % (time.monotonic() - t0))
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
        print("[lot81] FAIL: %s" % e, flush=True)
        sys.exit(1)
