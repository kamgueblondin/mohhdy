# AOS-139 — Service ARP NE2000 RX→réponse→TX

Le lot AOS-139 ajoute `ne2k_arp_service`, une orchestration bornée qui traite au plus une trame reçue. Le pilote effectue le polling RX et le décodage Ethernet/ARP, vérifie que la requête est une demande ARP destinée à l’IPv4 locale, construit la réponse dans un second buffer caller-owned, puis la soumet au TX PIO NE2000.

L’orchestrateur ne conserve aucun pointeur vers les buffers appelant et ne fait aucune allocation. Les buffers RX et TX sont distincts afin d’éviter toute écrasement de la trame reçue pendant la construction de la réponse. L’absence de paquet est retournée sans émission ; les trames non ciblées ou d’un type non pris en charge sont ignorées.

| Élément | État AOS-139 |
|---|---|
| RX NE2000 → parse Ethernet/ARP | Raccordé. |
| Filtrage requête ARP ciblant l’IPv4 locale | Raccordé. |
| Construction réponse caller-owned | Raccordée. |
| Soumission TX PIO caller-owned | Raccordée. |
| Gestion d’une trame ARP réelle injectée sous QEMU | Smoke dédié encore à ajouter. |
| IPv4/UDP, DHCP, DNS, TCP, TLS, HTTP/OpenAI | Toujours non déclarés fonctionnels sur le transport matériel. |

La non-régression locale reste à **293 tests verts**, avec build i386 réussi. Les smokes QEMU existants vérifient le boot avec et sans NE2000, mais ne constituent pas encore une preuve de bout en bout de réponse ARP sur le réseau virtuel.
