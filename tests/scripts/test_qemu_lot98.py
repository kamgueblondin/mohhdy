#!/usr/bin/env python3
"""Lot 98 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot98/anim.html, web-css): the reader clock is set with
"web-css time N" before the page is opened. At 500ms an animation set by
the longhands animation-name, -duration and -timing-function from
@keyframes declared inside @scope gives color #643200; the same keyframes
with animation-composition: add on rgb(0, 0, 100) give #643264; a
transition set by the transition longhands between hsl(0 100% 50%) and
hsl(240, 100%, 50%) gives the background #800080 (hsl() is interpolated
in sRGB, Lot 97 switched at one half); calc(2ex + 1ch) on a 16px element
is 18px (ex is 5/16 em, ch 1/2 em, from the engine font); an @property
inside @scope is not taken (registered 3); the css-anim line counts them
with "composed 1". At 2000ms the animations and the transition are over
(Lot 99, converted: the hsl() end value is now a reader colour, bg
#0000ff; Lot 98 refused bg there). The Lot 97 page is
checked by make qemu-lot97.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot98-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot98-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot98-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot98-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot98-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["akeep98", "bkeep98", "hkeep98", "fkeep98", "spkeep98"]


def say(msg):
    print("[lot98] %s" % msg, flush=True)


def at_time(mon, proc, u, ms):
    lw.run(mon, proc, "web-css time %d" % ms, "web-css time %d\n" % ms, 20)
    lw.run(mon, proc, "web-open %s/lot98/anim.html" % u, "osui web page sections", fail=["error="])
    return lw.run(mon, proc, "web-css", "css-props ", 30)


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    css = at_time(mon, proc, u, 500)
    read = lw.run(mon, proc, "web-read", "fin98", 30)
    need(read, KEEP, "web-read longhands")
    need(css, ["node div #a98 display auto color #643200\n",
               "node div #b98 display auto color #643264\n",
               "node div #h98 display auto bg #800080\n",
               "node span #sp98 display auto size 18px\n",
               "css-props registered 3 rejected 0 ",
               "css-anim keyframes 1 animated 2 transitioned 1 font 1 at 500ms composed 1\n"], "web-css at 500ms")
    css = at_time(mon, proc, u, 2000)
    need(css, ["node div #a98 display auto color #000000\n",
               "node div #b98 display auto color #000064\n",
               "node span #sp98 display auto size 18px\n",
               "node div #h98 display auto bg #0000ff\n",
               "css-anim keyframes 1 animated 0 transitioned 0 font 1 at 2000ms\n"], "web-css at 2000ms")
    refuse(css, ["composed"], "web-css at 2000ms")
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: at 500ms longhands from @scope keyframes #643200, composition add #643264, hsl transition "
        "bg #800080, calc(2ex + 1ch) 18px, composed 1; at 2000ms over (hsl end bg #0000ff)")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot98-serial-1x.log")


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
        say("lot98 qemu ok in %.1fs" % (time.monotonic() - t0))
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
