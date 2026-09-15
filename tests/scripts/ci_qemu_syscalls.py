#!/usr/bin/env python3
"""Boot QEMU, type ls/cat/ps/uptime/mem/getpid via HMP sendkey, assert serial output.

Used by ci_qemu_smoke.sh so GitHub Actions actually exercises the initrd and
kernel syscalls, not only the boot prompt.
"""
from __future__ import print_function

import os
import re
import socket
import subprocess
import sys
import time

_TIMER_ALIVE = re.compile(r"TIMER_ALIVE: tick=\S+\n?")
_SCHEDULER = re.compile(r"\[SCHED\] switching to task \d+\s*")

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KERNEL = os.environ.get("KERNEL", os.path.join(ROOT, "build", "mohhdy.bin"))
INITRD = os.environ.get("INITRD", os.path.join(ROOT, "my_initrd.tar"))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.environ.get("LOG", os.path.join(LOG_DIR, "ci-qemu-serial.log"))
QEMU_ERR = os.environ.get("QEMU_ERR", os.path.join(LOG_DIR, "ci-qemu-stderr.log"))
MON_SOCK = os.environ.get("QEMU_MON_SOCK", os.path.join(LOG_DIR, "qemu-monitor.sock"))
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "18"))
CMD_TIMEOUT = float(os.environ.get("CMD_TIMEOUT", "30"))
QEMU_MEMORY = os.environ.get("QEMU_MEMORY", "1024M")
# Echo-confirmed keys do not need the old 0.24s blind gap; keep a short
# pause so HMP sendkey does not pile up under TCG.
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.08"))
KEY_RETRIES = int(os.environ.get("KEY_RETRIES", "3"))
# Wait after the first echo so a late duplicate scancode can be backspaced
# before `ret`. Matches ci_qemu_core_smoke.KEY_DUPLICATE_SETTLE_DELAY.
KEY_DUPLICATE_SETTLE_DELAY = float(os.environ.get("KEY_DUPLICATE_SETTLE_DELAY", "0.25"))
ACTIVE_PROC = None


def say(msg):
    sys.stdout.write(msg + "\n")
    sys.stdout.flush()


def log_text():
    if not os.path.isfile(LOG):
        return ""
    with open(LOG, "r", errors="replace") as f:
        return f.read()


def without_timer(text):
    """Drop async diagnostics that QEMU can splice onto a payload line."""
    text = _TIMER_ALIVE.sub("", text)
    return _SCHEDULER.sub("", text)


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
        if needle in without_timer(text[start:]):
            return len(text)
        time.sleep(0.15)
    raise RuntimeError("timeout waiting for %r in serial log" % needle)


def wait_needle(needle, timeout, proc):
    wait_needle_from(needle, timeout, proc, 0)


def parse_spawn_pid(text):
    key = "spawn ok pid "
    i = text.find(key)
    if i < 0:
        raise RuntimeError("spawn ok pid not in log")
    digits = []
    for ch in text[i + len(key):]:
        if ch.isdigit():
            digits.append(ch)
        elif digits:
            break
    if not digits:
        raise RuntimeError("no pid after spawn ok pid")
    return "".join(digits)


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
    """Read HMP replies until the socket is quiet so sendkey commands do not pile up."""
    mon.settimeout(0.05)
    while True:
        try:
            data = mon.recv(8192)
            if not data:
                break
        except socket.timeout:
            break


def command_from_keys(keys):
    key_text = {"spc": " ", "dot": ".", "equal": "=", "minus": "-"}
    command = []
    for key in keys:
        if key == "ret":
            return "".join(command)
        if key in key_text:
            command.append(key_text[key])
        elif len(key) == 1:
            command.append(key)
        else:
            return None
    return None


def key_to_char(key):
    key_text = {"spc": " ", "dot": ".", "equal": "=", "minus": "-"}
    if key in key_text:
        return key_text[key]
    if len(key) == 1:
        return key
    return None


def sendkeys_once(mon, keys):
    for k in keys:
        mon.sendall(("sendkey %s\n" % k).encode("ascii"))
        drain_monitor(mon)
        time.sleep(KEY_DELAY)


def _echoed_char_count(text, char):
    pattern = r"SYS_GETS: caractère ajouté:\s*'%s'" % re.escape(char)
    return len(re.findall(pattern, without_timer(text)))


def sendkeys(mon, keys):
    """Type a command, confirm each character, then submit `ret` once.

    Never re-sends `ret` after a mismatched echo: a duplicated scancode
    (`newwd` instead of `newd`) already mutated the overlay.
    """
    expected = command_from_keys(keys)
    if expected is None or ACTIVE_PROC is None:
        sendkeys_once(mon, keys)
        return
    mark = len(log_text())
    for key in keys:
        if key == "ret":
            continue
        char = key_to_char(key)
        if char is None:
            sendkeys_once(mon, keys)
            return
        count = 0
        for _ in range(KEY_RETRIES):
            start = len(log_text())
            sendkeys_once(mon, [key])
            deadline = time.time() + 3
            while time.time() < deadline:
                if ACTIVE_PROC.poll() is not None:
                    raise RuntimeError("QEMU exited early with code %s" % ACTIVE_PROC.returncode)
                count = _echoed_char_count(log_text()[start:], char)
                if count:
                    time.sleep(KEY_DUPLICATE_SETTLE_DELAY)
                    count = _echoed_char_count(log_text()[start:], char)
                    break
                time.sleep(0.05)
            if count:
                break
        if count == 0:
            raise RuntimeError("character not received: %s" % char)
        for _ in range(count - 1):
            sendkeys_once(mon, ["backspace"])
            time.sleep(KEY_DUPLICATE_SETTLE_DELAY)
    sendkeys_once(mon, ["ret"])
    deadline = time.time() + CMD_TIMEOUT
    while time.time() < deadline:
        if ACTIVE_PROC.poll() is not None:
            raise RuntimeError("QEMU exited early with code %s" % ACTIVE_PROC.returncode)
        text = without_timer(log_text()[mark:])
        received = re.findall(r"SYS_GETS: ligne lue: ([^\r\n]+)", text)
        if expected in received:
            return
        if received:
            raise RuntimeError("command echo mismatch: expected %r got %r" % (expected, received))
        time.sleep(0.15)
    raise RuntimeError("timeout waiting for command echo %r" % expected)


def dump_logs():
    text = log_text()
    if text:
        say("=== serial log (tail) ===")
        say("\n".join(text.splitlines()[-80:]))
    err = err_text()
    if err:
        say("=== qemu stderr (tail) ===")
        say(err[-2000:])


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
        say("ERROR: missing %s or %s — run 'make all' first" % (KERNEL, INITRD))
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
        "-m", QEMU_MEMORY,
        "-display", "none",
        "-vga", "none",
        "-serial", "file:" + LOG,
        "-monitor", "unix:%s,server,nowait" % MON_SOCK,
        "-machine", "type=pc,accel=tcg",
        "-no-reboot",
        "-no-shutdown",
    ]
    say("=== QEMU syscall smoke (%s, key delay %.2fs, sendkey ls/cat/stat/test/alias/unalias/export/ai/ps/spawn/kill/uptime/mem/getpid/whoami/which/mkdir/cd/cp/mv/write/touch/append/grep/wc/sort/head/tail) ===" % (QEMU_MEMORY, KEY_DELAY))
    err_f = open(QEMU_ERR, "wb")
    proc = subprocess.Popen(
        cmd,
        stdout=err_f,
        stderr=err_f,
        start_new_session=True,
    )
    global ACTIVE_PROC
    ACTIVE_PROC = proc
    mon = None
    try:
        wait_needle("(-.-)", BOOT_TIMEOUT, proc)
        wait_needle("SYS_GETS: Debut", BOOT_TIMEOUT, proc)
        time.sleep(0.4)
        mon = monitor_connect()

        commands = [
            ("ls", ["l", "s", "ret"], "Initrd / VFS"),
            ("cat hello.txt",
             ["c", "a", "t", "spc", "h", "e", "l", "l", "o", "dot", "t", "x", "t", "ret"],
             "demonstration"),
            ("ls bin", ["l", "s", "spc", "b", "i", "n", "ret"], "fake_ai"),
            ("export tag=ok",
             ["e", "x", "p", "o", "r", "t", "spc", "t", "a", "g", "equal", "o", "k", "ret"],
             "export ok tag"),
            ("alias ll=whoami",
             ["a", "l", "i", "a", "s", "spc", "l", "l", "equal", "w", "h", "o", "a", "m", "i", "ret"],
             "alias ok ll"),
            ("ll", ["l", "l", "ret"], "whoami ok root"),
            ("unalias ll",
             ["u", "n", "a", "l", "i", "a", "s", "spc", "l", "l", "ret"],
             "unalias ok ll"),
            ("which ls",
             ["w", "h", "i", "c", "h", "spc", "l", "s", "ret"],
             "which ok builtin ls"),
            ("ai hello",
             ["a", "i", "spc", "h", "e", "l", "l", "o", "ret"],
             "[GPT-2 local]"),
            ("ps", ["p", "s", "ret"], "Processus (noyau)"),
        ]
        for name, keys, needle in commands:
            say("typing %s ..." % name)
            sendkeys(mon, keys)
            wait_needle(needle, CMD_TIMEOUT, proc)

        say("typing spawn idle ...")
        mark = len(log_text())
        sendkeys(mon, ["s", "p", "a", "w", "n", "spc", "i", "d", "l", "e", "ret"])
        wait_needle_from("spawn ok pid", CMD_TIMEOUT, proc, mark)
        idle_pid = parse_spawn_pid(log_text()[mark:])
        say("spawned idle pid %s" % idle_pid)

        say("typing ps (after spawn) ...")
        mark = len(log_text())
        sendkeys(mon, ["p", "s", "ret"])
        wait_needle_from("user  idle", CMD_TIMEOUT, proc, mark)

        say("typing kill %s ..." % idle_pid)
        mark = len(log_text())
        sendkeys(mon, ["k", "i", "l", "l", "spc"] + list(idle_pid) + ["ret"])
        kill_needle = "Processus %s termine" % idle_pid
        wait_needle_from(kill_needle, CMD_TIMEOUT, proc, mark)
        time.sleep(0.3)

        say("typing ps (after kill) ...")
        mark = len(log_text())
        sendkeys(mon, ["p", "s", "ret"])
        wait_needle_from("Processus (noyau)", CMD_TIMEOUT, proc, mark)
        after_kill_ps = log_text()[mark:]
        if "user  idle" in after_kill_ps:
            raise RuntimeError("idle still listed in ps after kill %s" % idle_pid)

        say("typing uptime ...")
        sendkeys(mon, ["u", "p", "t", "i", "m", "e", "ret"])
        wait_needle("PIT ticks", CMD_TIMEOUT, proc)

        say("typing mem ...")
        mark = len(log_text())
        sendkeys(mon, ["m", "e", "m", "ret"])
        wait_needle_from("mem ok", CMD_TIMEOUT, proc, mark)

        say("typing getpid ...")
        mark = len(log_text())
        sendkeys(mon, ["g", "e", "t", "p", "i", "d", "ret"])
        wait_needle_from("getpid ok", CMD_TIMEOUT, proc, mark)

        say("typing test f hello.txt ...")
        mark = len(log_text())
        sendkeys(mon, [
            "t", "e", "s", "t", "spc", "f", "spc",
            "h", "e", "l", "l", "o", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("test ok file hello.txt", CMD_TIMEOUT, proc, mark)

        say("typing mkdir mydir ...")
        mark = len(log_text())
        sendkeys(mon, ["m", "k", "d", "i", "r", "spc", "m", "y", "d", "i", "r", "ret"])
        wait_needle_from("mkdir ok mydir", CMD_TIMEOUT, proc, mark)

        say("typing stat mydir ...")
        mark = len(log_text())
        sendkeys(mon, ["s", "t", "a", "t", "spc", "m", "y", "d", "i", "r", "ret"])
        wait_needle_from("stat dir mydir", CMD_TIMEOUT, proc, mark)

        say("typing test d mydir ...")
        mark = len(log_text())
        sendkeys(mon, [
            "t", "e", "s", "t", "spc", "d", "spc", "m", "y", "d", "i", "r", "ret",
        ])
        wait_needle_from("test ok dir mydir", CMD_TIMEOUT, proc, mark)

        say("typing cd mydir ...")
        mark = len(log_text())
        sendkeys(mon, ["c", "d", "spc", "m", "y", "d", "i", "r", "ret"])
        wait_needle_from("cd ok mydir", CMD_TIMEOUT, proc, mark)

        say("typing pwd ...")
        mark = len(log_text())
        sendkeys(mon, ["p", "w", "d", "ret"])
        wait_needle_from("/mydir", CMD_TIMEOUT, proc, mark)

        say("typing mkdir sub ...")
        mark = len(log_text())
        sendkeys(mon, ["m", "k", "d", "i", "r", "spc", "s", "u", "b", "ret"])
        wait_needle_from("mkdir ok sub", CMD_TIMEOUT, proc, mark)

        say("typing ls (inside mydir) ...")
        mark = len(log_text())
        sendkeys(mon, ["l", "s", "ret"])
        wait_needle_from("sub", CMD_TIMEOUT, proc, mark)

        say("typing cd .. (to root) ...")
        mark = len(log_text())
        sendkeys(mon, ["c", "d", "spc", "dot", "dot", "ret"])
        wait_needle_from("cd ok ..", CMD_TIMEOUT, proc, mark)

        say("typing rmdir mydir (not empty) ...")
        mark = len(log_text())
        sendkeys(mon, ["r", "m", "d", "i", "r", "spc", "m", "y", "d", "i", "r", "ret"])
        wait_needle_from("repertoire non vide", CMD_TIMEOUT, proc, mark)

        say("typing cd mydir (cleanup sub) ...")
        mark = len(log_text())
        sendkeys(mon, ["c", "d", "spc", "m", "y", "d", "i", "r", "ret"])
        wait_needle_from("cd ok mydir", CMD_TIMEOUT, proc, mark)

        say("typing rmdir sub ...")
        mark = len(log_text())
        sendkeys(mon, ["r", "m", "d", "i", "r", "spc", "s", "u", "b", "ret"])
        wait_needle_from("rmdir ok sub", CMD_TIMEOUT, proc, mark)

        say("typing cd .. ...")
        mark = len(log_text())
        sendkeys(mon, ["c", "d", "spc", "dot", "dot", "ret"])
        wait_needle_from("cd ok ..", CMD_TIMEOUT, proc, mark)

        say("typing cp hello.txt copy.txt ...")
        mark = len(log_text())
        sendkeys(mon, [
            "c", "p", "spc", "h", "e", "l", "l", "o", "dot", "t", "x", "t",
            "spc", "c", "o", "p", "y", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("cp ok copy.txt", CMD_TIMEOUT, proc, mark)

        say("typing mv copy.txt notes.txt ...")
        mark = len(log_text())
        sendkeys(mon, [
            "m", "v", "spc", "c", "o", "p", "y", "dot", "t", "x", "t",
            "spc", "n", "o", "t", "e", "s", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("mv ok notes.txt", CMD_TIMEOUT, proc, mark)

        say("typing ls (after mv) ...")
        mark = len(log_text())
        sendkeys(mon, ["l", "s", "ret"])
        wait_needle_from("notes.txt", CMD_TIMEOUT, proc, mark)
        after_mv_ls = log_text()[mark:]
        if "copy.txt" in after_mv_ls:
            raise RuntimeError("copy.txt still listed after mv")

        say("typing cat notes.txt ...")
        mark = len(log_text())
        sendkeys(mon, [
            "c", "a", "t", "spc", "n", "o", "t", "e", "s", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("demonstration", CMD_TIMEOUT, proc, mark)

        say("typing rm notes.txt ...")
        mark = len(log_text())
        sendkeys(mon, [
            "r", "m", "spc", "n", "o", "t", "e", "s", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("rm ok notes.txt", CMD_TIMEOUT, proc, mark)

        say("typing ls (after rm notes) ...")
        mark = len(log_text())
        sendkeys(mon, ["l", "s", "ret"])
        wait_needle_from("Initrd / VFS", CMD_TIMEOUT, proc, mark)
        wait_needle_from("Total:", CMD_TIMEOUT, proc, mark)
        after_rm_notes = log_text()[mark:]
        if "notes.txt" in after_rm_notes:
            raise RuntimeError("notes.txt still listed in ls after rm")
        if "mydir" not in after_rm_notes:
            raise RuntimeError("ls after nested/mv missing mydir")
        if "hello.txt" not in after_rm_notes:
            raise RuntimeError("ls after nested/mv missing hello.txt")

        say("typing cp hello.txt mydir ...")
        mark = len(log_text())
        sendkeys(mon, [
            "c", "p", "spc", "h", "e", "l", "l", "o", "dot", "t", "x", "t",
            "spc", "m", "y", "d", "i", "r", "ret",
        ])
        wait_needle_from("cp ok hello.txt", CMD_TIMEOUT, proc, mark)

        say("typing ls mydir ...")
        mark = len(log_text())
        sendkeys(mon, ["l", "s", "spc", "m", "y", "d", "i", "r", "ret"])
        wait_needle_from("hello.txt", CMD_TIMEOUT, proc, mark)

        say("typing cp mydir cpd ...")
        mark = len(log_text())
        sendkeys(mon, [
            "c", "p", "spc", "m", "y", "d", "i", "r", "spc", "c", "p", "d", "ret",
        ])
        wait_needle_from("cp ok cpd", CMD_TIMEOUT, proc, mark)

        say("typing mv mydir newd ...")
        mark = len(log_text())
        sendkeys(mon, [
            "m", "v", "spc", "m", "y", "d", "i", "r", "spc", "n", "e", "w", "d", "ret",
        ])
        wait_needle_from("mv ok newd", CMD_TIMEOUT, proc, mark)

        say("typing ls (after mv dir) ...")
        mark = len(log_text())
        sendkeys(mon, ["l", "s", "ret"])
        wait_needle_from("Initrd / VFS", CMD_TIMEOUT, proc, mark)
        wait_needle_from("Total:", CMD_TIMEOUT, proc, mark)
        after_mv_dir = log_text()[mark:]
        if "mydir" in after_mv_dir:
            raise RuntimeError("mydir still listed after mv mydir newd")
        if "newd" not in after_mv_dir:
            raise RuntimeError("ls after mv dir missing newd")
        if "cpd" not in after_mv_dir:
            raise RuntimeError("ls after mv dir missing cpd copy")

        say("typing ls newd ...")
        mark = len(log_text())
        sendkeys(mon, ["l", "s", "spc", "n", "e", "w", "d", "ret"])
        wait_needle_from("hello.txt", CMD_TIMEOUT, proc, mark)

        say("typing cd newd ...")
        mark = len(log_text())
        sendkeys(mon, ["c", "d", "spc", "n", "e", "w", "d", "ret"])
        wait_needle_from("cd ok newd", CMD_TIMEOUT, proc, mark)

        say("typing cat hello.txt (in newd) ...")
        mark = len(log_text())
        sendkeys(mon, [
            "c", "a", "t", "spc", "h", "e", "l", "l", "o", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("demonstration", CMD_TIMEOUT, proc, mark)

        say("typing rm hello.txt (in newd) ...")
        mark = len(log_text())
        sendkeys(mon, [
            "r", "m", "spc", "h", "e", "l", "l", "o", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("rm ok hello.txt", CMD_TIMEOUT, proc, mark)

        say("typing cd .. (after mv dir) ...")
        mark = len(log_text())
        sendkeys(mon, ["c", "d", "spc", "dot", "dot", "ret"])
        wait_needle_from("cd ok ..", CMD_TIMEOUT, proc, mark)

        say("typing rmdir newd ...")
        mark = len(log_text())
        sendkeys(mon, ["r", "m", "d", "i", "r", "spc", "n", "e", "w", "d", "ret"])
        wait_needle_from("rmdir ok newd", CMD_TIMEOUT, proc, mark)

        say("typing ls (after rmdir) ...")
        mark = len(log_text())
        sendkeys(mon, ["l", "s", "ret"])
        wait_needle_from("Initrd / VFS", CMD_TIMEOUT, proc, mark)
        wait_needle_from("Total:", CMD_TIMEOUT, proc, mark)
        after_rmdir_ls = log_text()[mark:]
        if "mydir" in after_rmdir_ls:
            raise RuntimeError("mydir still listed in ls after rmdir")
        if "newd" in after_rmdir_ls:
            raise RuntimeError("newd still listed in ls after rmdir")
        if "cpd" not in after_rmdir_ls:
            raise RuntimeError("cpd missing after rmdir newd")

        say("typing cd cpd ...")
        mark = len(log_text())
        sendkeys(mon, ["c", "d", "spc", "c", "p", "d", "ret"])
        wait_needle_from("cd ok cpd", CMD_TIMEOUT, proc, mark)

        say("typing cat hello.txt (in cpd) ...")
        mark = len(log_text())
        sendkeys(mon, [
            "c", "a", "t", "spc", "h", "e", "l", "l", "o", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("demonstration", CMD_TIMEOUT, proc, mark)

        say("typing rm hello.txt (in cpd) ...")
        mark = len(log_text())
        sendkeys(mon, [
            "r", "m", "spc", "h", "e", "l", "l", "o", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("rm ok hello.txt", CMD_TIMEOUT, proc, mark)

        say("typing cd .. (after cp dir) ...")
        mark = len(log_text())
        sendkeys(mon, ["c", "d", "spc", "dot", "dot", "ret"])
        wait_needle_from("cd ok ..", CMD_TIMEOUT, proc, mark)

        say("typing rmdir cpd ...")
        mark = len(log_text())
        sendkeys(mon, ["r", "m", "d", "i", "r", "spc", "c", "p", "d", "ret"])
        wait_needle_from("rmdir ok cpd", CMD_TIMEOUT, proc, mark)

        say("typing ls (after rmdir cpd) ...")
        mark = len(log_text())
        sendkeys(mon, ["l", "s", "ret"])
        wait_needle_from("Initrd / VFS", CMD_TIMEOUT, proc, mark)
        wait_needle_from("Total:", CMD_TIMEOUT, proc, mark)
        after_rmdir_cpd = log_text()[mark:]
        if "cpd" in after_rmdir_cpd:
            raise RuntimeError("cpd still listed in ls after rmdir")

        say("typing touch z.txt ...")
        mark = len(log_text())
        sendkeys(mon, [
            "t", "o", "u", "c", "h", "spc", "z", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("touch ok z.txt", CMD_TIMEOUT, proc, mark)

        say("typing stat z.txt ...")
        mark = len(log_text())
        sendkeys(mon, [
            "s", "t", "a", "t", "spc", "z", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("stat file z.txt 0", CMD_TIMEOUT, proc, mark)

        say("typing append z.txt zap ...")
        mark = len(log_text())
        sendkeys(mon, [
            "a", "p", "p", "e", "n", "d", "spc", "z", "dot", "t", "x", "t",
            "spc", "z", "a", "p", "ret",
        ])
        wait_needle_from("append ok z.txt", CMD_TIMEOUT, proc, mark)

        say("typing stat z.txt (after append) ...")
        mark = len(log_text())
        sendkeys(mon, [
            "s", "t", "a", "t", "spc", "z", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("stat file z.txt 4", CMD_TIMEOUT, proc, mark)

        say("typing rm z.txt ...")
        mark = len(log_text())
        sendkeys(mon, ["r", "m", "spc", "z", "dot", "t", "x", "t", "ret"])
        wait_needle_from("rm ok z.txt", CMD_TIMEOUT, proc, mark)

        say("typing write hi.txt ping ...")
        mark = len(log_text())
        sendkeys(mon, [
            "w", "r", "i", "t", "e", "spc", "h", "i", "dot", "t", "x", "t",
            "spc", "p", "i", "n", "g", "ret",
        ])
        wait_needle_from("write ok hi.txt", CMD_TIMEOUT, proc, mark)

        say("typing cat hi.txt ...")
        mark = len(log_text())
        sendkeys(mon, ["c", "a", "t", "spc", "h", "i", "dot", "t", "x", "t", "ret"])
        wait_needle_from("ping", CMD_TIMEOUT, proc, mark)

        say("typing grep ping hi.txt ...")
        mark = len(log_text())
        sendkeys(mon, [
            "g", "r", "e", "p", "spc", "p", "i", "n", "g", "spc",
            "h", "i", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("grep hits 1", CMD_TIMEOUT, proc, mark)

        say("typing wc hi.txt ...")
        mark = len(log_text())
        sendkeys(mon, ["w", "c", "spc", "h", "i", "dot", "t", "x", "t", "ret"])
        wait_needle_from("wc ok 1 1 5 hi.txt", CMD_TIMEOUT, proc, mark)

        say("typing write lines.txt zulu ...")
        mark = len(log_text())
        sendkeys(mon, [
            "w", "r", "i", "t", "e", "spc", "l", "i", "n", "e", "s", "dot", "t", "x", "t",
            "spc", "z", "u", "l", "u", "ret",
        ])
        wait_needle_from("write ok lines.txt", CMD_TIMEOUT, proc, mark)

        say("typing append lines.txt alpha ...")
        mark = len(log_text())
        sendkeys(mon, [
            "a", "p", "p", "e", "n", "d", "spc", "l", "i", "n", "e", "s", "dot", "t", "x", "t",
            "spc", "a", "l", "p", "h", "a", "ret",
        ])
        wait_needle_from("append ok lines.txt", CMD_TIMEOUT, proc, mark)

        say("typing append lines.txt mango ...")
        mark = len(log_text())
        sendkeys(mon, [
            "a", "p", "p", "e", "n", "d", "spc", "l", "i", "n", "e", "s", "dot", "t", "x", "t",
            "spc", "m", "a", "n", "g", "o", "ret",
        ])
        wait_needle_from("append ok lines.txt", CMD_TIMEOUT, proc, mark)

        say("typing sort lines.txt ...")
        mark = len(log_text())
        sendkeys(mon, ["s", "o", "r", "t", "spc", "l", "i", "n", "e", "s", "dot", "t", "x", "t", "ret"])
        wait_needle_from("sort ok 3 lines.txt", CMD_TIMEOUT, proc, mark)
        sort_output = without_timer(log_text()[mark:])
        if "alpha\nmango\nzulu\nsort ok 3 lines.txt" not in sort_output:
            raise RuntimeError("sort lines.txt did not emit the expected ascending order")

        say("typing head -2 lines.txt ...")
        mark = len(log_text())
        sendkeys(mon, [
            "h", "e", "a", "d", "spc", "minus", "2", "spc", "l", "i", "n", "e", "s", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("head ok 2 lines.txt", CMD_TIMEOUT, proc, mark)
        head_output = without_timer(log_text()[mark:])
        if "zulu\nalpha\nhead ok 2 lines.txt" not in head_output:
            raise RuntimeError("head -2 lines.txt did not emit the first two lines")

        say("typing tail -1 lines.txt ...")
        mark = len(log_text())
        sendkeys(mon, [
            "t", "a", "i", "l", "spc", "minus", "1", "spc", "l", "i", "n", "e", "s", "dot", "t", "x", "t", "ret",
        ])
        wait_needle_from("tail ok 1 lines.txt", CMD_TIMEOUT, proc, mark)
        tail_output = without_timer(log_text()[mark:])
        if "mango\ntail ok 1 lines.txt" not in tail_output:
            raise RuntimeError("tail -1 lines.txt did not emit the final line")

        say("typing rm lines.txt ...")
        mark = len(log_text())
        sendkeys(mon, ["r", "m", "spc", "l", "i", "n", "e", "s", "dot", "t", "x", "t", "ret"])
        wait_needle_from("rm ok lines.txt", CMD_TIMEOUT, proc, mark)

        say("typing rm hi.txt ...")
        mark = len(log_text())
        sendkeys(mon, ["r", "m", "spc", "h", "i", "dot", "t", "x", "t", "ret"])
        wait_needle_from("rm ok hi.txt", CMD_TIMEOUT, proc, mark)

        say("typing ls (after rm hi) ...")
        mark = len(log_text())
        sendkeys(mon, ["l", "s", "ret"])
        wait_needle_from("Initrd / VFS", CMD_TIMEOUT, proc, mark)
        wait_needle_from("Total:", CMD_TIMEOUT, proc, mark)
        after_rm_hi = log_text()[mark:]
        if "hi.txt" in after_rm_hi:
            raise RuntimeError("hi.txt still listed in ls after rm")

        text = log_text()
        checks = [
            ("syscall ls", "Initrd / VFS"),
            ("initrd ls", "hello.txt"),
            ("initrd ls", "startup.sh"),
            ("cat hello.txt", "Un autre fichier de demonstration"),
            ("ls bin", "fake_ai"),
            ("ai local generation", "[GPT-2 local]"),
            ("ps kernel table", "Processus (noyau)"),
            ("ps kernel", "kern"),
            ("ps shell", "shell"),
            ("spawn idle", "spawn ok pid"),
            ("ps shows idle", "user  idle"),
            ("kill idle", "Processus %s termine" % idle_pid),
            ("uptime", "PIT ticks"),
            ("mem pmm", "mem ok"),
            ("getpid", "getpid ok"),
            ("whoami", "whoami ok root"),
            ("alias", "alias ok ll"),
            ("unalias", "unalias ok ll"),
            ("export", "export ok tag"),
            ("which builtin", "which ok builtin ls"),
            ("mkdir overlay", "mkdir ok mydir"),
            ("cd overlay", "cd ok mydir"),
            ("pwd overlay", "/mydir"),
            ("mkdir nested", "mkdir ok sub"),
            ("rmdir notempty", "repertoire non vide"),
            ("rmdir nested", "rmdir ok sub"),
            ("cp overlay", "cp ok copy.txt"),
            ("mv overlay", "mv ok notes.txt"),
            ("rm overlay", "rm ok notes.txt"),
            ("cp into dir", "cp ok hello.txt"),
            ("cp overlay dir", "cp ok cpd"),
            ("mv overlay dir", "mv ok newd"),
            ("cd renamed dir", "cd ok newd"),
            ("rmdir overlay", "rmdir ok newd"),
            ("cd copied dir", "cd ok cpd"),
            ("rmdir copied dir", "rmdir ok cpd"),
            ("write overlay", "write ok hi.txt"),
            ("touch overlay", "touch ok z.txt"),
            ("stat empty", "stat file z.txt 0"),
            ("append overlay", "append ok z.txt"),
            ("stat appended", "stat file z.txt 4"),
            ("grep overlay", "grep hits 1"),
            ("wc overlay", "wc ok 1 1 5 hi.txt"),
            ("sort overlay", "sort ok 3 lines.txt"),
            ("head overlay", "head ok 2 lines.txt"),
            ("tail overlay", "tail ok 1 lines.txt"),
            ("rm multiline", "rm ok lines.txt"),
            ("rm write", "rm ok hi.txt"),
            ("test file", "test ok file hello.txt"),
            ("stat dir", "stat dir mydir"),
            ("test dir", "test ok dir mydir"),
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
        say("QEMU syscall smoke passed.")
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
