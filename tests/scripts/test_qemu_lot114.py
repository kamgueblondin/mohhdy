#!/usr/bin/env python3
"""Lot 114 in QEMU, offline: the page pass after Lot 113 (US-031, still
partial). One boot at ui-scale 1x; the Lot 67 test server runs on the
host and the guest reaches it as 10.0.2.2 (slirp). Everything is typed
on the GUI input line, which runs the same osui_gui_web_command code as
the console; the web-js event words go to shell_page_js, the function
the GUI also calls for a click on a "?__wfjs=N" link and for keys.

/lot114/app.html (web-js on) loads lib114.js (src=) and late114.js
(defer) through the guest fetch path; the reader shows "ext114 42" (the
inline script used the external one) and "defer114 interactive".
missing114.js is a 404: counted in ext_failed, the page goes on.
fetch('data114.json') builds three list items ("item114 1..3"), an
XMLHttpRequest POST to echo114 answers "xhr114 200 echo114 text/plain
ping", an async function reads the approximate layout (offsetWidth of a
7 character button: 7 x 8 + 16 = 72, "async114 72"), and a cross-origin
fetch is refused (log line "cors114 Failed to fetch"). After load:
"web-js click #inc" runs the click listener ("count114 1", twice gives
2), "web-js key z" reaches the document keydown listener ("keys114 z"),
"web-js input #q hi" the input listener ("input114 hi"); "web-js clicks"
lists the button. A new page ends the live realm (no_live_page). No
guest clock is involved (no -rtc option).
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot114-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot114-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot114-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot114-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot114-web-server.log")
need, refuse = l88.need, l88.refuse
lw.ALIASES.update({"#": "shift-3", "@": "shift-2"})

SCRIPTED = ["ext114 42", "count114 0", "item114 1", "item114 2", "item114 3", "xhr114 200 echo114 text/plain ping",
            "async114 72", "defer114 interactive", "fin114"]
SCRIPT_TEXT = ["Lib114", "addEventListener", "XMLHttpRequest"]


def say(msg):
    print("[lot114] %s" % msg, flush=True)


def step_page(mon, proc, u):
    lw.run(mon, proc, "web-js on", "osui web-js on", 20)
    lw.run(mon, proc, "web-open %s/lot114/app.html" % u, "osui web page sections", fail=["error="])
    read = lw.run(mon, proc, "web-read", "fin114", 30)
    need(read, SCRIPTED, "web-read with web-js on")
    refuse(read, ["static114", "xhr114 wait", "async114 wait"] + SCRIPT_TEXT, "web-read with web-js on")
    out = lw.run(mon, proc, "web-js status", "osui web-js on", 20)
    need(out, [" external 3 ext_ran 2 ext_failed 1 ext_skipped 0 ", " net 3 net_failed 1 ", " alive 1"],
         "web-js status")
    out = lw.run(mon, proc, "web-js log", "osui web-js log", 20)
    need(out, ["osui web-js log: warn: script not loaded (HTTP status not 200): %s/lot114/missing114.js" % u,
               "cors114 Failed to fetch"], "web-js log")
    say("external and deferred scripts, fetch, XMLHttpRequest, async/await and a layout value in the reader; "
        "the 404 script and the cross-origin fetch are counted and logged")


def step_events(mon, proc):
    out = lw.run(mon, proc, "web-js clicks", "osui web-js clicks", 20)
    need(out, ["osui web-js clicks 1", " button #inc plus114"], "web-js clicks")
    out = lw.run(mon, proc, "web-js click #inc", "osui web-js event click", 30)
    need(out, ["osui web-js event click #inc ok"], "web-js click")
    lw.run(mon, proc, "web-js click #inc", "osui web-js event click", 30)
    read = lw.run(mon, proc, "web-read", "fin114", 30)
    need(read, ["count114 2", "ext114 42", "item114 3"], "web-read after two clicks")
    refuse(read, ["count114 0"], "web-read after two clicks")
    lw.run(mon, proc, "web-js key z", "osui web-js event key", 30)
    read = lw.run(mon, proc, "web-read", "fin114", 30)
    need(read, ["keys114 z"], "web-read after a key")
    lw.run(mon, proc, "web-js input #q hi", "osui web-js event input", 30)
    read = lw.run(mon, proc, "web-read", "fin114", 30)
    need(read, ["input114 hi", "count114 2"], "web-read after input")
    out = lw.run(mon, proc, "web-js click #none", "osui web-js event", 30)
    need(out, ["osui web-js event error=no_target click #none"], "web-js click on no element")
    say("after load: click, key and input listeners ran through web-js, the page was shown again")


def step_end(mon, proc, u):
    lw.run(mon, proc, "web-open %s/lot113/js.html" % u, "osui web page sections", fail=["error="])
    out = lw.run(mon, proc, "web-js click #inc", "osui web-js event", 30)
    need(out, ["osui web-js event error=no_target"], "the new page has its own realm")
    lw.run(mon, proc, "web-js off", "osui web-js off", 20)
    lw.run(mon, proc, "web-open %s/lot114/app.html" % u, "osui web page sections", fail=["error="])
    out = lw.run(mon, proc, "web-js click #inc", "osui web-js event", 30)
    need(out, ["osui web-js event error=no_live_page"], "web-js off: no live page")
    read = lw.run(mon, proc, "web-read", "fin114", 30)
    need(read, ["static114", "count114 0", "xhr114 wait"], "web-read with web-js off")
    say("a new page has its own realm; with web-js off the page is the static one and has no live realm")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        u = "http://10.0.2.2:%d" % hp
        step_page(mon, proc, u)
        step_events(mon, proc)
        step_end(mon, proc, u)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot114-serial-1x.log")


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
        say("lot114 qemu ok in %.1fs" % (time.monotonic() - t0))
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
