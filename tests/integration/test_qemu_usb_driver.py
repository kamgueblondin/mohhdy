#!/usr/bin/env python3
"""Lot 8 (US-001) contract: the UHCI USB tablet driver runs in Ring 3.

QEMU runs with -usb -device usb-tablet. The kernel only finds and stops
the controller at boot, owns the frame list and the schedule page, and
writes every TD after checking it (bounded DMA, no IOMMU needed: buffers
can only point into the driver's data page).

1. Boot spawns bin/usbdriver, which claims the controller; the bounds
   probe of the driver is refused by the kernel (buffer outside the data
   page, looping schedule), then the driver enumerates the tablet.
2. QMP absolute motion and a click reach the guest through the Ring 3
   driver: `devices usb` shows reports rising, the owner unchanged.
3. devrogue: USB claim, TD, run and report are refused (-180).
4. Kill usbdriver: the kernel stops the controller first (stops +1, no
   report while no driver lives), the supervisor respawns bin/usbdriver
   (respawns 1), which claims again, enumerates again and delivers the
   next reports.
"""
import os
import re
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_ata_driver as ata  # noqa: E402
from test_qemu_vfs_service import normalized_log  # noqa: E402
from test_qemu_input_drivers import Qmp  # noqa: E402

ata.LOG = os.path.join(ata.LOG_DIR, "usb-driver.log")
ata.ERR = os.path.join(ata.LOG_DIR, "usb-driver.err")
ata.MON = os.path.join(ata.LOG_DIR, "usb-driver-monitor.sock")
QMP = os.path.join(ata.LOG_DIR, "usb-driver-qmp.sock")
KERNEL = os.environ.get("MOHHDY_USB_KERNEL", os.path.join(ata.ROOT, "build", "mohhdy.bin"))
USB_RE = (r"devices usb present (\d+) owner (-?\d+) handed (\d+) tds (\d+) rejected (\d+) "
          r"reports (\d+) stops (\d+) respawns (\d+) end")
USB_KEYS = ("present", "owner", "handed", "tds", "rejected", "reports", "stops", "respawns")


def usb_status(client, proc):
    start = ata.send_command_until(client, "devices", "devices usb present", proc)
    deadline = time.monotonic() + 10
    while True:
        match = re.search(USB_RE, normalized_log(ata.log_text()[start:]))
        if match or time.monotonic() > deadline:
            break
        time.sleep(0.1)
    if not match:
        raise RuntimeError("unparsable devices usb: %s" % ata.log_text()[start:][-800:])
    return dict(zip(USB_KEYS, (int(v) for v in match.groups())))


def wait_usb(client, proc, check, what, rounds=12):
    st = None
    for _ in range(rounds):
        st = usb_status(client, proc)
        if check(st):
            return st
        ata.send_command_until(client, "yield", "yield ok", proc)
    raise RuntimeError("%s: %r" % (what, st))


def move(qmp, x, y, click=False):
    qmp.events([{"type": "abs", "data": {"axis": "x", "value": x}},
                {"type": "abs", "data": {"axis": "y", "value": y}}])
    if click:
        qmp.events([{"type": "btn", "data": {"down": True, "button": "left"}}])
        time.sleep(0.2)
        qmp.events([{"type": "btn", "data": {"down": False, "button": "left"}}])


def main():
    os.makedirs(ata.LOG_DIR, exist_ok=True)
    for path in (ata.LOG, ata.ERR, ata.MON, QMP):
        try:
            os.remove(path)
        except OSError:
            pass
    command = [
        "qemu-system-i386", "-kernel", KERNEL, "-initrd", ata.INITRD,
        "-m", "1024M", "-display", "none", "-vga", "std",
        "-usb", "-device", "usb-tablet",
        "-serial", "file:" + ata.LOG, "-monitor", "unix:%s,server,nowait" % ata.MON,
        "-qmp", "unix:%s,server,nowait" % QMP,
        "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
    ]
    proc = client = qmp = None
    try:
        with open(ata.ERR, "ab") as err:
            proc = subprocess.Popen(command, stdout=err, stderr=err)
        ata.wait_for("[0m : ", proc, timeout=60)
        client = ata.connect_monitor()
        # 1. Boot spawn, claim, bounds probe refused, enumeration.
        ata.wait_for("[USB] boot usbdriver spawned", proc, timeout=10)
        ata.wait_for("[USB] Ring 3 usbdriver claimed the UHCI controller pid", proc, timeout=10)
        ata.wait_for("usbdriver dma bounds probe outside rc -181 loop rc -181", proc, timeout=10)
        ata.wait_for("usbdriver tablet enumerated port ", proc, timeout=20)
        st1 = wait_usb(client, proc, lambda s: s["owner"] > 0 and s["handed"] == 1,
                       "usbdriver did not claim")
        if st1["present"] != 1 or st1["rejected"] < 2 or st1["tds"] < 4:
            raise RuntimeError("unexpected boot usb counters: %r" % st1)
        pid1 = st1["owner"]
        # 2. Absolute motion and a click through the Ring 3 driver.
        qmp = Qmp(QMP)
        move(qmp, 8000, 9000, click=True)
        time.sleep(0.5)
        st2 = wait_usb(client, proc, lambda s: s["reports"] > st1["reports"], "no tablet report")
        if st2["owner"] != pid1 or st2["stops"] != st1["stops"]:
            raise RuntimeError("driver changed while moving: %r -> %r" % (st1, st2))
        ata.wait_for("usbdriver first report applied", proc, timeout=5)
        # 3. Rogue task.
        _, start = ata.spawn(client, proc, "devrogue")
        ata.wait_child(client, proc, "devrogue usb claim rc -180 td rc -180 run rc -180 report rc -180", start)
        ata.wait_child(client, proc, ata.GP_FAULT, start)
        st3 = usb_status(client, proc)
        if st3["owner"] != pid1:
            raise RuntimeError("devrogue changed the usb owner: %r" % st3)
        # 4. Kill: controller stopped by the kernel, supervised respawn.
        mark = len(ata.log_text())
        ata.kill(client, proc, str(pid1))
        st4 = wait_usb(client, proc, lambda s: s["respawns"] == 1 and s["owner"] > 0
                       and s["owner"] != pid1, "usbdriver not respawned")
        ata.wait_for("[USB] supervised respawn bin/usbdriver pid", proc, mark, timeout=5)
        if st4["stops"] <= st3["stops"]:
            raise RuntimeError("controller not stopped after the driver died: %r -> %r" % (st3, st4))
        ata.wait_for("[USB] Ring 3 usbdriver claimed the UHCI controller pid %d" % st4["owner"],
                     proc, mark, timeout=10)
        ata.wait_for("usbdriver tablet enumerated port ", proc, mark, timeout=20)
        move(qmp, 20000, 15000, click=True)
        time.sleep(0.5)
        st5 = wait_usb(client, proc, lambda s: s["reports"] > st4["reports"],
                       "respawned usbdriver delivers no report")
        if st5["owner"] != st4["owner"]:
            raise RuntimeError("owner changed again: %r -> %r" % (st4, st5))
        print("usb: usbdriver pid %d claimed, bounds probe refused (rejected %d), %d reports; "
              "devrogue refused; killed: controller stopped (stops %d), respawned as pid %d, "
              "%d reports after respawn" %
              (pid1, st1["rejected"], st2["reports"], st4["stops"], st5["owner"],
               st5["reports"] - st4["reports"]))
        print("QEMU Ring 3 UHCI USB tablet driver contract passed.")
        return 0
    finally:
        if qmp is not None:
            qmp.close()
        if client is not None:
            client.close()
        if proc is not None and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=4)
            except subprocess.TimeoutExpired:
                proc.kill()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU Ring 3 USB driver contract failed: %s" % error)
        raise SystemExit(1)
