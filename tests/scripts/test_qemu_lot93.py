#!/usr/bin/env python3
"""Lot 93 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

  * math functions in registered values (/lot93/math.html): a registered
    <angle> computed by atan2(1, 0) gives a 90deg gradient and acos(-1) a
    180deg one; its initial value atan2(1, 1) is 45deg, which the engine
    does not draw (the colour stays); the same atan2() in an unregistered
    property keeps its text and the gradient is refused. A registered
    <length> hypot(30px, 40px) is a 50px radial radius; calc(pow(2, 4) *
    1px) gives a 16px font size, round(calc(pi * 5px), 1px) 16px, the
    inherited initial value calc(sqrt(144) * 1px) 12px and calc(log(8, 2)
    * exp(0) * 7px) 21px; calc(tan(90deg) * 1px) is infinite and kept, so
    font-size does not take it. css-props counts six computed values.
No guest clock is involved (no -rtc option).
"""
from __future__ import print_function

import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot93-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot93-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot93-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot93-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot93-web-server.log")
need, refuse = l88.need, l88.refuse


def say(msg):
    print("[lot93] %s" % msg, flush=True)


def css_page(mon, proc, u, page, fin, keep, hide=()):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-open %s/lot93/%s" % (u, page), "osui web page sections", fail=["error="])
    css = lw.run(mon, proc, "web-css", "css-props ", 30)
    read = lw.run(mon, proc, "web-read", fin, 30)
    need(read, list(keep), "web-read " + page)
    refuse(read, list(hide), "web-read " + page)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    return css


def node_line(css, ident):
    m = re.search(r"node \w+ #%s [^\n]*\n" % ident, css)
    if not m:
        raise RuntimeError("no node line for #%s: %r" % (ident, css[-900:]))
    return m.group(0)


def step_math(mon, proc, u):
    css = css_page(mon, proc, u, "math.html", "fin93",
                   ["angkeep93", "acoskeep93", "initkeep93", "uangkeep93", "radkeep93", "powkeep93",
                    "pikeep93", "sqrtkeep93", "tankeep93", "logkeep93"])
    need(css, ["node div #ang93 display auto bgimg linear-gradient(90deg, #ff0000, #0000ff)\n",
               "node div #acos93 display auto bgimg linear-gradient(180deg, #00ff00, #000000) bgrep no-repeat\n",
               "node div #init93 display auto bg #123456\n",
               "node div #uang93 display auto bg #654321\n",
               "node div #rad93 display auto bgimg radial-gradient(circle 50px, #ff0000, #0000ff)\n",
               "node p #pow93 display auto size 16px\n",
               "node p #pi93 display auto size 16px\n",
               "node p #sqrt93 display auto size 12px\n",
               "node p #tan93 display auto\n",
               "node p #log93 display auto size 21px\n",
               "css-props registered 3 rejected 0 ", " simplified 6\n"], "web-css math")
    for ident in ("ang93", "acos93", "rad93"):
        refuse(node_line(css, ident), ["atan2(", "acos(", "hypot("], "web-css #" + ident)
    say("computed: atan2(1, 0) -> 90deg and acos(-1) -> 180deg drawn, initial atan2(1, 1) = 45deg not drawn, "
        "unregistered atan2() refused; hypot() -> radius 50px; pow(), pi, sqrt(), log() and exp() font sizes "
        "16/16/12/21px; tan(90deg) kept; simplified 6")


def boot_reader(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        u = "http://10.0.2.2:%d" % hp
        step_math(mon, proc, u)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot93-serial-1x.log")


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
        boot_reader(hp)
        say("lot93 qemu ok in %.1fs" % (time.monotonic() - t0))
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
