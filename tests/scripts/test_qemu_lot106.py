#!/usr/bin/env python3
"""Lot 106 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot106/body.html, web-css, reader clock 500 ms): body
defines the view timeline --b106 and html --h106. The page is laid out
once (no mark: the reader view writes html and body bare) and the layout
hands the body box back in its own record (we_geom_feed: 640x480
viewport, body at block offset 8, 946 px high, root scroll range 482
px); the reader styles the page again with it. #b106 runs black to white
(1000ms linear) on --b106: cover [8 - 480, 954], 1426 px; at "web-css
scroll 0" 472 / 1426 is 330 ms, #545454; at 50% (241 px) exactly half,
#808080; at 100% (482 px) 669 ms, #ababab. #h106 runs on --h106: the
root element has no ancestor scroll container, the timeline is inactive
and asks for no geometry (no colour; "need 1"). #o106 runs color(srgb 1
0 0) to blue at 500 ms: not a legacy pair, so Oklab (CSS Color 4 12.1),
#8c53a2 (sRGB would give #800080); #s106 runs red to blue, a legacy
pair, #800080. The css-anim line ends with "view 1 need 1 geom 1".
The Lot 105 page is checked by make qemu-lot105.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot106-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot106-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot106-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot106-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot106-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["bkeep106", "hkeep106", "okeep106", "skeep106"]
ANIM = "css-anim keyframes 3 animated 0 transitioned 0 font 0 at 500ms color 3 0 view 1 need 1 geom 1\n"
FIXED = ["node div #h106 display auto\n", "node div #o106 display auto color #8c53a2\n",
         "node div #s106 display auto color #800080\n", ANIM]
NEVER = ["node div #h106 display auto color", "node div #o106 display auto color #800080",
         "node body display auto color", "id=\"wg"]


def say(msg):
    print("[lot106] %s" % msg, flush=True)


def at_scroll(mon, proc, u, pct):
    lw.run(mon, proc, "web-css scroll %d" % pct, "web-css scroll %d%%\n" % pct, 20)
    lw.run(mon, proc, "web-open %s/lot106/body.html" % u, "osui web page sections", fail=["error="])
    return lw.run(mon, proc, "web-css", "css-anim ", 30)


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-css time 500", "web-css time 500\n", 20)
    css = at_scroll(mon, proc, u, 0)
    read = lw.run(mon, proc, "web-read", "fin106", 30)
    need(read, KEEP, "web-read body")
    need(css, FIXED + ["node div #b106 display auto color #545454\n"], "web-css at 0%")
    refuse(css, NEVER, "web-css at 0%")
    css = at_scroll(mon, proc, u, 50)
    need(css, FIXED + ["node div #b106 display auto color #808080\n"], "web-css at 50%")
    refuse(css, NEVER + ["node div #b106 display auto color #545454"], "web-css at 50%")
    css = at_scroll(mon, proc, u, 100)
    need(css, FIXED + ["node div #b106 display auto color #ababab\n"], "web-css at 100%")
    refuse(css, NEVER + ["node div #b106 display auto color #808080"], "web-css at 100%")
    lw.run(mon, proc, "web-css scroll 0", "web-css scroll 0%\n", 20)
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: body view timeline #545454, #808080, #ababab at 0, 50, 100%; html view timeline inactive; "
        "color() keyframes in Oklab #8c53a2, legacy keyframes in sRGB #800080")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot106-serial-1x.log")


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
        say("lot106 qemu ok in %.1fs" % (time.monotonic() - t0))
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
