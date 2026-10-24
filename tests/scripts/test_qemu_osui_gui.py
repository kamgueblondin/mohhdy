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


def _model_packed():
    import tarfile
    try:
        with tarfile.open(os.environ.get("INITRD", os.path.join(ROOT, "my_initrd.tar"))) as tar:
            return any(n.endswith("models/gpt2_124M.bin") for n in tar.getnames())
    except (OSError, tarfile.TarError):
        return False


MODEL_PACKED = _model_packed()
AI_GUI_MARKER = "ai_status=ready" if MODEL_PACKED else "llm=gpt2_missing ai_status=no_model"
AI_GUI_NOTE = "llm=gpt2_local ai=ready" if MODEL_PACKED else "llm=gpt2_missing ai=no_model"


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
    menus = ["Chat", "Browser-OS", "Shell", "Admin", "Support", "Statut", "Fichiers", "Editeur"]
    mx = 122
    ty = (bar_h - 8) // 2
    gap = 8 if width < 720 else 14
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
        ww = clampi(width - (420 if width >= 780 else 48), 240, width - 48)
        # close button of the shared window manager (kernel/gfx_wm.c):
        # rightmost of the title bar, 14 px, title bar 30 px under a 2 px border
        traffic = (wx + ww - 2 - 18 + 7, wy + 2 + (30 - 14) // 2 + 7)
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
    dock_w = 8 * 44 + 16
    if dock_w > width - 16:
        dock_w = width - 16
    dock_x = (width - dock_w) // 2
    dock_y = height - 48
    if dock_y < bar_h + 8:
        dock_y = height - 36
    docks = []
    for i in range(8):
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


def send_command_once(client, command, proc, mode, enter=True):
    aliases = {" ": "spc", "-": "minus", ".": "dot", "/": "slash", "=": "equal"}
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
    if enter:
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
                wait_for(proc, "[0m : ", CMD_TIMEOUT, start)
            return start
        if mode == "gets" and "SYS_GETS: ligne lue: " in output:
            raise CommandEchoMismatch("command echo altered: %s" % command)
        time.sleep(0.1)
    raise RuntimeError("timeout waiting for command echo: %s" % command)


def read_ppm(path):
    with open(path, "rb") as handle:
        data = handle.read()
    fields = []
    pos = 0
    while len(fields) < 4:
        while data[pos:pos + 1].isspace():
            pos += 1
        if data[pos:pos + 1] == b"#":
            pos = data.index(b"\n", pos) + 1
            continue
        end = pos
        while not data[end:end + 1].isspace():
            end += 1
        fields.append(data[pos:end])
        pos = end
    pos += 1
    if fields[0] != b"P6":
        raise RuntimeError("expected P6 screendump, got %r" % fields[0])
    return int(fields[1]), int(fields[2]), data[pos:]


def screendump_rgb(monitor, name):
    dump = os.path.join(LOG_DIR, name)
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
    time.sleep(0.3)
    return read_ppm(dump)


def color_box(width, height, rgb, color):
    """Count and bounding box of the pixels of one exact color."""
    want = bytes(((color >> 16) & 255, (color >> 8) & 255, color & 255))
    n = 0
    x0 = y0 = 1 << 30
    x1 = y1 = -1
    for y in range(height):
        row = rgb[y * width * 3:(y + 1) * width * 3]
        x = row.find(want)
        while x >= 0:
            if x % 3 == 0:
                px = x // 3
                n += 1
                x0 = min(x0, px)
                x1 = max(x1, px)
                y0 = min(y0, y)
                y1 = max(y1, y)
                x = row.find(want, x + 3)
            else:
                x = row.find(want, x + 1)
    if n == 0:
        return 0, 0, 0
    return n, x1 - x0 + 1, y1 - y0 + 1


MUI_ICON_COLORS = (0x2E7D32, 0x263238, 0x1565C0, 0xEF6C00, 0x6A1B9A, 0x00838F, 0x546E7A, 0xC62828)


def check_mobile_home_pixels(monitor):
    """Lot 9 (US-077): the 320x448 phone home screen painted by the kernel
    on the VBE framebuffer from the guest mobile display list: wallpaper
    320x368 around eight 64x64 icons, one per app color."""
    width, height, rgb = screendump_rgb(monitor, "qemu-osui-gui-mobile-home.ppm")
    n, bw, bh = color_box(width, height, rgb, 0x0D47A1)
    say("mobile home: wallpaper %d px box %dx%d" % (n, bw, bh))
    if (bw, bh) != (320, 368):
        raise RuntimeError("mobile home: wallpaper box %dx%d, expected 320x368" % (bw, bh))
    # 320*368 minus 8 icons of 64x64, minus the white label glyphs
    if n > 320 * 368 - 8 * 64 * 64 or n < 320 * 368 - 8 * 64 * 64 - 8 * 8 * 64:
        raise RuntimeError("mobile home: wallpaper pixel count %d" % n)
    for c in MUI_ICON_COLORS:
        cn, cw, ch = color_box(width, height, rgb, c)
        if cn < 64 * 64 or cw < 64 or ch < 64:
            raise RuntimeError("mobile home: icon #%06x has %d px box %dx%d" % (c, cn, cw, ch))


def check_mobile_app_pixels(monitor):
    """After the tap on the chat icon: green app bar 320x48, no wallpaper."""
    width, height, rgb = screendump_rgb(monitor, "qemu-osui-gui-mobile-app.ppm")
    n, bw, bh = color_box(width, height, rgb, 0x2E7D32)
    wall, _, _ = color_box(width, height, rgb, 0x0D47A1)
    say("mobile app: app bar %d px box %dx%d, wallpaper %d px" % (n, bw, bh, wall))
    if (bw, bh) != (320, 48) or n < 320 * 48 - 4 * 64 or n > 320 * 48:
        raise RuntimeError("mobile app: app bar %d px box %dx%d, expected 320x48" % (n, bw, bh))
    if wall != 0:
        raise RuntimeError("mobile app: wallpaper still on screen (%d px)" % wall)


def check_web_pixels(monitor):
    """Lot 6 (US-031): find the #hero box (pure red, 208x48 in the layout)
    and the yellow centered paragraph and the blue border on the dump."""
    dump = os.path.join(LOG_DIR, "qemu-osui-gui-web.ppm")
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
    time.sleep(0.3)
    width, height, rgb = read_ppm(dump)
    xs, ys = [], []
    yellow = blue = 0
    for y in range(height):
        row = rgb[y * width * 3:(y + 1) * width * 3]
        for x in range(width):
            px = row[x * 3:x * 3 + 3]
            if px == b"\xff\x00\x00":
                xs.append(x)
                ys.append(y)
            elif px == b"\xff\xff\x00":
                yellow += 1
            elif px == b"\x00\x00\xff":
                blue += 1
    if not xs:
        raise RuntimeError("web pane: no #hero pixels on the framebuffer")
    box_w = max(xs) - min(xs) + 1
    box_h = max(ys) - min(ys) + 1
    say("web pane: hero box %dx%d at %d,%d, yellow %d px, blue %d px" % (box_w, box_h, min(xs), min(ys), yellow, blue))
    if (box_w, box_h) != (208, 48):
        raise RuntimeError("web pane: hero box %dx%d, expected 208x48" % (box_w, box_h))
    # the white text inside the red box leaves holes, so count is below 208*48
    if len(xs) < 208 * 48 - 72 * 8 or len(xs) >= 208 * 48:
        raise RuntimeError("web pane: hero pixel count %d" % len(xs))
    if yellow < 600 or blue < 3 * 300:
        raise RuntimeError("web pane: yellow %d blue %d" % (yellow, blue))


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
            wait_for(proc, "[0m : ", BOOT_TIMEOUT)
            wait_for(proc, "[0m : ", BOOT_TIMEOUT)
            wait_for(
                proc,
                "usbdriver tablet enumerated port ",
                CMD_TIMEOUT,
                fail_needles=[
                    "usbdriver enumeration failed",
                    "usbdriver claim refused",
                    "usbdriver no device on the root ports",
                    "USB Tablet: Controller UHCI non trouve",
                    "BAR4 non-IO",
                ],
            )
            # Lot 8 (US-001): the tablet is enumerated by the Ring 3
            # usbdriver; the kernel only owns the schedule and checks TDs.
            log = log_text()
            if "[USB] Ring 3 usbdriver claimed the UHCI controller pid " not in log:
                raise RuntimeError("USB controller not claimed by the Ring 3 driver")
            monitor = monitor_connect()
            qmp = qmp_connect()
            time.sleep(0.6)

            say("typing gui-status ...")
            send_command_until(monitor, "gui-status", "canonical=gui", proc)
            # Lot 6 (US-031, US-043, US-045): guest web engine from the
            # text console: layout, inspector, diagnostics console, timing.
            say("typing web-render /web/demo.html ...")
            start = send_command_until(monitor, "web-render /web/demo.html", "web-render ok", proc)
            out = normalized_log(log_text()[start:])
            for needle in ("osui web render file=/web/demo.html",
                           "web title Mohhdy web engine demo",
                           "web item rect 8 8 208x48 #00ff0000",
                           "web item text 12 13 72x8 #00ffffff \"Mohhdy OS\"",
                           "web item border",
                           "\"centered on yellow\""):
                if needle not in out:
                    raise RuntimeError("web-render missing %r: %s" % (needle, out[-1500:]))
            if "never shown" in out or "ignored" in out:
                raise RuntimeError("hidden or script content painted: %s" % out[-1500:])
            start = send_command_until(monitor, "web-inspect /web/demo.html hero", "inspect div#hero", proc)
            out = normalized_log(log_text()[start:])
            if "inspect div#hero display block box 8 8 208x48 color #ffffff bg #ff0000" not in out:
                raise RuntimeError("web-inspect hero: %s" % out[-800:])
            start = send_command_until(monitor, "web-console /web/demo.html", "web-console ok", proc)
            out = normalized_log(log_text()[start:])
            # Lot 7 (US-039): the sanitizer removes the script before the
            # engine sees it (the engine skip stays as a second line).
            if "scripts_skipped 0 sanitized 1" not in out:
                raise RuntimeError("web-console: %s" % out[-800:])
            start = send_command_until(monitor, "web-render /web/hostile.html", "web-render ok", proc)
            out = normalized_log(log_text()[start:])
            if (" sanitized 9" not in out or "\"clean paragraph\"" not in out or "pwned" in out
                    or "bad link" not in out or "#0000ff00" not in out):
                raise RuntimeError("hostile page: %s" % out[-1500:])
            # US-040/041/042 in the engine pipeline, US-044 audit
            start = send_command_until(monitor, "web-render /web/theme.html", "web-render ok", proc)
            out = normalized_log(log_text()[start:])
            if ("components 1 media_kept 1 media_dropped 1 vars 2" not in out
                    or "web item rect 8 " not in out or "#00ff00ff" not in out or "#00800000" not in out):
                raise RuntimeError("theme pipeline: %s" % out[-1500:])
            start = send_command_until(monitor, "web-console /web/theme.html", "web-console a11y issues", proc)
            out = normalized_log(log_text()[start:])
            if "web-console a11y issues 0" not in out:
                raise RuntimeError("theme a11y: %s" % out[-800:])
            start = send_command_until(monitor, "web-perf /web/demo.html 5", "web-perf runs 5", proc)
            wait_for(proc, " budget ", CMD_TIMEOUT, start)
            out = normalized_log(log_text()[start:])
            if not re.search(r"web-perf runs 5 parse_ticks \d+ style_ticks \d+ layout_ticks \d+ ms_per_render \d+ nodes \d+", out):
                raise RuntimeError("web-perf: %s" % out[-800:])
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
            # The Shell is the console itself: the desktop loop ends and the
            # kernel draws the console in a VBE window (one shell.c prompt).
            step = len(log_text())
            click_dock_and_wait(
                qmp,
                proc,
                width,
                height,
                2,
                "osui gui line=/shell",
                extra_marker="chrome=vbe_term",
            )
            wait_for(proc, "gfx term on fb ", CMD_TIMEOUT, step)
            wait_for(proc, "[0m : ", CMD_TIMEOUT, step)
            if "usbdriver first report applied" not in log_text():
                raise RuntimeError("tablet clicks did not go through the Ring 3 usbdriver")
            say("typing whoami in the console terminal window ...")
            send_command_until(monitor, "whoami", "whoami ok", proc)
            say("typing ai hello in the console terminal window ...")
            send_command_until(monitor, "ai hello", "[IA]", proc, wait_prompt=not MODEL_PACKED)
            if MODEL_PACKED:
                # The real GPT-2 takes 30-40 s under TCG: ESC goes to the
                # console app, as in text mode, and cancels it.
                step = len(log_text())
                send_key(monitor, "esc")
                wait_for(proc, "generation annulee (Echap)", CMD_TIMEOUT, step)
                wait_for(proc, "[0m : ", CMD_TIMEOUT, step)
                say("ESC cancelled ai in the console terminal window")
            say("typing gui to go back to the desktop ...")
            send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
            wait_for(proc, "OSUI-SNAP", CMD_TIMEOUT)
            say("typing /browser in gui ...")
            send_command_until(
                monitor, "/browser", "chat_mode=float", proc, mode="getc", wait_prompt=False
            )
            say("typing /p2p in gui ...")
            send_command_until(
                monitor, "/p2p", "pane=p2p", proc, mode="getc", wait_prompt=False
            )
            say("typing /collab in gui ...")
            send_command_until(
                monitor, "/collab", "pane=collab", proc, mode="getc", wait_prompt=False
            )
            say("typing /prod in gui ...")
            send_command_until(
                monitor, "/prod", "pane=prod", proc, mode="getc", wait_prompt=False
            )
            say("typing /center in gui ...")
            send_command_until(
                monitor, "/center", "chat_mode=center", proc, mode="getc", wait_prompt=False
            )
            # ESC on a half typed line clears it and keeps the desktop.
            step = len(log_text())
            for key in ("x", "y", "z"):
                send_key(monitor, key)
                time.sleep(0.2)
            send_key(monitor, "esc")
            wait_for(proc, "osui gui input cleared", CMD_TIMEOUT, step)
            if "osui gui exit" in log_text()[step:]:
                raise RuntimeError("ESC with typed input left the desktop")
            say("ESC cleared the typed line, desktop kept")
            say("typing media-play audio stream1 in gui ...")
            send_command_until(
                monitor, "media-play audio stream1", "status=playing", proc, mode="getc", wait_prompt=False
            )
            wait_for(proc, "media=playing:stream1", CMD_TIMEOUT)
            say("typing media-stop in gui ...")
            send_command_until(
                monitor, "media-stop", "status=stopped", proc, mode="getc", wait_prompt=False
            )
            # Roadmap step 2, end to end inside the open GUI: request to the
            # local GPT-2 session, its answer (or, without packed weights,
            # the distinct no-model state), explicit session end, close.
            say("typing chat ai a in gui ...")
            ai_start = len(log_text())
            send_command_until(
                monitor, "chat ai a", AI_GUI_MARKER, proc, mode="getc", wait_prompt=False
            )
            wait_for(proc, "osui stage mode=", CMD_TIMEOUT, ai_start)
            wait_for(proc, AI_GUI_NOTE, CMD_TIMEOUT, ai_start)
            say("typing session-end in gui ...")
            end_start = len(log_text())
            send_command_until(
                monitor, "session-end", "history=cleared", proc, mode="getc", wait_prompt=False
            )
            wait_for(proc, "ai=ended", CMD_TIMEOUT, end_start)
            # US-050: full-screen pm-ide on the desktop. The editor owns
            # every key (ESC is its command prefix), saves to the shell
            # store, and checks/runs on the PromptMessage VM.
            say("typing pm-ide ide.pm in gui ...")
            send_command_until(
                monitor, "pm-ide ide.pm", "osui pm-ide open fullscreen", proc, mode="getc", wait_prompt=False
            )
            wait_for(proc, " ide=ide.pm ide_ln=1", CMD_TIMEOUT)
            ide_start = len(log_text())
            send_command_once(monitor, "expect 4 == 4", proc, "getc")
            wait_for(proc, " ide=ide.pm ide_ln=2 ide_col=1 ide_lines=2 ide_mod=1", CMD_TIMEOUT, ide_start)
            # Tab completion: "sa" + Tab gives "say" (say/says common prefix).
            send_command_once(monitor, "sa", proc, "getc", enter=False)
            tab_at = len(log_text())
            send_key(monitor, "tab")
            wait_for(proc, " ide_tabs=1", CMD_TIMEOUT, tab_at)
            wait_for(proc, "status=complete: say says", CMD_TIMEOUT, tab_at)
            send_command_once(monitor, " 7", proc, "getc")
            wait_for(proc, " ide_ln=3 ide_col=1 ide_lines=3 ide_mod=1", CMD_TIMEOUT, tab_at)
            for key, marker in (("w", "osui pm-ide saved file=ide.pm"),
                                ("c", "osui pm-ide check ok statements 2"),
                                ("r", "osui pm-ide run ok line 2 prints 1 expects 1 failed 0 output 1"),
                                ("q", "osui pm-ide quit file=ide.pm")):
                step = len(log_text())
                send_key(monitor, "esc")
                time.sleep(0.3)
                send_key(monitor, key)
                wait_for(proc, marker, CMD_TIMEOUT, step)
                if key == "r":
                    # Program output pane: the say line is in the editor.
                    wait_for(proc, " ide_out=1 ide_pane=1 ", CMD_TIMEOUT, step)
            quit_at = log_text().find("osui pm-ide quit file=ide.pm", ide_start)
            wait_for(proc, "OSUI-SNAP", CMD_TIMEOUT, quit_at)
            snap = log_text()[quit_at:].split("OSUI-SNAP", 1)[1].split("\n", 1)[0]
            if " ide=" in snap or "pane=" not in snap:
                raise RuntimeError("pm-ide still active after ESC q: %s" % snap)
            say("pm-ide fullscreen: completion, saved, checked, ran with output pane, quit")
            # Lot 6 (US-031): the page painted by the guest web engine in the
            # OS-UI web pane, checked on the QEMU framebuffer pixels.
            say("typing web /web/demo.html in gui ...")
            send_command_until(
                monitor, "web /web/demo.html", "osui web render file=/web/demo.html", proc,
                mode="getc", wait_prompt=False
            )
            time.sleep(1.0)
            check_web_pixels(monitor)
            step = len(log_text())
            send_key(monitor, "esc")
            wait_for(proc, "osui web close file=/web/demo.html", CMD_TIMEOUT, step)
            # Lot 9 (US-077): the mobile shell on the OS-UI VBE screen.
            say("typing mobile-ui 320x448 in gui ...")
            send_command_until(
                monitor, "mobile-ui 320x448", "mobile-ui vbe pane web items", proc,
                mode="getc", wait_prompt=False
            )
            wait_for(proc, "mobile-ui ok form phone portrait grid 4x2 icon 64", CMD_TIMEOUT, step)
            time.sleep(1.0)
            check_mobile_home_pixels(monitor)
            step = len(log_text())
            send_command_until(
                monitor, "mobile-ui app chat", "title phone chat", proc,
                mode="getc", wait_prompt=False
            )
            wait_for(proc, "mobile-ui tap action open screen app app chat", CMD_TIMEOUT, step)
            time.sleep(1.0)
            check_mobile_app_pixels(monitor)
            step = len(log_text())
            send_key(monitor, "esc")
            wait_for(proc, "osui web close file=mobile", CMD_TIMEOUT, step)
            say("mobile shell rendered on the VBE screen: home grid, tap, app bar")
            say("typing console ...")
            send_command_until(
                monitor, "console", "chrome=text", proc, mode="getc", wait_prompt=True
            )
            say("typing gui-status after close ...")
            send_command_until(monitor, "gui-status", "gui_live=false", proc)
            # The file the desktop editor saved is in the shell store.
            send_command_until(monitor, "pm-check ide.pm", "pm-check ok ide.pm", proc)
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
