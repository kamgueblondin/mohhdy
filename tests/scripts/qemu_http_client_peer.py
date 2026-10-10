"""Host-side HTTP/1.0 and HTTPS client over the controlled NE2000 peer.

The peer (DHCP, DNS, ARP as in qemu_ne2k_controlled_peer) also opens plain
TCP connections to a guest port, several at once, and runs HTTP or TLS 1.2
(Python ssl over memory BIOs) on them, frame by frame, so the guest web
server is tested without TAP and without any internet.
"""
import ssl
import struct
import threading
import time

from qemu_ne2k_controlled_peer import (ControlledEthernetPeer, SERVER_IP, GUEST_IP,
                                       _ethernet, _ipv4_tcp)

SYN, ACK, PSH, FIN = 0x02, 0x10, 0x08, 0x01


class Conn:
    def __init__(self, peer, guest_port, client_port):
        self.peer = peer
        self.guest_port = guest_port
        self.port = client_port
        self.c_seq = 0x1000 * client_port & 0xffffffff
        self.g_seq = 0
        self.synack = threading.Event()
        self.fin = threading.Event()
        self.cv = threading.Condition()
        self.rx = b""
        self.last = None
        self.retransmits = 0

    def _send(self, flags, payload=b""):
        frame = _ethernet(self.peer.mac, 0x0800, _ipv4_tcp(SERVER_IP, GUEST_IP, self.port, self.guest_port,
                                                            self.c_seq, self.g_seq, flags, payload))
        with self.peer.lock:
            self.peer._send_frame(self.peer.conn, frame)

    def on_segment(self, seq, flags, payload):
        if flags & SYN and flags & ACK:
            self.g_seq = (seq + 1) & 0xffffffff
            self.synack.set()
            return
        with self.cv:
            if payload and seq == self.g_seq:
                self.rx += payload
                self.g_seq = (self.g_seq + len(payload)) & 0xffffffff
            if flags & FIN and seq + len(payload) == self.g_seq and not self.fin.is_set():
                self.g_seq = (self.g_seq + 1) & 0xffffffff
                self.fin.set()
            self.cv.notify_all()
        if payload or flags & FIN:
            self._send(ACK)

    def open(self, timeout=60.0):
        deadline = time.monotonic() + timeout
        while not self.synack.is_set():
            if time.monotonic() > deadline:
                raise RuntimeError("guest never answered the SYN on port %d" % self.guest_port)
            self._send(SYN)
            self.synack.wait(1.0)
        self.c_seq = (self.c_seq + 1) & 0xffffffff
        self._send(ACK)
        return self

    def retransmit(self):
        """Resend the last data with its original sequence (NE2000 frames
        can be lost on the guest side; TCP there drops duplicates)."""
        if self.last is None:
            return
        seq, data = self.last
        frame = _ethernet(self.peer.mac, 0x0800, _ipv4_tcp(SERVER_IP, GUEST_IP, self.port, self.guest_port,
                                                            seq, self.g_seq, ACK | PSH, data))
        with self.peer.lock:
            self.peer._send_frame(self.peer.conn, frame)
        self.retransmits += 1

    def send(self, data):
        if data:
            self.last = (self.c_seq, data[:1200])
        for off in range(0, len(data), 1200):
            chunk = data[off:off + 1200]
            self._send(ACK | PSH, chunk)
            self.c_seq = (self.c_seq + len(chunk)) & 0xffffffff
            time.sleep(0.02)

    def take(self, timeout):
        """Bytes received so far (waits for at least one byte or FIN)."""
        with self.cv:
            if not self.rx and not self.fin.is_set():
                self.cv.wait(timeout)
            data, self.rx = self.rx, b""
            return data

    def read_all(self, timeout=60.0):
        deadline = time.monotonic() + timeout
        out = b""
        while not self.fin.is_set() or self.rx:
            if time.monotonic() > deadline:
                raise RuntimeError("no complete response; got %r" % out[:200])
            out += self.take(0.5)
        self._send(ACK | FIN)
        return out


class TlsConn:
    """TLS 1.2 client (ECDHE-RSA-AES128-GCM-SHA256, test certificate not
    verified: the guest serves the repository test leaf)."""

    def __init__(self, conn):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        ctx.minimum_version = ssl.TLSVersion.TLSv1_2
        ctx.maximum_version = ssl.TLSVersion.TLSv1_2
        ctx.set_ciphers("ECDHE-RSA-AES128-GCM-SHA256:@SECLEVEL=0")
        ctx.options |= getattr(ssl, "OP_LEGACY_SERVER_CONNECT", 0x4)
        ctx.options |= ssl.OP_NO_TICKET
        self.conn = conn
        self.inc = ssl.MemoryBIO()
        self.out = ssl.MemoryBIO()
        self.obj = ctx.wrap_bio(self.inc, self.out, server_side=False)
        self.rx_total = 0

    def _flush(self):
        data = self.out.read()
        if data:
            self.conn.send(data)

    def _flush_mark(self):
        data = self.out.read()
        if data:
            self.conn.send(data)
            self.sent_at = time.monotonic()

    def handshake(self, timeout=120.0):
        deadline = time.monotonic() + timeout
        self.sent_at = None
        while True:
            try:
                self.obj.do_handshake()
                self._flush()
                return self
            except ssl.SSLWantReadError:
                self._flush_mark()
            if self.sent_at is not None and time.monotonic() - self.sent_at > 8.0:
                self.conn.retransmit()
                self.sent_at = time.monotonic()
            if time.monotonic() > deadline:
                raise RuntimeError("TLS handshake timeout (received %d bytes)" % self.rx_total)
            data = self.conn.take(0.5)
            if data:
                self.rx_total += len(data)
                self.sent_at = None
                self.inc.write(data)
            elif self.conn.fin.is_set():
                raise RuntimeError("guest closed during TLS handshake (port %d)" % self.conn.port)

    def send(self, data):
        self.obj.write(data)
        self._flush()

    def read_all(self, timeout=60.0):
        deadline = time.monotonic() + timeout
        out = b""
        while True:
            try:
                chunk = self.obj.read(4096)
                if not chunk:
                    break
                out += chunk
                continue
            except ssl.SSLWantReadError:
                pass
            except ssl.SSLZeroReturnError:
                break
            if self.conn.fin.is_set() and not self.conn.rx:
                break
            if time.monotonic() > deadline:
                raise RuntimeError("no complete TLS response; got %r" % out[:200])
            data = self.conn.take(0.5)
            if data:
                self.inc.write(data)
        self.conn._send(ACK | FIN)
        return out


class HttpClientPeer(ControlledEthernetPeer):
    def __init__(self):
        ControlledEthernetPeer.__init__(self, full_tls=False)
        self.conn = None
        self.mac = None
        self.lock = threading.Lock()
        self.next_port = 40000
        self.conns = {}

    def _handle_frame(self, connection, frame):
        self.conn = connection
        if len(frame) >= 54 and struct.unpack("!H", frame[12:14])[0] == 0x0800 and frame[23] == 6:
            ihl = (frame[14] & 0x0f) * 4
            t = 14 + ihl
            sport, dport = struct.unpack("!HH", frame[t:t + 4])
            c = self.conns.get(dport)
            if c is not None and sport == c.guest_port:
                self.mac = frame[6:12]
                seq = struct.unpack("!I", frame[t + 4:t + 8])[0]
                total = struct.unpack("!H", frame[16:18])[0]
                payload = frame[t + (frame[t + 12] >> 4) * 4:14 + total]
                c.on_segment(seq, frame[t + 13], payload)
                return
        if len(frame) >= 14 and self.mac is None and frame[12:14] == b"\x08\x00":
            self.mac = frame[6:12]
        ControlledEthernetPeer._handle_frame(self, connection, frame)

    def connect(self, guest_port, timeout=60.0):
        deadline = time.monotonic() + timeout
        while self.conn is None or self.mac is None:
            if time.monotonic() > deadline:
                raise RuntimeError("guest never talked to the peer")
            time.sleep(0.1)
        self.next_port += 1
        c = Conn(self, guest_port, self.next_port)
        self.conns[c.port] = c
        return c.open(max(1.0, deadline - time.monotonic()))

    def request(self, raw, guest_port=8080, timeout=60.0):
        c = self.connect(guest_port, timeout)
        c.send(raw)
        return c.read_all(timeout)

    def tls_request(self, raw, guest_port=8443, timeout=120.0):
        t = TlsConn(self.connect(guest_port, timeout)).handshake(timeout)
        t.send(raw)
        return t.read_all(timeout)
