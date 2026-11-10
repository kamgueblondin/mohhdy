#!/usr/bin/env python3
"""Lot 88 in QEMU, offline: the Lot 67 test server runs on the host and the
guest reaches it as 10.0.2.2 (slirp).

  * autofill per site: values stored by hand for login.example.com and
    www.example.com are offered to a page of shop.example.com (the site's
    hosts), never to example.org or to evil-example.com; an IP host shares
    with nobody. Forgetting from another host of the site works.
  * @scope: /lot88/scope.html hides the p of .card88 but not the one under
    its limit .foot88 nor the one outside; an @scope with no prelude in a
    <style> inside .wrap88 hides only the em there; :scope gets
    text-transform; all: revert undoes an author display: none.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot88-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot88-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot88-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot88-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
from test_qemu_gui_windows import W, H, BAR_H  # noqa: E402,F401

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot88-web-server.log")


def say(msg):
    print("[lot88] %s" % msg, flush=True)


def server_text():
    try:
        return open(SRV_LOG).read()
    except OSError:
        return ""


def wait_server(needle, timeout=60):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if needle in server_text():
            return
        time.sleep(0.3)
    raise RuntimeError("server never saw %r; tail:\n%s" % (needle, server_text()[-1500:]))


def need(text, want, what):
    for w in want:
        if w not in text:
            raise RuntimeError("%s: missing %r in %r" % (what, w, text[-900:]))


def refuse(text, bad, what):
    for b in bad:
        if b in text:
            raise RuntimeError("%s: unexpected %r in %r" % (what, b, text[-900:]))


def console_autofill(mon, proc):
    """Autofill shared inside a site, through the same store and the same
    list the desktop uses (shell_desk)."""
    for host, val in (("login.example.com", "ada88"), ("www.example.com", "cy88"),
                      ("evil-example.com", "mallory88"), ("10.0.2.2", "lab88")):
        out = lw.run(mon, proc, "autofill add %s user %s" % (host, val), "autofill ", 20)
        need(out, ["autofill added"], "autofill add " + host)
    out = lw.run(mon, proc, "autofill offers https://shop.example.com/login user", "autofill offers ", 20)
    need(out, ["autofill offer 0 ada88 site", "autofill offer 1 cy88 site", "autofill offers 2"],
         "offers for shop.example.com")
    refuse(out, ["mallory88", "lab88"], "offers for shop.example.com")
    out = lw.run(mon, proc, "autofill offers https://www.example.com/ user", "autofill offers ", 20)
    need(out, ["autofill offer 0 cy88 host", "autofill offer 1 ada88 site", "autofill offers 2"],
         "offers for www.example.com")
    out = lw.run(mon, proc, "autofill offers https://example.org/ user", "autofill offers ", 20)
    need(out, ["autofill offers 0"], "offers for example.org")
    out = lw.run(mon, proc, "autofill offers http://10.0.2.3/ user", "autofill offers ", 20)
    need(out, ["autofill offers 0"], "offers for another IP")
    out = lw.run(mon, proc, "autofill offers http://10.0.2.2/ user", "autofill offers ", 20)
    need(out, ["autofill offer 0 lab88 host", "autofill offers 1"], "offers for the IP itself")
    out = lw.run(mon, proc, "autofill offers user", "autofill ", 20)
    need(out, ["autofill error usage"], "autofill offers usage")
    out = lw.run(mon, proc, "autofill forget login.example.com user ada88", "autofill ", 20)
    need(out, ["autofill forgot value left 0"], "autofill forget on login")
    out = lw.run(mon, proc, "autofill offers https://shop.example.com/login user", "autofill offers ", 20)
    need(out, ["autofill offer 0 cy88 site", "autofill offers 1"], "offers after forget")
    say("autofill per site: shop.example.com gets login's and www's values, not example.org's, "
        "evil-example.com's or the IP's")


def main():
    if not os.path.isfile(os.path.join(lw.PKI, "test_ca.pem")):
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "lot67_gen_pki.py"), lw.PKI], check=True)
    os.makedirs(g.LOG_DIR, exist_ok=True)
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    hp = 41000 + os.getpid() % 9000
    srv_log = open(SRV_LOG, "w")
    srv = subprocess.Popen([sys.executable, os.path.join(ROOT, "scripts", "lot67_web_server.py"), lw.PKI,
                            str(hp), str(hp + 1)], stdout=srv_log, stderr=subprocess.STDOUT)
    proc = mon = qmp = None
    t0 = time.monotonic()
    try:
        g.prepare_test_disk()
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4",
                "-cpu", os.environ.get("LOT88_CPU", "max"),
                "-kernel", g.KERNEL, "-initrd", g.INITRD, "-m", "1024M",
                "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet",
                "-serial", "file:" + g.LOG,
                "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-qmp", "unix:%s,server,nowait" % g.QMP_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
                "-netdev", "user,id=n0", "-device", "ne2k_isa,netdev=n0",
            ] + g.qemu_disk_args(), cwd=ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            g.wait_for(proc, "usbdriver tablet enumerated port ", g.CMD_TIMEOUT)
            mon = g.monitor_connect()
            qmp = g.qmp_connect()
            g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
            time.sleep(0.5)
            u = "http://10.0.2.2:%d" % hp
            console_autofill(mon, proc)
            ctx = {"u": u, "mon": mon, "qmp": qmp, "proc": proc}
            for step in STEPS:
                step(ctx)
            say("lot88 qemu ok in %.1fs" % (time.monotonic() - t0))
            return 0
    finally:
        for s in (mon, qmp):
            if s:
                try:
                    s.close()
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


def step_scope(ctx):
    mon, proc, u = ctx["mon"], ctx["proc"], ctx["u"]
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-open %s/lot88/scope.html" % u, "osui web page sections", fail=["error="])
    css = lw.run(mon, proc, "web-css", "css-scope ", 30)
    need(css, ["css-scope scopes 2 rules 3", "node div .card88 display auto uppercase"], "web-css @scope")
    read = lw.run(mon, proc, "web-read", "fin88", 30)
    # the card is uppercase (:scope): compare without case
    read = read.lower()
    need(read, ["scopekeep88", "scopeout88", "implkeep88", "allback88"], "web-read @scope")
    refuse(read, ["scopehide88", "implhide88"], "web-read @scope")
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("@scope: the limit and the outside kept, the scoped p and the implicit-scope em hidden, "
        ":scope uppercase, all: revert undid display none")


# later steps of the lot (each takes the context dict)
STEPS = [step_scope]


if __name__ == "__main__":
    try:
        sys.exit(main() or 0)
    except Exception as e:
        print("[lot88] FAIL: %s" % e, flush=True)
        sys.exit(1)
