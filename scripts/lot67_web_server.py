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
  /html/?q=Q    Lot 69: a DuckDuckGo-html-like result page (3 results, the
                first title echoes the decoded query, redirect links uddg=)
  /block/?q=Q   Lot 69: a bot check page (no result)
  /form.html    Lot 71: a GET search form (text, hidden, submit) and a
                POST form; /echo answers both (method, query, body)
  /latin1.html  Lot 71: an ISO-8859-1 page with accents and entities
  /long.html    Lot 71: a long page (1500 paragraphs, about 120 KiB)
  /searx/search?q=Q[&format=json]  Lot 71: SearXNG-like results
Every request is logged on stdout ("REQ <scheme> <path>")."""
import html, http.server, socketserver, ssl, sys, threading, urllib.parse

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
        if self._lot71(p, b""):
            return
        if p.startswith("/html/?") or p.startswith("/block/?"):
            q = urllib.parse.parse_qs(urllib.parse.urlsplit(p).query).get("q", [""])[0]
            print(f"SEARCH q={q!r}", flush=True)
            if p.startswith("/block/"):
                body = ('<html><body><div class="anomaly-modal__title">Unfortunately, bots use DuckDuckGo too.'
                        '</div></body></html>')
            else:
                host = self.headers.get("Host", "10.0.2.2")
                items = [("Result for " + q, "http://%s/page.html" % host, "The offline Lot 67 page, opened by search-open."),
                         ("Mohhdy & the web", "https://example.org/mohhdy?a=1&b=2", "Second result with an <b>escaped</b> &amp; snippet."),
                         ("Third result", "https://example.net/", "Third snippet.")]
                rows = []
                for t, u, sn in items:
                    href = "//duckduckgo.com/l/?uddg=" + urllib.parse.quote(u, safe="") + "&amp;rut=0123"
                    rows.append('<div class="result results_links web-result"><h2 class="result__title">'
                                '<a rel="nofollow" class="result__a" href="%s">%s</a></h2>'
                                '<a class="result__snippet" href="%s">%s</a></div>' % (href, html.escape(t), href, sn))
                body = ('<html><head><title>%s at DuckDuckGo</title></head><body><div id="links">'
                        '<a href="/settings">Settings</a>%s</div></body></html>') % (html.escape(q), "".join(rows))
            self._send(200, body.encode("utf-8"), "text/html; charset=utf-8")
        elif p in ("/gzip", "/deflate", "/biggz"):
            # Lot 69: Content-Encoding the guest must decode
            import gzip as _gz, zlib as _zl
            raw = PAGE if p != "/biggz" else (b"<html><head><title>Big gzip page</title></head><body>" +
                                              b"".join(b"<p>line %d of a big compressed page</p>\n" % i for i in range(20000)) +
                                              b"</body></html>\n")
            enc = "deflate" if p == "/deflate" else "gzip"
            body = _zl.compress(raw) if enc == "deflate" else _gz.compress(raw, mtime=0)
            print(f"ENC {p} raw={len(raw)} wire={len(body)}", flush=True)
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Encoding", enc)
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            try:
                self.wfile.write(body)
            except OSError:
                pass   # the client may stop once its buffer is full
        elif p.startswith("/to?") or p.startswith("/to/"):
            # Lot 69: cross-scheme / cross-port redirect (http -> https, google.com -> www style)
            # /to?<url-encoded URL> or /to/<scheme>/<host>/<port>/<path> (typable in QEMU)
            if p.startswith("/to?"):
                loc = urllib.parse.unquote(p[4:])
            else:
                sch, hst, prt, rest = (p[4:].split("/", 3) + ["", "", "", ""])[:4]
                loc = "%s://%s:%s/%s" % (sch, hst, prt, rest)
            self.send_response(301)
            self.send_header("Location", loc)
            self.send_header("Content-Length", "0")
            self.end_headers()
        elif p == "/hello":
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

    def do_POST(self):
        n = int(self.headers.get("Content-Length", "0") or "0")
        body = self.rfile.read(n) if n > 0 else b""
        print(f"POST {self.path} ctype={self.headers.get('Content-Type')} body={body!r}", flush=True)
        if self.path.startswith("/post-redirect"):
            # 303 after a POST: the client follows with a GET
            self.send_response(303)
            self.send_header("Location", "/echo?after=post")
            self.send_header("Content-Length", "0")
            self.end_headers()
            return
        if not self._lot71(self.path, body, post=True):
            self._send(404, b"not found\n", "text/plain")

    def _lot71(self, p, body, post=False):
        """Lot 71 routes: forms, charsets, long page, SearXNG, cookies."""
        if p == "/controls.html":
            page = ("<html><head><meta charset=\"utf-8\"><title>Options</title></head><body>"
                    "<form action=\"/echo\" method=\"get\">"
                    "<p><input type=\"checkbox\" name=\"safe\" title=\"Filtre\"> filtre</p>"
                    "<p><input type=\"radio\" name=\"t\" value=\"web\" checked> web "
                    "<input type=\"radio\" name=\"t\" value=\"img\"> images</p>"
                    "<p><select name=\"lr\"><option value=\"\">toutes</option>"
                    "<option value=\"lang_fr\" selected>fran\u00e7ais</option><option value=\"lang_en\">anglais</option></select></p>"
                    "<input type=\"submit\" value=\"Chercher\"></form></body></html>")
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p.startswith("/cookie/login"):
            # a login answer: session cookie, then a redirect (as most sites do)
            self.send_response(302)
            self.send_header("Location", "/cookie/home")
            self.send_header("Set-Cookie", "sid=s3cr3t-session; Path=/cookie; HttpOnly")
            self.send_header("Set-Cookie", "theme=dark")
            self.send_header("Set-Cookie", "evil=1; Domain=example.com")
            self.send_header("Content-Length", "0")
            self.end_headers()
            return True
        if p.startswith("/cookie/home"):
            ck = self.headers.get("Cookie", "")
            print(f"COOKIE home={ck!r}", flush=True)
            ok = "sid=s3cr3t-session" in ck
            page = "<html><head><title>%s</title></head><body><p>cookie=%s</p></body></html>" % (
                "Connecte" if ok else "Anonyme", html.escape(ck))
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p == "/form.html":
            page = ("<!doctype html><html><head><meta charset=\"utf-8\"><title>Formulaire \u00e9t\u00e9</title></head><body>"
                    "<h1>Recherche caf\u00e9 &amp; th\u00e9</h1>"
                    "<form action=\"/echo\" method=\"get\" role=\"search\">"
                    "<input type=\"text\" name=\"q\" placeholder=\"Rechercher\">"
                    "<input type=\"hidden\" name=\"lang\" value=\"fr\">"
                    "<input type=\"submit\" name=\"go\" value=\"Envoyer\"></form>"
                    "<form action=\"/echo\" method=\"post\"><input name=\"user\" value=\"\">"
                    "<textarea name=\"msg\">bonjour</textarea><button type=\"submit\">Poster</button></form>"
                    "<form action=\"/post-redirect\" method=\"post\"><input name=\"x\"><input type=\"submit\"></form>"
                    "</body></html>")
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p.startswith("/echo"):
            q = urllib.parse.urlsplit(p).query
            ans = ("<html><head><title>Echo</title></head><body><p id=\"m\">method=%s</p><p>query=%s</p>"
                   "<p>body=%s</p><p>decoded=%s</p></body></html>") % (
                "POST" if post else "GET", html.escape(q), html.escape(body.decode("latin-1")),
                html.escape(repr(urllib.parse.parse_qs(body.decode("latin-1") if post else q))))
            print(f"ECHO {'POST' if post else 'GET'} q={q!r} body={body!r}", flush=True)
            self._send(200, ans.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p == "/latin1.html":
            page = ("<html><head><title>Caf\u00e9 Latin-1</title></head><body><p>\u00e9t\u00e9 \u00e0 la "
                    "plage, &eacute;l&egrave;ve &#233; &#x153;uvre &euro; &laquo;ok&raquo;</p></body></html>")
            self._send(200, page.encode("latin-1", "xmlcharrefreplace"), "text/html; charset=iso-8859-1")
            return True
        if p == "/long.html":
            rows = "".join("<p>Paragraphe num\u00e9ro %d de la longue page.</p>\n" % i for i in range(1, 1501))
            page = "<html><head><title>Longue page</title></head><body><h1>D\u00e9but</h1>%s<p>FIN-DE-PAGE</p></body></html>" % rows
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p.startswith("/searx/search"):
            qs = urllib.parse.parse_qs(urllib.parse.urlsplit(p).query)
            q = qs.get("q", [""])[0]
            print(f"SEARX q={q!r} format={qs.get('format', [''])[0]}", flush=True)
            res = [("R\u00e9sultat pour " + q, "https://example.org/1", "Premier extrait."),
                   ("Second & co", "https://example.org/2?a=1&b=2", "Deuxi\u00e8me extrait.")]
            if qs.get("format", [""])[0] == "json":
                import json as _js
                body = _js.dumps({"query": q, "results": [{"title": t, "url": u, "content": c} for t, u, c in res]})
                self._send(200, body.encode("utf-8"), "application/json")
                return True
            arts = "".join('<article class="result result-default"><h3><a href="%s">%s</a></h3>'
                           '<p class="content">%s</p></article>' % (html.escape(u), html.escape(t), html.escape(c))
                           for t, u, c in res)
            page = "<html><head><title>%s - SearXNG</title></head><body><div id=\"urls\">%s</div></body></html>" % (
                html.escape(q), arts)
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        return False

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
    # Lot 69: sp+1 TLS 1.3 only (ECDSA leaf), sp+2 TLS 1.3 only (RSA leaf,
    # RSA-PSS CertificateVerify), sp+3 TLS 1.2 ECDHE-RSA with P-256 only
    extra = []
    for off, ver, chain, key, curve in ((1, "1.3", "server_chain", "server_key", None),
                                        (2, "1.3", "server_rsa_chain", "server_rsa_key", None),
                                        (3, "1.2", "server_rsa_chain", "server_rsa_key", "prime256v1")):
        c = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        if ver == "1.3":
            c.minimum_version = ssl.TLSVersion.TLSv1_3
        else:
            c.minimum_version = c.maximum_version = ssl.TLSVersion.TLSv1_2
            c.set_ciphers("ECDHE-RSA-AES128-GCM-SHA256")
        if curve:
            c.set_ecdh_curve(curve)
        c.load_cert_chain(f"{pki}/{chain}.pem", f"{pki}/{key}.pem")
        srv = S((bind, sp + off), H)
        srv.socket = c.wrap_socket(srv.socket, server_side=True)
        threading.Thread(target=srv.serve_forever, daemon=True).start()
        extra.append(sp + off)
    print(f"READY http={hp} https={sp} tls13={extra[0]} tls13rsa={extra[1]} p256={extra[2]}", flush=True)
    tls.serve_forever()


if __name__ == "__main__":
    main()
