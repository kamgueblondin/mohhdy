#!/usr/bin/env python3
"""GTK smoke: tablet hit-test stays correct after VBE grow/shrink (OS-UI-G-2).

Builds on qemu-osui-gui-fit (COM2 fit path) and qemu-osui-gui tablet QMP clicks.
Hors make integration-qemu. Needs DISPLAY (default :1).
"""
from __future__ import print_function

import importlib.util
import json
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
LOG = os.path.join(LOG_DIR, "qemu-osui-gui-fit-input-serial.log")
ERR = os.path.join(LOG_DIR, "qemu-osui-gui-fit-input-stderr.log")
MON_SOCK = os.path.join(LOG_DIR, "qemu-osui-gui-fit-input-monitor.sock")
QMP_SOCK = os.path.join(LOG_DIR, "qemu-osui-gui-fit-input-qmp.sock")
FIT_SOCK = os.environ.get("MOHHDY_FIT_SOCK", "/tmp/mohhdy-qemu-fit-input-test.sock")
TEST_DISK = os.path.join(LOG_DIR, "qemu-osui-gui-fit-input-overlay.img")
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "75"))
CMD_TIMEOUT = float(os.environ.get("CMD_TIMEOUT", "25"))
KEY_DELAY = 0.08
KEY_HOLD_MS = 10
GROW_W = int(os.environ.get("OSUI_FIT_GROW_W", "1200"))
GROW_H = int(os.environ.get("OSUI_FIT_GROW_H", "750"))
SHRINK_W = int(os.environ.get("OSUI_FIT_SHRINK_W", "800"))
SHRINK_H = int(os.environ.get("OSUI_FIT_SHRINK_H", "600"))
TABLET_OK = "USB Tablet: Controller UHCI initialise et enumere"
TABLET_FAIL = "USB Tablet: Enumeration non terminee"

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


def wait_for(proc, needle, timeout, start=0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped; tail:\n%s" % log_text()[-2000:])
        chunk = log_text()[start:]
        if needle in chunk:
            time.sleep(0.25)
            return
        if TABLET_FAIL in chunk:
            raise RuntimeError("USB tablet enumeration failed; log tail:\n%s" % log_text()[-2000:])
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


def qmp_connect():
    deadline = time.monotonic() + 12
    while time.monotonic() < deadline:
        if os.path.exists(QMP_SOCK):
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                client.connect(QMP_SOCK)
                client.settimeout(1.0)
                try:
                    client.recv(4096)
                except socket.timeout:
                    pass
                client.sendall(b'{"execute":"qmp_capabilities"}\n')
                try:
                    client.recv(4096)
                except socket.timeout:
                    pass
                return client
            except OSError:
                client.close()
        time.sleep(0.1)
    raise RuntimeError("QEMU QMP unavailable")


def qmp_cmd(client, obj):
    client.sendall((json.dumps(obj) + "\n").encode("ascii"))
    time.sleep(0.05)
    try:
        client.recv(8192)
    except socket.timeout:
        pass


def send_key(client, key):
    client.sendall(("sendkey %s %d\n" % (key, KEY_HOLD_MS)).encode("ascii"))
    time.sleep(KEY_DELAY)


def send_command(client, command):
    aliases = {" ": "spc", "-": "minus", ".": "dot", "/": "slash"}
    for char in command:
        send_key(client, aliases.get(char, char.lower()))
    time.sleep(0.2)
    send_key(client, "ret")


def tablet_abs_xy(px, py, width, height):
    if width < 1:
        width = 1024
    if height < 1:
        height = 768
    ax = int((px * 32767) / width)
    ay = int((py * 32767) / height)
    if ax < 0:
        ax = 0
    if ay < 0:
        ay = 0
    if ax > 32767:
        ax = 32767
    if ay > 32767:
        ay = 32767
    return ax, ay


def clampi(v, lo, hi):
    if v < lo:
        return lo
    if v > hi:
        return hi
    return v


def dock_icon_center(width, height, index):
    """Match gfx_desktop_get_layout dock icons (7 icons of 36x32)."""
    bar_h = clampi(height // 22, 28, 40)
    dock_w = 7 * 44 + 16
    if dock_w > width - 16:
        dock_w = width - 16
    dock_x = (width - dock_w) // 2
    dock_y = height - 48
    if dock_y < bar_h + 8:
        dock_y = height - 36
    px = dock_x + 8 + index * 44 + 18
    py = dock_y + 4 + 16
    return px, py


def click_tablet(qmp, px, py, width, height):
    ax, ay = tablet_abs_xy(px, py, width, height)
    qmp_cmd(qmp, {"execute": "input-send-event", "arguments": {"events": [
        {"type": "abs", "data": {"axis": "x", "value": ax}},
        {"type": "abs", "data": {"axis": "y", "value": ay}},
    ]}})
    time.sleep(0.5)
    qmp_cmd(qmp, {"execute": "input-send-event", "arguments": {"events": [
        {"type": "btn", "data": {"down": True, "button": "left"}},
    ]}})
    time.sleep(0.5)
    qmp_cmd(qmp, {"execute": "input-send-event", "arguments": {"events": [
        {"type": "btn", "data": {"down": False, "button": "left"}},
    ]}})
    time.sleep(0.5)


def click_dock_shell(qmp, proc, width, height, label):
    px, py = dock_icon_center(width, height, 2)
    start = len(log_text())
    say("click %s dock[/shell] at %d,%d (fb %dx%d) ..." % (label, px, py, width, height))
    last_error = None
    for attempt in range(3):
        click_tablet(qmp, px, py, width, height)
        try:
            wait_for(proc, "osui gui line=/shell", min(CMD_TIMEOUT, 8.0), start)
            wait_for(proc, "live_eval=true", CMD_TIMEOUT, start)
            return
        except RuntimeError as err:
            last_error = err
            say("click attempt %d missed; retrying" % (attempt + 1))
            start = len(log_text())
    raise last_error


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
    for path in (LOG, ERR, MON_SOCK, QMP_SOCK, FIT_SOCK):
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
    qmp = None
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
                "-qmp", "unix:%s,server,nowait" % QMP_SOCK,
                "-drive", "file=%s,format=raw,if=ide,cache=writethrough" % TEST_DISK,
                "-machine", "type=pc,accel=tcg",
                "-no-reboot", "-no-shutdown",
            ], cwd=ROOT, stdout=err, stderr=err)
            watcher = threading.Thread(target=fit.fit_loop, args=(stop,))
            watcher.daemon = True
            watcher.start()

            wait_for(qemu, "(-.-)", BOOT_TIMEOUT)
            wait_for(qemu, "SYS_GETS: Debut", BOOT_TIMEOUT)
            wait_for(qemu, TABLET_OK, BOOT_TIMEOUT)
            say("USB tablet enumerated")
            monitor = monitor_connect()
            qmp = qmp_connect()
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

            click_dock_shell(qmp, qemu, first[0], first[1], "boot")

            wid = qemu_window_id()
            if not wid:
                raise RuntimeError("QEMU window missing after gui")

            subprocess.check_call(["xdotool", "windowsize", wid, str(GROW_W), str(GROW_H)])
            time.sleep(0.5)
            large = wait_fb_change(qemu, first, 12)
            say("after grow VBE %sx%s" % (large[0], large[1]))
            if not large[0] or large[0] < max(1000, GROW_W - 120) or large[1] < max(650, GROW_H - 120):
                raise RuntimeError("expected larger VBE after grow, got %s" % (large,))
            time.sleep(0.8)
            click_dock_shell(qmp, qemu, large[0], large[1], "after-grow")

            subprocess.check_call(["xdotool", "windowsize", wid, str(SHRINK_W), str(SHRINK_H)])
            time.sleep(0.5)
            small = wait_fb_change(qemu, large, 12)
            say("after shrink VBE %sx%s" % (small[0], small[1]))
            if not small[0] or small[0] >= large[0] or small[1] >= large[1]:
                raise RuntimeError("expected smaller VBE after shrink, got %s from %s" % (small, large))
            if small[0] > 900 or small[1] > 700:
                raise RuntimeError("shrink did not follow the window, got %s" % (small,))
            time.sleep(0.8)
            click_dock_shell(qmp, qemu, small[0], small[1], "after-shrink")

            say("QEMU GUI fit-input contract passed (%sx%s -> %sx%s -> %sx%s, dock /shell x3)." % (
                first[0], first[1], large[0], large[1], small[0], small[1]
            ))
            return 0
    finally:
        stop.set()
        if qmp is not None:
            qmp.close()
        if monitor is not None:
            monitor.close()
        terminate(qemu)
        for path in (MON_SOCK, QMP_SOCK, FIT_SOCK):
            try:
                os.remove(path)
            except OSError:
                pass


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        sys.stderr.write("QEMU GUI fit-input failed: %s\n" % error)
        raise SystemExit(1)
