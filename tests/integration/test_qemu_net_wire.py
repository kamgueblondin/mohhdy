#!/usr/bin/env python3
"""Tranche 5 slice 3 contract: wire TCP through the net-driver worker.

QEMU's NE2000 is connected to a local echo peer (tests/scripts/
qemu_wire_echo_peer.py) with ``-netdev socket``: no user-net, no public
internet. The peer owns 10.32.0.2 and echoes TCP port 7.

Degraded (no net-driver): netwire's direct SYS_NET_WIRE_* calls are refused
(-59) and its relayed connect is refused too (the wire path exists only
through the worker).
Worker live: netwire (a plain task) runs connect / send / receive / close
with the public socket syscalls; they are relayed to networker, which drives
real frames with the worker-only SYS_NET_WIRE_* path (ARP, IPv4/TCP framing,
receive demux). The test checks the echo in the guest, the worker relay
lines, the kernel wire counters (net-wire-status) and the peer's own view of
the frames (checksums verified host-side). The NE2000 driver, its IRQ
handler and the TCP state machine stay in Ring 0.
"""
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "scripts"))
import test_qemu_net_worker as nw  # noqa: E402
from test_qemu_vfs_service import normalized_log  # noqa: E402
from qemu_wire_echo_peer import WireEchoPeer  # noqa: E402

ROOT = nw.ROOT
nw.LOG = os.path.join(nw.LOG_DIR, "net-wire.log")
nw.ERR = os.path.join(nw.LOG_DIR, "net-wire.err")
nw.MON = os.path.join(nw.LOG_DIR, "net-wire-monitor.sock")

WIRE_KEYS = ("worker", "connects", "tx", "rx", "arp", "sends", "recvs", "closes",
             "demuxed", "dropped", "fins", "refused", "bound")
MESSAGE = b"mohhdy-wire-echo"


def wire_status(client, proc):
    start = nw.send_command_until(client, "net-wire-status", "net-wire ok worker", proc)
    nw.wait_for(" end", proc, start, timeout=5)
    text = normalized_log(nw.log_text()[start:])
    pattern = r"net-wire ok worker (-?\d+)" + "".join(
        r" %s (\d+)" % key for key in WIRE_KEYS[1:]) + " end"
    match = re.search(pattern, text)
    if not match:
        raise RuntimeError("unparsable net-wire-status")
    return dict(zip(WIRE_KEYS, (int(v) for v in match.groups())))


def main():
    os.makedirs(nw.LOG_DIR, exist_ok=True)
    for path in (nw.LOG, nw.ERR, nw.MON):
        try:
            os.remove(path)
        except OSError:
            pass
    peer = WireEchoPeer()
    peer.start()
    command = [
        "qemu-system-i386", "-kernel", nw.KERNEL, "-initrd", nw.INITRD,
        "-m", "1024M", "-display", "none", "-vga", "none",
        "-serial", "file:" + nw.LOG,
        "-monitor", "unix:%s,server,nowait" % nw.MON,
        "-machine", "type=pc,accel=tcg",
        "-netdev", "socket,id=n0,connect=127.0.0.1:%d" % peer.port,
        "-device", "ne2k_isa,netdev=n0,mac=52:54:00:12:34:56,irq=3",
        "-no-reboot", "-no-shutdown",
    ]
    with open(nw.ERR, "wb") as err_handle:
        proc = subprocess.Popen(command, stdout=err_handle, stderr=err_handle)
        monitor = None
        try:
            nw.wait_for("(-.-)", proc)
            monitor = nw.connect_monitor()
            time.sleep(0.5)
            nw.send_command_until(monitor, "net-status", "Carte Ethernet : detectee", proc)
            base = wire_status(monitor, proc)
            if base["worker"] != 0 or base["tx"] != 0 or base["bound"] != 0:
                raise RuntimeError("wire active at boot: %r" % base)
            # Degraded: no worker, so no wire path at all for anyone.
            pid, start = nw.spawn(monitor, proc, "netwire")
            nw.wait_child(monitor, proc, "netwire raw wire refused 4 of 4", start)
            nw.wait_child(monitor, proc, "netwire connect worker-required", start)
            nw.kill(monitor, proc, pid)
            degraded = wire_status(monitor, proc)
            if degraded["tx"] != 0 or degraded["refused"] != 5:
                raise RuntimeError("degraded wire counters: %r" % degraded)
            if peer.snapshot()["frames_rx"] != 0:
                raise RuntimeError("frames on the wire without a worker")
            # Worker live.
            worker_pid, start = nw.spawn(monitor, proc, "networker")
            nw.wait_child(monitor, proc, "net-driver ready", start)
            nw.wait_child(monitor, proc, "net-driver gated syscalls ok", start)
            # Tranche 5 suite: the worker owns the NE2000 ports through the
            # TSS I/O bitmap and re-reads the station PROM itself at CPL 3.
            nw.wait_child(monitor, proc, "[NET] NE2000 ports 0x300-0x31F handed to the Ring 3 worker",
                          start)
            nw.wait_child(monitor, proc,
                          "net-driver nic ring3 ok base 768 irq 3 mac 52:54:00:12:34:56 prom-match 1",
                          start)
            relay_before = nw.relay_status(monitor, proc)
            pid, start = nw.spawn(monitor, proc, "netwire")
            nw.wait_child(monitor, proc, "netwire raw wire refused 4 of 4", start)
            nw.wait_child(monitor, proc, "netwire connect ok socket", start, rounds=20)
            nw.wait_child(monitor, proc, "netwire echo ok bytes %d" % len(MESSAGE), start,
                          rounds=30)
            nw.wait_child(monitor, proc, "netwire done", start)
            text = normalized_log(nw.log_text()[start:])
            match = re.search(r"netwire echo ok bytes (\d+) relayed (\d+) frames tx (\d+) "
                              r"rx (\d+) demuxed (\d+) refused (\d+) bound (\d+)", text)
            if not match:
                raise RuntimeError("unparsable netwire summary")
            echo = [int(v) for v in match.groups()]
            _, relayed, g_tx, g_rx, g_demux, g_refused, g_bound = echo
            if relayed < 4 or g_tx < 5 or g_rx < 4 or g_demux < 3 or g_refused != 4 or g_bound != 0:
                raise RuntimeError("unexpected netwire summary: %r" % echo)
            # The worker ran the relayed connect (144) and the wire ops.
            if not re.search(r"net-driver relay op 144 rc \d+ reply 0 total \d+ wire \d+", text):
                raise RuntimeError("worker did not run the relayed connect")
            for op in (101, 103, 104):
                if not re.search(r"net-driver relay op %d rc 0 reply 0 total \d+ wire \d+" % op,
                                 text):
                    raise RuntimeError("worker did not run relayed op %d on the wire" % op)
            # Every frame went through the Ring 3 driver: the kernel only
            # built them (pumps), never touched the ports, and IRQ3 reached
            # the worker as a counter.
            reports = re.findall(r"net-driver nic owner (\d+) tx (\d+) txfail (\d+) rx (\d+) "
                                 r"irq (\d+) kirq (\d+) pumps (\d+) out (\d+) in (\d+) "
                                 r"refused (\d+) gated (\d+) end", text)
            if not reports:
                raise RuntimeError("worker never reported its NIC counters")
            nic = [int(v) for v in reports[-1]]
            n_owner, n_tx, n_txfail, n_rx, n_irq, n_kirq, n_pumps, n_out, n_in, n_ref, n_gated = nic
            if (n_owner != int(worker_pid) or n_txfail != 0 or n_tx != n_out or n_rx != n_in or
                    n_tx < 5 or n_rx < 4 or n_ref != 0 or n_gated != 0 or n_kirq < 1 or
                    n_irq < 1 or n_pumps < 4):
                raise RuntimeError("unexpected Ring 3 NIC counters: %r" % nic)
            # Tranche 5 pile: ARP/IPv4/TCP framing and demux ran in the
            # worker; the kernel wire engine built and decoded nothing.
            stacks = re.findall(r"net-driver stack ring3 sockets (\d+) wire (\d+) llm (\d+) "
                                r"rounds (\d+) framed (\d+) decoded (\d+) kernel-pumps (\d+) "
                                r"kernel-out (\d+) kernel-in (\d+) end", text)
            if "net-driver stack ring3 ready arp ipv4 tcp tls" not in text or not stacks:
                raise RuntimeError("worker did not run the Ring 3 stack")
            stk = [int(v) for v in stacks[-1]]
            if (stk[1] != 4 or stk[4] != n_tx or stk[5] != n_rx or stk[3] != n_pumps or
                    stk[6] != 0 or stk[7] != 0 or stk[8] != 0):
                raise RuntimeError("unexpected Ring 3 stack counters: %r" % stk)
            nw.kill(monitor, proc, pid)
            live = wire_status(monitor, proc)
            relay_after = nw.relay_status(monitor, proc)
            if (live["worker"] != int(worker_pid) or live["connects"] != 1 or
                    live["sends"] != 1 or live["recvs"] < 1 or live["closes"] != 1 or
                    live["fins"] != 1 or live["bound"] != 0 or live["refused"] != 9 or
                    live["arp"] < 1):
                raise RuntimeError("unexpected wire counters: %r" % live)
            host = peer.snapshot()
            if (host["arp_requests"] < 1 or host["syn"] != 1 or host["data_segments"] != 1 or
                    host["echoed_bytes"] != len(MESSAGE) or host["fins"] != 1 or
                    host["bad_checksum"] != 0 or peer.payloads != [MESSAGE] or peer.errors):
                raise RuntimeError("unexpected peer view: %r %r %r" %
                                   (host, peer.payloads, peer.errors))
            # Kernel and peer agree on frame counts (guest tx = peer rx).
            if live["tx"] != host["frames_rx"] or live["rx"] < host["frames_tx"]:
                raise RuntimeError("frame counts disagree: kernel %r peer %r" % (live, host))
            if n_tx != host["frames_rx"]:
                raise RuntimeError("Ring 3 tx %d != peer rx %d" % (n_tx, host["frames_rx"]))
            relayed_calls = relay_after["done"] - relay_before["done"]
            # Worker loss: the kernel takes the card back, re-initialises it
            # and a fresh worker can claim it again.
            mark = len(nw.log_text())
            nw.kill(monitor, proc, worker_pid)
            nw.wait_for("[NET] NE2000 back in Ring 0 after worker loss", proc, mark, timeout=20)
            worker2, start = nw.spawn(monitor, proc, "networker")
            nw.wait_child(monitor, proc,
                          "net-driver nic ring3 ok base 768 irq 3 mac 52:54:00:12:34:56 prom-match 1",
                          start)
            if int(worker2) == int(worker_pid):
                raise RuntimeError("respawned worker reused pid %s" % worker2)
            print("wire tcp: guest->peer frames %d, peer->guest frames %d (kernel rx %d), "
                  "arp %d, relayed calls %d, echo %d bytes, raw wire refused %d, "
                  "peer checksum errors %d" %
                  (live["tx"], host["frames_tx"], live["rx"], live["arp"], relayed_calls,
                   host["echoed_bytes"], live["refused"], host["bad_checksum"]))
            print("ring3 nic: owner pid %d, worker tx %d rx %d, irq3 %d (kernel counted %d), "
                  "pumps %d, kernel port accesses %d, reclaim+reclaim-claim ok" %
                  (n_owner, n_tx, n_rx, n_irq, n_kirq, n_pumps, n_ref))
            print("ring3 stack: wire ops %d, framed %d, decoded %d, rounds %d, kernel pumps %d" %
                  (stk[1], stk[4], stk[5], stk[3], stk[6]))
            print("MOHHDY Tranche 5 wire TCP via worker contract passed")
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
            peer.close()
            try:
                os.remove(nw.MON)
            except OSError:
                pass


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("MOHHDY Tranche 5 wire TCP via worker contract failed: %s" % error,
              file=sys.stderr)
        raise SystemExit(1)
