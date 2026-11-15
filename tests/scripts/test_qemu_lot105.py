#!/usr/bin/env python3
"""Lot 105 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot105/scroll.html, web-css, reader clock 500 ms): #o105 is
a box scroller (overflow-y: auto; height: 120px) the reader passes on
(style overflow-y:auto;height:120px, id="wgN") because timelines read it;
the layout hands back its own scroll state (we_geom_feed: content 486 px
in a 120 px scrollport, scroll range 366 px; the root scroll range 424
px). Every animation runs a keyframes colour (black to white, 1000ms
linear). #i105 on scroll(nearest), #j105 on the named scroll timeline
--s105 of the box with animation-range: 0px 100px, #v105 on view()
inside the box (block offset 268, 10 px high: cover [148, 278]), #c105
on scroll(nearest) in an overflow: hidden box (no effect), #r105 on
scroll(root).
At box offset 0: #i105 and #j105 #000000, #v105 none, #r105 #000000
(set position 0). "web-bscroll 630 28" (a click on the box scrollbar at
20/120 of its track) scrolls the box to 61 px and styles the page again:
#i105 #2a2a2a, #j105 #9c9c9c, #v105 none. "web-bscroll 630 68" (60/120:
183 px): #i105 #808080, #j105 over, #v105 #454545. "web-css scroll gui"
then "web-scroll 200": the root position is the GUI scroll offset,
#r105 #787878 (200 / 424), the box keeps its offset; "web-scroll 9999"
stops at 424, the end of the range (#r105 no effect). Then the field
q105 is given "keep105", the box is scrolled back (restyled) and
web-forms still shows the value (the reader form state is copied around
the pass). "web-css scroll 50"
leaves the GUI position: #r105 #808080 on the next page.
"which web-scroll" and "which web-bscroll" answer builtin.
The Lot 104 page is checked by make qemu-lot104.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot105-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot105-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot105-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot105-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot105-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["ikeep105", "jkeep105", "vkeep105", "ckeep105", "rkeep105"]
ANIM = "css-anim keyframes 1 animated 0 transitioned 0 font 0 at 500ms scroll 3 pos %s color %d 0 view 1 need %d geom 3 box 3%s\n"


def say(msg):
    print("[lot105] %s" % msg, flush=True)


def node(i, c=None):
    return "node p #%s display auto%s\n" % (i, " color %s" % c if c else "")


def css(mon, proc):
    return lw.run(mon, proc, "web-css", "css-anim ", 30)


def step_reader(mon, proc, u):
    # both commands are shell builtins (is_builtin: which, desktop tool windows)
    lw.run(mon, proc, "which web-scroll", "which ok builtin web-scroll\n", 20, fail=["bin/web-scroll"])
    lw.run(mon, proc, "which web-bscroll", "which ok builtin web-bscroll\n", 20, fail=["bin/web-bscroll"])
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-css time 500", "web-css time 500\n", 20)
    lw.run(mon, proc, "web-css scroll 0", "web-css scroll 0%\n", 20)
    lw.run(mon, proc, "web-open %s/lot105/scroll.html" % u, "osui web page sections", fail=["error="])
    read = lw.run(mon, proc, "web-read", "fin105", 30)
    need(read, KEEP, "web-read scroll")
    out = css(mon, proc)
    need(out, [node("i105", "#000000"), node("j105", "#000000"), node("v105"), node("c105"), node("r105", "#000000"),
               ANIM % ("0", 3, 3, "")], "web-css at box 0")
    refuse(out, ["#v105 display auto color", "#c105 display auto color"], "web-css at box 0")
    lw.run(mon, proc, "web-bscroll 630 28", "osui web bscroll hit restyled", 30, fail=["error="])
    out = css(mon, proc)
    need(out, [node("i105", "#2a2a2a"), node("j105", "#9c9c9c"), node("v105"), node("c105"), node("r105", "#000000"),
               ANIM % ("0", 3, 3, "")], "web-css at box 61")
    refuse(out, ["#v105 display auto color", "#c105 display auto color"], "web-css at box 61")
    lw.run(mon, proc, "web-bscroll 630 68", "osui web bscroll hit restyled", 30, fail=["error="])
    out = css(mon, proc)
    need(out, [node("i105", "#808080"), node("j105"), node("v105", "#454545"), node("c105"), node("r105", "#000000"),
               ANIM % ("0", 3, 3, "")], "web-css at box 183")
    refuse(out, ["#j105 display auto color", "#c105 display auto color"], "web-css at box 183")
    lw.run(mon, proc, "web-bscroll 1 1", "osui web bscroll none", 20)
    lw.run(mon, proc, "web-css scroll gui", "web-css scroll gui\n", 20)
    lw.run(mon, proc, "web-scroll 200", "osui web scroll 200 restyled", 30, fail=["error="])
    out = css(mon, proc)
    need(out, [node("i105", "#808080"), node("v105", "#454545"), node("r105", "#787878"),
               ANIM % ("gui", 3, 4, " gui 200")], "web-css at GUI 200")
    lw.run(mon, proc, "web-scroll 0", "osui web scroll 0 restyled", 30, fail=["error="])
    out = css(mon, proc)
    need(out, [node("r105", "#000000"), ANIM % ("gui", 3, 4, " gui 0")], "web-css at GUI 0")
    lw.run(mon, proc, "web-scroll 9999", "osui web scroll 424 restyled", 30, fail=["error="])
    out = css(mon, proc)
    need(out, [node("r105"), ANIM % ("gui", 2, 4, " gui 424")], "web-css at GUI 424")
    refuse(out, ["#r105 display auto color"], "web-css at GUI 424")
    lw.run(mon, proc, "web-scroll 0", "osui web scroll 0 restyled", 30, fail=["error="])
    lw.run(mon, proc, "web-input 0 keep105", "osui web input ok", 30)
    lw.run(mon, proc, "web-bscroll 630 28", "osui web bscroll hit restyled", 30, fail=["error="])
    need(lw.run(mon, proc, "web-forms", "name=q105", 30), ["name=q105 value=keep105"], "web-forms after restyle")
    lw.run(mon, proc, "web-css scroll 50", "web-css scroll 50%\n", 20)
    lw.run(mon, proc, "web-open %s/lot105/scroll.html" % u, "osui web page sections", fail=["error="])
    out = css(mon, proc)
    need(out, [node("r105", "#808080")], "web-css at 50%")
    refuse(out, ["pos gui", " gui "], "web-css at 50%")
    lw.run(mon, proc, "web-css scroll 0", "web-css scroll 0%\n", 20)
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: box scroller scroll(nearest) #2a2a2a then #808080, named box timeline 0px-100px #9c9c9c then over, "
        "view() in the box #454545, hidden box no effect, scroll(root) at the GUI offset #787878, a field kept")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot105-serial-1x.log")


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
        say("lot105 qemu ok in %.1fs" % (time.monotonic() - t0))
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
