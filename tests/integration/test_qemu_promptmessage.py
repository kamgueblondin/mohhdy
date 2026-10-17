#!/usr/bin/env python3
"""QEMU contract, phase 4 PromptMessage (US-046..US-060, partial) in the guest shell.

Boots the default image, then from the Multiboot shell: validates the
/pm/*.pm samples shipped in the initrd, runs a program that uses a library,
writes files and registers triggers, fires a trigger, runs the automated
tests, prints the documentation, compiles to a PMC1 image and executes the
image, rejects a tampered image, breaks in the debugger, versions a source,
runs an inline statement and authors a program with the line editor
(pm-edit and pm-ide). Also filters the local catalog, copies one sample,
and checks a local Schnorr record (pm-ca-init, pm-ca-sign, pm-ca-verify,
including a tampered source). Translates hello.pm to C99 (pm-cross), builds
and runs it on the host, and runs the rule based assistant (pm-assist).
No network marketplace, no public CA, no model.
"""
import os
import re
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_ipc_foundation as h  # noqa: E402

ROOT = h.ROOT
h.LOG = os.path.join(h.LOG_DIR, "promptmessage.log")
h.ERR = os.path.join(h.LOG_DIR, "promptmessage.err")
h.MON = os.path.join(h.LOG_DIR, "promptmessage-monitor.sock")
KEY_DELAY = float(os.environ.get("PM_KEY_DELAY", "0.12"))

KEYS = {" ": "spc", "-": "minus", "/": "slash", ".": "dot", "\"": "shift-apostrophe",
        "$": "shift-4", "+": "shift-equal", "=": "equal", "<": "shift-comma",
        ">": "shift-dot", "%": "shift-5", "#": "shift-3", "_": "shift-minus",
        "~": "shift-grave_accent", ":": "shift-semicolon", ",": "comma"}


def key_name(char):
    if char in KEYS:
        return KEYS[char]
    if char.isupper():
        return "shift-" + char.lower()
    return char


def type_line(client, line):
    for char in line:
        client.sendall(("sendkey %s %d\n" % (key_name(char), h.KEY_HOLD_MS)).encode("ascii"))
        time.sleep(KEY_DELAY)
    client.sendall(("sendkey ret %d\n" % h.KEY_HOLD_MS).encode("ascii"))


def run(client, proc, line, marker, timeout=30):
    """Types the line once, checks the guest read it verbatim, waits marker."""
    start = len(h.log_text())
    type_line(client, line)
    h.wait_for("ligne lue: " + line, proc, start, timeout=timeout)
    h.wait_for(marker, proc, start, timeout=timeout)
    return h.normalized_log(h.log_text()[start:])


def expect(chunk, needle):
    if needle not in chunk:
        raise RuntimeError("missing %r in: %s" % (needle, chunk[-1500:]))


def check_cross(raw):
    """Builds the C printed by pm-cross with the host cc and runs it."""
    import tempfile
    begin = raw.find("pm-cross begin\n")
    end = raw.find("pm-cross end", begin)
    if begin < 0 or end < 0:
        raise RuntimeError("pm-cross markers missing: %s" % raw[-800:])
    src = raw[begin + len("pm-cross begin\n"):end].replace("\r\n", "\n")
    with tempfile.TemporaryDirectory() as tmp:
        with open(os.path.join(tmp, "app.c"), "w") as handle:
            handle.write(src)
        subprocess.run(["cc", "-std=c99", "-w", "-o", "app", "app.c"], cwd=tmp, check=True,
                       timeout=60)
        env = dict(os.environ, MOHHDY_MEM="10")
        run1 = subprocess.run(["./app"], cwd=tmp, env=env, capture_output=True, text=True,
                              timeout=30)
        os.remove(os.path.join(tmp, "journal.txt"))
        run2 = subprocess.run(["./app", "bonjour"], cwd=tmp, env=env, capture_output=True,
                              text=True, timeout=30)
        with open(os.path.join(tmp, "journal.txt")) as handle:
            journal = handle.read()
    if run1.returncode != 0 or run1.stdout != "pm> bonjour monde\npm> memoire ok\n":
        raise RuntimeError("pm-cross binary: rc %d out %r" % (run1.returncode, run1.stdout))
    if run2.stdout != run1.stdout + "pm> salut monde\n":
        raise RuntimeError("pm-cross trigger: %r" % run2.stdout)
    if journal.count("ligne") != 2:
        raise RuntimeError("pm-cross journal.txt: %r" % journal)


def main():
    os.makedirs(h.LOG_DIR, exist_ok=True)
    for path in (h.LOG, h.ERR, h.MON):
        try:
            os.remove(path)
        except OSError:
            pass
    command = [
        os.environ.get("PM_QEMU", "qemu-system-i386"), "-kernel", h.KERNEL, "-initrd", h.INITRD,
        "-m", "1024M", "-display", "none", "-vga", "none",
        "-serial", "file:" + h.LOG,
        "-monitor", "unix:%s,server,nowait" % h.MON,
        "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
    ]
    t0 = time.monotonic()
    with open(h.ERR, "wb") as err_handle:
        proc = subprocess.Popen(command, stdout=err_handle, stderr=err_handle)
        client = None
        try:
            h.wait_for("[0m : ", proc, timeout=60)
            client = h.connect_monitor()
            # Validation (US-049): good and bad sources.
            out = run(client, proc, "pm-check /pm/hello.pm", "pm-check ok")
            expect(out, "pm-check ok /pm/hello.pm statements")
            out = run(client, proc, "pm-check /pm/bad.pm", "pm-check error")
            expect(out, "pm-check error line 2 col 7: expected 'to'")
            # Compiler + VM (US-047/048), library (US-053), file writes.
            out = run(client, proc, "pm-run /pm/hello.pm", "pm-run ok")
            for needle in ("pm> bonjour monde", "pm> memoire ok", "writes 3"):
                expect(out, needle)
            out = run(client, proc, "cat document.txt", "Hello MOHHDY")
            out = run(client, proc, "cat journal.txt", "ligne")
            if out.count("ligne") < 3:  # typed command echo + two lines
                raise RuntimeError("journal.txt: %s" % out[-400:])
            # Triggers: "when user says".
            out = run(client, proc, "pm-say bonjour", "pm-say ok")
            expect(out, "pm> salut monde")
            out = run(client, proc, "pm-say ouvre mes photos", "pm-say ok")
            expect(out, "pm> galerie ~/Pictures")
            out = run(client, proc, "pm-say rien", "pm-say error")
            expect(out, "pm-say error no trigger")
            # Automated tests (US-057).
            out = run(client, proc, "pm-test /pm/tests.pm", "pm-test ok")
            expect(out, "pm-test ok /pm/tests.pm expects 3 failed 0")
            # Documentation (US-051).
            out = run(client, proc, "pm-doc /pm/hello.pm", "pm-doc ok")
            for needle in ("pm-doc about PromptMessage demo", "pm-doc trigger \"bonjour\"",
                           "pm-doc variable who", "libraries 1"):
                expect(out, needle)
            # Bytecode image (PMC1) with checksum; tamper is rejected.
            out = run(client, proc, "pm-compile /pm/hello.pm hello.pmc", "pm-compile ok")
            expect(out, "pm-compile ok hello.pmc bytes")
            out = run(client, proc, "pm-exec hello.pmc", "pm-exec ok")
            expect(out, "pm> bonjour monde")
            out = run(client, proc, "pm-exec /pm/lib.pm", "pm-exec error")
            expect(out, "pm-exec error image rejected")
            # Debugger (US-052): break before line 5 with variables.
            out = run(client, proc, "pm-debug /pm/hello.pm 5", "pm-debug break")
            for needle in ("pm-break line 5", "pm-var greeting=bonjour", "pm-var who=monde"):
                expect(out, needle)
            out = run(client, proc, "pm-disasm /pm/lib.pm", "pm-disasm ok")
            expect(out, "STORE 0")
            # Versioning (US-054).
            out = run(client, proc, "pm-version /pm/lib.pm", "pm-version ok")
            expect(out, "pm-version ok /pm/lib.pm.v1 checksum")
            out = run(client, proc, "pm-versions /pm/lib.pm", "pm-versions ok 1")
            # Inline statement keeps the quotes.
            out = run(client, proc, "pm print \"inline \" + 6 + 1", "pm ok")
            expect(out, "pm> inline 61")
            out = run(client, proc, "pm create file \"../x\" with content \"no\"", "pm error")
            expect(out, "pm error path denied")
            # Integrated editor (US-050).
            run(client, proc, "pm-edit mine.pm", "pm-edit>")
            run(client, proc, "set n to 40 + 2", "pm-edit>")
            run(client, proc, "if $n > 41 then print \"n=\" + $n", "pm-edit>")
            out = run(client, proc, ".", "pm-edit check ok")
            expect(out, "pm-edit saved mine.pm")
            out = run(client, proc, "pm-run mine.pm", "pm-run ok")
            expect(out, "pm> n=42")
            # US-055 partial: C99 target. The guest prints the C program;
            # the host compiler builds it and the binary prints what the
            # guest VM printed (pm-run hello.pm earlier, then a trigger).
            start = len(h.log_text())
            out = run(client, proc, "pm-cross /pm/hello.pm", "pm-cross ok target=c99", timeout=60)
            raw = h.log_text()[start:]
            check_cross(raw)
            # US-058 partial: rule based assistant (no model).
            out = run(client, proc, "pm-assist /pm/bad.pm", "pm-assist error")
            expect(out, "write 'set NAME to VALUE'")
            out = run(client, proc, "pm-assist /pm/hello.pm", "pm-assist ok")
            expect(out, "engine=rules")
            # Local catalog, install, certification record (US-059/060 local).
            out = run(client, proc, "pm-catalog", "pm-catalog ok 4")
            expect(out, "pm-catalog hello.pm - PromptMessage demo")
            out = run(client, proc, "pm-install tests.pm", "pm-install ok")
            out = run(client, proc, "pm-certify tests.pm", "pm-certify ok")
            expect(out, "pmcert1 sum=")
            out = run(client, proc, "pm-verify tests.pm", "pm-verify ok")
            run(client, proc, "append tests.pm expect 1 == 1", "append ok")
            out = run(client, proc, "pm-verify tests.pm", "pm-verify mismatch")
            out = run(client, proc, "pm-certify /pm/lib.pm", "pm-certify refused")
            expect(out, "no expect")
            # US-050 partial: pm-ide is the line editor, not a full-screen IDE.
            run(client, proc, "pm-ide ide.pm", "pm-ide>")
            run(client, proc, "set n to 7", "pm-ide>")
            out = run(client, proc, ".l", "1: set n to 7")
            out = run(client, proc, ".c", "pm-ide check ok")
            expect(out, "statements")
            out = run(client, proc, ".r", "pm-ide run ok")
            out = run(client, proc, ".", "pm-ide saved ide.pm")
            expect(out, "pm-ide check ok")
            out = run(client, proc, "pm-run ide.pm", "pm-run ok")
            # US-059 partial: local name filter and local copy, not a marketplace.
            out = run(client, proc, "pm-catalog-search lib", "pm-catalog ok 1")
            expect(out, "pm-catalog lib.pm")
            if "hello.pm" in out or "tests.pm" in out or "bad.pm" in out:
                raise RuntimeError("catalog search was not filtered: %s" % out[-800:])
            out = run(client, proc, "pm-catalog-fetch lib.pm", "pm-catalog-fetch ok")
            expect(out, "pm-catalog-fetch ok lib.pm")
            # US-060 partial: local Schnorr CA. Modexp is slow under TCG.
            out = run(client, proc, "pm-ca-init", "pm-ca-init ok", timeout=180)
            expect(out, "/etc/pm_ca.pub")
            out = run(client, proc, "pm-ca-sign ide.pm", "pm-ca-sign ok", timeout=180)
            expect(out, "ide.pm.ca")
            out = run(client, proc, "pm-ca-verify ide.pm", "pm-ca-verify ok", timeout=180)
            expect(out, "trust_chain verified ide.pm")
            out = run(client, proc, "append ide.pm set z to 1", "append ok")
            out = run(client, proc, "pm-ca-verify ide.pm", "pm-ca-verify error signature invalid", timeout=180)
            print("PromptMessage QEMU contract passed in %.0f s" % (time.monotonic() - t0))
            return 0
        finally:
            if client is not None:
                try:
                    client.sendall(b"quit\n")
                except OSError:
                    pass
                client.close()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:  # noqa: BLE001
        print("PromptMessage QEMU contract failed: %s" % error)
        raise SystemExit(1)
