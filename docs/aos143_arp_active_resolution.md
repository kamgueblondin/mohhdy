# AOS-143 — Résolution ARP active caller-owned

Le lot AOS-143 ajoute `ne2k_arp_resolve`. La primitive consulte d’abord le cache ARP statique. En cas d’absence, elle construit une requête ARP dans un buffer caller-owned, la transmet par TX PIO, puis effectue un nombre d’essais RX borné. Chaque réponse reçue est décodée par le chemin Ethernet/ARP existant ; seule une réponse destinée à l’IPv4 locale et correspondant à la cible demandée alimente le cache.

`ne2k_tx_udp_resolve` réutilise ensuite cette résolution pour retrouver la MAC destination avant d’appeler `ne2k_tx_udp`. Les buffers de requête ARP, réception, émission UDP et cache restent fournis par l’appelant. Une absence de réponse retourne une erreur explicite après la limite d’essais, sans boucle infinie ni allocation dynamique.

| Élément | État AOS-143 |
|---|---|
| Lookup cache avant émission ARP | Implémenté. |
| Requête ARP caller-owned | Implémentée. |
| Attente RX bornée | Implémentée. |
| Validation et alimentation du cache | Implémentées. |
| Émission UDP après résolution | Intégrée. |
| Échange ARP réel observable sous QEMU | Non déclaré par les smokes actuels. |
| DHCP/DNS/TCP/TLS/HTTP/OpenAI | Toujours non déclarés fonctionnels sur le transport matériel. |

La validation locale reste à **294 tests verts**, avec build i386 réussi.
