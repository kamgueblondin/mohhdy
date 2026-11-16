#!/usr/bin/env python3
"""Lot 108 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot108/x.html, web-css, reader clock 500 ms). Every
keyframes colour runs 1000ms. #o108 has overflow-x: auto and width:
300px: scroll(nearest inline) on #i108 reads it, so the reader passes
"overflow-x:auto;width:300px"; the engine scrolls it along x (the
nowrap line is 380 px wider than the 300 px scrollport) and paints a
horizontal bar at y 64..70 (x 8..308). #m108 has overflow: auto,
max-width: 15em, height: 2em and min-height: 5em with a named x
timeline (--x108) read by #n108: the reader passes "overflow-y:auto;
height:32px;min-height:80px;overflow-x:auto;max-width:240px"; the box is
80 px high (min-height wins), its bar at y 144..150 (x 8..248), range
440 px along x. Before any click both are #000000.
"web-bscroll 68 100" is where #m108's bar would be with height 32px
alone: none. "web-bscroll 158 66" scrolls #o108 to 380 * 150 / 300 =
190: #i108 at 190 / 380, #808080. "web-bscroll 68 146" scrolls #m108
to 440 * 60 / 240 = 110: #n108 at 110 / 440, #404040.
#v108, a row of #m108 on view(inline), is #808080 at offset 0 (its
inline offset 0 in a 240 px scrollport: (0 + 240) / 480) and #bababa at
110 ((110 + 240) / 480); without the x shift of the contents it stays
#808080 (refused).
Relative colours (CSS Color 5): #r108 rgb(from #336699 r g calc(b -
16)) is #336689; #h108 hsl(from rgb(from red b g r) calc(h - 120) s
calc(l - 10)) is hsl(120 100 40), #00cc00; #c108 runs keyframes from
rgb(from red r g b) to rgb(from red b g r): #800080 at 500 ms.
The Lot 107 page is checked by make qemu-lot107.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot108-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot108-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot108-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot108-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot108-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["ikeep108", "nkeep108", "rkeep108", "hkeep108", "ckeep108"]
ANIM = "css-anim keyframes 2 animated 0 transitioned 0 font 0 at 500ms scroll 2 pos 0 color 4 0 view 1 need 3 geom 3 box 3\n"
FIXED = ["node p #r108 display auto color #336689\n", "node p #h108 display auto color #00cc00\n",
         "node p #c108 display auto color #800080\n"]
NEVER = ["node p #r108 display auto\n", "node p #h108 display auto\n", "node p #c108 display auto\n"]


def say(msg):
    print("[lot108] %s" % msg, flush=True)


def vnode(c):
    return "node p #v108 display auto color %s\n" % c


def node(i, c):
    return "node p #%s display auto color %s ws-nowrap\n" % (i, c)


def css(mon, proc):
    return lw.run(mon, proc, "web-css", "css-anim ", 30)


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-css time 500", "web-css time 500\n", 20)
    lw.run(mon, proc, "web-open %s/lot108/x.html" % u, "osui web page sections", fail=["error="])
    out = css(mon, proc)
    read = lw.run(mon, proc, "web-read", "fin108", 30)
    need(read, KEEP, "web-read x")
    need(out, FIXED + [node("i108", "#000000"), node("n108", "#000000"), vnode("#808080"), ANIM],
         "web-css before")
    refuse(out, NEVER, "web-css before")
    lw.run(mon, proc, "web-bscroll 68 100", "osui web bscroll none", 30, fail=["error=", "bscroll hit"])
    lw.run(mon, proc, "web-bscroll 158 66", "osui web bscroll hit restyled", 30, fail=["error="])
    out = css(mon, proc)
    need(out, FIXED + [node("i108", "#808080"), node("n108", "#000000"), vnode("#808080"), ANIM],
         "web-css at #o108 190")
    lw.run(mon, proc, "web-bscroll 68 146", "osui web bscroll hit restyled", 30, fail=["error="])
    out = css(mon, proc)
    need(out, FIXED + [node("i108", "#808080"), node("n108", "#404040"), vnode("#bababa"), ANIM],
         "web-css at #m108 110")
    refuse(out, NEVER + [vnode("#808080")], "web-css after")
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: x box 300 px (#808080 at 190/380), max-width 15em / min-height 5em box 80 px high (#404040 at "
        "110/440, no bar at 32 px), view(inline) #808080 then #bababa, relative colours #336689, #00cc00, keyframes #800080")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot108-serial-1x.log")


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
        say("lot108 qemu ok in %.1fs" % (time.monotonic() - t0))
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
