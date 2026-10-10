#!/usr/bin/env python3
"""Phase 8 production contract (US-106..US-120), one i386 guest.

Metrics sampled from meminfo/ps/task metrics, threshold alert that fires,
is deduplicated and resolves with hysteresis, trend prediction, log analysis
with incident verdict, backup/restore with corruption refusal, deployment
with health check and automatic rollback, integrity drift detection,
autoscaling decisions, performance measurements, diagnostics, tutorial,
feedback, usage metrics. No network service is used.
"""
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_net_worker as nw  # noqa: E402
from test_qemu_vfs_service import normalized_log  # noqa: E402

ROOT = nw.ROOT
nw.LOG = os.path.join(nw.LOG_DIR, "production.log")
nw.ERR = os.path.join(nw.LOG_DIR, "production.err")
nw.MON = os.path.join(nw.LOG_DIR, "production-monitor.sock")
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
            # US-107 monitoring.
            run(mon, proc, "prod-sample 6 20", "prod-sample ok n 6 mem_used ", attempts=2)
            run(mon, proc, "prod-metrics", ["prod-metric mem_used last ", "prod-metric procs last ", "prod-metrics ok samples 6"], attempts=2)
            # US-114 alerts: for 2 samples, dedupe, hysteresis.
            run(mon, proc, "prod-alert-add app custom above 50 2", "prod-alert-add ok app")
            run(mon, proc, "prod-inject 70", "prod-inject ok custom 70")
            run(mon, proc, "prod-inject 80", ["prod-alert FIRING app v=80", "prod-inject ok custom 80"])
            run(mon, proc, "prod-inject 90", "prod-inject ok custom 90")
            run(mon, proc, "prod-inject 10", "prod-inject ok custom 10")
            run(mon, proc, "prod-inject 10", ["prod-alert RESOLVED app v=10", "prod-inject ok custom 10"])
            run(mon, proc, "prod-alerts", ["prod-rule app custom > 50 for 2 state ok", "prod-alerts ok fired 1 resolved 1 deduped 1"], attempts=2)
            # US-115 predictive maintenance on a rising metric.
            for value in (20, 30, 40, 50):
                run(mon, proc, "prod-inject %d" % value, "prod-inject ok custom %d" % value)
            run(mon, proc, "prod-predict custom 1000", ["prod-predict custom slope_milli_per_tick ", " reaches 1000 in_ticks "], attempts=2)
            run(mon, proc, "prod-predict nothing 5", "prod-predict error usage", attempts=2)
            # US-113 log analysis.
            run(mon, proc, "mkdir /var", "(-.-)")
            for text in ("info boot ok", "error timeout on req 17", "error timeout on req 18", "warn disk slow", "error timeout on req 99"):
                run(mon, proc, "prod-log-append /var/app.log " + text, "prod-log-append ok bytes ")
            run(mon, proc, "prod-log-analyze /var/app.log", ["prod-log lines 5 errors 3 warns 1 infos 1 other 0",
                                                             "prod-log top 3 x error timeout on req #",
                                                             "prod-log verdict incident"], attempts=2)
            # US-111 backup and restore, corruption refused.
            run(mon, proc, "mkdir /cfg", "(-.-)")
            run(mon, proc, "write /cfg/a.conf mode=eco", "(-.-)")
            run(mon, proc, "write /cfg/b.conf level=2", "(-.-)")
            run(mon, proc, "prod-backup daily /cfg", "prod-backup ok daily files 2 bytes ")
            run(mon, proc, "write /cfg/a.conf broken", "(-.-)")
            run(mon, proc, "prod-restore daily", "prod-restore ok files 2")
            run(mon, proc, "cat /cfg/a.conf", "mode=eco", attempts=2)
            run(mon, proc, "prod-backup-verify daily", "prod-backup-verify ok files 2", attempts=2)
            run(mon, proc, "prod-backup-corrupt daily", "prod-backup-corrupt ok")
            run(mon, proc, "prod-backup-verify daily", "prod-backup-verify error corrupt /cfg/", attempts=2)
            run(mon, proc, "prod-restore daily", "prod-restore error backup corrupt, nothing written")
            run(mon, proc, "prod-backups", ["prod-backup daily dir /cfg files 2 tick ", " corrupt", "prod-backups ok 1"], attempts=2)
            # US-108/109 deployment with health check and automatic rollback; US-112 integrity.
            run(mon, proc, "mkdir /app", "(-.-)")
            run(mon, proc, "write /app/main v1", "(-.-)")
            run(mon, proc, "write /app/health ok", "(-.-)")
            run(mon, proc, "prod-integrity baseline /app", "prod-integrity baseline files 2")
            run(mon, proc, "mkdir /stg", "(-.-)")
            run(mon, proc, "write /stg/main v2", "(-.-)")
            run(mon, proc, "write /stg/health ok", "(-.-)")
            run(mon, proc, "prod-manifest /stg /app", "prod-manifest ok files 2")
            run(mon, proc, "prod-deploy /stg /app", ["prod-deploy backup pre-deploy files 2", "prod-deploy ok healthy files 2"])
            run(mon, proc, "cat /app/main", "v2", attempts=2)
            run(mon, proc, "prod-integrity check /app", ["prod-integrity changed /app/main", "prod-integrity check same 1 changed 1 new 0 verdict drift"])
            run(mon, proc, "write /stg/main v3", "(-.-)")
            run(mon, proc, "write /stg/health fail", "(-.-)")
            run(mon, proc, "prod-manifest /stg /app", "prod-manifest ok files 2")
            run(mon, proc, "prod-deploy /stg /app", ["prod-deploy health failed: health check not ok", "prod-deploy rolled back files 2"])
            run(mon, proc, "cat /app/main", "v2", attempts=2)
            run(mon, proc, "cat /app/health", "ok", attempts=2)
            run(mon, proc, "prod-rollback", "prod-rollback ok files 2")
            # US-110 autoscaling decisions.
            run(mon, proc, "prod-scale-sim 2 10 10 10 0 0 0 0", ["prod-scale workers 1 2 2 3 3 2 2 1", "prod-scale-sim ok ups 2 downs 2"], attempts=2)
            # US-106 performance measurements.
            run(mon, proc, "prod-bench", ["prod-bench alu ops_per_s ", "prod-bench memcpy kib_per_s ", "prod-bench syscall calls_per_s ",
                                          "prod-bench file512 rw_per_s ", "prod-bench ok checksum "], timeout=60)
            # US-116..US-120 support, training, feedback, usage, roadmap.
            run(mon, proc, "prod-diag", ["prod-diag uptime_ticks ", "prod-diag net_status ", "prod-diag ok"], attempts=2)
            run(mon, proc, "prod-tutorial 3", "prod-tutorial lesson 3 logs", attempts=2)
            run(mon, proc, "prod-feedback 5 great tool", "prod-feedback ok total 1")
            run(mon, proc, "prod-feedback 3 slow boot", "prod-feedback ok total 2")
            run(mon, proc, "prod-feedback summary", ["prod-feedback total 2 avg 4.0 r1=0 r2=0 r3=1 r4=0 r5=1"], attempts=2)
            run(mon, proc, "prod-usage", ["prod-usage prod-inject ", "prod-usage ok commands "], attempts=2)
            run(mon, proc, "prod-roadmap", "prod-roadmap ok phases 8", attempts=2)
            out = normalized_log(nw.log_text())
            if "prod-alerts ok fired 1 resolved 1 deduped 1" not in out:
                raise RuntimeError("alert counters missing")
            print("MOHHDY phase 8 production contract: PASS in %.0fs" % (time.time() - t0))
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
        print("MOHHDY phase 8 production contract failed: %s" % error)
        print(nw.log_text()[-2500:])
        sys.exit(1)
