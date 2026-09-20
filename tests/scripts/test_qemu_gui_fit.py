#!/usr/bin/env python3
"""GTK smoke: the VBE desktop follows the QEMU window size.

Hors make integration-qemu. Needs a graphical display (DISPLAY=:1).
"""
from __future__ import print_function

import importlib.util
import os
import socket
import subprocess
import sys
import threading
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KERNEL = os.environ.get("KERNEL", os.path.join(ROOT, "build", "mohhdy.bin"))
INITRD = os.environ.get("INITRD", os.path.join(ROOT, "my_initrd.tar"))
LOG_DIR = os.path.join(ROOT, "test_logs")
ART = os.environ.get("OSUI_FIT_ART", "/opt/cursor/artifacts")
LOG = os.path.join(LOG_DIR, "qemu-osui-gui-fit-serial.log")
ERR = os.path.join(LOG_DIR, "qemu-osui-gui-fit-stderr.log")
MON_SOCK = os.path.join(LOG_DIR, "qemu-osui-gui-fit-monitor.sock")
FIT_SOCK = os.environ.get("MOHHDY_FIT_SOCK", "/tmp/mohhdy-qemu-fit-test.sock")
TEST_DISK = os.path.join(LOG_DIR, "qemu-osui-gui-fit-overlay.img")
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "75"))
CMD_TIMEOUT = float(os.environ.get("CMD_TIMEOUT", "25"))
KEY_DELAY = 0.08
KEY_HOLD_MS = 10

os.environ.setdefault("DISPLAY", ":1")
os.environ["MOHHDY_FIT_SOCK"] = FIT_SOCK


def say(message):
    sys.stdout.write(message + "\n")
    sys.stdout.flush()


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(proc, needle, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped; tail:\n%s" % log_text()[-2000:])
        if needle in log_text():
            time.sleep(0.25)
            return
        time.sleep(0.12)
    raise RuntimeError("timeout %r; tail:\n%s" % (needle, log_text()[-2000:]))


def load_fit():
    path = os.path.join(ROOT, "scripts", "qemu_gui_fit.py")
    spec = importlib.util.spec_from_file_location("qemu_gui_fit", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def monitor_connect():
    deadline = time.monotonic() + 12
    while time.monotonic() < deadline:
        if os.path.exists(MON_SOCK):
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                client.connect(MON_SOCK)
                client.settimeout(0.15)
                try:
                    client.recv(4096)
                except socket.timeout:
                    pass
                return client
            except OSError:
                client.close()
        time.sleep(0.1)
    raise RuntimeError("monitor unavailable")


def send_key(client, key):
    client.sendall(("sendkey %s %d\n" % (key, KEY_HOLD_MS)).encode("ascii"))
    time.sleep(KEY_DELAY)


def send_command(client, command):
    aliases = {" ": "spc", "-": "minus", ".": "dot", "/": "slash"}
    for char in command:
        send_key(client, aliases.get(char, char.lower()))
    time.sleep(0.2)
    send_key(client, "ret")


def qemu_window_id():
    try:
        out = subprocess.check_output(
            ["xdotool", "search", "--name", "QEMU"],
            stderr=subprocess.DEVNULL,
        ).decode("ascii", "replace").strip()
    except (OSError, subprocess.CalledProcessError):
        return None
    ids = [line for line in out.splitlines() if line.strip()]
    return ids[-1] if ids else None


def window_geom(wid):
    info = subprocess.check_output(
        ["xwininfo", "-id", wid],
        stderr=subprocess.DEVNULL,
    ).decode("ascii", "replace")
    x = y = w = h = None
    for line in info.splitlines():
        line = line.strip()
        if line.startswith("Absolute upper-left X:"):
            x = int(line.split(":")[1])
        elif line.startswith("Absolute upper-left Y:"):
            y = int(line.split(":")[1])
        elif line.startswith("Width:"):
            w = int(line.split(":")[1])
        elif line.startswith("Height:"):
            h = int(line.split(":")[1])
    if None in (x, y, w, h):
        raise RuntimeError("xwininfo incomplete")
    return x, y, w, h


def screenshot_window(path):
    wid = qemu_window_id()
    if not wid:
        raise RuntimeError("QEMU window not found")
    x, y, w, h = window_geom(wid)
    if w < 8:
        w = 8
    if h < 8:
        h = 8
    w &= ~1
    h &= ~1
    subprocess.check_call([
        "ffmpeg", "-y", "-loglevel", "error",
        "-f", "x11grab",
        "-video_size", "%dx%d" % (w, h),
        "-i", "%s+%d,%d" % (os.environ.get("DISPLAY", ":1"), x, y),
        "-frames:v", "1",
        path,
    ])
    return w, h


def ppm_size(path):
    with open(path, "rb") as handle:
        magic = handle.readline().strip()
        dims = handle.readline()
        while dims.startswith(b"#"):
            dims = handle.readline()
        parts = dims.split()
        return magic, int(parts[0]), int(parts[1])


def ppm_to_png(ppm, png):
    from PIL import Image
    img = Image.open(ppm)
    img.save(png)


def last_fb_size():
    text = log_text()
    width = height = None
    for line in text.splitlines():
        if "osui gui fb " in line or "osui gui fb resize " in line:
            token = line.split("fb", 1)[1].strip()
            if token.startswith("resize "):
                token = token[7:]
            token = token.split()[0]
            if "x" in token:
                a, b = token.split("x", 1)
                if a.isdigit() and b.isdigit():
                    width, height = int(a), int(b)
    return width, height


def wait_fb_change(proc, prev, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped; tail:\n%s" % log_text()[-2000:])
        now = last_fb_size()
        if now[0] and now != prev:
            return now
        time.sleep(0.15)
    return last_fb_size()


def screendump(monitor, path):
    try:
        os.remove(path)
    except OSError:
        pass
    monitor.sendall(("screendump %s\n" % path).encode("ascii"))
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        if os.path.isfile(path) and os.path.getsize(path) > 8000:
            time.sleep(0.2)
            return ppm_size(path)
        time.sleep(0.15)
    raise RuntimeError("screendump missing: %s" % path)


def terminate(proc):
    if proc is None or proc.poll() is not None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=4)
    except subprocess.TimeoutExpired:
        proc.kill()


def main():
    if not os.path.isfile(KERNEL) or not os.path.isfile(INITRD):
        raise RuntimeError("missing build artefacts")
    os.makedirs(LOG_DIR, exist_ok=True)
    os.makedirs(ART, exist_ok=True)
    for path in (LOG, ERR, MON_SOCK, FIT_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass

    subprocess.run([
        sys.executable,
        os.path.join(ROOT, "tests", "scripts", "make_fat16_image.py"),
        "--image", TEST_DISK,
    ], check=True)

    fit = load_fit()
    stop = threading.Event()
    qemu = None
    monitor = None
    watcher = None
    try:
        with open(ERR, "wb") as err:
            qemu = subprocess.Popen([
                "qemu-system-i386",
                "-name", "Mohhdy OS",
                "-cpu", "pentium3",
                "-kernel", KERNEL,
                "-initrd", INITRD,
                "-m", "1024M",
                "-vga", "std",
                "-usb",
                "-device", "usb-tablet",
                "-display", "gtk,zoom-to-fit=on,show-menubar=off,grab-on-hover=off,show-cursor=on",
                "-serial", "file:" + LOG,
                "-serial", "unix:%s,server,nowait" % FIT_SOCK,
                "-monitor", "unix:%s,server,nowait" % MON_SOCK,
                "-drive", "file=%s,format=raw,if=ide,cache=writethrough" % TEST_DISK,
                "-machine", "type=pc,accel=tcg",
                "-no-reboot", "-no-shutdown",
            ], cwd=ROOT, stdout=err, stderr=err)
            watcher = threading.Thread(target=fit.fit_loop, args=(stop,))
            watcher.daemon = True
            watcher.start()

            wait_for(qemu, "(-.-)", BOOT_TIMEOUT)
            wait_for(qemu, "SYS_GETS: Debut", BOOT_TIMEOUT)
            monitor = monitor_connect()
            time.sleep(0.6)
            say("typing gui ...")
            send_command(monitor, "gui")
            wait_for(qemu, "osui gui live", CMD_TIMEOUT)
            wait_for(qemu, "osui gui fb ", CMD_TIMEOUT)
            time.sleep(1.2)
            first = last_fb_size()
            say("initial VBE %sx%s" % first)
            if not first[0]:
                raise RuntimeError("no VBE size logged")

            os.makedirs(ART, exist_ok=True)
            w0, h0 = screenshot_window(os.path.join(ART, "desktop_fit_boot.png"))
            dump0 = os.path.join(LOG_DIR, "fit-boot.ppm")
            magic, dw, dh = screendump(monitor, dump0)
            ppm_to_png(dump0, os.path.join(ART, "desktop_vbe_boot.png"))
            say("boot window %dx%d screendump %s %dx%d" % (w0, h0, magic.decode("ascii"), dw, dh))

            wid = qemu_window_id()
            if not wid:
                raise RuntimeError("QEMU window missing after gui")

            subprocess.check_call(["xdotool", "windowsize", wid, "1480", "920"])
            time.sleep(0.5)
            geom = window_geom(wid)
            say("grow window geom %dx%d at %d,%d" % (geom[2], geom[3], geom[0], geom[1]))
            large = wait_fb_change(qemu, first, 12)
            say("after grow VBE %sx%s" % (large[0], large[1]))
            if not large[0] or large[0] < 1100 or large[1] < 700:
                raise RuntimeError("expected larger VBE after grow, got %s" % (large,))
            w1, h1 = screenshot_window(os.path.join(ART, "desktop_fit_large.png"))
            dump1 = os.path.join(LOG_DIR, "fit-large.ppm")
            magic, dw, dh = screendump(monitor, dump1)
            ppm_to_png(dump1, os.path.join(ART, "desktop_vbe_large.png"))
            say("large window %dx%d screendump %dx%d" % (w1, h1, dw, dh))
            if dw < 1100 or dh < 700:
                raise RuntimeError("screendump not enlarged: %dx%d" % (dw, dh))
            if abs(w1 - dw) > 48 or abs(h1 - dh) > 48:
                raise RuntimeError("large VBE does not fill window: fb %dx%d window %dx%d" % (dw, dh, w1, h1))

            subprocess.check_call(["xdotool", "windowsize", wid, "800", "600"])
            time.sleep(0.5)
            small = wait_fb_change(qemu, large, 12)
            say("after shrink VBE %sx%s" % (small[0], small[1]))
            if not small[0] or small[0] >= large[0] or small[1] >= large[1]:
                raise RuntimeError("expected smaller VBE after shrink, got %s from %s" % (small, large))
            if small[0] > 900 or small[1] > 700:
                raise RuntimeError("shrink did not follow the window, got %s" % (small,))
            w2, h2 = screenshot_window(os.path.join(ART, "desktop_fit_small.png"))
            dump2 = os.path.join(LOG_DIR, "fit-small.ppm")
            magic, dw, dh = screendump(monitor, dump2)
            ppm_to_png(dump2, os.path.join(ART, "desktop_vbe_small.png"))
            say("small window %dx%d screendump %dx%d" % (w2, h2, dw, dh))
            if dw > 900 or dh > 700:
                raise RuntimeError("screendump not reduced: %dx%d" % (dw, dh))
            if abs(w2 - dw) > 48 or abs(h2 - dh) > 48:
                raise RuntimeError("small VBE does not fill window: fb %dx%d window %dx%d" % (dw, dh, w2, h2))

            say("QEMU GUI fit contract passed (%sx%s -> %sx%s -> %sx%s)." % (
                first[0], first[1], large[0], large[1], small[0], small[1]
            ))
            return 0
    finally:
        stop.set()
        if monitor is not None:
            monitor.close()
        terminate(qemu)
        try:
            os.remove(MON_SOCK)
        except OSError:
            pass
        try:
            os.remove(FIT_SOCK)
        except OSError:
            pass


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        sys.stderr.write("QEMU GUI fit failed: %s\n" % error)
        raise SystemExit(1)
