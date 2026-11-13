#!/usr/bin/env python3
"""Lot 95 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

  * @property limits and computed values in the reader (/lot95/props.html,
    web-css): nine registrations are kept (Lot 90 kept eight), with
    '<length> | <transform-list>', eight alternatives ('a | ... | g |
    <length>'), <angle>#, <length-percentage>, <transform-list>,
    <resolution>, <image>, '<string> | <url>' and a 31-byte name; nine
    alternatives are refused (rejected 1). Registered values are computed:
    the inherited initial 0.125in is a 12px font size, 2.5vw at the 640 x
    480 reader viewport 16px, the initial 1pc of the eight-alternative
    property 16px, the 31-byte name's 0.25in 24px; a one-item <angle>#
    list 100grad gives a 90deg gradient; calc(1in + 10% - 2%) is the mix
    calc(8% + 96px), which text-indent takes (97.28px, as the reader
    resolved that text before; the raw text it did not); an <image>
    initial value is drawn as a gradient. css-props counts five computed
    declared values (2.5vw, 100grad, the mix, the transform list and
    96dpi).
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot95-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot95-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot95-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot95-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot95-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["inkeep95", "vwkeep95", "pckeep95", "angkeep95", "mixkeep95", "tkeep95", "rkeep95", "imgkeep95",
        "longkeep95"]


def say(msg):
    print("[lot95] %s" % msg, flush=True)


def node_line(css, ident):
    m = re.search(r"node \w+ #%s [^\n]*\n" % ident, css)
    if not m:
        raise RuntimeError("no node line for #%s: %r" % (ident, css[-900:]))
    return m.group(0)


def step_props(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-open %s/lot95/props.html" % u, "osui web page sections", fail=["error="])
    css = lw.run(mon, proc, "web-css", "css-props ", 30)
    read = lw.run(mon, proc, "web-read", "fin95", 30)
    need(read, KEEP, "web-read props")
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    need(css, ["node p #in95 display auto size 12px\n",
               "node p #vw95 display auto size 16px\n",
               "node p #pc95 display auto size 16px\n",
               "node div #ang95 display auto bgimg linear-gradient(90deg, #ff0000, #0000ff)\n",
               "node p #mix95 display auto indent 97.28px\n",
               "node p #t95 display auto\n",
               "node p #r95 display auto\n",
               "node div #img95 display auto bgimg linear-gradient(#ff0000, #0000ff)\n",
               "node p #long95 display auto size 24px\n",
               "css-props registered 9 rejected 1 computed 0 invalid 0 unset 0 ", " simplified 5\n"],
         "web-css props")
    for ident in ("in95", "vw95", "ang95", "mix95", "long95"):
        refuse(node_line(css, ident), ["var(", "calc(", "100grad", "2.5vw", "0.125in", "0.25in"], "web-css #" + ident)
    say("nine registrations, eight alternatives, a 31-byte name, nine alternatives refused; 0.125in -> 12px, "
        "2.5vw -> 16px, 1pc -> 16px, 0.25in -> 24px, 100grad -> 90deg, calc(8% + 96px) -> indent 97.28px, "
        "<image> drawn; simplified 5")


def boot_reader(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_props(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot95-serial-1x.log")


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
        say("lot95 qemu ok in %.1fs" % (time.monotonic() - t0))
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
