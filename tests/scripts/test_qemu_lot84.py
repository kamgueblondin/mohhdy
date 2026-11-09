#!/usr/bin/env python3
"""Lot 84: definite-first masonry, packed row height, border percentage math, absolute insets in QEMU offline.

- web-open /web/lot84.html
- #d0..#d3: definite-first next places the explicit columns before the autos
- #cf: grid-auto-flow:column still packs four 10px items into two columns (height 20)
- #bb: border-left min(2%, 6px) on 200px is 4px
- #ia: absolute left min(10%, 30px) and top max(10%, 2px) against the masonry grid
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot84-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot84-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot84-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot84-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402

ROOT = g.ROOT


def say(msg):
    print("[lot84] %s" % msg, flush=True)


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
    hp = int(os.environ.get("LOT84_PORT", "18084"))
    srv_log = open(os.path.join(g.LOG_DIR, "lot84-web-server.log"), "w")
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
                "-cpu", os.environ.get("LOT84_CPU", "max"),
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

            lw.run(mon, proc, "web-open /web/lot84.html", "osui web", 60, fail=["error="])

            def box(ident, want):
                dump = lw.run(mon, proc, "web-inspect /web/lot84.html %s" % ident, "web-inspect", 30)
                say("inspect %s: %r" % (ident, dump[-400:]))
                m = re.search(r"box (\d+) (\d+) (\d+)x(\d+)", dump)
                if not m:
                    raise RuntimeError("lot84 #%s missing box: %r" % (ident, dump[-300:]))
                got = tuple(int(v) for v in m.groups())
                for k, v in enumerate(want):
                    if v is not None and got[k] != v:
                        raise RuntimeError("lot84 #%s box %r want %r" % (ident, got, want))
                return got

            # body margin 8. definite-first: d2 (column 1) and d0 (column 2) first.
            d2 = box("d2", (8, None, 30, 14))
            box("d0", (38, d2[1], 30, 10))
            box("d1", (38, d2[1] + 10, 30, 10))
            box("d3", (68, d2[1], 30, 6))
            box("df", (8, d2[1], 90, 20))
            f0 = box("f0", (8, None, 30, 10))
            box("f1", (38, f0[1], 30, 10))
            box("f2", (8, f0[1] + 10, 30, 10))
            box("f3", (38, f0[1] + 10, 30, 10))
            box("cf", (8, f0[1], 60, 20))
            bb = box("bb", (12, None, 196, 10))
            ig = box("ig", (8, None, 200, 40))
            box("ia", (ig[0] + 20, ig[1] + 4, 8, 8))
            box("ic", (8, ig[1], 100, 10))
            say("lot84 qemu ok in %.1fs (bb y %d)" % (time.monotonic() - t0, bb[1]))
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
        print("[lot84] FAIL: %s" % e, flush=True)
        sys.exit(1)
