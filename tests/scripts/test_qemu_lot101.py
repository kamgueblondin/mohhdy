#!/usr/bin/env python3
"""Lot 101 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot101/space.html, web-css): the reader scroll position is
set with "web-css scroll N" (N in percent) before the page is opened.
lab(50 0 0) is painted as color #777777; oklch(0.5 0 0) as bg #636363;
color(a98-rgb 0.5 0.5 0.5) as color #818181; color-mix(in srgb,
currentcolor 25%, blue) on an element whose colour is red as bg #4000bf.
The root html names a scroll timeline (scroll-timeline: --page101) and
#s101 runs a keyframes colour (black to white, 1000ms linear) on it: at
25% the colour is #404040 and at 75% #bfbfbf whatever the reader clock
(500ms is set to show it), and the css-anim line has "scroll 1 pos
2500 color 1 0"; at 100% the animation is over (no colour, no colour
part). #v101 names a view timeline, which is never active, so its
animation stays idle. An out-of-range position is refused with the
usage line. The Lot 100 page is checked by make qemu-lot100.
No guest clock is involved (no -rtc option).
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "..", "scripts"))
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot101-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot101-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot101-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot101-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot101-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["lkeep101", "okeep101", "akeep101", "ukeep101", "skeep101", "vkeep101"]
STATIC = ["node div #l101 display auto color #777777\n",
          "node div #o101 display auto bg #636363\n",
          "node div #a101 display auto color #818181\n",
          "node div #u101 display auto color #ff0000 bg #4000bf\n",
          "node div #v101 display auto\n"]


def say(msg):
    print("[lot101] %s" % msg, flush=True)


def at_scroll(mon, proc, u, pct):
    lw.run(mon, proc, "web-css scroll %d" % pct, "web-css scroll %d%%\n" % pct, 20)
    lw.run(mon, proc, "web-open %s/lot101/space.html" % u, "osui web page sections", fail=["error="])
    return lw.run(mon, proc, "web-css", "css-anim ", 30)


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-css time 500", "web-css time 500\n", 20)
    css = at_scroll(mon, proc, u, 25)
    read = lw.run(mon, proc, "web-read", "fin101", 30)
    need(read, KEEP, "web-read spaces")
    need(css, STATIC + ["node div #s101 display auto color #404040\n",
                        "css-anim keyframes 1 animated 0 transitioned 0 font 0 at 500ms scroll 1 pos 2500 color 1 0\n"],
         "web-css at 25%")
    css = at_scroll(mon, proc, u, 75)
    need(css, STATIC + ["node div #s101 display auto color #bfbfbf\n",
                        "css-anim keyframes 1 animated 0 transitioned 0 font 0 at 500ms scroll 1 pos 7500 color 1 0\n"],
         "web-css at 75%")
    css = at_scroll(mon, proc, u, 100)
    need(css, STATIC + ["node div #s101 display auto\n",
                        "css-anim keyframes 1 animated 0 transitioned 0 font 0 at 500ms scroll 1 pos 10000\n"],
         "web-css at 100%")
    refuse(css, [" color 1", " color 0"], "web-css at 100%")
    lw.run(mon, proc, "web-css scroll 101", "usage: web-css scroll <0..100>", 20)
    lw.run(mon, proc, "web-css scroll 0", "web-css scroll 0%\n", 20)
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: lab #777777, oklch bg #636363, a98-rgb #818181, currentcolor mix bg #4000bf; "
        "scroll timeline 25% #404040, 75% #bfbfbf, 100% over; view timeline idle; 101 refused")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot101-serial-1x.log")


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
        boot(hp)
        say("lot101 qemu ok in %.1fs" % (time.monotonic() - t0))
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
