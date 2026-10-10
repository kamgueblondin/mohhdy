#!/usr/bin/env python3
"""Noyau par defaut (strict, NET_RING0_FALLBACK=0) : le noyau ne sert plus aucun
syscall socket/LLM/peer/wire depuis sa pile Ring 0.

Contrat :
1. networker (Ring 3) possede la NE2000 et sert seul DHCP/DNS/TCP/TLS/HTTP
   (meme contrat que qemu-net-tls-worker), le serveur pair (ecoute 128 et
   poll TLS 130) et les sockets TCP (netrelay, boucle locale ping/pong
   relayee au worker) ;
2. le worker lui-meme voit la pile socket noyau refusee (-59) ;
3. networker tue : socket et LLM renvoient -59 (aucun repli Ring 0), le
   compteur denied monte, aucune op relayee ni execution noyau.
"""
import os
import re
import time
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_net_tls_worker as tw  # noqa: E402

tw.LOG = os.path.join(tw.LOG_DIR, "net-no-ring0%s.log" % tw.RUN_SUFFIX)
tw.ERR = os.path.join(tw.LOG_DIR, "net-no-ring0%s.err" % tw.RUN_SUFFIX)
tw.MON = os.path.join(tw.LOG_DIR, "net-no-ring0%s-monitor.sock" % tw.RUN_SUFFIX)

BANNER = "[NET] build NET_RING0_FALLBACK=0"
STATUS_RE = (r"net-relay ok worker (-?\d+) fwd (\d+) done (\d+) aborted (\d+) "
             r"timeouts (\d+) denied (\d+) stale (\d+) pending (\d+) end")


def command(proc, client, line, marker, timeout=20):
    start = tw.send_command(client, proc, line)
    tw.wait_for(proc, marker, timeout, start)
    tw.wait_for_prompt(proc, start)
    return start


def yield_until(proc, client, needle, start, rounds=40):
    for _ in range(rounds):
        if needle in tw.normalized_log(tw.text()[start:]):
            return
        command(proc, client, "yield", "yield ok", 15)
    if needle not in tw.normalized_log(tw.text()[start:]):
        raise RuntimeError("missing %r: %s" % (needle, tw.text()[-1500:]))


def relay_status(proc, client):
    start = command(proc, client, "net-relay-status", "net-relay ok worker")
    match = re.search(STATUS_RE, tw.normalized_log(tw.text()[start:]))
    if not match:
        raise RuntimeError("unparsable net-relay-status")
    return [int(v) for v in match.groups()]


def spawn(proc, client, name):
    start = command(proc, client, "spawn %s" % name, "spawn ok pid")
    # "spawn ok pid", the pid and the name are printed by separate writes:
    # the marker can be in the log before the rest, so wait for the line.
    deadline = time.monotonic() + 10
    while True:
        match = re.search(r"spawn ok pid[\s\S]*?(\d+) %s" % name,
                          tw.normalized_log(tw.text()[start:]))
        if match or time.monotonic() > deadline:
            break
        time.sleep(0.1)
    if not match:
        raise RuntimeError("%s not spawned" % name)
    return match.group(1), start


def main():
    peer = proc = client = None
    try:
        peer, proc = tw.start_guest("mohhdy.bin")
        tw.wait_for(proc, "(-.-)")
        client = tw.monitor()
        boot = tw.normalized_log(tw.text())
        if BANNER not in boot:
            raise RuntimeError("strict build banner absent (wrong kernel?)")
        if "net-driver kernel socket stack absent, ring3 only" not in boot:
            raise RuntimeError("worker still reaches the kernel socket stack")
        tw.wait_ring3_stack(proc, client, spawn_extra=False)

        # 1. TLS/HTTP through networker only.
        tls_ops = tw.run_tls_http(proc, client, peer)

        # 1b. Peer TLS server 128/130 in the worker (lease from step 1): the
        #     listen succeeds and the TLS poll (no client yet) is answered
        #     by the worker, not by the kernel peer server.
        start = command(proc, client, "ai-peer-listen", "ai-peer-listen:")
        command(proc, client, "ai-peer-tls-poll", "ai-peer-tls-poll", 30)
        chunk = tw.normalized_log(tw.text()[start:])
        if "ai-peer-listen: LISTEN" not in chunk:
            raise RuntimeError("peer listen failed through networker: %s" % chunk[-800:])
        for op in (128, 130):
            if not re.search(r"net-driver relay op %d rc -?\d+ reply 0" % op, chunk):
                raise RuntimeError("peer op %d not run by the worker" % op)

        # 2. TCP sockets (open/listen/handshake/send/feed/receive/close) served
        #    by the worker's Ring 3 registry.
        pid, start = spawn(proc, client, "netrelay")
        yield_until(proc, client, "netrelay forged reply refused", start)
        chunk = tw.normalized_log(tw.text()[start:])
        match = re.search(r"netrelay tcp loopback ok ping pong mode relay forwarded (\d+) "
                          r"completed (\d+)", chunk)
        if not match or match.group(1) != match.group(2) or int(match.group(1)) < 10:
            raise RuntimeError("socket loopback not served by networker: %s" % chunk[-1200:])
        sockets = int(match.group(1))
        for op in (99, 100, 101, 102, 103, 104, 105, 106, 107, 108):
            if not re.search(r"net-driver relay op %d rc -?\d+ reply 0" % op, chunk):
                raise RuntimeError("socket op %d not run by the worker" % op)
        command(proc, client, "kill %s" % pid, "Processus %s termine" % pid)
        if "[NET] ring0 fallback absent" in tw.normalized_log(tw.text()[len(boot):]):
            raise RuntimeError("a network syscall fell to Ring 0 while the worker was live")

        # 3. Kill networker: no Ring 0 fallback.
        live = relay_status(proc, client)
        worker = live[0]
        if worker <= 0:
            raise RuntimeError("no live net-driver before kill")
        command(proc, client, "kill %d" % worker, "Processus %d termine" % worker)
        mark = len(tw.text())
        pid, start = spawn(proc, client, "netrelay")
        yield_until(proc, client, "netrelay forged reply refused", start)
        chunk = tw.normalized_log(tw.text()[start:])
        if "netrelay tcp loopback failed at open rc -59" not in chunk:
            raise RuntimeError("socket did not fail closed without worker: %s" % chunk[-1200:])
        command(proc, client, "kill %s" % pid, "Processus %s termine" % pid)
        command(proc, client, "ai-acquire example.com", "ai-acquire:")
        after = tw.normalized_log(tw.text()[mark:])
        if "ai-acquire: DHCP, DNS et SYN LLM demarres" in after:
            raise RuntimeError("LLM acquire ran without the worker")
        for number in (99, 91):
            if "[NET] ring0 fallback absent: syscall %d refused (-59)" % number not in after:
                raise RuntimeError("syscall %d not refused by the strict gate" % number)
        if "net-driver relay op" in after:
            raise RuntimeError("relay op after the worker was killed")
        dead = relay_status(proc, client)
        if dead[0] != 0 or dead[5] <= live[5]:
            raise RuntimeError("unexpected counters after kill: %r -> %r" % (live, dead))
        refused = len(re.findall(r"\[NET\] ring0 fallback absent: syscall \d+", after))
        print("net no-ring0: %d LLM ops + %d socket ops served by networker; "
              "after kill %d syscalls refused (-59), denied %d -> %d" %
              (tls_ops, sockets, refused, live[5], dead[5]))
        print("QEMU strict network build (no Ring 0 fallback) contract passed.")
        return 0
    finally:
        if peer is not None:
            tw.stop_guest(peer, proc, client)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU strict network build (no Ring 0 fallback) contract failed: %s" % error)
        raise SystemExit(1)
