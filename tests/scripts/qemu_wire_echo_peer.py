#!/usr/bin/env python3
"""On-link TCP echo peer for the Tranche 5 slice 3 wire proof.

QEMU connects its NE2000 to this process with
``-netdev socket,id=n0,connect=127.0.0.1:PORT``; every Ethernet frame is
carried with a 4-byte big-endian length prefix. The peer owns 10.32.0.2
(MAC 52:54:00:a0:20:02), answers ARP for it and runs a minimal TCP echo
service on port 7 (SYN-ACK, echo each in-order data segment piggybacked on
its ACK, FIN-ACK on FIN). No traffic leaves the host. Every guest frame's
IPv4 and TCP checksums are verified and counted, so the test can assert
that real frames crossed the wire.
"""
import socket
import struct
import threading

SERVER_MAC = b"\x52\x54\x00\xa0\x20\x02"
SERVER_IP = b"\x0a\x20\x00\x02"  # 10.32.0.2
ECHO_PORT = 7
SERVER_ISN = 0x10000000


def _checksum(data):
    if len(data) & 1:
        data += b"\0"
    total = sum((data[i] << 8) | data[i + 1] for i in range(0, len(data), 2))
    while total >> 16:
        total = (total & 0xFFFF) + (total >> 16)
    return (~total) & 0xFFFF


def _ipv4_tcp(src_ip, dst_ip, src_port, dst_port, seq, ack, flags, payload=b""):
    tcp = bytearray(20 + len(payload))
    tcp[0:4] = struct.pack("!HH", src_port, dst_port)
    tcp[4:12] = struct.pack("!II", seq & 0xFFFFFFFF, ack & 0xFFFFFFFF)
    tcp[12] = 0x50
    tcp[13] = flags
    tcp[14:16] = b"\xff\xff"
    tcp[20:] = payload
    pseudo = src_ip + dst_ip + b"\x00\x06" + struct.pack("!H", len(tcp))
    tcp[16:18] = struct.pack("!H", _checksum(pseudo + bytes(tcp)))
    ip = bytearray(20)
    ip[0] = 0x45
    ip[2:4] = struct.pack("!H", 20 + len(tcp))
    ip[6:8] = b"\x40\x00"
    ip[8] = 64
    ip[9] = 6
    ip[12:16] = src_ip
    ip[16:20] = dst_ip
    ip[10:12] = struct.pack("!H", _checksum(bytes(ip)))
    return bytes(ip) + bytes(tcp)


class WireEchoPeer:
    def __init__(self):
        self.counters = {
            "frames_rx": 0, "frames_tx": 0, "arp_requests": 0, "syn": 0,
            "data_segments": 0, "echoed_bytes": 0, "fins": 0, "acks": 0,
            "bad_checksum": 0, "other": 0,
        }
        self.payloads = []
        self.guest_ip = None
        self.errors = []
        self._conns = {}
        self._stop = threading.Event()
        self._lock = threading.Lock()
        self._listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._listener.bind(("127.0.0.1", 0))
        self._listener.listen(1)
        self._listener.settimeout(0.2)
        self.port = self._listener.getsockname()[1]
        self._thread = threading.Thread(target=self._serve, daemon=True)

    def start(self):
        self._thread.start()

    def close(self):
        self._stop.set()
        self._thread.join(timeout=2)
        try:
            self._listener.close()
        except OSError:
            pass

    def snapshot(self):
        with self._lock:
            return dict(self.counters)

    def _bump(self, key, amount=1):
        with self._lock:
            self.counters[key] += amount

    def _send(self, conn, dst_mac, ethertype, payload):
        frame = dst_mac + SERVER_MAC + struct.pack("!H", ethertype) + payload
        if len(frame) < 60:
            frame += b"\0" * (60 - len(frame))
        conn.sendall(struct.pack("!I", len(frame)) + frame)
        self._bump("frames_tx")

    def _handle(self, conn, frame):
        self._bump("frames_rx")
        if len(frame) < 14:
            self._bump("other")
            return
        src_mac = frame[6:12]
        ethertype = struct.unpack("!H", frame[12:14])[0]
        if ethertype == 0x0806 and len(frame) >= 42:
            opcode = struct.unpack("!H", frame[20:22])[0]
            if opcode == 1 and frame[38:42] == SERVER_IP:
                self._bump("arp_requests")
                sender_mac, sender_ip = frame[22:28], frame[28:32]
                reply = (b"\x00\x01\x08\x00\x06\x04\x00\x02" + SERVER_MAC + SERVER_IP +
                         sender_mac + sender_ip)
                self._send(conn, sender_mac, 0x0806, reply)
                return
            self._bump("other")
            return
        if ethertype != 0x0800 or len(frame) < 34:
            self._bump("other")
            return
        ip = frame[14:]
        ihl = (ip[0] & 0x0F) * 4
        total = struct.unpack("!H", ip[2:4])[0]
        if ip[9] != 6 or ip[16:20] != SERVER_IP or total > len(ip):
            self._bump("other")
            return
        if _checksum(bytes(ip[:ihl])) != 0:
            self._bump("bad_checksum")
            return
        tcp = bytes(ip[ihl:total])
        src_ip = bytes(ip[12:16])
        pseudo = src_ip + SERVER_IP + b"\x00\x06" + struct.pack("!H", len(tcp))
        if _checksum(pseudo + tcp) != 0:
            self._bump("bad_checksum")
            return
        src_port, dst_port, seq, _ack = struct.unpack("!HHII", tcp[0:12])
        flags = tcp[13]
        payload = tcp[(tcp[12] >> 4) * 4:]
        if dst_port != ECHO_PORT:
            self._bump("other")
            return
        self.guest_ip = src_ip
        key = (src_ip, src_port)
        state = self._conns.get(key)

        def reply(flags_out, data=b""):
            s = self._conns[key]
            packet = _ipv4_tcp(SERVER_IP, src_ip, ECHO_PORT, src_port, s["snd"], s["rcv"],
                               flags_out, data)
            self._send(conn, src_mac, 0x0800, packet)

        if flags & 0x02:  # SYN (first or retried)
            self._bump("syn")
            self._conns[key] = {"snd": SERVER_ISN, "rcv": (seq + 1) & 0xFFFFFFFF}
            reply(0x12)
            self._conns[key]["snd"] = SERVER_ISN + 1
            return
        if state is None:
            self._bump("other")
            return
        if payload:
            if seq != state["rcv"]:
                reply(0x10)  # duplicate / out of order: re-ACK only
                return
            self._bump("data_segments")
            self.payloads.append(bytes(payload))
            state["rcv"] = (state["rcv"] + len(payload)) & 0xFFFFFFFF
            reply(0x18, payload)  # PSH|ACK carrying the echo
            state["snd"] = (state["snd"] + len(payload)) & 0xFFFFFFFF
            self._bump("echoed_bytes", len(payload))
        if flags & 0x01:  # FIN
            self._bump("fins")
            state["rcv"] = (state["rcv"] + 1) & 0xFFFFFFFF
            reply(0x11)
            state["snd"] = (state["snd"] + 1) & 0xFFFFFFFF
            return
        if not payload:
            self._bump("acks")

    def _serve(self):
        conn = None
        while not self._stop.is_set() and conn is None:
            try:
                conn, _ = self._listener.accept()
            except socket.timeout:
                continue
            except OSError:
                return
        if conn is None:
            return
        conn.settimeout(0.2)
        buffer = b""
        try:
            while not self._stop.is_set():
                try:
                    chunk = conn.recv(65536)
                except socket.timeout:
                    continue
                if not chunk:
                    return
                buffer += chunk
                while len(buffer) >= 4:
                    size = struct.unpack("!I", buffer[:4])[0]
                    if len(buffer) < 4 + size:
                        break
                    frame, buffer = buffer[4:4 + size], buffer[4 + size:]
                    try:
                        self._handle(conn, frame)
                    except Exception as error:  # keep serving, report later
                        self.errors.append(repr(error))
        except OSError:
            return
        finally:
            conn.close()
