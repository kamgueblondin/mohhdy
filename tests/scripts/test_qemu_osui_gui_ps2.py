#!/usr/bin/env python3
"""QEMU smoke: OS-UI gui with PS/2 relative mouse only (no usb-tablet).

OS-UI-G-3 / AOS-003. Hors make integration-qemu.
Proves relative move, dock hit-test click, edge clamping, console return.
"""
from __future__ import print_function

import json
import os
import re
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KERNEL = os.environ.get("KERNEL", os.path.join(ROOT, "build", "mohhdy.bin"))
INITRD = os.environ.get("INITRD", os.path.join(ROOT, "my_initrd.tar"))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.environ.get("OSUI_GUI_PS2_LOG", os.path.join(LOG_DIR, "qemu-osui-gui-ps2-serial.log"))
QEMU_ERR = os.environ.get("OSUI_GUI_PS2_ERR", os.path.join(LOG_DIR, "qemu-osui-gui-ps2-stderr.log"))
MON_SOCK = os.environ.get("OSUI_GUI_PS2_MON_SOCK", os.path.join(LOG_DIR, "qemu-osui-gui-ps2-monitor.sock"))
QMP_SOCK = os.environ.get("OSUI_GUI_PS2_QMP_SOCK", os.path.join(LOG_DIR, "qemu-osui-gui-ps2-qmp.sock"))
TEST_DISK = os.environ.get("OVERLAY_DISK", os.path.join(LOG_DIR, "qemu-osui-gui-ps2-overlay.img"))
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "75"))
CMD_TIMEOUT = float(os.environ.get("CMD_TIMEOUT", "20"))
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.05"))
KEY_HOLD_MS = int(os.environ.get("KEY_HOLD_MS", "10"))
KEY_ECHO_TIMEOUT = float(os.environ.get("KEY_ECHO_TIMEOUT", "3"))
KEY_DUPLICATE_SETTLE_DELAY = float(os.environ.get("KEY_DUPLICATE_SETTLE_DELAY", "0.25"))
KEY_CHAR_RETRIES = int(os.environ.get("KEY_CHAR_RETRIES", "3"))
# Guest gfx_desktop_move_mouse doubles deltas when !usb_tablet_present().
PS2_GAIN = 2


def say(message):
    sys.stdout.write(message + "\n")
    sys.stdout.flush()


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def normalized_log(output):
    output = re.sub(r"(?<=\w)TIMER_ALIVE: tick=\d+\+?\r?\n(?=\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?\r?\n(?=\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?\r?\n(?=\s+\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?", "", output)
    output = re.sub(r"\[SCHED\] switching to task \d+\r?\n?", "", output)
    return output


def wait_for(proc, needle, timeout, start=0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped unexpectedly; log tail:\n%s" % log_text()[-2000:])
        output = normalized_log(log_text()[start:])
        if needle in output:
            time.sleep(0.35)
            return
        time.sleep(0.15)
    raise RuntimeError("timeout waiting for %r; log tail:\n%s" % (needle, log_text()[-2000:]))


def monitor_connect():
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        if os.path.exists(MON_SOCK):
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                client.connect(MON_SOCK)
                client.settimeout(0.1)
                try:
                    client.recv(4096)
                except socket.timeout:
                    pass
                return client
            except OSError:
                client.close()
        time.sleep(0.1)
    raise RuntimeError("QEMU monitor unavailable")


def qmp_connect():
    deadline = time.monotonic() + 10
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
    time.sleep(0.03)
    try:
        client.recv(8192)
    except socket.timeout:
        pass


def prepare_test_disk():
    directory = os.path.dirname(TEST_DISK)
    if directory:
        os.makedirs(directory, exist_ok=True)
    subprocess.run([
        sys.executable,
        os.path.join(ROOT, "tests", "scripts", "make_fat16_image.py"),
        "--image", TEST_DISK,
    ], check=True)


def qemu_disk_args():
    return ["-drive", "file=%s,format=raw,if=ide,cache=writethrough" % TEST_DISK]


def send_key(client, key):
    client.sendall(("sendkey %s %d\n" % (key, KEY_HOLD_MS)).encode("ascii"))


def clampi(v, lo, hi):
    if v < lo:
        return lo
    if v > hi:
        return hi
    return v


def dock_icon_center(width, height, index):
    bar_h = clampi(height // 22, 28, 40)
    dock_w = 7 * 44 + 16
    if dock_w > width - 16:
        dock_w = width - 16
    dock_x = (width - dock_w) // 2
    dock_y = height - 48
    if dock_y < bar_h + 8:
        dock_y = height - 36
    return (dock_x + 8 + index * 44 + 18, dock_y + 4 + 16)


def send_rel_raw(qmp, dx, dy):
    """Send one QMP relative step (values should fit PS/2 int8 after QEMU packing)."""
    qmp_cmd(qmp, {"execute": "input-send-event", "arguments": {"events": [
        {"type": "rel", "data": {"axis": "x", "value": int(dx)}},
        {"type": "rel", "data": {"axis": "y", "value": int(dy)}},
    ]}})


def move_screen(qmp, screen_dx, screen_dy):
    """Move guest cursor by screen pixels (accounts for PS/2 x2 gain).

    QMP rel Y positive = screen down. Guest mouse_handle_byte applies -dy from
    the PS/2 packet; QEMU's PS/2 device inverts so QMP +y still moves down.
    """
    # Host QMP deltas before guest gain.
    host_dx = int(round(float(screen_dx) / float(PS2_GAIN)))
    host_dy = int(round(float(screen_dy) / float(PS2_GAIN)))
    step = 60
    while host_dx != 0 or host_dy != 0:
        sx = clampi(host_dx, -step, step)
        sy = clampi(host_dy, -step, step)
        send_rel_raw(qmp, sx, sy)
        host_dx -= sx
        host_dy -= sy
        time.sleep(0.04)
    time.sleep(0.15)


def click_left(qmp):
    qmp_cmd(qmp, {"execute": "input-send-event", "arguments": {"events": [
        {"type": "btn", "data": {"down": True, "button": "left"}},
    ]}})
    time.sleep(0.35)
    qmp_cmd(qmp, {"execute": "input-send-event", "arguments": {"events": [
        {"type": "btn", "data": {"down": False, "button": "left"}},
    ]}})
    time.sleep(0.35)


def key_echo_count(output, char, mode):
    if mode == "getc":
        pattern = r"SYS_GETC: caract.re retourn.:\s*'%s'" % re.escape(char)
    else:
        pattern = r"SYS_GETS: caract.re ajout.:\s*'%s'" % re.escape(char)
    return len(re.findall(pattern, normalized_log(output)))


def send_command_once(client, command, proc, mode):
    aliases = {" ": "spc", "-": "minus", ".": "dot", "/": "slash"}
    for char in command:
        count = 0
        for _ in range(KEY_CHAR_RETRIES):
            start = len(log_text())
            send_key(client, aliases.get(char, char.lower()))
            deadline = time.monotonic() + KEY_ECHO_TIMEOUT
            while time.monotonic() < deadline:
                if proc.poll() is not None:
                    raise RuntimeError("QEMU stopped unexpectedly; log tail:\n%s" % log_text()[-2000:])
                time.sleep(KEY_DELAY)
                count = key_echo_count(log_text()[start:], char.lower(), mode)
                if count:
                    time.sleep(KEY_DUPLICATE_SETTLE_DELAY)
                    count = key_echo_count(log_text()[start:], char.lower(), mode)
                    break
            if count:
                break
        if count == 0:
            raise RuntimeError("character not received: %s" % char)
        for _ in range(count - 1):
            send_key(client, "backspace")
            time.sleep(KEY_DUPLICATE_SETTLE_DELAY)
    send_key(client, "ret")


def command_echoed(output, command, mode):
    expected = " ".join(command.lower().split())
    text = normalized_log(output)
    if mode == "getc":
        for received in re.findall(r"osui gui line=([^\r\n]+)", text):
            if " ".join(received.lower().split()) == expected:
                return True
        return False
    for received in re.findall(r"SYS_GETS: ligne lue: ([^\r\n]+)", text):
        if " ".join(received.lower().split()) == expected:
            return True
    return False


def send_command_until(client, command, marker, proc, mode="gets", wait_prompt=True):
    start = len(log_text())
    send_command_once(client, command, proc, mode)
    deadline = time.monotonic() + CMD_TIMEOUT
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped unexpectedly; log tail:\n%s" % log_text()[-2000:])
        output = normalized_log(log_text()[start:])
        if command_echoed(output, command, mode):
            wait_for(proc, marker, CMD_TIMEOUT, start)
            if wait_prompt:
                wait_for(proc, "(-.-)", CMD_TIMEOUT, start)
            return
        if mode == "gets" and "SYS_GETS: ligne lue: " in output:
            raise RuntimeError("command echo altered: %s" % command)
        time.sleep(0.1)
    raise RuntimeError("timeout waiting for command echo: %s" % command)


def terminate(proc):
    if proc is None or proc.poll() is not None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=4)
    except subprocess.TimeoutExpired:
        proc.kill()


def click_dock_shell_ps2(qmp, proc, width, height, cur_x, cur_y):
    """Relative-move to dock[/shell] and click; return updated cursor guess."""
    tx, ty = dock_icon_center(width, height, 2)
    # Nudge grid around the predicted center (relative-only aiming).
    offsets = [(0, 0), (-8, 0), (8, 0), (0, -6), (0, 6), (-16, -8), (16, 8), (-24, 0), (24, 0)]
    for ox, oy in offsets:
        target_x = tx + ox
        target_y = ty + oy
        say("PS/2 rel move to dock[/shell] ~%d,%d (from ~%d,%d) ..." % (target_x, target_y, cur_x, cur_y))
        move_screen(qmp, target_x - cur_x, target_y - cur_y)
        cur_x, cur_y = target_x, target_y
        start = len(log_text())
        say("PS/2 left click at guessed %d,%d ..." % (cur_x, cur_y))
        click_left(qmp)
        try:
            wait_for(proc, "osui gui line=/shell", min(CMD_TIMEOUT, 6.0), start)
            wait_for(proc, "live_eval=true", CMD_TIMEOUT, start)
            return cur_x, cur_y
        except RuntimeError:
            say("dock click miss; adjusting")
    raise RuntimeError("PS/2 relative dock[/shell] click failed")


def main():
    if not os.path.isfile(KERNEL) or not os.path.isfile(INITRD):
        raise RuntimeError("missing build artefacts; run make all first")
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, QEMU_ERR, MON_SOCK, QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass

    prepare_test_disk()
    proc = None
    monitor = None
    qmp = None
    try:
        with open(QEMU_ERR, "wb") as err:
            # Intentionally NO -usb / usb-tablet: PS/2 relative only.
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", KERNEL,
                "-initrd", INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-serial", "file:" + LOG, "-monitor", "unix:%s,server,nowait" % MON_SOCK,
                "-qmp", "unix:%s,server,nowait" % QMP_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ] + qemu_disk_args(), cwd=ROOT, stdout=err, stderr=err)
            wait_for(proc, "(-.-)", BOOT_TIMEOUT)
            wait_for(proc, "SYS_GETS: Debut", BOOT_TIMEOUT)
            wait_for(proc, "USB Tablet: Controller UHCI non trouve", CMD_TIMEOUT)
            if "Controller UHCI initialise et enumere" in log_text():
                raise RuntimeError("unexpected UHCI tablet enum without usb-tablet")
            monitor = monitor_connect()
            qmp = qmp_connect()
            time.sleep(0.6)

            say("typing gui-status ...")
            send_command_until(monitor, "gui-status", "canonical=gui", proc)
            say("typing gui ...")
            send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
            wait_for(proc, "chrome=qemu_fb", CMD_TIMEOUT)
            wait_for(proc, "OSUI-SNAP", CMD_TIMEOUT)
            wait_for(proc, "1024x768", CMD_TIMEOUT)

            dump = os.path.join(LOG_DIR, "qemu-osui-gui-ps2-desktop.ppm")
            try:
                os.remove(dump)
            except OSError:
                pass
            monitor.sendall(("screendump %s\n" % dump).encode("ascii"))
            deadline = time.monotonic() + 10
            while time.monotonic() < deadline:
                if os.path.isfile(dump) and os.path.getsize(dump) > 10000:
                    break
                time.sleep(0.2)
            if not os.path.isfile(dump) or os.path.getsize(dump) < 10000:
                raise RuntimeError("QEMU screendump missing after gui")
            with open(dump, "rb") as handle:
                magic = handle.readline().strip()
                dims = handle.readline()
                while dims.startswith(b"#"):
                    dims = handle.readline()
                _maxv = handle.readline()
                parts = dims.split()
                width = int(parts[0])
                height = int(parts[1])
            say("screendump %s %dx%d" % (magic.decode("ascii", "replace"), width, height))
            if width < 1000 or height < 700:
                raise RuntimeError("expected VBE 1024x768, got %sx%s" % (width, height))

            # First relative move initializes guest cursor near FB center.
            say("PS/2 relative init nudge ...")
            move_screen(qmp, 2, 2)
            cur_x = width // 2
            cur_y = height // 2

            # Edge clamping: flood past bottom-right; guest must clamp to (w-1,h-1).
            say("PS/2 edge clamp flood (bottom-right) ...")
            for _ in range(30):
                send_rel_raw(qmp, 80, 80)
                time.sleep(0.03)
            time.sleep(0.4)
            cur_x = width - 1
            cur_y = height - 1
            say("assumed clamped cursor at %d,%d" % (cur_x, cur_y))

            # Also flood past top-left then re-clamp bottom-right so both edges exercise.
            say("PS/2 edge clamp flood (top-left) ...")
            for _ in range(30):
                send_rel_raw(qmp, -80, -80)
                time.sleep(0.03)
            time.sleep(0.4)
            cur_x = 0
            cur_y = 0
            say("PS/2 edge clamp flood (bottom-right again) ...")
            for _ in range(35):
                send_rel_raw(qmp, 80, 80)
                time.sleep(0.03)
            time.sleep(0.4)
            cur_x = width - 1
            cur_y = height - 1

            cur_x, cur_y = click_dock_shell_ps2(qmp, proc, width, height, cur_x, cur_y)

            say("typing console ...")
            send_command_until(
                monitor, "console", "chrome=text", proc, mode="getc", wait_prompt=True
            )
        say("QEMU OS-UI GUI PS/2 fallback contract passed.")
        return 0
    finally:
        if qmp is not None:
            qmp.close()
        if monitor is not None:
            monitor.close()
        terminate(proc)
        try:
            os.remove(MON_SOCK)
        except OSError:
            pass
        try:
            os.remove(QMP_SOCK)
        except OSError:
            pass


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU OS-UI GUI PS/2 failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
