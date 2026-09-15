# AOS-147 — SYN TCP IPv4 sur NE2000

Le lot AOS-147 ajoute la construction d’un segment SYN TCP encapsulé dans IPv4. Le paquet est écrit dans un buffer caller-owned, avec ports, numéro de séquence, TTL et protocole TCP explicitement définis. Le checksum TCP inclut le pseudo-en-tête IPv4 ; le checksum IPv4 est ensuite calculé sur l’en-tête construit.

`ne2k_tcp_syn` réutilise le cache ARP caller-owned et le chemin de résolution borné. Lorsque la MAC distante est connue, il construit l’en-tête Ethernet et soumet la trame par TX PIO NE2000. Le lot ne conserve aucun buffer ni état global de connexion.

| Élément | État AOS-147 |
|---|---|
| Construction SYN IPv4 | Implémentée. |
| Checksum TCP pseudo-en-tête | Implémenté. |
| Émission Ethernet NE2000 | Implémentée avec cache ARP. |
| Handshake SYN/SYN-ACK/ACK | Non implémenté. |
| État de connexion TCP persistant | Non implémenté. |
| Retransmission et temporisation | Non implémentées. |
| TLS, HTTP et OpenAI | Toujours non déclarés fonctionnels sur le transport matériel. |

La validation locale reste à **294 tests verts**, avec build i386 réussi. Les smokes QEMU existants valident le boot et la détection NE2000, mais n’injectent pas encore de SYN distant.
