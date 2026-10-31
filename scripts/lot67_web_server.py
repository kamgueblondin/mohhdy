#!/usr/bin/env python3
"""Lot 67: deterministic offline HTTP + HTTPS test server.

  lot67_web_server.py PKI_DIR HTTP_PORT HTTPS_PORT [BIND]

PKI_DIR holds server_chain.pem / server_key.pem from lot67_gen_pki.py (a
throw-away test CA). TLS is pinned to TLS 1.2, X25519 and the two
AES128-GCM-SHA256 ECDHE suites the Mohhdy client offers. Routes:
  /hello        text/plain, Content-Length
  /chunked      chunked body in three chunks
  /page.html    a small HTML page (rendered by /browser)
  /redirect     302 -> /page.html
  /loop         302 -> /loop (redirect limit test)
  /big          200 KiB close-delimited body (truncation test)
Every request is logged on stdout ("REQ <scheme> <path>")."""
import http.server, socketserver, ssl, sys, threading

PAGE = (b"<!doctype html><html><head><title>Mohhdy offline test page</title></head>"
        b"<body><h1>Hello from the Lot 67 test server</h1>"
        b"<p id=\"proof\">fetched over the network by Mohhdy</p>"
        b"<ul><li>chunked</li><li>redirect</li><li>tls</li></ul></body></html>\n")


class H(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, *a):
        pass

    def do_GET(self):
        scheme = "https" if isinstance(self.connection, ssl.SSLSocket) else "http"
        print(f"REQ {scheme} {self.path} host={self.headers.get('Host')}", flush=True)
        p = self.path
        if p == "/hello":
            self._send(200, b"hello from lot67\n", "text/plain")
        elif p == "/page.html" or p == "/":
            self._send(200, PAGE, "text/html; charset=utf-8")
        elif p == "/chunked":
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.send_header("Transfer-Encoding", "chunked")
            self.end_headers()
            for part in (b"chunk-one ", b"chunk-two ", b"chunk-three\n"):
                self.wfile.write(b"%x\r\n%s\r\n" % (len(part), part))
            self.wfile.write(b"0\r\n\r\n")
        elif p == "/redirect":
            self.send_response(302)
            self.send_header("Location", "/page.html")
            self.send_header("Content-Length", "0")
            self.end_headers()
        elif p == "/loop":
            self.send_response(302)
            self.send_header("Location", "/loop")
            self.send_header("Content-Length", "0")
            self.end_headers()
        elif p == "/big":
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.send_header("Connection", "close")
            self.end_headers()
            self.wfile.write(b"x" * (200 * 1024))
            self.close_connection = True
        else:
            self._send(404, b"not found\n", "text/plain")

    def _send(self, code, body, ctype):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


class S(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    allow_reuse_address = True


def main():
    pki, hp, sp = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
    bind = sys.argv[4] if len(sys.argv) > 4 else "127.0.0.1"
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ctx.minimum_version = ssl.TLSVersion.TLSv1_2
    ctx.maximum_version = ssl.TLSVersion.TLSv1_2
    ctx.set_ciphers("ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256")
    ctx.set_ecdh_curve("X25519")
    ctx.load_cert_chain(f"{pki}/server_chain.pem", f"{pki}/server_key.pem")
    plain = S((bind, hp), H)
    tls = S((bind, sp), H)
    tls.socket = ctx.wrap_socket(tls.socket, server_side=True)
    threading.Thread(target=plain.serve_forever, daemon=True).start()
    print(f"READY http={hp} https={sp}", flush=True)
    tls.serve_forever()


if __name__ == "__main__":
    main()
