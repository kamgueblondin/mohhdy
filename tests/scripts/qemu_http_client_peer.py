"""Host-side HTTP/1.0 client over the controlled NE2000 Ethernet peer.

The peer (DHCP, DNS, ARP as in qemu_ne2k_controlled_peer) also opens plain
TCP connections to a guest port and exchanges HTTP, frame by frame, so the
guest web server can be tested without TAP and without any internet.
"""
import struct
import threading
import time

from qemu_ne2k_controlled_peer import (ControlledEthernetPeer, SERVER_IP, GUEST_IP,
                                       _ethernet, _ipv4_tcp)

SYN, ACK, PSH, FIN = 0x02, 0x10, 0x08, 0x01


class HttpClientPeer(ControlledEthernetPeer):
    def __init__(self, guest_port=8080):
        ControlledEthernetPeer.__init__(self, full_tls=False)
        self.web_port = guest_port
        self.conn = None
        self.mac = None
        self.lock = threading.Lock()
        self.client_port = 40000
        self._reset()

    def _reset(self):
        self.synack = threading.Event()
        self.done = threading.Event()
        self.data = b""
        self.g_seq = 0
        self.c_seq = 0

    def _send(self, flags, payload=b""):
        frame = _ethernet(self.mac, 0x0800, _ipv4_tcp(SERVER_IP, GUEST_IP, self.client_port,
                                                       self.web_port, self.c_seq, self.g_seq,
                                                       flags, payload))
        with self.lock:
            self._send_frame(self.conn, frame)

    def _handle_frame(self, connection, frame):
        self.conn = connection
        if len(frame) >= 54 and struct.unpack("!H", frame[12:14])[0] == 0x0800 and frame[23] == 6:
            ihl = (frame[14] & 0x0f) * 4
            t = 14 + ihl
            sport, dport = struct.unpack("!HH", frame[t:t + 4])
            if sport == self.web_port and dport == self.client_port:
                self.mac = frame[6:12]
                seq, ack = struct.unpack("!II", frame[t + 4:t + 12])
                flags = frame[t + 13]
                total = struct.unpack("!H", frame[16:18])[0]
                payload = frame[t + (frame[t + 12] >> 4) * 4:14 + total]
                if flags & SYN and flags & ACK:
                    self.g_seq = (seq + 1) & 0xffffffff
                    self.synack.set()
                    return
                if payload and seq == self.g_seq:
                    self.data += payload
                    self.g_seq = (self.g_seq + len(payload)) & 0xffffffff
                if flags & FIN and seq + len(payload) == self.g_seq - (0 if not payload else 0):
                    self.g_seq = (self.g_seq + 1) & 0xffffffff
                    self._send(ACK | FIN)
                    self.c_seq = (self.c_seq + 1) & 0xffffffff
                    self.done.set()
                    return
                if payload:
                    self._send(ACK)
                return
        if len(frame) >= 12 and self.mac is None and frame[12:14] == b"\x08\x00":
            self.mac = frame[6:12]
        ControlledEthernetPeer._handle_frame(self, connection, frame)

    def request(self, raw, timeout=60.0):
        """One HTTP exchange; returns the raw response bytes."""
        self._reset()
        self.client_port += 1
        self.c_seq = 0x1000 * self.client_port
        deadline = time.monotonic() + timeout
        while not self.synack.is_set():
            if time.monotonic() > deadline:
                raise RuntimeError("guest never answered the SYN on port %d" % self.web_port)
            if self.conn is not None and self.mac is not None:
                self._send(SYN)
            self.synack.wait(1.0)
        self.c_seq = (self.c_seq + 1) & 0xffffffff
        self._send(ACK)
        time.sleep(0.2)
        self._send(ACK | PSH, raw)
        self.c_seq = (self.c_seq + len(raw)) & 0xffffffff
        if not self.done.wait(max(1.0, deadline - time.monotonic())):
            raise RuntimeError("no complete HTTP response; got %r" % self.data[:200])
        return self.data
