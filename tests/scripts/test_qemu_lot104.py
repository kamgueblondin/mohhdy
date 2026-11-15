#!/usr/bin/env python3
"""Lot 104 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot104/view.html, web-css, reader clock 500 ms): the page is
laid out once with id="wgN" on its four view timeline subjects, the
layout hands their boxes back (we_geom_feed: 640x480 viewport, root
scroll range 976 px, subjects at block offsets 52, 390, 728 and 1066,
10 px high) and the reader styles the page again with that geometry.
Every animation runs a keyframes colour (black to white, 1000ms linear);
"web-css scroll N" sets the root scroll position (N% of 976 px).
#a104 on view() (cover [-428, 62]): #dfdfdf at 0%, over after.
#e104 on view() with animation-range: entry 0% exit 0% ([-90, 390]):
#303030 at 0%, over at 50%. #n104 defines --v104 (view-timeline-name,
-axis block, -inset 0px 100px) and runs on it with animation-range:
contain ([358, 728]): before at 0%, #5a5a5a at 50% (488 px), over at
75% (732 px). #l104 on view() with animation-range: cover 0px cover
200px ([586, 786]): #bababa at 75%, not yet at 50%. #d104 has
view(diagonal), which is invalid, so it keeps the document timeline:
#808080 at 500 ms. The css-anim line ends with "view 4 need 4 geom 4".
The Lot 103 page is checked by make qemu-lot103.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot104-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot104-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot104-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot104-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot104-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["akeep104", "ekeep104", "nkeep104", "lkeep104", "dkeep104"]
ANIM = "css-anim keyframes 1 animated 0 transitioned 0 font 0 at 500ms color %d 0 view 4 need 4 geom 4\n"
DOC = "node div #d104 display auto color #808080\n"


def say(msg):
    print("[lot104] %s" % msg, flush=True)


def at_scroll(mon, proc, u, pct):
    lw.run(mon, proc, "web-css scroll %d" % pct, "web-css scroll %d%%\n" % pct, 20)
    lw.run(mon, proc, "web-open %s/lot104/view.html" % u, "osui web page sections", fail=["error="])
    return lw.run(mon, proc, "web-css", "css-anim ", 30)


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-css time 500", "web-css time 500\n", 20)
    css = at_scroll(mon, proc, u, 0)
    read = lw.run(mon, proc, "web-read", "fin104", 30)
    need(read, KEEP, "web-read view")
    need(css, [DOC, "node div #a104 display auto color #dfdfdf\n", "node div #e104 display auto color #303030\n",
               "node div #n104 display auto\n", "node div #l104 display auto\n", ANIM % 3], "web-css at 0%")
    refuse(css, ["node div #n104 display auto color", "node div #l104 display auto color"], "web-css at 0%")
    css = at_scroll(mon, proc, u, 50)
    need(css, [DOC, "node div #a104 display auto\n", "node div #e104 display auto\n",
               "node div #n104 display auto color #5a5a5a\n", "node div #l104 display auto\n", ANIM % 2],
         "web-css at 50%")
    refuse(css, ["node div #a104 display auto color", "node div #e104 display auto color",
                 "node div #l104 display auto color"], "web-css at 50%")
    css = at_scroll(mon, proc, u, 75)
    need(css, [DOC, "node div #a104 display auto\n", "node div #e104 display auto\n",
               "node div #n104 display auto\n", "node div #l104 display auto color #bababa\n", ANIM % 2],
         "web-css at 75%")
    refuse(css, ["node div #a104 display auto color", "node div #e104 display auto color",
                 "node div #n104 display auto color"], "web-css at 75%")
    lw.run(mon, proc, "web-css scroll 0", "web-css scroll 0%\n", 20)
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: view() #dfdfdf, entry 0%-exit 0% #303030 then over, named view timeline with insets contain "
        "#5a5a5a at 50%, cover 0px-200px #bababa at 75%, view(diagonal) keeps the document timeline #808080")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot104-serial-1x.log")


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
        say("lot104 qemu ok in %.1fs" % (time.monotonic() - t0))
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
