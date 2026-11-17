#!/usr/bin/env python3
"""Lot 112 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot112/lh.html, web-css, reader clock 500 ms): line-height
now reaches the engine. html's line-height is 15px and its height
calc(26rlh + 10px) (400px), so the body's height: 50% is 200px. The
body's line-height calc(100% + 4px) is 20px: each paragraph is 20 px
high (10 px with normal), #r112's 2rlh is 30 px (html's 15px twice), so
the body's content ends at 1152 px and its range is 952 (634 in Lot
111). #i112 (scroll(nearest)) and #n112 (the body's timeline --b112)
take the body's progress, which follows the bar (y * 952 / 200);
#r112 (view(), top at 548, 30 px high) is in view from 348 to 578: it
tells the range. Before any click: #000000, no colour on #r112;
"web-bscroll 637 100" (476): #808080, #r112 (476 - 348) / 230 #8e8e8e;
"web-bscroll 637 120" (571): #999999, #r112 #f7f7f7; "web-bscroll 637
190" (904): #f2f2f2, #r112 past its range (no colour). With the Lot
111 range (634) #r112 could not be in view at these positions.
Node lines end with the line height passed to the engine: "lh 20"
(calc(100% + 4px) inherited), #r112 "lh 30" (2rlh), #u112 "lh 20"
(line-height: 3 then var(--u112), undefined: unset, so inherited),
#t112 "lh 18" (revert-layer to its layer), #k112 "opacity 0.4000"
(keyframes from var(--a) 0.2 to calc(var(--a) * 3), at 500 ms) and
"color #ff8080" (keyframes from var(--c) #ff0000 to #ffffff). The
counts: "color N 0" is 3 (#i112, #n112, #k112) plus #r112 when it is in
view, "opacity 1 0" the one opacity animation, "animated 1" (the
registered --m112), "keyframes 4" (k112, o112, c112, m112).
The second part of the lot, in three paragraphs that replace three
filler lines (each still 20 px high, so the geometry above is unchanged):
#g112 (18px, #0000ff) holds #f112 (font-size: 2ex, the x height of the
18px font the engine draws: scale 5, 6 px, so 12px; 11px with the 16 px
proportions; its color: var(--bad112) is unset, so #0000ff inherited,
not #00ff00) and #e112 (3ch: the 10 px advance, 30px; 27px before);
#h112 (font-size: calc(1lh + 2px), the body's 20 px line: 22px); #m112
(the registered --m112 animated from var(--c112) #00ff00 to #ffffff at
500 ms: #80ff80; #808080 when the var() frame was left out).
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot112-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot112-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot112-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot112-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot112-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["ikeep112", "nkeep112", "rkeep112", "ukeep112", "tkeep112", "kkeep112", "gkeep112", "fkeep112", "ekeep112",
        "hkeep112", "mkeep112"]
ANIM = "css-anim keyframes 4 animated 1 transitioned 0 font 0 at 500ms scroll 2 pos 0 color %d 0 opacity 1 0 view 1 need 2 geom 2 box 3\n"
FIXED = ["node p #u112 display auto lh 20\n", "node p #t112 display auto lh 18\n",
         "node p #k112 display auto color #ff8080 opacity 0.4000 lh 20\n",
         "node p #g112 display auto color #0000ff size 18px lh 20\n",
         "node span #f112 display auto color #0000ff size 12px lh 20\n",
         "node span #e112 display auto size 30px lh 20\n", "node p #h112 display auto size 22px lh 20\n",
         "node p #m112 display auto color #80ff80 lh 20\n"]
NEVER = ["lh 40", "lh normal", "node p #k112 display auto lh 20\n", "opacity 0.8000", "opacity 0.2000", "#808080 opacity",
         "#u112 display auto lh 15", "#f112 display auto color #00ff00", "size 11px", "size 27px",
         "#m112 display auto color #808080", "#h112 display auto lh"]


def say(msg):
    print("[lot112] %s" % msg, flush=True)


def node(i, c):
    return "node p #%s display auto color %s lh 20\n" % (i, c)


def css(mon, proc):
    return lw.run(mon, proc, "web-css", "css-anim ", 30)


def view(c):
    return ("node p #r112 display auto color %s lh 30\n" % c) if c else "node p #r112 display auto lh 30\n"


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-css time 500", "web-css time 500\n", 20)
    lw.run(mon, proc, "web-open %s/lot112/lh.html" % u, "osui web page sections", fail=["error="])
    out = css(mon, proc)
    read = lw.run(mon, proc, "web-read", "fin112", 30)
    need(read, KEEP, "web-read lh")
    need(out, [node("i112", "#000000"), node("n112", "#000000"), view(None), ANIM % 3] + FIXED, "web-css before")
    refuse(out, NEVER, "web-css before")
    steps = [(100, "#808080", "#8e8e8e", 4, "476"), (120, "#999999", "#f7f7f7", 4, "571"),
             (190, "#f2f2f2", None, 3, "904")]
    for y, c, v, nc, at in steps:
        lw.run(mon, proc, "web-bscroll 637 %d" % y, "osui web bscroll hit restyled", 30, fail=["error="])
        out = css(mon, proc)
        need(out, [node("i112", c), node("n112", c), view(v), ANIM % nc] + FIXED, "web-css at body " + at)
        refuse(out, NEVER, "web-css at body " + at)
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: line heights in the engine (20 px lines, 2rlh 30 px), body range 952: #808080 / #r112 #8e8e8e at "
        "476, #999999 / #f7f7f7 at 571, #f2f2f2 at 904; #u112 lh 20 (var() unset), #t112 lh 18 (revert-layer), "
        "#k112 opacity 0.4000 and #ff8080 (var() in keyframes); #f112 12px and #e112 30px (ex and ch of the "
        "engine's font), #h112 22px (lh in font-size), #f112 #0000ff (var() unset), #m112 #80ff80 (var() in a "
        "registered property's keyframes)")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot112-serial-1x.log")


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
        say("lot112 qemu ok in %.1fs" % (time.monotonic() - t0))
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
