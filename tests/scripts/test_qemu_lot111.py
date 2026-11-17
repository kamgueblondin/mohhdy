#!/usr/bin/env python3
"""Lot 111 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot111/box.html, web-css, reader clock 500 ms): the Lot 110
page (same content, so the same geometry) with html's height given as
max(20lh, min(10%, 40px)) and html's line-height 20px. lh is html's own
line height (20 px, not the 10 px of normal), and the mixed min() nested
in max() is taken (Lot 110 refused both): html is 400 px whatever the
viewport (min(10%, 40px) is at most 40 px), so the body's height: 50% is
passed as 200px and the engine paints the body's bar at y 0..200, range
634 px, as in Lot 110. #i111 (scroll(nearest)), #n111 (the body's named
timeline --b111) and #v111 (view()) find the body: #000000, #000000 and
no colour before any click; "web-bscroll 637 100" (317): #808080 and
#v111 #848484; "web-bscroll 637 190" (602): #f2f2f2; "web-bscroll 637
20" (63): #191919.
Opacity in the report's node lines (Lot 111): #o111 opacity: calc(0.25 +
0.5) is "opacity 0.7500", #a111 opacity: var(--a) with --a: 40% on the
body is "opacity 0.4000", #t111 opacity: 0.9 then revert-layer goes back
to its layer's 0.2: "opacity 0.2000". #t111's colour is
color-mix(in srgb, rgb(from rgb(none 0 0) r g b), rgb(200 0 0)): the
origin's missing red stays missing, so the mix takes 200: #c80000
(#640000 before Lot 111).
The Lot 110 page is checked by make qemu-lot110.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot111-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot111-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot111-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot111-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot111-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["ikeep111", "nkeep111", "vkeep111", "okeep111", "akeep111", "tkeep111"]
ANIM = "css-anim keyframes 1 animated 0 transitioned 0 font 0 at 500ms scroll 2 pos 0 color %d 0 view 1 need 2 geom 2 box 3\n"
FIXED = ["node p #o111 display auto opacity 0.7500\n", "node p #a111 display auto opacity 0.4000\n",
         "node p #t111 display auto color #c80000 opacity 0.2000\n"]
NEVER = ["node p #o111 display auto\n", "node p #a111 display auto\n", "#640000", "opacity 0.9000",
         "node p #o111 display auto color", "node p #a111 display auto color"]


def say(msg):
    print("[lot111] %s" % msg, flush=True)


def node(i, c):
    return "node p #%s display auto color %s\n" % (i, c)


def css(mon, proc):
    return lw.run(mon, proc, "web-css", "css-anim ", 30)


def view(c):
    return node("v111", c) if c else "node p #v111 display auto\n"


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-css time 500", "web-css time 500\n", 20)
    lw.run(mon, proc, "web-open %s/lot111/box.html" % u, "osui web page sections", fail=["error="])
    out = css(mon, proc)
    read = lw.run(mon, proc, "web-read", "fin111", 30)
    need(read, KEEP, "web-read box")
    need(out, [node("i111", "#000000"), node("n111", "#000000"), view(None), ANIM % 2] + FIXED, "web-css before")
    refuse(out, NEVER, "web-css before")
    steps = [(100, "#808080", "#848484", 3, "317"), (190, "#f2f2f2", None, 2, "602"), (20, "#191919", None, 2, "63")]
    for y, c, v, nc, at in steps:
        lw.run(mon, proc, "web-bscroll 637 %d" % y, "osui web bscroll hit restyled", 30, fail=["error="])
        out = css(mon, proc)
        need(out, [node("i111", c), node("n111", c), view(v), ANIM % nc] + FIXED, "web-css at body " + at)
        refuse(out, NEVER, "web-css at body " + at)
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: body box scroller 200 px (50% of html max(20lh, min(10%, 40px)) = 400px, range 634): #808080 / "
        "view #848484 at 317, #f2f2f2 at 602, #191919 at 63; opacity 0.75 (calc), 0.4 (var), 0.2 (revert-layer); "
        "rgb(from rgb(none 0 0) ...) mixed #c80000")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot111-serial-1x.log")


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
        say("lot111 qemu ok in %.1fs" % (time.monotonic() - t0))
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
