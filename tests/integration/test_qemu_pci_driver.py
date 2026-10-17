#!/usr/bin/env python3
"""Lot 4 (US-001) contract: PCI enumeration moved to the Ring 3 pcidriver.

1. The kernel spawns bin/pcidriver at boot. The worker opens 0xCF8-0xCFF
   through its own TSS IOPB grant, walks the bus at CPL 3 and publishes the
   inventory; the kernel then closes its own Ring 0 config access.
2. `lspci` lists the published functions (host bridge, VGA with its BAR)
   from the inventory, owner = publisher = the worker.
3. pcirogue: publish, claim and the "pci-driver" name are refused; a raw
   IN on 0xCFC raises #GP and kills only that task.
4. Supervised restart: the root shell kills pcidriver; the next client
   request respawns bin/pcidriver (budgeted), which republishes
   (generation 2, stale 0, respawns 1).
5. `pci-rescan` (IPC) makes the live worker walk the bus again (generation 3).
"""
import os
import re
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_ata_driver as ata  # noqa: E402
from test_qemu_vfs_service import normalized_log  # noqa: E402

ata.LOG = os.path.join(ata.LOG_DIR, "pci-driver.log")
ata.ERR = os.path.join(ata.LOG_DIR, "pci-driver.err")
ata.MON = os.path.join(ata.LOG_DIR, "pci-driver-monitor.sock")
KERNEL = os.environ.get("MOHHDY_PCI_KERNEL", os.path.join(ata.ROOT, "build", "mohhdy.bin"))
STATUS_RE = (r"lspci ok functions (\d+) owner (-?\d+) publisher (-?\d+) gen (\d+) stale (\d+) "
             r"respawns (\d+) ring0denied (\d+) rejected (\d+) end")


def lspci(client, proc):
    start = ata.send_command_until(client, "lspci", "lspci ok functions", proc)
    deadline = time.monotonic() + 10
    while True:
        chunk = normalized_log(ata.log_text()[start:])
        match = re.search(STATUS_RE, chunk)
        if match or time.monotonic() > deadline:
            break
        time.sleep(0.1)
    if not match:
        raise RuntimeError("unparsable lspci: %s" % chunk[-800:])
    return [int(v) for v in match.groups()], chunk


def main():
    os.makedirs(ata.LOG_DIR, exist_ok=True)
    for path in (ata.LOG, ata.ERR, ata.MON):
        try:
            os.remove(path)
        except OSError:
            pass
    command = [
        "qemu-system-i386", "-kernel", KERNEL, "-initrd", ata.INITRD,
        "-m", "1024M", "-display", "none", "-vga", "std",
        "-serial", "file:" + ata.LOG, "-monitor", "unix:%s,server,nowait" % ata.MON,
        "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
    ]
    proc = client = None
    try:
        with open(ata.ERR, "ab") as err:
            proc = subprocess.Popen(command, stdout=err, stderr=err)
        ata.wait_for("[0m : ", proc, timeout=60)
        client = ata.connect_monitor()
        # 1. Boot spawn and Ring 3 publication.
        ata.wait_for("[PCI] boot pcidriver spawned", proc, timeout=10)
        try:
            ata.wait_for("[PCI] inventory published by Ring 3 pcidriver pid", proc, timeout=5)
        except RuntimeError:
            ata.send_command_until(client, "yield", "yield ok", proc)
            ata.wait_for("[PCI] inventory published by Ring 3 pcidriver pid", proc, timeout=10)
        boot = normalized_log(ata.log_text())
        match = re.search(r"pcidriver boot ring3 enumeration functions (\d+) publish rc (-?\d+)", boot)
        if not match or match.group(1) != match.group(2) or int(match.group(1)) < 3:
            raise RuntimeError("pcidriver boot enumeration missing: %s" % boot[-1500:])
        # 2. lspci from the inventory.
        status, chunk = lspci(client, proc)
        count, owner, publisher, gen, stale, respawns, _, _ = status
        if owner <= 0 or owner != publisher or gen != 1 or stale != 0 or respawns != 0 or count < 3:
            raise RuntimeError("unexpected inventory status %r" % status)
        if not re.search(r"pci 0:0\.0 id 8086[0-9a-f]{4} classe 6\.0", chunk):
            raise RuntimeError("host bridge absent from lspci: %s" % chunk[-800:])
        if not re.search(r"classe 3\.0 bar0 [0-9a-f]{7}[08]", chunk):
            raise RuntimeError("VGA function with memory BAR absent: %s" % chunk[-800:])
        # 3. Rogue task.
        _, start = ata.spawn(client, proc, "pcirogue")
        for needle in ("pcirogue publish refused", "pcirogue claim refused",
                       "pcirogue register refused"):
            ata.wait_child(client, proc, needle, start)
        ata.wait_child(client, proc, ata.GP_FAULT, start)
        tail = normalized_log(ata.log_text()[start:])
        if "unexpected" in tail:
            raise RuntimeError("pcirogue obtained a PCI capability: %s" % tail[-800:])
        # 4. Kill and supervised respawn.
        ata.kill(client, proc, str(owner))
        mark = len(ata.log_text())
        status, _ = lspci(client, proc)
        ata.wait_for("[PCI] supervised respawn bin/pcidriver pid", proc, mark, timeout=10)
        new_owner = status[1]
        for _ in range(12):
            status, _ = lspci(client, proc)
            if status[1] > 0 and status[3] >= 2:
                break
            ata.send_command_until(client, "yield", "yield ok", proc)
        count2, owner2, publisher2, gen2, stale2, respawns2, _, _ = status
        if owner2 <= 0 or owner2 == owner or owner2 != publisher2 or gen2 != 2 or stale2 != 0 \
                or respawns2 != 1 or count2 != count:
            raise RuntimeError("respawned pcidriver did not republish: %r (first %r)" % (status, new_owner))
        # 5. Rescan through IPC.
        start = ata.send_command_until(client, "pci-rescan", "pci-rescan sent rc 0", proc)
        try:
            ata.wait_for("pcidriver rescan ring3 enumeration functions", proc, start, timeout=5)
        except RuntimeError:
            ata.send_command_until(client, "yield", "yield ok", proc)
            ata.wait_for("pcidriver rescan ring3 enumeration functions", proc, start, timeout=10)
        status, _ = lspci(client, proc)
        if status[3] != 3 or status[1] != owner2:
            raise RuntimeError("rescan did not republish: %r" % status)
        print("pci driver: %d functions enumerated in Ring 3 by pid %d; rogue refused (#GP); "
              "killed then respawned as pid %d (gen %d); rescan gen %d" %
              (count, owner, owner2, gen2, status[3]))
        print("QEMU Ring 3 PCI driver contract passed.")
        return 0
    finally:
        if client is not None:
            client.close()
        if proc is not None and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=4)
            except subprocess.TimeoutExpired:
                proc.kill()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU Ring 3 PCI driver contract failed: %s" % error)
        raise SystemExit(1)
