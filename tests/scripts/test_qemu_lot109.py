#!/usr/bin/env python3
"""Lot 109 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot109/box.html, web-css, reader clock 500 ms). Every
keyframes colour runs 1000ms. Font metrics (16 px): ex 5/16 em, ch 8/16
em, lh 10/16 em. #o109 has overflow: auto and height: 20ex: the reader
passes "overflow-y:auto;height:100px", the engine paints its bar at y
44..144 (x 626..632), range 230 px. #p109 has height: 12lh (120 px,
y 144..264, range 286 px) read by a named timeline (--p109, #t109);
inside it #q109 has height: clamp(20%, calc(50% + 2ch), 90%): the
reader passes the expression "clamp(20%, calc(50% + 16px), 90%)" and the
engine resolves it against #p109's 120 px: 76 px (y 144..220, range
254 px), read by scroll(nearest) on #s109. Before any click #i109,
#s109 and #t109 are #000000.
"web-bscroll 629 200" scrolls #q109 to 254 * 56 / 76 = 187: #s109
#bcbcbc. "web-bscroll 629 250" scrolls #p109 to 286 * 106 / 120 = 252:
#t109 #e1e1e1. "web-bscroll 629 100" scrolls #o109 to 230 * 56 / 100 =
128: #i109 #8e8e8e.
Relative colours (CSS Color 5): #l109 lab(from #336699 l a b) is
#336699; #h109 lch(from #336699 l c calc(h + 180)) is #755e29; #w109
hwb(from #808080 h calc(w + 20) b) is #959595 (w + b over 100%
normalised); #x109 color(from red display-p3 r g b) is #ff0000; #u109
rgb(from color(display-p3 1 0 0) calc(r - 40) g b) keeps the origin's
unclamped channels: #ef0000 (#d70000 if clamped first); #c109 runs
keyframes from lab(from #000000 l a b) to lch(from #ffffff l c h):
#636363 at 500 ms (Oklab).
The Lot 108 page is checked by make qemu-lot108.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot109-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot109-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot109-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot109-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot109-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["ikeep109", "skeep109", "tkeep109", "lkeep109", "hkeep109", "wkeep109", "xkeep109", "ukeep109", "ckeep109"]
ANIM = "css-anim keyframes 2 animated 0 transitioned 0 font 0 at 500ms scroll 3 pos 0 color 4 0 view 0 need 3 geom 3 box 3\n"
FIXED = ["node p #l109 display auto color #336699\n", "node p #h109 display auto color #755e29\n",
         "node p #w109 display auto color #959595\n", "node p #x109 display auto color #ff0000\n",
         "node p #u109 display auto color #ef0000\n", "node p #c109 display auto color #636363\n"]
NEVER = ["node p #l109 display auto\n", "node p #h109 display auto\n", "node p #w109 display auto\n",
         "node p #x109 display auto\n", "node p #u109 display auto\n", "node p #c109 display auto\n",
         "node p #u109 display auto color #d70000\n"]


def say(msg):
    print("[lot109] %s" % msg, flush=True)


def node(i, c):
    return "node p #%s display auto color %s\n" % (i, c)


def css(mon, proc):
    return lw.run(mon, proc, "web-css", "css-anim ", 30)


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-css time 500", "web-css time 500\n", 20)
    lw.run(mon, proc, "web-open %s/lot109/box.html" % u, "osui web page sections", fail=["error="])
    out = css(mon, proc)
    read = lw.run(mon, proc, "web-read", "fin109", 30)
    need(read, KEEP, "web-read box")
    need(out, FIXED + [node("i109", "#000000"), node("s109", "#000000"), node("t109", "#000000"), ANIM],
         "web-css before")
    refuse(out, NEVER, "web-css before")
    lw.run(mon, proc, "web-bscroll 629 200", "osui web bscroll hit restyled", 30, fail=["error="])
    out = css(mon, proc)
    need(out, FIXED + [node("i109", "#000000"), node("s109", "#bcbcbc"), node("t109", "#000000"), ANIM],
         "web-css at #q109 187")
    lw.run(mon, proc, "web-bscroll 629 250", "osui web bscroll hit restyled", 30, fail=["error="])
    out = css(mon, proc)
    need(out, FIXED + [node("i109", "#000000"), node("s109", "#bcbcbc"), node("t109", "#e1e1e1"), ANIM],
         "web-css at #p109 252")
    lw.run(mon, proc, "web-bscroll 629 100", "osui web bscroll hit restyled", 30, fail=["error="])
    out = css(mon, proc)
    need(out, FIXED + [node("i109", "#8e8e8e"), node("s109", "#bcbcbc"), node("t109", "#e1e1e1"), ANIM],
         "web-css at #o109 128")
    refuse(out, NEVER, "web-css after")
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: 20ex box 100 px (#8e8e8e at 128/230), 12lh box 120 px (#e1e1e1 at 252/286), clamp() box 76 px "
        "inside it (#bcbcbc at 187/254), relative lab/lch/hwb/color() #336699, #755e29, #959595, #ff0000, "
        "unclamped origin #ef0000, keyframes #636363")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot109-serial-1x.log")


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
        say("lot109 qemu ok in %.1fs" % (time.monotonic() - t0))
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
