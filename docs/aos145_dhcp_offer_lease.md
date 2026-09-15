# AOS-145 — Offre DHCP reçue et état de bail caller-owned

Le lot AOS-145 ajoute `ne2k_dhcp_poll_offer`. Cette primitive effectue un polling RX borné, réutilise le parseur IPv4/UDP, filtre les datagrammes UDP source 67 vers destination 68, puis valide le XID et les options DHCP avant de publier une `net_dhcp_offer_t` caller-owned.

Un état `net_dhcp_lease_t` fixe permet ensuite d’appliquer ou d’effacer l’offre sans allocation. L’application copie l’adresse IPv4 proposée, l’adresse du serveur et le XID, puis publie le champ `valid`. Cette structure reste fournie et détenue par l’appelant ; aucune configuration globale de l’interface n’est modifiée automatiquement dans ce lot.

| Élément | État AOS-145 |
|---|---|
| Réception d’offre via RX NE2000 | Implémentée par polling borné. |
| Filtrage UDP DHCP 67 → 68 | Implémenté. |
| Validation XID et options | Réutilise `net_dhcp_parse_offer`. |
| État IPv4 caller-owned | Implémenté par `net_dhcp_lease_t`. |
| Application globale du bail au noyau | Non déclarée. |
| DHCP REQUEST/ACK et renouvellement | Prochains lots. |
| DNS sur transport matériel | Prochain groupe. |

La validation locale reste à **294 tests verts**, avec build i386 réussi. Les smokes QEMU existants valident le boot et la détection NE2000, mais n’injectent pas encore une offre DHCP réelle.
