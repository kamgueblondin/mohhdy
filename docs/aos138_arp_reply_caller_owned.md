# AOS-138 — Réponse ARP caller-owned

Le lot AOS-138 ajoute `net_arp_build_reply`. À partir d’une requête ARP Ethernet/IPv4 déjà décodée, la primitive construit dans un buffer fourni par l’appelant une réponse unicast : la destination Ethernet et la MAC cible deviennent l’émetteur de la requête, tandis que la MAC et l’IPv4 locales deviennent l’émetteur de la réponse. L’opcode passe de REQUEST à REPLY et tous les champs sont réécrits en big-endian ou copiés explicitement.

La fonction refuse les buffers trop courts, les requêtes d’un autre type matériel ou protocole, les tailles d’adresse non conformes et les paquets qui ne sont pas des requêtes ARP. Elle n’alloue aucune mémoire, ne conserve aucun pointeur vers la requête et retourne toujours la taille Ethernet/ARP de 42 octets en cas de succès.

| Élément | État AOS-138 |
|---|---|
| Construction réponse ARP Ethernet/IPv4 | Implémentée. |
| Destination unicast vers le demandeur | Implémentée. |
| Validation de la requête décodée | Implémentée. |
| Buffers caller-owned et sans allocation | Respectés. |
| Orchestration RX → réponse → TX | Non raccordée dans ce lot. |
| Smoke QEMU d’une requête ARP réelle | Non déclaré. |
| IPv4/UDP, DHCP, DNS, TCP, TLS, HTTP/OpenAI | Toujours non fonctionnels sur le transport matériel. |

La suite locale atteint **293 tests verts**, sans échec ni test ignoré, et le build i386 réussit.
