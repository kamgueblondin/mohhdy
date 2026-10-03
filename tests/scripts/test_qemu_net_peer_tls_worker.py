#!/usr/bin/env python3
"""Guest-to-guest TLS 1.2 handshake + encrypted peer chat through networker
on the default strict kernel (NET_RING0_FALLBACK=0).

Reuses the two-guest harness of test_qemu_ne2k_guest_tls_server.py (two QEMU
TCG on SharedEthernetHub, no hub proxy, no TAP, no Internet, no secret) with
METIER=1, and adds the Ring 3 evidence:

1. Both guests boot the strict kernel ("[NET] build NET_RING0_FALLBACK=0")
   and spawn networker at boot; the worker reports "kernel socket stack
   absent, ring3 only" and takes the NE2000 ports.
2. B: listen (op 128 rc 0), accept (op 129 rc 1 SYN-ACK, then rc 0
   ESTABLISHED), TLS server role (op 130 rc 1 ServerHello .. rc 7
   Finished), METIER reply (op 130 rc 10). A: LLM/TLS client steps
   (op 91/92) then METIER (op 130 rc 12 "facture emis", rc 10 "ok"). Every
   one of these shell syscalls is answered by the worker
   ("net-driver relay op N rc R"); no Ring 0 network refusal is logged once
   the shell starts (only the worker's boot self-check).
3. net-relay-status on both guests: the live worker is the boot networker,
   fwd == done, aborted 0, timeouts 0, pending 0.
4. Hub (wire view, no crypto): guest ServerHello, server Finished/CCS, at
   least one application_data record in each direction, and no guest-to-
   guest TCP payload carrying the METIER plaintext (the chat is encrypted).
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
os.environ["METIER"] = "1"
import test_qemu_ne2k_guest_tls_server as h  # noqa: E402

STRICT_BANNER = "[NET] build NET_RING0_FALLBACK=0"
RELAY_RE = re.compile(r"net-driver relay op (\d+) rc (-?\d+) reply (-?\d+)")
STATUS_RE = re.compile(r"net-relay ok worker (-?\d+) fwd (\d+) done (\d+) aborted (\d+) "
                       r"timeouts (\d+) denied (\d+) stale (\d+) pending (\d+) end")


def relays(text):
    return [(int(op), int(rc), int(rep)) for op, rc, rep in RELAY_RE.findall(text)]


def check_guest(inst, run_command, peer_alive_check):
    text = h.normalized_log(h.log_text(inst["log"]))
    label = inst["label"]
    for needle in (STRICT_BANNER, "[NET] boot networker spawned",
                   "net-driver kernel socket stack absent, ring3 only",
                   "[NET] NE2000 ports 0x300-0x31F handed to the Ring 3 worker"):
        if needle not in text:
            raise RuntimeError("guest %s: missing %r" % (label, needle))
    first_relay = text.find("net-driver relay op")
    if first_relay < 0:
        raise RuntimeError("guest %s: no relayed network syscall" % label)
    late = text[first_relay:]
    if "[NET] ring0 fallback absent" in late or "refused (-59)" in late:
        raise RuntimeError("guest %s: Ring 0 network refusal after the shell started: %s"
                           % (label, late[late.find("refused"):][:200]))
    rel = relays(text)
    if any(rep != 0 for _, _, rep in rel):
        raise RuntimeError("guest %s: relay reply error %r" % (label, rel))
    start = run_command(inst, "net-relay-status", "net-relay ok", peer_alive_check, timeout=30)
    m = STATUS_RE.search(h.normalized_log(h.log_text(inst["log"])[start:]))
    if not m:
        raise RuntimeError("guest %s: net-relay-status unreadable" % label)
    worker, fwd, done, aborted, timeouts, _denied, _stale, pending = (int(v) for v in m.groups())
    spawn = re.search(r"spawn ok pid[\s\S]*?(\d+) networker", text)
    if worker <= 0 or (spawn and int(spawn.group(1)) != worker):
        raise RuntimeError("guest %s: live worker %d is not the boot networker (%r)"
                           % (label, worker, spawn.group(0) if spawn else None))
    if fwd != done or aborted or timeouts or pending or fwd < len(rel):
        raise RuntimeError("guest %s: relay counters %s (relayed lines %d)"
                           % (label, m.group(0), len(rel)))
    return rel, m.group(0)


def post_check(instances, run_command, peer_alive_check, hub):
    guest_a, guest_b = instances
    rel_a, st_a = check_guest(guest_a, run_command, peer_alive_check)
    rel_b, st_b = check_guest(guest_b, run_command, peer_alive_check)
    ops_b = [(op, rc) for op, rc, _ in rel_b]
    ops_a = [(op, rc) for op, rc, _ in rel_a]
    for need in ((128, 0), (129, 1), (129, 0), (130, 1), (130, 7), (130, 10)):
        if need not in ops_b:
            raise RuntimeError("guest b: relayed %r missing from %r" % (need, ops_b))
    for need in ((91, 0), (130, 12), (130, 10)):
        if need not in ops_a:
            raise RuntimeError("guest a: relayed %r missing from %r" % (need, ops_a))
    if not any(op == 92 for op, _ in ops_a):
        raise RuntimeError("guest a: no TLS client poll (op 92) relayed: %r" % ops_a)
    ev = hub.events
    if ev["guest_server_hello"] < 1 or ev["guest_server_finished"] < 1 or \
            ev["guest_app_data"] < 1 or ev["guest_client_app_data"] < 1:
        raise RuntimeError("hub: TLS records missing %r" % ev)
    if ev["guest_plaintext_metier"] != 0:
        raise RuntimeError("hub: METIER plaintext seen on the wire %r" % ev)
    h.say("[net-peer-tls-worker] a: %s; relayed ops %s" % (st_a, sorted(set(ops_a))))
    h.say("[net-peer-tls-worker] b: %s; relayed ops %s" % (st_b, sorted(set(ops_b))))
    h.say("[net-peer-tls-worker] wire: client app records %d, server app records %d, "
          "plaintext METIER frames %d" % (ev["guest_client_app_data"], ev["guest_app_data"],
                                          ev["guest_plaintext_metier"]))


def main():
    h.POST_CHECK = post_check
    rc = h.main()
    if rc == 0:
        print("QEMU guest-guest TLS + encrypted peer chat through networker "
              "(strict kernel) passed", flush=True)
    return rc


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        sys.stderr.write("QEMU net peer TLS worker contract failed: %s\n" % error)
        raise SystemExit(1)
