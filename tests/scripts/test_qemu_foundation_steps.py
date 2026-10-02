#!/usr/bin/env python3
"""QEMU: jeton de capacite, retrait d'evenement, journal de montages apres reboot.

Hors CI. Un disque FAT reserve les LBA 4222-4223 au journal MNTJ, hors AIOV.
Le second boot est un nouveau processus QEMU sur la meme image.
"""
from __future__ import print_function

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
LOG = os.path.join(LOG_DIR, "qemu-foundation-steps.log")
ERR = os.path.join(LOG_DIR, "qemu-foundation-steps.err")
MON = os.path.join(LOG_DIR, "qemu-foundation-steps.monitor.sock")
DISK = os.path.join(ROOT, "build", "foundation-steps.img")
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "40"))
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.05"))
KEY_HOLD_MS = int(os.environ.get("KEY_HOLD_MS", "10"))
KEY_ECHO_TIMEOUT = float(os.environ.get("KEY_ECHO_TIMEOUT", "3"))


def say(message):
    sys.stdout.write(message + "\n")
    sys.stdout.flush()


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def normalized(output):
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?", "", output)
    output = re.sub(r"\[SCHED\] switching to task \d+\r?\n?", "", output)
    return output


def wait_for(proc, needle, timeout, start=0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped; tail:\n%s" % log_text()[-2000:])
        if needle in normalized(log_text()[start:]):
            time.sleep(0.2)
            return
        time.sleep(0.1)
    raise RuntimeError("timeout waiting for %r; tail:\n%s" % (needle, log_text()[-1500:]))


def monitor_connect():
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
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
    raise RuntimeError("monitor unavailable")


def send_key(client, key):
    client.sendall(("sendkey %s %d\n" % (key, KEY_HOLD_MS)).encode("ascii"))


def key_echo_count(output, char):
    pattern = r"SYS_GETS: caractère ajouté:\s*'%s'" % re.escape(char)
    return len(re.findall(pattern, normalized(output)))


def command_echoed(output, command):
    expected = " ".join(command.split())
    for received in re.findall(r"SYS_GETS: ligne lue: ([^\r\n]+)", normalized(output)):
        if " ".join(received.split()) == expected:
            return True
    return False


def send_command_once(client, proc, command):
    aliases = {" ": "spc", ".": "dot", "-": "minus", "/": "slash"}
    for char in command:
        count = 0
        for _ in range(3):
            start = len(log_text())
            send_key(client, aliases.get(char, char.lower()))
            deadline = time.monotonic() + KEY_ECHO_TIMEOUT
            while time.monotonic() < deadline:
                if proc.poll() is not None:
                    raise RuntimeError("QEMU stopped during keyboard input")
                time.sleep(KEY_DELAY)
                count = key_echo_count(log_text()[start:], char)
                if count:
                    break
            if count:
                break
        if count == 0:
            raise RuntimeError("missing keyboard echo for %r" % char)
        for _ in range(count - 1):
            send_key(client, "backspace")
            time.sleep(KEY_DELAY)
    send_key(client, "ret")


def run_command(proc, client, command, needle, timeout=25):
    start = len(log_text())
    send_command_once(client, proc, command)
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("guest died after %s" % command)
        output = normalized(log_text()[start:])
        if command_echoed(output, command):
            break
        if "SYS_GETS: ligne lue:" in output:
            raise RuntimeError("altered command %s: %s" % (command, output[-500:]))
        time.sleep(0.1)
    else:
        raise RuntimeError("command echo absent: %s" % command)
    wait_for(proc, needle, timeout, start)
    wait_for(proc, "(-.-)", 20, start)
    return log_text()[start:]


def parse_pid(chunk, prefix):
    match = re.search(re.escape(prefix) + r"\s+(\d+)", normalized(chunk))
    if not match:
        raise RuntimeError("pid absent after %s: %s" % (prefix, chunk[-400:]))
    return match.group(1)


def qemu_cmd():
    return [
        "qemu-system-i386", "-cpu", "pentium3", "-kernel", KERNEL,
        "-initrd", INITRD, "-m", "1024M", "-display", "none", "-vga", "none",
        "-serial", "file:" + LOG, "-monitor", "unix:%s,server,nowait" % MON,
        "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
        "-drive", "file=%s,format=raw,if=ide,cache=writethrough" % DISK,
    ]


def boot():
    try:
        os.remove(MON)
    except OSError:
        pass
    try:
        os.remove(LOG)
    except OSError:
        pass
    err = open(ERR, "ab")
    proc = subprocess.Popen(qemu_cmd(), cwd=ROOT, stdout=err, stderr=err)
    wait_for(proc, "(-.-)", BOOT_TIMEOUT)
    client = monitor_connect()
    return proc, client, err


def stop(proc, client, err):
    if client is not None:
        try:
            client.close()
        except OSError:
            pass
    if proc is not None and proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=4)
        except subprocess.TimeoutExpired:
            proc.kill()
    if err is not None:
        try:
            err.close()
        except OSError:
            pass


def prepare_disk():
    os.makedirs(os.path.dirname(DISK), exist_ok=True)
    subprocess.check_call(
        ["python3", os.path.join(ROOT, "tests", "scripts", "make_fat16_image.py"), "--image", DISK],
        cwd=ROOT,
    )


def main():
    if not os.path.isfile(KERNEL) or not os.path.isfile(INITRD):
        raise RuntimeError("missing KERNEL or INITRD")
    os.makedirs(LOG_DIR, exist_ok=True)
    prepare_disk()
    proc = client = err = None
    try:
        proc, client, err = boot()
        say("[foundation] first boot")
        chunk = run_command(proc, client, "getpid", "getpid ok ")
        shell_pid = parse_pid(chunk, "getpid ok")
        run_command(proc, client, "vfs-backend-grant " + shell_pid, "vfs-backend-grant ok")
        chunk = run_command(proc, client, "cap-token", "cap-token ok ")
        token = parse_pid(chunk, "cap-token ok")
        if token == "0":
            raise RuntimeError("token is zero")
        say("[foundation] cap-token %s" % token)
        run_command(proc, client, "service-publish demo", "service-publish ok")
        run_command(proc, client, "service-watch demo", "service-watch ok")
        chunk = run_command(proc, client, "service-find vfs", "service-find ok vfs")
        vfs_pid = parse_pid(chunk, "service-find ok vfs")
        run_command(proc, client, "service-grant demo " + vfs_pid, "service-grant ok")
        chunk = run_command(proc, client, "service-event-pull", "service-event-pull ok demo")
        if " demo " not in normalized(chunk):
            raise RuntimeError("event pull missing demo: %s" % chunk[-400:])
        say("[foundation] service-event-pull kept the grant")
        run_command(proc, client, "mount-journal-add alias/ overlay", "mount-journal-add ok alias/")
        chunk = run_command(proc, client, "mount-journal", "mount-journal ok ")
        if "alias/" not in chunk:
            raise RuntimeError("alias missing before reboot: %s" % chunk[-400:])
        say("[foundation] journal lists alias/ before reboot")
    finally:
        stop(proc, client, err)

    proc = client = err = None
    try:
        proc, client, err = boot()
        say("[foundation] second boot")
        chunk = run_command(proc, client, "mount-journal", "mount-journal ok ")
        if "alias/" not in chunk:
            raise RuntimeError("alias missing after reboot: %s" % chunk[-500:])
        if "initrd/" not in chunk or "overlay/" not in chunk:
            raise RuntimeError("default mounts missing after reboot: %s" % chunk[-500:])
        say("[foundation] alias/ survived reboot on LBA 4222")
        return 0
    finally:
        stop(proc, client, err)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        sys.stderr.write("QEMU foundation steps failed: %s\n" % error)
        raise SystemExit(1)
