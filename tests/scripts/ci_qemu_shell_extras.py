#!/usr/bin/env python3
"""Second QEMU boot: shell extras only (env/history/jobs/top/rc/which idle).

Kept separate from ci_qemu_syscalls.py so overlay sendkeys stay identical to #73
(extra keys in that session ghosted `cd ..` into `ccd ..`).
"""
from __future__ import print_function

import os
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KERNEL = os.environ.get("KERNEL", os.path.join(ROOT, "build", "mohhdy.bin"))
INITRD = os.environ.get("INITRD", os.path.join(ROOT, "my_initrd.tar"))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.environ.get("EXTRAS_LOG", os.path.join(LOG_DIR, "ci-qemu-extras-serial.log"))
QEMU_ERR = os.environ.get("EXTRAS_ERR", os.path.join(LOG_DIR, "ci-qemu-extras-stderr.log"))
MON_SOCK = os.environ.get("EXTRAS_MON_SOCK", os.path.join(LOG_DIR, "qemu-extras-monitor.sock"))
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "18"))
CMD_TIMEOUT = float(os.environ.get("CMD_TIMEOUT", "12"))
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.15"))


def say(msg):
    sys.stdout.write(msg + "\n")
    sys.stdout.flush()


def log_text():
    if not os.path.isfile(LOG):
        return ""
    with open(LOG, "r", errors="replace") as f:
        return f.read()


def err_text():
    if not os.path.isfile(QEMU_ERR):
        return ""
    with open(QEMU_ERR, "r", errors="replace") as f:
        return f.read()


def wait_needle_from(needle, timeout, proc, start):
    t0 = time.time()
    while time.time() - t0 < timeout:
        if proc.poll() is not None:
            raise RuntimeError(
                "QEMU exited early with code %s\nstderr: %s"
                % (proc.returncode, err_text()[-1500:])
            )
        text = log_text()
        if needle in text[start:]:
            return len(text)
        time.sleep(0.15)
    raise RuntimeError("timeout waiting for %r in serial log" % needle)


def wait_needle(needle, timeout, proc):
    wait_needle_from(needle, timeout, proc, 0)


def monitor_connect(retries=50):
    last = None
    for _ in range(retries):
        if not os.path.exists(MON_SOCK):
            time.sleep(0.1)
            continue
        try:
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            s.settimeout(1)
            s.connect(MON_SOCK)
            try:
                s.recv(4096)
            except socket.timeout:
                pass
            return s
        except (socket.error, OSError) as e:
            last = e
            try:
                s.close()
            except Exception:
                pass
            time.sleep(0.1)
    raise RuntimeError("cannot connect to QEMU monitor: %s" % last)


def drain_monitor(mon):
    mon.settimeout(0.05)
    while True:
        try:
            data = mon.recv(8192)
            if not data:
                break
        except socket.timeout:
            break


def sendkeys(mon, keys):
    for k in keys:
        mon.sendall(("sendkey %s\n" % k).encode("ascii"))
        drain_monitor(mon)
        time.sleep(KEY_DELAY)


def send_command_until(mon, name, keys, needle, proc, attempts=3):
    last_error = None
    for attempt in range(attempts):
        start = len(log_text())
        sendkeys(mon, keys)
        try:
            wait_needle_from(needle, CMD_TIMEOUT, proc, start)
            return
        except RuntimeError as e:
            last_error = e
            if attempt + 1 < attempts:
                say("retrying %s (%d/%d) ..." % (name, attempt + 2, attempts))
                time.sleep(0.25)
    raise last_error


def dump_logs():
    text = log_text()
    if text:
        say("=== extras serial log (tail) ===")
        say("\n".join(text.splitlines()[-80:]))
    err = err_text()
    if err:
        say("=== extras qemu stderr (tail) ===")
        say(err[-2000:])


def qemu_disk_args():
    disk = os.environ.get("OVERLAY_DISK", os.path.join(ROOT, "build", "overlay.img"))
    if os.path.isfile(disk):
        return ["-drive", "file=%s,format=raw,if=ide,cache=writethrough" % disk]
    return []


def kill_qemu(proc):
    if proc.poll() is not None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=2)
    except Exception:
        proc.kill()
        try:
            proc.wait(timeout=2)
        except Exception:
            pass


def main():
    if not os.path.isfile(KERNEL) or not os.path.isfile(INITRD):
        say("ERROR: missing %s or %s" % (KERNEL, INITRD))
        return 1

    os.makedirs(os.path.dirname(os.path.abspath(LOG)) or ".", exist_ok=True)
    for path in (LOG, QEMU_ERR, MON_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass

    cmd = [
        "qemu-system-i386",
        "-kernel", KERNEL,
        "-initrd", INITRD,
        "-m", "1024M",
        "-cpu", "pentium3",
        "-display", "none",
        "-vga", "none",
        "-serial", "file:" + LOG,
        "-monitor", "unix:%s,server,nowait" % MON_SOCK,
        "-machine", "type=pc,accel=tcg",
        "-no-reboot",
        "-no-shutdown",
    ] + qemu_disk_args()
    say("=== QEMU shell extras (history/jobs/top/env/date/echo/rc/which idle/aistats) ===")
    err_f = open(QEMU_ERR, "wb")
    proc = subprocess.Popen(
        cmd,
        stdout=err_f,
        stderr=err_f,
        start_new_session=True,
    )
    mon = None
    try:
        wait_needle("(-.-)", BOOT_TIMEOUT, proc)
        wait_needle("SYS_GETS: Debut", BOOT_TIMEOUT, proc)
        time.sleep(0.4)
        mon = monitor_connect()
        time.sleep(0.6)

        commands = [
            ("which idle",
             ["w", "h", "i", "c", "h", "spc", "i", "d", "l", "e", "ret"],
             "which ok bin/idle"),
            ("history", ["h", "i", "s", "t", "o", "r", "y", "ret"], "history ok"),
            ("jobs", ["j", "o", "b", "s", "ret"], "jobs ok"),
            ("top", ["t", "o", "p", "ret"], "top ok"),
            ("env", ["e", "n", "v", "ret"], "env ok"),
            ("date", ["d", "a", "t", "e", "ret"], "date ok"),
            ("echo hi", ["e", "c", "h", "o", "spc", "h", "i", "ret"], "echo ok"),
            ("task-priority 1 3",
             ["t", "a", "s", "k", "minus", "p", "r", "i", "o", "r", "i", "t", "y", "spc", "1", "spc", "3", "ret"],
             "task-priority ok 1 3"),
            ("task-metrics 1",
             ["t", "a", "s", "k", "minus", "m", "e", "t", "r", "i", "c", "s", "spc", "1", "ret"],
             "task-metrics ok 1 3"),
            ("rc", ["r", "c", "ret"], "rc ok 0"),
            ("test no zz",
             ["t", "e", "s", "t", "spc", "n", "o", "spc", "z", "z", "ret"],
             "test no zz"),
            ("rc after test no", ["r", "c", "ret"], "rc ok 1"),
            ("aistats", ["a", "i", "s", "t", "a", "t", "s", "ret"], "aistats ok"),
            ("aimode", ["a", "i", "m", "o", "d", "e", "ret"], "aimode ok on"),
            ("aihelp", ["a", "i", "h", "e", "l", "p", "ret"], "aihelp ok"),
        ]
        for name, keys, needle in commands:
            say("typing %s ..." % name)
            send_command_until(mon, name, keys, needle, proc)
            time.sleep(0.2)

        text = log_text()
        checks = [
            ("which bin", "which ok bin/idle"),
            ("history", "history ok"),
            ("jobs", "jobs ok"),
            ("top", "top ok"),
            ("env", "env ok"),
            ("date", "date ok"),
            ("echo", "echo ok"),
            ("task priority", "task-priority ok 1 3"),
            ("task metrics priority", "task-metrics ok 1 3"),
            ("rc success", "rc ok 0"),
            ("test missing", "test no zz"),
            ("rc fail", "rc ok 1"),
            ("aistats", "aistats ok"),
            ("aimode", "aimode ok on"),
            ("aihelp", "aihelp ok"),
        ]
        fail = 0
        for label, needle in checks:
            if needle in text:
                say("OK: %s (%r)" % (label, needle))
            else:
                say("FAIL: %s missing %r" % (label, needle))
                fail = 1
        if fail:
            dump_logs()
            return 1
        say("QEMU shell extras smoke passed.")
        return 0
    except Exception as e:
        say("FAIL: %s" % e)
        dump_logs()
        return 1
    finally:
        if mon is not None:
            try:
                mon.close()
            except Exception:
                pass
        kill_qemu(proc)
        err_f.close()
        try:
            os.remove(MON_SOCK)
        except OSError:
            pass


if __name__ == "__main__":
    sys.exit(main())
