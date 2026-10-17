#!/usr/bin/env python3
"""Reprise sur perte TCP/TLS du client invite (noyau strict, networker Ring 3).

Le pair Ethernet controle perd volontairement des trames et en duplique :
1. le premier ClientHello invite est ignore (perdu) : seule la
   retransmission invite (RTO par sondage, backoff) peut relancer la poignee ;
2. le premier Certificate serveur n'est pas emis : le pair le reemet par RTO
   (comme TCP) pendant que l'invite attend sans echouer ;
3. le ServerKeyExchange est emis deux fois : l'invite ignore le doublon
   (ACK duplique) au lieu d'echouer la poignee ;
puis TLS 1.2 complet et HTTP 200 passent par le meme chemin que
qemu-net-no-ring0.
"""
import functools
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_net_tls_worker as tw  # noqa: E402
from qemu_ne2k_controlled_peer import ControlledEthernetPeer  # noqa: E402

tw.LOG = os.path.join(tw.LOG_DIR, "net-loss-recovery%s.log" % tw.RUN_SUFFIX)
tw.ERR = os.path.join(tw.LOG_DIR, "net-loss-recovery%s.err" % tw.RUN_SUFFIX)
tw.MON = os.path.join(tw.LOG_DIR, "net-loss-recovery%s-monitor.sock" % tw.RUN_SUFFIX)
tw.ControlledEthernetPeer = functools.partial(
    ControlledEthernetPeer, loss_plan=("client_hello", "certificate"),
    duplicate_plan=("server_key_exchange",))


def main():
    peer = proc = client = None
    try:
        peer, proc = tw.start_guest("mohhdy.bin")
        tw.wait_for(proc, "[0m : ")
        client = tw.monitor()
        tw.wait_ring3_stack(proc, client, spawn_extra=False)
        tls_ops = tw.run_tls_http(proc, client, peer)
        events = dict(peer.events)
        for name in ("dropped_client_hello", "dropped_certificate",
                     "duplicated_server_key_exchange", "peer_rto"):
            if events.get(name, 0) < 1:
                raise RuntimeError("loss scenario incomplete, %s absent: %r" % (name, events))
        if events.get("client_hello_frames", 0) < 2:
            raise RuntimeError("guest never retransmitted its ClientHello: %r" % events)
        if events.get("client_hello", 0) != 1:
            raise RuntimeError("ClientHello served %r times" % events.get("client_hello"))
        print("net loss recovery: ClientHello lost then retransmitted by the guest "
              "(%d ClientHello frames), Certificate lost then resent by peer RTO (%d), "
              "duplicate ServerKeyExchange ignored; TLS+HTTP ok, %d relayed ops" %
              (events["client_hello_frames"], events["peer_rto"], tls_ops))
        print("QEMU TCP/TLS loss recovery contract passed.")
        return 0
    finally:
        if peer is not None:
            tw.stop_guest(peer, proc, client)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU TCP/TLS loss recovery contract failed: %s" % error)
        raise SystemExit(1)
