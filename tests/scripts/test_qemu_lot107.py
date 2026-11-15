#!/usr/bin/env python3
"""Lot 107 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot107/box.html, web-css, reader clock 500 ms). Every
keyframes colour runs 1000ms. The body has font-size: 20px: the reader
writes it with the body's colour on the body element ("color:#808080;
font-size:20px"; Lot 106 wrote a bare body), so the layout below is at
20 px; without the body style it would be at 16 px and #v107 would be
#191919 at 60% (refused). #o107 is a box scroller with height: 10em at
font-size: 10px: the reader passes "font-size:10px;overflow-y:auto;
height:100px" (scroll(nearest) on #i107 reads it) and the layout hands
back a 100 px scrollport (scroll range 126 px, block offset 49). #m107
has max-height: 5em (20 px font): the reader passes "overflow-y:auto;
max-height:100px", the layout gives a 100 px scrollport (range 153 px,
offset 149). "web-bscroll 630 94" scrolls #o107 to 56 px: #i107 56 /
126, #717171; "web-bscroll 630 164" scrolls #m107 to 22 px: #n107 22 /
153, #242424. Before, both are #000000.
#v107 runs view() on the root (block offset 953, 13 px high, view 480,
root range 908; the page fits one section, under 120 nodes) with auto
insets taken from html's scroll-padding-block: 100px 50px (CSS
Scroll-driven Animations 1): cover [953 - 480 + 50, 966 - 100] = [523,
866]. At "web-css scroll 55" (499.4 px): before the start, no colour
(without the insets [473, 966] it would be #0e0e0e); at 60% (544.8 px):
#101010 (#252525 without them); at 100% (908 px): past the end, no
colour (#e1e1e1 without them).
#e107 runs transparent to palegoldenrod with cubic-bezier(0.5, 1.5, 0.5,
1.5): progress 1.25 at 500 ms; legacy sRGB premultiplied with the
unclamped alpha 1.25 then divided by it (CSS Color 4 12.3): #eee8aa
(clamping the alpha first gave #ffffd5).
The Lot 106 page is checked by make qemu-lot106.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot107-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot107-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot107-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot107-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot107-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["ikeep107", "nkeep107", "vkeep107", "ekeep107"]
ANIM = "css-anim keyframes 2 animated 0 transitioned 0 font 0 at 500ms scroll 2 pos %d color %d 0 view 1 need 3 geom 3 box 2\n"
FIXED = ["node body display auto color #808080 size 20px\n", "node div #o107 display auto size 10px\n",
         "node p #e107 display auto color #eee8aa\n"]
NEVER = ["#e107 display auto color #ffffd5", "node body display auto\n"]


def say(msg):
    print("[lot107] %s" % msg, flush=True)


def node(i, c=None):
    return "node p #%s display auto%s\n" % (i, " color %s" % c if c else "")


def css(mon, proc):
    return lw.run(mon, proc, "web-css", "css-anim ", 30)


def at_scroll(mon, proc, u, pct):
    lw.run(mon, proc, "web-css scroll %d" % pct, "web-css scroll %d%%\n" % pct, 20)
    lw.run(mon, proc, "web-open %s/lot107/box.html" % u, "osui web page sections", fail=["error="])
    return css(mon, proc)


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-css time 500", "web-css time 500\n", 20)
    out = at_scroll(mon, proc, u, 0)
    read = lw.run(mon, proc, "web-read", "fin107", 30)
    need(read, KEEP, "web-read box")
    need(out, FIXED + [node("i107", "#000000"), node("n107", "#000000"), node("v107"), ANIM % (0, 4)], "web-css at 0%")
    refuse(out, NEVER + ["#v107 display auto color"], "web-css at 0%")
    lw.run(mon, proc, "web-bscroll 630 94", "osui web bscroll hit restyled", 30, fail=["error="])
    out = css(mon, proc)
    need(out, FIXED + [node("i107", "#717171"), node("n107", "#000000"), ANIM % (0, 4)], "web-css at #o107 56")
    lw.run(mon, proc, "web-bscroll 630 164", "osui web bscroll hit restyled", 30, fail=["error="])
    out = css(mon, proc)
    need(out, FIXED + [node("i107", "#717171"), node("n107", "#242424"), ANIM % (0, 4)], "web-css at #m107 22")
    out = at_scroll(mon, proc, u, 55)
    need(out, FIXED + [node("v107"), ANIM % (5500, 4)], "web-css at 55%")
    refuse(out, NEVER + ["#v107 display auto color"], "web-css at 55%")
    out = at_scroll(mon, proc, u, 60)
    need(out, FIXED + [node("v107", "#101010"), ANIM % (6000, 5)], "web-css at 60%")
    refuse(out, NEVER + ["#v107 display auto color #252525", "#v107 display auto color #191919"], "web-css at 60%")
    out = at_scroll(mon, proc, u, 100)
    need(out, FIXED + [node("v107"), ANIM % (10000, 4)], "web-css at 100%")
    refuse(out, NEVER + ["#v107 display auto color"], "web-css at 100%")
    lw.run(mon, proc, "web-css scroll 0", "web-css scroll 0%\n", 20)
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: 10em box 100 px (#717171 at 56/126), max-height 5em box 100 px (#242424 at 22/153), view() with "
        "the root's scroll-padding insets: none at 55%, #101010 at 60%, none at 100%, body #808080 at 20 px laid "
        "out, legacy overshoot #eee8aa")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot107-serial-1x.log")


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
        say("lot107 qemu ok in %.1fs" % (time.monotonic() - t0))
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
