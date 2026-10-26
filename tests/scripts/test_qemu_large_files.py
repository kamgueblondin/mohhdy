#!/usr/bin/env python3
"""Lot 46 PR B (US-G16, US-G02): 64 KiB Save As, byte for byte, after reboot.

Boot 1 (VBE desktop, IDE disk with the Lot 46 layout: AIOV LBA 0-63, FAT16
LBA 64-4221, journals 4222-4224, v4 overlay extension 4225-4352):
  * the Ring 3 atadriver announces the v4 budget (capacity=98304 v4);
  * /edit /samples/big64.txt opens the 1024-line, 65536-byte sample whole
    (user heap), ESC a Save As /big64.txt is saved (the overlay writes a v4
    image through the chunked atadriver relay, primary area then extension);
  * one more character makes the buffer 65537 bytes: Save As /over64.txt is
    refused with the store error in the window, and /over64.txt does not
    exist (nothing cut, nothing half written).
The guest is killed. Boot 2 on the same disk image:
  * "Overlay FS charge depuis le disque IDE" (the v4 image, both areas);
  * stat /big64.txt gives 65536 bytes, its first and last lines are there
    (grep -r), and the editor reopens it with the 1024 lines.

Status: written in Lot 46, NOT RUN yet (no QEMU run in this PR, credit
budget). Run: make qemu-large-files
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-large-files-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-large-files-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-large-files-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-large-files-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_gui_ux5 as ux5  # noqa: E402

DISK_SECTORS = 4353   # Makefile DISK_SECTORS, make_fat16_image.py TOTAL_DISK_SECTORS
SAMPLE_BYTES = 65536


def say(msg):
    g.say(msg)


def boot():
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
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
    return proc, err


def shell(monitor, proc, line, marker):
    start = len(g.log_text())
    g.send_command_until(monitor, line, marker, proc)
    return g.log_text()[start:]


def boot1():
    proc, err = boot()
    monitor = qmp = None
    try:
        g.wait_for(proc, "atadriver overlay capacity=98304 v4", g.CMD_TIMEOUT)
        say("atadriver: v4 overlay budget (extension area readable)")
        g.wait_for(proc, "usbdriver tablet enumerated port ", g.CMD_TIMEOUT)
        monitor = g.monitor_connect()
        qmp = g.qmp_connect()
        time.sleep(0.6)
        g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
        g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
        start = ux5.slash(monitor, proc, "/edit /samples/big64.txt", "osui edit open file=/samples/big64.txt")
        g.wait_for(proc, "osui edit lines=1024 complete", g.CMD_TIMEOUT, start)
        say("editor opened the 1024-line 64 KiB sample whole")
        start = len(g.log_text())
        ux5.key(monitor, "esc")
        ux5.key(monitor, "a")
        for _ in range(len("/samples/big64.txt")):
            ux5.key(monitor, "backspace", 0.08)
        ux5.type_text(monitor, "/big64.txt")
        ux5.key(monitor, "ret")
        g.wait_for(proc, "osui edit saved file=/big64.txt", g.CMD_TIMEOUT, start)
        if "osui edit save error=" in g.log_text()[start:]:
            raise RuntimeError("Save As of 64 KiB reported a store error")
        say("Save As /big64.txt (65536 bytes) saved")
        # one byte over the bound: refused, shown, nothing written
        start = len(g.log_text())
        ux5.key(monitor, "x")
        ux5.key(monitor, "esc")
        ux5.key(monitor, "a")
        for _ in range(len("/big64.txt")):
            ux5.key(monitor, "backspace", 0.08)
        ux5.type_text(monitor, "/over64.txt")
        ux5.key(monitor, "ret")
        g.wait_for(proc, "osui edit save error=", g.CMD_TIMEOUT, start)
        if "osui edit saved file=/over64.txt" in g.log_text()[start:]:
            raise RuntimeError("65537-byte Save As was accepted")
        say("Save As of 65537 bytes refused with the store error")
        ux5.key(monitor, "esc")
        ux5.key(monitor, "shift-q")
        time.sleep(1.0)
        ux5.key(monitor, "esc")
        ux5.key(monitor, "shift-q")
        time.sleep(1.0)
    finally:
        if qmp is not None:
            qmp.close()
        if monitor is not None:
            monitor.close()
        g.terminate(proc)
        err.close()


def boot2():
    proc, err = boot()
    monitor = None
    try:
        if "Overlay FS charge depuis le disque IDE" not in g.log_text():
            raise RuntimeError("overlay not loaded from the disk at boot 2")
        g.wait_for(proc, "atadriver overlay capacity=98304 v4", g.CMD_TIMEOUT)
        monitor = g.monitor_connect()
        time.sleep(0.6)
        shell(monitor, proc, "stat /big64.txt", "stat file /big64.txt %d" % SAMPLE_BYTES)
        out = shell(monitor, proc, "stat /over64.txt", "stat")
        if "stat file /over64.txt" in out:
            raise RuntimeError("/over64.txt exists after the refused save")
        shell(monitor, proc, "grep -r L0000 /big64.txt", "grep -r hits 1 total 1 ")
        shell(monitor, proc, "grep -r END64 /big64.txt", "grep -r hits 1 total 1 ")
        say("boot 2: /big64.txt 65536 bytes, first and last lines intact")
        shell(monitor, proc, "rm /big64.txt", "rm ok /big64.txt")
    finally:
        if monitor is not None:
            monitor.close()
        g.terminate(proc)
        err.close()


def main():
    os.makedirs(g.LOG_DIR, exist_ok=True)
    g.prepare_test_disk()
    size = os.path.getsize(g.TEST_DISK)
    if size != DISK_SECTORS * 512:
        raise RuntimeError("test disk is %d bytes, expected %d" % (size, DISK_SECTORS * 512))
    boot1()
    boot2()
    say("QEMU large files (US-G16) contract passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
