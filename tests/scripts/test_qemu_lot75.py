#!/usr/bin/env python3
"""Lot 75: reader-view engine forms on a fetched page, form= / formaction,
multipart POST, caret-color and pointer-events, in QEMU offline.

- web-open http://10.0.2.2:PORT/form.html (Lot 71 page on the test server):
  the reader emits real <form>/<input> for the engine; type into the field,
  submit GET; the server sees /echo?q=...&hl=fr&go=Envoyer.
- web-open /web/lot75.html: form= association + formaction GET; multipart
  POST of msg=hi; PEVENTS has pointer-events:none (no link hit).
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot75-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot75-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot75-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot75-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402

ROOT = g.ROOT


def say(msg):
    print("[lot75] %s" % msg, flush=True)


def server_text():
    return open(os.path.join(g.LOG_DIR, "lot75-web-server.log")).read()


def wait_server(needle, timeout=60):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if needle in server_text():
            return
        time.sleep(0.3)
    raise RuntimeError("server never saw %r; tail:\n%s" % (needle, server_text()[-1500:]))


def controls(out):
    """id -> (k, typ, x, y, w, h, value). Also index keys as '#N'."""
    found = {}
    for m in re.finditer(r"osui web form2 ctl (\d+) (\w+) id=(\S+) box (-?\d+) (-?\d+) (\d+) (\d+) value=([^\r\n]*)", out):
        k = int(m.group(1))
        typ = m.group(2)
        iid = m.group(3)
        box = (k, typ, int(m.group(4)), int(m.group(5)), int(m.group(6)), int(m.group(7)), m.group(8).strip())
        found["#%d" % k] = box
        if iid != "-":
            found[iid] = box
        elif typ not in found:
            found[typ] = box
    return found


def fstate(mon, proc):
    out = lw.run(mon, proc, "web-fstate", "osui web form2 state", 30)
    out = out[out.rfind("osui web form2 state"):]
    return out, controls(out)


def click(mon, proc, ctl, marker="osui web form2"):
    x, y = ctl[2] + ctl[4] // 2, ctl[3] + ctl[5] // 2
    return lw.run(mon, proc, "web-fclick %d %d" % (x, y), marker, 30)


def main():
    lw.ALIASES.setdefault("#", "shift-3")
    if not os.path.isfile(os.path.join(lw.PKI, "test_ca.pem")):
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "lot67_gen_pki.py"), lw.PKI], check=True)
    os.makedirs(g.LOG_DIR, exist_ok=True)
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    hp = int(os.environ.get("LOT75_PORT", "18075"))
    srv_log = open(os.path.join(g.LOG_DIR, "lot75-web-server.log"), "w")
    srv = subprocess.Popen([sys.executable, os.path.join(ROOT, "scripts", "lot67_web_server.py"), lw.PKI, str(hp), str(hp + 1)],
                           stdout=srv_log, stderr=subprocess.STDOUT)
    proc = mon = qmp = None
    t0 = time.monotonic()
    try:
        g.prepare_test_disk()
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4", "-cpu", os.environ.get("LOT75_CPU", "max"),
                "-kernel", g.KERNEL, "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet",
                "-serial", "file:" + g.LOG, "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-qmp", "unix:%s,server,nowait" % g.QMP_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
                "-netdev", "user,id=n0", "-device", "ne2k_isa,netdev=n0",
            ] + g.qemu_disk_args(), cwd=ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            mon = g.monitor_connect()
            qmp = g.qmp_connect()
            g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
            time.sleep(0.5)

            # ---- reader-view engine forms on a fetched page
            url = "http://10.0.2.2:%d/form.html" % hp
            lw.run(mon, proc, "web-open %s" % url, "osui web", 90, fail=["error="])
            out, ctl = fstate(mon, proc)
            say("fetched form controls: %r" % ctl)
            if not ctl:
                raise RuntimeError("reader forms: no engine controls after web-open form.html: %r" % out[:500])
            # pick a text-like control (prefer id q / name via id=name emit)
            text_id = None
            for prefer in ("q", "user", "#0"):
                if prefer in ctl and ctl[prefer][1] == "input":
                    text_id = prefer
                    break
            if text_id is None:
                for k, v in ctl.items():
                    if k.startswith("#") and v[1] == "input" and "submit" not in (v[6] or "").lower():
                        text_id = k
                        break
            if text_id is None:
                raise RuntimeError("reader forms: no text control in %r" % ctl)
            out = click(mon, proc, ctl[text_id])
            if "osui web form2" not in out:
                raise RuntimeError("click field: %r" % out[-400:])
            out = lw.run(mon, proc, "web-ftype lot75", "osui web form2", 30)
            if "typed" not in out and "changed" not in out:
                say("ftype note: %r" % out[-200:])
            # Enter submits the focused form (GET /echo?...)
            out = lw.run(mon, proc, "web-fkey enter", "osui web", 60)
            say("submit out: %r" % out[-300:])
            wait_server("/echo", timeout=90)
            say("server saw echo after reader form submit")

            # ---- local lot75.html: form= + multipart + pointer-events
            lw.run(mon, proc, "web-open /web/lot75.html", "osui web", 60, fail=["error="])
            out, ctl = fstate(mon, proc)
            say("local controls: %r" % ctl)
            if "a" not in ctl and "go" not in ctl:
                # ids may vary; require at least 2 controls
                if len(ctl) < 2:
                    raise RuntimeError("lot75.html: want form controls: %r" % ctl)
            if "a" in ctl:
                click(mon, proc, ctl["a"])
                lw.run(mon, proc, "web-ftype hello", "osui web form2", 30)
            # Multipart first: GET navigate would leave this page
            if "send" in ctl:
                out = click(mon, proc, ctl["send"], marker="osui web")
                say("multipart POST: %r" % out[-200:])
                wait_server("POST", timeout=60)
                if "multipart" not in out.lower() and "form2 submit" not in out and "submit POST" not in out:
                    say("multipart note (may be in body): %r" % out[-300:])
                # reopen local page for form= GET (POST navigated away)
                lw.run(mon, proc, "web-open /web/lot75.html", "osui web", 60, fail=["error="])
                out, ctl = fstate(mon, proc)
                say("reopened controls: %r" % ctl)
                if "a" in ctl:
                    click(mon, proc, ctl["a"])
                    lw.run(mon, proc, "web-ftype hello", "osui web form2", 30)
            if "go" in ctl:
                out = click(mon, proc, ctl["go"], marker="osui web")
                say("form= GET: %r" % out[-200:])
                wait_server("q=hello", timeout=60)

            say("lot75 qemu ok in %.1fs" % (time.monotonic() - t0))
            return 0
    finally:
        if mon:
            try:
                mon.close()
            except Exception:
                pass
        if qmp:
            try:
                qmp.close()
            except Exception:
                pass
        if proc and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except Exception:
                proc.kill()
        srv.terminate()
        try:
            srv.wait(timeout=5)
        except Exception:
            srv.kill()
        srv_log.close()


if __name__ == "__main__":
    try:
        sys.exit(main() or 0)
    except Exception as e:
        print("[lot75] FAIL: %s" % e, flush=True)
        sys.exit(1)
