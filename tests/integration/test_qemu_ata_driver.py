#!/usr/bin/env python3
"""Tranche 4 contract (slices 1 and 2): Ring 3 ATA PIO driver.

Slice 1: atadriver executes IN/OUT on 0x1F0-0x1F7/0x3F6 at CPL 3 (TSS IOPB)
and serves sector windows over IPC to the ata-client owner. A non-driver task
cannot publish ata-driver/ata-client (-58), its sector IPC is refused and a raw
IN on 0x1F7 raises #GP which kills only that task.

Slice 2: with the driver live, the overlay snapshot flush goes through the
driver (driver-side counter line, kernel PIO overlay counter unchanged), the
file survives a cold reboot (second QEMU process on the same disk), and the
next driver loads the snapshot back through its own PIO. Client writes to the
FAT16 volume are refused. A non-driver task cannot claim the controller nor
touch the kernel job queue. After the driver dies, the kernel Ring 0 PIO path
persists overlay writes again (degraded fallback).
"""
import os
import re
import socket
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_qemu_vfs_service import normalized_log  # noqa: E402

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "ata-driver.log")
LOG_BOOT1 = LOG
LOG_BOOT2 = os.path.join(LOG_DIR, "ata-driver-boot2.log")
ERR = os.path.join(LOG_DIR, "ata-driver.err")
MON = os.path.join(LOG_DIR, "ata-driver-monitor.sock")
DISK = os.path.join(LOG_DIR, "ata-driver.img")
KERNEL = os.path.join(ROOT, "build", "mohhdy.bin")
INITRD = os.path.join(ROOT, "my_initrd.tar")
KEY_HOLD_MS = int(os.environ.get("KEY_HOLD_MS", "10"))
GP_FAULT = "[FAULT] user task killed: int=0x0000000d"


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(needle, proc, offset=0, timeout=25):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped early")
        if needle in normalized_log(log_text()[offset:]):
            return
        time.sleep(0.1)
    raise RuntimeError("missing output: %s" % needle)


def connect_monitor():
    deadline = time.time() + 5
    while time.time() < deadline:
        if os.path.exists(MON):
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                client.connect(MON)
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


def send_command(client, command):
    special = {" ": "spc", "-": "minus"}
    for char in command:
        client.sendall(("sendkey %s %d\n" %
                        (special.get(char, char.lower()), KEY_HOLD_MS)).encode("ascii"))
        time.sleep(0.35)
    client.sendall(("sendkey ret %d\n" % KEY_HOLD_MS).encode("ascii"))


def send_command_until(client, command, marker, proc, attempts=3):
    failure = None
    for _ in range(attempts):
        start = len(log_text())
        send_command(client, command)
        try:
            wait_for(marker, proc, start)
            return start
        except RuntimeError as error:
            failure = error
            time.sleep(0.4)
    raise failure


def wait_child(client, proc, needle, start, rounds=6):
    """Give a freshly spawned child bounded cooperative turns."""
    for _ in range(rounds):
        try:
            wait_for(needle, proc, start, timeout=1)
            return
        except RuntimeError:
            pass
        send_command_until(client, "yield", "yield ok", proc)
    wait_for(needle, proc, start, timeout=3)


def spawn(client, proc, name):
    start = send_command_until(client, "spawn %s" % name, "spawn ok pid", proc)
    match = re.search(r"spawn ok pid[\s\S]*?(\d+) %s" % name,
                      normalized_log(log_text()[start:]))
    if not match:
        raise RuntimeError("%s not spawned" % name)
    return match.group(1), start


def kill(client, proc, pid):
    send_command_until(client, "kill %s" % pid, "Processus %s termine" % pid, proc)


def assert_no_port_leak(start):
    tail = log_text()[start:]
    if "atarogue port unexpectedly allowed" in tail:
        raise RuntimeError("ATA port reachable from a non-driver task")
    for needle in ("atarogue register unexpected", "atarogue client claim unexpected",
                   "atarogue sector ipc unexpected", "ataclient overlay region unexpected",
                   "atarogue claim or job unexpected", "ataclient fat region unexpected"):
        if needle in tail:
            raise RuntimeError("unexpected ATA capability outcome: %s" % needle)


FLUSH_RE = re.compile(r"atadriver snapshot flush ok gen=(\d+) flushes=(\d+) loads=(\d+) kpio=(\d+)")
READY_RE = re.compile(r"atadriver ring3 pio ready flushes=(\d+) loads=(\d+) kpio=(\d+)")
LOAD_RE = re.compile(r"atadriver snapshot load ok gen=(\d+) flushes=(\d+) loads=(\d+) kpio=(\d+)")


def make_disk():
    """Standard FAT16 fixture (LBA 64-4223) extended to 4 MiB of scratch."""
    subprocess.check_call([sys.executable, os.path.join(ROOT, "tests", "scripts", "make_fat16_image.py"),
                           "--image", DISK], stdout=subprocess.DEVNULL)
    with open(DISK, "r+b") as handle:
        handle.truncate(8192 * 512)


def snapshot_bytes():
    with open(DISK, "rb") as handle:
        return handle.read(64 * 512)


def boot(log_path):
    global LOG
    LOG = log_path
    for path in (log_path, MON):
        try:
            os.remove(path)
        except OSError:
            pass
    command = [
        "qemu-system-i386", "-kernel", KERNEL, "-initrd", INITRD,
        "-m", "1024M", "-display", "none", "-vga", "none",
        "-serial", "file:" + log_path,
        "-monitor", "unix:%s,server,nowait" % MON,
        "-machine", "type=pc,accel=tcg",
        "-drive", "file=%s,format=raw,if=ide,index=0,cache=writethrough" % DISK,
        "-no-reboot", "-no-shutdown",
    ]
    err_handle = open(ERR, "ab")
    proc = subprocess.Popen(command, stdout=err_handle, stderr=err_handle)
    err_handle.close()
    wait_for("(-.-)", proc)
    monitor = connect_monitor()
    time.sleep(0.5)
    return proc, monitor


def shutdown(proc, monitor):
    if monitor is not None:
        monitor.close()
    if proc is not None and proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            proc.kill()
    try:
        os.remove(MON)
    except OSError:
        pass


def search_after(regex, start, what, client=None, proc=None, rounds=8):
    """Find a complete counter line; the driver prints it with several putc
    syscalls, so give it bounded cooperative turns to finish the line."""
    for _ in range(rounds + 1):
        match = regex.search(normalized_log(log_text()[start:]))
        if match:
            return [int(x) for x in match.groups()]
        if client is None:
            break
        time.sleep(0.3)
        send_command_until(client, "yield", "yield ok", proc)
    raise RuntimeError("missing %s" % what)


def boot1():
    proc, monitor = boot(LOG_BOOT1)
    try:
        # 1. No driver: every task has the ATA ports denied by the IOPB and
        #    cannot claim the controller or touch the kernel job queue.
        _, start = spawn(monitor, proc, "atarogue")
        wait_child(monitor, proc, "atarogue register refused", start)
        wait_child(monitor, proc, "atarogue client claim refused", start)
        wait_child(monitor, proc, "atarogue claim and job refused", start)
        wait_child(monitor, proc, GP_FAULT, start)
        assert_no_port_leak(start)
        send_command_until(monitor, "uptime", "uptime", proc)
        # 2. Driver live: Ring 3 PIO; empty disk snapshot, so the load job is
        #    dropped (nothing valid to restore).
        driver_pid, start = spawn(monitor, proc, "atadriver")
        wait_child(monitor, proc, "atadriver ring3 pio ready", start)
        ready = search_after(READY_RE, start, "driver ready counters", monitor, proc)
        wait_child(monitor, proc, "atadriver snapshot load skipped", start, rounds=12)
        send_command_until(monitor, "service-find ata-driver",
                           "service-find ok ata-driver %s" % driver_pid, proc)
        # 3. Client sector window + overlay and FAT16 fences.
        _, start = spawn(monitor, proc, "ataclient")
        wait_child(monitor, proc, "ataclient ring3 sector roundtrip ok", start, rounds=12)
        wait_child(monitor, proc, "ataclient overlay region refused", start, rounds=12)
        wait_child(monitor, proc, "ataclient fat region refused", start, rounds=12)
        assert_no_port_leak(start)
        # 4. Overlay write while the driver is live: the snapshot flush goes
        #    through the driver; the kernel PIO overlay counter does not move.
        start = send_command_until(monitor, "write t4s2 viadrv", "write ok", proc)
        wait_child(monitor, proc, "atadriver snapshot flush ok", start, rounds=16)
        flush = search_after(FLUSH_RE, start, "driver flush counters", monitor, proc)
        if flush[1] < 1 or flush[3] != ready[2]:
            raise RuntimeError("flush not via driver: ready=%r flush=%r" % (ready, flush))
        send_command_until(monitor, "cat t4s2", "viadrv", proc)
        # 5. Driver live: non-driver refused at register, claim/job, IPC, port.
        _, start = spawn(monitor, proc, "atarogue")
        wait_child(monitor, proc, "atarogue register refused", start)
        wait_child(monitor, proc, "atarogue client claim refused", start)
        wait_child(monitor, proc, "atarogue claim and job refused", start)
        wait_child(monitor, proc, "atarogue sector ipc refused", start, rounds=12)
        wait_child(monitor, proc, GP_FAULT, start)
        assert_no_port_leak(start)
        send_command_until(monitor, "service-find ata-driver",
                           "service-find ok ata-driver %s" % driver_pid, proc)
        return flush
    finally:
        shutdown(proc, monitor)


def boot2():
    proc, monitor = boot(LOG_BOOT2)
    try:
        # 6. Cold reboot: the kernel loads the driver-written snapshot at boot
        #    (no driver exists yet, so Ring 0 PIO load is the fallback).
        wait_for("Overlay FS charge depuis le disque IDE", proc)
        send_command_until(monitor, "cat t4s2", "viadrv", proc)
        # 7. New driver: the snapshot is loaded back through its Ring 3 PIO.
        driver_pid, start = spawn(monitor, proc, "atadriver")
        wait_child(monitor, proc, "atadriver snapshot load ok", start, rounds=16)
        load = search_after(LOAD_RE, start, "driver load counters", monitor, proc)
        if load[2] < 1:
            raise RuntimeError("load not via driver: %r" % load)
        send_command_until(monitor, "cat t4s2", "viadrv", proc)
        # 8. Driver gone: kernel Ring 0 overlay path persists files again.
        kill(monitor, proc, driver_pid)
        send_command_until(monitor, "service-find ata-driver",
                           "service-find: service indisponible", proc)
        send_command_until(monitor, "write t4note ok4", "write ok", proc)
        send_command_until(monitor, "cat t4note", "ok4", proc)
        return load
    finally:
        shutdown(proc, monitor)


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG_BOOT1, LOG_BOOT2, ERR, MON, DISK):
        try:
            os.remove(path)
        except OSError:
            pass
    make_disk()
    flush = boot1()
    # The overlay snapshot on disk (LBA 0-63) was written by the driver only.
    if b"viadrv" not in snapshot_bytes():
        raise RuntimeError("driver flush did not reach the disk snapshot")
    load = boot2()
    # After the driver died, the kernel Ring 0 fallback wrote t4note.
    if b"t4note" not in snapshot_bytes():
        raise RuntimeError("kernel fallback flush did not reach the disk snapshot")
    print("ata-driver flush gen=%d flushes=%d kpio=%d; reboot load loads=%d" %
          (flush[0], flush[1], flush[3], load[2]))
    print("MOHHDY Tranche 4 Ring 3 ATA driver contract passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("MOHHDY Tranche 4 Ring 3 ATA driver contract failed: %s" % error,
              file=sys.stderr)
        raise SystemExit(1)
