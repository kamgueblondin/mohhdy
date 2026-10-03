#!/usr/bin/env python3
"""Inventory item 4 contract: GPT-2 FP32 inference in the Ring 3 aiworker.

CI has no GPT-2 weights, so the initrd is packed with the synthetic llm.c
checkpoint of tests/scripts/ai_worker_fixture.py (T=32 V=16 L=2 heads=2
C=32, seeded non-zero weights). The same loader / BPE / forward pass /
sampler code runs on it as on GPT-2 124M.

1. Boot: the kernel spawns aiworker, which registers "ai-engine", maps the
   initrd checkpoint and tokenizer read-only and refuses a stale reply.
2. aiclient (SYS_GPT2_GENERATE) is served by the worker: path worker,
   counters fwd 1 done 1 kernel 0 live 0.
3. Boot worker killed: the same call runs in Ring 0 (path kernel) and
   produces the same token ids and text.
4. A new aiworker serves again (same tokens).
5. Worker at low priority with a job in flight (pending 1), then killed:
   the IRQ0 watchdog aborts the job and the caller gets the Ring 0
   fallback, same tokens (aborted 1 fallback 1).
6. airogue: register ai-engine, rename to aiworker, forged reply, map and
   fetch are all refused; the forged reply is counted (rogue) and a
   following generation still returns the true tokens.
kernel_infer_while_live stays 0 throughout.
"""
import os
import re
import shutil
import socket
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_qemu_vfs_service import normalized_log  # noqa: E402

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "ai-worker.log")
ERR = os.path.join(LOG_DIR, "ai-worker.err")
MON = os.path.join(LOG_DIR, "ai-worker-monitor.sock")
KERNEL = os.path.join(ROOT, "build", "mohhdy.bin")
INITRD = os.path.join(ROOT, "my_initrd.tar")
FIXTURE = os.path.join(ROOT, "build", "ai_fixture")
KEY_HOLD_MS = int(os.environ.get("KEY_HOLD_MS", "10"))

OS_AI_ENGINE_REQUIRED = -144
OS_AI_ENGINE_STALE = -146


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(needle, proc, offset=0, timeout=25):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped early")
        if needle in normalized_log(log_text()[offset:]):
            return
        time.sleep(0.1)
    raise RuntimeError("missing output: %s" % needle)


def wait_regex(pattern, proc, offset=0, timeout=25):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped early")
        match = re.search(pattern, normalized_log(log_text()[offset:]))
        if match:
            return match
        time.sleep(0.1)
    raise RuntimeError("missing output: %s" % pattern)


def connect_monitor():
    deadline = time.time() + 5
    while time.time() < deadline:
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
    raise RuntimeError("QEMU monitor unavailable")


def send_command(client, command):
    special = {" ": "spc", "-": "minus"}
    for char in command:
        client.sendall(("sendkey %s %d\n" %
                        (special.get(char, char.lower()), KEY_HOLD_MS)).encode("ascii"))
        time.sleep(0.35)
    client.sendall(("sendkey ret %d\n" % KEY_HOLD_MS).encode("ascii"))


def send_command_until(client, command, marker, proc, attempts=3):
    failure = None
    for _ in range(attempts):
        start = len(log_text())
        send_command(client, command)
        try:
            wait_for(marker, proc, start)
            return start
        except RuntimeError as error:
            failure = error
            time.sleep(0.4)
    raise failure


def child_regex(client, proc, pattern, start, rounds=8):
    """Give a spawned child bounded cooperative turns until pattern shows."""
    for _ in range(rounds):
        try:
            return wait_regex(pattern, proc, start, timeout=1)
        except RuntimeError:
            pass
        send_command_until(client, "yield", "yield ok", proc)
    return wait_regex(pattern, proc, start, timeout=3)


def spawn(client, proc, name):
    start = send_command_until(client, "spawn %s" % name, "spawn ok pid", proc)
    match = re.search(r"spawn ok pid[\s\S]*?(\d+) %s" % name,
                      normalized_log(log_text()[start:]))
    if not match:
        raise RuntimeError("%s not spawned" % name)
    return match.group(1), start


def kill(client, proc, pid):
    send_command_until(client, "kill %s" % pid, "Processus %s termine" % pid, proc)


STATUS_RE = (r"(\w+) status worker (-?\d+) fwd (\d+) done (\d+) aborted (\d+) "
             r"fallback (\d+) kernel (\d+) live (\d+) rogue (\d+) stale (\d+) "
             r"pending (\d+) mapped (\d+) end")
STATUS_KEYS = ("worker", "fwd", "done", "aborted", "fallback", "kernel", "live",
               "rogue", "stale", "pending", "mapped")


def parse_status(match):
    return dict(zip(STATUS_KEYS, (int(v) for v in match.groups()[1:])))


def run_client(client, proc, expect_path):
    pid, start = spawn(client, proc, "aiclient")
    gen = child_regex(client, proc, r"aiclient rc (-?\d+) path (\w+) text \[([^\]\n]*)\]", start)
    toks = child_regex(client, proc, r"aiclient tokens([ |0-9]*) end", start)
    st = child_regex(client, proc, r"(aiclient) status worker" + STATUS_RE[len(r"(\w+) status worker"):], start)
    result = {
        "rc": int(gen.group(1)), "path": gen.group(2), "text": gen.group(3),
        "tokens": toks.group(1).strip(), "status": parse_status(st),
    }
    if result["path"] != expect_path:
        raise RuntimeError("expected path %s, got %r" % (expect_path, result))
    if result["rc"] < 0 or result["rc"] != len(result["text"]):
        raise RuntimeError("bad generation: %r" % result)
    return result


def aistat(client, proc):
    _, start = spawn(client, proc, "aistat")
    return parse_status(child_regex(client, proc, r"(aistat) status worker" +
                                    STATUS_RE[len(r"(\w+) status worker"):], start))


def mem_free(client, proc):
    start = send_command_until(client, "mem", "mem ok", proc)
    match = wait_regex(r"mem ok (\d+) (\d+) (\d+)", proc, start)
    return int(match.group(3))


def worker_ready(client, proc, start):
    match = child_regex(client, proc, r"aiworker ready ai-engine checkpoint (\d+) at (0x[0-9a-f]+) "
                        r"tokenizer (\d+) layers (\d+) channels (\d+)", start)
    child_regex(client, proc, r"aiworker stale reply refused rc %d" % OS_AI_ENGINE_STALE, start)
    return match


def pack_fixture():
    subprocess.run([sys.executable, os.path.join(ROOT, "tests", "scripts", "ai_worker_fixture.py"),
                    FIXTURE], check=True)
    subprocess.run(["make", "-s", "pack-initrd", "MODEL_DIR=%s" % FIXTURE], cwd=ROOT, check=True,
                   stdout=subprocess.DEVNULL)


def restore_initrd():
    subprocess.run(["make", "-s", "pack-initrd"], cwd=ROOT, check=False, stdout=subprocess.DEVNULL)
    shutil.rmtree(FIXTURE, ignore_errors=True)


def contract(monitor, proc):
    wait_for("[AI] boot aiworker spawned", proc, 0, timeout=20)
    boot = worker_ready(monitor, proc, 0)
    if int(boot.group(1)) != 109056 or boot.group(2)[:6] != "0x8000":
        raise RuntimeError("unexpected mapping: %s" % boot.group(0))

    # 2. Worker path.
    w1 = run_client(monitor, proc, "worker")
    s = w1["status"]
    if (s["worker"] <= 0 or s["fwd"] != 1 or s["done"] != 1 or s["kernel"] != 0 or
            s["live"] != 0 or s["pending"] != 0 or s["stale"] != 1 or s["mapped"] != 109056):
        raise RuntimeError("worker counters: %r" % s)
    boot_pid = s["worker"]
    if not w1["tokens"] or "|" not in w1["tokens"]:
        raise RuntimeError("no generated tokens: %r" % w1)

    # 3. Worker gone: Ring 0 path, same tokens.
    kill(monitor, proc, boot_pid)
    k1 = run_client(monitor, proc, "kernel")
    s = k1["status"]
    if s["worker"] != 0 or s["kernel"] != 1 or s["live"] != 0 or s["fwd"] != 1:
        raise RuntimeError("kernel counters: %r" % s)
    if (k1["tokens"], k1["text"]) != (w1["tokens"], w1["text"]):
        raise RuntimeError("token mismatch worker %r kernel %r" % (w1, k1))

    # 4. New worker (child of the shell) serves again.
    wpid, start = spawn(monitor, proc, "aiworker")
    worker_ready(monitor, proc, start)
    w2 = run_client(monitor, proc, "worker")
    if (w2["tokens"], w2["text"]) != (w1["tokens"], w1["text"]):
        raise RuntimeError("token mismatch second worker %r" % w2)
    if w2["status"]["fwd"] != 2 or w2["status"]["done"] != 2 or w2["status"]["kernel"] != 1:
        raise RuntimeError("second worker counters: %r" % w2["status"])

    # 5. Job in flight, worker killed before it runs: Ring 0 fallback.
    # Priority 1 keeps the worker behind the shell and the clients (strict
    # priority scheduling), so the relayed job stays pending until the kill.
    # (task-suspend only accepts READY children; the worker sleeps in
    # SYS_IPC_RECV_WAIT.)
    send_command_until(monitor, "task-priority %s 1" % wpid, "task-priority ok %s 1" % wpid, proc)
    cpid, start = spawn(monitor, proc, "aiclient")
    child_regex(monitor, proc, r"aiclient start", start)
    for _ in range(8):
        if aistat(monitor, proc)["pending"] == 1:
            break
    else:
        raise RuntimeError("relayed job never pending")
    kill(monitor, proc, wpid)
    child_regex(monitor, proc, r"\[AI\] relay aborted: ai-engine lost or stalled", start)
    gen = child_regex(monitor, proc, r"aiclient rc (-?\d+) path (\w+) text \[([^\]\n]*)\]", start)
    toks = child_regex(monitor, proc, r"aiclient tokens([ |0-9]*) end", start)
    if gen.group(2) != "fallback" or (toks.group(1).strip(), gen.group(3)) != (w1["tokens"], w1["text"]):
        raise RuntimeError("fallback mismatch: %s / %s" % (gen.group(0), toks.group(0)))
    s = aistat(monitor, proc)
    if s["aborted"] != 1 or s["fallback"] != 1 or s["kernel"] != 2 or s["live"] != 0 or s["pending"] != 0:
        raise RuntimeError("fallback counters: %r" % s)

    # 6. Rogue task against a live worker. Free frames are sampled around
    # this worker's whole life: its .bss/ELF/stack/page tables come back on
    # exit while the borrowed initrd frames (PAGE_BORROWED) are never freed.
    send_command_until(monitor, "yield", "yield ok", proc)  # reap exited clients
    free_before = mem_free(monitor, proc)
    wpid, start = spawn(monitor, proc, "aiworker")
    worker_ready(monitor, proc, start)
    free_live = mem_free(monitor, proc)
    _, start = spawn(monitor, proc, "airogue")
    rogue = child_regex(monitor, proc, r"airogue register rc (-?\d+) rename rc (-?\d+) reply rc (-?\d+) "
                        r"map rc (-?\d+) fetch rc (-?\d+) gguf rc (-?\d+) (-?\d+) (-?\d+) end", start)
    reg, ren, rep, mp, fe, go, gr, gy = (int(v) for v in rogue.groups())
    if reg != OS_AI_ENGINE_REQUIRED or ren >= 0 or rep != OS_AI_ENGINE_REQUIRED or \
            mp != OS_AI_ENGINE_REQUIRED or fe != OS_AI_ENGINE_REQUIRED or \
            (go, gr, gy) != (OS_AI_ENGINE_REQUIRED,) * 3:
        raise RuntimeError("rogue not refused: %s" % rogue.group(0))
    w3 = run_client(monitor, proc, "worker")
    s = w3["status"]
    if (w3["tokens"], w3["text"]) != (w1["tokens"], w1["text"]):
        raise RuntimeError("token mismatch after rogue %r" % w3)
    if s["rogue"] != 1 or s["stale"] != 3 or s["fwd"] != 4 or s["done"] != 3 or s["live"] != 0 \
            or s["kernel"] != 2:
        raise RuntimeError("final counters: %r" % s)
    kill(monitor, proc, wpid)
    send_command_until(monitor, "yield", "yield ok", proc)
    free_after = mem_free(monitor, proc)
    footprint = free_before - free_live
    # 12.0 MiB of .bss alone (FP32 + GGUF runtime buffers) is 3071 pages.
    if footprint < 3071 or abs(free_after - free_before) > 8:
        raise RuntimeError("memory: before %d live %d after %d" % (free_before, free_live, free_after))
    print("ai worker: tokens [%s] text [%s]; worker/kernel/fallback paths equal; "
          "final counters %r; worker footprint %d pages, free before/after %d/%d" %
          (w1["tokens"], w1["text"], s, footprint, free_before, free_after))


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, ERR, MON):
        try:
            os.remove(path)
        except OSError:
            pass
    pack_fixture()
    command = [
        "qemu-system-i386", "-kernel", KERNEL, "-initrd", INITRD,
        "-m", "1024M", "-display", "none", "-vga", "none",
        "-serial", "file:" + LOG,
        "-monitor", "unix:%s,server,nowait" % MON,
        "-machine", "type=pc,accel=tcg",
        "-no-reboot", "-no-shutdown",
    ]
    try:
        with open(ERR, "wb") as err_handle:
            proc = subprocess.Popen(command, stdout=err_handle, stderr=err_handle)
            monitor = None
            try:
                wait_for("(-.-)", proc, 0, timeout=40)
                monitor = connect_monitor()
                time.sleep(0.5)
                contract(monitor, proc)
                print("MOHHDY inventory item 4 aiworker contract passed")
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
        restore_initrd()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("aiworker contract failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
