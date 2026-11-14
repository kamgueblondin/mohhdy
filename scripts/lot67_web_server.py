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
  /lot72/images.html  Lot 72: a page with PNG, GIF (animated), JPEG
                (baseline, progressive), a missing image with alt text and
                a favicon link; the image bytes are the unit test fixtures
                (tests/unit/userspace/imgdec*_samples.h), /lot72/img/NAME
  /favicon.ico  Lot 72: the .ico fixture (DIB entries)
  /lot73/anim.html    Lot 73: an animated GIF (red, green, blue frames,
                      50 cs, loop forever) and a two-frame GIF played twice,
                      /lot73/img/NAME (tests/unit/userspace/imgdec_anim_samples.h)
  /lot73/pic.html     Lot 73: picture / srcset choice and a WebP image
  /lot73/etag.txt     Lot 73: ETag + no-cache, 304 on If-None-Match
  /lot73/fresh.txt    Lot 73: Cache-Control max-age=300
  /lot72/cp1251.html  Lot 72: a windows-1251 page (charset in the header)
                with a GET form whose field holds Cyrillic text: the
                browser must send it back in windows-1251 (/echo)
  /lot72/greek.html   Lot 72: an ISO-8859-7 page (charset in a meta tag)
  /lot87/page.html    Lot 87: a page with two linked stylesheets (one
                      for print, not fetched) and a <style> @import; the
                      sheets hide the banner and the side note
  /lot87/main.css, /lot87/base.css, /lot87/print.css  the sheets ("CSS name")
  /lot87/wall.html    Lot 87: an enablejs script wall with a noscript link
  /lot87/basic.html   Lot 87: the plain fallback page
  /lot87/blocked      Lot 87: 403 with a block notice
  /lot88/scope.html   Lot 88: @scope (limit, implicit root, :scope), all
  /lot88/props.html   Lot 88: background-color, list-style, font-size,
                      text-indent, letter-spacing in the reader sheet
  /lot89/scope.html   Lot 89: @scope start and end lists, &:first-child,
                      :scope:last-child
  /lot89/stray1.html  Lot 89: the same in one write (read together by the
                      guest, which must drop the connection)
  /lot89/props.html   Lot 89: calc(), min(), clamp() and var() anywhere in
                      a reader length
  /lot89/vars.html    Lot 89: custom properties set per element (a class,
                      a style attribute, an @scope block) and inherited
  /kastray/N          Lot 89: a keep-alive answer and an unsolicited 404 in
                      the same write (the client must drop the connection)
  /lot89/stray.html   Lot 89: a page followed by an unsolicited second
                      response on the same keep-alive connection (the
                      server bug that exposed the NE2000 ring stall)
  /lot90/bg.html      Lot 90: background shorthand with a gradient,
                      list-style with an image, calc()/rem/clamp() font sizes
  /lot90/props.html   Lot 90: @property and declaration-time values
  /lot90/scope.html   Lot 90: a seven-selector @scope list, root proximity
  /lot91/bg.html      Lot 91: radial gradients with explicit radii,
                      gradians, background-clip and background-origin
  /lot91/props.html   Lot 91: @property syntaxes with alternatives and
                      lists, calc() type checking
  /lot92/bg.html      Lot 92: linear-gradient angles in radians, a
                      registered angle computed by mod()
  /lot92/props.html   Lot 92: registered math values computed (calc,
                      round(), a computed initial value), sin() (Lot 93:
                      computed)
  /lot93/math.html    Lot 93: registered values computed with atan2(),
                      acos(), hypot(), pow(), pi, sqrt(), log(), exp();
                      tan(90deg) kept
  /lot94/grad.html    Lot 94: linear angles that are not quarter turns
                      (radians as whole degrees), a corner, conic-gradient,
                      a registered angle of 45deg, a refused conic stop
  /lot95/props.html   Lot 95: @property with eight alternatives, the Lot 95
                      types, a 31-byte name; plain dimensions, viewport
                      units, a list, a mix, a transform list computed
  /lot96/grad.html    Lot 96: repeating gradients, radians written to 1/100
                      degree (a radian conic "from"), a conic stop with
                      decimals, a period of 0 (Lot 97: passed on), a
                      registered <image> with a period of 0
  /lot97/anim.html    Lot 97: @keyframes and a transition of registered
                      properties at the reader clock (web-css time N), a
                      registered length in em inherited as a length, an
                      !important value kept, @starting-style without a
                      custom property, a dropped @keyframes name
  /lot98/anim.html    Lot 98: animation and transition longhands, @keyframes
                      inside @scope, animation-composition: add, a
                      transition between hsl() colours, ex and ch from the
                      engine font
  /lot99/anim.html    Lot 99: hsl() painted with turn and grad hues, the
                      generated 0% and 100% keyframes composed with add,
                      animation-timeline: none (idle), an hsl() value of a
                      registered colour read through var()
Every request is logged on stdout ("REQ <scheme> <path>")."""
import html, http.server, os, re, socketserver, ssl, sys, threading, urllib.parse

_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def _c_arrays(path):
    """byte arrays of a generated C header: name -> bytes"""
    out = {}
    try:
        text = open(path).read()
    except OSError:
        return out
    for m in re.finditer(r"static const unsigned char (\w+)\[\d+\] = \{(.*?)\};", text, re.S):
        out[m.group(1)] = bytes(int(x, 0) for x in re.findall(r"0x[0-9a-fA-F]+|\d+", m.group(2)))
    return out


_IMG = {}
for _h in ("imgdec_samples.h", "imgdec_gif_samples.h", "imgdec_anim_samples.h", "imgdec_webp_samples.h"):
    _IMG.update(_c_arrays(os.path.join(_ROOT, "tests", "unit", "userspace", _h)))
LOT72_FILES = {
    "mire.png": ("img_mire_png", "image/png"),
    "mire444.jpg": ("img_mire444_jpg", "image/jpeg"),
    "mire-prog.jpg": ("img_mire_prog_jpg", "image/jpeg"),
    "anim.gif": ("gif_anim", "image/gif"),
    "big.gif": ("gif_big", "image/gif"),
    "fav.ico": ("gif_ico_bmp", "image/x-icon"),
}
LOT72_PAGE = ("<!doctype html><html><head><meta charset=\"utf-8\"><title>Images Lot 72</title>"
              "<link rel=\"icon\" href=\"/lot72/img/fav.ico\"></head><body>"
              "<h1>Images dans le navigateur</h1>"
              "<p>PNG <img src=\"img/mire.png\" width=\"144\" height=\"96\" alt=\"mire png\"> "
              "GIF <img src=\"/lot72/img/anim.gif\" alt=\"gif\"></p>"
              "<p>JPEG <img src=\"img/mire444.jpg\" width=\"96\"> progressif "
              "<img data-src=\"img/mire-prog.jpg\" src=\"data:image/gif;base64,R0lGODlh\" width=\"96\"></p>"
              "<p><img src=\"img/big.gif\" alt=\"grande image\"></p>"
              "<p>absente : <img src=\"img/absente.png\" alt=\"image absente\"></p>"
              "<p>FIN-IMAGES</p></body></html>")

# Lot 73: an animated GIF page (32x24, red / green / blue frames of
# 50 cs, looping forever; scripts/gen_gif_anim_fixtures.py)
LOT73_FILES = {
    "anim3.gif": ("gif_anim_page", "image/gif"),
    "twice.gif": ("gif_anim_twice", "image/gif"),
    "pal4.webp": ("webp_pal4", "image/webp"),
}
# Lot 73 (round 2): picture / srcset choice (never.gif must never be
# asked for) and a WebP lossless image
LOT73_PIC = ("<!doctype html><html><head><title>Picture Lot 73</title></head><body>"
             "<p><picture><source srcset=\"img/never.gif\" type=\"image/avif\">"
             "<source srcset=\"img/anim3.gif\" type=\"image/gif\"><img src=\"img/never.gif\" alt=\"pic\"></picture> "
             "<img srcset=\"img/twice.gif 1x, img/never.gif 2x\" src=\"img/never.gif\" alt=\"set\"> "
             "<img src=\"img/pal4.webp\" alt=\"webp\"></p><p>FIN-PIC</p></body></html>")
LOT73_PAGE = ("<!doctype html><html><head><meta charset=\"utf-8\"><title>Animation Lot 73</title></head><body>"
              "<h1>GIF anime</h1>"
              "<p><img src=\"img/anim3.gif\" alt=\"anim\"> <img src=\"img/twice.gif\" alt=\"twice\"></p>"
              "<p>FIN-ANIM</p></body></html>")

PAGE = (b"<!doctype html><html><head><title>Mohhdy offline test page</title></head>"
        b"<body><h1>Hello from the Lot 67 test server</h1>"
        b"<p id=\"proof\">fetched over the network by Mohhdy</p>"
        b"<ul><li>chunked</li><li>redirect</li><li>tls</li></ul></body></html>\n")


# Lot 90: reader pages (tests/scripts/test_qemu_lot90.py)
_LOT90_NEST_NEAR = "<div>" * 8
_LOT90_NEST_END = "</div>" * 8
LOT90_PAGES = {
    # background shorthand with a gradient, list-style with an image,
    # font lengths in px (calc with em, rem, a long clamp())
    "/lot90/bg.html": (
        "<!doctype html><html style=\"font-size: 20px\"><head><title>Lot 90 backgrounds</title><style>"
        "#bg90 { background: #102030 linear-gradient(to right, #ff0000, #0000ff) center / cover no-repeat }\n"
        "#ls90 { list-style: square url(dot90.png) inside }\n"
        ".par90 { font-size: 16px }\n"
        "#em90 { font-size: calc(1.5em + 2px) }\n"
        "#rem90 { font-size: 2rem }\n"
        "#long90 { font-size: clamp(12.000px, calc(0.5000rem + 0.2500em + 1.5000px + 2.0000px), 40.000px) }\n"
        "</style></head><body><h1>Fonds</h1>"
        "<div id=\"bg90\"><p>bgkeep90</p></div>"
        "<ul id=\"ls90\"><li>lskeep90</li></ul>"
        "<div class=\"par90\"><p id=\"em90\">emkeep90</p></div>"
        "<p id=\"rem90\">remkeep90</p>"
        "<p id=\"long90\">longkeep90</p>"
        "<p>fin90b</p></body></html>"),
    # @property: a registered length that does not inherit, a refused
    # initial-value, a custom property computed where it is declared
    "/lot90/props.html": (
        "<!doctype html><html style=\"font-size: 20px\"><head><title>Lot 90 @property</title><style>"
        "@property --sz90 { syntax: '<length>'; inherits: false; initial-value: 15px }\n"
        "@property --bad90 { syntax: '<length>'; inherits: false; initial-value: 2em }\n"
        ":root { --base90: 10px; --dbl90: calc(var(--base90) * 2) }\n"
        ".in90 { --base90: 30px }\n"
        "#root90, #decl90 { font-size: var(--dbl90) }\n"
        ".hold90 { --sz90: 30px }\n"
        "#reg90 { font-size: var(--sz90) }\n"
        "#wrong90 { --sz90: red; font-size: var(--dbl90); text-indent: var(--sz90) }\n"
        "</style></head><body><h1>Proprietes</h1>"
        "<p id=\"root90\">proot90</p>"
        "<div class=\"in90\"><p id=\"decl90\">pdecl90</p></div>"
        "<div class=\"hold90\"><p id=\"reg90\">preg90</p></div>"
        "<p id=\"wrong90\">pwrong90</p>"
        "<p>fin90p</p></body></html>"),
    # @scope: a start list of seven selectors (six kept) and two roots,
    # nine and eleven generations up (the nearer wins)
    "/lot90/scope.html": (
        "<!doctype html><html><head><title>Lot 90 @scope</title><style>"
        "@scope (.s1, .s2, .s3, .s4, .s5, .s6, .s7) { p { display: none } }\n"
        "@scope (.near90) { p { color: #00aa00 } }\n"
        "@scope (.far90) { p { color: #aa0000 } }\n"
        "</style></head><body><h1>Portee</h1>"
        "<div class=\"s6\"><p>sixhide90</p></div>"
        "<div class=\"s7\"><p>sevenkeep90</p></div>"
        "<div class=\"far90\"><div><div class=\"near90\">" + _LOT90_NEST_NEAR +
        "<p id=\"prox90\">proxkeep90</p>" + _LOT90_NEST_END + "</div></div></div>"
        "<p>fin90s</p></body></html>"),
}


# Lot 91: reader pages (tests/scripts/test_qemu_lot91.py)
LOT91_PAGES = {
    # explicit radial radii, a gradian angle, the box keywords of the
    # background shorthand and the longhands; two refused values
    "/lot91/bg.html": (
        "<!doctype html><html><head><title>Lot 91 backgrounds</title><style>"
        "#rad91 { background: radial-gradient(circle 40px at center, #ff0000, #0000ff) no-repeat }\n"
        "#ell91 { background-image: radial-gradient(ELLIPSE 30px 20% at left top, #00ff00, #000000) }\n"
        "#grad91 { background-image: linear-gradient(100grad, #ff0000, #0000ff) }\n"
        "#box91 { background: #eeeeee content-box padding-box }\n"
        "#one91 { background: #dddddd border-box }\n"
        "#clip91 { background-color: #ddeeff; background-clip: content-box; background-origin: border-box }\n"
        "#badr91 { background-color: #123456; background-image: radial-gradient(circle 10%, #ff0000, #0000ff) }\n"
        "#badc91 { background-color: #654321; background-clip: text }\n"
        "</style></head><body><h1>Fonds 91</h1>"
        "<div id=\"rad91\"><p>radkeep91</p></div>"
        "<div id=\"ell91\"><p>ellkeep91</p></div>"
        "<div id=\"grad91\"><p>gradkeep91</p></div>"
        "<div id=\"box91\"><p>boxkeep91</p></div>"
        "<div id=\"one91\"><p>onekeep91</p></div>"
        "<div id=\"clip91\"><p>clipkeep91</p></div>"
        "<div id=\"badr91\"><p>badrkeep91</p></div>"
        "<div id=\"badc91\"><p>badckeep91</p></div>"
        "<p>fin91b</p></body></html>"),
    # @property: an alternative syntax (Lot 90 refused the rule), a
    # list, nine alternatives (refused; Lot 95 converted it from five), a calc() whose type is not a
    # <length> (Lot 90 took it) and one that is
    "/lot91/props.html": (
        "<!doctype html><html><head><title>Lot 91 @property</title><style>"
        "@property --w91 { syntax: 'auto | <length>'; inherits: false; initial-value: 11px }\n"
        "@property --l91 { syntax: '<length>#'; inherits: true; initial-value: 1px, 2px }\n"
        # Lot 95 (converted): nine alternatives (five are read since Lot 95)
        "@property --five91 { syntax: 'a | b | c | d | e | f | g | h | i'; inherits: false; initial-value: a }\n"
        "@property --len91 { syntax: '<length>'; inherits: false; initial-value: 13px }\n"
        "#alt91 { --w91: 24px; font-size: var(--w91) }\n"
        "#altbad91 { --w91: red; font-size: var(--w91) }\n"
        "#calcbad91 { --len91: calc(10px + 5%); font-size: var(--len91) }\n"
        "#calcok91 { --len91: calc(2px * 3 + 4px); font-size: var(--len91) }\n"
        "</style></head><body><h1>Proprietes 91</h1>"
        "<p id=\"alt91\">altkeep91</p>"
        "<p id=\"altbad91\">altbadkeep91</p>"
        "<p id=\"calcbad91\">calcbadkeep91</p>"
        "<p id=\"calcok91\">calcokkeep91</p>"
        "<p>fin91p</p></body></html>"),
}


# Lot 92: reader pages (tests/scripts/test_qemu_lot92.py)
LOT92_PAGES = {
    # radian angles (two quarter turns kept as whole degrees, 1rad
    # refused; Lot 94: written as 57deg; Lot 96: 57.3deg), a registered <angle> computed by mod() feeds the gradient,
    # the same value unregistered keeps its text and is refused
    "/lot92/bg.html": (
        "<!doctype html><html><head><title>Lot 92 backgrounds</title><style>"
        "#rad92 { background-image: linear-gradient(1.5708rad, #ff0000, #0000ff) }\n"
        "#radn92 { background: linear-gradient(-3.1416RAD, #00ff00, #000000) no-repeat }\n"
        "#radbad92 { background-color: #123456; background-image: linear-gradient(1rad, #ff0000, #0000ff) }\n"
        "@property --a92 { syntax: '<angle>'; inherits: false; initial-value: 0deg }\n"
        "#ang92 { --a92: mod(450deg, 360deg); background-image: linear-gradient(var(--a92), #ff0000, #0000ff) }\n"
        "#angu92 { --u92: mod(450deg, 360deg); background-color: #654321; background-image: linear-gradient(var(--u92), #ff0000, #0000ff) }\n"
        "</style></head><body><h1>Fonds 92</h1>"
        "<div id=\"rad92\"><p>radkeep92</p></div>"
        "<div id=\"radn92\"><p>radnkeep92</p></div>"
        "<div id=\"radbad92\"><p>radbadkeep92</p></div>"
        "<div id=\"ang92\"><p>angkeep92</p></div>"
        "<div id=\"angu92\"><p>angukeep92</p></div>"
        "<p>fin92b</p></body></html>"),
    # registered <length> values computed: a calc() radius, a round()
    # font size, a computed initial value (1in / 8 = 12px) inherited;
    # sin() was typed and kept in Lot 92; Lot 93 computes it (5px)
    "/lot92/props.html": (
        "<!doctype html><html><head><title>Lot 92 math</title><style>"
        "@property --r92 { syntax: '<length>'; inherits: false; initial-value: 0px }\n"
        "@property --s92 { syntax: '<length>'; inherits: true; initial-value: calc(1in / 8) }\n"
        "@property --n92 { syntax: '<integer>'; inherits: false; initial-value: 0 }\n"
        "#circ92 { --r92: calc(20px * 2); background-image: radial-gradient(circle var(--r92), #ff0000, #0000ff) }\n"
        "#ucirc92 { --u92: calc(20px * 2); background-color: #224466; background-image: radial-gradient(circle var(--u92), #ff0000, #0000ff) }\n"
        "#size92 { --s92: round(up, 21px, 4px); font-size: var(--s92) }\n"
        "#init92 { font-size: var(--s92) }\n"
        "#trig92 { --s92: calc(sin(30deg) * 10px); font-size: var(--s92) }\n"
        "</style></head><body><h1>Maths 92</h1>"
        "<div id=\"circ92\"><p>circkeep92</p></div>"
        "<div id=\"ucirc92\"><p>ucirckeep92</p></div>"
        "<p id=\"size92\">sizekeep92</p>"
        "<p id=\"init92\">initkeep92</p>"
        "<p id=\"trig92\">trigkeep92</p>"
        "<p>fin92p</p></body></html>"),
}


# Lot 93: reader page (tests/scripts/test_qemu_lot93.py)
LOT93_PAGES = {
    # registered <angle> values computed by atan2() and acos() feed the
    # gradient (90deg, 180deg); the computed initial value atan2(1, 1) is
    # 45deg, which the engine did not draw (the colour stayed; Lot 94: it
    # is drawn); the same
    # atan2() unregistered keeps its text and is refused. A registered
    # <length> computed by hypot() is a 50px radius; pow(), pi with
    # round(), the initial sqrt() and log() with exp() give font sizes;
    # tan(90deg) is infinite and kept, so font-size does not take it
    "/lot93/math.html": (
        "<!doctype html><html><head><title>Lot 93 math</title><style>"
        "@property --a93 { syntax: '<angle>'; inherits: false; initial-value: atan2(1, 1) }\n"
        "@property --r93 { syntax: '<length>'; inherits: false; initial-value: 0px }\n"
        "@property --s93 { syntax: '<length>'; inherits: true; initial-value: calc(sqrt(144) * 1px) }\n"
        "#ang93 { --a93: atan2(1, 0); background-image: linear-gradient(var(--a93), #ff0000, #0000ff) }\n"
        "#acos93 { --a93: acos(-1); background: linear-gradient(var(--a93), #00ff00, #000000) no-repeat }\n"
        "#init93 { background-color: #123456; background-image: linear-gradient(var(--a93), #ff0000, #0000ff) }\n"
        "#uang93 { --u93: atan2(1, 0); background-color: #654321; background-image: linear-gradient(var(--u93), #ff0000, #0000ff) }\n"
        "#rad93 { --r93: hypot(30px, 40px); background-image: radial-gradient(circle var(--r93), #ff0000, #0000ff) }\n"
        "#pow93 { --s93: calc(pow(2, 4) * 1px); font-size: var(--s93) }\n"
        "#pi93 { --s93: round(calc(pi * 5px), 1px); font-size: var(--s93) }\n"
        "#sqrt93 { font-size: var(--s93) }\n"
        "#tan93 { --s93: calc(tan(90deg) * 1px); font-size: var(--s93) }\n"
        "#log93 { --s93: calc(log(8, 2) * exp(0) * 7px); font-size: var(--s93) }\n"
        "</style></head><body><h1>Maths 93</h1>"
        "<div id=\"ang93\"><p>angkeep93</p></div>"
        "<div id=\"acos93\"><p>acoskeep93</p></div>"
        "<div id=\"init93\"><p>initkeep93</p></div>"
        "<div id=\"uang93\"><p>uangkeep93</p></div>"
        "<div id=\"rad93\"><p>radkeep93</p></div>"
        "<p id=\"pow93\">powkeep93</p>"
        "<p id=\"pi93\">pikeep93</p>"
        "<p id=\"sqrt93\">sqrtkeep93</p>"
        "<p id=\"tan93\">tankeep93</p>"
        "<p id=\"log93\">logkeep93</p>"
        "<p>fin93</p></body></html>"),
}


# Lot 94: reader page (tests/scripts/test_qemu_lot94.py)
LOT94_PAGES = {
    # 0.7854rad is written as 45deg; a corner and a conic gradient keep
    # their words; a registered angle computed to 45deg is drawn now; a
    # conic stop past 360deg refuses the gradient (the colour stays)
    "/lot94/grad.html": (
        "<!doctype html><html><head><title>Lot 94 gradients</title><style>"
        "@property --a94 { syntax: '<angle>'; inherits: false; initial-value: 0deg }\n"
        "#lin94 { background-image: linear-gradient(0.7854rad, #ff0000, #0000ff) }\n"
        "#cor94 { background: linear-gradient(TO Bottom   left, #00ff00, #000000) }\n"
        "#con94 { background-image: conic-gradient(from 30deg at 25% 75%, #ff0000, #0000ff 180deg, #00ff00) }\n"
        "#reg94 { --a94: calc(30deg + 15deg); background-image: linear-gradient(var(--a94), #ff0000, #0000ff) }\n"
        "#bad94 { background-color: #123456; background-image: conic-gradient(#ff0000 400deg, #0000ff) }\n"
        "#deg94 { background: linear-gradient(-30.5DEG, #ff0000 10%, #0000ff) }\n"
        "</style></head><body><h1>Gradients 94</h1>"
        "<div id=\"lin94\"><p>linkeep94</p></div>"
        "<div id=\"cor94\"><p>corkeep94</p></div>"
        "<div id=\"con94\"><p>conkeep94</p></div>"
        "<div id=\"reg94\"><p>regkeep94</p></div>"
        "<div id=\"bad94\"><p>badkeep94</p></div>"
        "<div id=\"deg94\"><p>degkeep94</p></div>"
        "<p>fin94</p></body></html>"),
}


# Lot 95: reader page (tests/scripts/test_qemu_lot95.py)
LOT95_PAGES = {
    # @property with up to eight alternatives and the Lot 95 types, nine
    # alternatives refused, a 31-byte name, more than eight registrations;
    # registered values computed: an initial 0.125in (12px), 2.5vw at the
    # 640 x 480 reader viewport (16px), a one-item <angle># list (100grad
    # -> 90deg), a length-percentage mix (calc(8% + 96px), indent 97.28px
    # as the reader resolved that text before), a transform list and a
    # resolution (counted: simplified 5)
    "/lot95/props.html": (
        "<!doctype html><html><head><title>Lot 95 @property</title><style>"
        "@property --l95 { syntax: '<length> | <transform-list>'; inherits: true; initial-value: 0.125in }\n"
        "@property --e95 { syntax: 'a | b | c | d | e | f | g | <length>'; inherits: false; initial-value: 1pc }\n"
        "@property --nine95 { syntax: 'a | b | c | d | e | f | g | h | i'; inherits: false; initial-value: a }\n"
        "@property --a95 { syntax: '<angle>#'; inherits: false; initial-value: 0.25turn }\n"
        "@property --m95 { syntax: '<length-percentage>'; inherits: false; initial-value: 0px }\n"
        "@property --t95 { syntax: '<transform-list>'; inherits: false; initial-value: rotate(0) }\n"
        "@property --r95 { syntax: '<resolution>'; inherits: false; initial-value: 1x }\n"
        "@property --i95 { syntax: '<image>'; inherits: false; initial-value: linear-gradient(#ff0000, #0000ff) }\n"
        "@property --s95 { syntax: '<string> | <url>'; inherits: false; initial-value: 'x' }\n"
        "@property --abcdefghijklmnopqrstuvwxyz095 { syntax: '<length>'; inherits: false; initial-value: 0.25in }\n"
        "#in95 { font-size: var(--l95) }\n"
        "#vw95 { --l95: 2.5vw; font-size: var(--l95) }\n"
        "#pc95 { font-size: var(--e95) }\n"
        "#ang95 { --a95: 100grad; background-image: linear-gradient(var(--a95), #ff0000, #0000ff) }\n"
        "#mix95 { --m95: calc(1in + 10% - 2%); text-indent: var(--m95) }\n"
        "#t95 { --t95: translate(1in) rotate(0) }\n"
        "#r95 { --r95: 96dpi }\n"
        "#img95 { background-image: var(--i95) }\n"
        "#long95 { font-size: var(--abcdefghijklmnopqrstuvwxyz095) }\n"
        "</style></head><body><h1>Props 95</h1>"
        "<p id=\"in95\">inkeep95</p>"
        "<div id=\"vwbox95\"><p id=\"vw95\">vwkeep95</p></div>"
        "<p id=\"pc95\">pckeep95</p>"
        "<div id=\"ang95\"><p>angkeep95</p></div>"
        "<p id=\"mix95\">mixkeep95</p>"
        "<p id=\"t95\">tkeep95</p>"
        "<p id=\"r95\">rkeep95</p>"
        "<div id=\"img95\"><p>imgkeep95</p></div>"
        "<p id=\"long95\">longkeep95</p>"
        "<p>fin95</p></body></html>"),
}


# Lot 96: reader page (tests/scripts/test_qemu_lot96.py)
LOT96_PAGES = {
    # repeating gradients (1rad written as 57.3deg, a radian conic "from",
    # a 45.5deg stop), a period of 0 (Lot 97: passed on with the colour;
    # Lot 96 refused it), a registered <image> with a period of 0 (Lot 97:
    # its own value), a conic gradient with stops in hundredths of a degree
    "/lot96/grad.html": (
        "<!doctype html><html><head><title>Lot 96 gradients</title><style>"
        "@property --i96 { syntax: '<image>'; inherits: false; initial-value: linear-gradient(#ff0000, #0000ff) }\n"
        "#rl96 { background-image: repeating-linear-gradient(1rad, #ff0000, #0000ff 20%) }\n"
        "#rc96 { background: Repeating-Conic-Gradient(from 0.5RAD at 25% 75%, #ff0000, #0000ff 45.5deg) }\n"
        "#rr96 { background-image: repeating-radial-gradient(circle 10px, #00ff00, #000000 50%) }\n"
        "#flat96 { background-color: #123456; background-image: repeating-linear-gradient(#ff0000 50%, #0000ff 50%) }\n"
        "#img96 { --i96: repeating-linear-gradient(#ff0000 10%, #0000ff 10%); background-image: var(--i96) }\n"
        "#deg96 { background-image: conic-gradient(#ff0000 0.25deg, #0000ff 359.75deg) }\n"
        "</style></head><body><h1>Gradients 96</h1>"
        "<div id=\"rl96\"><p>rlkeep96</p></div>"
        "<div id=\"rc96\"><p>rckeep96</p></div>"
        "<div id=\"rr96\"><p>rrkeep96</p></div>"
        "<div id=\"flat96\"><p>flatkeep96</p></div>"
        "<div id=\"img96\"><p>imgkeep96</p></div>"
        "<div id=\"deg96\"><p>degkeep96</p></div>"
        "<p>fin96</p></body></html>"),
}


# Lot 97: reader page (tests/scripts/test_qemu_lot97.py); the guest sets
# the reader clock first (web-css time N)
LOT97_PAGES = {
    "/lot97/anim.html": (
        "<!doctype html><html><head><title>Lot 97 animations</title><style>"
        "@property --c97 { syntax: '<color>'; inherits: true; initial-value: black }\n"
        "@property --w97 { syntax: '<length>'; inherits: true; initial-value: 0px }\n"
        "@property --t97 { syntax: '<color>'; inherits: false; initial-value: black }\n"
        "@keyframes k97 { from { --c97: rgb(0, 0, 0) } to { --c97: rgb(200, 100, 0) } }\n"
        "@keyframes 'q97' { from { --c97: rgb(0, 0, 0) } to { --c97: rgb(0, 200, 0) } }\n"
        "@keyframes a.b { to { --c97: red } }\n"
        "#a97 { color: var(--c97); animation: k97 1000ms linear }\n"
        "#q97 { color: var(--c97); animation: q97 1000ms steps(2) }\n"
        "#i97 { --c97: rgb(1, 2, 3) !important; color: var(--c97); animation: k97 1000ms linear }\n"
        "#f97 { font-size: 20px; --w97: 2em } #f97 p { font-size: 10px } #f97 span { font-size: var(--w97) }\n"
        "#t97 { --t97: rgb(0, 0, 255); background-color: var(--t97); transition: --t97 1000ms linear }\n"
        "@starting-style { #t97 { --t97: rgb(0, 0, 55) } #s97 { color: #ff0000 } }\n"
        "</style></head><body><h1>Animations 97</h1>"
        "<div id=\"a97\"><p>akeep97</p></div>"
        "<div id=\"q97\"><p>qkeep97</p></div>"
        "<div id=\"i97\"><p>ikeep97</p></div>"
        "<div id=\"f97\"><p>fkeep97 <span id=\"sp97\">spkeep97</span></p></div>"
        "<div id=\"t97\"><p>tkeep97</p></div>"
        "<div id=\"s97\"><p>skeep97</p></div>"
        "<p>fin97</p></body></html>"),
}


# Lot 98: reader page (tests/scripts/test_qemu_lot98.py); the guest sets
# the reader clock first (web-css time N)
LOT98_PAGES = {
    "/lot98/anim.html": (
        "<!doctype html><html><head><title>Lot 98 longhands</title><style>"
        "@property --c98 { syntax: '<color>'; inherits: true; initial-value: black }\n"
        "@property --w98 { syntax: '<length>'; inherits: true; initial-value: 0px }\n"
        "@property --t98 { syntax: '<color>'; inherits: false; initial-value: black }\n"
        "@scope (#a98) { @keyframes k98 { from { --c98: rgb(0, 0, 0) } to { --c98: rgb(200, 100, 0) } } }\n"
        "@scope (#a98) { @property --x98 { syntax: '<length>'; inherits: false; initial-value: 0px } }\n"
        "#a98 { color: var(--c98); animation-name: k98; animation-duration: 1000ms; animation-timing-function: linear }\n"
        "#b98 { --c98: rgb(0, 0, 100); color: var(--c98); animation: k98 1000ms linear; animation-composition: add }\n"
        "#h98 { --t98: hsl(240, 100%, 50%); background-color: var(--t98); transition-property: --t98;"
        " transition-duration: 1000ms; transition-timing-function: linear }\n"
        "@starting-style { #h98 { --t98: hsl(0 100% 50%) } }\n"
        "#f98 { font-size: 16px; --w98: calc(2ex + 1ch) } #f98 span { font-size: var(--w98) }\n"
        "</style></head><body><h1>Longhands 98</h1>"
        "<div id=\"a98\"><p>akeep98</p></div>"
        "<div id=\"b98\"><p>bkeep98</p></div>"
        "<div id=\"h98\"><p>hkeep98</p></div>"
        "<div id=\"f98\"><p>fkeep98 <span id=\"sp98\">spkeep98</span></p></div>"
        "<p>fin98</p></body></html>"),
}


# Lot 99: reader page (tests/scripts/test_qemu_lot99.py); the guest sets
# the reader clock first (web-css time N)
LOT99_PAGES = {
    "/lot99/anim.html": (
        "<!doctype html><html><head><title>Lot 99 composition</title><style>"
        "@property --w99 { syntax: '<length>'; inherits: true; initial-value: 0px }\n"
        "@property --c99 { syntax: '<color>'; inherits: false; initial-value: black }\n"
        "@property --h99 { syntax: '<color>'; inherits: false; initial-value: black }\n"
        "@keyframes k99 { to { --w99: 20px } }\n"
        "@keyframes m99 { from { --c99: rgb(0, 0, 0) } to { --c99: rgb(200, 0, 0) } }\n"
        "#c99 { color: hsl(0.5turn 100% 50%) }\n"
        "#g99 { background: hsl(200grad 100% 25%) }\n"
        "#a99 { --w99: 10px; animation: k99 1000ms linear; animation-composition: add }\n"
        "#a99 span { font-size: var(--w99) }\n"
        "#n99 { --c99: rgb(0, 0, 200); color: var(--c99); animation: m99 1000ms linear; animation-timeline: none }\n"
        "#h99 { --h99: hsl(0.25turn 100% 50%); background-color: var(--h99) }\n"
        "</style></head><body><h1>Composition 99</h1>"
        "<div id=\"c99\"><p>ckeep99</p></div>"
        "<div id=\"g99\"><p>gkeep99</p></div>"
        "<div id=\"a99\"><p>akeep99 <span id=\"sp99\">spkeep99</span></p></div>"
        "<div id=\"n99\"><p>nkeep99</p></div>"
        "<div id=\"h99\"><p>hkeep99</p></div>"
        "<p>fin99</p></body></html>"),
}


class H(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, *a):
        pass

    def _ka_log(self):
        # Lot 72: client port per request (keep-alive reuse shows one port)
        print(f"KA port={self.client_address[1]} path={self.path}", flush=True)

    def do_GET(self):
        if self.path.startswith("/ka/"):
            self._ka_log()
            self._send(200, ("keep-alive answer %s\n" % self.path[4:]).encode(), "text/plain")
            return
        if self.path.startswith("/kastray/"):
            # Lot 89: an answer and a second one nobody asked for, in one
            # write so the client reads both together; the client must not
            # reuse this connection (its next request would read the 404)
            self._ka_log()
            body = ("keep-alive answer %s\n" % self.path[9:]).encode()
            stray = b"HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nnot found"
            head = ("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n" % len(body)).encode()
            self.wfile.write(head + body + stray)
            self.wfile.flush()
            return
        if self.path.startswith("/kaclose/"):
            # answers then closes: the client must open a new connection
            self._ka_log()
            body = b"closing\n"
            self.send_response(200)
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Connection", "close")
            self.end_headers()
            self.wfile.write(body)
            self.close_connection = True
            return
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

    def _lot87(self, p):
        """Lot 87 routes: external stylesheets and script walls."""
        sheets = {
            "/lot87/main.css": "@import url(base.css);\n.side { display: none }\nh1 { text-decoration: underline }\n",
            "/lot87/base.css": "p.ad { display: none }\nem.k { font-style: italic }\n",
            "/lot87/print.css": "body { display: none }\n",
            # cascade layers: reset is a layer, so its more specific rule
            # loses to the unlayered one; gone.css is never fetched
            "/lot87/layers.css": ("@layer reset, site;\n@import url(reset.css) layer(reset);\n"
                                  "@import url(gone.css) supports(not (display: block));\n"
                                  ".keep87 { display: block }\n"
                                  ".back87 { display: block }\np.back87 { display: revert-layer }\n"
                                  ".ini87 { display: initial }\n"),
            "/lot87/reset.css": "div.keep87 { display: none }\n.gone87 { display: none }\n.back87 { display: none }\n.ini87 { display: none }\n",
            "/lot87/gone.css": "body { display: none }\n",
        }
        if p in sheets:
            print("CSS %s" % p.rsplit("/", 1)[1], flush=True)
            self._send(200, sheets[p].encode("ascii"), "text/css")
            return True
        if p == "/lot87/page.html":
            page = ("<!doctype html><html><head><title>Lot 87 sheets</title>"
                    "<link rel=\"stylesheet\" href=\"main.css\">"
                    "<link rel=\"stylesheet\" href=\"print.css\" media=\"print\">"
                    "<style>@import \"missing.css\";\n.keep { color: #336699 }</style></head><body>"
                    "<h1>Feuilles externes</h1><p class=\"ad\">banner87</p>"
                    "<p class=\"keep\">Texte garde <em class=\"k\">important</em></p>"
                    "<div class=\"side\">sidenote87</div></body></html>")
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p == "/lot87/layers.html":
            page = ("<!doctype html><html><head><title>Lot 87 layers</title>"
                    "<link rel=\"stylesheet\" href=\"layers.css\"></head><body>"
                    "<h1>Couches</h1><div class=\"keep87\">layerkeep87 visible</div>"
                    "<p class=\"gone87\">layergone87</p><p class=\"back87\">layerback87</p><p class=\"ini87\">layerinitial87</p><p>fin87</p></body></html>")
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p in LOT90_PAGES:
            self._send(200, LOT90_PAGES[p].encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p in LOT91_PAGES:
            self._send(200, LOT91_PAGES[p].encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p in LOT92_PAGES:
            self._send(200, LOT92_PAGES[p].encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p in LOT93_PAGES:
            self._send(200, LOT93_PAGES[p].encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p in LOT94_PAGES:
            self._send(200, LOT94_PAGES[p].encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p in LOT95_PAGES:
            self._send(200, LOT95_PAGES[p].encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p in LOT96_PAGES:
            self._send(200, LOT96_PAGES[p].encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p in LOT97_PAGES:
            self._send(200, LOT97_PAGES[p].encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p in LOT98_PAGES:
            self._send(200, LOT98_PAGES[p].encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p in LOT99_PAGES:
            self._send(200, LOT99_PAGES[p].encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p == "/lot89/scope.html":
            # Lot 89: a start list and an end list, & and :scope with a
            # second pseudo-class (both used to lose the root mark). The end
            # list uses blockquote: the reader drops aside like nav and
            # footer, so an aside limit would prove nothing
            page = ("<!doctype html><html><head><title>Lot 89 scope lists</title><style>"
                    "@scope (.la89, .lb89) to (.keep89, blockquote) { p { display: none } }\n"
                    "@scope (.amp89) { &:first-child { display: none } }\n"
                    "@scope (.sc89) { :scope:last-child { display: none } }\n"
                    "</style></head><body><h1>Listes</h1>"
                    "<div class=\"la89\"><p>listhidea89</p><div class=\"keep89\"><p>listkeepa89</p></div></div>"
                    "<div class=\"lb89\"><p>listhideb89</p><blockquote><p>listkeepb89</p></blockquote></div>"
                    "<div class=\"lc89\"><p>listout89</p></div>"
                    "<section><div class=\"amp89\"><p>amphide89</p></div><div class=\"amp89\"><p>ampkeep89</p></div></section>"
                    "<section><div class=\"sc89\"><p>sckeep89</p></div><div class=\"sc89\"><p>schide89</p></div></section>"
                    "<p>fin89s</p></body></html>")
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p == "/lot89/stray.html":
            # Lot 89: what a buggy route did by accident: the page, then a
            # 404 nobody asked for, on the same kept-alive connection
            page = ("<!doctype html><html><head><title>Lot 89 stray</title></head>"
                    "<body><p>stray89</p></body></html>")
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            self.wfile.write(b"HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n"
                             b"Content-Length: 9\r\n\r\nnot found")
            self.wfile.flush()
            print("STRAY sent", flush=True)
            return True
        if p == "/lot89/stray1.html":
            # Lot 89: the page and an unasked 404 in one write, so the guest
            # reads them together and must not keep the connection
            page = ("<!doctype html><html><head><title>Lot 89 stray one</title></head>"
                    "<body><p>strayone89</p></body></html>").encode("utf-8")
            head = ("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                    "Content-Length: %d\r\n\r\n" % len(page)).encode()
            self.wfile.write(head + page + b"HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nnot found")
            self.wfile.flush()
            print("STRAY1 sent", flush=True)
            return True
        if p == "/lot89/vars.html":
            # Lot 89: custom properties per element
            page = ("<!doctype html><html><head><title>Lot 89 vars</title><style>"
                    ":root { --fs89: 14px }\n"
                    ".card89 { --fs89: 20px }\n"
                    ".card89 .big89 { --fs89: 24px }\n"
                    "@scope (.panel89) { --fs89: 22px }\n"
                    "p { font-size: var(--fs89); text-indent: var(--in89, 1px) }\n"
                    "</style></head><body>"
                    "<p id=root89>varroot89</p>"
                    "<div class=card89><div><p id=deep89>vardeep89</p></div>"
                    "<p id=big89 class=big89>varbig89</p></div>"
                    "<div class=panel89><p id=pan89>varpan89</p></div>"
                    "<div style=\"--in89: 5px\"><p id=inl89>varinl89</p></div>"
                    "<p>fin89v</p></body></html>")
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p == "/lot89/props.html":
            # Lot 89: math functions and var() in the reader's lengths
            page = ("<!doctype html><html><head><title>Lot 89 lengths</title><style>"
                    ":root { --b89: 1rem; --g89: var(--u89); --u89: 3px; --a89: var(--c89); --c89: var(--a89) }\n"
                    "#calc89 { font-size: calc(1rem + 4px); text-indent: min(2em, 150%) }\n"
                    "#clamp89 { font-size: clamp(1rem, 0.5rem + 1.3vw, 2rem); letter-spacing: max(1px, 0.5px * 4) }\n"
                    "#var89 { font-size: calc(var(--b89) + 2px); text-indent: var(--no89, var(--g89)) }\n"
                    "#cyc89 { font-size: 20px; font-size: var(--a89, 30px) }\n"
                    "#mix89 { text-indent: 1em; text-indent: calc(1em + 2px) }\n"
                    "</style></head><body><h1>Longueurs</h1>"
                    "<p id=\"calc89\">lencalc89</p><p id=\"clamp89\">lenclamp89</p><p id=\"var89\">lenvar89</p>"
                    "<p id=\"cyc89\">lencyc89</p><p id=\"mix89\">lenmix89</p><p>fin89p</p></body></html>")
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p == "/lot88/scope.html":
            # Lot 88: @scope with a limit, an implicit scope (the parent of
            # the <style> in body), declarations in the block, the all
            # shorthand and text-transform
            page = ("<!doctype html><html><head><title>Lot 88 scope</title><style>"
                    "@scope (.card88) to (.foot88) { p { display: none } :scope { text-transform: uppercase } }\n"
                    "p.all88 { display: none } p.all88 { all: revert }\n"
                    "</style></head><body><h1>Portee</h1>"
                    "<div class=\"card88\"><p>scopehide88</p><div class=\"foot88\"><p>scopekeep88</p></div></div>"
                    "<p>scopeout88</p><p class=\"all88\">allback88</p>"
                    "<div class=\"wrap88\"><style>@scope { em { display: none } }</style><em>implhide88</em></div>"
                    "<p><em>implkeep88</em> fin88</p></body></html>")
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p == "/lot88/props.html":
            # Lot 88 (chunk 4): five more properties the reader passes on,
            # an invalid list-style and an all: initial reset
            page = ("<!doctype html><html><head><title>Lot 88 props</title><style>"
                    "#big88 { font-size: 1.5em; letter-spacing: 2px; background: #FFEEDD }\n"
                    "#ind88 { text-indent: 2em }\n"
                    "ol.roman88 { list-style: upper-roman inside }\n"
                    "ul.bad88 { list-style: square sideways }\n"
                    "p.reset88 { font-size: 30px } p.reset88 { all: initial }\n"
                    "</style></head><body><h1>Proprietes</h1>"
                    "<p id=\"big88\">propbig88</p><p id=\"ind88\">propind88</p>"
                    "<ol class=\"roman88\" id=\"rom88\"><li>romi88</li><li>romii88</li></ol>"
                    "<ul class=\"bad88\" id=\"bad88\"><li>badli88</li></ul>"
                    "<p class=\"reset88\" id=\"res88\">propreset88</p><p>fin88p</p></body></html>")
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p == "/lot87/wall.html":
            page = ("<!doctype html><html><head><title>Wall</title><script>var x=1;</script></head><body>"
                    "<noscript><p>Please enable JavaScript or use the "
                    "<a href=\"/lot87/basic.html\">basic page</a>.</p></noscript></body></html>")
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p == "/lot87/basic.html":
            page = "<html><head><title>Basic</title></head><body><p>basic87 plain page</p></body></html>"
            self._send(200, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p == "/lot87/blocked":
            page = "<html><head><title>Blocked</title></head><body><p>You have been blocked by network security.</p></body></html>"
            self._send(403, page.encode("utf-8"), "text/html; charset=utf-8")
            return True
        return False

    def _lot71(self, p, body, post=False):
        """Lot 71 routes: forms, charsets, long page, SearXNG, cookies."""
        if self._lot87(p):
            return True
        if p == "/lot72/images.html":
            self._send(200, LOT72_PAGE.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p == "/lot73/anim.html":
            self._send(200, LOT73_PAGE.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p == "/lot73/pic.html":
            self._send(200, LOT73_PIC.encode("utf-8"), "text/html; charset=utf-8")
            return True
        if p == "/lot73/etag.txt":
            # revalidated every time (no-cache), 304 while the ETag matches
            if self.headers.get("If-None-Match") == '"lot73v1"':
                print("ETAG 304", flush=True)
                self.send_response(304)
                self.send_header("ETag", '"lot73v1"')
                self.end_headers()
                return True
            print("ETAG 200", flush=True)
            body = b"etag body lot73\n"
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("ETag", '"lot73v1"')
            self.send_header("Cache-Control", "no-cache")
            self.end_headers()
            self.wfile.write(body)
            return True
        if p == "/lot73/fresh.txt":
            print("FRESH GET", flush=True)
            body = b"fresh body lot73\n"
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "max-age=300")
            self.end_headers()
            self.wfile.write(body)
            return True
        if p.startswith("/lot73/img/"):
            name = p[len("/lot73/img/"):]
            print(f"IMG {name} port={self.client_address[1]}", flush=True)
            ent = LOT73_FILES.get(name)
            if ent is None or ent[0] not in _IMG:
                self._send(404, b"no image\n", "text/plain")
            else:
                self._send(200, _IMG[ent[0]], ent[1])
            return True
        if p == "/lot72/cp1251.html":
            page = ("<html><head><title>\u041d\u043e\u0432\u043e\u0441\u0442\u0438</title></head><body>"
                    "<p>\u041f\u0440\u0438\u0432\u0435\u0442, \u043c\u0438\u0440! \u20ac 5</p>"
                    "<form action=\"/echo\" method=\"get\"><input type=\"text\" name=\"q\" "
                    "value=\"\u041c\u0438\u0440\"><input type=\"submit\" value=\"OK\"></form></body></html>")
            self._send(200, page.encode("cp1251"), "text/html; charset=windows-1251")
            return True
        if p == "/lot72/greek.html":
            page = ("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-7\">"
                    "<title>\u0395\u03b9\u03b4\u03ae\u03c3\u03b5\u03b9\u03c2</title></head><body>"
                    "<p>\u039a\u03b1\u03bb\u03b7\u03bc\u03ad\u03c1\u03b1 \u03ba\u03cc\u03c3\u03bc\u03b5</p></body></html>")
            self._send(200, page.encode("iso8859_7"), "text/html")
            return True
        if p.startswith("/lot72/img/") or p == "/favicon.ico":
            name = "fav.ico" if p == "/favicon.ico" else p[len("/lot72/img/"):]
            print(f"IMG {name} port={self.client_address[1]}", flush=True)
            ent = LOT72_FILES.get(name)
            if ent is None or ent[0] not in _IMG:
                self._send(404, b"no image\n", "text/plain")
            else:
                self._send(200, _IMG[ent[0]], ent[1])
            return True
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
    # RSA-PSS CertificateVerify), sp+3 TLS 1.2 ECDHE-RSA with P-256 only.
    # Lot 72: sp+4 TLS 1.2 ECDSA leaf and sp+5 TLS 1.2 RSA leaf with the
    # AES-256-GCM-SHA384 and ChaCha20-Poly1305 suites (P-256 on sp+5).
    extra = []
    for off, ver, chain, key, curve, ciphers in (
            (1, "1.3", "server_chain", "server_key", None, None),
            (2, "1.3", "server_rsa_chain", "server_rsa_key", None, None),
            (3, "1.2", "server_rsa_chain", "server_rsa_key", "prime256v1", "ECDHE-RSA-AES128-GCM-SHA256"),
            (4, "1.2", "server_chain", "server_key", None,
             "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256"),
            (5, "1.2", "server_rsa_chain", "server_rsa_key", "prime256v1",
             "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-RSA-AES128-GCM-SHA256")):
        c = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        if ver == "1.3":
            c.minimum_version = ssl.TLSVersion.TLSv1_3
        else:
            c.minimum_version = c.maximum_version = ssl.TLSVersion.TLSv1_2
            c.set_ciphers(ciphers)
        if curve:
            c.set_ecdh_curve(curve)
        c.load_cert_chain(f"{pki}/{chain}.pem", f"{pki}/{key}.pem")
        srv = S((bind, sp + off), H)
        srv.socket = c.wrap_socket(srv.socket, server_side=True)
        threading.Thread(target=srv.serve_forever, daemon=True).start()
        extra.append(sp + off)
    print(f"READY http={hp} https={sp} tls13={extra[0]} tls13rsa={extra[1]} p256={extra[2]} tls12ec={extra[3]} tls12rsa={extra[4]}", flush=True)
    tls.serve_forever()


if __name__ == "__main__":
    main()
