# AOS-148 — Réception TCP caller-owned sur NE2000

Le lot AOS-148 ajoute `ne2k_rx_poll_tcp`. La primitive lit une trame depuis la RAM distante NE2000, vérifie l’EtherType IPv4, valide la longueur de l’en-tête IPv4 et filtre le protocole TCP avant d’exposer une `net_tcp_view_t` directement dans le buffer caller-owned.

Aucune copie de payload n’est réalisée et aucun état de connexion n’est conservé. Les codes distinguent l’absence de paquet, une trame non IPv4, un en-tête IPv4 invalide et un segment TCP mal formé.

| Élément | État AOS-148 |
|---|---|
| Polling RX Ethernet/IPv4/TCP | Implémenté. |
| Vue TCP caller-owned | Implémentée. |
| Filtrage protocole TCP | Implémenté. |
| Validation checksum TCP reçu | À ajouter dans un lot dédié. |
| Validation SYN-ACK et état de connexion | À ajouter. |
| ACK, retransmission et temporisation | Non implémentés. |
| TLS, HTTP et OpenAI | Toujours non déclarés fonctionnels sur le transport matériel. |

La validation locale reste à **294 tests verts**, avec build i386 réussi. Les smokes QEMU existants valident le boot et la détection NE2000, mais n’injectent pas encore de segment TCP réel.
