#!/usr/bin/env python3
"""Lot 78: place-* / grid-template-rows / grid-row span in QEMU offline.

- web-open /web/lot78.html
- web-inspect #g: display grid, box 98x68
- web-inspect #a: span 2x2 cell area (64x44 with gap)
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot78-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot78-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot78-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot78-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402

ROOT = g.ROOT


def say(msg):
    print("[lot78] %s" % msg, flush=True)


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
    hp = int(os.environ.get("LOT78_PORT", "18078"))
    srv_log = open(os.path.join(g.LOG_DIR, "lot78-web-server.log"), "w")
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
                "-cpu", os.environ.get("LOT78_CPU", "max"),
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

            lw.run(mon, proc, "web-open /web/lot78.html", "osui web", 60, fail=["error="])
            dump_g = lw.run(mon, proc, "web-inspect /web/lot78.html g", "web-inspect", 30)
            say("inspect g: %r" % dump_g[-400:])
            if "display grid" not in dump_g:
                raise RuntimeError("lot78 #g want display grid: %r" % dump_g[-300:])
            if "box 8 34 98x68" not in dump_g and "98x68" not in dump_g:
                # y may vary with h1; require width x height
                if not re.search(r"box \d+ \d+ 98x68", dump_g):
                    raise RuntimeError("lot78 #g want box 98x68: %r" % dump_g[-300:])
            dump_a = lw.run(mon, proc, "web-inspect /web/lot78.html a", "web-inspect", 30)
            say("inspect a: %r" % dump_a[-400:])
            # span 2 cols + gap + 2 rows: 30+4+30=64 wide, 20+4+20=44 tall
            if not re.search(r"box \d+ \d+ 64x44", dump_a):
                raise RuntimeError("lot78 #a want box 64x44: %r" % dump_a[-300:])
            dump_e = lw.run(mon, proc, "web-inspect /web/lot78.html e", "web-inspect", 30)
            say("inspect e: %r" % dump_e[-400:])
            if not re.search(r"box \d+ \d+ 64x20", dump_e):
                raise RuntimeError("lot78 #e want box 64x20: %r" % dump_e[-300:])
            say("lot78 qemu ok in %.1fs" % (time.monotonic() - t0))
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
        print("[lot78] FAIL: %s" % e, flush=True)
        sys.exit(1)
