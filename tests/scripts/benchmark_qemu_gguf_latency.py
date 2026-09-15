#!/usr/bin/env python3
"""Quantifie la latence observée du runtime GGUF local MOHHDY sous QEMU TCG.

Le benchmark mesure uniquement les deux demandes de génération Ring 3 après le
boot et la sélection du modèle : premier token ``ai bonjour`` puis réutilisation
coopérative ``ai-continue``. Les résultats sont écrits dans un JSON portable;
ils ne constituent pas une promesse de performance sur matériel physique.
"""
from __future__ import print_function

import json
import os
import socket
import statistics
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KERNEL = os.environ.get("KERNEL", os.path.join(ROOT, "build", "mohhdy.bin"))
INITRD = os.environ.get("INITRD", os.path.join(ROOT, "my_initrd.tar"))
DISK = os.environ.get("OVERLAY_DISK", os.path.join(ROOT, "build", "gpt2_gguf_fat16.img"))
LOG_DIR = os.path.join(ROOT, "test_logs")
RUNS = int(os.environ.get("GGUF_BENCH_RUNS", "3"))
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "90"))
GENERATION_TIMEOUT = float(os.environ.get("GGUF_GENERATION_TIMEOUT", "600"))
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.65"))
MAX_SPREAD_RATIO = float(os.environ.get("GGUF_BENCH_MAX_SPREAD_RATIO", "0"))
REPORT = os.environ.get("GGUF_BENCH_REPORT", os.path.join(LOG_DIR, "gguf-qemu-latency.json"))


def read_text(path):
    try:
        with open(path, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(proc, log_path, needle, timeout, start=0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped while waiting for %r: %s" %
                               (needle, read_text(log_path)[-2000:]))
        if needle in read_text(log_path)[start:]:
            time.sleep(0.4)
            return
        time.sleep(0.15)
    raise RuntimeError("timeout for %r: %s" % (needle, read_text(log_path)[-2000:]))


def connect_monitor(path):
    deadline = time.monotonic() + 10.0
    while time.monotonic() < deadline:
        if os.path.exists(path):
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                client.connect(path)
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


def send(client, command):
    aliases = {" ": "spc", "-": "minus", ".": "dot"}
    for char in command:
        client.sendall(("sendkey %s\n" % aliases.get(char, char.lower())).encode("ascii"))
        time.sleep(KEY_DELAY)
    time.sleep(KEY_DELAY)
    client.sendall(b"sendkey ret\n")


def summarize(values):
    if not values:
        raise ValueError("empty latency sample")
    median = statistics.median(values)
    return {
        "min_seconds": min(values),
        "median_seconds": median,
        "max_seconds": max(values),
        "spread_seconds": max(values) - min(values),
        "spread_ratio": 0.0 if median == 0.0 else (max(values) - min(values)) / median,
    }


def run_once(index):
    log = os.path.join(LOG_DIR, "gguf-qemu-latency-%02d.log" % index)
    err_path = os.path.join(LOG_DIR, "gguf-qemu-latency-%02d.err" % index)
    monitor_path = os.path.join(LOG_DIR, "gguf-qemu-latency-%02d.sock" % index)
    for path in (log, err_path, monitor_path):
        try:
            os.remove(path)
        except OSError:
            pass
    proc = None
    client = None
    try:
        with open(err_path, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", KERNEL,
                "-initrd", INITRD, "-m", "1024M", "-display", "none", "-vga", "none",
                "-serial", "file:" + log, "-monitor", "unix:%s,server,nowait" % monitor_path,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
                "-drive", "file=%s,format=raw,if=ide,cache=writethrough" % DISK,
            ], cwd=ROOT, stdout=err, stderr=err)
            wait_for(proc, log, "GGUF: profil local FAT16 pret", BOOT_TIMEOUT)
            wait_for(proc, log, "SYS_GETS: Debut", BOOT_TIMEOUT)
            client = connect_monitor(monitor_path)
            start = len(read_text(log))
            send(client, "ai-model use gpt2.gguf")
            wait_for(proc, log, "Profil GPT-2 GGUF selectionne", BOOT_TIMEOUT, start)
            start = len(read_text(log))
            first_started = time.monotonic()
            send(client, "ai bonjour")
            wait_for(proc, log, "[GPT-2 GGUF local]", GENERATION_TIMEOUT, start)
            segment = read_text(log)[start:]
            if "[GPT-2 GGUF local] indisponible" in segment:
                raise RuntimeError("GGUF local rejected generation: %s" % segment[-1000:])
            first_elapsed = time.monotonic() - first_started
            start = len(read_text(log))
            continued_started = time.monotonic()
            send(client, "ai-continue")
            wait_for(proc, log, "[GPT-2 GGUF local suite]", GENERATION_TIMEOUT, start)
            segment = read_text(log)[start:]
            if "session indisponible" in segment:
                raise RuntimeError("GGUF continuation rejected: %s" % segment[-1000:])
            continued_elapsed = time.monotonic() - continued_started
            return {
                "run": index,
                "first_token_seconds": first_elapsed,
                "continuation_seconds": continued_elapsed,
                "log": os.path.relpath(log, ROOT),
            }
    finally:
        if client is not None:
            client.close()
        if proc is not None and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=4)
            except subprocess.TimeoutExpired:
                proc.kill()
        try:
            os.remove(monitor_path)
        except OSError:
            pass


def main():
    if RUNS < 1 or RUNS > 9:
        raise RuntimeError("GGUF_BENCH_RUNS must be between 1 and 9")
    if not all(os.path.isfile(path) for path in (KERNEL, INITRD, DISK)):
        raise RuntimeError("missing kernel, initrd or GGUF FAT16 disk")
    os.makedirs(LOG_DIR, exist_ok=True)
    samples = []
    for index in range(1, RUNS + 1):
        sample = run_once(index)
        samples.append(sample)
        print("GGUF_RUN=%d FIRST_TOKEN_SECONDS=%.3f CONTINUATION_SECONDS=%.3f" %
              (index, sample["first_token_seconds"], sample["continuation_seconds"]))
    first_summary = summarize([sample["first_token_seconds"] for sample in samples])
    continuation_summary = summarize([sample["continuation_seconds"] for sample in samples])
    report = {
        "schema_version": 1,
        "runtime": "MOHHDY GGUF local FAT16 under QEMU TCG",
        "sample_count": RUNS,
        "first_token": first_summary,
        "continuation": continuation_summary,
        "samples": samples,
    }
    with open(REPORT, "w") as handle:
        json.dump(report, handle, indent=2, sort_keys=True)
        handle.write("\n")
    print("GGUF_FIRST_TOKEN_MEDIAN_SECONDS=%.3f" % first_summary["median_seconds"])
    print("GGUF_CONTINUATION_MEDIAN_SECONDS=%.3f" % continuation_summary["median_seconds"])
    print("GGUF_REPORT=%s" % os.path.relpath(REPORT, ROOT))
    if MAX_SPREAD_RATIO > 0.0:
        observed = max(first_summary["spread_ratio"], continuation_summary["spread_ratio"])
        if observed > MAX_SPREAD_RATIO:
            print("GGUF_VARIANCE_ALERT=1 observed=%.3f threshold=%.3f" %
                  (observed, MAX_SPREAD_RATIO))
        else:
            print("GGUF_VARIANCE_ALERT=0 observed=%.3f threshold=%.3f" %
                  (observed, MAX_SPREAD_RATIO))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("GGUF QEMU benchmark failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
