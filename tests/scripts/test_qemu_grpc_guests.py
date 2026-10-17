#!/usr/bin/env python3
"""Lot 6 US-067: one gRPC unary call over a real TCP connection between two
guests.

Two QEMU TCG guests share SharedEthernetHub(proxy_peer_syn_ack=False): the
hub floods frames and answers DHCP/ARP, it never answers TCP. Guest B runs
`grpc-serve` (plain TCP listening slots of its Ring 3 networker); guest A
runs `grpc-call` (SYS_SOCKET_CONNECT relayed to its networker). The hub
checks the bytes on the wire: SYN and SYN-ACK emitted by the guests, the
HTTP/2 client preface from A, a HEADERS frame back from B. Cleartext h2c,
no TLS. Out of make ci; target make qemu-grpc-guests.
"""
from __future__ import print_function

import os
import re
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_ne2k_guest_tls_server as base  # noqa: E402
from qemu_shared_ethernet_hub import SharedEthernetHub  # noqa: E402

PORT = 50051
LOG_DIR = base.LOG_DIR
base.INSTANCES = (
    {"label": "a", "mac": "52:54:00:a0:67:0a",
     "log": os.path.join(LOG_DIR, "grpc-guests-a.log"),
     "err": os.path.join(LOG_DIR, "grpc-guests-a.err"),
     "mon": os.path.join(LOG_DIR, "grpc-guests-a.monitor.sock")},
    {"label": "b", "mac": "52:54:00:a0:67:0b",
     "log": os.path.join(LOG_DIR, "grpc-guests-b.log"),
     "err": os.path.join(LOG_DIR, "grpc-guests-b.err"),
     "mon": os.path.join(LOG_DIR, "grpc-guests-b.monitor.sock")},
)


def send_command_once(client, command, proc, path):
    """base.send_command_once with upper case letters (gRPC paths are case
    sensitive): QEMU sendkey shift-x."""
    aliases = {" ": "spc", ".": "dot", "-": "minus", "/": "slash"}
    for char in command:
        key = aliases.get(char, ("shift-" + char.lower()) if char.isupper() else char.lower())
        count = 0
        for _ in range(base.KEY_CHAR_RETRIES):
            start = len(base.log_text(path))
            base.send_key(client, key)
            deadline = time.monotonic() + base.KEY_ECHO_TIMEOUT
            while time.monotonic() < deadline:
                if proc.poll() is not None:
                    raise RuntimeError("QEMU stopped during keyboard input")
                time.sleep(base.KEY_DELAY)
                count = base.key_echo_count(base.log_text(path)[start:], char)
                if count:
                    if char in ".s":
                        time.sleep(base.KEY_DUPLICATE_SETTLE_DELAY)
                        count = base.key_echo_count(base.log_text(path)[start:], char)
                    break
            if count:
                break
        if count == 0:
            raise RuntimeError("missing keyboard echo for %r on %s" % (char, path))
        for _ in range(count - 1):
            base.send_key(client, "backspace")
            time.sleep(base.KEY_DELAY)
    base.send_key(client, "ret")


base.send_command_once = send_command_once
WIRE = {"syn": 0, "syn_ack": 0, "preface": 0, "h2_headers_back": 0}


def ip_str(raw):
    return ".".join(str(b) for b in raw)


def watch(hub):
    """Wraps the hub frame handler to count gRPC TCP frames on the wire."""
    original = hub._handle_frame

    def handle(connection, frame):
        try:
            if len(frame) >= 54 and struct.unpack("!H", frame[12:14])[0] == 0x0800 and frame[23] == 6:
                ihl = (frame[14] & 0x0f) * 4
                t = 14 + ihl
                sport, dport = struct.unpack("!HH", frame[t:t + 4])
                flags = frame[t + 13]
                total = struct.unpack("!H", frame[16:18])[0]
                payload = frame[t + (frame[t + 12] >> 4) * 4:14 + total]
                if dport == PORT and flags & 0x12 == 0x02:
                    WIRE["syn"] += 1
                if sport == PORT and flags & 0x12 == 0x12:
                    WIRE["syn_ack"] += 1
                if dport == PORT and payload.startswith(b"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"):
                    WIRE["preface"] += 1
                if sport == PORT and len(payload) >= 9:
                    # walk the HTTP/2 frames of the server segment (the
                    # server writes whole frames per segment) and count
                    # HEADERS frames on a client stream
                    pos = 0
                    while pos + 9 <= len(payload):
                        flen = (payload[pos] << 16) | (payload[pos + 1] << 8) | payload[pos + 2]
                        sid = struct.unpack("!I", payload[pos + 5:pos + 9])[0] & 0x7FFFFFFF
                        if payload[pos + 3] == 1 and sid != 0:
                            WIRE["h2_headers_back"] += 1
                        pos += 9 + flen
        except Exception:  # never break the hub on a parse error
            pass
        return original(connection, frame)

    hub._handle_frame = handle


def main():
    if not os.path.isfile(base.KERNEL) or not os.path.isfile(base.INITRD):
        raise RuntimeError("missing KERNEL or INITRD (build first)")
    os.makedirs(LOG_DIR, exist_ok=True)
    hub = SharedEthernetHub(respond=True, full_tls=False, proxy_peer_syn_ack=False)
    watch(hub)
    hub.start()
    t0 = time.monotonic()
    a, b = base.INSTANCES
    try:
        for inst in base.INSTANCES:
            base.start_guest(inst, hub.port)
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline and hub.client_count < 2:
            time.sleep(0.1)
        for inst in base.INSTANCES:
            base.wait_for(inst["proc"], inst["log"], "[0m : ", base.BOOT_TIMEOUT)
            inst["client"] = base.monitor_connect(inst["mon"])

        def alive():
            for peer in base.INSTANCES:
                if peer["proc"].poll() is not None:
                    raise RuntimeError("guest %s died" % peer["label"])
            if hub.error is not None:
                raise RuntimeError("hub error: %s" % hub.error)

        leases = {}
        for inst in (b, a):
            base.spawn_networker(inst, alive)
            base.run_command(inst, "ai-acquire example.com",
                             "ai-acquire: DHCP, DNS et SYN LLM demarres", alive, timeout=90)
            deadline = time.monotonic() + 20
            while time.monotonic() < deadline and inst["mac"] not in {k.lower() for k in hub.leases}:
                time.sleep(0.1)
            for k, v in hub.leases.items():
                if k.lower() == inst["mac"]:
                    leases[inst["label"]] = ip_str(v)
            if inst["label"] not in leases:
                raise RuntimeError("no lease for %s: %r" % (inst["label"], hub.leases))
        base.say("[grpc-guests] leases %r (%ds)" % (leases, time.monotonic() - t0))

        # B serves two calls (one ok, one unimplemented) and blocks its shell.
        start_b = len(base.log_text(b["log"]))
        with base.INJECT_LOCK:
            send_command_once(b["client"], "grpc-serve %d 5 300" % PORT, b["proc"], b["log"])
        base.wait_for(b["proc"], b["log"], "grpc-serve listening port %d" % PORT, 30, start_b)

        start_a = base.run_command(
            a, "grpc-call %s %s %d echo.EchoService SayHello guest-a" % (leases["a"], leases["b"], PORT),
            "grpc-call ", alive, timeout=180)
        base.wait_for(a["proc"], a["log"], "[0m : ", 200, start_a)
        out_a = base.normalized_log(base.log_text(a["log"])[start_a:])
        want = ("grpc-call ok http 200 grpc-status 0 frames 3 server_settings 1 settings_ack 1",
                "message Hello guest-a from guest over tcp transport=tcp")
        for w in want:
            if w not in out_a:
                raise RuntimeError("grpc-call A: missing %r in %s; wire %r" % (w, out_a[-1500:], WIRE))
        base.say("[grpc-guests] call 1 ok (%ds)" % (time.monotonic() - t0))

        start_a = base.run_command(
            a, "grpc-call %s %s %d x.Nope Missing" % (leases["a"], leases["b"], PORT),
            "grpc-call ", alive, timeout=180)
        base.wait_for(a["proc"], a["log"], "[0m : ", 200, start_a)
        out_a = base.normalized_log(base.log_text(a["log"])[start_a:])
        if "grpc-call ok http 200 grpc-status 12 frames 2" not in out_a or "message unknown method" not in out_a:
            raise RuntimeError("grpc-call A unimplemented: %s" % out_a[-1500:])

        # Lot 7 (US-067): three calls pipelined on ONE connection (streams
        # 1, 3, 5) with a 16-byte stream window: the server must hold its
        # DATA and split it until the client WINDOW_UPDATE frames reopen it.
        start_a = base.run_command(
            a, "grpc-calls %s %s %d 3 16 echo.EchoService SayHello multi" % (leases["a"], leases["b"], PORT),
            "grpc-calls ", alive, timeout=240)
        base.wait_for(a["proc"], a["log"], "[0m : ", 300, start_a)
        out_a = base.normalized_log(base.log_text(a["log"])[start_a:])
        want = ["grpc-calls stream %d ok http 200 grpc-status 0 frames 5 message Hello multi-%d from guest over tcp"
                % (1 + 2 * i, i + 1) for i in range(3)]
        want.append("grpc-calls ok calls 3 succeeded 3 connections 1 window 16")
        for w in want:
            if w not in out_a:
                raise RuntimeError("grpc-calls A: missing %r in %s; wire %r" % (w, out_a[-2000:], WIRE))
        m = re.search(r"window_updates_sent (\d+) window_updates_recv (\d+) data_frames_recv (\d+)", out_a)
        if not m or int(m.group(1)) < 6 or int(m.group(2)) < 3 or int(m.group(3)) < 9:
            raise RuntimeError("grpc-calls A flow control counters: %s" % out_a[-1200:])
        base.say("[grpc-guests] 3 calls on one connection ok, window 16 (%ds)" % (time.monotonic() - t0))

        base.wait_for(b["proc"], b["log"], "grpc-serve ok served 5 failed 0 connections 3", 60, start_b)
        out_b = base.normalized_log(base.log_text(b["log"])[start_b:])
        if "path /echo.EchoService/SayHello client_settings 1" not in out_b or "path /x.Nope/Missing" not in out_b:
            raise RuntimeError("grpc-serve B: %s" % out_b[-1500:])
        for sid in (1, 3, 5):
            if "stream %d path /echo.EchoService/SayHello client_settings 1 peer_window 16" % sid not in out_b:
                raise RuntimeError("grpc-serve B stream %d: %s" % (sid, out_b[-1500:]))
        m = re.search(r"connection slot \d+ calls 3 window_updates_sent \d+ window_updates_recv (\d+) "
                      r"data_frames_sent (\d+) blocked (\d+) goaway 1", out_b)
        if not m or int(m.group(1)) < 6 or int(m.group(2)) < 6 or int(m.group(3)) < 3:
            raise RuntimeError("grpc-serve B flow control: %s" % out_b[-1500:])
        if WIRE["syn"] < 2 or WIRE["syn_ack"] < 2 or WIRE["preface"] < 2 or WIRE["h2_headers_back"] < 2:
            raise RuntimeError("wire counters %r" % WIRE)
        if hub.events.get("cross_syn_ack", 0) != 0:
            raise RuntimeError("hub proxied a SYN-ACK: %r" % hub.events)
        base.say("[grpc-guests] PASS wire %r in %ds" % (WIRE, time.monotonic() - t0))
        return 0
    finally:
        for inst in base.INSTANCES:
            base.terminate(inst.get("proc"))
            if inst.get("err_handle"):
                inst["err_handle"].close()
        hub.close()


if __name__ == "__main__":
    try:
        sys.exit(main())
    except RuntimeError as error:
        sys.stderr.write("FAIL: %s\n" % error)
        sys.exit(1)
