#!/usr/bin/env python3
"""Lot 8 (US-076) contract: the Mohhdy ARM kernel core boots on
qemu-system-arm -M virt (cortex-a15) and proves, on its serial console:
MMU on with a PL1-only kernel section, portable kernel/sha256.c self-test,
GIC + generic timer, user tasks in USR mode entering by SVC, IPC through
the portable kernel/ipc.c mailbox, a kernel pointer refused by SYS_PUTS,
a user read of kernel memory killing only that task, and two tasks that
never yield both advancing (timer preemption)."""
import os
import re
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
IMAGE = os.path.join(ROOT, "build", "arm", "mohhdy-arm.elf")
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "arm-boot.log")
NEEDLES = (
    "Mohhdy ARM kernel (US-076) armv7-a",
    "arm mmu on: kernel section PL1-only, user section 0x40100000",
    "arm sha256 self-test ok (kernel/sha256.c)",
    "arm aes-gcm self-test ok (kernel/aes_gcm.c)",
    "arm edid self-test ok 1280x800 325x203mm 15.1in (kernel/edid.c)",
    "arm p2p flow self-test ok rate 62500 backoff 54687 (userspace/p2p_flow.c)",
    "arm gic + generic timer armed at 100 Hz",
    "arm tasks spawned 5 (USR mode)",
    "arm user reporter pid 0x00000001 running in USR mode via svc",
    "arm user ipc send ok",
    "arm user ipc received from pid 0x00000001 request 0x00c0ffee data moh",
    "arm user rogue puts kernel pointer rc 0xffffffff",
    "arm task 4 (rogue) killed: data abort (permission, kernel section) at 0x40000000",
)
REPORT = re.compile(r"arm preempt report ticks 150 switches (\d+) svc (\d+) refused (\d+) killed (\d+) "
                    r"spinner (\w+) spinner2 (\w+) .*spinner=(\d+) rogue=1 spinner2=(\d+)")


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    if not os.path.exists(IMAGE):
        raise RuntimeError("missing %s (make arm-kernel)" % IMAGE)
    with open(LOG, "wb") as out:
        proc = subprocess.Popen(["qemu-system-arm", "-M", "virt", "-cpu", "cortex-a15", "-m", "256M",
                                 "-nographic", "-nic", "none", "-monitor", "none", "-serial", "stdio",
                                 "-kernel", IMAGE], stdout=out, stderr=subprocess.STDOUT,
                                stdin=subprocess.DEVNULL)
    try:
        deadline = time.time() + 60
        text = ""
        while time.time() < deadline:
            with open(LOG, "rb") as f:
                text = f.read().decode("ascii", "replace").replace("\r", "")
            if REPORT.search(text) or "panic" in text or proc.poll() is not None:
                break
            time.sleep(0.2)
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=4)
        except subprocess.TimeoutExpired:
            proc.kill()
    for needle in NEEDLES:
        if needle not in text:
            raise RuntimeError("missing %r in ARM serial log:\n%s" % (needle, text[-2000:]))
    if "UNEXPECTED" in text or "FAILED" in text or "panic" in text:
        raise RuntimeError("ARM kernel reported a failure:\n%s" % text[-2000:])
    m = REPORT.search(text)
    if not m:
        raise RuntimeError("no preemption report:\n%s" % text[-2000:])
    switches, svc, refused, killed = (int(m.group(i)) for i in range(1, 5))
    s1, s2 = int(m.group(7)), int(m.group(8))
    if m.group(5) != "advanced" or m.group(6) != "advanced" or s1 < 20 or s2 < 20:
        raise RuntimeError("non-yielding tasks not both preempted in: %s" % m.group(0))
    if refused != 1 or killed != 1 or switches < 40 or svc < 10:
        raise RuntimeError("unexpected counters: %s" % m.group(0))
    print("arm: MMU, sha256, GIC timer, USR tasks via SVC, portable IPC, kernel pointer refused, "
          "rogue killed by permission fault, %d switches, spinners %d/%d slices" % (switches, s1, s2))
    print("QEMU ARM kernel boot contract passed.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU ARM kernel boot contract failed: %s" % error)
        raise SystemExit(1)
