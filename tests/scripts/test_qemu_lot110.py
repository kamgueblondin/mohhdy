#!/usr/bin/env python3
"""Lot 110 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot110/body.html, web-css, reader clock 500 ms). Every
keyframes colour runs 1000ms. html has overflow: hidden and height:
400px, so the body's overflow: auto is no longer propagated to the
viewport (CSS Overflow 3): the body is a box scroller of its own. Its
height: 50% is taken against html's 400px: the reader passes
"margin:0px 0px 0px 0px;overflow-y:auto;height:200px" and the engine
paints the body's bar at y 0..200 (x 634..640), range 634 px. #i110
(scroll(nearest)), #n110 (the body's named timeline --b110) and #v110
(view()) all find the body. Before any click #i110 and #n110 are
#000000 and #v110 has no colour (before its view range).
"web-bscroll 637 100" scrolls the body to 634 * 100 / 200 = 317: #i110
and #n110 #808080, #v110 #848484. "web-bscroll 637 190" scrolls to
602: #f2f2f2, #v110 past its range (no colour). "web-bscroll 637 20"
scrolls to 63: #191919.
Opacity: #o110 opacity: 0.5 (opacity:50%), #a110 keyframes from 0 to
100% at 500 ms (opacity:50%, one animated), #t110 opacity: 0.8 with a
1s transition from @starting-style 0 (opacity:40%, one transitioned):
the css-anim line reads "opacity 1 1".
The Lot 109 page is checked by make qemu-lot109.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot110-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot110-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot110-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot110-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot110-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["ikeep110", "nkeep110", "vkeep110", "okeep110", "akeep110", "tkeep110"]
ANIM = ("css-anim keyframes 2 animated 0 transitioned 0 font 0 at 500ms scroll 2 pos 0 color %d 0 opacity 1 1 "
        "view 1 need 2 geom 2 box 3\n")
NEVER = ["node p #o110 display auto color", "node p #a110 display auto color", "node p #t110 display auto color",
         "opacity 0 0 view"]


def say(msg):
    print("[lot110] %s" % msg, flush=True)


def node(i, c):
    return "node p #%s display auto color %s\n" % (i, c)


def css(mon, proc):
    return lw.run(mon, proc, "web-css", "css-anim ", 30)


def view(c):
    return node("v110", c) if c else "node p #v110 display auto\n"


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-css time 500", "web-css time 500\n", 20)
    lw.run(mon, proc, "web-open %s/lot110/body.html" % u, "osui web page sections", fail=["error="])
    out = css(mon, proc)
    read = lw.run(mon, proc, "web-read", "fin110", 30)
    need(read, KEEP, "web-read body")
    need(out, [node("i110", "#000000"), node("n110", "#000000"), view(None), ANIM % 2], "web-css before")
    refuse(out, NEVER, "web-css before")
    steps = [(100, "#808080", "#848484", 3, "317"), (190, "#f2f2f2", None, 2, "602"), (20, "#191919", None, 2, "63")]
    for y, c, v, nc, at in steps:
        lw.run(mon, proc, "web-bscroll 637 %d" % y, "osui web bscroll hit restyled", 30, fail=["error="])
        out = css(mon, proc)
        need(out, [node("i110", c), node("n110", c), view(v), ANIM % nc], "web-css at body " + at)
        refuse(out, NEVER, "web-css at body " + at)
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: body box scroller 200 px (50% of html 400px, range 634): #808080 / view #848484 at 317, "
        "#f2f2f2 at 602, #191919 at 63; opacity animated 1, transitioned 1")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot110-serial-1x.log")


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
        say("lot110 qemu ok in %.1fs" % (time.monotonic() - t0))
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
