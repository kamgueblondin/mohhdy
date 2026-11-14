#!/usr/bin/env python3
"""Lot 97 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

  * the engine (web-render of the local page /web/lot97.html): a
    repeating linear gradient to the right with hard stops on 64x8 is
    painted per pixel run (4 rectangles), a repeating gradient whose
    period is 0 is painted in its average colour (1 rectangle), a
    repeating radial gradient in a vertical-rl box is painted in cells
    with its centre mirrored (Lot 96 refused it); every rectangle the
    guest prints is checked against scripts/lot97_gradmodel.py (with
    scripts/lot94_gradmodel.py and scripts/lot96_gradmodel.py);
  * the reader (/lot97/anim.html, web-css): the reader clock is set with
    "web-css time N" before the page is opened. At 500ms the animated
    registered colours give color #643200 (linear) and #006400 (steps(2),
    a quoted @keyframes name), an !important value keeps #010203, the
    transition from @starting-style gives the background #00009b, a
    registered 2em on a 20px element is inherited as 40px; the css-anim
    line counts them. At 2000ms the animations and the transition are
    over. Bad clock values print the usage.
No guest clock is involved (no -rtc option): the reader clock is the
value the shell sets.
"""
from __future__ import print_function

import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "..", "scripts"))
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot97-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot97-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot97-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot97-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)
import lot94_gradmodel as M  # noqa: E402
import lot96_gradmodel as M96  # noqa: E402
import lot97_gradmodel as M97  # noqa: E402

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot97-web-server.log")
need, refuse = l88.need, l88.refuse
KEEP = ["akeep97", "qkeep97", "ikeep97", "fkeep97", "spkeep97", "tkeep97", "skeep97"]


def say(msg):
    print("[lot97] %s" % msg, flush=True)


def expected_items():
    """the rectangles of /web/lot97.html from the models, top to bottom"""
    out = []
    o = (0, 0, 64, 8)
    st = [(0xFF0000, 0), (0xFF0000, 2500), (0x0000FF, 2500), (0x0000FF, 5000)]
    out += M97.axis_items(o, o, st, 90)
    o = (0, 8, 40, 10)
    out += M97.solid_items(o, o, M97.average([0xFF0000, 0x00FF00, 0x0000FF]))
    # the vertical-rl box at y 18: logical frame h x w, centre (5px, 10px)
    # mirrored, rows from the logical bottom
    w, h, y = 40, 20, 18
    o_l = (0, 0, h, w)
    st = [(0x000000, None), (0xFFFFFF, 3000)]
    cx, cy, rx, ry = M96.radial_radii(o_l, ("circle", None, [(9, 0)]), ((5, 0), (10, 0)))
    cy = w - cy
    pos = M.stop_positions(st)
    items = [cell + (M.colour_at(st, M96.repeat_p(pos, M96.radial_p(cx, cy, rx, ry, cell))),)
             for cell in M97.grid(o_l, o_l, rl=True)]
    out += [(x, yy + y, ww, hh, col) for (x, yy, ww, hh, col) in M97.rl_physical(items, w)]
    return ["web item rect %d %d %dx%d #%08x" % it for it in out]


def step_engine(mon, proc):
    out = lw.run(mon, proc, "web-render /web/lot97.html", "web-render ok", 60, fail=["error"])
    mark = "web-render /web/lot97.html"
    out = out[out.rfind(mark):] if mark in out else out
    got = [ln.strip().lower() for ln in out.splitlines() if ln.strip().startswith("web item rect ")]
    want = [w.lower() for w in expected_items()]
    if got != want:
        diff = [(i, a, b) for i, (a, b) in enumerate(zip(got, want)) if a != b][:5]
        raise RuntimeError("web-render lot97: %d rects, want %d; first differences %r" % (len(got), len(want), diff))
    refuse(out, ["refused"], "web-render lot97")
    say("engine: %d rectangles (4 axis runs, 1 average, rl repeating radial cells) as the models" % len(got))


def node_line(css, ident):
    m = re.search(r"node \w+ #%s [^\n]*\n" % ident, css)
    if not m:
        raise RuntimeError("no node line for #%s: %r" % (ident, css[-900:]))
    return m.group(0)


def at_time(mon, proc, u, ms):
    lw.run(mon, proc, "web-css time %d" % ms, "web-css time %d\n" % ms, 20)
    lw.run(mon, proc, "web-open %s/lot97/anim.html" % u, "osui web page sections", fail=["error="])
    return lw.run(mon, proc, "web-css", "css-props ", 30)


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    css = at_time(mon, proc, u, 500)
    read = lw.run(mon, proc, "web-read", "fin97", 30)
    need(read, KEEP, "web-read anim")
    need(css, ["node div #a97 display auto color #643200\n",
               "node div #q97 display auto color #006400\n",
               "node div #i97 display auto color #010203\n",
               "node span #sp97 display auto size 40px\n",
               "node div #t97 display auto bg #00009b\n",
               "css-props registered 3 rejected 0 ",
               "css-anim keyframes 2 animated 2 transitioned 1 font 1 at 500ms dropped 1\n"], "web-css at 500ms")
    refuse(node_line(css, "s97"), ["color"], "web-css #s97")
    css = at_time(mon, proc, u, 2000)
    need(css, ["node div #a97 display auto color #000000\n",
               "node div #q97 display auto color #000000\n",
               "node div #i97 display auto color #010203\n",
               "node span #sp97 display auto size 40px\n",
               "node div #t97 display auto bg #0000ff\n",
               "css-anim keyframes 2 animated 0 transitioned 0 font 1 at 2000ms dropped 1\n"], "web-css at 2000ms")
    for bad in ("time", "time x", "time 100000001", "time 5 x", "time -1"):
        lw.run(mon, proc, "web-css %s" % bad, "usage: web-css time <0..100000000>", 20)
    lw.run(mon, proc, "web-css time 100000000", "web-css time 100000000\n", 20)
    lw.run(mon, proc, "web-css time 0", "web-css time 0\n", 20)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("reader: at 500ms color #643200 (linear), #006400 (steps(2), quoted name), !important #010203, "
        "transition bg #00009b, 2em inherited as 40px; at 2000ms over; bad clock values refused")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_engine(mon, proc)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot97-serial-1x.log")


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
        say("lot97 qemu ok in %.1fs" % (time.monotonic() - t0))
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
