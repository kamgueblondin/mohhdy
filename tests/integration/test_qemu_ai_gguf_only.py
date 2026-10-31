#!/usr/bin/env python3
"""GGUF-only contract: no FP32 checkpoint in the initrd, GGUF on the disk.

Strict boot defers the FAT mount to the Ring 3 atadriver, so when the
kernel decides whether to spawn the boot aiworker no GGUF is resident yet.
The worker must still be spawned (tokenizer present, boot atadriver live)
and must load GPT2.GGU itself through that driver. Before this contract the
spawn required a FP32 checkpoint, and a GGUF-only initrd answered -148
(OS_AI_GGUF_NO_WORKER) to every 109/110 step.

1. Initrd: synthetic tokenizer only (gpt2_124M.bin removed from the
   fixture). Boot log has no FP32 checkpoint and "[AI] boot aiworker spawned".
2. The worker reports "aiworker gguf ready bytes 4024704" and
   "[AI] ai-engine GGUF ready in Ring 3".
3. ggufclient: every 109/110 step runs on the worker path (rc never -148),
   at least two generated tokens, kernel 0 and live 0, one worker session.
"""
import os
import shutil
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_ai_worker as base  # noqa: E402
import test_qemu_ai_gguf as gguf  # noqa: E402

ROOT = base.ROOT
FIXTURE = os.path.join(ROOT, "build", "ai_gguf_only_fixture")
base.LOG = os.path.join(base.LOG_DIR, "ai-gguf-only.log")
base.ERR = os.path.join(base.LOG_DIR, "ai-gguf-only.err")
base.MON = os.path.join(base.LOG_DIR, "ai-gguf-only-monitor.sock")
base.FIXTURE = os.path.join(ROOT, "build", "ai_fixture_gguf_only")


def pack_tokenizer_only():
    subprocess.run([sys.executable, os.path.join(ROOT, "tests", "scripts", "ai_worker_fixture.py"),
                    base.FIXTURE], check=True)
    os.remove(os.path.join(base.FIXTURE, "gpt2_124M.bin"))
    subprocess.run(["make", "-s", "pack-initrd", "MODEL_DIR=%s" % base.FIXTURE], cwd=ROOT,
                   check=True, stdout=subprocess.DEVNULL)


def contract(monitor, proc):
    base.wait_for("[AI] boot aiworker spawned", proc, 0, timeout=30)
    log = base.normalized_log(base.log_text())
    if "GPT-2: checkpoint absent de l'initrd" not in log:
        raise RuntimeError("initrd still carries a FP32 checkpoint")
    m = base.child_regex(monitor, proc, r"aiworker gguf ready bytes (\d+) at (0x[0-9a-f]+) chunks (\d+)",
                         0, rounds=60)
    if int(m.group(1)) != gguf.GGUF_BYTES or m.group(2) != "0xa1000000":
        raise RuntimeError("unexpected gguf load: %s" % m.group(0))
    base.wait_for("[AI] ai-engine GGUF ready in Ring 3", proc, 0, timeout=30)
    if "aiworker model unavailable" in base.normalized_log(base.log_text()):
        raise RuntimeError("aiworker reported no usable model")
    run = gguf.run_gguf(monitor, proc)
    s = run["status"]
    n = len(run["paths"])
    if set(run["paths"]) != {"worker"} or gguf.OS_AI_GGUF_NO_WORKER in run["rcs"]:
        raise RuntimeError("gguf-only steps not on the worker: %r" % run)
    if s["worker"] <= 0 or s["bytes"] != gguf.GGUF_BYTES or s["fwd"] != n or s["done"] != n or \
            s["kernel"] != 0 or s["live"] != 0 or s["sess_ring0"] != 0:
        raise RuntimeError("gguf-only counters: %r" % s)
    if "|" not in run["tokens"] or len(run["tokens"].split("|")[1].split()) < 2:
        raise RuntimeError("gguf-only generated too few tokens: %r" % run)
    gguf.check_worker_session(run)
    print("gguf-only worker path: %d steps tokens %s" % (n, run["tokens"]), flush=True)


def main():
    os.makedirs(base.LOG_DIR, exist_ok=True)
    for path in (base.LOG, base.ERR, base.MON):
        try:
            os.remove(path)
        except OSError:
            pass
    pack_tokenizer_only()
    subprocess.run([sys.executable, os.path.join(ROOT, "tests", "scripts", "ai_gguf_fixture.py"), FIXTURE],
                   check=True)
    disk = os.path.join(FIXTURE, "gguf_fat16.img")
    command = [
        "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4", "-kernel", base.KERNEL, "-initrd", base.INITRD,
        "-m", "1024M", "-display", "none", "-vga", "none",
        "-drive", "file=%s,format=raw,if=ide,cache=writethrough" % disk,
        "-serial", "file:" + base.LOG,
        "-monitor", "unix:%s,server,nowait" % base.MON,
        "-machine", "type=pc,accel=tcg",
        "-no-reboot", "-no-shutdown",
    ]
    try:
        with open(base.ERR, "wb") as err_handle:
            proc = subprocess.Popen(command, stdout=err_handle, stderr=err_handle)
            monitor = None
            try:
                base.wait_for("[0m : ", proc, 0, timeout=180)
                monitor = base.connect_monitor()
                time.sleep(0.5)
                contract(monitor, proc)
                print("MOHHDY aiworker GGUF-only contract passed")
                return 0
            finally:
                if monitor is not None:
                    monitor.close()
                if proc.poll() is None:
                    proc.terminate()
                    try:
                        proc.wait(timeout=2)
                    except subprocess.TimeoutExpired:
                        proc.kill()
    finally:
        base.restore_initrd()
        shutil.rmtree(FIXTURE, ignore_errors=True)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("aiworker GGUF-only contract failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
