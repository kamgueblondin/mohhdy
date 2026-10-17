#!/usr/bin/env python3
"""Lot 5 (US-001) contract: PS/2 keyboard and COM1 moved to Ring 3 drivers.

1. The kernel spawns bin/ps2driver and bin/serialdriver at boot; each
   claims its device (TSS IOPB grant: 0x60/0x64, 0x3F8-0x3FF).
2. Every command typed below goes QEMU sendkey -> IRQ1 -> ps2driver (CPL 3
   reads 0x60, decodes, submits) -> shell; the serial log the test reads is
   written by serialdriver. `devices` shows the counters rising.
3. devrogue: both claims and SUBMIT are refused; IN 0x64 raises #GP and
   kills only that task.
4. Kill ps2driver: keys still reach the shell (Ring 0 fallback, counted),
   the supervisor respawns bin/ps2driver (respawns 1), which claims again.
5. Kill serialdriver: the log keeps flowing (Ring 0 fallback, counted), the
   supervisor respawns bin/serialdriver (respawns 1), which claims again.
The PS/2 mouse decoder is proven by unit tests only (QEMU uses usb-tablet).
"""
import os
import re
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_ata_driver as ata  # noqa: E402
from test_qemu_vfs_service import normalized_log  # noqa: E402

ata.LOG = os.path.join(ata.LOG_DIR, "input-drivers.log")
ata.ERR = os.path.join(ata.LOG_DIR, "input-drivers.err")
ata.MON = os.path.join(ata.LOG_DIR, "input-drivers-monitor.sock")
KERNEL = os.environ.get("MOHHDY_INPUT_KERNEL", os.path.join(ata.ROOT, "build", "mohhdy.bin"))
KEYS = ("ps2_owner", "ps2_handed", "irqs", "events", "ps2_ring0", "ps2_stalls", "ps2_respawns",
        "serial_owner", "serial_handed", "ring3", "serial_ring0", "serial_stalls", "overflows",
        "rx", "serial_respawns")
STATUS_RE = (r"devices ok ps2 owner (-?\d+) handed (\d+) irqs (\d+) events (\d+) ring0 (\d+) "
             r"stalls (\d+) respawns (\d+) serial owner (-?\d+) handed (\d+) ring3 (\d+) "
             r"ring0 (\d+) stalls (\d+) overflows (\d+) rx (\d+) respawns (\d+) end")


def devices(client, proc):
    start = ata.send_command_until(client, "devices", "devices ok ps2", proc)
    deadline = time.monotonic() + 10
    while True:
        match = re.search(STATUS_RE, normalized_log(ata.log_text()[start:]))
        if match or time.monotonic() > deadline:
            break
        time.sleep(0.1)
    if not match:
        raise RuntimeError("unparsable devices: %s" % ata.log_text()[start:][-800:])
    return dict(zip(KEYS, (int(v) for v in match.groups())))


def wait_status(client, proc, check, what, rounds=10):
    st = None
    for _ in range(rounds):
        st = devices(client, proc)
        if check(st):
            return st
        ata.send_command_until(client, "yield", "yield ok", proc)
    raise RuntimeError("%s: %r" % (what, st))


def main():
    os.makedirs(ata.LOG_DIR, exist_ok=True)
    for path in (ata.LOG, ata.ERR, ata.MON):
        try:
            os.remove(path)
        except OSError:
            pass
    command = [
        "qemu-system-i386", "-kernel", KERNEL, "-initrd", ata.INITRD,
        "-m", "1024M", "-display", "none", "-vga", "std",
        "-serial", "file:" + ata.LOG, "-monitor", "unix:%s,server,nowait" % ata.MON,
        "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
    ]
    proc = client = None
    try:
        with open(ata.ERR, "ab") as err:
            proc = subprocess.Popen(command, stdout=err, stderr=err)
        ata.wait_for("[0m : ", proc, timeout=60)
        client = ata.connect_monitor()
        # 1. Boot spawns and claims.
        ata.wait_for("[PS2] boot ps2driver spawned", proc, timeout=10)
        ata.wait_for("[SERIAL] boot serialdriver spawned", proc, timeout=10)
        st = wait_status(client, proc, lambda s: s["ps2_owner"] > 0 and s["serial_owner"] > 0,
                         "drivers did not claim")
        ata.wait_for("[PS2] Ring 3 ps2driver claimed the controller pid", proc, timeout=5)
        ata.wait_for("[SERIAL] Ring 3 serialdriver claimed COM1 pid", proc, timeout=5)
        ps2_pid, ser_pid = st["ps2_owner"], st["serial_owner"]
        # 2. Typing goes through the Ring 3 driver, the log through serialdriver.
        st2 = devices(client, proc)
        if st2["events"] < st["events"] + len("devices") + 1 or st2["irqs"] <= st["irqs"]:
            raise RuntimeError("keys did not go through ps2driver: %r -> %r" % (st, st2))
        if st2["ring3"] <= st["ring3"] or st2["ps2_owner"] != ps2_pid or st2["serial_owner"] != ser_pid:
            raise RuntimeError("serial output did not go through serialdriver: %r -> %r" % (st, st2))
        # 3. Rogue task.
        _, start = ata.spawn(client, proc, "devrogue")
        ata.wait_child(client, proc, "devrogue ps2 claim rc -180 serial claim rc -180 submit rc -180", start)
        ata.wait_child(client, proc, ata.GP_FAULT, start)
        if "devrogue unexpected" in normalized_log(ata.log_text()[start:]):
            raise RuntimeError("devrogue reached a device port")
        # 4. Kill ps2driver: keys still arrive (Ring 0 until the new claim), then
   supervised respawn.
        mark = len(ata.log_text())
        ata.kill(client, proc, str(ps2_pid))
        st3 = wait_status(client, proc, lambda s: s["ps2_respawns"] == 1 and s["ps2_owner"] > 0
                          and s["ps2_owner"] != ps2_pid, "ps2driver not respawned")
        ata.wait_for("[PS2] supervised respawn bin/ps2driver pid", proc, mark, timeout=5)
        # The keys typed between the death and the new claim are read by the
        # Ring 0 fallback; the respawn is fast, so that window may hold no key
        # at all (the stall/death fallback itself is unit tested).
        if st3["events"] + st3["ps2_ring0"] <= st2["events"] + st2["ps2_ring0"]:
            raise RuntimeError("no key delivered after the kill: %r" % st3)
        st4 = devices(client, proc)
        if st4["events"] <= st3["events"] or st4["ps2_owner"] != st3["ps2_owner"]:
            raise RuntimeError("respawned ps2driver does not deliver keys: %r -> %r" % (st3, st4))
        # 5. Kill serialdriver: "Processus N termine" is queued after the death,
        # so Ring 0 must write it (counted), then supervised respawn.
        mark = len(ata.log_text())
        ata.kill(client, proc, str(ser_pid))
        st5 = wait_status(client, proc, lambda s: s["serial_respawns"] == 1 and s["serial_owner"] > 0
                          and s["serial_owner"] != ser_pid, "serialdriver not respawned")
        ata.wait_for("[SERIAL] supervised respawn bin/serialdriver pid", proc, mark, timeout=5)
        if st5["serial_ring0"] <= st4["serial_ring0"]:
            raise RuntimeError("no Ring 0 serial fallback after the kill: %r" % st5)
        st6 = devices(client, proc)
        if st6["ring3"] <= st5["ring3"] or st6["serial_owner"] != st5["serial_owner"]:
            raise RuntimeError("respawned serialdriver does not write: %r -> %r" % (st5, st6))
        print("input drivers: ps2driver pid %d (%d events, %d irqs), serialdriver pid %d (%d bytes); "
              "rogue refused (#GP); ps2 respawned as %d (ring0 fallback %d bytes), serial respawned "
              "as %d (ring0 fallback %d bytes)" %
              (ps2_pid, st2["events"], st2["irqs"], ser_pid, st2["ring3"], st3["ps2_owner"],
               st3["ps2_ring0"], st5["serial_owner"], st5["serial_ring0"]))
        print("QEMU Ring 3 PS/2 and serial driver contract passed.")
        return 0
    finally:
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
        print("QEMU Ring 3 PS/2 and serial driver contract failed: %s" % error)
        raise SystemExit(1)
