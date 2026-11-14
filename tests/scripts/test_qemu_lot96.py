#!/usr/bin/env python3
"""Lot 96 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

  * the engine (web-render of the local page /web/lot96.html): a linear
    gradient at 30.25deg on 80x40 (cells at a fractional angle), a
    repeating linear gradient to the right on 64x16 (32 columns) and a
    repeating radial gradient (circle 10px at 25% 50%) on 40x40 are
    painted in cells; every rectangle the guest prints is checked against
    scripts/lot96_gradmodel.py and scripts/lot94_gradmodel.py (94
    rectangles);
  * the reader (/lot96/grad.html, web-css): repeating gradients are passed
    on (1rad as 57.3deg, a radian conic "from" as 28.65deg, a 45.5deg
    stop), a period of 0 refuses the gradient (the colour stays), a
    registered <image> whose stops the reader refuses gives its initial
    value, a conic gradient with stops at 0.25deg and 359.75deg is kept.
No guest clock is involved (no -rtc option).
"""
from __future__ import print_function

import os
import re
import subprocess
import sys
import time
from fractions import Fraction as F

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "..", "scripts"))
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot96-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot96-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot96-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot96-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)
import lot94_gradmodel as M  # noqa: E402
import lot96_gradmodel as M96  # noqa: E402

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot96-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["rlkeep96", "rckeep96", "rrkeep96", "flatkeep96", "imgkeep96", "degkeep96"]


def say(msg):
    print("[lot96] %s" % msg, flush=True)


def expected_items():
    """the rectangles of /web/lot96.html from the models, top to bottom"""
    out = []
    o = (0, 0, 80, 40)
    st = [(0x000000, None), (0xFFFFFF, None)]
    out += [cell + (M.colour_at(st, M.linear_p(o, cell, angle=F(3025, 100))),) for cell in M.grid(o, o)]
    o = (0, 40, 64, 16)
    st = [(0xFF0000, None), (0x0000FF, 2500)]
    pos = M.stop_positions(st)
    out += [cell + (M.colour_at(st, M96.repeat_p(pos, M.linear_p(o, cell, angle=90))),)
            for cell in M96.grid_axis(o, o, False)]
    o = (0, 56, 40, 40)
    st = [(0x00FF00, None), (0x000000, 5000)]
    pos = M.stop_positions(st)
    cx, cy, rx, ry = M96.radial_radii(o, ("circle", None, [(10, 0)]), ((25, 1), (50, 1)))
    out += [cell + (M.colour_at(st, M96.repeat_p(pos, M96.radial_p(cx, cy, rx, ry, cell))),) for cell in M.grid(o, o)]
    return ["web item rect %d %d %dx%d #%08x" % it for it in out]


def step_engine(mon, proc):
    out = lw.run(mon, proc, "web-render /web/lot96.html", "web-render ok", 60, fail=["error"])
    mark = "web-render /web/lot96.html"
    out = out[out.rfind(mark):] if mark in out else out
    got = [ln.strip().lower() for ln in out.splitlines() if ln.strip().startswith("web item rect ")]
    want = [w.lower() for w in expected_items()]
    if got != want:
        diff = [(i, a, b) for i, (a, b) in enumerate(zip(got, want)) if a != b][:5]
        raise RuntimeError("web-render lot96: %d rects, want %d; first differences %r" % (len(got), len(want), diff))
    say("engine: %d cells (30.25deg, repeating to right, repeating circle 10px at 25%% 50%%) as the models" % len(got))


def node_line(css, ident):
    m = re.search(r"node \w+ #%s [^\n]*\n" % ident, css)
    if not m:
        raise RuntimeError("no node line for #%s: %r" % (ident, css[-900:]))
    return m.group(0)


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-open %s/lot96/grad.html" % u, "osui web page sections", fail=["error="])
    css = lw.run(mon, proc, "web-css", "css-props ", 30)
    read = lw.run(mon, proc, "web-read", "fin96", 30)
    need(read, KEEP, "web-read grad")
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    need(css, ["node div #rl96 display auto bgimg repeating-linear-gradient(57.3deg, #ff0000, #0000ff 20%)\n",
               "node div #rc96 display auto bgimg repeating-conic-gradient(from 28.65deg at 25% 75%, #ff0000, #0000ff 45.5deg)\n",
               "node div #rr96 display auto bgimg repeating-radial-gradient(circle 10px, #00ff00, #000000 50%)\n",
               "node div #flat96 display auto bg #123456\n",
               "node div #img96 display auto bgimg linear-gradient(#ff0000, #0000ff)\n",
               "node div #deg96 display auto bgimg conic-gradient(#ff0000 0.25deg, #0000ff 359.75deg)\n",
               "css-props registered 1 rejected 0 "], "web-css grad")
    refuse(node_line(css, "rl96"), ["rad,", "1rad"], "web-css #rl96")
    refuse(node_line(css, "rc96"), ["0.5rad", "Repeating"], "web-css #rc96")
    refuse(node_line(css, "img96"), ["10%", "var("], "web-css #img96")
    say("reader: repeating linear/conic/radial passed on (1rad -> 57.3deg, from 0.5rad -> 28.65deg, 45.5deg stop); "
        "a period of 0 refused; a registered <image> with a period of 0 -> its initial value; 0.25deg and 359.75deg stops")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_engine(mon, proc)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot96-serial-1x.log")


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
        say("lot96 qemu ok in %.1fs" % (time.monotonic() - t0))
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
