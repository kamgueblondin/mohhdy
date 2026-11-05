#!/usr/bin/env python3
"""Lot 74: interactive forms in the browser, @layer, :has(), rowspan and
:target, in QEMU, offline and deterministic.

The page /web/lot74.html (initrd) is driven from the console with the
same code as the desktop (osui_gui_web_command):
- web-render: LAYERWIN painted in the colour of the later layer (theme)
  although the earlier one (base) is more specific; HASYES in the :has()
  colour, HASNO (its p is not a child of the section) not; the rowspan=0
  cell spans the three rows of its group.
- web-open, web-fstate (controls with their boxes and values), a click
  in the text field (web-fclick at its box), web-ftype hello, Tab to the
  checkbox, space checks it, Tab to the number field, Up steps it to 3;
  web-fstate shows q=hello, c=on, n=3. Enter submits the first form with
  GET to the Lot 67 test server (scripts/lot67_web_server.py on the
  fixed port LOT74_PORT, 18074 by default, the action of the page):
  the server sees /echo?q=hello&c=on&n=3.
- the second form: Send with its required field empty is refused (the
  focus moves to the field, no request); web-ftype x y, Enter: POST
  application/x-www-form-urlencoded body t=x+y through the shell fetch
  path (shell_page_request).
- web-open /web/lot74.html#tgt: the target element is found and the
  page scrolls to it.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot74-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot74-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot74-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot74-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402

ROOT = g.ROOT


def say(msg):
    print("[lot74] %s" % msg, flush=True)


def server_text():
    return open(os.path.join(g.LOG_DIR, "lot74-web-server.log")).read()


def wait_server(needle, timeout=60):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if needle in server_text():
            return
        time.sleep(0.3)
    raise RuntimeError("server never saw %r; tail:\n%s" % (needle, server_text()[-1500:]))


def items(out):
    found = []
    for m in re.finditer(r'web item text (-?\d+) (-?\d+) (\d+)x(\d+) (#[0-9a-fA-F]+)(?: bold)? "([^"]*)"', out):
        found.append((int(m.group(1)), int(m.group(2)), m.group(5).lower(), m.group(6)))
    return found


def controls(out):
    """web-fstate lines: id -> (k, x, y, w, h, value)"""
    found = {}
    for m in re.finditer(r"osui web form2 ctl (\d+) (\w+) id=(\S+) box (-?\d+) (-?\d+) (\d+) (\d+) value=([^\r\n]*)", out):
        found[m.group(3)] = (int(m.group(1)), int(m.group(4)), int(m.group(5)), int(m.group(6)), int(m.group(7)), m.group(8).strip())
    return found


def fstate(mon, proc):
    out = lw.run(mon, proc, "web-fstate", "osui web form2 state", 30)
    out = out[out.rfind("osui web form2 state"):]
    return out, controls(out)


def click(mon, proc, ctl, marker="osui web form2"):
    x, y = ctl[1] + ctl[3] // 2, ctl[2] + ctl[4] // 2
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
    hp = int(os.environ.get("LOT74_PORT", "18074"))
    srv_log = open(os.path.join(g.LOG_DIR, "lot74-web-server.log"), "w")
    srv = subprocess.Popen([sys.executable, os.path.join(ROOT, "scripts", "lot67_web_server.py"), lw.PKI, str(hp), str(hp + 1)],
                           stdout=srv_log, stderr=subprocess.STDOUT)
    proc = mon = qmp = None
    t0 = time.monotonic()
    try:
        g.prepare_test_disk()
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4", "-cpu", os.environ.get("LOT74_CPU", "max"),
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
            # ---- layout: @layer, :has(), rowspan=0
            out = lw.run(mon, proc, "web-render /web/lot74.html", "web-render ok", 60, fail=["error"])
            out = out[out.rfind("web-render /web/lot74.html"):] if "web-render /web/lot74.html" in out else out
            found = items(out)
            by = {}
            for it in found:
                by.setdefault(it[3], it)
            say("layout items: %r" % [(t, c) for (_, _, c, t) in found])
            if "LAYERWIN" not in by or by["LAYERWIN"][2] != "#0000aa00":
                raise RuntimeError("@layer: want LAYERWIN in #00aa00 (the later layer): %r" % (by.get("LAYERWIN"),))
            if "HASYES" not in by or by["HASYES"][2] != "#000000aa":
                raise RuntimeError(":has(> p > b): want HASYES in #0000aa: %r" % (by.get("HASYES"),))
            if "HASNO" not in by or by["HASNO"][2] == "#000000aa":
                raise RuntimeError(":has() anchored: HASNO must keep the body colour: %r" % (by.get("HASNO"),))
            for want in ("SPAN3", "R1", "R2"):
                if want not in by:
                    raise RuntimeError("table: want %r in %r" % (want, sorted(by)))
            if not (by["R1"][0] > by["SPAN3"][0] and by["R2"][0] == by["R1"][0] and by["R2"][1] > by["R1"][1]):
                raise RuntimeError("rowspan=0: R1 and R2 beside the span: %r %r %r" % (by["SPAN3"], by["R1"], by["R2"]))
            say("layout: the later layer wins, :has() anchored, rowspan=0 spans its group")
            # ---- forms: focus, typing, checkbox, number, GET
            lw.run(mon, proc, "web-open /web/lot74.html", "osui web", 60, fail=["error="])
            out, ctl = fstate(mon, proc)
            say("controls: %r" % ctl)
            if "controls 6 focus -1" not in out:
                raise RuntimeError("web-fstate: want 6 controls, no focus: %r" % out[:300])
            for want in ("q", "c", "n", "go", "t", "send"):
                if want not in ctl:
                    raise RuntimeError("web-fstate: no control %r in %r" % (want, ctl))
            out = click(mon, proc, ctl["q"])
            if "osui web form2 changed focus 0" not in out:
                raise RuntimeError("click on the text field: %r" % out[-400:])
            out = lw.run(mon, proc, "web-ftype hello", "osui web form2", 30)
            if "typed 5" not in out:
                raise RuntimeError("web-ftype: %r" % out[-400:])
            lw.run(mon, proc, "web-fkey tab", "osui web form2 changed focus 1", 30)
            lw.run(mon, proc, "web-fkey space", "osui web form2", 30)
            lw.run(mon, proc, "web-fkey tab", "osui web form2 changed focus 2", 30)
            lw.run(mon, proc, "web-fkey up", "osui web form2", 30)
            out, ctl = fstate(mon, proc)
            if ctl["q"][5] != "hello" or ctl["c"][5] != "on" or ctl["n"][5] != "3":
                raise RuntimeError("form state: want q=hello c=on n=3: %r" % ctl)
            say("forms: click focus, typing, Tab, checkbox, number step: q=hello c=on n=3")
            out = lw.run(mon, proc, "web-fkey enter", "osui web form2 submit", 60)
            if "osui web form2 submit GET http://10.0.2.2:%d/echo?q=hello&c=on&n=3" % hp not in out:
                raise RuntimeError("GET submission: %r" % out[-600:])
            wait_server("ECHO GET q='q=hello&c=on&n=3'")
            say("GET: the server saw /echo?q=hello&c=on&n=3")
            # ---- POST: required refusal, then the urlencoded body
            lw.run(mon, proc, "web-open /web/lot74.html", "osui web", 60, fail=["error="])
            out, ctl = fstate(mon, proc)
            n_post = server_text().count("POST /echo")
            out = click(mon, proc, ctl["send"])
            if "osui web form2 changed focus 4" not in out:
                raise RuntimeError("required: want the focus on the empty field: %r" % out[-400:])
            time.sleep(1.0)
            if server_text().count("POST /echo") != n_post:
                raise RuntimeError("required: an invalid form was sent")
            lw.run(mon, proc, "web-ftype x y", "osui web form2 typed 3", 30)
            out = lw.run(mon, proc, "web-fkey enter", "osui web form2 submit", 60)
            if "submit POST http://10.0.2.2:%d/echo body t=x+y" % hp not in out:
                raise RuntimeError("POST submission: %r" % out[-600:])
            wait_server("POST /echo ctype=application/x-www-form-urlencoded body=b't=x+y'")
            wait_server("ECHO POST q='' body=b't=x+y'")
            say("POST: required refused first, then t=x+y urlencoded through the fetch path")
            # ---- fragment: :target and the scroll
            out = lw.run(mon, proc, "web-open /web/lot74.html#tgt", "osui web target", 60)
            m = re.search(r"osui web target id=tgt scroll (\d+)", out)
            if not m or int(m.group(1)) < 900:
                raise RuntimeError("fragment: want the target found and the page scrolled: %r" % out[-400:])
            say("fragment: #tgt found, scroll %s" % m.group(1))
        say("QEMU Lot 74 contract passed in %.0f s." % (time.monotonic() - t0))
        return 0
    finally:
        if qmp is not None:
            qmp.close()
        if mon is not None:
            mon.close()
        g.terminate(proc)
        srv.terminate()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU Lot 74 failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
