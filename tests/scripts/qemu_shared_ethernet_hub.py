#!/usr/bin/env python3
"""Hub L2 QEMU socket pour topologie Ethernet locale partagée (tranche 2).

Plusieurs invités `-netdev socket,connect=127.0.0.1:PORT` partagent le même
segment. Chaque trame Ethernet est préfixée d'une longueur big-endian 32 bits
(protocole stream QEMU). Le hub inonde les autres clients et peut répondre
DHCP/ARP/DNS localement (pair contrôlé, sans TAP ni Internet).
"""
from __future__ import print_function

import socket
import struct
import threading

SERVER_MAC = b"\x52\x54\x00\xa0\x20\x02"
SERVER_IP = b"\x0a\x20\x00\x02"  # 10.32.0.2
GUEST_IP = b"\x0a\x20\x00\x0f"  # 10.32.0.15
NETMASK = b"\xff\xff\xff\x00"
REMOTE_IP = b"\xcb\x00\x71\x14"  # 203.0.113.20 TEST-NET-3


def _checksum(data):
    if len(data) & 1:
        data += b"\0"
    total = sum((data[index] << 8) | data[index + 1] for index in range(0, len(data), 2))
    while total >> 16:
        total = (total & 0xffff) + (total >> 16)
    return (~total) & 0xffff


def _ipv4_packet(source_ip, destination_ip, protocol, payload):
    total_length = 20 + len(payload)
    ip = bytearray(20)
    ip[0] = 0x45
    ip[2:4] = struct.pack("!H", total_length)
    ip[6:8] = b"\x40\x00"
    ip[8] = 64
    ip[9] = protocol
    ip[12:16] = source_ip
    ip[16:20] = destination_ip
    ip[10:12] = struct.pack("!H", _checksum(bytes(ip)))
    return bytes(ip) + payload


def _ipv4_udp(source_ip, destination_ip, source_port, destination_port, payload):
    udp_length = 8 + len(payload)
    udp = struct.pack("!HHHH", source_port, destination_port, udp_length, 0)
    return _ipv4_packet(source_ip, destination_ip, 17, udp + payload)


def _ethernet(destination_mac, ethertype, payload):
    return destination_mac + SERVER_MAC + struct.pack("!H", ethertype) + payload


def _dhcp_type(payload):
    position = 240
    while position < len(payload):
        code = payload[position]
        position += 1
        if code == 255:
            return 0
        if code == 0:
            continue
        if position >= len(payload):
            return 0
        length = payload[position]
        position += 1
        if position + length > len(payload):
            return 0
        if code == 53 and length == 1:
            return payload[position]
        position += length
    return 0


def _dhcp_reply(request, message_type):
    xid = request[4:8]
    client_mac = request[28:34]
    payload = bytearray(278)
    payload[0:4] = b"\x02\x01\x06\x00"
    payload[4:8] = xid
    payload[16:20] = GUEST_IP
    payload[28:34] = client_mac
    payload[236:240] = b"\x63\x82\x53\x63"
    options = bytearray()
    options += b"\x35\x01" + bytes((message_type,))
    options += b"\x36\x04" + SERVER_IP
    if message_type == 5:
        options += b"\x01\x04" + NETMASK
        options += b"\x03\x04" + SERVER_IP
        options += b"\x06\x04" + SERVER_IP
        options += b"\x33\x04" + struct.pack("!I", 86400)
    options += b"\xff"
    payload[240:240 + len(options)] = options
    return _ethernet(
        b"\xff" * 6,
        0x0800,
        _ipv4_udp(SERVER_IP, b"\xff\xff\xff\xff", 67, 68, bytes(payload)),
    )


def _arp_reply(request):
    sender_mac = request[22:28]
    sender_ip = request[28:32]
    payload = (
        b"\x00\x01\x08\x00\x06\x04\x00\x02"
        + SERVER_MAC
        + SERVER_IP
        + sender_mac
        + sender_ip
    )
    return _ethernet(sender_mac, 0x0806, payload)


def _dns_reply(request_payload):
    if len(request_payload) < 17:
        return None
    question_end = 12
    while question_end < len(request_payload) and request_payload[question_end] != 0:
        label_length = request_payload[question_end]
        question_end += 1 + label_length
    question_end += 5
    if question_end > len(request_payload):
        return None
    header = bytearray(12)
    header[0:2] = request_payload[0:2]
    header[2:4] = b"\x81\x80"
    header[4:6] = b"\x00\x01"
    header[6:8] = b"\x00\x01"
    answer = b"\xc0\x0c\x00\x01\x00\x01\x00\x00\x00\x3c\x00\x04" + REMOTE_IP
    return bytes(header) + request_payload[12:question_end] + answer


def _mac_str(mac):
    return ":".join("%02x" % byte for byte in mac)


class SharedEthernetHub(object):
    """Hub stream multi-clients + pair DHCP/ARP/DNS local (127.0.0.1)."""

    def __init__(self, respond=True):
        self.respond = respond
        self.events = {
            "discover": 0,
            "offer": 0,
            "request": 0,
            "ack": 0,
            "arp": 0,
            "dns": 0,
            "frames": 0,
        }
        self.source_macs = set()
        self.error = None
        self._clients = []
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._listener.bind(("127.0.0.1", 0))
        self._listener.listen(8)
        self._listener.settimeout(0.2)
        self.port = self._listener.getsockname()[1]
        self._thread = threading.Thread(target=self._accept_loop, daemon=True)

    def start(self):
        self._thread.start()
        return self.port

    def close(self):
        self._stop.set()
        try:
            self._listener.close()
        except OSError:
            pass
        with self._lock:
            clients = list(self._clients)
            self._clients = []
        for connection in clients:
            try:
                connection.close()
            except OSError:
                pass
        self._thread.join(timeout=2)

    @property
    def client_count(self):
        with self._lock:
            return len(self._clients)

    @staticmethod
    def _read_exact(connection, length):
        chunks = []
        remaining = length
        while remaining:
            try:
                chunk = connection.recv(remaining)
            except socket.timeout:
                if not chunks:
                    return b""  # idle: no bytes yet
                # Partial frame: keep waiting for the rest.
                continue
            if not chunk:
                return None
            chunks.append(chunk)
            remaining -= len(chunk)
        return b"".join(chunks)

    @staticmethod
    def _send_frame(connection, frame):
        connection.sendall(struct.pack("!I", len(frame)) + frame)

    def _flood(self, source, frame):
        with self._lock:
            clients = list(self._clients)
        for connection in clients:
            if connection is source:
                continue
            try:
                self._send_frame(connection, frame)
            except OSError:
                pass

    def _reply(self, source, frame):
        """Envoie une réponse au client source et l'inonde aux autres (L2)."""
        try:
            self._send_frame(source, frame)
        except OSError:
            return
        self._flood(source, frame)

    def _handle_frame(self, connection, frame):
        self.events["frames"] += 1
        if len(frame) < 14:
            return
        source_mac = frame[6:12]
        self.source_macs.add(_mac_str(source_mac))
        # Toujours inonder les autres invités (segment partagé).
        self._flood(connection, frame)
        if not self.respond:
            return
        ethertype = struct.unpack("!H", frame[12:14])[0]
        if ethertype == 0x0806 and len(frame) >= 42:
            if frame[20:22] == b"\x00\x01" and frame[38:42] == SERVER_IP:
                self.events["arp"] += 1
                self._reply(connection, _arp_reply(frame))
            return
        if ethertype != 0x0800 or len(frame) < 42:
            return
        ip_offset = 14
        if (frame[ip_offset] >> 4) != 4:
            return
        header_length = (frame[ip_offset] & 0x0f) * 4
        total_length = struct.unpack("!H", frame[ip_offset + 2 : ip_offset + 4])[0]
        if header_length < 20 or total_length < header_length or len(frame) < ip_offset + total_length:
            return
        protocol = frame[ip_offset + 9]
        if protocol != 17:
            return
        udp_offset = ip_offset + header_length
        if len(frame) < udp_offset + 8:
            return
        source_port, destination_port, udp_length, _ = struct.unpack(
            "!HHHH", frame[udp_offset : udp_offset + 8]
        )
        if udp_length < 8 or len(frame) < udp_offset + udp_length:
            return
        payload = frame[udp_offset + 8 : udp_offset + udp_length]
        if source_port == 68 and destination_port == 67 and len(payload) >= 244:
            kind = _dhcp_type(payload)
            if kind == 1:
                self.events["discover"] += 1
                self._reply(connection, _dhcp_reply(payload, 2))
                self.events["offer"] += 1
            elif kind == 3:
                self.events["request"] += 1
                self._reply(connection, _dhcp_reply(payload, 5))
                self.events["ack"] += 1
            return
        if source_port == 49152 and destination_port == 53:
            response = _dns_reply(payload)
            if response is not None:
                guest_mac = frame[6:12]
                guest_ip = frame[ip_offset + 12 : ip_offset + 16]
                reply = _ethernet(
                    guest_mac,
                    0x0800,
                    _ipv4_udp(SERVER_IP, guest_ip, 53, source_port, response),
                )
                self.events["dns"] += 1
                self._reply(connection, reply)

    def _serve_client(self, connection):
        try:
            while not self._stop.is_set():
                header = self._read_exact(connection, 4)
                if header is None:
                    break
                if header == b"":
                    continue  # idle timeout, keep client
                (length,) = struct.unpack("!I", header)
                if length == 0 or length > 65535:
                    break
                frame = None
                while not self._stop.is_set():
                    frame = self._read_exact(connection, length)
                    if frame is None:
                        break
                    if frame == b"":
                        continue  # still waiting for body
                    break
                if frame is None or frame == b"" or len(frame) != length:
                    break
                try:
                    self._handle_frame(connection, frame)
                except Exception as exc:  # noqa: BLE001 — publier puis continuer
                    self.error = exc
        finally:
            with self._lock:
                if connection in self._clients:
                    self._clients.remove(connection)
            try:
                connection.close()
            except OSError:
                pass

    def _accept_loop(self):
        while not self._stop.is_set():
            try:
                connection, _address = self._listener.accept()
            except socket.timeout:
                continue
            except OSError:
                break
            connection.settimeout(1.0)
            with self._lock:
                self._clients.append(connection)
            threading.Thread(
                target=self._serve_client, args=(connection,), daemon=True
            ).start()
