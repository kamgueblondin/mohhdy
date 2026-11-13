#!/usr/bin/env python3
"""Lot 92 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

  * reader backgrounds (/lot92/bg.html): linear-gradient angles in
    radians are taken when they are a quarter turn within half a degree
    and written as whole degrees (1.5708rad -> 90deg, -3.1416rad ->
    -180deg); 1rad is refused (the colour stays). A registered <angle>
    whose value is mod(450deg, 360deg) is computed to 90deg and the
    gradient takes it; the same value in an unregistered property keeps
    its text and the gradient is refused.
  * computed registered values (/lot92/props.html): a registered <length>
    calc(20px * 2) gives a 40px radial radius (unregistered: refused);
    round(up, 21px, 4px) gives a 24px font size; the initial value
    calc(1in / 8) is computed to 12px; calc(sin(30deg) * 10px) was typed
    and kept in Lot 92; Lot 93 (converted) computes it to 5px, so
    font-size takes it and css-props counts three computed values
    ("simplified 3", was 2).
No guest clock is involved (no -rtc option).
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot92-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot92-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot92-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot92-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot92-web-server.log")
need, refuse = l88.need, l88.refuse


def say(msg):
    print("[lot92] %s" % msg, flush=True)


def css_page(mon, proc, u, page, fin, keep, hide=()):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-open %s/lot92/%s" % (u, page), "osui web page sections", fail=["error="])
    css = lw.run(mon, proc, "web-css", "css-props ", 30)
    read = lw.run(mon, proc, "web-read", fin, 30)
    need(read, list(keep), "web-read " + page)
    refuse(read, list(hide), "web-read " + page)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    return css


def node_line(css, ident):
    m = re.search(r"node \w+ #%s [^\n]*\n" % ident, css)
    if not m:
        raise RuntimeError("no node line for #%s: %r" % (ident, css[-900:]))
    return m.group(0)


def step_backgrounds(mon, proc, u):
    css = css_page(mon, proc, u, "bg.html", "fin92b",
                   ["radkeep92", "radnkeep92", "radbadkeep92", "angkeep92", "angukeep92"])
    need(css, ["node div #rad92 display auto bgimg linear-gradient(90deg, #ff0000, #0000ff)\n",
               "node div #radn92 display auto bgimg linear-gradient(-180deg, #00ff00, #000000) bgrep no-repeat\n",
               "node div #radbad92 display auto bg #123456\n",
               "node div #ang92 display auto bgimg linear-gradient(90deg, #ff0000, #0000ff)\n",
               "node div #angu92 display auto bg #654321\n",
               "css-props registered 1 rejected 0 ", " simplified 1\n"], "web-css backgrounds")
    for ident in ("rad92", "radn92", "ang92"):
        refuse(node_line(css, ident), ["rad,", "rad)", "mod("], "web-css #" + ident)
    say("reader: 1.5708rad -> 90deg, -3.1416rad -> -180deg, 1rad refused; registered <angle> "
        "mod(450deg, 360deg) -> 90deg drawn, the unregistered one refused")


def step_props(mon, proc, u):
    css = css_page(mon, proc, u, "props.html", "fin92p",
                   ["circkeep92", "ucirckeep92", "sizekeep92", "initkeep92", "trigkeep92"])
    need(css, ["node div #circ92 display auto bgimg radial-gradient(circle 40px, #ff0000, #0000ff)\n",
               "node div #ucirc92 display auto bg #224466\n",
               "node p #size92 display auto size 24px\n",
               "node p #init92 display auto size 12px\n",
               "node p #trig92 display auto size 5px\n",
               "css-props registered 3 rejected 0 ", " simplified 3\n"], "web-css computed values")
    say("computed: calc(20px * 2) -> radius 40px (unregistered refused), round(up, 21px, 4px) -> 24px, "
        "initial calc(1in / 8) -> 12px, calc(sin(30deg) * 10px) -> 5px (Lot 93); simplified 3")


def boot_reader(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        u = "http://10.0.2.2:%d" % hp
        step_backgrounds(mon, proc, u)
        step_props(mon, proc, u)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot92-serial-1x.log")


def main():
    if not os.path.isfile(os.path.join(lw.PKI, "test_ca.pem")):
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "lot67_gen_pki.py"), lw.PKI], check=True)
    os.makedirs(g.LOG_DIR, exist_ok=True)
    hp = 31000 + os.getpid() % 9000
    srv_log = open(SRV_LOG, "w")
    srv = subprocess.Popen([sys.executable, os.path.join(ROOT, "scripts", "lot67_web_server.py"), lw.PKI,
                            str(hp), str(hp + 1)], stdout=srv_log, stderr=subprocess.STDOUT)
    t0 = time.monotonic()
    try:
        boot_reader(hp)
        say("lot92 qemu ok in %.1fs" % (time.monotonic() - t0))
        return 0
    finally:
        srv.terminate()
        try:
            srv.wait(timeout=5)
        except Exception:
            srv.kill()
        srv_log.close()


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:  # noqa: BLE001
        say("FAIL: %s" % exc)
        sys.exit(1)
