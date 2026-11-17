#!/usr/bin/env python3
"""Lot 113 in QEMU, offline: the JavaScript subset (US-031). One boot at
ui-scale 1x; the Lot 67 test server runs on the host and the guest
reaches it as 10.0.2.2 (slirp). Everything is typed on the GUI input
line, which runs the same osui_gui_web_command code as the console.

/lot113/js.html is built by its inline scripts. With web-js off (the
default) the page is the static one: "static113" in the reader, none of
the script text. With web-js on the same page shows what the scripts
made: the h1 "dynamic113 1+4+9" (an arrow function in map), three list
items made with createElement, "written113 {"n":42}" (document.write and
JSON), "ready113 3" (a DOMContentLoaded listener and querySelectorAll),
"timer113 20" (a timer and a regex capture). The second script throws a
ReferenceError after it registered both: the page pass reports it and
the rest still runs. web-js status counts 2 scripts, 1 ran to the end, 1
failed; web-js log has the page's console line and the error. Then the
REPL: "js 2**5+10" answers 42 and a variable set by one js line is seen
by the next one (one realm). The page pass and the REPL run in Ring 3 in
the shell; no guest clock is involved (no -rtc option).
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot113-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot113-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot113-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot113-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot113-web-server.log")
need, refuse = l88.need, l88.refuse
# the JavaScript lines need a few more keys than the earlier lots typed
lw.ALIASES.update({"*": "shift-8", "+": "shift-equal", "[": "bracket_left", "]": "bracket_right", ",": "comma"})

SCRIPTED = ["dynamic113 1+4+9", "1. alpha113", "2. beta113", "3. gamma113", "written113 {\"n\":42}", "ready113 3",
            "timer113 20", "fin113"]
SCRIPT_TEXT = ["items.forEach", "missing113", "createElement"]


def say(msg):
    print("[lot113] %s" % msg, flush=True)


def step_pages(mon, proc, u):
    out = lw.run(mon, proc, "web-js", "osui web-js ", 20)
    need(out, ["osui web-js off pages 0"], "web-js default")
    lw.run(mon, proc, "web-open %s/lot113/js.html" % u, "osui web page sections", fail=["error="])
    read = lw.run(mon, proc, "web-read", "fin113", 30)
    need(read, ["static113"], "web-read with web-js off")
    refuse(read, ["dynamic113", "written113", "ready113", "timer113"] + SCRIPT_TEXT, "web-read with web-js off")
    say("web-js off: the static page, scripts not run and not shown")

    lw.run(mon, proc, "web-js on", "osui web-js on", 20)
    lw.run(mon, proc, "web-open %s/lot113/js.html" % u, "osui web page sections", fail=["error="])
    read = lw.run(mon, proc, "web-read", "fin113", 30)
    need(read, SCRIPTED, "web-read with web-js on")
    refuse(read, ["static113", "nojs113", "wait113"] + SCRIPT_TEXT, "web-read with web-js on")
    out = lw.run(mon, proc, "web-js status", "osui web-js on", 20)
    need(out, ["pages 1 ", " scripts 2 ran 1 failed 1 external 0 other 0 writes 1 timers 1 events 1 ",
               "error=Uncaught ReferenceError: missing113 is not defined"], "web-js status")
    out = lw.run(mon, proc, "web-js log", "osui web-js log", 20)
    need(out, ["osui web-js log: log113 3", "osui web-js log: error: Uncaught ReferenceError: missing113 is not defined"],
         "web-js log")
    say("web-js on: h1, list, document.write, DOMContentLoaded and timer output in the reader; the throwing script "
        "is counted and logged, the rest of the page still ran")


def step_repl(mon, proc):
    out = lw.run(mon, proc, "js 2**5+10", "osui js steps", 20)
    need(out, ["osui js => 42"], "js 2**5+10")
    lw.run(mon, proc, "js var a113=[1,2,3]", "osui js steps", 20)
    out = lw.run(mon, proc, "js a113.length*14", "osui js steps", 20)
    need(out, ["osui js => 42"], "js a113.length*14")
    out = lw.run(mon, proc, "js nope113", "osui js steps", 20)
    need(out, ["osui js error=thrown Uncaught ReferenceError: nope113 is not defined"], "js error")
    lw.run(mon, proc, "web-js off", "osui web-js off", 20)
    say("js: 2**5+10 is 42, one realm across lines, errors reported")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_pages(mon, proc, "http://10.0.2.2:%d" % hp)
        step_repl(mon, proc)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot113-serial-1x.log")


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
        say("lot113 qemu ok in %.1fs" % (time.monotonic() - t0))
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
