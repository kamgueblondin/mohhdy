#!/usr/bin/env python3
"""Tranche 4 contract (slices 1, 2 and 3): Ring 3 ATA PIO driver.

Slice 1: atadriver executes IN/OUT on 0x1F0-0x1F7/0x3F6 at CPL 3 (TSS IOPB)
and serves sector windows over IPC to the ata-client owner. A non-driver task
cannot publish ata-driver/ata-client (-58), its sector IPC is refused and a raw
IN on 0x1F7 raises #GP which kills only that task.

Slice 2: with the driver live, the overlay snapshot flush goes through the
driver (driver-side counter line, kernel PIO overlay counter unchanged) and
survives a cold reboot; a newly registered driver loads the snapshot back
through its own PIO. Client writes to the FAT16 volume are refused. A
non-driver task cannot claim the controller nor touch the kernel job queue.

Slice 3: the kernel spawns atadriver at boot (default path, no manual
spawn); a second driver instance cannot take the service. FAT16 sector I/O
(vfs-write/vfs-read through the VFS worker) goes through the driver: driver
counters rd/wr grow, the kernel FAT PIO counter does not move and the
"FAT PIO while a driver is live" counter stays 0. After the root shell kills
the boot driver, FAT and overlay writes fall back to Ring 0 PIO and persist
(checked after a cold reboot, where the boot driver is live again).

Crash proof: with a test hook armed by the root shell (SYS_ATA_DEBUG, refused
to any other task), the driver dies with #GP in the middle of a FAT write
job (write command issued, half of the sector pushed, claim held). The
kernel resets the ATA channel, resumes the blocked VFS worker and redoes the
request through Ring 0 PIO: the write succeeds, earlier files are intact
and the payload is on disk (host check), with no hang.
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
    # Lower case only: shift combos can stick in the guest keyboard driver.
    special = {" ": "spc", "-": "minus", ".": "dot", "/": "slash"}
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
                   "atarogue claim or job unexpected", "ataclient fat region unexpected",
                   "atarogue debug arm unexpected"):
        if needle in tail:
            raise RuntimeError("unexpected ATA capability outcome: %s" % needle)


# Counter lines are emitted with several putc syscalls; IRQ0 preemption can
# interleave "[SCHED]" lines and shell output between tokens (CI flake). The
# contract therefore reads the tokens in order after the line prefix.
FLUSH_SPEC = ("atadriver snapshot flush ok", ("gen", "flushes", "loads", "kpio"))
READY_SPEC = ("atadriver ring3 pio ready", ("flushes", "loads", "kpio"))
LOAD_SPEC = ("atadriver snapshot load ok", ("gen", "flushes", "loads", "kpio"))
FATIO_SPEC = ("atadriver fat io", ("rd", "wr", "kfat"))
STATUS_KEYS = ("driver", "boot", "fatrd", "fatwr", "fatkpio", "fatkpiolive",
               "aborts", "flushes", "kpio", "resets")
SCHED_LINE = re.compile(r"\[SCHED\] switching to task \d+\s*")


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


def parse_counters(spec, start):
    prefix, keys = spec
    text = SCHED_LINE.sub("", log_text()[start:])
    pos = text.find(prefix)
    if pos < 0:
        return None
    pos += len(prefix)
    values = []
    for key in keys:
        match = re.compile(r"\b%s=(\d+)" % key).search(text, pos)
        if not match:
            return None
        values.append(int(match.group(1)))
        pos = match.end()
    return values


def ata_status(client, proc):
    """Kernel view (SYS_ATA_STATUS) printed by the shell: key value pairs."""
    start = send_command_until(client, "ata-status", "ata-status ok", proc)
    wait_for(" end", proc, start, timeout=10)
    text = SCHED_LINE.sub("", log_text()[start:])
    pos = text.find("ata-status ok")
    values = {}
    for key in STATUS_KEYS:
        match = re.compile(r"\b%s (-?\d+)" % key).search(text, pos)
        if not match:
            raise RuntimeError("ata-status missing %s" % key)
        values[key] = int(match.group(1))
        pos = match.end()
    return values


def last_counters(spec, start):
    """Like parse_counters, for the last occurrence of the line prefix."""
    prefix, keys = spec
    text = SCHED_LINE.sub("", log_text()[start:])
    pos = text.rfind(prefix)
    if pos < 0:
        return None
    return _counters_at(text, pos + len(prefix), keys)


def _counters_at(text, pos, keys):
    values = []
    for key in keys:
        match = re.compile(r"\b%s=(\d+)" % key).search(text, pos)
        if not match:
            return None
        values.append(int(match.group(1)))
        pos = match.end()
    return values


def vfs_write_read(client, proc, path, payload):
    start = send_command_until(client, "vfs-write %s %s" % (path, payload), "vfs-write ok", proc)
    wait_for("vfsvirtual write %s" % path, proc, start, timeout=20)
    for _ in range(2):
        send_command_until(client, "yield", "yield ok", proc)
    for _ in range(6):
        before = send_command_until(client, "vfs-read %s" % path, "vfs-read", proc)
        try:
            wait_for(payload, proc, before, timeout=4)
            return start
        except RuntimeError:
            send_command_until(client, "yield", "yield ok", proc)
    raise RuntimeError("payload %s not read back from %s" % (payload, path))


def vfs_read_payload(client, proc, path, payload):
    for _ in range(6):
        before = send_command_until(client, "vfs-read %s" % path, "vfs-read", proc)
        try:
            wait_for(payload, proc, before, timeout=4)
            return
        except RuntimeError:
            send_command_until(client, "yield", "yield ok", proc)
    raise RuntimeError("payload %s not read back from %s" % (payload, path))


def start_vfs(client, proc):
    worker, start = spawn(client, proc, "vfsvirtual")
    wait_child(client, proc, "vfsvirtual ready", start)
    server, start = spawn(client, proc, "vfsserver")
    wait_child(client, proc, "vfsserver ready vfs", start, rounds=12)
    return worker, server


def search_after(spec, start, what, client=None, proc=None, rounds=8):
    """Give the driver bounded cooperative turns to finish its counter line."""
    for _ in range(rounds + 1):
        values = parse_counters(spec, start)
        if values is not None:
            return values
        if client is None:
            break
        time.sleep(0.3)
        send_command_until(client, "yield", "yield ok", proc)
    raise RuntimeError("missing %s" % what)


def boot1():
    proc, monitor = boot(LOG_BOOT1)
    try:
        # 0. Default boot: the kernel spawned atadriver, it registered before
        #    the shell prompt; the snapshot was loaded by the kernel at boot
        #    (no task yet), so the boot driver does not reload it.
        wait_for("[ATA] boot atadriver spawned", proc)
        wait_for("[ATA] boot driver registered; kernel boot load kept", proc)
        ready = search_after(READY_SPEC, 0, "driver ready counters", monitor, proc)
        st = ata_status(monitor, proc)
        if st["driver"] <= 0 or st["driver"] != st["boot"]:
            raise RuntimeError("boot driver not live: %r" % st)
        driver_pid = str(st["driver"])
        send_command_until(monitor, "service-find ata-driver",
                           "service-find ok ata-driver %s" % driver_pid, proc)
        # 1. Non-driver task: register, claim/job, sector IPC and raw port
        #    access are all refused (driver live).
        _, start = spawn(monitor, proc, "atarogue")
        wait_child(monitor, proc, "atarogue register refused", start)
        wait_child(monitor, proc, "atarogue client claim refused", start)
        wait_child(monitor, proc, "atarogue claim and job refused", start)
        wait_child(monitor, proc, "atarogue debug arm refused", start)
        wait_child(monitor, proc, "atarogue sector ipc refused", start, rounds=12)
        wait_child(monitor, proc, GP_FAULT, start)
        assert_no_port_leak(start)
        # 2. A second driver instance cannot take the ata-driver service.
        dup_pid, start = spawn(monitor, proc, "atadriver")
        wait_child(monitor, proc, "atadriver register failed", start)
        kill(monitor, proc, dup_pid)
        send_command_until(monitor, "service-find ata-driver",
                           "service-find ok ata-driver %s" % driver_pid, proc)
        # 3. Client sector window + overlay and FAT16 fences.
        _, start = spawn(monitor, proc, "ataclient")
        wait_child(monitor, proc, "ataclient ring3 sector roundtrip ok", start, rounds=12)
        wait_child(monitor, proc, "ataclient overlay region refused", start, rounds=12)
        wait_child(monitor, proc, "ataclient fat region refused", start, rounds=12)
        assert_no_port_leak(start)
        # 4. Overlay write: the snapshot flush goes through the driver; the
        #    kernel PIO overlay counter does not move.
        start = send_command_until(monitor, "write t4s2 viadrv", "write ok", proc)
        wait_child(monitor, proc, "atadriver snapshot flush ok", start, rounds=16)
        flush = search_after(FLUSH_SPEC, start, "driver flush counters", monitor, proc)
        if flush[1] < 1 or flush[3] != ready[2]:
            raise RuntimeError("flush not via driver: ready=%r flush=%r" % (ready, flush))
        send_command_until(monitor, "cat t4s2", "viadrv", proc)
        # 5. FAT16 through the driver: the VFS worker's FAT sector I/O is a
        #    synchronous RPC served by the driver's PIO. Driver counters grow;
        #    the kernel FAT PIO counter stays where the boot mount left it.
        vfs_pids = start_vfs(monitor, proc)
        before = ata_status(monitor, proc)
        start = vfs_write_read(monitor, proc, "fat16/t4s3.txt", "viadrvfat")
        wait_child(monitor, proc, "atadriver fat io", start, rounds=8)
        fatio = last_counters(FATIO_SPEC, start)
        if fatio is None:
            raise RuntimeError("missing driver fat io counters")
        after = ata_status(monitor, proc)
        if (fatio[0] < 1 or fatio[1] < 1 or fatio[2] != 0 or
                after["fatrd"] <= before["fatrd"] or after["fatwr"] <= before["fatwr"] or
                after["fatkpio"] != before["fatkpio"] or after["fatkpiolive"] != 0 or
                after["aborts"] != 0):
            raise RuntimeError("FAT not via driver: before=%r after=%r driver=%r" %
                               (before, after, fatio))
        # 6. Kill the boot driver (root shell): FAT and overlay writes fall
        #    back to Ring 0 PIO.
        kill(monitor, proc, driver_pid)
        send_command_until(monitor, "service-find ata-driver",
                           "service-find: service indisponible", proc)
        vfs_write_read(monitor, proc, "fat16/t4fb.txt", "viakernel")
        fallback = ata_status(monitor, proc)
        if fallback["driver"] != 0 or fallback["fatkpio"] <= after["fatkpio"]:
            raise RuntimeError("FAT fallback not via Ring 0: %r" % fallback)
        # Plain overlay writes are reserved to vfsvirtual while it is live
        # (AOS-2178): stop the VFS pair first.
        for pid in vfs_pids:
            kill(monitor, proc, pid)
        send_command_until(monitor, "write t4note ok4", "write ok", proc)
        send_command_until(monitor, "cat t4note", "ok4", proc)
        return flush, fatio, before, after, fallback
    finally:
        shutdown(proc, monitor)


def boot2():
    proc, monitor = boot(LOG_BOOT2)
    try:
        # 7. Cold reboot: the kernel loads the snapshot at boot (Ring 0, no
        #    task yet) and the boot driver is live again.
        wait_for("Overlay FS charge depuis le disque IDE", proc)
        wait_for("[ATA] boot driver registered; kernel boot load kept", proc)
        send_command_until(monitor, "cat t4s2", "viadrv", proc)
        send_command_until(monitor, "cat t4note", "ok4", proc)
        st = ata_status(monitor, proc)
        driver_pid = str(st["driver"])
        if st["driver"] <= 0:
            raise RuntimeError("boot driver not live after reboot: %r" % st)
        # 8. Both FAT files persisted (driver-written and fallback-written),
        #    read back through the boot driver.
        send_command_until(monitor, "fat16-cat t4s3.txt", "viadrvfat", proc)
        send_command_until(monitor, "fat16-cat t4fb.txt", "viakernel", proc)
        # 9. Boot driver crash in the middle of a FAT write job (test hook
        #    armed by the root shell; the boot driver has no user parent, so
        #    no exit notice lands in the shell mailbox mid-command). The
        #    driver starts the sector write, pushes half of the data and
        #    takes a #GP while holding the claim. The kernel resets the channel, resumes the blocked VFS worker and
        #    redoes the request through Ring 0 PIO: no hang, no data loss.
        vfs_pids = start_vfs(monitor, proc)
        before = ata_status(monitor, proc)
        send_command_until(monitor, "ata-debug-crash", "ata-debug-crash ok armed", proc)
        start = vfs_write_read(monitor, proc, "fat16/t4cr.txt", "viacrash")
        wait_for("atadriver debug crash mid-job", proc, start, timeout=10)
        wait_for(GP_FAULT, proc, start, timeout=10)
        wait_for("[ATA] channel reset after driver loss", proc, start, timeout=10)
        send_command_until(monitor, "service-find ata-driver",
                           "service-find: service indisponible", proc)
        crash = ata_status(monitor, proc)
        if (crash["driver"] != 0 or crash["aborts"] != before["aborts"] + 1 or
                crash["resets"] != before["resets"] + 1 or
                crash["fatkpio"] <= before["fatkpio"]):
            raise RuntimeError("crash fallback not clean: before=%r after=%r" % (before, crash))
        # Files written before the crash are intact.
        vfs_read_payload(monitor, proc, "fat16/t4s3.txt", "viadrvfat")
        vfs_read_payload(monitor, proc, "fat16/t4fb.txt", "viakernel")
        # Plain overlay reads/writes are reserved to vfsvirtual while live.
        for pid in vfs_pids:
            kill(monitor, proc, pid)
        # 10. Respawned driver: loads the snapshot back through its Ring 3 PIO.
        driver_pid, start = spawn(monitor, proc, "atadriver")
        wait_child(monitor, proc, "atadriver snapshot load ok", start, rounds=16)
        load = search_after(LOAD_SPEC, start, "driver load counters", monitor, proc)
        if load[2] < 1:
            raise RuntimeError("load not via driver: %r" % load)
        send_command_until(monitor, "cat t4s2", "viadrv", proc)
        return load, before, crash
    finally:
        shutdown(proc, monitor)


def disk_bytes():
    with open(DISK, "rb") as handle:
        return handle.read()


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG_BOOT1, LOG_BOOT2, ERR, MON, DISK):
        try:
            os.remove(path)
        except OSError:
            pass
    make_disk()
    flush, fatio, before, after, fallback = boot1()
    if b"viadrv" not in snapshot_bytes() or b"t4note" not in snapshot_bytes():
        raise RuntimeError("overlay snapshot missing driver or fallback writes")
    fat_area = disk_bytes()[64 * 512:]
    if b"viadrvfat" not in fat_area or b"viakernel" not in fat_area:
        raise RuntimeError("FAT16 files did not reach the disk")
    load, pre_crash, crash = boot2()
    if b"viacrash" not in disk_bytes()[64 * 512:]:
        raise RuntimeError("FAT write redone after the driver crash did not reach the disk")
    print("ata-driver flush gen=%d flushes=%d kpio=%d; fat via driver rd=%d wr=%d kfat=%d "
          "kernel fatkpio %d->%d; fallback fatkpio %d; reboot load loads=%d; "
          "mid-job crash aborts %d->%d resets %d->%d fatkpio %d->%d" %
          (flush[0], flush[1], flush[3], fatio[0], fatio[1], fatio[2],
           before["fatkpio"], after["fatkpio"], fallback["fatkpio"], load[2],
           pre_crash["aborts"], crash["aborts"], pre_crash["resets"], crash["resets"],
           pre_crash["fatkpio"], crash["fatkpio"]))
    print("MOHHDY Tranche 4 Ring 3 ATA driver contract passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("MOHHDY Tranche 4 Ring 3 ATA driver contract failed: %s" % error,
              file=sys.stderr)
        raise SystemExit(1)
