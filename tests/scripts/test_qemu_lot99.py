#!/usr/bin/env python3
"""Lot 99 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot99/anim.html, web-css): the reader clock is set with
"web-css time N" before the page is opened. hsl(0.5turn 100% 50%) is
painted as color #00ffff and hsl(200grad 100% 25%) in the background
shorthand as bg #008080 (hue units, hsl() as a reader colour); a
registered <color> whose value is hsl(0.25turn 100% 50%), read through
var(), gives bg #80ff00. At 500ms an animation with only a 100% keyframe
(20px) and animation-composition: add on an element whose own value is
10px gives 25px to a child's font-size: the generated 0% keyframe is the
own value added on the value below (20px), 100% is 30px (CSS Animations
2; Lot 98 gave 20px). An animation with animation-timeline: none has no
effect (color #0000c8) and is counted as idle. The css-anim line counts
them with "composed 1 idle 1". At 2000ms the animation is over (10px)
and the idle one is still idle. The Lot 98 page is checked by make
qemu-lot98.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot99-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot99-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot99-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot99-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot99-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["ckeep99", "gkeep99", "akeep99", "spkeep99", "nkeep99", "hkeep99"]


def say(msg):
    print("[lot99] %s" % msg, flush=True)


def at_time(mon, proc, u, ms):
    lw.run(mon, proc, "web-css time %d" % ms, "web-css time %d\n" % ms, 20)
    lw.run(mon, proc, "web-open %s/lot99/anim.html" % u, "osui web page sections", fail=["error="])
    return lw.run(mon, proc, "web-css", "css-props ", 30)


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    css = at_time(mon, proc, u, 500)
    read = lw.run(mon, proc, "web-read", "fin99", 30)
    need(read, KEEP, "web-read composition")
    need(css, ["node div #c99 display auto color #00ffff\n",
               "node div #g99 display auto bg #008080\n",
               "node span #sp99 display auto size 25px\n",
               "node div #n99 display auto color #0000c8\n",
               "node div #h99 display auto bg #80ff00\n",
               "css-props registered 3 rejected 0 ",
               "css-anim keyframes 2 animated 1 transitioned 0 font 0 at 500ms composed 1 idle 1\n"], "web-css at 500ms")
    css = at_time(mon, proc, u, 2000)
    need(css, ["node div #c99 display auto color #00ffff\n",
               "node span #sp99 display auto size 10px\n",
               "node div #n99 display auto color #0000c8\n",
               "css-anim keyframes 2 animated 0 transitioned 0 font 0 at 2000ms idle 1\n"], "web-css at 2000ms")
    refuse(css, ["composed"], "web-css at 2000ms")
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: hsl turn color #00ffff, grad bg #008080, var hsl bg #80ff00; at 500ms generated keyframe "
        "composed 25px, timeline none idle (#0000c8), composed 1 idle 1; at 2000ms 10px")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot99-serial-1x.log")


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
        say("lot99 qemu ok in %.1fs" % (time.monotonic() - t0))
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
