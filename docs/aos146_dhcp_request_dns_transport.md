# AOS-146 — DHCP REQUEST/ACK et DNS sur UDP

Le lot AOS-146 complète le chemin DHCP avec la construction d’un DHCP REQUEST et le parsing borné d’un DHCP ACK. La requête reprend le XID, la MAC locale, l’adresse IPv4 proposée et l’identifiant du serveur. L’ACK est accepté uniquement si le message, le XID, le cookie et l’option serveur sont cohérents ; l’adresse offerte est alors copiée dans un bail caller-owned.

Le même lot ajoute `ne2k_dns_query` et `ne2k_dns_poll_a`. La requête DNS A est construite dans un buffer caller-owned, émise en UDP vers le port 53 après résolution ARP, puis les réponses reçues sont filtrées sur les ports 53 vers 49152 et transmises au parseur DNS existant. Le nombre d’essais RX reste borné.

| Élément | État AOS-146 |
|---|---|
| DHCP REQUEST caller-owned | Implémenté. |
| Parsing DHCP ACK | Implémenté et borné. |
| Requête DNS A sur UDP | Implémentée. |
| Résolution ARP avant DNS | Réutilisée via `ne2k_tx_udp_resolve`. |
| Parsing DNS A RX | Implémenté via polling borné. |
| DHCP réel injecté sous QEMU | Non couvert par les smokes actuels. |
| Résolution DNS réelle | Non déclarée fonctionnelle avant smoke d’injection. |
| TCP, TLS, HTTP et OpenAI | Toujours non déclarés fonctionnels sur le transport matériel. |

La validation locale reste à **294 tests verts**, avec build i386 réussi.
