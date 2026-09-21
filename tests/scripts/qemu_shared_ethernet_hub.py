#!/usr/bin/env python3
"""Hub L2 QEMU socket pour topologie Ethernet locale partagée (tranche 2).

Plusieurs invités `-netdev socket,connect=127.0.0.1:PORT` partagent le même
segment. Chaque trame Ethernet est préfixée d'une longueur big-endian 32 bits
(protocole stream QEMU). Le hub inonde les autres clients et peut répondre
DHCP/ARP/DNS localement (pair contrôlé, sans TAP ni Internet).

Extensions guest↔guest : ARP proxy pour les IPs louées, DNS `peer.local` vers
le pair, et SYN-ACK minimal vers l'IP du pair (flux applicatif simple).
"""
from __future__ import print_function

import socket
import struct
import threading

from qemu_ne2k_tls12_server import LocalTls12Server

SERVER_MAC = b"\x52\x54\x00\xa0\x20\x02"
SERVER_IP = b"\x0a\x20\x00\x02"  # 10.32.0.2
GUEST_IP = b"\x0a\x20\x00\x0f"  # 10.32.0.15 — first lease
GUEST_IP_B = b"\x0a\x20\x00\x10"  # 10.32.0.16 — second lease
NETMASK = b"\xff\xff\xff\x00"
REMOTE_IP = b"\xcb\x00\x71\x14"  # 203.0.113.20 TEST-NET-3
MAX_TCP_PAYLOAD = 256


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


def _ethernet(destination_mac, ethertype, payload, source_mac=None):
    if source_mac is None:
        source_mac = SERVER_MAC
    return destination_mac + source_mac + struct.pack("!H", ethertype) + payload


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


def _dhcp_reply(request, message_type, guest_ip):
    xid = request[4:8]
    client_mac = request[28:34]
    payload = bytearray(278)
    payload[0:4] = b"\x02\x01\x06\x00"
    payload[4:8] = xid
    payload[16:20] = guest_ip
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


def _ipv4_tcp(source_ip, destination_ip, source_port, destination_port, sequence,
              acknowledgment, flags, payload=b""):
    tcp = bytearray(20 + len(payload))
    tcp[0:4] = struct.pack("!HH", source_port, destination_port)
    tcp[4:12] = struct.pack("!II", sequence, acknowledgment)
    tcp[12] = 0x50
    tcp[13] = flags
    tcp[14:16] = b"\xff\xff"
    tcp[20:] = payload
    pseudo_header = source_ip + destination_ip + b"\x00\x06" + struct.pack("!H", len(tcp))
    tcp[16:18] = struct.pack("!H", _checksum(pseudo_header + bytes(tcp)))
    return _ipv4_packet(source_ip, destination_ip, 6, bytes(tcp))


class _TlsSession(object):
    """Etat TLS/HTTP local pour un seul invité du hub (pair controle)."""

    def __init__(self):
        self.tls = LocalTls12Server()
        self.tls_step = 0
        self.last_sent_end = 0
        self.last_payload = b""
        self.last_payload_sequence = 0
        self.pending_advance = False
        self.deferred_ack = None
        self.server_sequence = 0x10203041
        self.complete = False

    @staticmethod
    def _tls_handshake_type(payload):
        if len(payload) < 6 or payload[:3] != b"\x16\x03\x03":
            return None
        return payload[5]


def _arp_reply(request, target_ip=None, reply_mac=None):
    sender_mac = request[22:28]
    sender_ip = request[28:32]
    if target_ip is None:
        target_ip = SERVER_IP
    if reply_mac is None:
        reply_mac = SERVER_MAC
    payload = (
        b"\x00\x01\x08\x00\x06\x04\x00\x02"
        + reply_mac
        + target_ip
        + sender_mac
        + sender_ip
    )
    return _ethernet(sender_mac, 0x0806, payload, source_mac=reply_mac)


def _dns_qname(request_payload):
    if len(request_payload) < 13:
        return ""
    labels = []
    position = 12
    while position < len(request_payload) and request_payload[position] != 0:
        length = request_payload[position]
        position += 1
        if length > 63 or position + length > len(request_payload):
            return ""
        labels.append(request_payload[position:position + length].decode("ascii", "ignore"))
        position += length
    return ".".join(labels).lower()


def _dns_reply(request_payload, answer_ip=None):
    if len(request_payload) < 17:
        return None
    if answer_ip is None:
        answer_ip = REMOTE_IP
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
    answer = b"\xc0\x0c\x00\x01\x00\x01\x00\x00\x00\x3c\x00\x04" + answer_ip
    return bytes(header) + request_payload[12:question_end] + answer


def _mac_str(mac):
    return ":".join("%02x" % byte for byte in mac)


class SharedEthernetHub(object):
    """Hub stream multi-clients + pair DHCP/ARP/DNS local (127.0.0.1)."""

    def __init__(self, respond=True, full_tls=False):
        self.respond = respond
        self.full_tls = full_tls
        self.events = {
            "discover": 0,
            "offer": 0,
            "request": 0,
            "ack": 0,
            "arp": 0,
            "dns": 0,
            "frames": 0,
            "syn": 0,
            "syn_ack": 0,
            "client_hello": 0,
            "server_finished": 0,
            "http_response": 0,
            "tls_complete_sessions": 0,
            "cross_arp": 0,
            "cross_arp_reply": 0,
            "cross_syn": 0,
            "cross_syn_ack": 0,
            "peer_dns": 0,
        }
        self.source_macs = set()
        self.leases = {}  # mac_str -> guest_ip bytes
        self.ip_owners = {}  # guest_ip bytes -> mac bytes
        self.error = None
        self._clients = []
        self._sessions = {}  # connection -> _TlsSession
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._listener.bind(("127.0.0.1", 0))
        self._listener.listen(8)
        self._listener.settimeout(0.2)
        self.port = self._listener.getsockname()[1]
        self._thread = threading.Thread(target=self._accept_loop, daemon=True)

    def _lease_for_mac(self, client_mac):
        key = _mac_str(client_mac)
        with self._lock:
            if key in self.leases:
                return self.leases[key]
            guest_ip = GUEST_IP if len(self.leases) == 0 else GUEST_IP_B
            if len(self.leases) >= 2:
                # Reuse second lease pool slot for any further MAC (test uses 2).
                guest_ip = GUEST_IP_B
            self.leases[key] = guest_ip
            self.ip_owners[guest_ip] = bytes(client_mac)
            return guest_ip

    def _peer_ip_for(self, guest_ip):
        with self._lock:
            owners = list(self.ip_owners.keys())
        for candidate in owners:
            if candidate != guest_ip:
                return candidate
        return None

    def _mac_for_ip(self, guest_ip):
        with self._lock:
            return self.ip_owners.get(guest_ip)

    def _session(self, connection):
        with self._lock:
            session = self._sessions.get(connection)
            if session is None:
                session = _TlsSession()
                self._sessions[connection] = session
            return session

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
        """Envoie une reponse au seul client source (pas d'inondation).

        Les trames emises par les invites restent inondees via `_flood`. Les
        reponses de controle (DHCP/DNS/ARP/TCP) restent unicast-hub→client pour
        eviter que deux invites au meme xid DHCP ne consomment le bail du pair.
        """
        try:
            self._send_frame(source, frame)
        except OSError:
            return

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
            # Pair local : repondre pour la passerelle et pour l'IP TLS (meme MAC).
            if frame[20:22] == b"\x00\x01":
                target_ip = frame[38:42]
                if target_ip in (SERVER_IP, REMOTE_IP):
                    self.events["arp"] += 1
                    self._reply(connection, _arp_reply(frame, target_ip))
                else:
                    owner_mac = self._mac_for_ip(target_ip)
                    if owner_mac is not None and owner_mac != source_mac:
                        # ARP croise guest↔guest : proxy avec la MAC du titulaire du bail.
                        self.events["cross_arp"] += 1
                        self._reply(
                            connection,
                            _arp_reply(frame, target_ip, reply_mac=owner_mac),
                        )
                        self.events["cross_arp_reply"] += 1
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
        ip_end = ip_offset + total_length
        protocol = frame[ip_offset + 9]
        source_ip = frame[ip_offset + 12 : ip_offset + 16]
        dest_ip = frame[ip_offset + 16 : ip_offset + 20]
        if protocol == 6:
            tcp_offset = ip_offset + header_length
            if len(frame) < tcp_offset + 20:
                return
            source_port, destination_port = struct.unpack("!HH", frame[tcp_offset:tcp_offset + 4])
            sequence = struct.unpack("!I", frame[tcp_offset + 4:tcp_offset + 8])[0]
            flags = frame[tcp_offset + 13]
            tcp_header_length = (frame[tcp_offset + 12] >> 4) * 4
            if tcp_header_length < 20 or ip_end < tcp_offset + tcp_header_length:
                return
            payload = frame[tcp_offset + tcp_header_length:ip_end]
            peer_mac = self._mac_for_ip(dest_ip)
            # Flux applicatif simple : SYN vers l'IP louee du pair → SYN-ACK proxy.
            if (
                peer_mac is not None
                and dest_ip != source_ip
                and destination_port == 443
                and (flags & 0x12) == 0x02
            ):
                self.events["cross_syn"] += 1
                syn_ack = _ethernet(
                    source_mac,
                    0x0800,
                    _ipv4_tcp(
                        dest_ip, source_ip, destination_port, source_port,
                        0x10203040, sequence + 1, 0x12,
                    ),
                    source_mac=peer_mac,
                )
                self._reply(connection, syn_ack)
                self.events["cross_syn_ack"] += 1
                return
            if self.full_tls and source_port == 49152 and destination_port == 443:
                acknowledgment = struct.unpack("!I", frame[tcp_offset + 8:tcp_offset + 12])[0]
                self._handle_tcp_443(
                    connection, source_mac, source_ip, source_port,
                    sequence, acknowledgment, flags, payload,
                )
            return
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
            guest_ip = self._lease_for_mac(payload[28:34])
            if kind == 1:
                self.events["discover"] += 1
                self._reply(connection, _dhcp_reply(payload, 2, guest_ip))
                self.events["offer"] += 1
            elif kind == 3:
                self.events["request"] += 1
                self._reply(connection, _dhcp_reply(payload, 5, guest_ip))
                self.events["ack"] += 1
            return
        if source_port == 49152 and destination_port == 53:
            guest_mac = frame[6:12]
            guest_ip = frame[ip_offset + 12 : ip_offset + 16]
            qname = _dns_qname(payload)
            answer_ip = REMOTE_IP
            if qname == "peer.local":
                peer_ip = self._peer_ip_for(guest_ip)
                if peer_ip is None:
                    return
                answer_ip = peer_ip
                self.events["peer_dns"] += 1
            response = _dns_reply(payload, answer_ip)
            if response is not None:
                reply = _ethernet(
                    guest_mac,
                    0x0800,
                    _ipv4_udp(SERVER_IP, guest_ip, 53, source_port, response),
                )
                self.events["dns"] += 1
                self._reply(connection, reply)

    def _send_tcp_chunk(self, connection, dest_mac, dest_ip, dest_port, sequence,
                        guest_ack, payload, flags=0x18):
        frame = _ethernet(
            dest_mac, 0x0800,
            _ipv4_tcp(REMOTE_IP, dest_ip, 443, dest_port, sequence, guest_ack, flags, payload),
        )
        self._reply(connection, frame)

    def _send_tcp(self, connection, session, dest_mac, dest_ip, dest_port, guest_ack,
                  payload, flags=0x18):
        if (not session.last_payload or
                session.last_payload_sequence + len(session.last_payload) != session.server_sequence):
            session.last_payload = b""
            session.last_payload_sequence = session.server_sequence
        session.last_payload += payload
        if not payload:
            self._send_tcp_chunk(connection, dest_mac, dest_ip, dest_port,
                                 session.server_sequence, guest_ack, b"", flags)
        offset = 0
        while offset < len(payload):
            chunk = payload[offset:offset + MAX_TCP_PAYLOAD]
            self._send_tcp_chunk(connection, dest_mac, dest_ip, dest_port,
                                 session.server_sequence, guest_ack, chunk, flags)
            session.server_sequence += len(chunk)
            offset += len(chunk)
        session.last_sent_end = session.server_sequence
        session.pending_advance = True

    def _retransmit_tcp(self, connection, session, dest_mac, dest_ip, dest_port,
                        guest_ack, acknowledgment=None):
        if not session.last_payload:
            return
        if acknowledgment is None or acknowledgment < session.last_payload_sequence:
            offset = 0
            sequence = session.last_payload_sequence
        else:
            offset = acknowledgment - session.last_payload_sequence
            sequence = acknowledgment
        remaining = session.last_payload[offset:]
        while remaining:
            chunk = remaining[:MAX_TCP_PAYLOAD]
            remaining = remaining[MAX_TCP_PAYLOAD:]
            self._send_tcp_chunk(connection, dest_mac, dest_ip, dest_port,
                                 sequence, guest_ack, chunk)
            sequence += len(chunk)

    def _handle_tcp_443(self, connection, dest_mac, dest_ip, dest_port, sequence,
                        acknowledgment, flags, payload):
        session = self._session(connection)
        if (flags & 0x12) == 0x02:
            self.events["syn"] += 1
            self._reply(
                connection,
                _ethernet(
                    dest_mac, 0x0800,
                    _ipv4_tcp(REMOTE_IP, dest_ip, 443, dest_port, 0x10203040,
                              sequence + 1, 0x12),
                ),
            )
            self.events["syn_ack"] += 1
            return
        kind = _TlsSession._tls_handshake_type(payload)
        if kind == 1 and session.tls_step == 0:
            self.events["client_hello"] += 1
            guest_ack = sequence + len(payload)
            session.tls.note_client_hello(payload)
            self._send_tcp(connection, session, dest_mac, dest_ip, dest_port, guest_ack,
                           session.tls.server_hello_record())
            session.tls_step = 1
            return
        if kind == 1 and 1 <= session.tls_step < 4:
            self._retransmit_tcp(connection, session, dest_mac, dest_ip, dest_port,
                                 sequence + len(payload), acknowledgment)
            return
        if payload and session.tls_step == 4:
            session.tls.accept_client_flight(payload)
            self._send_tcp(connection, session, dest_mac, dest_ip, dest_port,
                           sequence + len(payload), session.tls.change_cipher_spec_record())
            session.tls_step = 5
            return
        if payload and session.tls.ready and session.tls_step >= 6 and not session.complete:
            request = session.tls.open_application(payload)
            if b"POST" not in request:
                raise RuntimeError("HTTP POST local attendu")
            self._send_tcp(connection, session, dest_mac, dest_ip, dest_port,
                           sequence + len(payload), session.tls.http_ok_record())
            self.events["http_response"] += 1
            session.tls_step = 7
            session.complete = True
            self.events["tls_complete_sessions"] += 1
            return
        if (flags & 0x10) and not payload:
            if session.tls_step >= 1:
                session.deferred_ack = (dest_mac, dest_ip, dest_port, sequence, acknowledgment)

    def _advance_tls_on_ack(self, connection, session, dest_mac, dest_ip, dest_port,
                            sequence, acknowledgment):
        if not session.pending_advance or acknowledgment < session.last_sent_end:
            return
        session.pending_advance = False
        guest_ack = sequence
        if session.tls_step == 1:
            self._send_tcp(connection, session, dest_mac, dest_ip, dest_port, guest_ack,
                           session.tls.certificate_record())
            session.tls_step = 2
        elif session.tls_step == 2:
            self._send_tcp(connection, session, dest_mac, dest_ip, dest_port, guest_ack,
                           session.tls.server_key_exchange_record())
            session.tls_step = 3
        elif session.tls_step == 3:
            self._send_tcp(connection, session, dest_mac, dest_ip, dest_port, guest_ack,
                           session.tls.server_hello_done_record())
            session.tls_step = 4
        elif session.tls_step == 5:
            self._send_tcp(connection, session, dest_mac, dest_ip, dest_port, guest_ack,
                           session.tls.finished_record())
            session.tls_step = 6
            self.events["server_finished"] += 1
            if not session.complete:
                session.complete = True
                self.events["tls_complete_sessions"] += 1

    def _serve_client(self, connection):
        try:
            while not self._stop.is_set():
                header = self._read_exact(connection, 4)
                if header is None:
                    break
                if header == b"":
                    # Idle: advance deferred TLS ACK for this client.
                    session = self._sessions.get(connection)
                    if session is not None and session.deferred_ack is not None:
                        dest_mac, dest_ip, dest_port, sequence, acknowledgment = session.deferred_ack
                        session.deferred_ack = None
                        try:
                            self._advance_tls_on_ack(
                                connection, session, dest_mac, dest_ip, dest_port,
                                sequence, acknowledgment,
                            )
                        except Exception as exc:  # noqa: BLE001
                            self.error = exc
                    continue
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
                self._sessions.pop(connection, None)
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
