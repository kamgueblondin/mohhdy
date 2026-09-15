# AOS-122 — Codec IPv4/UDP caller-owned

Le lot AOS-122 ajoute le checksum IPv4 et la construction/parsing bornés de datagrammes UDP sur IPv4. Les adresses, ports, protocole, longueur totale et checksum d’en-tête sont vérifiés sans allocation dynamique. Le checksum UDP reste explicitement nul, ce qui est autorisé pour UDP sur IPv4 et sera complété lors de l’intégration du pseudo-en-tête et du transport réel.

Le test `test_net_ipv4_udp.c` couvre la construction, le parsing, les ports, le payload et le rejet d’un en-tête modifié. La validation locale est de **278 tests verts**, avec build i386 et initrd réussis. Le codec ne transmet encore aucun paquet: il constitue la couche de représentation avant le raccordement au NE2000 et au DHCP.
