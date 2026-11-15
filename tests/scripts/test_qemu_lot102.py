#!/usr/bin/env python3
"""Lot 102 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot102/range.html, web-css): the reader scroll position is
set with "web-css scroll N" (N in percent) before the page is opened.
rgb(255 none 0) is painted as color #ff0000; color-mix(in srgb, rgb(none
0 0), rgb(200 0 0)) as bg #c80000 (the missing red takes the other
colour's 200; 0 would give #640000). #s102 runs a keyframes colour
(black to white, 1000ms linear) on scroll() (nearest: the reader
viewport): #404040 at 25%, #808080 at 50%, #bfbfbf at 75%. #r102 runs it
on scroll(root) with animation-range: 20% 60%: #202020 at 25%, #bfbfbf
at 50%, over at 75% (no colour). The root names --page102 but body has
timeline-scope: --page102 and no descendant defines it, so #q102 stays
idle. #v102 on view() (Lot 104: active, fed by the layout geometry; the
page does not scroll, so its progress is that of its box at offset 262 in
the 480 px viewport) is #747474 at every position. The css-anim line
counts the two scroll animations (scroll 2), the colours painted (color
3, then 2 at 75%) and ends with "view 1 need 1 geom 1". The Lot 101 page is checked by make qemu-lot101.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot102-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot102-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot102-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot102-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot102-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["nkeep102", "mkeep102", "skeep102", "rkeep102", "qkeep102", "vkeep102"]
STATIC = ["node div #n102 display auto color #ff0000\n",
          "node div #m102 display auto bg #c80000\n",
          "node div #q102 display auto\n",
          "node div #v102 display auto color #747474\n"]
ANIM = "css-anim keyframes 1 animated 0 transitioned 0 font 0 at 500ms scroll 2 pos %d color %d 0 view 1 need 1 geom 1\n"


def say(msg):
    print("[lot102] %s" % msg, flush=True)


def at_scroll(mon, proc, u, pct):
    lw.run(mon, proc, "web-css scroll %d" % pct, "web-css scroll %d%%\n" % pct, 20)
    lw.run(mon, proc, "web-open %s/lot102/range.html" % u, "osui web page sections", fail=["error="])
    return lw.run(mon, proc, "web-css", "css-anim ", 30)


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-css time 500", "web-css time 500\n", 20)
    css = at_scroll(mon, proc, u, 25)
    read = lw.run(mon, proc, "web-read", "fin102", 30)
    need(read, KEEP, "web-read ranges")
    need(css, STATIC + ["node div #s102 display auto color #404040\n",
                        "node div #r102 display auto color #202020\n", ANIM % (2500, 3)], "web-css at 25%")
    css = at_scroll(mon, proc, u, 50)
    need(css, STATIC + ["node div #s102 display auto color #808080\n",
                        "node div #r102 display auto color #bfbfbf\n", ANIM % (5000, 3)], "web-css at 50%")
    css = at_scroll(mon, proc, u, 75)
    need(css, STATIC + ["node div #s102 display auto color #bfbfbf\n",
                        "node div #r102 display auto\n", ANIM % (7500, 2)], "web-css at 75%")
    refuse(css, ["node div #r102 display auto color", "node div #q102 display auto color",
                 "node div #v102 display auto\n"], "web-css at 75%")
    lw.run(mon, proc, "web-css scroll 0", "web-css scroll 0%\n", 20)
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: none red #ff0000, carried mix bg #c80000; scroll() 25/50/75% #404040 #808080 #bfbfbf; "
        "range 20%-60% #202020 #bfbfbf then over; scoped name idle, view() #747474")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot102-serial-1x.log")


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
        say("lot102 qemu ok in %.1fs" % (time.monotonic() - t0))
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
