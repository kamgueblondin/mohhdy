#!/usr/bin/env python3
"""Run every versioned QEMU integration contract with bounded concurrency.

The individual contracts own their serial logs, QEMU monitor sockets and mutable
fixtures.  This runner only schedules them: it neither elides assertions nor
changes the commands exercised by each contract.
"""

import os
import signal
import subprocess
import sys
import time


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
PYTHON = sys.executable

# Les scénarios restent isolés mais partagent le contrôleur PS/2 émulé. Une
# exécution séquentielle est donc le défaut CI : elle évite qu’une frappe d’un
# contrat soit perdue par une autre instance QEMU tout en conservant les sept
# assertions sous le budget de 25 minutes. Une concurrence explicite reste
# disponible via QEMU_INTEGRATION_JOBS pour le diagnostic local.
CONTRACTS = (
    ("vfs-service", (PYTHON, "tests/integration/test_qemu_vfs_service.py")),
    ("service-grant", (PYTHON, "tests/integration/test_qemu_service_grant.py")),
    ("core", (PYTHON, "tests/integration/test_qemu_core_contract.py")),
    ("ipc-foundation", (PYTHON, "tests/integration/test_qemu_ipc_foundation.py")),
    ("ai-provider", (PYTHON, "tests/scripts/test_ai_provider_commands.py")),
    ("irq0-preemption", (PYTHON, "tests/integration/test_qemu_irq0_preemption.py")),
    ("ne2k-status", (PYTHON, "tests/scripts/test_qemu_ne2k_status.py")),
)


def configured_jobs():
    raw = os.environ.get("QEMU_INTEGRATION_JOBS", "1")
    try:
        jobs = int(raw)
    except ValueError:
        raise RuntimeError("QEMU_INTEGRATION_JOBS must be a positive integer, got %r" % raw)
    if jobs < 1:
        raise RuntimeError("QEMU_INTEGRATION_JOBS must be at least 1, got %d" % jobs)
    return min(jobs, len(CONTRACTS))


def assert_artifacts():
    required = ("build/mohhdy.bin", "my_initrd.tar")
    missing = [path for path in required if not os.path.isfile(os.path.join(ROOT, path))]
    if missing:
        raise RuntimeError("missing integration artefacts: %s" % ", ".join(missing))


def terminate(process):
    if process.poll() is not None:
        return
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        return
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            return
        process.wait(timeout=5)


def cancel_active(active):
    for item in active:
        terminate(item["process"])


def report(item, status):
    elapsed = time.monotonic() - item["started"]
    result = "passed" if status == 0 else "failed (%d)" % status
    print("[QEMU integration] %s %s in %.1fs" % (item["name"], result, elapsed), flush=True)


def reap_one(active):
    while True:
        for item in tuple(active):
            status = item["process"].poll()
            if status is None:
                continue
            active.remove(item)
            report(item, status)
            if status != 0:
                cancel_active(active)
                raise RuntimeError("QEMU integration contract failed: %s" % item["name"])
            return
        time.sleep(0.1)


def run():
    assert_artifacts()
    jobs = configured_jobs()
    dry_run = os.environ.get("QEMU_INTEGRATION_DRY_RUN") == "1"
    print("[QEMU integration] scheduling %d contracts with %d worker(s)" % (len(CONTRACTS), jobs), flush=True)

    if dry_run:
        for name, command in CONTRACTS:
            print("[QEMU integration] plan %s: %s" % (name, " ".join(command)), flush=True)
        return

    active = []
    started_at = time.monotonic()
    try:
        for name, command in CONTRACTS:
            while len(active) >= jobs:
                reap_one(active)
            print("[QEMU integration] starting %s" % name, flush=True)
            process = subprocess.Popen(command, cwd=ROOT, start_new_session=True)
            active.append({"name": name, "process": process, "started": time.monotonic()})
        while active:
            reap_one(active)
    except BaseException:
        cancel_active(active)
        raise

    elapsed = time.monotonic() - started_at
    print("[QEMU integration] all %d contracts passed in %.1fs" % (len(CONTRACTS), elapsed), flush=True)


if __name__ == "__main__":
    try:
        run()
    except Exception as error:
        print("QEMU integration runner failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
