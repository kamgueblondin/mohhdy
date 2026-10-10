#!/usr/bin/env python3
"""Phase 4/6 partials, one i386 guest with a VGA device (no network).

- US-077: screen-adapt WxH re-lays out the VBE desktop: the framebuffer
  QEMU shows (screendump) takes that size and the kernel logs it.
- US-081: power profiles are applied: the kernel preemption quantum read
  back through SYS_SCHED_TUNE follows the profile.
- US-050: pm-ide full-screen PromptMessage editor driven with keys only:
  typed program saved and checked, error shown in the status line.
"""
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_production as prodc  # noqa: E402

nw = prodc.nw
nw.LOG = os.path.join(nw.LOG_DIR, "desktop-partials.log")
nw.ERR = os.path.join(nw.LOG_DIR, "desktop-partials.err")
nw.MON = os.path.join(nw.LOG_DIR, "desktop-partials-monitor.sock")
run = prodc.run
KEYS = dict(prodc.KEYS)


def keys(mon, text):
    """Raw keys for full-screen apps (no shell echo to wait for)."""
    for char in text:
        mon.sendall(("sendkey %s %d\n" % (KEYS.get(char, char.lower()), nw.KEY_HOLD_MS)).encode("ascii"))
        time.sleep(0.3)


def key(mon, name):
    mon.sendall(("sendkey %s %d\n" % (name, nw.KEY_HOLD_MS)).encode("ascii"))
    time.sleep(0.4)


def ppm_size(path):
    with open(path, "rb") as handle:
        data = handle.read(64)
    parts = data.split()
    if parts[0] != b"P6":
        raise RuntimeError("not a PPM screendump")
    return int(parts[1]), int(parts[2])


def screen(mon, proc, label, want):
    dump = os.path.join(nw.LOG_DIR, "desktop-partials-%s.ppm" % label)
    try:
        os.remove(dump)
    except OSError:
        pass
    for _ in range(20):
        mon.sendall(("screendump %s\n" % dump).encode("ascii"))
        time.sleep(1.0)
        if os.path.exists(dump) and os.path.getsize(dump) > 64:
            size = ppm_size(dump)
            if size == want:
                return size
    raise RuntimeError("screendump %s is %r, want %r" % (label, ppm_size(dump) if os.path.exists(dump) else None, want))


def gui_at(mon, proc, w, h):
    start = len(nw.log_text())
    run(mon, proc, "screen-adapt %dx%d" % (w, h), "desktop relayout %dx%d" % (w, h))
    start = len(nw.log_text())
    prodc.send_exact(mon, proc, "gui")
    nw.wait_for("osui gui fb adapt %dx%d" % (w, h), proc, start, timeout=40)
    size = screen(mon, proc, "%dx%d" % (w, h), (w, h))
    key(mon, "esc")
    nw.wait_for("osui gui exit", proc, start, timeout=30)
    nw.wait_for("(-.-)", proc, start, timeout=30)
    return size


def main():
    os.makedirs(nw.LOG_DIR, exist_ok=True)
    for path in (nw.LOG, nw.ERR, nw.MON):
        try:
            os.remove(path)
        except OSError:
            pass
    command = [
        "qemu-system-i386", "-kernel", prodc.KERNEL, "-initrd", nw.INITRD,
        "-m", "1024M", "-display", "none", "-vga", "std",
        "-serial", "file:" + nw.LOG,
        "-monitor", "unix:%s,server,nowait" % nw.MON,
        "-machine", "type=pc,accel=tcg", "-cpu", "max",
        "-no-reboot", "-no-shutdown",
    ]
    t0 = time.time()
    with open(nw.ERR, "wb") as err_handle:
        proc = subprocess.Popen(command, stdout=err_handle, stderr=err_handle)
        mon = None
        try:
            nw.wait_for("(-.-)", proc, timeout=90)
            mon = nw.connect_monitor()
            time.sleep(0.5)
            # US-081 power profile applied to the kernel scheduler.
            run(mon, proc, "power-profile saver", "quantum_ticks 40 kernel_quantum 40 preemptions ")
            run(mon, proc, "power-profile performance", "quantum_ticks 10 kernel_quantum 10 preemptions ")
            run(mon, proc, "power-status", "kernel_quantum 10", attempts=2)
            run(mon, proc, "power-profile balanced", "kernel_quantum 20")
            # US-077 desktop re-laid out at the adapted size.
            a = gui_at(mon, proc, 800, 600)
            b = gui_at(mon, proc, 640, 480)
            run(mon, proc, "screen-adapt host", "screen-adapt ok desktop follows the host window")
            # US-050 full-screen editor, keys only.
            start = len(nw.log_text())
            prodc.send_exact(mon, proc, "pm-ide /t.pm")
            nw.wait_for("pm-ide open /t.pm lines 1", proc, start, timeout=30)
            time.sleep(1.0)
            keys(mon, "expect 4 == 4")
            key(mon, "ret")
            keys(mon, "expect 5 == 5")
            key(mon, "esc")
            keys(mon, "x")
            nw.wait_for("pm-ide saved /t.pm lines 2 status: saved, check ok statements 2", proc, start, timeout=40)
            nw.wait_for("pm-ide screen 0: PromptMessage IDE /t.pm", proc, start, timeout=10)
            nw.wait_for("pm-ide screen 2: 1 expect 4 == 4", proc, start, timeout=10)
            nw.wait_for("pm-ide closed /t.pm", proc, start, timeout=30)
            nw.wait_for("(-.-)", proc, start, timeout=30)
            run(mon, proc, "pm-check /t.pm", "pm-check ok /t.pm statements 2", attempts=2)
            # an error is reported in the status line, then quit without saving
            start = len(nw.log_text())
            prodc.send_exact(mon, proc, "pm-ide /t.pm")
            nw.wait_for("pm-ide open /t.pm lines 2", proc, start, timeout=30)
            time.sleep(1.0)
            key(mon, "down")
            keys(mon, "zz")
            key(mon, "esc")
            keys(mon, "c")
            nw.wait_for("pm-ide checked /t.pm lines 2 status: error line ", proc, start, timeout=40)
            key(mon, "esc")
            keys(mon, "q")
            key(mon, "esc")
            keys(mon, "q")
            nw.wait_for("pm-ide closed /t.pm", proc, start, timeout=30)
            nw.wait_for("(-.-)", proc, start, timeout=30)
            run(mon, proc, "pm-check /t.pm", "pm-check ok /t.pm statements 2", attempts=2)
            print("MOHHDY desktop partials contract: PASS desktop %r %r in %.0fs" % (a, b, time.time() - t0))
            return 0
        finally:
            if mon is not None:
                mon.close()
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    proc.kill()


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:  # noqa: BLE001
        print("MOHHDY desktop partials contract failed: %s" % error)
        tail = nw.log_text()[-3000:]
        print(tail)
        sys.exit(1)
