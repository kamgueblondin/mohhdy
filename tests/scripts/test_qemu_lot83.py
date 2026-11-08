#!/usr/bin/env python3
"""Lot 83: margin/padding math with percentages, masonry-auto-flow next, masonry intrinsic width, subgrid auto margins and masonry stretch in QEMU offline.

- web-open /web/lot83.html
- #pb: padding-left:min(10%, 30px) on 200px = 20px (x 28, w 180)
- #ml: margin-left:max(5%, 3px) = 10px (x 18, w 190)
- #n0..#n2: masonry-auto-flow:next places the third item back in row 1 after the 50px item
- #fi: an inline-grid masonry-columns container shrinks to its packed width (70px)
- #j0/#j1: a subgrid with margin-left:auto still fills its area (50/50, no refusal)
- #t1: align-items:stretch makes an auto-height masonry item fill its 20px row
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot83-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot83-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot83-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot83-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402

ROOT = g.ROOT


def say(msg):
    print("[lot83] %s" % msg, flush=True)


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
    hp = int(os.environ.get("LOT83_PORT", "18083"))
    srv_log = open(os.path.join(g.LOG_DIR, "lot83-web-server.log"), "w")
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
                "-cpu", os.environ.get("LOT83_CPU", "max"),
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

            lw.run(mon, proc, "web-open /web/lot83.html", "osui web", 60, fail=["error="])

            def box(ident, want):
                dump = lw.run(mon, proc, "web-inspect /web/lot83.html %s" % ident, "web-inspect", 30)
                say("inspect %s: %r" % (ident, dump[-400:]))
                m = re.search(r"box (\d+) (\d+) (\d+)x(\d+)", dump)
                if not m:
                    raise RuntimeError("lot83 #%s missing box: %r" % (ident, dump[-300:]))
                got = tuple(int(v) for v in m.groups())
                for k, v in enumerate(want):
                    if v is not None and got[k] != v:
                        raise RuntimeError("lot83 #%s box %r want %r" % (ident, got, want))
                return got

            pb = box("pb", (28, None, 180, 10))
            box("ml", (18, pb[1] + 10, 190, 10))
            n0 = box("n0", (8, None, 50, 10))
            box("n1", (8, n0[1] + 10, 5, 10))
            box("n2", (58, n0[1], 5, 10))
            box("fi", (8, None, 70, 20))
            j0 = box("j0", (8, None, 50, 10))
            box("j1", (58, j0[1], 50, 10))
            t0 = box("t0", (8, None, 30, 10))
            box("t1", (8, t0[1] + 10, 20, 20))
            say("lot83 qemu ok in %.1fs" % (time.monotonic() - t0))
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
        print("[lot83] FAIL: %s" % e, flush=True)
        sys.exit(1)
