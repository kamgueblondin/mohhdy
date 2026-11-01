#!/usr/bin/env python3
"""Lot 72: images in the browser, in QEMU, offline and deterministic.

The Lot 67 test server (scripts/lot67_web_server.py) serves
/lot72/images.html: PNG, baseline and progressive JPEG (data-src), an
animated GIF, a 200x150 GIF, a missing image (404, alt text) and a .ico
favicon. The image bytes are the unit test fixtures.
Console part: web-images on, web-open of the page: the server sees the
image requests, web-images reports found/shown/failed and the status line
"favicon ok"; the image requests reuse the kept-alive connection. TLS
part: tls-suite forces each new suite against the OpenSSL test server
(TLS 1.3 AES-256-GCM-SHA384 / ChaCha20-Poly1305, TLS 1.2 ECDHE-ECDSA and
ECDHE-RSA (P-256, constant-time ladder) with AES-256-GCM / ChaCha20);
http-keepalive: the page, favicon and images over one TCP connection
(one server port), one connection per request when switched off.
GUI part: the same page in the web window; the decoded
pixels (saturated colours of the test pattern) are counted on the screen
and the favicon is drawn before the title. Screenshots (PNG, when Pillow
is there) go to LOT72_SHOTS (default test_logs/lot72-shots).
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot72-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot72-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot72-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot72-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
from test_qemu_gui_windows import W, H, BAR_H  # noqa: E402

ROOT = g.ROOT
SHOTS = os.environ.get("LOT72_SHOTS", os.path.join(g.LOG_DIR, "lot72-shots"))


def say(msg):
    print("[lot72] %s" % msg, flush=True)


def shot(mon, name):
    w, h, rgb = g.screendump_rgb(mon, "qemu-lot72-%s.ppm" % name)
    try:
        from PIL import Image
        os.makedirs(SHOTS, exist_ok=True)
        path = os.path.join(SHOTS, name + ".png")
        Image.frombytes("RGB", (w, h), bytes(rgb)).save(path)
        say("screenshot %s" % path)
    except ImportError:
        pass
    return rgb


def server_text():
    return open(os.path.join(g.LOG_DIR, "lot72-web-server.log")).read()


def wait_server(needle, timeout=60):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if needle in server_text():
            return
        time.sleep(0.3)
    raise RuntimeError("server never saw %r; tail:\n%s" % (needle, server_text()[-1500:]))


def saturated(rgb, x0, y0, x1, y1):
    """pixels of strong pure colours (the test pattern), never UI chrome"""
    n = 0
    for y in range(y0, y1):
        for x in range(x0, x1):
            i = (y * W + x) * 3
            r, gg, b = rgb[i], rgb[i + 1], rgb[i + 2]
            hi, lo = max(r, gg, b), min(r, gg, b)
            if hi > 180 and lo < 70 and not (b > 150 and r < 90 and gg < 140):   # not link blue
                n += 1
    return n


def main():
    if not os.path.isfile(os.path.join(lw.PKI, "test_ca.pem")):
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "lot67_gen_pki.py"), lw.PKI], check=True)
    os.makedirs(g.LOG_DIR, exist_ok=True)
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    hp = 40000 + os.getpid() % 10000
    srv_log = open(os.path.join(g.LOG_DIR, "lot72-web-server.log"), "w")
    srv = subprocess.Popen([sys.executable, os.path.join(ROOT, "scripts", "lot67_web_server.py"), lw.PKI, str(hp), str(hp + 1)],
                           stdout=srv_log, stderr=subprocess.STDOUT)
    proc = mon = qmp = None
    t0 = time.monotonic()
    try:
        g.prepare_test_disk()
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4", "-cpu", os.environ.get("LOT72_CPU", "max"),
                "-kernel", g.KERNEL, "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet",
                "-serial", "file:" + g.LOG, "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
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
            # ---- console
            lw.run(mon, proc, "web-images on", "web-images on", 30)
            out = lw.run(mon, proc, "web-open %s/lot72/images.html" % u, "osui web page sections", fail=["error"])
            if "favicon ok" not in out:
                raise RuntimeError("favicon not loaded: %r" % out[-600:])
            for name in ("fav.ico", "mire.png", "anim.gif", "mire444.jpg", "mire-prog.jpg", "big.gif", "absente.png"):
                wait_server("IMG %s" % name)
            out = lw.run(mon, proc, "web-images", "web-images on found", 30)
            line = out[out.rfind("web-images on found"):].splitlines()[0]
            say(line)
            for want in ("found 6 ", "shown 5 ", "failed 1 "):
                if want not in line:
                    raise RuntimeError("web-images: want %r in %r" % (want, line))
            say("console: 6 images found, 5 decoded, the 404 one failed, favicon ok")
            out = lw.run(mon, proc, "http-keepalive", "http-keepalive on opened", 30)
            line = out[out.rfind("http-keepalive on opened"):].splitlines()[0]
            say(line)
            if int(line.split(" reused ")[1].split()[0]) < 3:
                raise RuntimeError("image requests did not reuse the connection: %r" % line)
            # ---- TLS suites (each offered alone) and keep-alive
            sp = hp + 1
            for suite, port, want in (("TLS13-AES256-GCM-SHA384", sp + 1, "tls TLS13-AES256-GCM-SHA384"),
                                      ("TLS13-CHACHA20-POLY1305-SHA256", sp + 2, "tls TLS13-CHACHA20-POLY1305-SHA256"),
                                      ("cca9", sp + 4, "tls ECDHE-ECDSA-CHACHA20-POLY1305"),
                                      ("c02c", sp + 4, "tls ECDHE-ECDSA-AES256-GCM-SHA384"),
                                      ("c030", sp + 5, "tls ECDHE-RSA-AES256-GCM-SHA384"),
                                      ("cca8", sp + 5, "tls ECDHE-RSA-CHACHA20-POLY1305")):
                lw.run(mon, proc, "tls-suite %s" % suite, "tls-suite ", 30, fail=["error"])
                out = lw.run(mon, proc, "fetch https://10.0.2.2:%d/hello" % port, "fetch ok status 200", lw.FETCH_TIMEOUT, fail=["fetch error"])
                if want not in out or "fetch ok status 200" not in out:
                    raise RuntimeError("suite %s: %r" % (suite, out[-700:]))
                say("guest TLS %s ok" % want[4:])
            lw.run(mon, proc, "tls-suite auto", "tls-suite auto offered", 30)
            # keep-alive seen from the server: the page, its favicon and its
            # images came over one TCP connection; with keep-alive off, one
            # connection per request
            img_ports = set(l.split("port=")[1] for l in server_text().splitlines() if l.startswith("IMG "))
            say("server: image requests from ports %s" % sorted(img_ports))
            if len(img_ports) != 1:
                raise RuntimeError("images not on one kept-alive connection: %r" % img_ports)
            lw.run(mon, proc, "http-keepalive off", "http-keepalive off", 30)
            n0 = len(server_text().splitlines())
            lw.run(mon, proc, "web-open %s/lot72/images.html" % u, "osui web page sections", fail=["error"])
            wait_server("IMG absente.png", 30)
            img_ports = set(l.split("port=")[1] for l in server_text().splitlines()[n0:] if l.startswith("IMG "))
            say("keep-alive off: image requests from %d ports" % len(img_ports))
            if len(img_ports) < 6:
                raise RuntimeError("keep-alive off still reused: %r" % img_ports)
            lw.run(mon, proc, "http-keepalive on", "http-keepalive on", 30)
            say("keep-alive: one connection for the page and its images, one per request when off")
            # ---- GUI
            g.send_command_until(mon, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            time.sleep(1.0)
            start = len(g.log_text())
            lw.type_line(mon, proc, "web %s/lot72/images.html" % u, mode="getc")
            g.wait_for(proc, "osui web fetch ok status 200", lw.FETCH_TIMEOUT, start)
            time.sleep(3.0)
            rgb = shot(mon, "images-page")
            body = saturated(rgb, 8, BAR_H + 60, W - 8, H - 40)
            say("saturated image pixels in the page: %d" % body)
            if body < 3000:
                raise RuntimeError("images not drawn: %d saturated pixels" % body)
            title = saturated(rgb, 8, BAR_H + 8, W // 2, BAR_H + 48)
            say("saturated favicon pixels in the title bar: %d" % title)
            if title < 12:   # 16x16 gradient icon, about 30 strong pixels
                raise RuntimeError("favicon not drawn: %d pixels" % title)
            g.send_key(mon, "esc")
            time.sleep(1.0)
        say("QEMU Lot 72 contract passed in %.0f s." % (time.monotonic() - t0))
        return 0
    finally:
        if qmp is not None:
            qmp.close()
        if mon is not None:
            mon.close()
        g.terminate(proc)
        srv.terminate()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU Lot 72 failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
