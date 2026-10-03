#!/usr/bin/env python3
"""GGUF slice contract: GPT-2 GGUF sampling steps (109/110) in the Ring 3 aiworker.

CI has no GPT-2 GGUF weights. The initrd gets the synthetic llm.c fixture
of tests/scripts/ai_worker_fixture.py (tokenizer + FP32 checkpoint) and the
IDE disk the synthetic GGUF of tests/scripts/ai_gguf_fixture.py (C=768,
1 layer, vocab 16; attn_qkv/ffn_down Q4_K, attn_output/output Q6_K, ffn_up
Q3_K) as FAT16 GPT2.GGU. The kernel loads its resident GGUF snapshot at boot
(Ring 0, unchanged); the boot aiworker bulk-reads its own copy
(OS_AI_ENGINE_GGUF_READ) and declares GGUF_READY.

1. Boot: worker ready (FP32 + GGUF), "[AI] ai-engine GGUF ready in Ring 3".
2. ggufclient (109 then 110 until the session ends): every step path
   worker; gguf counters fwd = done = steps, kernel 0, live 0.
3. Worker killed: the same session runs in Ring 0 (path kernel), same token
   ids.
4. A new aiworker loads its copy again and serves (same tokens).
5. Worker at low priority with a step in flight, then killed: the step is
   aborted, the caller runs it in Ring 0 (path fallback) and the session
   ends in Ring 0 with the same tokens.
6. airogue: GGUF_OPEN / GGUF_READ / GGUF_READY refused (-144) like the
   FP32 paths; then a worker-path session, same tokens.
7. The same worker still serves FP32 SYS_GPT2_GENERATE (aiclient).
gguf_kernel_while_live stays 0 throughout.
"""
import os
import re
import shutil
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_ai_worker as base  # noqa: E402

ROOT = base.ROOT
FIXTURE = os.path.join(ROOT, "build", "ai_gguf_fixture")
base.LOG = os.path.join(base.LOG_DIR, "ai-gguf.log")
base.ERR = os.path.join(base.LOG_DIR, "ai-gguf.err")
base.MON = os.path.join(base.LOG_DIR, "ai-gguf-monitor.sock")
base.FIXTURE = os.path.join(ROOT, "build", "ai_fixture")
GGUF_BYTES = 4024704
FP32_TOKENS = "0 1 2 14 3 4 |3 0 1 7 6 4"

GGUF_RE = (r"ggufclient gguf worker (-?\d+) bytes (\d+) fwd (\d+) done (\d+) kernel (\d+) "
           r"live (\d+) fallback (\d+) aborted (\d+) rogue (\d+) pending (\d+) end")
GGUF_KEYS = ("worker", "bytes", "fwd", "done", "kernel", "live", "fallback", "aborted",
             "rogue", "pending")


def run_gguf(monitor, proc, start=None):
    if start is None:
        _, start = base.spawn(monitor, proc, "ggufclient")
    st = base.child_regex(monitor, proc, GGUF_RE, start, rounds=40)
    text = base.normalized_log(base.log_text()[start:])
    steps = re.findall(r"ggufclient step (\d+) rc (-?\d+) path (\w+)", text)
    toks = re.search(r"ggufclient tokens([ |0-9]*) end", text)
    if not steps or not toks:
        raise RuntimeError("ggufclient incomplete: %r" % text[-400:])
    return {
        "paths": [p for _, _, p in steps],
        "rcs": [int(r) for _, r, _ in steps],
        "tokens": toks.group(1).strip(),
        "status": dict(zip(GGUF_KEYS, (int(v) for v in st.groups()))),
    }


def worker_gguf_ready(monitor, proc, start):
    base.worker_ready(monitor, proc, start)
    m = base.child_regex(monitor, proc, r"aiworker gguf ready bytes (\d+) at (0x[0-9a-f]+) chunks (\d+)",
                         start, rounds=20)
    if int(m.group(1)) != GGUF_BYTES or m.group(2) != "0xa1000000":
        raise RuntimeError("unexpected gguf load: %s" % m.group(0))
    return m


def contract(monitor, proc):
    base.wait_for("[AI] boot aiworker spawned", proc, 0, timeout=30)
    worker_gguf_ready(monitor, proc, 0)
    base.wait_for("[AI] ai-engine GGUF ready in Ring 3", proc, 0, timeout=30)

    # 2. Worker path.
    w1 = run_gguf(monitor, proc)
    s = w1["status"]
    n = len(w1["paths"])
    if set(w1["paths"]) != {"worker"} or s["worker"] <= 0 or s["bytes"] != GGUF_BYTES or \
            s["fwd"] != n or s["done"] != n or s["kernel"] != 0 or s["live"] != 0:
        raise RuntimeError("gguf worker run: %r" % w1)
    if "|" not in w1["tokens"] or len(w1["tokens"].split("|")[1].split()) < 2:
        raise RuntimeError("gguf generated too few tokens: %r" % w1)
    boot_pid = s["worker"]
    print("gguf worker session: %d steps, tokens [%s]" % (n, w1["tokens"]), flush=True)

    # 3. Worker gone: Ring 0.
    base.kill(monitor, proc, boot_pid)
    k1 = run_gguf(monitor, proc)
    if set(k1["paths"]) != {"kernel"} or k1["tokens"] != w1["tokens"] or k1["status"]["live"] != 0 \
            or k1["status"]["kernel"] != len(k1["paths"]) or k1["status"]["worker"] != 0:
        raise RuntimeError("gguf kernel run: %r vs %r" % (k1, w1))

    # 4. New worker.
    wpid, start = base.spawn(monitor, proc, "aiworker")
    worker_gguf_ready(monitor, proc, start)
    w2 = run_gguf(monitor, proc)
    if set(w2["paths"]) != {"worker"} or w2["tokens"] != w1["tokens"]:
        raise RuntimeError("gguf second worker: %r" % w2)

    # 5. Step in flight, worker killed: Ring 0 fallback, session finishes.
    base.send_command_until(monitor, "task-priority %s 1" % wpid, "task-priority ok %s 1" % wpid, proc)
    cpid, start = base.spawn(monitor, proc, "ggufclient")
    base.child_regex(monitor, proc, r"ggufclient start", start)
    for _ in range(8):
        if base.aistat(monitor, proc)["pending"] == 1:
            break
    else:
        raise RuntimeError("gguf step never pending")
    base.kill(monitor, proc, wpid)
    base.child_regex(monitor, proc, r"\[AI\] relay aborted: ai-engine lost or stalled", start)
    f1 = run_gguf(monitor, proc, start)
    if f1["paths"][0] != "fallback" or set(f1["paths"][1:]) - {"kernel"} or \
            f1["tokens"] != w1["tokens"] or f1["status"]["fallback"] != 1 or \
            f1["status"]["live"] != 0 or f1["status"]["aborted"] != 1:
        raise RuntimeError("gguf fallback: %r" % f1)

    # 6. Rogue against a live GGUF worker, memory around its life.
    base.send_command_until(monitor, "yield", "yield ok", proc)
    free_before = base.mem_free(monitor, proc)
    wpid, start = base.spawn(monitor, proc, "aiworker")
    worker_gguf_ready(monitor, proc, start)
    free_live = base.mem_free(monitor, proc)
    _, start = base.spawn(monitor, proc, "airogue")
    rogue = base.child_regex(monitor, proc, r"airogue register rc (-?\d+) rename rc (-?\d+) reply rc (-?\d+) "
                             r"map rc (-?\d+) fetch rc (-?\d+) gguf rc (-?\d+) (-?\d+) (-?\d+) end", start)
    vals = [int(v) for v in rogue.groups()]
    if vals[0] != -144 or vals[1] >= 0 or any(v != -144 for v in vals[2:]):
        raise RuntimeError("rogue not refused: %s" % rogue.group(0))
    w3 = run_gguf(monitor, proc)
    s = w3["status"]
    if set(w3["paths"]) != {"worker"} or w3["tokens"] != w1["tokens"] or s["rogue"] != 1 or s["live"] != 0:
        raise RuntimeError("gguf after rogue: %r" % w3)

    # 7. Same worker, FP32 path.
    fp = base.run_client(monitor, proc, "worker")
    if fp["tokens"] != FP32_TOKENS:
        raise RuntimeError("fp32 tokens changed: %r" % fp)
    base.kill(monitor, proc, wpid)
    base.send_command_until(monitor, "yield", "yield ok", proc)
    free_after = base.mem_free(monitor, proc)
    footprint = free_before - free_live
    gguf_pages = (GGUF_BYTES + 4095) // 4096
    if footprint < gguf_pages + 3071 or abs(free_after - free_before) > 8:
        raise RuntimeError("memory: before %d live %d after %d" % (free_before, free_live, free_after))
    print("ai gguf: tokens [%s] equal on worker/kernel/fallback paths; final gguf counters %r; "
          "worker footprint %d pages (GGUF copy %d pages), free before/after %d/%d" %
          (w1["tokens"], s, footprint, gguf_pages, free_before, free_after), flush=True)


def main():
    os.makedirs(base.LOG_DIR, exist_ok=True)
    for path in (base.LOG, base.ERR, base.MON):
        try:
            os.remove(path)
        except OSError:
            pass
    base.pack_fixture()
    subprocess.run([sys.executable, os.path.join(ROOT, "tests", "scripts", "ai_gguf_fixture.py"), FIXTURE],
                   check=True)
    disk = os.path.join(FIXTURE, "gguf_fat16.img")
    command = [
        "qemu-system-i386", "-kernel", base.KERNEL, "-initrd", base.INITRD,
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
                base.wait_for("(-.-)", proc, 0, timeout=90)
                monitor = base.connect_monitor()
                time.sleep(0.5)
                contract(monitor, proc)
                print("MOHHDY aiworker GGUF contract passed")
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
        print("aiworker GGUF contract failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
