#!/usr/bin/env python3
"""Lot 46 PR B (US-G16, US-G02): editor Save As of a 64 KiB file on the
real OS, byte exact on the disk and after a cold reboot.

Boot 1 (VBE desktop): /edit /samples/big64.txt (1024 lines, 65536 bytes)
opens the file in full (user heap); ESC a Save As /big64s.txt writes it
through the chunked atadriver relay into the v4 overlay image (LBA 0-63
plus the extension area at LBA 4225-4352, after the FAT16 volume and the
journals). /edit /samples/big64p.txt (65537 bytes) then Save As /over.txt
is refused with the store error shown (no file, no cut copy).
Host check: the disk image is parsed (v4 header, records); /big64s.txt is
equal to the sample byte for byte and the FAT16 volume (LBA 64-4221) is
byte identical to the fresh fixture: the overlay never wrote into it.
Boot 2 (same disk, console): stat gives 65536 bytes, grep finds the last
line, /over.txt does not exist.
"""
from __future__ import print_function

import os
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-gui-bigsave-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-bigsave-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-bigsave-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-gui-bigsave-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_gui_ux5 as ux  # noqa: E402

SAMPLE = os.path.join(HERE, "..", "..", "initrd_content", "samples", "big64.txt")
SECTOR = 512
EXT_LBA, EXT_SECTORS = 4225, 128
FAT_FIRST, FAT_LAST = 64, 4221


def say(msg):
    g.say(msg)


def boot(gui):
    for path in (g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    err = open(os.devnull, "wb")
    proc = subprocess.Popen([
        "qemu-system-i386", "-cpu", "pentium3", "-kernel", g.KERNEL,
        "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
        "-usb", "-device", "usb-tablet",
        "-serial", "file:" + g.LOG, "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
        "-qmp", "unix:%s,server,nowait" % g.QMP_SOCK,
        "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
    ] + g.qemu_disk_args(), cwd=g.ROOT, stdout=err, stderr=err)
    g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
    g.wait_for(proc, "usbdriver tablet enumerated port ", g.CMD_TIMEOUT)
    monitor = g.monitor_connect()
    qmp = g.qmp_connect()
    time.sleep(0.6)
    if gui:
        g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
        g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
    return proc, monitor, qmp


def save_as(monitor, proc, src, dst, src_len):
    start = ux.slash(monitor, proc, "/edit " + src, "osui edit open file=" + src)
    g.wait_for(proc, "osui edit lines=", g.CMD_TIMEOUT, start)
    start = len(g.log_text())
    ux.key(monitor, "esc")
    ux.key(monitor, "a")
    for _ in range(src_len):
        ux.key(monitor, "backspace", 0.08)
    ux.type_text(monitor, dst)
    ux.key(monitor, "ret")
    deadline = time.time() + 240
    while time.time() < deadline:
        tail = g.log_text()[start:]
        if "osui edit saved file=" + dst in tail or "osui edit save error=" in tail:
            break
        time.sleep(0.5)
    tail = g.log_text()[start:]
    return start, tail


def close_editor(monitor, proc, qmp):
    start = len(g.log_text())
    ux.key(monitor, "esc")
    ux.key(monitor, "shift-q")
    g.wait_for(proc, "osui edit close file=", g.CMD_TIMEOUT, start)
    ux.desktop_focus(qmp)


def parse_overlay(disk):
    with open(disk, "rb") as h:
        img = h.read()
    base = img[:64 * SECTOR]
    magic, version, nodes, recs, ext, ext_n = struct.unpack_from("<IIIIII", base, 0)
    if magic != 0x564F4941 or version != 4 or ext != EXT_LBA or ext_n != EXT_SECTORS:
        raise RuntimeError("disk overlay is not a v4 image at LBA %d: %r" % (EXT_LBA, (hex(magic), version, ext, ext_n)))
    full = base + img[ext * SECTOR:(ext + ext_n) * SECTOR]
    off, files, cur, want = 32, {}, None, 0
    for _ in range(recs):
        typ, plen, dlen, size = struct.unpack_from("<BBHI", full, off)
        path = full[off + 8:off + 8 + plen].decode("ascii")
        data = full[off + 8 + plen:off + 8 + plen + dlen]
        off += 8 + plen + dlen
        if typ == 0:
            cur, want = path, size
            files[cur] = bytearray(data)
        elif typ == 2:
            files[cur] += data
        if cur and typ != 1 and len(files[cur]) == want:
            cur = None
    return img, files, off


def main():
    os.makedirs(g.LOG_DIR, exist_ok=True)
    try:
        os.remove(g.LOG)
    except OSError:
        pass
    g.prepare_test_disk()
    with open(g.TEST_DISK, "rb") as h:
        fresh = h.read()
    if len(fresh) < (EXT_LBA + EXT_SECTORS) * SECTOR:
        raise RuntimeError("fixture disk has no room for the extension area")
    sample = open(SAMPLE, "rb").read()
    if len(sample) != 65536:
        raise RuntimeError("sample is not 64 KiB")
    proc = monitor = qmp = None
    try:
        proc, monitor, qmp = boot(True)
        out = g.normalized_log(g.log_text())
        if "atadriver overlay ext lba=%d sectors=%d fat16 end=4222" % (EXT_LBA, EXT_SECTORS) not in out:
            raise RuntimeError("atadriver did not enable the extension area after the FAT16 volume")
        say("atadriver: extension area LBA %d-%d (FAT16 volume ends at 4221, journals 4222-4224)" %
            (EXT_LBA, EXT_LBA + EXT_SECTORS - 1))
        start, tail = save_as(monitor, proc, "/samples/big64.txt", "/big64s.txt", len("/samples/big64.txt"))
        if "osui edit saved file=/big64s.txt" not in tail:
            raise RuntimeError("Save As of 64 KiB failed: %r" % tail[-400:])
        say("boot 1: Save As /big64s.txt of the 65536-byte file: saved")
        close_editor(monitor, proc, qmp)
        start, tail = save_as(monitor, proc, "/samples/big64p.txt", "/over.txt", len("/samples/big64p.txt"))
        if "osui edit save error=" not in tail or "osui edit saved file=/over.txt" in tail:
            raise RuntimeError("Save As of 65537 bytes was not refused: %r" % tail[-400:])
        say("boot 1: Save As /over.txt of 65537 bytes refused, error shown in the editor")
        g.screendump_rgb(monitor, "qemu-gui-lot46-bigsave-refused.ppm")
        close_editor(monitor, proc, qmp)
        time.sleep(3.0)   # last atadriver flush (writethrough disk)
    finally:
        for c in (qmp, monitor):
            if c is not None:
                c.close()
        g.terminate(proc)
    img, files, used = parse_overlay(g.TEST_DISK)
    got = bytes(files.get("big64s.txt", files.get("/big64s.txt", b"")))
    if got != sample:
        raise RuntimeError("disk copy of /big64s.txt differs (%d bytes, files %r)" % (len(got), sorted(files)))
    if any(k.strip("/") == "over.txt" for k in files):
        raise RuntimeError("/over.txt reached the disk")
    if used <= 64 * SECTOR:
        raise RuntimeError("image does not use the extension area (%d bytes)" % used)
    if img[FAT_FIRST * SECTOR:(FAT_LAST + 1) * SECTOR] != fresh[FAT_FIRST * SECTOR:(FAT_LAST + 1) * SECTOR]:
        raise RuntimeError("the FAT16 volume (LBA 64-4221) changed: the overlay wrote into it")
    say("host: v4 image of %d bytes over LBA 0-63 + %d-%d, /big64s.txt byte exact, FAT16 LBA 64-4221 untouched" %
        (used, EXT_LBA, EXT_LBA + EXT_SECTORS - 1))
    proc = monitor = qmp = None
    try:
        proc, monitor, qmp = boot(False)
        g.send_command_until(monitor, "stat /big64s.txt", "stat file /big64s.txt 65536", proc)
        g.send_command_until(monitor, "grep -r 1023 /big64s.txt", "grep -r hits 1 total 1 ", proc)
        g.send_command_until(monitor, "stat /over.txt", "stat", proc)
        out = g.normalized_log(g.log_text())
        if "stat file /over.txt" in out:
            raise RuntimeError("/over.txt exists after reboot")
        say("boot 2: /big64s.txt is 65536 bytes after the cold reboot, last line found, /over.txt absent")
    finally:
        for c in (qmp, monitor):
            if c is not None:
                c.close()
        g.terminate(proc)
    say("QEMU Lot 46 large save contract passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
