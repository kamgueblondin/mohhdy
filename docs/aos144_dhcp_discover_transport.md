# AOS-144 — DHCP Discover sur le transport NE2000

Le lot AOS-144 raccorde `net_dhcp_build_discover` au transport Ethernet/IPv4/UDP du pilote NE2000. `ne2k_dhcp_discover` construit une requête DHCP Discover avec la MAC locale, l’embarque dans un paquet UDP source port 68 vers port 67, utilise les adresses IPv4 0.0.0.0 et 255.255.255.255, puis diffuse la trame Ethernet via la MAC broadcast.

Le chemin reste entièrement caller-owned. Le payload DHCP est construit directement après les en-têtes IPv4/UDP afin d’éviter tout chevauchement entre la zone source et la zone destination du constructeur UDP. La capacité minimale est vérifiée avant écriture et le TX NE2000 conserve son padding Ethernet et ses limites existantes.

| Élément | État AOS-144 |
|---|---|
| DHCP Discover caller-owned | Implémenté. |
| Diffusion Ethernet/IP/UDP | Implémentée. |
| Ports DHCP 68 → 67 | Implémentés. |
| Parsing d’offre DHCP | Codec existant validé séparément. |
| Réception d’offre via NIC et état de bail | Prochain sous-lot. |
| Configuration IPv4 effective dans le noyau | Non déclarée. |
| DNS, TCP, TLS, HTTP/OpenAI | Toujours non déclarés fonctionnels sur le transport matériel. |

La validation locale atteint **294 tests verts**, avec build i386 réussi. Les smokes QEMU de boot et de détection NE2000 restent nécessaires avant publication.
