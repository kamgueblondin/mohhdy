#!/usr/bin/env python3
"""Lot 76: type=color / type=date, autocomplete, formtarget, output, attr()
in QEMU offline.

- web-open /web/lot76.html: paint color/date/output; submit GET with
  formtarget=_blank to the host echo server; expect c, d, sum in the query.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot76-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot76-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot76-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot76-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402

ROOT = g.ROOT


def say(msg):
    print("[lot76] %s" % msg, flush=True)


def server_text():
    return open(os.path.join(g.LOG_DIR, "lot76-web-server.log")).read()


def wait_server(needle, timeout=60):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if needle in server_text():
            return
        time.sleep(0.3)
    raise RuntimeError("server never saw %r; tail:\n%s" % (needle, server_text()[-1500:]))


def controls(out):
    found = {}
    for m in re.finditer(r"osui web form2 ctl (\d+) (\w+) id=(\S+) box (-?\d+) (-?\d+) (\d+) (\d+) value=([^\r\n]*)", out):
        k = int(m.group(1))
        typ = m.group(2)
        iid = m.group(3)
        box = (k, typ, int(m.group(4)), int(m.group(5)), int(m.group(6)), int(m.group(7)), m.group(8).strip())
        found["#%d" % k] = box
        if iid != "-":
            found[iid] = box
        elif typ not in found:
            found[typ] = box
    return found


def fstate(mon, proc):
    out = lw.run(mon, proc, "web-fstate", "osui web form2 state", 30)
    out = out[out.rfind("osui web form2 state"):]
    return out, controls(out)


def click(mon, proc, ctl, marker="osui web form2"):
    x, y = ctl[2] + ctl[4] // 2, ctl[3] + ctl[5] // 2
    return lw.run(mon, proc, "web-fclick %d %d" % (x, y), marker, 30)


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
    hp = int(os.environ.get("LOT76_PORT", "18076"))
    srv_log = open(os.path.join(g.LOG_DIR, "lot76-web-server.log"), "w")
    srv = subprocess.Popen([sys.executable, os.path.join(ROOT, "scripts", "lot67_web_server.py"), lw.PKI, str(hp), str(hp + 1)],
                           stdout=srv_log, stderr=subprocess.STDOUT)
    proc = mon = qmp = None
    t0 = time.monotonic()
    try:
        g.prepare_test_disk()
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4", "-cpu", os.environ.get("LOT76_CPU", "max"),
                "-kernel", g.KERNEL, "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet",
                "-serial", "file:" + g.LOG, "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-qmp", "unix:%s,server,nowait" % g.QMP_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
                "-netdev", "user,id=n0", "-device", "ne2k_isa,netdev=n0",
            ] + g.qemu_disk_args(), cwd=ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            mon = g.monitor_connect()
            qmp = g.qmp_connect()
            g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
            time.sleep(0.5)

            lw.run(mon, proc, "web-open /web/lot76.html", "osui web", 60, fail=["error="])
            out, ctl = fstate(mon, proc)
            say("local controls: %r" % ctl)
            if "c" not in ctl or "d" not in ctl or "go" not in ctl:
                raise RuntimeError("lot76.html: want c/d/go controls: %r" % ctl)
            # color and date values from markup
            if "#ff0000" not in ctl["c"][6] and "ff0000" not in ctl["c"][6]:
                say("color value note: %r" % ctl["c"][6])
            if "2024-02-29" not in ctl["d"][6]:
                say("date value note: %r" % ctl["d"][6])
            out = click(mon, proc, ctl["go"], marker="osui web")
            say("submit: %r" % out[-300:])
            wait_server("c=%23ff0000", timeout=90)
            wait_server("d=2024-02-29", timeout=30)
            wait_server("sum=42", timeout=30)
            say("lot76 qemu ok in %.1fs" % (time.monotonic() - t0))
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
        print("[lot76] FAIL: %s" % e, flush=True)
        sys.exit(1)
