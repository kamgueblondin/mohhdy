#!/usr/bin/env python3
"""Quantifie la latence GGUF locale MOHHDY sous QEMU KVM (Multiboot guest).

Meme contrat de mesure que le benchmark TCG : horloge monotone hote avant
chaque commande Ring 3 ``ai bonjour`` / ``ai-continue``, exclusion du boot et
de la selection de modele, rapport JSON min/mediane/max/dispersion.

Sans ``/dev/kvm`` accessible, le script se termine en skip (code 0) sauf si
``GGUF_KVM_REQUIRE=1``. Les poids GGUF hors depot absents produisent aussi un
skip documente. Ce n'est pas une promesse "moins d'une seconde".
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
REPORT = os.environ.get(
    "GGUF_KVM_BENCH_REPORT",
    os.path.join(LOG_DIR, "gguf-qemu-kvm-latency.json"),
)
KVM_DEVICE = os.environ.get("GGUF_KVM_DEVICE", "/dev/kvm")
REQUIRE_KVM = os.environ.get("GGUF_KVM_REQUIRE", "0") == "1"
# Observation thresholds (docs): do not claim <1s without a KVM campaign.
TCG_REF_FIRST_MEDIAN_S = 48.739
TCG_REF_CONTINUATION_MEDIAN_S = 22.781
SUB_SECOND_CLAIM_SECONDS = 1.0


def read_text(path):
    try:
        with open(path, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def kvm_usable(device=None):
    """Return (ok, reason). ok means the host can open the KVM device RW."""
    path = device if device is not None else KVM_DEVICE
    if not os.path.exists(path):
        return False, "kvm_device_missing:%s" % path
    if not os.access(path, os.R_OK | os.W_OK):
        return False, "kvm_device_permission_denied:%s" % path
    try:
        fd = os.open(path, os.O_RDWR)
    except OSError as error:
        return False, "kvm_device_open_failed:%s" % error
    os.close(fd)
    return True, "kvm_ok"


def skip(reason):
    print("GGUF_KVM_SKIP=1 reason=%s" % reason)
    print(
        "GGUF_KVM_NOTE=documented skip; run on a host with usable /dev/kvm "
        "and models/gpt2-Q3_K_M.gguf (see docs/aos_gguf_kvm_latency_harness.md)"
    )
    return 0


def fail_or_skip(reason):
    if REQUIRE_KVM:
        print("GGUF KVM benchmark required but unavailable: %s" % reason, file=sys.stderr)
        return 1
    return skip(reason)


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


def qemu_kvm_argv(log, monitor_path):
    """Multiboot guest (-kernel/-initrd) under QEMU KVM only."""
    return [
        "qemu-system-i386", "-cpu", "pentium3", "-kernel", KERNEL,
        "-initrd", INITRD, "-m", "1024M", "-display", "none", "-vga", "none",
        "-serial", "file:" + log, "-monitor", "unix:%s,server,nowait" % monitor_path,
        "-machine", "type=pc,accel=kvm", "-no-reboot", "-no-shutdown",
        "-drive", "file=%s,format=raw,if=ide,cache=writethrough" % DISK,
    ]


def run_once(index):
    log = os.path.join(LOG_DIR, "gguf-qemu-kvm-latency-%02d.log" % index)
    err_path = os.path.join(LOG_DIR, "gguf-qemu-kvm-latency-%02d.err" % index)
    monitor_path = os.path.join(LOG_DIR, "gguf-qemu-kvm-latency-%02d.sock" % index)
    for path in (log, err_path, monitor_path):
        try:
            os.remove(path)
        except OSError:
            pass
    proc = None
    client = None
    try:
        with open(err_path, "wb") as err:
            proc = subprocess.Popen(
                qemu_kvm_argv(log, monitor_path),
                cwd=ROOT, stdout=err, stderr=err,
            )
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

    ok, reason = kvm_usable()
    if not ok:
        return fail_or_skip(reason)

    missing = [path for path in (KERNEL, INITRD, DISK) if not os.path.isfile(path)]
    if missing:
        return fail_or_skip("missing_artifacts:%s" % ",".join(
            os.path.relpath(path, ROOT) for path in missing
        ))

    os.makedirs(LOG_DIR, exist_ok=True)
    samples = []
    for index in range(1, RUNS + 1):
        sample = run_once(index)
        samples.append(sample)
        print("GGUF_KVM_RUN=%d FIRST_TOKEN_SECONDS=%.3f CONTINUATION_SECONDS=%.3f" %
              (index, sample["first_token_seconds"], sample["continuation_seconds"]))
    first_summary = summarize([sample["first_token_seconds"] for sample in samples])
    continuation_summary = summarize([sample["continuation_seconds"] for sample in samples])
    first_median = first_summary["median_seconds"]
    cont_median = continuation_summary["median_seconds"]
    sub_second_claim_ok = (
        first_median < SUB_SECOND_CLAIM_SECONDS
        and cont_median < SUB_SECOND_CLAIM_SECONDS
    )
    report = {
        "schema_version": 1,
        "runtime": "MOHHDY GGUF local FAT16 under QEMU KVM Multiboot",
        "accelerator": "kvm",
        "machine": "type=pc,accel=kvm",
        "sample_count": RUNS,
        "first_token": first_summary,
        "continuation": continuation_summary,
        "samples": samples,
        "thresholds": {
            "tcg_reference_first_token_median_seconds": TCG_REF_FIRST_MEDIAN_S,
            "tcg_reference_continuation_median_seconds": TCG_REF_CONTINUATION_MEDIAN_S,
            "sub_second_claim_seconds": SUB_SECOND_CLAIM_SECONDS,
            "sub_second_claim_allowed": bool(sub_second_claim_ok),
            "note": (
                "Do not declare sub-second latency without a native or KVM "
                "campaign; TCG medians remain reference-only."
            ),
        },
    }
    with open(REPORT, "w") as handle:
        json.dump(report, handle, indent=2, sort_keys=True)
        handle.write("\n")
    print("GGUF_KVM_FIRST_TOKEN_MEDIAN_SECONDS=%.3f" % first_median)
    print("GGUF_KVM_CONTINUATION_MEDIAN_SECONDS=%.3f" % cont_median)
    print("GGUF_KVM_SUB_SECOND_CLAIM_ALLOWED=%d" % (1 if sub_second_claim_ok else 0))
    print("GGUF_KVM_REPORT=%s" % os.path.relpath(REPORT, ROOT))
    if MAX_SPREAD_RATIO > 0.0:
        observed = max(first_summary["spread_ratio"], continuation_summary["spread_ratio"])
        if observed > MAX_SPREAD_RATIO:
            print("GGUF_KVM_VARIANCE_ALERT=1 observed=%.3f threshold=%.3f" %
                  (observed, MAX_SPREAD_RATIO))
        else:
            print("GGUF_KVM_VARIANCE_ALERT=0 observed=%.3f threshold=%.3f" %
                  (observed, MAX_SPREAD_RATIO))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("GGUF QEMU KVM benchmark failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
