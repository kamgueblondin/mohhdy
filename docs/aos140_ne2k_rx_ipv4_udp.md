# AOS-140 — Raccordement RX NE2000 vers IPv4/UDP

Le lot AOS-140 ajoute `ne2k_rx_poll_udp`. La primitive réutilise le polling RX NE2000, décode l’en-tête Ethernet, filtre l’EtherType IPv4 puis transmet uniquement le paquet IP à `net_udp_parse_ipv4`. La vue UDP expose les adresses, ports et payload dans le buffer caller-owned ; aucune copie de payload et aucune allocation dynamique ne sont effectuées.

Les retours distinguent l’absence de paquet, une trame non-IPv4 et un paquet IPv4/UDP invalide. Le parseur existant continue de vérifier la longueur totale et le checksum IPv4. Les règles Makefile et le runner CI lient explicitement `net_ipv4_udp.c` au test NE2000 pour éviter qu’une dépendance de production soit masquée par une compilation partielle.

| Élément | État AOS-140 |
|---|---|
| RX PIO NE2000 vers Ethernet | Réutilisé. |
| Filtrage IPv4 | Implémenté. |
| Parse UDP caller-owned | Implémenté. |
| Émission IPv4/UDP via TX | Non orchestrée dans ce lot. |
| DHCP/DNS sur transport matériel | Non déclarés fonctionnels. |
| TCP/TLS/HTTP/OpenAI | Non déclarés fonctionnels. |

La suite locale reste à **293 tests verts**, avec build i386 réussi. Les smokes QEMU existants restent des vérifications de boot et de détection NE2000 ; ils ne démontrent pas encore un échange UDP réel.
