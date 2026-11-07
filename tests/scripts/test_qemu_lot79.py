#!/usr/bin/env python3
"""Lot 79: dense flow, auto-rows, space-between in QEMU offline.

- web-open /web/lot79.html
- web-inspect #g: display grid
- web-inspect #c: dense hole fill (third column, first row)
- web-inspect #auto: three auto-rows
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot79-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot79-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot79-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot79-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402

ROOT = g.ROOT


def say(msg):
    print("[lot79] %s" % msg, flush=True)


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
    hp = int(os.environ.get("LOT79_PORT", "18079"))
    srv_log = open(os.path.join(g.LOG_DIR, "lot79-web-server.log"), "w")
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
                "-cpu", os.environ.get("LOT79_CPU", "max"),
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

            lw.run(mon, proc, "web-open /web/lot79.html", "osui web", 60, fail=["error="])
            dump_g = lw.run(mon, proc, "web-inspect /web/lot79.html g", "web-inspect", 30)
            say("inspect g: %r" % dump_g[-400:])
            if "display grid" not in dump_g:
                raise RuntimeError("lot79 #g want display grid: %r" % dump_g[-300:])
            dump_c = lw.run(mon, proc, "web-inspect /web/lot79.html c", "web-inspect", 30)
            say("inspect c: %r" % dump_c[-400:])
            if not re.search(r"box \d+ \d+ 36x12", dump_c):
                raise RuntimeError("lot79 #c want 36x12: %r" % dump_c[-300:])
            # dense hole fill: c shares a's row; x is third column (8+36+6+36=86)
            m = re.search(r"box (\d+) (\d+) 36x12", dump_c)
            dump_a = lw.run(mon, proc, "web-inspect /web/lot79.html a", "web-inspect", 30)
            ma = re.search(r"box (\d+) (\d+) ", dump_a)
            if not m or not ma:
                raise RuntimeError("lot79 missing boxes a/c")
            if int(m.group(2)) != int(ma.group(2)):
                raise RuntimeError("lot79 dense: #c y=%s want same as #a y=%s" % (m.group(2), ma.group(2)))
            if int(m.group(1)) < int(ma.group(1)) + 36:
                raise RuntimeError("lot79 dense: #c x=%s should be right of #a" % m.group(1))
            dump_auto = lw.run(mon, proc, "web-inspect /web/lot79.html auto", "web-inspect", 30)
            say("inspect auto: %r" % dump_auto[-400:])
            if "display grid" not in dump_auto:
                raise RuntimeError("lot79 #auto want display grid")
            # 3 * 14 = 42 tall
            if not re.search(r"box \d+ \d+ 60x42", dump_auto):
                raise RuntimeError("lot79 #auto want 60x42: %r" % dump_auto[-300:])
            dump_box = lw.run(mon, proc, "web-inspect /web/lot79.html box", "web-inspect", 30)
            say("inspect box: %r" % dump_box[-400:])
            if not re.search(r"box \d+ \d+ 100x50", dump_box):
                raise RuntimeError("lot79 #box want 100x50: %r" % dump_box[-300:])
            say("lot79 qemu ok in %.1fs" % (time.monotonic() - t0))
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
        print("[lot79] FAIL: %s" % e, flush=True)
        sys.exit(1)
