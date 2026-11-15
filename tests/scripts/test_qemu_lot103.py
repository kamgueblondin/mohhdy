#!/usr/bin/env python3
"""Lot 103 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

The reader (/lot103/gamut.html, web-css) at reader times 500 and 2000 ms:
color(display-p3 1 0 0) is gamut-mapped (CSS Color 4 13.2.2) to color
#ff0b0c (clipping would give #ff0000); oklch(0.7 0.4 30) to bg #ff5843;
color-mix(in oklch, red, blue) paints #b700be (in srgb #800080);
color-mix(in hsl longer hue, red, blue) the hue 120, #00ff00 (shorter:
#ff00ff); color-mix(in lch, lch(50 40 none), lch(70 60 240)) carries the
hue 240: bg #009fda; color-mix(in hsl, hsl(none 50% 50%), hsl(120 100%
50%)) carries the hue 120: #20df20. color-mix(in srgb longer hue, ...) is
invalid, #b103 has no colour. #a103 runs a keyframes colour from
display-p3 green to black: Lot 106 (converted, was #007e14 in sRGB from
the mapped #00fb29): color() is not a legacy form, so the interpolation
is in Oklab (CSS Color 4 12.1), #006300 at 500 ms; no colour at 2000 ms. The Lot 102
page is checked by make qemu-lot102.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot103-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot103-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot103-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot103-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot103-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["gkeep103", "okeep103", "mkeep103", "lkeep103", "nkeep103", "hkeep103", "bkeep103", "akeep103"]
STATIC = ["node div #g103 display auto color #ff0b0c\n",
          "node div #o103 display auto bg #ff5843\n",
          "node div #m103 display auto color #b700be\n",
          "node div #l103 display auto color #00ff00\n",
          "node div #n103 display auto bg #009fda\n",
          "node div #h103 display auto color #20df20\n",
          "node div #b103 display auto\n"]


def say(msg):
    print("[lot103] %s" % msg, flush=True)


def at_time(mon, proc, u, ms):
    lw.run(mon, proc, "web-css time %d" % ms, "web-css time %d\n" % ms, 20)
    lw.run(mon, proc, "web-open %s/lot103/gamut.html" % u, "osui web page sections", fail=["error="])
    return lw.run(mon, proc, "web-css", "css-anim ", 30)


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    css = at_time(mon, proc, u, 500)
    read = lw.run(mon, proc, "web-read", "fin103", 30)
    need(read, KEEP, "web-read gamut")
    need(css, STATIC + ["node div #a103 display auto color #006300\n",
                        "css-anim keyframes 1 animated 0 transitioned 0 font 0 at 500ms color 1 0\n"],
         "web-css at 500 ms")
    refuse(css, ["node div #g103 display auto color #ff0000", "node div #m103 display auto color #800080",
                 "node div #b103 display auto color", "node div #a103 display auto color #007e14"], "web-css at 500 ms")
    css = at_time(mon, proc, u, 2000)
    need(css, STATIC + ["node div #a103 display auto\n",
                        "css-anim keyframes 1 animated 0 transitioned 0 font 0 at 2000ms\n"], "web-css at 2000 ms")
    refuse(css, ["node div #a103 display auto color"], "web-css at 2000 ms")
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: p3 red #ff0b0c, oklch bg #ff5843, oklch mix #b700be, hsl longer #00ff00, lch none hue bg #009fda, "
        "hsl none hue #20df20, srgb longer invalid; keyframes from p3 green in Oklab #006300 then over")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot103-serial-1x.log")


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
        say("lot103 qemu ok in %.1fs" % (time.monotonic() - t0))
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
