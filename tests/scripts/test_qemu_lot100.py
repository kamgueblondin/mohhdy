#!/usr/bin/env python3
"""Lot 100 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot100/color.html, web-css): the reader clock is set with
"web-css time N" before the page is opened. hwb(120 30% 20%) is painted
as color #4dcc4d; color-mix(in srgb, red 20%, blue 20%) as bg #80008066
(the percentages sum to 40%, so the alpha is 0.4: 102 = 0x66, written
#rrggbbaa); RebeccaPurple as color #663399 (a CSS Color 4 name in any
case); darkorange in the background shorthand as bg #ff8c00. At 500ms a
keyframes colour (red to blue, 1000ms linear) gives color #800080 and a
transition from the @starting-style colour red to the own lime (1s
linear) gives color #808000; the css-anim line counts them with
"color 1 1". At 2000ms both are over (#ff0000 and #00ff00) and the
css-anim line has no colour part. The Lot 99 page is checked by make
qemu-lot99.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot100-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot100-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot100-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot100-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot100-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["ckeep100", "mkeep100", "nkeep100", "gkeep100", "kkeep100", "tkeep100"]


def say(msg):
    print("[lot100] %s" % msg, flush=True)


def at_time(mon, proc, u, ms):
    lw.run(mon, proc, "web-css time %d" % ms, "web-css time %d\n" % ms, 20)
    lw.run(mon, proc, "web-open %s/lot100/color.html" % u, "osui web page sections", fail=["error="])
    return lw.run(mon, proc, "web-css", "css-anim ", 30)


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    css = at_time(mon, proc, u, 500)
    read = lw.run(mon, proc, "web-read", "fin100", 30)
    need(read, KEEP, "web-read colours")
    need(css, ["node div #c100 display auto color #4dcc4d\n",
               "node div #m100 display auto bg #80008066\n",
               "node div #n100 display auto color #663399\n",
               "node div #g100 display auto bg #ff8c00\n",
               "node div #k100 display auto color #800080\n",
               "node div #t100 display auto color #808000\n",
               "css-anim keyframes 1 animated 0 transitioned 0 font 0 at 500ms color 1 1\n"], "web-css at 500ms")
    css = at_time(mon, proc, u, 2000)
    need(css, ["node div #m100 display auto bg #80008066\n",
               "node div #k100 display auto color #ff0000\n",
               "node div #t100 display auto color #00ff00\n",
               "css-anim keyframes 1 animated 0 transitioned 0 font 0 at 2000ms\n"], "web-css at 2000ms")
    refuse(css, [" color 1", " color 0"], "web-css at 2000ms")
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: hwb color #4dcc4d, color-mix bg #80008066, RebeccaPurple #663399, darkorange shorthand bg #ff8c00; "
        "at 500ms keyframes #800080, starting-style transition #808000, color 1 1; at 2000ms #ff0000 and #00ff00")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot100-serial-1x.log")


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
        say("lot100 qemu ok in %.1fs" % (time.monotonic() - t0))
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
