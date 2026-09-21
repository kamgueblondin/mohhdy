#!/usr/bin/env python3
"""QEMU smoke: commande canonique `gui` entre le bureau VBE QEMU.

Hors make integration-qemu. Complements test_qemu_osui_runtime.py.
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
LOG = os.environ.get("OSUI_GUI_LOG", os.path.join(LOG_DIR, "qemu-osui-gui-serial.log"))
QEMU_ERR = os.environ.get("OSUI_GUI_ERR", os.path.join(LOG_DIR, "qemu-osui-gui-stderr.log"))
MON_SOCK = os.environ.get("OSUI_GUI_MON_SOCK", os.path.join(LOG_DIR, "qemu-osui-gui-monitor.sock"))
QMP_SOCK = os.environ.get("OSUI_GUI_QMP_SOCK", os.path.join(LOG_DIR, "qemu-osui-gui-qmp.sock"))
TEST_DISK = os.environ.get("OVERLAY_DISK", os.path.join(LOG_DIR, "qemu-osui-gui-overlay.img"))
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "75"))
CMD_TIMEOUT = float(os.environ.get("CMD_TIMEOUT", "20"))
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.05"))
KEY_HOLD_MS = int(os.environ.get("KEY_HOLD_MS", "10"))
KEY_ECHO_TIMEOUT = float(os.environ.get("KEY_ECHO_TIMEOUT", "3"))
KEY_DUPLICATE_SETTLE_DELAY = float(os.environ.get("KEY_DUPLICATE_SETTLE_DELAY", "0.25"))
KEY_CHAR_RETRIES = int(os.environ.get("KEY_CHAR_RETRIES", "3"))


def say(message):
    sys.stdout.write(message + "\n")
    sys.stdout.flush()


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(proc, needle, timeout, start=0, fail_needles=None):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped unexpectedly; log tail:\n%s" % log_text()[-2000:])
        output = normalized_log(log_text()[start:])
        if fail_needles:
            for fail_needle in fail_needles:
                if fail_needle in output:
                    raise RuntimeError("USB tablet enumeration failed with %r; log tail:\n%s" % (fail_needle, log_text()[-2000:]))
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


class CommandEchoMismatch(RuntimeError):
    pass


def normalized_log(output):
    output = re.sub(r"(?<=\w)TIMER_ALIVE: tick=\d+\+?\r?\n(?=\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?\r?\n(?=\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?\r?\n(?=\s+\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?", "", output)
    output = re.sub(r"\[SCHED\] switching to task \d+\r?\n?", "", output)
    return output


def send_key(client, key):
    client.sendall(("sendkey %s %d\n" % (key, KEY_HOLD_MS)).encode("ascii"))




def tablet_abs_xy(px, py, width, height):
    """Map guest framebuffer pixels to QEMU usb-tablet absolute 0..32767."""
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


def desktop_hit_centers(width, height, chat_mode="center", pane=None):
    """Mirror gfx_desktop_get_layout centers used by gfx_desktop_handle_click."""
    bar_h = clampi(height // 22, 28, 40)
    menus = ["Chat", "Browser-OS", "Shell", "Admin", "Support", "Statut", "FS"]
    mx = 122
    ty = (bar_h - 8) // 2
    gap = 14
    flag_w = (8 * 36 + 12) if width >= 920 else ((8 * 16 + 12) if width >= 720 else 8)
    menu = []
    for label in menus:
        lw = len(label) * 8
        if mx + lw > width - flag_w - 8:
            break
        menu.append((mx + lw // 2, ty + 4))
        mx += lw + gap
    stage_badge = (12 + 48, bar_h + 8 + 9)
    traffic = None
    if pane:
        wx = 36
        wy = bar_h + 28
        traffic = (wx + 8 + 21, wy + 8 + 8)
    if chat_mode == "float":
        chat_w = clampi(width // 3, 220, 360)
        chat_h = clampi(height // 2, 180, 400)
    else:
        chat_w = clampi(width // 2, 240, 640)
        chat_h = clampi((height * 5) // 12, 160, 360)
    if chat_w > width - 24:
        chat_w = width - 24
    if chat_h > height - bar_h - 56:
        chat_h = height - bar_h - 56
    if chat_h < 120:
        chat_h = 120
    if chat_mode == "float":
        chat_x = width - chat_w - 16
        chat_y = height - chat_h - 70
    else:
        chat_x = (width - chat_w) // 2
        chat_y = bar_h + (height // 14)
        if chat_y + chat_h > height - 52:
            chat_y = bar_h + 8
    send_btn = (chat_x + 14 + 44, chat_y + chat_h - 34 + 11)
    dock_w = 7 * 44 + 16
    if dock_w > width - 16:
        dock_w = width - 16
    dock_x = (width - dock_w) // 2
    dock_y = height - 48
    if dock_y < bar_h + 8:
        dock_y = height - 36
    docks = []
    for i in range(7):
        docks.append((dock_x + 8 + i * 44 + 18, dock_y + 4 + 16))
    return {
        "menu": menu,
        "stage_badge": stage_badge,
        "send_btn": send_btn,
        "traffic": traffic,
        "docks": docks,
    }


def dock_icon_center(width, height, index):
    """Match gfx_desktop_get_layout dock icons for 7 icons of 36x32."""
    return desktop_hit_centers(width, height)["docks"][index]


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
    time.sleep(0.05)
    try:
        client.recv(8192)
    except socket.timeout:
        pass


def click_tablet(qmp, px, py, width, height):
    """Absolute tablet click via QMP (HMP mouse_* is unreliable after screendump)."""
    ax, ay = tablet_abs_xy(px, py, width, height)
    # Gaps must exceed guest UHCI poll+re-arm (timer 100Hz).
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



def wait_for_empty_gui_line(proc, timeout, start=0):
    """Assert serial logged an empty enter: 'osui gui line=' with nothing after '='."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped unexpectedly; log tail:\n%s" % log_text()[-2000:])
        chunk = normalized_log(log_text()[start:])
        if re.search(r"osui gui line=\r?\n", chunk):
            time.sleep(0.35)
            return
        time.sleep(0.15)
    raise RuntimeError("timeout waiting for empty osui gui line; log tail:\n%s" % log_text()[-2000:])


def click_xy_and_wait(qmp, proc, width, height, px, py, label, line_marker, extra_marker=None):
    """Click absolute guest pixel and assert gfx_desktop_handle_click serial reaction."""
    start = len(log_text())
    say("click %s at %d,%d (tablet abs via QMP) ..." % (label, px, py))
    last_error = None
    for attempt in range(3):
        click_tablet(qmp, px, py, width, height)
        try:
            wait_for(proc, line_marker, min(CMD_TIMEOUT, 8.0), start)
            if extra_marker:
                wait_for(proc, extra_marker, CMD_TIMEOUT, start)
            return
        except RuntimeError as err:
            last_error = err
            say("click attempt %d missed; retrying" % (attempt + 1))
            start = len(log_text())
    raise last_error


def click_dock_and_wait(qmp, proc, width, height, index, line_marker, extra_marker=None):
    """Click a dock icon and assert gfx_desktop_handle_click serial reaction."""
    px, py = dock_icon_center(width, height, index)
    click_xy_and_wait(
        qmp, proc, width, height, px, py, "dock[%d]" % index, line_marker, extra_marker
    )


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
            raise CommandEchoMismatch("command echo altered: %s" % command)
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
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", KERNEL,
                "-initrd", INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet",
                "-serial", "file:" + LOG, "-monitor", "unix:%s,server,nowait" % MON_SOCK,
                "-qmp", "unix:%s,server,nowait" % QMP_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ] + qemu_disk_args(), cwd=ROOT, stdout=err, stderr=err)
            wait_for(proc, "(-.-)", BOOT_TIMEOUT)
            wait_for(proc, "SYS_GETS: Debut", BOOT_TIMEOUT)
            wait_for(
                proc,
                "USB Tablet: Controller UHCI initialise et enumere",
                CMD_TIMEOUT,
                fail_needles=[
                    "Enumeration non terminee",
                    "Controller UHCI non trouve",
                    "BAR4 non-IO",
                    "Aucun peripherique USB detecte",
                ],
            )
            log = log_text()
            if "Enumeration non terminee" in log or "Controller UHCI non trouve" in log:
                raise RuntimeError("USB tablet enumeration failed in serial log")
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
            dump = os.path.join(LOG_DIR, "qemu-osui-gui-desktop.ppm")
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
                rgb = handle.read(3)
            say("screendump %s %dx%d" % (magic.decode("ascii", "replace"), width, height))
            if width < 1000 or height < 700:
                raise RuntimeError("expected VBE 1024x768, got %sx%s" % (width, height))
            if magic not in (b"P6", b"P3"):
                raise RuntimeError("unexpected ppm magic")
            if len(rgb) == 3 and rgb == b"\x00\x00\x00":
                say("corner pixel black (ok if top-left landscape is dark)")
            # OS-UI-G-1: extend hit-test beyond dock[/shell] (menu/stage/send/close).
            hits = desktop_hit_centers(width, height, chat_mode="center", pane=None)
            sx, sy = hits["stage_badge"]
            click_xy_and_wait(
                qmp, proc, width, height, sx, sy, "stage_badge",
                "osui gui line=/stage", extra_marker="osui stage mode=",
            )
            mx, my = hits["menu"][1]
            click_xy_and_wait(
                qmp, proc, width, height, mx, my, "menu[Browser-OS]",
                "osui gui line=/browser", extra_marker="chat_mode=float",
            )
            # After /browser, floating chat + browser pane -> close (traffic) maps to /center.
            hits_pane = desktop_hit_centers(width, height, chat_mode="float", pane="browser")
            tx, ty = hits_pane["traffic"]
            click_xy_and_wait(
                qmp, proc, width, height, tx, ty, "pane_close",
                "osui gui line=/center", extra_marker="chat_mode=center",
            )
            hits = desktop_hit_centers(width, height, chat_mode="center", pane=None)
            ex, ey = hits["send_btn"]
            start_send = len(log_text())
            say("click send_btn at %d,%d (tablet abs via QMP) ..." % (ex, ey))
            last_error = None
            for attempt in range(3):
                click_tablet(qmp, ex, ey, width, height)
                try:
                    wait_for_empty_gui_line(proc, min(CMD_TIMEOUT, 8.0), start_send)
                    last_error = None
                    break
                except RuntimeError as err:
                    last_error = err
                    say("click attempt %d missed; retrying" % (attempt + 1))
                    start_send = len(log_text())
            if last_error is not None:
                raise last_error
            # Dock index 2 is "/shell" (C, B, sh, A, S, i, F). Hit-test via UHCI tablet.
            click_dock_and_wait(
                qmp,
                proc,
                width,
                height,
                2,
                "osui gui line=/shell",
                extra_marker="live_eval=true",
            )
            say("typing /browser in gui ...")
            send_command_until(
                monitor, "/browser", "chat_mode=float", proc, mode="getc", wait_prompt=False
            )
            say("typing /shell in gui ...")
            send_command_until(
                monitor, "/shell", "live_eval=true", proc, mode="getc", wait_prompt=False
            )
            say("typing whoami in gui shell ...")
            send_command_until(
                monitor, "whoami", "whoami ok", proc, mode="getc", wait_prompt=False
            )
            say("typing ai hello in gui shell ...")
            send_command_until(
                monitor, "ai hello", "[IA]", proc, mode="getc", wait_prompt=False
            )
            say("typing /center in gui ...")
            send_command_until(
                monitor, "/center", "chat_mode=center", proc, mode="getc", wait_prompt=False
            )
            say("typing console ...")
            send_command_until(
                monitor, "console", "chrome=text", proc, mode="getc", wait_prompt=True
            )
        say("QEMU OS-UI GUI contract passed.")
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
        print("QEMU OS-UI GUI failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
