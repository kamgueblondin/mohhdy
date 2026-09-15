#!/usr/bin/env python3
"""Serveur TLS 1.2 local pour le pair QEMU (ECDHE_RSA_AES_128_GCM_SHA256).

Le guest assemble un record TLS sur plusieurs segments TCP, mais refuse deux
records complets dans le meme segment. Ce module construit les messages
handshake et les records AES-GCM compatibles avec kernel/net_tls_record.c.
La cle et les certificats sont du materiel de test pour example.com.
"""
import hashlib
import hmac
import struct

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey, X25519PublicKey
from cryptography.hazmat.primitives.ciphers.aead import AESGCM

from tls_test_material import LEAF_DER, LEAF_KEY_PEM

TLS_VERSION = b"\x03\x03"
CONTENT_CHANGE_CIPHER_SPEC = 20
CONTENT_ALERT = 21
CONTENT_HANDSHAKE = 22
CONTENT_APPLICATION_DATA = 23
HANDSHAKE_SERVER_HELLO = 2
HANDSHAKE_CERTIFICATE = 11
HANDSHAKE_SERVER_KEY_EXCHANGE = 12
HANDSHAKE_SERVER_HELLO_DONE = 14
HANDSHAKE_CLIENT_KEY_EXCHANGE = 16
HANDSHAKE_FINISHED = 20
CIPHER_ECDHE_RSA_AES128_GCM = b"\xc0\x2f"
NAMED_CURVE_X25519 = 29
HTTP_JSON_OK = b'{"response":"ok"}'
HTTP_REPLY = (
    b"HTTP/1.1 200 OK\r\n"
    b"Content-Length: 17\r\n"
    b"Connection: close\r\n"
    b"\r\n" + HTTP_JSON_OK
)
SSE_EVENT = b'data: {"response":"ok"}\n\n'
SSE_DONE = b"data: [DONE]\n\n"
SSE_HEADERS = (
    b"HTTP/1.1 200 OK\r\n"
    b"Transfer-Encoding: chunked\r\n"
    b"\r\n"
)


def _http_chunk(payload):
    return b"%x\r\n" % len(payload) + payload + b"\r\n"


def _u24(value):
    return struct.pack("!I", value)[1:]


def _handshake(kind, body):
    return bytes((kind,)) + _u24(len(body)) + body


def _record(content_type, payload):
    return bytes((content_type,)) + TLS_VERSION + struct.pack("!H", len(payload)) + payload


def _prf_sha256(secret, label, seed, output_length):
    seed = label + seed
    a = hmac.new(secret, seed, hashlib.sha256).digest()
    output = bytearray()
    while len(output) < output_length:
        output.extend(hmac.new(secret, a + seed, hashlib.sha256).digest())
        a = hmac.new(secret, a, hashlib.sha256).digest()
    return bytes(output[:output_length])


def _aes_gcm_seal(key, fixed_iv, sequence, content_type, plaintext):
    explicit = struct.pack("!Q", sequence)
    additional = explicit + bytes((content_type,)) + TLS_VERSION + struct.pack("!H", len(plaintext))
    sealed = AESGCM(key).encrypt(fixed_iv + explicit, plaintext, additional)
    payload = explicit + sealed
    return _record(content_type, payload)


def _aes_gcm_open(key, fixed_iv, sequence, record):
    if len(record) < 5 + 24:
        raise ValueError("short TLS GCM record")
    content_type = record[0]
    payload_length = struct.unpack("!H", record[3:5])[0]
    payload = record[5:5 + payload_length]
    if len(payload) != payload_length or payload_length < 24:
        raise ValueError("truncated TLS GCM payload")
    explicit, ciphertext = payload[:8], payload[8:]
    plaintext_length = payload_length - 24
    additional = struct.pack("!Q", sequence) + bytes((content_type,)) + TLS_VERSION + struct.pack("!H", plaintext_length)
    return content_type, AESGCM(key).decrypt(fixed_iv + explicit, ciphertext, additional)


def iter_tls_records(stream):
    offset = 0
    while offset + 5 <= len(stream):
        payload_length = struct.unpack("!H", stream[offset + 3:offset + 5])[0]
        total = 5 + payload_length
        if offset + total > len(stream):
            raise ValueError("incomplete TLS record")
        yield stream[offset:offset + total]
        offset += total
    if offset != len(stream):
        raise ValueError("trailing TLS bytes")


class LocalTls12Server:
    """Automate TLS 1.2 serveur borne, sans reseau."""

    def __init__(self, leaf_der=LEAF_DER, leaf_key_pem=LEAF_KEY_PEM):
        self.leaf_der = leaf_der
        self.leaf_key = serialization.load_pem_private_key(leaf_key_pem.encode("ascii"), password=None)
        self.server_random = bytes(range(32))
        self.client_random = b""
        self.transcript = bytearray()
        self.x25519 = X25519PrivateKey.generate()
        self.master_secret = b""
        self.client_write_key = b""
        self.server_write_key = b""
        self.client_fixed_iv = b""
        self.server_fixed_iv = b""
        self.server_write_sequence = 0
        self.client_read_sequence = 0
        self.ready = False

    def note_client_hello(self, payload):
        if len(payload) < 5 or payload[0] != CONTENT_HANDSHAKE:
            raise ValueError("ClientHello record attendu")
        handshake_length = struct.unpack("!H", payload[3:5])[0]
        handshake = payload[5:5 + handshake_length]
        if handshake[0] != 1 or len(handshake) < 38 or len(handshake) != handshake_length:
            raise ValueError("ClientHello handshake invalide")
        self.client_random = handshake[6:38]
        self.transcript = bytearray(handshake)
        return payload[:5 + handshake_length]

    def server_hello_record(self):
        hello = bytearray(42)
        hello[0] = HANDSHAKE_SERVER_HELLO
        hello[3] = 38
        hello[4:6] = TLS_VERSION
        hello[6:38] = self.server_random
        hello[38] = 0
        hello[39:41] = CIPHER_ECDHE_RSA_AES128_GCM
        hello[41] = 0
        self.transcript.extend(hello)
        return _record(CONTENT_HANDSHAKE, bytes(hello))

    def certificate_record(self):
        cert = self.leaf_der
        body = _u24(len(cert) + 3) + _u24(len(cert)) + cert
        message = _handshake(HANDSHAKE_CERTIFICATE, body)
        self.transcript.extend(message)
        return _record(CONTENT_HANDSHAKE, message)

    def server_key_exchange_record(self):
        public = self.x25519.public_key().public_bytes_raw()
        params = b"\x03" + struct.pack("!H", NAMED_CURVE_X25519) + bytes((len(public),)) + public
        signed = self.client_random + self.server_random + params
        signature = self.leaf_key.sign(signed, padding.PKCS1v15(), hashes.SHA256())
        body = params + b"\x04\x01" + struct.pack("!H", len(signature)) + signature
        message = _handshake(HANDSHAKE_SERVER_KEY_EXCHANGE, body)
        self.transcript.extend(message)
        return _record(CONTENT_HANDSHAKE, message)

    def server_hello_done_record(self):
        message = _handshake(HANDSHAKE_SERVER_HELLO_DONE, b"")
        self.transcript.extend(message)
        return _record(CONTENT_HANDSHAKE, message)

    def _derive_keys(self, premaster):
        self.master_secret = _prf_sha256(premaster, b"master secret", self.client_random + self.server_random, 48)
        key_block = _prf_sha256(self.master_secret, b"key expansion", self.server_random + self.client_random, 40)
        self.client_write_key = key_block[0:16]
        self.server_write_key = key_block[16:32]
        self.client_fixed_iv = key_block[32:36]
        self.server_fixed_iv = key_block[36:40]

    def accept_client_flight(self, payload):
        records = list(iter_tls_records(payload))
        if len(records) < 3:
            raise ValueError("flight client incomplet")
        key_exchange = records[0]
        change_cipher = records[1]
        finished = records[2]
        if key_exchange[0] != CONTENT_HANDSHAKE or key_exchange[5] != HANDSHAKE_CLIENT_KEY_EXCHANGE:
            raise ValueError("ClientKeyExchange attendu")
        handshake = key_exchange[5:]
        if handshake[4] != 32 or len(handshake) < 37:
            raise ValueError("cle X25519 cliente invalide")
        peer_public = handshake[5:37]
        shared = self.x25519.exchange(X25519PublicKey.from_public_bytes(peer_public))
        self.transcript.extend(handshake[:37])
        self._derive_keys(shared)
        if change_cipher != _record(CONTENT_CHANGE_CIPHER_SPEC, b"\x01"):
            raise ValueError("ChangeCipherSpec client invalide")
        content_type, finished_plain = _aes_gcm_open(
            self.client_write_key, self.client_fixed_iv, 0, finished)
        self.client_read_sequence = 1
        if content_type != CONTENT_HANDSHAKE or finished_plain[0] != HANDSHAKE_FINISHED:
            raise ValueError("Finished client invalide")
        expected = _prf_sha256(
            self.master_secret, b"client finished",
            hashlib.sha256(self.transcript).digest(), 12)
        if finished_plain[4:16] != expected:
            raise ValueError("verify_data client refuse")
        self.transcript.extend(finished_plain)
        self.ready = True
        return True

    def change_cipher_spec_record(self):
        return _record(CONTENT_CHANGE_CIPHER_SPEC, b"\x01")

    def finished_record(self):
        verify = _prf_sha256(
            self.master_secret, b"server finished",
            hashlib.sha256(self.transcript).digest(), 12)
        finished = _handshake(HANDSHAKE_FINISHED, verify)
        record = _aes_gcm_seal(
            self.server_write_key, self.server_fixed_iv, self.server_write_sequence,
            CONTENT_HANDSHAKE, finished)
        self.server_write_sequence += 1
        self.transcript.extend(finished)
        return record

    def open_record(self, payload):
        if not self.ready:
            raise ValueError("session TLS incomplete")
        records = list(iter_tls_records(payload))
        if len(records) != 1:
            raise ValueError("un unique record TLS attendu")
        content_type, plaintext = _aes_gcm_open(
            self.client_write_key, self.client_fixed_iv, self.client_read_sequence, records[0])
        self.client_read_sequence += 1
        return content_type, plaintext

    def open_application(self, payload):
        content_type, plaintext = self.open_record(payload)
        if content_type != CONTENT_APPLICATION_DATA:
            raise ValueError("application_data attendu")
        return plaintext

    def application_record(self, plaintext):
        record = _aes_gcm_seal(
            self.server_write_key, self.server_fixed_iv, self.server_write_sequence,
            CONTENT_APPLICATION_DATA, plaintext)
        self.server_write_sequence += 1
        return record

    def http_ok_record(self):
        return self.application_record(HTTP_REPLY)

    def sse_headers_and_event_record(self):
        return self.application_record(SSE_HEADERS + _http_chunk(SSE_EVENT))

    def sse_done_record(self):
        return self.application_record(_http_chunk(SSE_DONE) + b"0\r\n\r\n")

    def close_notify_record(self):
        record = _aes_gcm_seal(
            self.server_write_key, self.server_fixed_iv, self.server_write_sequence,
            CONTENT_ALERT, b"\x01\x00")
        self.server_write_sequence += 1
        return record


def _self_check():
    server = LocalTls12Server()
    client_private = X25519PrivateKey.generate()
    client_random = bytes((0xA0 + index) & 0xff for index in range(32))
    hello = bytearray(65)
    hello[0] = 1
    hello[3] = 61
    hello[4:6] = TLS_VERSION
    hello[6:38] = client_random
    hello[40] = 4
    hello[41:45] = b"\xc0\x2b\xc0\x2f"
    hello[45] = 1
    hello[47:49] = b"\x00\x10"
    hello[49:65] = b"\x00\x0a\x00\x04\x00\x02\x00\x1d\x00\x0d\x00\x04\x00\x02\x04\x01"
    server.note_client_hello(_record(CONTENT_HANDSHAKE, bytes(hello)))
    for builder in (
        server.server_hello_record, server.certificate_record,
        server.server_key_exchange_record, server.server_hello_done_record,
    ):
        builder()
    client_public = client_private.public_key().public_bytes_raw()
    key_exchange = _handshake(HANDSHAKE_CLIENT_KEY_EXCHANGE, bytes((32,)) + client_public)
    shared = client_private.exchange(X25519PublicKey.from_public_bytes(
        server.x25519.public_key().public_bytes_raw()))
    master = _prf_sha256(shared, b"master secret", client_random + server.server_random, 48)
    transcript = bytes(server.transcript) + key_exchange
    verify = _prf_sha256(master, b"client finished", hashlib.sha256(transcript).digest(), 12)
    key_block = _prf_sha256(master, b"key expansion", server.server_random + client_random, 40)
    finished = _handshake(HANDSHAKE_FINISHED, verify)
    finished_record = _aes_gcm_seal(key_block[0:16], key_block[32:36], 0, CONTENT_HANDSHAKE, finished)
    flight = _record(CONTENT_HANDSHAKE, key_exchange) + _record(CONTENT_CHANGE_CIPHER_SPEC, b"\x01") + finished_record
    from qemu_ne2k_controlled_peer import ControlledEthernetPeer
    hello_record = _record(CONTENT_HANDSHAKE, bytes(hello))
    if ControlledEthernetPeer._tls_handshake_type(hello_record) != 1:
        raise RuntimeError("ClientHello local non reconnu")
    if ControlledEthernetPeer._tls_handshake_type(flight) != 16:
        raise RuntimeError("vol client local pris pour un ClientHello")
    server.accept_client_flight(flight)
    if server.master_secret != master:
        raise RuntimeError("master secret diverge")
    server.change_cipher_spec_record()
    finished_server = server.finished_record()
    content_type, plain = _aes_gcm_open(
        server.server_write_key, server.server_fixed_iv, 0, finished_server)
    if content_type != CONTENT_HANDSHAKE or plain[0] != HANDSHAKE_FINISHED:
        raise RuntimeError("Finished serveur illisible")
    request = b"POST /api/generate HTTP/1.1\r\nHost: api.example.test\r\n\r\n"
    client_app = _aes_gcm_seal(key_block[0:16], key_block[32:36], 1, CONTENT_APPLICATION_DATA, request)
    if server.open_application(client_app) != request:
        raise RuntimeError("POST local illisible")
    reply = server.http_ok_record()
    _, body = _aes_gcm_open(server.server_write_key, server.server_fixed_iv, 1, reply)
    if HTTP_JSON_OK not in body:
        raise RuntimeError("HTTP local incomplet")
    first = server.sse_headers_and_event_record()
    _, body = _aes_gcm_open(server.server_write_key, server.server_fixed_iv, 2, first)
    if b"Transfer-Encoding: chunked" not in body or SSE_EVENT not in body:
        raise RuntimeError("SSE local incomplet")
    if b"Content-Length" in body:
        raise RuntimeError("SSE local avec Content-Length")
    second = server.sse_done_record()
    _, body = _aes_gcm_open(server.server_write_key, server.server_fixed_iv, 3, second)
    if SSE_DONE not in body or b"0\r\n\r\n" not in body:
        raise RuntimeError("SSE local sans [DONE]")
    second_post = _aes_gcm_seal(key_block[0:16], key_block[32:36], 2, CONTENT_APPLICATION_DATA, request)
    if server.open_application(second_post) != request:
        raise RuntimeError("second POST local illisible")
    reply2 = server.http_ok_record()
    _, body = _aes_gcm_open(server.server_write_key, server.server_fixed_iv, 4, reply2)
    if HTTP_JSON_OK not in body:
        raise RuntimeError("second HTTP local incomplet")
    close_record = server.close_notify_record()
    content_type, alert = _aes_gcm_open(
        server.server_write_key, server.server_fixed_iv, 5, close_record)
    if content_type != CONTENT_ALERT or alert != b"\x01\x00":
        raise RuntimeError("close_notify local invalide")
    client_close = _aes_gcm_seal(
        key_block[0:16], key_block[32:36], 3, CONTENT_ALERT, b"\x01\x00")
    content_type, alert = server.open_record(client_close)
    if content_type != CONTENT_ALERT or alert != b"\x01\x00":
        raise RuntimeError("close_notify client local invalide")
    print("local TLS 1.2 self-check passed.")


if __name__ == "__main__":
    _self_check()
