#!/usr/bin/env python3
"""Lot 94 in QEMU, offline. One boot at ui-scale 1x; the Lot 67 test
server runs on the host and the guest reaches it as 10.0.2.2 (slirp).

  * the engine (web-render of the local page /web/lot94.html): a 45deg
    linear gradient on 80x40, a gradient to the top right corner on 64x32
    and a conic gradient (from 90deg at 25% 75%, an angle stop) on 40x40
    are painted in cells; every rectangle the guest prints is checked
    against scripts/lot94_gradmodel.py (94 rectangles);
  * the reader (/lot94/grad.html, web-css): 0.7854rad is written as 45deg,
    "to bottom left" and the conic gradient keep their words, a registered
    angle calc(30deg + 15deg) is 45deg, -30.5deg is passed on, and a conic
    stop at 400deg refuses the gradient (the colour stays).
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
sys.path.insert(0, os.path.join(HERE, "..", "..", "scripts"))
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot94-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot94-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot94-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot94-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)
import lot94_gradmodel as M  # noqa: E402

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot94-web-server.log")
need, refuse = l88.need, l88.refuse


def say(msg):
    print("[lot94] %s" % msg, flush=True)


def expected_items():
    """the rectangles of /web/lot94.html from the model, top to bottom"""
    out = []
    o = (0, 0, 80, 40)
    st = [(0x000000, None), (0xFFFFFF, None)]
    out += [cell + (M.colour_at(st, M.linear_p(o, cell, angle=45)),) for cell in M.grid(o, o)]
    o = (0, 40, 64, 32)
    st = [(0xFF0000, None), (0x0000FF, None)]
    out += [cell + (M.colour_at(st, M.linear_p(o, cell, corner=("top", "right"))),) for cell in M.grid(o, o)]
    o = (0, 72, 40, 40)
    st = [(0x00FF00, None), (0x0000FF, M.conic_stop_pos(270)), (0xFF0000, None)]
    cx, cy = o[0] + o[2] * 25 // 100, o[1] + o[3] * 75 // 100
    out += [cell + (M.colour_at(st, M.conic_p(2 * cx, 2 * cy, cell, 90)),) for cell in M.grid(o, o)]
    return ["web item rect %d %d %dx%d #%08x" % it for it in out]


def step_engine(mon, proc):
    out = lw.run(mon, proc, "web-render /web/lot94.html", "web-render ok", 60, fail=["error"])
    mark = "web-render /web/lot94.html"
    out = out[out.rfind(mark):] if mark in out else out
    got = [ln.strip().lower() for ln in out.splitlines() if ln.strip().startswith("web item rect ")]
    want = [w.lower() for w in expected_items()]
    if got != want:
        diff = [(i, a, b) for i, (a, b) in enumerate(zip(got, want)) if a != b][:5]
        raise RuntimeError("web-render lot94: %d rects, want %d; first differences %r" % (len(got), len(want), diff))
    say("engine: %d cells (45deg, to top right, conic from 90deg at 25%% 75%%) as the model" % len(got))


def step_reader(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-open %s/lot94/grad.html" % u, "osui web page sections", fail=["error="])
    css = lw.run(mon, proc, "web-css", "css-props ", 30)
    read = lw.run(mon, proc, "web-read", "fin94", 30)
    need(read, ["linkeep94", "corkeep94", "conkeep94", "regkeep94", "badkeep94", "degkeep94"], "web-read grad")
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    need(css, ["node div #lin94 display auto bgimg linear-gradient(45deg, #ff0000, #0000ff)\n",
               "node div #cor94 display auto bgimg linear-gradient(to bottom left, #00ff00, #000000)\n",
               "node div #con94 display auto bgimg conic-gradient(from 30deg at 25% 75%, #ff0000, #0000ff 180deg, #00ff00)\n",
               "node div #reg94 display auto bgimg linear-gradient(45deg, #ff0000, #0000ff)\n",
               "node div #bad94 display auto bg #123456\n",
               "node div #deg94 display auto bgimg linear-gradient(-30.5deg, #ff0000 10%, #0000ff)\n",
               "css-props registered 1 rejected 0 "], "web-css grad")
    m = re.search(r"node div #reg94 [^\n]*\n", css)
    refuse(m.group(0) if m else "", ["calc(", "var("], "web-css #reg94")
    say("reader: 0.7854rad -> 45deg, to bottom left, conic from/at/angle stop, registered 45deg, -30.5deg passed; "
        "conic stop 400deg refused")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_engine(mon, proc)
        step_reader(mon, proc, "http://10.0.2.2:%d" % hp)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot94-serial-1x.log")


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
        say("lot94 qemu ok in %.1fs" % (time.monotonic() - t0))
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
