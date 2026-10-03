# Rapport Tranche 5 pile : ARP/IPv4/TCP/TLS en Ring 3 (27 septembre 2026)

Branche `feat/tranche4-5-ring3-drivers`, build arm64 croisé (`i686-linux-gnu-gcc`, nasm).

## Résultats

| Contrat | Résultat |
|---|---|
| `make test-all` (unitaires sous `qemu-i386`) | 51/51 binaires, 612/612 tests (605 existants conservés + 7 nouveaux) ; registre commandes 198 OK ; façade Python absente OK |
| `make qemu-smoke` | 6/6 |
| `make qemu-net-wire` | OK : 7 trames construites en Ring 3 = 7 reçues par le pair, 4 décodées, 0 pump noyau, reprise + re-claim |
| `make qemu-net-worker` | OK : 18 relais (16 socket + 2 LLM répondus -94 par le client TLS Ring 3), peer -59, 1 timeout (worker suspendu), 1 abort (worker tué) |
| `make qemu-net-tls-worker` (nouveau) | OK en 42 s : DHCP/DNS/TCP/TLS 1.2 authentifié/HTTP 200 exécutés par `networker`, 15 syscalls LLM relayés |
| `make qemu-ne2k-tls-http` (repli Ring 0) | OK, sans régression après extraction de la session LLM |
| `make integration-qemu` | 7/7 en 813 s (13 min 33 s) |

## Ce qui est fait

- `kernel/net_llm_client.c/.h` : session LLM (DHCP, DNS, TCP, TLS 1.2, HTTP, SSE) sortie telle quelle de `kernel.c`, liée au noyau (repli) et au worker (`-DMOHHDY_RING3`, UTC via `SYS_NET_NIC` UTC).
- `kernel/net_stack_exec.c/.h` : exécuteur partagé d'un appel relayé (socket 99-108, connect/send/recv/close fil, LLM 91-98) sur la pile liée localement ; boucle fil pilotée par callbacks (emit/poll/idle/ack).
- `networker` lie toute la pile (registre sockets, machine TCP, framing/demux ARP/IPv4/TCP, TLS, HTTP) et l'exécute à CPL 3 dès qu'il détient la NE2000 ; sans carte, comportement précédent inchangé. Lignes de log atomiques (`SYS_NET_NIC` LOG) : plus de ligne coupée par une préemption (flake observé sur la base).
- Noyau : relais LLM 91-98 quand le worker détient la carte, canal bulk `SYS_NET_RELAY_BULK` (147), délai 60 s pour les ops TLS ; `SYS_NET_NIC` PUBLISH/STACK ; `SYS_NET_WIRE_STATUS` fusionne les compteurs Ring 3 (base retraitée à la perte du worker) ; `SYS_LLM_SESSION_STATUS` renvoie le mot de session Ring 3.
- Tests : `test_net_stack_exec` (5), `test_net_relay` + canal bulk, `test_net_nic_owner` + publication ; `netclaim`/`netrelay` et `test_qemu_net_worker.py` adaptés au nouveau contrat LLM (relayé au lieu de -59, compteurs 16 → 18), `test_qemu_net_wire.py` vérifie la ligne pile Ring 3.

## Ce qui reste

- Serveur TLS invite-invite (peer 128-130) : les appels du shell sont relayes au worker (`OS_IPC_NET_PEER_RELAY_REQUEST`), qui les execute dans sa pile Ring 3 ; le chemin noyau direct reste refuse (-141) s'il n'a pas ete relaye.
- Repli sans worker (session LLM et moteur fil noyau) et sonde au boot restent Ring 0 par conception. `networker` est lance au boot quand la sonde NE2000 a vu une carte (`[NET] boot networker spawned`, `kernel/kernel.c`).
- `qemu-net-tls-worker` est hors `integration-qemu` (7 contrats conservés) ; le libellé shell « Session LLM noyau » est conservé pour la compatibilité des contrats.
