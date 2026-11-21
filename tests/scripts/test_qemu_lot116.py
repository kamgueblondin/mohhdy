#!/usr/bin/env python3
"""Lot 116 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot116/lh.html, web-css, reader clock 500 ms): html's
line-height is 15px and its height calc(26rlh + 10px) (400px), so the
body's height: 50% is 200px. The body's line-height is the number 1.25,
passed to the engine as the number ("lh num 1.25"), which the engine
multiplies by each element's own font: the paragraphs at 16 px are 20 px
high, h1 (32 px) 40, #b116 (24 px) 30, #e116 (font-size: 2rcap, the
root's capitals: 7 rows at 16 px, 14px) 17. #q116 (line-height 20.5px,
two lines) is 41 px high (the fraction carried: 20 + 21), #w116 (45px)
45 px (past the old 40), #c116 (line-height: 4cap, 28px) 28 px. #d116's
height max(5% + 10px, 10%, 2% + 30px, 1rlh + 25px, 25% - 5px) keeps five
arguments no one of which covers another (past the Lot 112 four) and is
45 px (25% of the body's 200 less 5); #v116's height: 30px then
var(--x116), undefined: unset, so auto (36 px, its paragraph and
margins) and a box scroller with no range. The body's content ends at
1090 px: its range is 890. #i116 (scroll(nearest)) and #n116 (the body's
timeline --b116) take the body's progress, which follows the bar (y *
890 / 200); #r116 (view(), top at 818, 20 px high) is in view from 618
to 838. Before any click: #000000, no colour on #r116; "web-bscroll 637
100" (445): #808080, #r116 not yet in view; "web-bscroll 637 150"
(667): #bfbfbf, #r116 (667 - 618) / 220 #393939; "web-bscroll 637 175"
(778): #dfdfdf, #r116 #b9b9b9; "web-bscroll 637 190" (845): #f2f2f2,
#r116 past its range (no colour). With Lot 112's line heights (px, cut
within 10..40, cap and rcap not read, the five-argument max() lost,
#v116 30 px) #r116 would sit elsewhere and the range would differ.
#dd116 (in #d116, range 0) and #vv116 (in #v116, range 0) take no
colour. The counts: "color N 0" is 2 (#i116, #n116) plus #r116 when it
is in view, "need 4 geom 4" the four timeline subjects, "box 3" the
body, #d116 and #v116. No guest clock is involved (no -rtc option).
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot116-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot116-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot116-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot116-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot116-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["ikeep116", "nkeep116", "bkeep116", "qkeep116", "qline116", "wkeep116", "ckeep116", "ekeep116", "dkeep116",
        "vkeep116", "rkeep116", "fin116"]
ANIM = "css-anim keyframes 1 animated 0 transitioned 0 font 0 at 500ms scroll 2 pos 0 color %d 0 view 1 need 4 geom 4 box 3\n"
FIXED = ["node body display auto lh num 1.25\n", "node h1 display auto lh num 1.25\n",
         "node p #b116 display auto size 24px lh num 1.25\n", "node p #q116 display auto lh 20.5\n",
         "node br display auto lh 20.5\n", "node p #w116 display auto lh 45\n", "node p #c116 display auto lh 28\n",
         "node p #e116 display auto size 14px lh num 1.25\n", "node div #d116 display auto lh num 1.25\n",
         "node p #dd116 display auto lh num 1.25\n", "node div #v116 display auto lh num 1.25\n",
         "node p #vv116 display auto lh num 1.25\n"]
NEVER = ["lh 40\n", "lh 20\n", "lh 41", "lh 20.50", "lh normal", "size 16px", "#vv116 display auto color",
         "#dd116 display auto color", "#c116 display auto lh num"]


def say(msg):
    print("[lot116] %s" % msg, flush=True)


def node(i, c):
    return "node p #%s display auto color %s lh num 1.25\n" % (i, c)


def css(mon, proc):
    return lw.run(mon, proc, "web-css", "css-anim ", 30)


def view(c):
    return ("node p #r116 display auto color %s lh num 1.25\n" % c) if c else "node p #r116 display auto lh num 1.25\n"


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-css time 500", "web-css time 500\n", 20)
    lw.run(mon, proc, "web-open %s/lot116/lh.html" % u, "osui web page sections", fail=["error="])
    out = css(mon, proc)
    read = lw.run(mon, proc, "web-read", "fin116", 30)
    need(read, KEEP, "web-read lh")
    need(out, [node("i116", "#000000"), node("n116", "#000000"), view(None), ANIM % 2] + FIXED, "web-css before")
    refuse(out, NEVER, "web-css before")
    steps = [(100, "#808080", None, 2, "445"), (150, "#bfbfbf", "#393939", 3, "667"),
             (175, "#dfdfdf", "#b9b9b9", 3, "778"), (190, "#f2f2f2", None, 2, "845")]
    for y, c, v, nc, at in steps:
        lw.run(mon, proc, "web-bscroll 637 %d" % y, "osui web bscroll hit restyled", 30, fail=["error="])
        out = css(mon, proc)
        need(out, [node("i116", c), node("n116", c), view(v), ANIM % nc] + FIXED, "web-css at body " + at)
        refuse(out, NEVER, "web-css at body " + at)
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: line-height 1.25 passed as a number (20, 30, 40 and 17 px lines), 20.5px (41 px, two lines), "
        "45px, 4cap 28px, 2rcap 14px, a five-argument max() 45 px, var() unset height auto; body range 890: "
        "#808080 at 445, #bfbfbf / #r116 #393939 at 667, #dfdfdf / #b9b9b9 at 778, #f2f2f2 at 845")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot116-serial-1x.log")


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
        say("lot116 qemu ok in %.1fs" % (time.monotonic() - t0))
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
