# AOS-137 — Raccordement RX NE2000 vers Ethernet/ARP

Le lot AOS-137 relie le polling RX PIO NE2000 aux codecs Ethernet et ARP caller-owned. `ne2k_rx_poll_arp` lit une trame dans le buffer fourni par l’appelant, décode l’en-tête Ethernet, distingue les trames non-ARP et parse les paquets ARP Ethernet/IPv4 valides dans une structure de sortie sans pointeur vers la trame.

Les codes de retour séparent l’absence de paquet, une trame non-ARP et une trame Ethernet/ARP invalide. La chaîne n’alloue aucune mémoire et ne conserve aucun buffer matériel. La règle du runner Unity a également été mise à jour pour lier `net_ethernet_arp.c` au test NE2000 ; cette correction est nécessaire pour que la CI compile réellement le nouveau chemin.

| Élément | État AOS-137 |
|---|---|
| Polling RX PIO NE2000 | Réutilisé. |
| Décodage Ethernet | Raccordé. |
| Décodage ARP Ethernet/IPv4 | Raccordé. |
| Buffer et sorties caller-owned | Respectés. |
| Runner Unity et dépendances | Corrigés. |
| Réponse ARP automatique | Non implémentée. |
| Émission ARP via TX | Préparée par AOS-134, non orchestrée ici. |
| IPv4/UDP/DHCP/DNS/TCP/TLS/HTTP/OpenAI | Non déclarés fonctionnels sur le transport matériel. |

La non-régression locale est de **292 tests verts**, avec build i386 réussi. La validation QEMU confirme le boot avec et sans matériel NE2000, mais ne prétend pas encore injecter puis observer une requête ARP de bout en bout.
