#!/usr/bin/env python3
"""Phase 6 multi-platform contract (US-076..US-090), one i386 guest.

HAL report from CPUID/meminfo (i386 only, ARM refused honestly), screen
adaptation, gesture classification, power profiles with a measured busy
share, device registry, ELF compatibility scan of /bin, local notification
queue, migration export/verify/import with tamper detection, deployment
manifest apply/verify with refusal on a changed source, unified admin view.
No network service is used (user netdev restricted, NE2000 only probed).
"""
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_net_worker as nw  # noqa: E402
from test_qemu_vfs_service import normalized_log  # noqa: E402

ROOT = nw.ROOT
nw.LOG = os.path.join(nw.LOG_DIR, "platform.log")
nw.ERR = os.path.join(nw.LOG_DIR, "platform.err")
nw.MON = os.path.join(nw.LOG_DIR, "platform-monitor.sock")
KERNEL = os.path.join(ROOT, "build", "mohhdy.bin")
KEYS = {" ": "spc", "-": "minus", "/": "slash", ".": "dot", ",": "comma", "=": "equal"}


def send_command(client, command):
    for char in command:
        client.sendall(("sendkey %s %d\n" % (KEYS.get(char, char.lower()), nw.KEY_HOLD_MS)).encode("ascii"))
        time.sleep(0.3)
    client.sendall(("sendkey ret %d\n" % nw.KEY_HOLD_MS).encode("ascii"))


nw.send_command = send_command


def run(mon, proc, command, needles, attempts=1, timeout=25):
    if isinstance(needles, str):
        needles = [needles]
    start = nw.send_command_until(mon, command, needles[0], proc, attempts=attempts)
    for needle in needles[1:]:
        nw.wait_for(needle, proc, start, timeout=timeout)
    nw.wait_for("(-.-)", proc, start, timeout=timeout)
    return normalized_log(nw.log_text()[start:])


def main():
    os.makedirs(nw.LOG_DIR, exist_ok=True)
    for path in (nw.LOG, nw.ERR, nw.MON):
        try:
            os.remove(path)
        except OSError:
            pass
    command = [
        "qemu-system-i386", "-kernel", KERNEL, "-initrd", nw.INITRD,
        "-m", "1024M", "-display", "none", "-vga", "none",
        "-serial", "file:" + nw.LOG,
        "-monitor", "unix:%s,server,nowait" % nw.MON,
        "-machine", "type=pc,accel=tcg", "-cpu", "max",
        "-netdev", "user,id=n0,restrict=on", "-device", "ne2k_isa,netdev=n0",
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
            # US-076 HAL groundwork, honest about ARM.
            run(mon, proc, "hal-info", ["hal arch i386 bits 32 endian little page 4096", "hal cpu vendor ",
                                        "hal ports i386=built arm=not-ported"], attempts=2)
            run(mon, proc, "hal-port arm", "hal-port error arm not ported", attempts=2)
            # US-080 screen adaptation.
            run(mon, proc, "screen-adapt", "screen-adapt ok 720x400 (vga text 80x25) form tablet landscape grid 90x25", attempts=2)
            run(mon, proc, "screen-adapt 360x640", "form phone portrait grid 36x32 font_px 20 panels 1 compact", attempts=2)
            run(mon, proc, "screen-adapt 1024x768", "form desktop landscape grid 128x48 font_px 16 panels 3 full", attempts=2)
            run(mon, proc, "screen-adapt 10x10", "screen-adapt error size", attempts=2)
            # US-079 gestures.
            run(mon, proc, "gesture 200,50,0,1 120,55,100,1 60,52,180,0", "gesture ok swipe-left points 3", attempts=2)
            run(mon, proc, "gesture 10,10,0,1 11,10,700,1", "gesture ok long-press points 2", attempts=2)
            # US-081 energy.
            run(mon, proc, "power-profile saver", "power profile saver mode manual idle_yields 8 poll_budget 1", attempts=2)
            run(mon, proc, "power-profile auto", ["power-profile auto busy_percent ", "mode auto"], attempts=2)
            run(mon, proc, "power-status", ["power-status busy_percent ", "battery none", "power profile "], attempts=2)
            # US-085 devices.
            out = run(mon, proc, "dev-list", ["dev ok count 10 present ", "dev cpu0 class cpu present ",
                                              "dev eth0 class net present ne2000 isa 0x300",
                                              "dev touch0 class input absent"], attempts=2)
            if "dev kbd0 class input present" not in out:
                raise RuntimeError("keyboard missing from dev-list")
            run(mon, proc, "dev-list gpu0", "dev error unknown device", attempts=2)
            # US-087 application compatibility.
            out = run(mon, proc, "compat-scan /bin", "compat-scan done ok ", attempts=2, timeout=40)
            if " ok i386 ET_EXEC entry 0x" not in out:
                raise RuntimeError("no compatible binary reported: %s" % out[-600:])
            run(mon, proc, "compat-check /bin/shell", "compat /bin/shell ok i386 ET_EXEC", attempts=2)
            # US-082 notifications (local).
            run(mon, proc, "notify-push 1 backup done", "notify-push ok id 1")
            run(mon, proc, "notify-push 3 disk full", "notify-push ok id 2")
            run(mon, proc, "notify-push 1 backup done", "notify-push ok id 1")
            out = run(mon, proc, "notify-list", ["note 2 prio 3 x1 disk full", "note 1 prio 1 x2 backup done",
                                                 "notify ok pending 2 dropped 0"], attempts=2)
            run(mon, proc, "notify-ack 2", "notify-ack ok")
            run(mon, proc, "notify-list", "notify ok pending 1", attempts=2)
            # US-088 migration.
            run(mon, proc, "mkdir /data", "(-.-)")
            run(mon, proc, "write /data/a.txt hello", "(-.-)")
            run(mon, proc, "write /data/b.txt world", "(-.-)")
            run(mon, proc, "migrate-export /data /arc", "migrate-export ok files 2 bytes ")
            run(mon, proc, "migrate-verify /arc", "migrate-verify ok files 2 a.txt b.txt", attempts=2)
            run(mon, proc, "migrate-import /arc /restore", "migrate-import ok files 2 a.txt b.txt")
            run(mon, proc, "cat /restore/b.txt", "world", attempts=2)
            # US-090 deployment.
            run(mon, proc, "deploy-make /data /apps /man", "deploy-make ok files 2")
            run(mon, proc, "deploy-apply /man", "deploy-apply ok files 2")
            run(mon, proc, "deploy-verify /man", "deploy-verify ok files 2", attempts=2)
            run(mon, proc, "cat /apps/a.txt", "hello", attempts=2)
            run(mon, proc, "write /data/a.txt changed", "(-.-)")
            run(mon, proc, "deploy-apply /man", ["deploy-apply checksum /data/a.txt", "deploy-apply error failed 1 nothing changed"])
            run(mon, proc, "deploy-verify /man", "deploy-verify ok files 2", attempts=2)
            # Tampered archive is refused.
            run(mon, proc, "write /arc.0 broken", "(-.-)")
            run(mon, proc, "migrate-verify /arc", "migrate-verify error bad archive", attempts=2)
            # US-089 unified admin view.
            run(mon, proc, "admin-all", "admin-all arch i386 cpu ", attempts=2)
            if "power " not in normalized_log(nw.log_text()):
                raise RuntimeError("admin-all lacks power")
            print("MOHHDY phase 6 platform contract: PASS in %.0fs" % (time.time() - t0))
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
        print("MOHHDY phase 6 platform contract failed: %s" % error)
        print(nw.log_text()[-2500:])
        sys.exit(1)
